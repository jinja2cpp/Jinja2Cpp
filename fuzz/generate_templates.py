#!/usr/bin/env python3
"""Generates well-formed templates for the differential check (docs/tasks/0095, fuzz/README.md).

Byte mutants of a template are mostly rejected by both engines, which teaches the differential
check nothing. This generator writes templates from a small grammar over the context of
fuzz/differential.py (CONTEXT and SUPPORT_TEMPLATES, which mirror fuzz/fuzz_common.h): output
of typed expressions, filters, tests, if/elif/else, for with else, filters and loop variables,
set and block set, with, macros with defaults, varargs and caller, call blocks, filter blocks,
include, import, from-import and extends with block overrides. Expressions are built by type
(integer, number, string, list, mapping, boolean), so most templates render in Python too;
every template kept parses with Python Jinja2, so a difference in the output is a divergence,
not a rejected input.

    python3 fuzz/generate_templates.py OUT_DIR [--count 2000] [--seed 95]

Files are named by content hash, like the seed corpus (make_corpus.py). The same seed and count
write the same files.
"""
import argparse
import hashlib
import pathlib
import random
import sys

MAX_SIZE = 4096  # fuzz_common.h MaxInputSize
DEFAULT_SEED = 95


class Generator:
    def __init__(self, rng):
        self.rng = rng
        self.macros = []      # (name, takes_caller)
        self.ints = ["x", "neg", "foo", "bar"]
        self.strs = ["s", "html"]
        self.lists_int = ["l", "dup"]
        self.lists_str = ["words"]
        self.counter = 0

    def pick(self, *items):
        return self.rng.choice(items)

    def chance(self, p):
        return self.rng.random() < p

    def fresh(self, stem):
        self.counter += 1
        return f"{stem}{self.counter}"

    # Typed expressions: depth bounds the nesting

    def int_expr(self, depth=2):
        leaves = [str(self.rng.randint(-5, 12)), self.pick(*self.ints), "l|length", "l|sum", "l[1]", "d['a']",
                  "users[0].age", "words|length", "s|length", "e|length"]
        if depth <= 0 or self.chance(0.45):
            return self.pick(*leaves)
        d = depth - 1
        return self.pick(
            f"({self.int_expr(d)} {self.pick('+', '-', '*')} {self.int_expr(d)})",
            f"({self.int_expr(d)} // {self.rng.randint(1, 4)})",
            f"({self.int_expr(d)} % {self.rng.randint(2, 5)})",
            f"{self.int_expr(d)}|abs",
            f"[{self.int_expr(d)}, {self.int_expr(d)}]|max",
            f"{self.list_int_expr(d)}|sum",
            f"{self.list_int_expr(d)}|length",
            f"({self.int_expr(d)} if {self.bool_expr(d)} else {self.int_expr(d)})",
            f"{self.str_expr(d)}|length",
            f"'{self.rng.randint(0, 99)}'|int",
        )

    def num_expr(self, depth=2):
        if depth <= 0 or self.chance(0.4):
            return self.pick("y", "2.5", "0.1", "-1.25", "y * 2")
        d = depth - 1
        return self.pick(
            f"({self.int_expr(d)} / {self.rng.randint(1, 4)})",
            f"({self.num_expr(d)} + {self.int_expr(d)})",
            f"{self.num_expr(d)}|round({self.rng.randint(0, 2)})",
            f"{self.num_expr(d)}|round(1, 'floor')",
            f"{self.int_expr(d)}|float",
            f"{self.num_expr(d)}|int",
        )

    def str_literal(self):
        return repr(self.pick("a", "Hello", "x y z", "<i>&amp;</i>", "", " pad ", "it's", "été", "a,b,c"))

    def str_expr(self, depth=2):
        if depth <= 0 or self.chance(0.4):
            return self.pick(self.str_literal(), *self.strs, "words[0]", "users[1].name", "d|first")
        d = depth - 1
        return self.pick(
            f"({self.str_expr(d)} ~ {self.str_expr(d)})",
            f"{self.str_expr(d)}|{self.pick('upper', 'lower', 'title', 'capitalize', 'trim', 'string')}",
            f"{self.str_expr(d)}|replace({self.str_literal()}, {self.str_literal()})",
            f"{self.str_expr(d)}[{self.rng.randint(0, 3)}:{self.rng.randint(0, 8)}]",
            f"{self.int_expr(d)}|string",
            f"{self.list_str_expr(d)}|join({self.pick(repr(','), repr(' '), repr(''))})",
            f"{self.list_int_expr(d)}|join('-')",
            f"'%s-%s'|format({self.str_expr(d)}, {self.int_expr(d)})",
            f"({self.str_expr(d)} * {self.rng.randint(0, 3)})",
            f"{self.str_expr(d)}|center({self.rng.randint(0, 12)})",
            f"{self.str_expr(d)}|truncate({self.rng.randint(3, 12)}, true, '..', 0)",
            f"{self.str_expr(d)}|wordcount|string",
            f"{self.str_expr(d)}|e",
            f"{self.str_expr(d)}|default('dflt', true)",
            f"({self.str_expr(d)} if {self.bool_expr(d)} else {self.str_expr(d)})",
            f"undefined_name|default({self.str_expr(d)})",
            f"{self.str_expr(d)}|{self.pick('striptags', 'urlencode', 'forceescape', 'wordwrap(5)', 'indent(2)', 'indent(2, true)')}",
            f"{self.int_expr(d)}|filesizeformat",
            f"{self.list_str_expr(d)}|tojson",
            f"{self.list_int_expr(d)}|pprint",
            f"{self.int_expr(d)}|string|reverse",
            f"{self.list_str_expr(d)}|first|default('')",
            f"'{{:>5}}'.format({self.str_expr(d)})",
            f"({self.str_expr(d)}).{self.pick('upper()', 'strip()', 'title()', 'capitalize()', 'swapcase()')}",
            f"{self.str_expr(d)}|replace({self.str_literal()}, {self.str_literal()}, {self.rng.randint(0, 2)})",
        )

    def list_int_expr(self, depth=2):
        if depth <= 0 or self.chance(0.4):
            return self.pick(*self.lists_int, f"range({self.rng.randint(0, 6)})|list", "e", "[3, 1, 2]")
        d = depth - 1
        return self.pick(
            f"{self.list_int_expr(d)}|sort",
            f"{self.list_int_expr(d)}|sort(reverse=true)",
            f"{self.list_int_expr(d)}|reverse|list",
            f"{self.list_int_expr(d)}|unique|list",
            f"({self.list_int_expr(d)} + {self.list_int_expr(d)})",
            f"[{self.int_expr(d)}, {self.int_expr(d)}]",
            f"{self.list_int_expr(d)}|select('odd')|list",
            f"{self.list_int_expr(d)}|reject('even')|list",
            f"{self.list_int_expr(d)}[{self.rng.randint(0, 2)}:]",
            f"users|map(attribute='age')|list",
            f"range({self.rng.randint(0, 3)}, {self.rng.randint(3, 9)}, {self.rng.randint(1, 3)})|list",
            f"{self.list_int_expr(d)}|batch({self.rng.randint(1, 3)})|map('sum')|list",
            f"{self.list_int_expr(d)}|slice({self.rng.randint(1, 3)})|map('length')|list",
            f"[{self.list_int_expr(d)}|min|default(0), {self.list_int_expr(d)}|max|default(0)]",
        )

    def list_str_expr(self, depth=2):
        if depth <= 0 or self.chance(0.4):
            return self.pick(*self.lists_str, "users|map(attribute='name')|list", "d|list", "['p', 'q']")
        d = depth - 1
        return self.pick(
            f"{self.list_str_expr(d)}|sort",
            f"{self.list_str_expr(d)}|sort(case_sensitive=true)",
            f"{self.list_str_expr(d)}|map('upper')|list",
            f"{self.list_int_expr(d)}|map('string')|list",
            f"({self.str_expr(d)}).split()",
            f"{self.str_expr(d)}|list",
            f"({self.list_str_expr(d)} + [{self.str_expr(d)}])",
        )

    def bool_expr(self, depth=2):
        if depth <= 0 or self.chance(0.4):
            return self.pick("t", "not t", "n is none", "x is odd", "l is sequence", "missing is defined", "d is mapping")
        d = depth - 1
        return self.pick(
            f"({self.int_expr(d)} {self.pick('<', '>', '==', '!=', '<=', '>=')} {self.int_expr(d)})",
            f"({self.bool_expr(d)} {self.pick('and', 'or')} {self.bool_expr(d)})",
            f"not {self.bool_expr(d)}",
            f"({self.str_expr(d)} in {self.list_str_expr(d)})",
            f"({self.int_expr(d)} in {self.list_int_expr(d)})",
            f"{self.int_expr(d)} is {self.pick('even', 'odd', 'number', 'integer')}",
            f"{self.str_expr(d)} is {self.pick('string', 'lower', 'upper')}",
            f"({self.str_expr(d)}).startswith({self.str_literal()})",
        )

    def any_expr(self, depth=2):
        kind = self.rng.randrange(8)
        if kind == 0:
            return self.int_expr(depth)
        if kind == 1:
            return self.num_expr(depth)
        if kind in (2, 3):
            return self.str_expr(depth)
        if kind == 4:
            return self.list_int_expr(depth)
        if kind == 5:
            return self.list_str_expr(depth)
        if kind == 6:
            return self.bool_expr(depth)
        return self.pick("d|dictsort", "users|selectattr('age', 'gt', 26)|map(attribute='name')|list", "d.items()|list",
                         "users|groupby('age')|map(attribute='grouper')|list", "users|sum(attribute='age')",
                         "d|dictsort(by='value')", "{'k': " + self.int_expr(depth - 1) + "}", "n",
                         "users|min(attribute='age')", "users|map(attribute='city', default='-')|list",
                         "users|rejectattr('city')|map(attribute='name')|list", "d|xmlattr", "users|first|items|list",
                         "d|tojson", "words|sort(attribute='0')", "(" + self.int_expr(depth - 1) + ", " + self.str_expr(depth - 1) + ")")

    # Statements: `budget` bounds the size of the template

    def ws(self, tag):
        # Whitespace control on a tenth of the tags
        if self.chance(0.1):
            return tag[:2] + "-" + tag[2:-2] + "-" + tag[-2:]
        return tag

    def text(self):
        return self.pick("", "a", " ", "\n", " | ", "<p>", "  text  ", "\n  ")

    def output(self, depth=2):
        return self.ws("{{ " + self.any_expr(depth) + " }}")

    def body(self, budget, scope):
        parts = []
        while budget > 0:
            budget -= 1
            parts.append(self.statement(budget, scope))
            if self.chance(0.3):
                break
        return "".join(parts)

    def statement(self, budget, scope):
        choice = self.rng.randrange(15 if budget > 0 else 3)
        if choice == 0:
            return self.text()
        if choice in (1, 2):
            return self.output()
        if choice == 3:
            text = self.ws("{% if " + self.bool_expr() + " %}") + self.body(budget - 1, scope)
            while self.chance(0.3):
                text += self.ws("{% elif " + self.bool_expr() + " %}") + self.body(budget - 1, scope)
            if self.chance(0.5):
                text += self.ws("{% else %}") + self.body(budget - 1, scope)
            return text + self.ws("{% endif %}")
        if choice == 4:
            return self.for_loop(budget, scope)
        if choice == 5:
            name = self.fresh("v")
            text = self.ws("{% set " + name + " = " + self.int_expr() + " %}")
            self.ints.append(name)
            return text
        if choice == 6:
            name = self.fresh("cap")
            text = "{% set " + name + (" | " + self.pick("upper", "trim", "e") if self.chance(0.4) else "") + " %}"
            text += self.body(budget - 1, scope) + "{% endset %}"
            self.strs.append(name)
            return text
        if choice == 7:
            name = self.fresh("w")
            text = "{% with " + name + " = " + self.str_expr() + " %}"
            self.strs.append(name)
            text += self.body(budget - 1, scope) + "{% endwith %}"
            self.strs.remove(name)
            return text
        if choice == 8:
            return self.macro(budget, scope)
        if choice == 9 and self.macros:
            name, takes_caller = self.rng.choice(self.macros)
            if takes_caller:
                return "{% call " + name + "(" + self.str_expr() + ") %}" + self.body(budget - 1, scope) + "{% endcall %}"
            return self.ws("{{ " + name + "(" + self.str_expr() + (", " + self.int_expr() if self.chance(0.5) else "") + ") }}")
        if choice == 10:
            return "{% filter " + self.pick("upper", "lower|trim", "title", "e", "replace('a', 'b')") + " %}" \
                + self.body(budget - 1, scope) + "{% endfilter %}"
        if choice == 11:
            return self.pick("{% include 'header' %}", "{% include ['missing', 'header'] %}",
                             "{% include 'missing' ignore missing %}",
                             "{% import 'macros' as mac %}{{ mac.m(" + self.str_expr() + ") }}{{ mac.v }}",
                             "{% from 'macros' import m %}{{ m(" + self.int_expr() + ", " + self.str_expr() + ") }}",
                             "{% from 'macros' import m %}{% call m(1) %}" + self.str_literal() + "{% endcall %}")
        if choice == 14:
            ns = self.fresh("ns")
            return ("{% set " + ns + " = namespace(c=0, s='') %}{% for i in " + self.list_int_expr() + " %}"
                    + self.pick("", "{% if i > 2 %}{% break %}{% endif %}", "{% if i is odd %}{% continue %}{% endif %}")
                    + "{% set " + ns + ".c = " + ns + ".c + i %}{% set " + ns + ".s = " + ns + ".s ~ loop.changed(i) %}"
                    + "{% endfor %}{{ " + ns + ".c }}{{ " + ns + ".s }}")
        if choice == 12:
            return "{# " + self.pick("note", "{{ x }}", "") + " #}"
        return "{% raw %}" + self.pick("{{ x }}", "{% if %}", "a") + "{% endraw %}"

    def for_loop(self, budget, scope):
        kind = self.rng.randrange(4)
        if kind == 0:
            var, head = self.fresh("i"), self.list_int_expr()
            self.ints.append(var)
            added = (self.ints, var)
        elif kind == 1:
            var, head = self.fresh("w"), self.list_str_expr()
            self.strs.append(var)
            added = (self.strs, var)
        elif kind == 2:
            k, v = self.fresh("k"), self.fresh("v")
            var, head = f"{k}, {v}", "d|dictsort"
            self.strs.append(k)
            self.ints.append(v)
            added = None
        else:
            var, head = self.fresh("u"), "users"
            added = None
        condition = " if " + self.bool_expr(1) if self.chance(0.2) else ""
        inner = self.body(budget - 1, scope + 1)
        if self.chance(0.5):
            inner += self.ws("{{ " + self.pick("loop.index", "loop.index0", "loop.first", "loop.last", "loop.length",
                                               "loop.revindex", "loop.cycle('a', 'b')", "loop.previtem|default('-')") + " }}")
        if kind == 3:
            inner += "{{ " + var + ".name }}:{{ " + var + ".age }}" + ("{{ " + var + ".city|default('?') }}" if self.chance(0.5) else "")
        text = self.ws("{% for " + var + " in " + head + condition + " %}") + inner
        if self.chance(0.3):
            text += "{% else %}" + self.body(budget - 1, scope)
        text += self.ws("{% endfor %}")
        if added:
            added[0].remove(added[1])
        elif kind == 2:
            self.strs.remove(k)
            self.ints.remove(v)
        return text

    def macro(self, budget, scope):
        name = self.fresh("mc")
        takes_caller = self.chance(0.3)
        uses_varargs = self.chance(0.2)
        self.strs.append("a")
        body = self.body(budget - 1, scope)
        self.strs.remove("a")
        body += "{{ a }}-{{ b }}"
        if takes_caller:
            body += "[{{ caller() }}]"
        if uses_varargs:
            body += "{{ varargs|join(',') }}{{ kwargs|dictsort }}"
        text = "{% macro " + name + "(a, b=" + str(self.rng.randint(0, 9)) + ") %}" + body + "{% endmacro %}"
        self.macros.append((name, takes_caller))
        if not takes_caller and self.chance(0.6):
            extra = ", 1, 2, k=3" if uses_varargs else ""
            text += "{{ " + name + "(" + self.str_expr() + extra + ") }}"
        return text

    def template(self, budget):
        if self.chance(0.15):
            # A child of `base`: only its blocks render
            blocks = "".join("{% block " + name + " %}" + self.pick("", "{{ super() }}") + self.body(budget // 2, 0) + "{% endblock %}"
                             for name in self.rng.sample(["content", "other"], self.rng.randint(1, 2)))
            return "{% extends 'base' %}" + blocks
        return self.body(budget, 0) + "".join(self.statement(budget // 2, 0) for _ in range(self.rng.randint(0, 3)))


def generate(count, seed, parses):
    """Yields `count` templates that `parses` accepts"""
    rng = random.Random(seed)
    made = 0
    while made < count:
        source = Generator(rng).template(rng.randint(2, 8))
        if not source or len(source.encode("utf-8")) > MAX_SIZE or not parses(source):
            continue
        made += 1
        yield source


def python_parser():
    import jinja2

    env = jinja2.Environment(extensions=["jinja2.ext.do", "jinja2.ext.loopcontrols", "jinja2.ext.i18n"])

    def parses(source):
        try:
            env.parse(source)
            return True
        except Exception:  # noqa: BLE001 - any rejection drops the template
            return False

    return parses


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("out", type=pathlib.Path, help="directory to write the templates to (created if missing)")
    parser.add_argument("--count", type=int, default=2000)
    parser.add_argument("--seed", type=int, default=DEFAULT_SEED)
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    added = 0
    for source in generate(args.count, args.seed, python_parser()):
        data = source.encode("utf-8")
        target = args.out / hashlib.sha1(data).hexdigest()
        if not target.exists():
            target.write_bytes(data)
            added += 1
    print(f"{args.count} generated templates, {added} new files in {args.out}", file=sys.stderr)


if __name__ == "__main__":
    main()
