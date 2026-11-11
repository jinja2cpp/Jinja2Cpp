"""Template.make_module and Template.module: exports, macros, lifetimes, and jinja2.meta."""

import gc
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
    # a global is visible, unless the context is shared (the engine adds its own builtins only)
    for module in (jinja2cpp, jinja2):
        env = module.Environment()
        env.globals["g"] = "G"
        out = env.from_string("{{ a }}-{{ g }}").make_module({"a": 1})
        assert str(out) == "1-G"
        assert str(env.from_string("{{ a }}-{{ g }}").make_module({"a": 1}, shared=True)) == "1-"


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
        # The host API has one empty value: an exported Undefined arrives as None (docs/python.md)
        assert (mod.u is None) if module is jinja2cpp else isinstance(mod.u, jinja2.Undefined)


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
    # an exported host list is not readable by the export copy yet (docs/tasks/0411)
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
