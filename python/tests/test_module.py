"""Template.make_module and Template.module: exports, macros, lifetimes, and jinja2.meta."""

import faulthandler
import gc
import threading
import time
import weakref

import jinja2
import jinja2cpp
import jinja2cpp.nodes
import markupsafe
import pytest
from jinja2 import meta as jmeta
from jinja2cpp import meta

SOURCE = """\
{% set greeting = 'hello' %}{% set _hidden = 1 %}{% set n = 41 + 1 %}{% set items = [1, 2, 3] %}\
{% macro shout(who, punct='!') %}{{ greeting|upper }} {{ who }}{{ punct }}{% endmacro %}\
{% macro twice(x) %}{{ x * 2 }}{% endmacro %}\
body {{ n }}"""


def both(source, *args, **kwargs):
    return [module.Environment().from_string(source).make_module(*args, **kwargs) for module in (jinja2cpp, jinja2)]


def test_exports_and_body():
    ours, theirs = both(SOURCE)
    assert str(ours) == str(theirs) == "body 42"
    assert ours.greeting == theirs.greeting and ours.n == theirs.n == 42
    assert ours.items == theirs.items == [1, 2, 3]
    assert not hasattr(ours, "_hidden") and not hasattr(theirs, "_hidden")
    assert ours.shout("you") == theirs.shout("you") == "HELLO you!"
    assert ours.shout("you", punct="?") == theirs.shout("you", punct="?")
    assert ours.shout(who="k") == theirs.shout(who="k")
    assert ours.twice(21) == theirs.twice(21) == "42"
    assert ours.__html__() == theirs.__html__() and isinstance(ours.__html__(), markupsafe.Markup)
    assert repr(ours).startswith("<TemplateModule 'memory:") or repr(ours).startswith("<TemplateModule memory:")


def test_module_repr_has_the_template_name():
    env = jinja2cpp.Environment(loader=jinja2cpp.DictLoader({"m.html": "{% set a = 1 %}"}))
    module = env.get_template("m.html").module
    assert repr(module) == "<TemplateModule 'm.html'>"
    assert module.__name__ == "m.html" and module.a == 1


def test_module_property_is_made_once():
    for module in (jinja2cpp, jinja2):
        tpl = module.Environment().from_string("{% set a = [] %}x")
        assert tpl.module is tpl.module
        assert tpl.make_module() is not tpl.module


def test_vars_locals_and_shared():
    source = "{% set total = a + b %}{{ a }}-{{ g }}"
    for kwargs in ({"vars": {"a": 1, "b": 2}}, {"vars": {"a": 1}, "locals": {"b": 5}}, {"vars": {"a": 1, "b": 2}, "shared": False}):
        ours, theirs = [m.Environment().from_string(source).make_module(**kwargs) for m in (jinja2cpp, jinja2)]
        assert (str(ours), ours.total) == (str(theirs), theirs.total), kwargs
    # a global is visible, unless the context is shared
    for module in (jinja2cpp, jinja2):
        env = module.Environment()
        env.globals["g"] = "G"
        out = env.from_string("{{ a }}-{{ g }}").make_module({"a": 1})
        assert str(out) == "1-G"
        assert str(env.from_string("{{ a }}-{{ g }}").make_module({"a": 1}, shared=True)) == "1-"
        # any truth value, as Jinja2 takes it
        assert str(env.from_string("{{ a }}-{{ g }}").make_module({"a": 1}, shared=1)) == "1-"
        assert str(env.from_string("{{ a }}-{{ g }}").make_module({"a": 1}, shared=None)) == "1-G"


SHARED_TEMPLATES = {
    "inc": "{{ x }}:{{ range is defined }}",
    "imp": "{% macro m() %}{{ range(2)|list }}{{ x is defined }}{% endmacro %}",
}


@pytest.mark.parametrize(
    "source",
    [
        "{{ range(2)|list }}",
        "{% set ns = namespace(a=1) %}{{ ns.a }}",
        "{{ dict(a=1) }}",
        "{{ cycler(1, 2).next() }}",
        "{{ joiner()() }}",
        "{{ lipsum(1) }}",
        "{{ x }}{{ range is defined }}{{ namespace is defined }}{{ gettext is defined }}",
        "{% macro m() %}{{ range is defined }}{% endmacro %}{{ m() }}",
        '{% include "inc" %}',
        '{% include "inc" without context %}',
        '{% import "imp" as i %}{{ i.m() }}',
        '{% import "imp" as i with context %}{{ i.m() }}',
        '{% from "imp" import m %}{{ m() }}',
    ],
)
def test_shared_module_leaves_out_the_builtin_globals(source):
    # Jinja2's shared=True renders with the vars alone: range, dict, cycler, joiner, namespace and lipsum are undefined,
    # also in an include and an import with context; an include or import without context sees the environment's (0574)
    results = []
    for module in (jinja2cpp, jinja2):
        env = module.Environment(loader=module.DictLoader(SHARED_TEMPLATES))
        try:
            results.append(str(env.from_string(source).make_module({"x": 1}, shared=True)))
        except module.UndefinedError as error:
            results.append(("UndefinedError", str(error)))
    assert results[0] == results[1], source


def test_module_globals_and_filters_of_the_environment():
    def shout(value):
        return str(value).upper() + "!"

    for module in (jinja2cpp, jinja2):
        env = module.Environment()
        env.filters["shout"] = shout
        env.globals["answer"] = lambda: 42
        mod = env.from_string("{% macro m(x) %}{{ x|shout }}{{ answer() }}{% endmacro %}{% set v = answer() %}").make_module()
        assert mod.v == 42 and mod.m("a") == "A!42"


def test_autoescape_macro_returns_markup():
    ours, theirs = [m.Environment(autoescape=True).from_string("{% macro e(x) %}<b>{{ x }}</b>{% endmacro %}{% set s = '<i>'|safe %}").make_module() for m in (jinja2cpp, jinja2)]
    assert ours.e("<&>") == theirs.e("<&>") == "<b>&lt;&amp;&gt;</b>"
    assert isinstance(ours.e("x"), markupsafe.Markup)
    assert isinstance(ours.s, markupsafe.Markup)


def test_module_errors():
    for module in (jinja2cpp, jinja2):
        env = module.Environment(undefined=module.StrictUndefined)
        with pytest.raises(jinja2.UndefinedError):
            env.from_string("{{ nope }}").make_module()
        mod = env.from_string("{% macro bad() %}{{ nope }}{% endmacro %}{% macro div(a) %}{{ 1 // a }}{% endmacro %}").make_module()
        with pytest.raises(jinja2.UndefinedError):
            mod.bad()
        # Jinja2 lets the ZeroDivisionError through; ours is a TemplateRuntimeError (docs/python.md)
        with pytest.raises((jinja2.TemplateRuntimeError, ZeroDivisionError)):
            mod.div(0)
    with pytest.raises(jinja2cpp.TemplateSyntaxError):
        jinja2cpp.Environment().from_string("{% if %}")
    with pytest.raises(TypeError):
        jinja2cpp.Template("x").make_module(5)


def test_macro_calls_a_python_exception_as_itself():
    class Boom(Exception):
        pass

    def boom(value):
        raise Boom(value)

    env = jinja2cpp.Environment()
    env.filters["boom"] = boom
    mod = env.from_string("{% macro m(x) %}{{ x|boom }}{% endmacro %}").make_module()
    with pytest.raises(Boom):
        mod.m(7)


def test_module_macro_survives_every_other_reference():
    """The macro keeps the module, the template and the environment alive (docs/tasks/0284)."""
    calls = []

    def filt(value):
        calls.append(value)
        return f"<{value}>"

    env = jinja2cpp.Environment(loader=jinja2cpp.DictLoader({"lib": "{% macro w(x) %}[{{ x|f }}]{% endmacro %}"}))
    env.filters["f"] = filt
    tpl = env.from_string("{% from 'lib' import w %}{% set ns = namespace(count=0) %}{% macro m(x) %}{{ w(x) }}{% set ns.count = ns.count + 1 %}{{ ns.count }}{% endmacro %}{% set pack = {'inner': [m]} %}")
    module = tpl.make_module()
    macro = module.m
    nested = module.pack["inner"][0]
    refs = [weakref.ref(env), weakref.ref(tpl), weakref.ref(module)]
    del env, tpl, module
    gc.collect()
    assert refs[1]() is None and refs[2]() is None
    assert refs[0]() is not None  # kept by the macro, which calls its filters
    assert macro("a") == "[<a>]1"
    assert nested("b") == "[<b>]2"  # a macro inside an exported list and dict shares the module
    assert calls == ["a", "b"]
    gc.collect()
    assert macro("c") == "[<c>]3"
    del macro, nested
    gc.collect()
    assert refs[0]() is None


def test_module_macro_cycle_is_collected():
    env = jinja2cpp.Environment()
    mod = env.from_string("{% macro m() %}x{% endmacro %}").make_module()
    env.keep = mod.m  # environment -> macro -> environment
    refs = [weakref.ref(env)]
    del env, mod
    gc.collect()
    assert refs[0]() is None


def test_module_macro_repr_and_type():
    mod = jinja2cpp.Template("{% macro hello() %}x{% endmacro %}").module
    assert callable(mod.hello) and "hello" in repr(mod.hello)


def test_module_params_and_exports_are_copies():
    data = {"k": [1, 2]}
    for module in (jinja2cpp, jinja2):
        mod = module.Environment().from_string("{% set d = data %}{% set s = 'str' %}{% set u = nope %}").make_module({"data": data})
        assert mod.d == data and mod.s == "str"
        if module is jinja2cpp:
            # a module takes its params by value (Jinja2's module shares the dict)
            data["k"].append(3)
            assert mod.d == {"k": [1, 2]}
            data["k"].pop()
        # An exported Undefined is the environment's Undefined (0411)
        assert isinstance(mod.u, module.Undefined) and not mod.u


# jinja2.meta


META_SOURCES = [
    "{% set foo = 42 %}{{ bar + foo }}",
    "{{ a }} {% for x in items %}{{ x }}{{ y }}{% endfor %}",
    "{% macro m(p) %}{{ p }}{{ q }}{% endmacro %}{{ m(1) }}{{ r }}",
    "{% if c %}{% set v = 1 %}{% endif %}{{ v }}",
    "{{ range(3) }}{{ dict }}{{ lipsum }}",
    "",
    "plain text",
]


@pytest.mark.parametrize("source", META_SOURCES)
def test_find_undeclared_variables(source):
    ours = meta.find_undeclared_variables(jinja2cpp.Environment().parse(source))
    theirs = jmeta.find_undeclared_variables(jinja2.Environment().parse(source))
    assert ours == theirs and isinstance(ours, set)


def test_find_undeclared_variables_leaves_out_the_globals():
    env = jinja2cpp.Environment()
    env.globals["g"] = 1
    assert meta.find_undeclared_variables(env.parse("{{ g }}{{ h }}")) == {"h"}


REFERENCED = [
    "{% extends 'base.html' %}{% include 'a.html' %}",
    "{% include 'a' %}{% import 'b' as b %}{% from 'c' import d %}",
    "{% include ['a','b'] %}",
    "{% include ('a', 'b') %}",
    "{% include name %}{% include 'x' %}",
    "{% include 'a' ~ 'b' %}",
    "{% include 'a' if x else 'b' %}",
    "{% include 1 %}",
    "{% include ['a', 1] %}",
    "{% if x %}{% include 'a' %}{% else %}{% include 'b' %}{% endif %}",
    "{% macro m() %}{% include 'in-macro' %}{% endmacro %}{% for i in x %}{% from 'lp' import q %}{% endfor %}",
    "{% include 'a' ignore missing without context %}",
    "no references",
]


@pytest.mark.parametrize("source", REFERENCED)
def test_find_referenced_templates(source):
    ours = list(meta.find_referenced_templates(jinja2cpp.Environment().parse(source)))
    theirs = list(jmeta.find_referenced_templates(jinja2.Environment().parse(source)))
    assert ours == theirs


def test_parse_errors_and_arguments():
    env = jinja2cpp.Environment()
    with pytest.raises(jinja2cpp.TemplateSyntaxError) as info:
        env.parse("{% if %}", name="n.html", filename="/p/n.html")
    assert info.value.lineno == 1 and info.value.name == "n.html"
    ast = env.parse("{{ x }}")
    assert meta.find_undeclared_variables(ast) == {"x"} and "ParsedTemplate" in repr(ast)
    with pytest.raises(TypeError):
        meta.find_undeclared_variables("{{ x }}")
    with pytest.raises(TypeError):
        list(meta.find_referenced_templates(None))
    # a Template works too
    assert meta.find_undeclared_variables(env.from_string("{{ y }}")) == {"y"}


def test_module_macro_after_environment_is_reconfigured():
    env = jinja2cpp.Environment(loader=jinja2cpp.DictLoader({"lib": "{% macro w(x) %}[{{ x }}]{% endmacro %}"}))
    mod = env.from_string("{% macro m(x) %}{% import 'lib' as l %}{{ l.w(x) }}{% endmacro %}").make_module()
    env.trim_blocks = True
    env.loader = jinja2cpp.DictLoader({"lib": "{% macro w(x) %}<{{ x }}>{% endmacro %}"})
    env.autoescape = True
    assert mod.m(1) in ("[1]", "<1>")
    del env
    gc.collect()
    assert mod.m(2) in ("[2]", "<2>")


def test_module_arguments():
    mod = jinja2cpp.Template("{% macro kinds(a, b=None) %}{{ a }}|{{ b }}|{{ varargs }}|{{ kwargs|dictsort }}{% endmacro %}").module
    ref = jinja2.Template("{% macro kinds(a, b=None) %}{{ a }}|{{ b }}|{{ varargs }}|{{ kwargs|dictsort }}{% endmacro %}").module
    for args, kwargs in [((1,), {}), ((1, 2, 3, 4), {}), ((1,), {"z": 5, "y": [1, "a"]}), (([1, {"k": [1, 2]}], None, 3.5, True), {})]:
        assert mod.kinds(*args, **kwargs) == ref.kinds(*args, **kwargs), (args, kwargs)
    for args, kwargs in [((), {}), ((1,), {"a": 2}), ((), {"b": 1})]:
        outcomes = []
        for macro in (mod.kinds, ref.kinds):
            try:
                outcomes.append(macro(*args, **kwargs))
            except Exception as error:  # noqa: BLE001 - compared by class name
                outcomes.append(type(error).__name__)
        assert outcomes[0] == outcomes[1], (args, kwargs, outcomes)


def test_module_macro_takes_callables_and_big_numbers():
    mod = jinja2cpp.Template("{% macro apply(f, v) %}{{ f(v) }}{% endmacro %}{% macro big(n) %}{{ n * 2 }}{% endmacro %}").module
    assert mod.apply(lambda v: v + 1, 41) == "42"
    assert mod.big(2**70) == str(2**71)
    assert mod.apply(len, [1, 2, 3]) == "3"


def test_module_tuple_argument_arrives_as_a_list():
    mod = jinja2cpp.Template("{% macro show(x) %}{{ x }}{% endmacro %}").module
    assert mod.show((1, "a")) == "[1, 'a']"  # Jinja2: (1, 'a'); the host API has no tuple (docs/python.md)


def test_module_macro_called_from_another_render():
    env = jinja2cpp.Environment()
    mod = env.from_string("{% macro greet(n) %}hi {{ n }}{% endmacro %}{% macro bad(n) %}{{ 1 // n }}{% endmacro %}").make_module()
    assert env.from_string("{{ greet('x') }}|{{ greet(n='y') }}").render(greet=mod.greet) == "hi x|hi y"
    with pytest.raises(jinja2cpp.TemplateRuntimeError):
        env.from_string("{{ bad(0) }}").render(bad=mod.bad)


def _nested(kind, depth):
    value = []
    for _ in range(depth):
        value = [value] if kind == "list" else ({"a": value} if kind == "dict" else (value,))
    return value


@pytest.mark.parametrize("kind,depth", [("list", 10000), ("dict", 10000), ("tuple", 10000), ("list", 200000)])
def test_deeply_nested_module_arguments_raise_recursion_error(kind, depth):
    tpl = jinja2cpp.Template("{% macro m(x) %}{{ x|length }}{% endmacro %}{{ x|length }}")
    value = _nested(kind, depth)
    with pytest.raises(RecursionError):
        tpl.make_module({"x": value})
    with pytest.raises(RecursionError):
        tpl.module.m(value)
    del value



def test_shared_references_in_module_arguments_are_bounded():
    # A small acyclic graph whose copy doubles at each level: refused before it allocates
    tpl = jinja2cpp.Template("{% macro m(x) %}{{ 1 }}{% endmacro %}")
    x = []
    for _ in range(30):
        x = [x, x]
    with pytest.raises(MemoryError):
        tpl.module.m(x)
    with pytest.raises(MemoryError):
        tpl.make_module({"x": x})
    # The budget is per call, not per argument
    y = []
    for _ in range(19):
        y = [y, y]
    with pytest.raises(MemoryError):
        tpl.module.m(y, y, y)
    assert tpl.module.m(y) == "1"


def test_module_argument_changed_by_a_key_str_during_the_copy():
    tpl = jinja2cpp.Template("{% macro m(x) %}{{ x|length }}{% endmacro %}")
    arg = []
    holder = {}

    class Key:
        def __str__(self):
            arg.clear()
            holder.clear()
            return "k"

    arg.extend([{Key(): 0}, 0, [1, 2]])
    assert tpl.module.m(arg) == "3"
    holder.update({Key(): 1, "b": 2})
    assert tpl.module.m(holder) in ("1", "2")
    assert tpl.make_module({"x": [{Key(): 0}, 1]}) is not None

def test_moderately_nested_module_arguments_work_and_cycles_are_cut():
    tpl = jinja2cpp.Template("{% macro m(x) %}{{ x|length }}{% endmacro %}")
    assert tpl.module.m(_nested("list", 500)) == "1"
    cyc = []
    cyc.append(cyc)
    assert tpl.module.m(cyc) == "1"


def _marked(module, kind):
    def f(first, value):
        return f"{type(first).__name__}:{value}"

    return {"context": module.pass_context, "eval": module.pass_eval_context, "env": module.pass_environment}[kind](f)


@pytest.mark.parametrize("kind", ["context", "eval", "env"])
def test_pass_decorators_in_module_macros_and_bodies(kind):
    outs = []
    for module in (jinja2cpp, jinja2):
        env = module.Environment()
        env.filters["f"] = _marked(module, kind)
        mod = env.from_string("{% macro m(x) %}{{ x|f }}{% endmacro %}{{ 1|f }}").make_module()
        outs.append((str(mod), mod.m(2), env.from_string("{{ 3|f }}").module.__html__()))
    assert outs[0][0].split(":")[1:] == outs[1][0].split(":")[1:]
    assert outs[0][1].split(":")[1:] == outs[1][1].split(":")[1:]
    assert outs[0][2].split(":")[1:] == outs[1][2].split(":")[1:]


def test_oldstyle_gettext_in_a_module_macro():
    outs = []
    for module in (jinja2cpp, jinja2):
        env = module.Environment(extensions=["jinja2.ext.i18n"] if module is jinja2 else [jinja2cpp.ext.i18n])
        env.install_gettext_callables(lambda s: s.upper(), lambda s, p, n: s if n == 1 else p, newstyle=False)
        mod = env.from_string("{% macro m(n) %}{{ _('hi') }} {{ ngettext('a', 'b', n) }}{% endmacro %}{{ _('top') }}").make_module()
        outs.append((str(mod), mod.m(2)))
    assert outs[0] == outs[1] == ("TOP", "HI b")


class User:
    def __init__(self):
        self.tags = ["a", "b"]
        self.name = "u"

    def m(self):
        return "called"


def test_python_object_params_keep_attributes_lists_and_methods():
    source = "{% set t = users[0].tags %}{% set called = users[0].m() %}{% macro g(u) %}{{ u.tags|join(',') }}{{ u.m() }}{% endmacro %}{{ users[0].name }}"
    mods = [m.Environment().from_string(source).make_module({"users": [User()]}) for m in (jinja2cpp, jinja2)]
    assert str(mods[0]) == str(mods[1]) == "u"
    # a list read off a Python object is that list (0411)
    assert mods[0].t == mods[1].t == ["a", "b"]
    assert mods[0].called == "called"
    assert mods[0].g(User()) == mods[1].g(User()) == "a,bcalled"


# Jinja2's Macro.__call__: an EvalContext first argument is not the macro's, and its autoescape decides whether the
# result is Markup (0434)
@pytest.mark.parametrize("autoescape", [True, False])
def test_macro_takes_an_eval_context_first(autoescape):
    source = "{% macro hello(name) %}<p>{{ name }}</p>{% endmacro %}"
    results = []
    for module, eval_context in ((jinja2cpp, jinja2cpp.nodes.EvalContext), (jinja2, jinja2.nodes.EvalContext)):
        env = module.Environment(autoescape=True)
        macro = env.from_string(source).module.hello
        context = eval_context(env)
        context.autoescape = autoescape
        result = macro(context, "<b>")
        results.append((type(result).__name__, str(result), type(macro("<b>")).__name__))
    assert results[0] == results[1]
    # Jinja2's own EvalContext is taken too
    assert jinja2cpp.Environment().from_string(source).module.hello(jinja2.nodes.EvalContext(jinja2.Environment()), "x") == "<p>x</p>"


MACRO_SOURCE = (
    "{% macro foo(a, b) %}{% endmacro %}{% macro bar() %}{{ varargs }}{{ kwargs }}{% endmacro %}"
    "{% macro baz() %}{{ caller() }}{% endmacro %}{% set alias = foo %}"
)


def test_module_macro_attributes():
    ours, theirs = both(MACRO_SOURCE)
    for name in ("foo", "bar", "baz", "alias"):
        mine, expected = getattr(ours, name), getattr(theirs, name)
        for attribute in ("name", "arguments", "catch_kwargs", "catch_varargs", "caller"):
            assert getattr(mine, attribute) == getattr(expected, attribute), (name, attribute)
    assert ours.foo.arguments == ("a", "b")


class Tagged:
    def __init__(self):
        self.tags = ["a", "b"]


def test_module_exports_keep_python_objects():
    obj = Tagged()
    source = "{% set o = obj %}{% set tags = obj.tags %}{% set lst = [obj] %}{% set tup = (obj, 1) %}{% set d = {'k': obj} %}"
    mods = both(source, {"obj": obj})
    for mod in mods:
        assert mod.o is obj and mod.tags is obj.tags and mod.lst[0] is obj and mod.tup[0] is obj and mod.d["k"] is obj
    assert type(mods[0].lst) is type(mods[1].lst) and type(mods[0].tup) is type(mods[1].tup) and type(mods[0].d) is type(mods[1].d)


def test_module_exports_undefined():
    source = "{% set u = nope %}{% set v = [nope, 1] %}"
    for module in (jinja2cpp, jinja2):
        mod = module.Environment().from_string(source).make_module()
        assert type(mod.u) is module.Undefined and str(mod.u) == "" and not mod.u
        assert type(mod.v[0]) is module.Undefined and mod.v[1] == 1
        # the environment's class: a strict Undefined is made, and raises where it is used
        env = module.Environment(undefined=module.StrictUndefined)
        mod = env.from_string(source).make_module()
        assert type(mod.u) is module.StrictUndefined
        with pytest.raises(jinja2.UndefinedError):
            str(mod.u)
        with pytest.raises(jinja2.UndefinedError):
            bool(mod.u)


def test_module_exports_tuples_pairs_and_keys():
    source = "{% set t = (1, 'a', (2, 3)) %}{% set ds = {'b': 1, 'a': 2}|dictsort %}{% set d = {1: 'x', 'k': 2, (1, 2): 3, None: 4} %}{% set e = [] %}{% set one = (1,) %}"
    ours, theirs = both(source)
    assert ours.t == theirs.t == (1, "a", (2, 3)) and type(ours.t) is tuple
    assert ours.ds == theirs.ds == [("a", 2), ("b", 1)] and type(ours.ds) is list and type(ours.ds[0]) is tuple
    assert ours.d == theirs.d == {1: "x", "k": 2, (1, 2): 3, None: 4}
    assert list(ours.d) == list(theirs.d)
    assert ours.e == theirs.e == [] and ours.one == theirs.one == (1,)


def test_module_exports_macros_stay_callable_in_tuples_and_keyed_dicts():
    source = "{% macro m(x) %}<{{ x }}>{% endmacro %}{% set t = (m, 1) %}{% set d = {1: m} %}"
    ours, theirs = both(source)
    assert ours.t[0](2) == theirs.t[0](2) == "<2>"
    assert ours.d[1](3) == theirs.d[1](3) == "<3>"


LIVE_SOURCE = (
    "{% set ns = namespace(count=0) %}{% set cyc = cycler('a', 'b', 'c') %}{% set j = joiner('|') %}"
    "{% macro inc() %}{% set ns.count = ns.count + 1 %}{{ ns.count }}{% endmacro %}"
    "{% macro cur() %}{{ cyc.current }}{% endmacro %}{% macro nxt() %}{{ cyc.next() }}{% endmacro %}"
    "{% macro show(x) %}{{ x.count }}{% endmacro %}{% macro set_on(x) %}{% set x.count = 40 %}{% endmacro %}"
)


def test_module_namespace_is_live():
    ours, theirs = both(LIVE_SOURCE)
    for mod in (ours, theirs):
        assert mod.ns.count == 0
        assert mod.inc() == "1"
        assert mod.ns.count == 1
    assert repr(ours.ns) == repr(theirs.ns) == "<Namespace {'count': 1}>"
    assert str(ours.ns) == str(theirs.ns)
    assert hasattr(ours.ns, "count") and not hasattr(ours.ns, "nope") and not hasattr(theirs.ns, "nope")
    with pytest.raises(AttributeError):
        ours.ns.nope
    # what a macro does to the namespace it is passed is done to the module's own
    for mod in (ours, theirs):
        assert mod.show(mod.ns) == "1"
        mod.set_on(mod.ns)
        assert mod.ns.count == 40
        assert mod.inc() == "41"


def test_module_cycler_is_live():
    ours, theirs = both(LIVE_SOURCE)
    for mod in (ours, theirs):
        assert (mod.cyc.next(), mod.cyc.next(), mod.cyc.current, mod.cyc.pos) == ("a", "b", "c", 2)
        # the macros read the cycler the host advanced, and the host the one they advanced
        assert (mod.cur(), mod.nxt(), mod.cyc.current, mod.cyc.pos) == ("c", "c", "a", 0)
        assert next(mod.cyc) == "a"
        assert mod.cyc.reset() is None
        assert mod.cyc.current == "a"
        assert list(mod.cyc.items) == ["a", "b", "c"]
        assert (mod.j(), mod.j(), mod.j.sep) == ("", "|", "|")


def test_module_objects_reject_what_they_are_not():
    ours, theirs = both(LIVE_SOURCE)
    for mod in (ours, theirs):
        for call in (lambda: mod.ns(), lambda: next(mod.ns), lambda: iter(mod.ns), lambda: next(mod.j), lambda: iter(mod.cyc)):
            with pytest.raises(TypeError):
                call()


def test_module_object_passed_to_render_and_nested():
    source = "{% set ns = namespace(inner=namespace(n=1), l=[namespace(k=2)]) %}{% set d = {'o': ns.inner} %}{% macro bump() %}{% set i = ns.inner %}{% set i.n = i.n + 1 %}{% endmacro %}"
    for module in (jinja2cpp, jinja2):
        env = module.Environment()
        mod = env.from_string(source).make_module()
        inner = mod.ns.inner
        assert inner.n == 1 and mod.ns.l[0].k == 2 and mod.d["o"].n == 1
        mod.bump()
        assert inner.n == 2 and mod.d["o"].n == 2
        assert env.from_string("{{ ns.n }}").render(ns=inner) == "2"


def test_module_object_outlives_template_and_environment():
    env = jinja2cpp.Environment()
    tpl = env.from_string(LIVE_SOURCE)
    mod = tpl.make_module()
    ns, cyc, inc = mod.ns, mod.cyc, mod.inc
    del tpl, env, mod
    gc.collect()
    assert ns.count == 0
    inc()
    assert ns.count == 1
    assert cyc.next() == "a" and cyc.current == "b"


def test_module_object_cycle_is_collected():
    env = jinja2cpp.Environment()
    mod = env.from_string(LIVE_SOURCE).make_module()
    ns = mod.ns
    env.keep = ns  # environment -> object -> environment
    refs = [weakref.ref(env), weakref.ref(ns)]
    del env, mod, ns
    gc.collect()
    assert refs[0]() is None and refs[1]() is None


def test_module_object_holding_itself():
    source = "{% set ns = namespace(n=1) %}{% set ns.me = ns %}"
    ours, theirs = both(source)
    for mod in (ours, theirs):
        me = mod.ns.me
        assert me.n == 1 and me.me.n == 1 and me is mod.ns and me.me is me
    # Python's dict repr stops where the namespace repeats
    assert repr(ours.ns) == repr(theirs.ns) == "<Namespace {'n': 1, 'me': <Namespace {...}>}>"
    del me, ours
    gc.collect()


OBJECTS_SOURCE = (
    "{% set ns = namespace(count=0, inner=namespace(a=1)) %}{% set alias = ns %}{% set l = [ns.inner] %}"
    "{% set cyc = cycler(1, 2) %}{% set j = joiner('-') %}"
)


def test_module_objects_are_jinja2s_classes():
    # isinstance and callable() as of Jinja2's Namespace, Cycler and Joiner, without the module importing Jinja2 (0575)
    import jinja2.utils

    classes = (jinja2.utils.Namespace, jinja2.utils.Cycler, jinja2.utils.Joiner)
    ours, theirs = both(OBJECTS_SOURCE)
    for mod in (ours, theirs):
        objects = (mod.ns, mod.cyc, mod.j)
        assert [[isinstance(o, cls) for cls in classes] for o in objects] == [[True, False, False], [False, True, False], [False, False, True]]
        assert [callable(o) for o in objects] == [False, False, True]
        assert [type(o).__name__ for o in objects] == ["Namespace", "Cycler", "Joiner"]
        assert [o.__class__ for o in objects] == list(classes)
        with pytest.raises(AttributeError) as error:
            mod.ns.nope
        assert str(error.value) == "nope"
        with pytest.raises(AttributeError, match="'Cycler' object has no attribute 'nope'"):
            mod.cyc.nope


def test_module_objects_read_twice_are_one_object():
    # A nested namespace, and one exported under two names or in a list, is one Python object, as in Jinja2 (0575)
    ours, theirs = both(OBJECTS_SOURCE)
    for mod in (ours, theirs):
        assert mod.ns.inner is mod.ns.inner
        assert mod.alias is mod.ns
        assert mod.l[0] is mod.ns.inner
    # Python's object goes with the last reference to it, and the module makes another
    mod = jinja2cpp.Environment().from_string("{% set ns = namespace(inner=namespace(a=1)) %}").make_module()
    ref = weakref.ref(mod.ns.inner)
    gc.collect()
    assert ref() is None and mod.ns.inner.a == 1


def test_module_object_method_reprs_as_a_bound_method():
    ours, theirs = both(OBJECTS_SOURCE)
    for mod in (ours, theirs):
        for obj, method in ((mod.cyc, "next"), (mod.cyc, "reset")):
            cls = type(obj).__name__
            assert repr(getattr(obj, method)) == f"<bound method {cls}.{method} of {obj!r}>"
            assert repr(obj).startswith(f"<jinja2.utils.{cls} object at 0x")


def test_module_object_passed_to_a_render_prints_as_the_object():
    ours, theirs = both(OBJECTS_SOURCE)
    source = "{{ x }}|{{ [x] }}|{{ x.inner }}|{{ x.count }}|{{ c.next() }}{{ c.next() }}"
    assert jinja2cpp.Template(source).render(x=ours.ns, c=ours.cyc) == jinja2.Template(source).render(x=theirs.ns, c=theirs.cyc)
    for mod, module in ((ours, jinja2cpp), (theirs, jinja2)):
        assert module.Template("{{ c }}").render(c=mod.cyc).startswith("<jinja2.utils.Cycler object at 0x")


def test_module_macro_result_that_is_an_object():
    source = "{% set cyc = cycler(namespace(n=1), namespace(n=2)) %}"
    for module in (jinja2cpp, jinja2):
        mod = module.Environment().from_string(source).make_module()
        first = mod.cyc.next()
        assert first.n == 1 and mod.cyc.next().n == 2 and first.n == 1


def test_module_waits_without_the_gil():
    # A macro that sleeps in a Python callable holds the module; a read or a call from another thread waits for it
    # with the GIL let go, or the sleeping thread could never take the GIL back
    source = "{% set ns = namespace(count=0) %}{% macro slow(f) %}{{ f() }}{% endmacro %}{% macro inc() %}{% set ns.count = ns.count + 1 %}{% endmacro %}"
    mod = jinja2cpp.Environment().from_string(source).make_module()
    started = threading.Event()

    def pause():
        started.set()
        time.sleep(0.5)
        return ""

    worker = threading.Thread(target=lambda: mod.slow(pause))
    faulthandler.dump_traceback_later(30, exit=True)
    try:
        worker.start()
        assert started.wait(10)
        assert mod.ns.count == 0
        mod.inc()
        assert mod.ns.count == 1
        worker.join(10)
        assert not worker.is_alive()
        # The engine reading a live object handed back to it, in a render and in another module's macro
        env = jinja2cpp.Environment()
        other = env.from_string("{% macro peek(v) %}{{ v.count }}{% endmacro %}{% macro first(v) %}{{ v[0].count }}{% endmacro %}").make_module()
        for read in (lambda: env.from_string("{{ v.count }}").render(v=mod.ns), lambda: other.peek(mod.ns), lambda: other.first([mod.ns])):
            started.clear()
            worker = threading.Thread(target=lambda: mod.slow(pause))
            worker.start()
            assert started.wait(10)
            assert read() == "1"
            worker.join(10)
        assert not worker.is_alive()
    finally:
        faulthandler.cancel_dump_traceback_later()


def test_module_reentered_on_the_same_thread():
    source = "{% set ns = namespace(count=0) %}{% macro run(f) %}{{ f() }}{% endmacro %}{% macro inc() %}{% set ns.count = ns.count + 1 %}{% endmacro %}"
    mod = jinja2cpp.Environment().from_string(source).make_module()
    assert mod.run(lambda: str(mod.ns.count) + (mod.inc() or "") + str(mod.ns.count)) == "01"
