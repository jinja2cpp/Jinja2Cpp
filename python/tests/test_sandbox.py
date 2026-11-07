"""The sandbox (jinja2cpp.sandbox, docs/python.md): Jinja2's own security tests, and the same
templates rendered by Jinja2's sandbox as the oracle.

TestSandbox, TestStringFormat and TestStringFormatMap are the cases of Jinja2 3.1.6's
tests/test_security.py (BSD-3-Clause, the Pallets Projects), with the same names; they differ
only in importing jinja2cpp, in the cases that need compiled-code internals (documented below)
and in the helper classes being module level. The rest compares the two sandboxes.
"""

import collections
import copy
import re

import jinja2
import jinja2.sandbox as j2sandbox
import jinja2cpp
import pytest
from jinja2cpp.sandbox import ImmutableSandboxedEnvironment
from jinja2cpp.sandbox import SandboxedEnvironment
from jinja2cpp.sandbox import unsafe
from markupsafe import Markup
from markupsafe import escape

from jinja2cpp import Environment
from jinja2cpp.exceptions import SecurityError
from jinja2cpp.exceptions import TemplateRuntimeError
from jinja2cpp.exceptions import TemplateSyntaxError


class PrivateStuff:
    def bar(self):
        return 23

    @unsafe
    def foo(self):
        return 42

    def __repr__(self):
        return "PrivateStuff"


class PublicStuff:
    def bar(self):
        return 23

    def _foo(self):
        return 42

    def __repr__(self):
        return "PublicStuff"


class TestSandbox:
    def test_unsafe(self):
        env = SandboxedEnvironment()
        pytest.raises(SecurityError, env.from_string("{{ foo.foo() }}").render, foo=PrivateStuff())
        assert env.from_string("{{ foo.bar() }}").render(foo=PrivateStuff()) == "23"

        pytest.raises(SecurityError, env.from_string("{{ foo._foo() }}").render, foo=PublicStuff())
        assert env.from_string("{{ foo.bar() }}").render(foo=PublicStuff()) == "23"
        assert env.from_string("{{ foo.__class__ }}").render(foo=42) == ""
        assert env.from_string("{{ foo.func_code }}").render(foo=lambda: None) == ""
        # security error comes from __class__ already.
        pytest.raises(
            SecurityError,
            env.from_string("{{ foo.__class__.__subclasses__() }}").render,
            foo=42,
        )

    def test_immutable_environment(self):
        env = ImmutableSandboxedEnvironment()
        pytest.raises(SecurityError, env.from_string("{{ [].append(23) }}").render)
        pytest.raises(SecurityError, env.from_string("{{ [].clear() }}").render)
        pytest.raises(SecurityError, env.from_string("{{ [1].pop() }}").render)
        pytest.raises(SecurityError, env.from_string("{{ {1:2}.clear() }}").render)

    def test_restricted(self):
        env = SandboxedEnvironment()
        pytest.raises(
            TemplateSyntaxError,
            env.from_string,
            "{% for item.attribute in seq %}...{% endfor %}",
        )
        pytest.raises(
            TemplateSyntaxError,
            env.from_string,
            "{% for foo, bar.baz in seq %}...{% endfor %}",
        )

    def test_template_data(self):
        # Jinja2's case also calls say_hello(EvalContext(env), ...): jinja2cpp has no jinja2.nodes
        env = Environment(autoescape=True)
        t = env.from_string("{% macro say_hello(name) %}<p>Hello {{ name }}!</p>{% endmacro %}" '{{ say_hello("<blink>foo</blink>") }}')
        escaped_out = "<p>Hello &lt;blink&gt;foo&lt;/blink&gt;!</p>"
        assert t.render() == escaped_out
        assert str(t.module) == escaped_out
        assert escape(t.module) == escaped_out
        assert t.module.say_hello("<blink>foo</blink>") == escaped_out
        assert escape(t.module.say_hello("<blink>foo</blink>")) == escaped_out

    def test_attr_filter(self):
        env = SandboxedEnvironment()
        tmpl = env.from_string('{{ cls|attr("__subclasses__")() }}')
        pytest.raises(SecurityError, tmpl.render, cls=int)

    @pytest.mark.xfail(reason="intercepted operators are not supported: UnsupportedError (docs/python.md, task 0412)", raises=jinja2cpp.UnsupportedError, strict=True)
    def test_binary_operator_intercepting(self):
        def disable_op(left, right):
            raise TemplateRuntimeError("that operator so does not work")

        for expr, ctx, rv in ("1 + 2", {}, "3"), ("a + 2", {"a": 2}, "4"):
            env = SandboxedEnvironment()
            env.binop_table["+"] = disable_op
            t = env.from_string(f"{{{{ {expr} }}}}")
            assert t.render(ctx) == rv
            env.intercepted_binops = frozenset(["+"])
            t = env.from_string(f"{{{{ {expr} }}}}")
            with pytest.raises(TemplateRuntimeError):
                t.render(ctx)

    @pytest.mark.xfail(reason="intercepted operators are not supported: UnsupportedError (docs/python.md, task 0412)", raises=jinja2cpp.UnsupportedError, strict=True)
    def test_unary_operator_intercepting(self):
        def disable_op(arg):
            raise TemplateRuntimeError("that operator so does not work")

        for expr, ctx, rv in ("-1", {}, "-1"), ("-a", {"a": 2}, "-2"):
            env = SandboxedEnvironment()
            env.unop_table["-"] = disable_op
            t = env.from_string(f"{{{{ {expr} }}}}")
            assert t.render(ctx) == rv
            env.intercepted_unops = frozenset(["-"])
            t = env.from_string(f"{{{{ {expr} }}}}")
            with pytest.raises(TemplateRuntimeError):
                t.render(ctx)


class TestStringFormat:
    def test_basic_format_safety(self):
        env = SandboxedEnvironment()
        t = env.from_string('{{ "a{0.__class__}b".format(42) }}')
        assert t.render() == "ab"

    def test_basic_format_all_okay(self):
        env = SandboxedEnvironment()
        t = env.from_string('{{ "a{0.foo}b".format({"foo": 42}) }}')
        assert t.render() == "a42b"

    def test_safe_format_safety(self):
        env = SandboxedEnvironment()
        t = env.from_string('{{ ("a{0.__class__}b{1}"|safe).format(42, "<foo>") }}')
        assert t.render() == "ab&lt;foo&gt;"

    def test_safe_format_all_okay(self):
        env = SandboxedEnvironment()
        t = env.from_string('{{ ("a{0.foo}b{1}"|safe).format({"foo": 42}, "<foo>") }}')
        assert t.render() == "a42b&lt;foo&gt;"

    def test_empty_braces_format(self):
        env = SandboxedEnvironment()
        t1 = env.from_string('{{ ("a{}b{}").format("foo", "42")}}')
        t2 = env.from_string('{{ ("a{}b{}"|safe).format(42, "<foo>") }}')
        assert t1.render() == "afoob42"
        assert t2.render() == "a42b&lt;foo&gt;"


class TestStringFormatMap:
    def test_basic_format_safety(self):
        env = SandboxedEnvironment()
        t = env.from_string('{{ "a{x.__class__}b".format_map({"x":42}) }}')
        assert t.render() == "ab"

    def test_basic_format_all_okay(self):
        env = SandboxedEnvironment()
        t = env.from_string('{{ "a{x.foo}b".format_map({"x":{"foo": 42}}) }}')
        assert t.render() == "a42b"

    def test_safe_format_all_okay(self):
        env = SandboxedEnvironment()
        t = env.from_string('{{ ("a{x.foo}b{y}"|safe).format_map({"x":{"foo": 42}, "y":"<foo>"}) }}')
        assert t.render() == "a42b&lt;foo&gt;"

    def test_indirect_call(self):
        def run(value, arg):
            return value.run(arg)

        env = SandboxedEnvironment()
        env.filters["run"] = run
        t = env.from_string(
            """{% set
                ns = namespace(run="{0.__call__.__builtins__[__import__]}".format)
            %}
            {{ ns | run(not_here) }}
            """
        )

        with pytest.raises(SecurityError):
            t.render()

    def test_attr_filter(self) -> None:
        env = SandboxedEnvironment()
        t = env.from_string(
            """{{ "{0.__call__.__builtins__[__import__]}"
                  | attr("format")(not_here) }}"""
        )

        with pytest.raises(SecurityError):
            t.render()


# ---------------------------------------------------------------------------------------------
# The attribute-path sweep: attribute and item reads and calls on Python objects and on the
# engine's own values, rendered by both sandboxes under both undefined policies. The outcome is
# the output or the exception class (and its message for a SecurityError or an OverflowError); a memory address in an output is dropped.


class Obj:
    pub = 1
    _priv = 2

    def __init__(self):
        self.d = {"k": 1}
        self.items_ = [3, 1, 2]

    def meth(self):
        return 3

    @property
    def prop(self):
        return 4


class Item:
    def __init__(self, n, g):
        self.name = n
        self.group = g
        self._secret = n * 2


def _gen():
    yield 1


def _context():
    return dict(
        o=Obj(),
        n=42,
        s="text",
        lst=[3, 1, 2],
        d={"a": 1, "b": 2},
        t=(1, 2),
        f=lambda x: x,
        g=_gen(),
        cls=int,
        mod=collections,
        fn=Obj().meth,
        items=[Item("x", 1), Item("y", 1), Item("z", 2)],
        dq=collections.deque([1]),
        st={1, 2},
        md={"k": Obj()},
        none=None,
        flt=1.5,
        mk=Markup("<b>"),
    )


SWEEP = [
    'o.pub',
    'o._priv',
    'o.__class__',
    'o.__dict__',
    'o.__class__.__name__',
    'o.__init__',
    'o.meth',
    'o.meth()',
    'o.meth.__self__',
    'o.meth.__func__',
    'o.meth.__globals__',
    'o.prop',
    'o.d.k',
    'o.d.items()',
    'o["pub"]',
    'o["__class__"]',
    'o["_priv"]',
    'o|attr("pub")',
    'o|attr("__class__")',
    'o|attr("_priv")',
    'o|attr("__dict__")',
    'n.__class__',
    'n.real',
    'n.__add__',
    's.__class__',
    's.upper()',
    's.__len__',
    's.__doc__',
    '"{0.__class__}".format(o)',
    '"{0._priv}".format(o)',
    '"{0.pub}{1}".format(o, n)',
    '"{0[__class__]}".format(o)',
    '"{0.d[k]}".format(o)',
    '"{x.__class__}".format_map({"x": o})',
    '"{x.pub}".format_map({"x": o})',
    's.__class__.__mro__',
    'lst.__class__',
    'lst.append',
    'lst.append(4)',
    'lst.sort()',
    'lst.__len__()',
    'lst.index(1)',
    'd.items()',
    'd.update',
    'd.update({"c":3})',
    'd.__class__',
    'd.__getitem__',
    'd.a',
    'd.get("a")',
    't.__class__',
    't.count(1)',
    'dq.append(1)',
    'dq.__class__',
    'dq.popleft',
    'st.add',
    'st.__class__',
    'cls.__subclasses__',
    'cls.mro',
    'cls.__name__',
    'cls.__mro__',
    'mod.OrderedDict',
    'mod.__name__',
    'mod.__dict__',
    'f.__code__',
    'f.__globals__',
    'f.__closure__',
    'f.__name__',
    'f()',
    'f.func_code',
    'g.gi_frame',
    'g.gi_code',
    'g.__next__',
    'g|list',
    'fn.__self__',
    'fn.__func__',
    'fn()',
    'items|map(attribute="name")|list',
    'items|map(attribute="_secret")|list',
    'items|map(attribute="__class__")|list',
    'items|sort(attribute="name")|map(attribute="name")|list',
    'items|sort(attribute="_secret")|map(attribute="name")|list',
    'items|groupby("group")|map(attribute="grouper")|list',
    'items|groupby("_secret")|list',
    'items|selectattr("group")|map(attribute="name")|list',
    'items|selectattr("_secret")|list',
    'items|rejectattr("_secret")|list',
    'items|sum(attribute="group")',
    'items|sum(attribute="_secret")',
    'items|unique(attribute="group")|list|length',
    'items|unique(attribute="_secret")|list|length',
    'items|min(attribute="_secret")',
    'items|max(attribute="group")|attr("name")',
    'md.k.pub',
    'md.k._priv',
    'md["k"].__class__',
    'none.__class__',
    'flt.__class__',
    'mk.__class__',
    'mk.upper()',
    '("{0.__class__}"|safe).format(o)',
    'range.__self__',
    'range(3)|list',
    'range(100001)|list',
    'range(0, 200000, 2)|list|length',
    'range(10**6)|list|length',
    'cycler.__init__',
    'cycler("a")',
    'cycler("a").__class__',
    'cycler("a").__init__',
    'cycler("a")._priv',
    'namespace.__init__',
    'namespace().__class__',
    'namespace(a=1).a',
    'joiner.__class__',
    'dict.__class__',
    '{}.__class__',
    '{}.update',
    '[].__class__',
    '[].append',
    '().__class__',
    '"".__class__',
    '"".join',
    '1.__class__',
    'none.__bool__',
    'loop',
    '{% for i in [1] %}{{ loop.__class__ }}{% endfor %}',
    '{% for i in [1] %}{{ loop.index }}{% endfor %}',
    '{% for i in [1] %}{{ loop._x }}{% endfor %}',
    '{% for i in [1] %}{{ loop.__init__ }}{% endfor %}',
    '{% macro m() %}x{% endmacro %}{{ m.__class__ }}',
    '{% macro m() %}x{% endmacro %}{{ m.__globals__ }}',
    '{% macro m() %}x{% endmacro %}{{ m.name }}',
    '{% macro m() %}{{ caller.__class__ }}{% endmacro %}{% call m() %}{% endcall %}',
    '{% macro m() %}{{ varargs.__class__ }}{% endmacro %}{{ m() }}',
    '{{ self.__class__ }}',
    '{{ self.__init__ }}',
    '{{ self._TemplateReference__context }}',
    '{{ lipsum.__class__ }}',
    '{{ lipsum.__globals__ }}',
    '{{ config }}',
    "{{ ''.__class__.__mro__[1].__subclasses__() }}",
    '{{ o.__class__.__init__.__globals__ }}',
    '{{ cls.__subclasses__() }}',
    '{{ [].__class__.__base__.__subclasses__() }}',
    '{% set x = o %}{{ x.__class__ }}',
    '{% set x = o.meth %}{{ x() }}',
    '{% set x = o.__class__ %}{{ x }}',
    '{% with a = o %}{{ a.__class__ }}{% endwith %}',
    '{% for a in [o] %}{{ a.__class__ }}{% endfor %}',
    '{% for a in [o] %}{{ a._priv }}{% endfor %}',
    '{% for a in items %}{{ a._secret }}{% endfor %}',
    '{{ o|string }}',
    '{{ o|e }}',
    '{{ o|tojson if False else 1 }}',
    '{{ o.pub is defined }}',
    '{{ o._priv is defined }}',
    '{{ o.__class__ is defined }}',
    '{{ o.__class__ is none }}',
    '{{ o._priv|default("dflt") }}',
    '{{ o.__class__|default("dflt") }}',
    '{{ (o.__class__)|length }}',
    '{{ o.__class__ ~ "x" }}',
    '{{ o.__class__ == 1 }}',
    '{{ o.__class__ and 1 }}',
    '{{ o.__class__ + 1 }}',
    '{{ -o.__class__ }}',
    '{{ o.__class__ in [1] }}',
    '{{ [1] | join(o.__class__) }}',
    '{{ o.__class__ | upper }}',
    '{{ o.__class__ is string }}',
    '{{ o.__class__ | list }}',
    '{% for x in o.__class__ %}a{% endfor %}',
    '{% if o.__class__ %}y{% else %}n{% endif %}',
    '{{ o.__class__.x }}',
    '{{ o.__class__() }}',
    '{{ o.__class__[0] }}',
    '{{ o.__class__|int }}',
    '{{ o.__class__ | default("d", true) }}',
    '{{ o|attr("meth")() }}',
    '{{ o|attr("__init__") }}',
    '{{ cls|attr("__subclasses__")() }}',
    '{{ lst|attr("append") }}',
    '{{ s|attr("upper")() }}',
    '{{ s|attr("__class__") }}',
    '{{ n|attr("real") }}',
    '{{ d|attr("items")() }}',
    '{{ d|attr("__class__") }}',
    '{{ d|attr("a") }}',
    '{{ none|attr("__class__") }}',
]

# Under StrictUndefined, a private name an object of the engine lacks (loop._x, cycler()._priv, a
# macro's __globals__) is refused by jinja2cpp, with SecurityError, where Jinja2 reports a plain
# UndefinedError; and sort/unique of an unsafe attribute do not fail as Jinja2's do (the engine
# does not raise for a strict undefined sort key, with or without the sandbox)
STRICT_ONLY = {
    'items|sort(attribute="_secret")|map(attribute="name")|list',
    'items|unique(attribute="_secret")|list|length',
    "range.__self__",
    'cycler("a")._priv',
    "{% for i in [1] %}{{ loop._x }}{% endfor %}",
    "{% macro m() %}x{% endmacro %}{{ m.__globals__ }}",
}


# The class named in the message differs: the engine's cycler, joiner, namespace and dict are
# functions to it, Jinja2's are classes ('type'); and an attribute read from the unsafe undefined of
# an earlier attribute (o.__class__.__init__) is named after that undefined by Jinja2, whose
# Undefined has a real __init__, and after the first object by jinja2cpp
MESSAGE_DIFFERS = {
    "cycler.__init__",
    "namespace.__init__",
    "joiner.__class__",
    "dict.__class__",
    "{{ o.__class__.__init__.__globals__ }}",
}


def _outcome(env_class, source, undefined):
    template = source if "{{" in source or "{%" in source else "{{ " + source + " }}"
    try:
        env = env_class(undefined=undefined, extensions=["jinja2.ext.do"])
        return ("ok", re.sub(r" at 0x[0-9a-f]+", "", env.from_string(template).render(**_context())))
    except Exception as error:  # noqa: BLE001 - compared by class name
        # The sandbox's messages are part of the contract; the others differ in wording (docs/python.md)
        return ("exception", type(error).__name__, str(error) if type(error).__name__ in ("SecurityError", "OverflowError") and source not in MESSAGE_DIFFERS else "")


@pytest.mark.parametrize("strict", [False, True], ids=["default", "strict"])
@pytest.mark.parametrize("immutable", [False, True], ids=["sandbox", "immutable"])
@pytest.mark.parametrize("source", SWEEP)
def test_attribute_path_sweep(source, immutable, strict):
    if strict and source in STRICT_ONLY:
        pytest.skip("documented difference (docs/python.md, Sandbox)")
    reference, mine = (j2sandbox.ImmutableSandboxedEnvironment, ImmutableSandboxedEnvironment) if immutable else (j2sandbox.SandboxedEnvironment, SandboxedEnvironment)
    expected = _outcome(reference, source, jinja2.StrictUndefined if strict else jinja2.Undefined)
    got = _outcome(mine, source, jinja2cpp.StrictUndefined if strict else jinja2cpp.Undefined)
    assert got == expected, source


# ---------------------------------------------------------------------------------------------
# The rest of the contract: the policy hooks, range, calls, the scope of a sandbox, and its API


def _both(source, context=None, immutable=False, **kwargs):
    """(jinja2cpp's outcome, Jinja2's) of a template in the sandbox of each."""
    ours = ImmutableSandboxedEnvironment if immutable else SandboxedEnvironment
    theirs = j2sandbox.ImmutableSandboxedEnvironment if immutable else j2sandbox.SandboxedEnvironment
    outcomes = []
    for env_class in (ours, theirs):
        try:
            outcomes.append(("ok", env_class(**kwargs).from_string(source).render(**copy.deepcopy(context or {}))))
        except Exception as error:  # noqa: BLE001
            outcomes.append(("exception", type(error).__name__, str(error)))
    return tuple(outcomes)


class Dangerous:
    def go(self):
        return "went"

    go.alters_data = True

    def fine(self):
        return "fine"

    def __repr__(self):
        return "Dangerous"


def test_the_api_of_jinja2_sandbox_is_there():
    import jinja2cpp.sandbox as sandbox

    for name in dir(j2sandbox):
        if name.startswith("__") or name in ("abc", "types", "t", "operator", "update_wrapper", "Formatter", "Environment", "EscapeFormatter", "Markup", "Context", "Undefined", "F", "deque", "formatter_field_name_split"):
            continue
        assert hasattr(sandbox, name), name
    assert issubclass(jinja2cpp.SecurityError, j2sandbox.SecurityError)
    assert sandbox.SecurityError is jinja2cpp.SecurityError
    assert sandbox.MAX_RANGE == j2sandbox.MAX_RANGE == 100000
    assert sandbox.UNSAFE_GENERATOR_ATTRIBUTES == j2sandbox.UNSAFE_GENERATOR_ATTRIBUTES
    assert issubclass(SandboxedEnvironment, Environment) and issubclass(ImmutableSandboxedEnvironment, SandboxedEnvironment)
    assert SandboxedEnvironment.sandboxed and not Environment.sandboxed and not Environment().sandboxed


def test_policy_functions_agree_with_jinja2():
    import jinja2cpp.sandbox as sandbox

    samples = [int, str, lambda: 0, Obj().meth, _gen(), {}, [], set(), collections.deque(), Obj(), 1, "s"]
    for obj in samples:
        for attr in ("mro", "gi_frame", "gi_code", "__class__", "x", "_x", "append", "clear", "add", "keys", "popleft", "update", "setdefault"):
            assert sandbox.is_internal_attribute(obj, attr) == j2sandbox.is_internal_attribute(obj, attr)
            assert sandbox.modifies_known_mutable(obj, attr) == j2sandbox.modifies_known_mutable(obj, attr)


def test_doctests_of_the_policy_functions():
    import doctest

    import jinja2cpp.sandbox as sandbox

    assert doctest.testmod(sandbox).failed == 0


def test_range_is_capped_like_safe_range():
    from jinja2cpp.sandbox import MAX_RANGE
    from jinja2cpp.sandbox import safe_range

    assert SandboxedEnvironment().globals["range"] is safe_range
    for source in (
        "{{ range(100000)|length }}",
        "{{ range(100001)|length }}",
        "{{ range(0, 200000, 2)|length }}",
        "{{ range(10)|list }}",
        "{{ range(5, 1, -1)|list }}",
        "{{ range(10**9)|length }}",
        "{% for i in range(100001) %}{% endfor %}",
        "{% for i in range(100000) %}{% if loop.last %}{{ i }}{% endif %}{% endfor %}",
    ):
        for immutable in (False, True):
            ours, theirs = _both(source, immutable=immutable)
            assert ours == theirs, source
    with pytest.raises(OverflowError, match="MAX_RANGE"):
        safe_range(MAX_RANGE + 1)
    # A plain environment has no cap
    assert Environment().from_string("{{ range(100001)|length }}").render() == "100001"
    # ... and the cap of a sandbox does not leak into the next render of another environment
    sandbox = SandboxedEnvironment()
    plain = Environment()
    with pytest.raises(OverflowError):
        sandbox.from_string("{{ range(100001)|length }}").render()
    assert plain.from_string("{{ range(100001)|length }}").render() == "100001"


def test_a_range_global_the_user_replaced_is_used():
    env = SandboxedEnvironment()
    env.globals["range"] = lambda *a: [7]
    assert env.from_string("{{ range(1000000)|list }}").render() == "[7]"


def test_is_safe_attribute_override_is_the_one_checkpoint():
    seen = []

    class Picky(SandboxedEnvironment):
        def is_safe_attribute(self, obj, attr, value):
            seen.append(attr)
            return attr != "pub" and super().is_safe_attribute(obj, attr, value)

    class PickyJ2(j2sandbox.SandboxedEnvironment):
        def is_safe_attribute(self, obj, attr, value):
            return attr != "pub" and super().is_safe_attribute(obj, attr, value)

    sources = [
        "{{ o.pub }}",
        "{{ o['pub'] }}",
        "{{ o|attr('pub') }}",
        "{{ [o]|map(attribute='pub')|list }}",
        "{{ [o]|sort(attribute='pub')|map(attribute='meth')|list|length }}",
        "{{ [o]|sum(attribute='pub') }}",
        "{{ [o]|groupby('pub')|list|length }}",
        "{{ [o]|unique(attribute='pub')|list|length }}",
        "{{ [o]|selectattr('pub')|list|length }}",
        "{{ [o]|rejectattr('pub')|list|length }}",
        "{{ [o]|min(attribute='pub') is defined }}",
        "{{ '{0.pub}'.format(o) }}",
        "{{ '{0[pub]}'.format(o) }}",
        "{{ o.d.k }}",
        "{{ o.meth() }}",
    ]
    for source in sources:
        ours = theirs = None
        try:
            ours = Picky().from_string(source).render(o=Obj())
        except Exception as error:  # noqa: BLE001
            ours = type(error).__name__
        try:
            theirs = PickyJ2().from_string(source).render(o=Obj())
        except Exception as error:  # noqa: BLE001
            theirs = type(error).__name__
        assert ours == theirs, source
    assert "pub" in seen


def test_is_safe_callable_override_and_alters_data():
    class NoCalls(SandboxedEnvironment):
        def is_safe_callable(self, obj):
            return getattr(obj, "__name__", "") != "fine" and super().is_safe_callable(obj)

    class NoCallsJ2(j2sandbox.SandboxedEnvironment):
        def is_safe_callable(self, obj):
            return getattr(obj, "__name__", "") != "fine" and super().is_safe_callable(obj)

    for source in ("{{ d.fine() }}", "{{ d.go() }}", "{{ g() }}", "{{ gu() }}", "{{ [d.fine][0]() }}", "{% set x = d.fine %}{{ x() }}", "{% macro m() %}{{ d.go() }}{% endmacro %}{{ m() }}", "{{ d.fine }}"):
        results = []
        for env_class in (NoCalls, NoCallsJ2):
            env = env_class()
            try:
                results.append(env.from_string(source).render(d=Dangerous(), g=Dangerous().go, gu=unsafe(lambda: 1)))
            except Exception as error:  # noqa: BLE001
                results.append((type(error).__name__, re.sub(r" at 0x[0-9a-f]+", "", str(error))))
        assert results[0] == results[1], source
    # the message names the callable as Jinja2's does
    with pytest.raises(SecurityError) as ours:
        SandboxedEnvironment().from_string("{{ d.go() }}").render(d=Dangerous())
    with pytest.raises(j2sandbox.SecurityError) as theirs:
        j2sandbox.SandboxedEnvironment().from_string("{{ d.go() }}").render(d=Dangerous())
    assert str(ours.value) == str(theirs.value) == "<bound method Dangerous.go of Dangerous> is not safely callable"


def test_globals_and_callables_in_the_context_are_checked_when_called():
    env = SandboxedEnvironment()
    env.globals["danger"] = unsafe(lambda: "x")
    env.globals["safe"] = lambda: "y"
    assert env.from_string("{{ safe() }}").render() == "y"
    with pytest.raises(SecurityError):
        env.from_string("{{ danger() }}").render()
    # filters and tests are not calls of `call` in Jinja2 either
    env.filters["f"] = unsafe(lambda v: "filtered")
    assert env.from_string("{{ 1|f }}").render() == "filtered"
    jenv = j2sandbox.SandboxedEnvironment()
    jenv.filters["f"] = unsafe(lambda v: "filtered")
    assert jenv.from_string("{{ 1|f }}").render() == "filtered"


def test_immutable_form_on_the_engines_own_containers():
    for source in (
        "{% set l = [1] %}{% do l.append(2) %}{{ l }}",
        "{% set l = [1] %}{{ l.append(2) }}",
        "{% set l = [3, 1] %}{% do l.sort() %}",
        "{% set d = {'a': 1} %}{% do d.update({'b': 2}) %}",
        "{% set d = {'a': 1} %}{% do d.setdefault('b', 2) %}",
        "{% set d = {'a': 1} %}{{ d.pop('a') }}",
        "{% set d = {'a': 1} %}{{ d.popitem() }}",
        "{% set l = [1] %}{% do l.extend([2]) %}",
        "{% set l = [1] %}{% do l.insert(0, 2) %}",
        "{% set l = [1] %}{% do l.remove(1) %}",
        "{% set l = [1] %}{% do l.reverse() %}",
        "{% set l = [1] %}{{ l.copy() }}{{ l.index(1) }}{{ l.count(1) }}",
        "{% set d = {'a': 1} %}{{ d.get('a') }}{{ d.keys()|list }}{{ d.items()|list }}{{ d.copy() }}",
        "{% set f = [].append %}{{ f }}",
        "{{ [].append|string }}",
        "{{ [].append is defined }}",
        "{% set a = [1]|attr('append') %}{{ a }}",
        "{% set a = [1]|attr('append') %}{{ a(1) }}",
    ):
        ours, theirs = _both(source, immutable=True, extensions=["jinja2.ext.do"])
        assert ours[0] == theirs[0] and ours[1:2] == theirs[1:2], source
        if ours[0] == "exception":
            assert ours[2] == theirs[2], source
    # the plain sandbox lets the template change the lists it made
    assert SandboxedEnvironment(extensions=["jinja2.ext.do"]).from_string("{% set l = [1] %}{% do l.append(2) %}{{ l }}").render() == "[1, 2]"


def test_python_containers_in_the_immutable_form():
    for source in ("{% do l.append(1) %}", "{{ l.pop() }}", "{% do d.update({}) %}", "{{ d.clear() }}", "{{ l.index(1) }}", "{{ d.items()|list }}", "{% do s.add(1) %}", "{{ dq.popleft() }}"):
        ours, theirs = _both(source, {"l": [1], "d": {"a": 1}, "s": {1}, "dq": collections.deque([1])}, immutable=True, extensions=["jinja2.ext.do"])
        assert ours[0] == theirs[0], source
        if ours[0] == "exception":
            assert ours[1] == theirs[1] and ours[2] == theirs[2], source
    # Jinja2 gives the template a list it can change
    data = [1]
    SandboxedEnvironment(extensions=["jinja2.ext.do"]).from_string("{% do l.append(2) %}").render(l=data)
    assert data == [1, 2]
    data = [1]
    with pytest.raises(SecurityError):
        ImmutableSandboxedEnvironment(extensions=["jinja2.ext.do"]).from_string("{% do l.append(2) %}").render(l=data)
    assert data == [1]


def test_a_plain_environment_never_asks_the_sandbox():
    asked = []

    class Spy(Environment):
        def _sandbox_attribute(self, obj, attribute, value):  # pragma: no cover - must not run
            asked.append(attribute)
            return value

        def _sandbox_check_call(self, obj):  # pragma: no cover - must not run
            asked.append("call")

        def is_safe_attribute(self, obj, attr, value):  # pragma: no cover - must not run
            asked.append(attr)
            return True

    env = Spy()
    assert env.from_string("{{ o.pub }}{{ o.meth() }}{{ o.__class__.__name__ }}{{ f(1) }}{{ [o]|map(attribute='pub')|list }}").render(o=Obj(), f=lambda x: x) == "13Obj1[1]"
    assert asked == []


def test_the_sandbox_is_scoped_to_its_render():
    inner_plain = Environment()
    outer = SandboxedEnvironment()
    outer.globals["render_plain"] = lambda: inner_plain.from_string("{{ o.__class__.__name__ }}").render(o=Obj())
    outer.globals["render_sandbox"] = lambda: SandboxedEnvironment().from_string("{{ o.__class__ }}|{{ o.pub }}").render(o=Obj())
    # the sandbox of an outer render does not reach the plain render a callable starts
    assert outer.from_string("{{ render_plain() }}|{{ o.__class__ }}|{{ o.pub }}").render(o=Obj()) == "Obj||1"
    assert outer.from_string("{{ render_sandbox() }}").render() == "|1"
    plain = Environment()
    plain.globals["render_sandbox"] = lambda: SandboxedEnvironment().from_string("{{ o.__class__ }}|{{ o.pub }}").render(o=Obj())
    assert plain.from_string("{{ render_sandbox() }}|{{ o.__class__.__name__ }}").render(o=Obj()) == "|1|Obj"
    # and afterwards the thread is as it was
    assert plain.from_string("{{ o.__class__.__name__ }}").render(o=Obj()) == "Obj"


def test_sandbox_applies_to_loaded_templates_macros_and_modules():
    loader = jinja2cpp.DictLoader(
        {
            "base.html": "{% block b %}{{ o.__class__ }}{{ o.pub }}{% endblock %}",
            "child.html": "{% extends 'base.html' %}{% block b %}[{{ super() }}{{ o._priv }}]{% endblock %}",
            "inc.html": "{{ o.__class__ }}{{ o.pub }}",
            "macros.html": "{% macro show(x) %}{{ x.__class__ }}{{ x.pub }}{{ x._priv }}{% endmacro %}",
            "main.html": "{% include 'inc.html' %}{% import 'macros.html' as m %}{{ m.show(o) }}",
        }
    )
    env = SandboxedEnvironment(loader=loader)
    assert env.get_template("child.html").render(o=Obj()) == "[1]"
    assert env.get_template("main.html").render(o=Obj()) == "11"
    module = env.get_template("macros.html").module
    assert module.show(Obj()) == "1"
    unsafe_env = SandboxedEnvironment(loader=loader, undefined=jinja2cpp.StrictUndefined)
    with pytest.raises(SecurityError):
        unsafe_env.get_template("macros.html").module.show(Obj())
    with pytest.raises(SecurityError):
        unsafe_env.get_template("child.html").render(o=Obj())


def test_unsafe_undefined_reaches_a_python_filter_as_security_undefined():
    seen = []

    def spy(value):
        seen.append(value)
        return "spied"

    env = SandboxedEnvironment()
    env.filters["spy"] = spy
    assert env.from_string("{{ o.__class__|spy }}").render(o=Obj()) == "spied"
    undefined = seen[0]
    assert isinstance(undefined, jinja2cpp.Undefined)
    with pytest.raises(SecurityError, match="access to attribute '__class__' of 'Obj' object is unsafe."):
        str(undefined) or undefined._fail_with_undefined_error()


def test_interception_is_refused_not_ignored():
    env = SandboxedEnvironment()
    env.intercepted_binops = frozenset(["+"])
    with pytest.raises(jinja2cpp.UnsupportedError):
        env.from_string("{{ 1 + 2 }}")
    env = SandboxedEnvironment()
    env.intercepted_unops = frozenset(["-"])
    with pytest.raises(jinja2cpp.UnsupportedError):
        env.from_string("{{ -1 }}")
    # the default call_binop and call_unop work for code that calls them
    plain = SandboxedEnvironment()
    assert plain.call_binop(None, "+", 1, 2) == 3 and plain.call_unop(None, "-", 2) == -2


def test_str_format_through_the_formatter_of_the_sandbox():
    for source in (
        "{{ '{0.pub}|{0.d[k]}|{0[pub]}'.format(o) }}",
        "{{ '{0._priv}|{0.__class__}|{0.__dict__}'.format(o) }}",
        "{{ '{x.pub}|{x._priv}'.format_map({'x': o}) }}",
        "{{ ('{0.__class__}{1}'|safe).format(o, '<') }}",
        "{{ ('{x.pub}{y}'|safe).format_map({'x': o, 'y': '<'}) }}",
        "{{ '{}{}'.format(1, 2) }}",
        "{{ '{a}'.format(a=3) }}",
        "{{ '{0:>5}|{1!r}'.format(1, 'x') }}",
        "{{ s.format(o) }}",
        "{{ s.format_map(d) }}",
        "{{ '{0.real}'.format(n) }}",
        "{{ '{0[0]}{0[__len__]}'.format(lst) }}",
        "{{ '{0.items}'.format(d)|length > 0 }}",
        "{{ '{0}'.format(missing) }}",
        "{{ '{0.__class__.__mro__}'.format(o) }}",
        "{{ '{0}'.format() }}",
        "{{ f.format }}",
    ):
        ours, theirs = _both(source, {"o": Obj(), "s": "{0.pub}", "d": {"pub": 5}, "n": 3, "lst": [1, 2], "f": "{}"})
        if source == "{{ f.format }}":
            # a method prints by its repr, which differs in wording
            assert ours[0] == theirs[0] == "ok"
            continue
        assert ours[:2] == theirs[:2], source


def test_markup_format_escapes_in_the_sandbox():
    ours, theirs = _both("{{ ('{0.pub}{1}'|safe).format(o, '<b>') }}", {"o": Obj()})
    assert ours == theirs == ("ok", "1&lt;b&gt;")


def test_sum_of_a_missing_attribute_fails_as_undefined():
    # sum(attribute=) adds the undefined value: UndefinedError in Jinja2 (it was a TemplateRuntimeError here)
    for undefined in ("Undefined", "StrictUndefined"):
        outcomes = []
        for module in (jinja2cpp, jinja2):
            env = module.Environment(undefined=getattr(module, undefined))
            try:
                outcomes.append(env.from_string("{{ items|sum(attribute='z') }}").render(items=[{"a": 1}]))
            except Exception as error:  # noqa: BLE001
                outcomes.append((type(error).__name__, str(error)))
        assert outcomes[0] == outcomes[1] == ("UndefinedError", "'dict object' has no attribute 'z'")


def test_an_overridden_is_safe_attribute_judges_the_public_names_of_engine_values():
    class NoUpper(SandboxedEnvironment):
        def is_safe_attribute(self, obj, attr, value):
            return attr not in ("upper", "items", "index") and super().is_safe_attribute(obj, attr, value)

    class NoUpperJ2(j2sandbox.SandboxedEnvironment):
        def is_safe_attribute(self, obj, attr, value):
            return attr not in ("upper", "items", "index") and super().is_safe_attribute(obj, attr, value)

    for source in (
        "{{ 'x'.upper() }}",
        "{{ 'x'.lower() }}",
        "{{ 'x'.upper }}",
        "{{ {'a': 1}.items() }}",
        "{{ {'a': 1}.keys()|list }}",
        "{{ [1].index(1) }}",
        "{{ [1].count(1) }}",
        "{{ ('x'|attr('upper'))() }}",
        "{{ 'x'['upper'] }}",
        "{% set s = 'x' %}{{ s.upper() }}",
        "{{ loop }}",
    ):
        results = []
        for env_class in (NoUpper, NoUpperJ2):
            try:
                results.append(("ok", env_class().from_string(source).render()))
            except Exception as error:  # noqa: BLE001
                results.append(("exception", type(error).__name__))
        assert results[0] == results[1], source


def _outcome_of(env, source, **context):
    try:
        return ("ok", env.from_string(source).render(**context))
    except Exception as error:  # noqa: BLE001
        return ("exception", type(error).__name__)


@pytest.mark.parametrize("name", ["call", "getattr", "getitem"])
def test_an_overridden_call_getattr_or_getitem_is_refused_not_ignored(name):
    def body(self, *args, **kwargs):
        raise SecurityError("no")

    # Jinja2 calls the override; the engine would not, so loading refuses (the 0412 rule of the interception)
    sub = type("Sub", (SandboxedEnvironment,), {name: body})
    with pytest.raises(jinja2cpp.UnsupportedError, match=name):
        sub().from_string("{{ f() }}")
    # the same set on the instance, and on the immutable form
    env = ImmutableSandboxedEnvironment()
    setattr(env, name, body.__get__(env))
    with pytest.raises(jinja2cpp.UnsupportedError, match=name):
        env.from_string("{{ f() }}")
    # a subclass of another kind is untouched
    assert type("Plain", (SandboxedEnvironment,), {})().from_string("{{ f() }}").render(f=lambda: 1) == "1"


def test_a_call_override_is_the_one_jinja2_runs():
    class E(j2sandbox.SandboxedEnvironment):
        def call(self, ctx, obj, *a, **k):
            raise SecurityError("no calls")

    with pytest.raises(SecurityError):
        E().from_string("{{ f() }}").render(f=lambda: 1)


DENY_SOURCES = [
    "{% for i in [1] %}{{ loop.index }}{% endfor %}",
    "{% for i in [1] %}{{ loop.first }}{% endfor %}",
    "{% for i in [1, 2] %}{{ loop.cycle('a', 'b') }}{% endfor %}",
    "{% for i in [1, 2] %}{{ loop.changed(i) }}{% endfor %}",
    "{% set c = cycler(1, 2) %}{{ c.next() }}",
    "{% set c = cycler(1, 2) %}{{ c.current }}",
    "{% set ns = namespace(a=1) %}{{ ns.a }}",
    "{% macro m() %}x{% endmacro %}{{ m.name }}",
    "{% macro m() %}x{% endmacro %}{{ m.arguments }}",
    "{{ 'x'.upper() }}",
    "{{ [1].count(1) }}",
]


@pytest.mark.parametrize("source", DENY_SOURCES)
def test_an_overridden_is_safe_attribute_judges_public_names_of_loop_cycler_namespace_and_macro(source):
    def deny(self, obj, attr, value):
        return False

    ours = type("Deny", (SandboxedEnvironment,), {"is_safe_attribute": deny})()
    oracle = type("DenyJ2", (j2sandbox.SandboxedEnvironment,), {"is_safe_attribute": deny})()
    assert _outcome_of(ours, source) == _outcome_of(oracle, source), source


def test_public_names_of_loop_are_not_asked_by_the_default_sandbox():
    env = SandboxedEnvironment()
    assert env.from_string("{% for i in [1, 2] %}{{ loop.index }}{{ loop.cycle('a', 'b') }}{% endfor %}").render() == "1a2b"
    assert env.from_string("{% set ns = namespace(a=1) %}{{ ns.a }}").render() == "1"


@pytest.mark.parametrize("patch_instance", [False, True])
def test_is_safe_attribute_replaced_after_the_first_load_is_seen_by_the_next_load(patch_instance):
    class Env(SandboxedEnvironment):
        pass

    env = Env()
    source = "{% for i in [1] %}{{ loop.index }}{% endfor %}"
    assert env.from_string(source).render() == "1"

    def deny_public(obj, attr, value):
        return attr.startswith("_") is False and False

    if patch_instance:
        env.is_safe_attribute = deny_public
    else:
        Env.is_safe_attribute = lambda self, obj, attr, value: False
    assert env.from_string(source).render() == ""


ATTR_ITEM_SOURCES = [
    "{% set ns = namespace(secret=42) %}{{ ns|attr('secret') }}|{{ ns.secret }}|{{ ns['secret'] }}",
    "{% for i in [1] %}{{ loop|attr('index') }}|{{ loop.index }}|{{ loop['index'] }}{% endfor %}",
    "{% macro m() %}x{% endmacro %}{{ m|attr('name') }}|{{ m.name }}|{{ m['name'] }}",
    "{% set c = cycler(1, 2) %}{{ c|attr('current') }}|{{ c.current }}|{{ c['current'] }}",
]


@pytest.mark.parametrize("source", ATTR_ITEM_SOURCES)
@pytest.mark.parametrize("immutable", [False, True], ids=["sandbox", "immutable"])
def test_attr_filter_and_items_of_loop_namespace_and_macro_reach_is_safe_attribute(source, immutable):
    def deny(self, obj, attr, value):
        return False

    base_ours, base_oracle = (ImmutableSandboxedEnvironment, j2sandbox.ImmutableSandboxedEnvironment) if immutable else (SandboxedEnvironment, j2sandbox.SandboxedEnvironment)
    ours = type("Deny", (base_ours,), {"is_safe_attribute": deny})()
    oracle = type("DenyJ2", (base_oracle,), {"is_safe_attribute": deny})()
    assert _outcome_of(ours, source) == _outcome_of(oracle, source), source
    # and the default sandbox reads them all
    default = _outcome_of(SandboxedEnvironment(), source)
    assert default == _outcome_of(j2sandbox.SandboxedEnvironment(), source), source
    assert default[0] == "ok" and default[1] != "||", source


TOO_LONG = [
    "{{ '{0:200000000}'.format('x')|length }}",
    "{{ '{0:>200000000}'.format('x')|length }}",
    "{{ '{0:.200000000f}'.format(1.5)|length }}",
    "{{ '{0:{1}}'.format('x', 200000000)|length }}",
    "{{ '{a:200000000}'.format_map({'a': 'x'})|length }}",
    "{{ ('{0:200000000}'|safe).format('x')|length }}",
    "{{ '{0:30000000}{0:30000000}{0:30000000}'.format('x')|length }}",
    "{{ '{0:99999999999999999999999}'.format('x')|length }}",
]


@pytest.mark.parametrize("source", TOO_LONG)
@pytest.mark.parametrize("env_class", [SandboxedEnvironment, ImmutableSandboxedEnvironment])
def test_str_format_in_the_sandbox_is_capped_before_it_allocates(source, env_class):
    # Python would build the string first; the engine's cap (MaxSequenceSize) is checked from the width, the
    # precision and what the fields wrote, and fails as the engine's own str.format does
    with pytest.raises(TemplateRuntimeError, match="too long"):
        env_class().from_string(source).render()
    with pytest.raises(TemplateRuntimeError, match="too (long|big)"):
        Environment().from_string(source.replace("'{a:200000000}'.format_map({'a': 'x'})", "'{0:200000000}'.format('x')")).render()


def test_str_format_in_the_sandbox_still_formats_what_fits():
    for source in (
        "{{ '{0:>10}|{1:.3f}|{2:,}|{3:{4}}|{0:^7}'.format('x', 1.23456, 1234567, 'y', 6) }}",
        "{{ '{a:*<8}'.format_map({'a': 'x'}) }}",
        "{{ ('{0:>6}'|safe).format('<b>') }}",
        "{{ '{0:.100000000s}'.format('abc') }}",
        "{{ '{0:08.3f}{0:+d}'.format(3) if false else '{0:08.3f}'.format(3.5) }}",
    ):
        assert _outcome_of(SandboxedEnvironment(), source) == _outcome_of(j2sandbox.SandboxedEnvironment(), source), source


UNICODE_DIGITS = {
    "arabic_indic": "١٠٠٠٠٠٠٠٠٠",
    "fullwidth": "１０００００００００",
    "devanagari": "१०००००००००",
}


@pytest.mark.parametrize("digits", UNICODE_DIGITS.values(), ids=UNICODE_DIGITS.keys())
@pytest.mark.parametrize(
    "template",
    [
        "{{ '{0:%s}'.format('x')|length }}",
        "{{ '{0:.%sf}'.format(1.5)|length }}",
        "{{ '{0:>%s}'.format('x')|length }}",
        "{{ '{a:%s}'.format_map({'a': 'x'})|length }}",
        "{{ ('{0:%s}'|safe).format('x')|length }}",
        "{{ '{0:{1}}'.format('x', %s)|length }}",
    ],
)
def test_str_format_cap_sees_unicode_digits_in_widths_and_precisions(template, digits):
    source = template % digits
    if "{1}" in template:
        source = template % "1000000000"
    with pytest.raises(TemplateRuntimeError, match="too (long|big)"):
        SandboxedEnvironment().from_string(source).render()


def test_str_format_cap_fails_closed_on_a_spec_outside_the_grammar():
    # CPython rejects it (or a type's own __format__ reads it): the largest number in it is the bound
    import datetime

    env = SandboxedEnvironment()
    with pytest.raises(TemplateRuntimeError, match="too (long|big)"):
        env.from_string("{{ '{0:%Y 999999999}'.format(d) }}").render(d=datetime.date(2020, 1, 2))
    assert env.from_string("{{ '{0:%Y-%m-%d}'.format(d) }}").render(d=datetime.date(2020, 1, 2)) == "2020-01-02"


OBJECTS = [
    ("{% set o = namespace(secret=42) %}", "secret"),
    ("{% for i in [1] %}{% set o = loop %}", "index"),
    ("{% macro m() %}x{% endmacro %}{% set o = m %}", "name"),
    ("{% set o = cycler(1, 2) %}", "current"),
]
ATTRIBUTE_FILTERS = [
    "{{ [o]|map(attribute='%s')|join }}",
    "{{ [o]|selectattr('%s')|list|length }}",
    "{{ [o]|rejectattr('%s')|list|length }}",
    "{{ [o]|sort(attribute='%s')|list|length }}",
    "{{ [o]|groupby('%s')|list|length }}",
    "{{ [o]|unique(attribute='%s')|list|length }}",
    "{{ [o]|min(attribute='%s') is defined }}",
    "{{ [o]|max(attribute='%s') is defined }}",
    "{{ [o]|sum(attribute='%s') }}",
    "{{ [o]|map(attribute='%s.x', default='d')|join }}",
]


@pytest.mark.parametrize("filter_source", ATTRIBUTE_FILTERS)
@pytest.mark.parametrize("prefix,name", OBJECTS, ids=["namespace", "loop", "macro", "cycler"])
@pytest.mark.parametrize("strict", [False, True], ids=["default", "strict"])
def test_attribute_filters_ask_is_safe_attribute_about_loop_namespace_and_macro(filter_source, prefix, name, strict):
    if strict and "unique" in filter_source:
        pytest.skip("documented difference: unique does not raise on a StrictUndefined key (docs/python.md, task 0413)")

    def deny(self, obj, attr, value):
        return False

    source = prefix + filter_source % name + ("{% endfor %}" if "{% for" in prefix else "")
    ours = type("Deny", (SandboxedEnvironment,), {"is_safe_attribute": deny})(undefined=jinja2cpp.StrictUndefined if strict else jinja2cpp.Undefined)
    oracle = type("DenyJ2", (j2sandbox.SandboxedEnvironment,), {"is_safe_attribute": deny})(undefined=jinja2.StrictUndefined if strict else jinja2.Undefined)
    assert _outcome_of(ours, source) == _outcome_of(oracle, source), source
