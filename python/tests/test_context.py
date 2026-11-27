"""pass_context, pass_eval_context and pass_environment, compared with Python Jinja2."""

import gc

import jinja2
import jinja2cpp
import pytest

MODULES = (jinja2cpp, jinja2)


def both(make, source, **render):
    """Renders `source` in an environment made by make(module) with both engines."""
    out = []
    for module in MODULES:
        env = make(module)
        try:
            out.append(env.from_string(source).render(**render))
        except Exception as error:  # noqa: BLE001 - compared by class name
            out.append(type(error).__name__)
    assert out[0] == out[1], f"{source!r}: jinja2cpp {out[0]!r}, Jinja2 {out[1]!r}"
    return out[0]


def make_env(module, loader=None, **options):
    @module.pass_context
    def show(ctx, *names):
        return "|".join(f"{n}={ctx.get(n, 'MISSING')!r}" for n in names) + f" name={ctx.name}"

    @module.pass_eval_context
    def ev(eval_ctx):
        return f"{eval_ctx.autoescape},{eval_ctx.volatile}"

    @module.pass_environment
    def pe(environment, value):
        return environment.globals["g"] + value

    env = module.Environment(loader=module.DictLoader(loader) if loader else None, **options)
    env.globals.update(show=show, ev=ev, g=3, pe=pe)
    env.filters["pe"] = pe
    return env


def test_context_variables():
    both(make_env, "{% set a = 1 %}{{ show('a', 'w', 'nope') }}", w=2)
    both(make_env, "{{ show('w') }}{{ show('g') }}", w=[1])


def test_a_global_function_read_through_the_context_is_callable():
    # The engine's own `range` reaches Python as a callable lent for the call
    @jinja2cpp.pass_context
    def call(ctx):
        return str(list(ctx.get("range")(3)))

    env = jinja2cpp.Environment()
    env.globals["call"] = call
    assert env.from_string("{{ call() }}").render() == "[0, 1, 2]"


@pytest.mark.parametrize(
    "source",
    [
        "{{ f(range) }}",
        "{{ f(dict) }}",
        "{{ f(cycler) }}",
        "{{ f(joiner) }}",
        "{{ f(namespace) }}",
        "{% macro m() %}{% endmacro %}{{ f(m) }}",
        "{% for x in [1, 2] %}{{ f(loop.cycle) }}{% endfor %}",
    ],
)
@pytest.mark.parametrize("show", [repr, str], ids=["repr", "str"])
def test_a_lent_engine_callable_prints_as_jinja2s(source, show):
    # A callable of the template lent to a Python callable prints as the object Jinja2 passes (task 0439)
    both(make_env, source, f=show)


@pytest.mark.parametrize("name", ["range", "dict", "cycler", "joiner", "namespace", "lipsum"])
def test_a_lent_global_is_pythons_own_object(name):
    # Jinja2 passes Python's own range, dict and jinja2.utils objects, and the object a Python callable gives back is the
    # template's global again (task 0559)
    import builtins

    import jinja2.utils

    objects = (builtins.range, builtins.dict, jinja2.utils.Cycler, jinja2.utils.Joiner, jinja2.utils.Namespace)
    objects += (jinja2.utils.generate_lorem_ipsum,)
    both(make_env, f"{{{{ f({name}) }}}}", f=lambda x: f"{type(x).__name__} {any(x is o for o in objects)}")
    both(make_env, f"{{{{ f({name}) is sameas {name} }}}}", f=lambda x: x)
    both(make_env, f"{{{{ (f({name}), {name})|map('pprint')|unique|list|length }}}}", f=lambda x: x)


def test_a_lent_global_read_through_the_context_is_pythons_own_object():
    @jinja2cpp.pass_context
    def get(ctx):
        return ctx.get("range") is range and ctx["dict"] is dict

    env = jinja2cpp.Environment()
    env.globals["get"] = get
    assert env.from_string("{{ get() }}").render() == "True"


def test_python_range_given_back_after_the_call_is_the_templates():
    # Past the call it was lent to, Python's range is still the template's, as in Jinja2 (task 0583)
    for module in (jinja2cpp, jinja2):
        kept = []

        def keep(x):
            kept.append(x)
            return ""

        source = "{{ keep(range) }}{{ back()(2)|list }} {{ back() is sameas range }}"
        assert module.Template(source).render(keep=keep, back=lambda: kept[0]) == "[0, 1] True", module


@pytest.mark.parametrize("name", ["range", "dict", "cycler", "joiner", "namespace", "lipsum"])
def test_pythons_own_global_given_to_a_render_is_the_templates(name):
    # render(r=range) and a global: Python's object is the template's global in Jinja2, which is Python's (task 0583)
    import jinja2.utils

    objects = {"range": range, "dict": dict, "cycler": jinja2.utils.Cycler, "joiner": jinja2.utils.Joiner}
    objects.update({"namespace": jinja2.utils.Namespace, "lipsum": jinja2.utils.generate_lorem_ipsum})
    source = f"{{{{ r is sameas {name} }}}} {{{{ g is sameas {name} }}}} {{{{ [r] == [{name}] }}}}"
    for module in (jinja2cpp, jinja2):
        env = module.Environment()
        env.globals["g"] = objects[name]
        assert env.from_string(source).render(r=objects[name]) == "True True True", (module, name)
    calls = {"range": "{{ r(1, 7, 2)|list }}", "dict": "{{ r(a=1) }}", "cycler": "{{ r(1, 2).next() }}{{ r(1, 2).current }}"}
    calls.update({"joiner": "{% set j = r('-') %}{{ j() }}{{ j() }}", "namespace": "{% set n = r(a=2) %}{{ n.a }}"})
    calls["lipsum"] = "{{ r(2, false).count(\"\\n\") }}"
    for source in (calls[name], "{{ r is callable }}", "{{ r.__name__ }}"):
        results = [module.Template(source).render(r=objects[name]) for module in (jinja2cpp, jinja2)]
        assert results[0] == results[1], (name, source)


def test_lipsum_lent_to_python_prints_as_a_function():
    out = jinja2cpp.Template("{{ f(lipsum) }}").render(f=repr)
    assert out.startswith("<function generate_lorem_ipsum at 0x"), out


def make_split_env(module):
    @module.pass_context
    def split(ctx):
        names = ("a", "c", "i", "w", "x", "y", "z", "lib", "m")
        return (
            f"vars={sorted(k for k in ctx.vars if k != 'split')} "
            f"parent={sorted(k for k in ctx.parent if k in names)} "
            f"keys={sorted(k for k in ctx.keys() if k in names)}"
        )

    env = module.Environment(
        loader=module.DictLoader({"lib": "{% set z = 1 %}{% macro q() %}{% endmacro %}", "inc": "{% set c = 1 %}{{ split() }}", "bare": "{{ split() }}"})
    )
    env.globals["split"] = split
    return env


@pytest.mark.parametrize(
    "source",
    [
        "{% set a = 1 %}{{ split() }}",
        "{% set a = 1 %}{% for i in [1] %}{{ split() }}{% endfor %}",
        "{% set a = 1 %}{% for i in [1] %}{{ split() }}{% set y = 2 %}{% endfor %}",
        "{% set a = 1 %}{% if true %}{% set y = 2 %}{{ split() }}{% endif %}",
        "{% set a = 1 %}{% with x = 3 %}{% set y = 2 %}{{ split() }}{% endwith %}",
        "{% set a = 1 %}{% block b %}{{ split() }}{% endblock %}",
        "{% macro m(x) %}{% set y = 1 %}{{ split() }}{% endmacro %}{% set a = 1 %}{{ m(5) }}",
        "{% for i in [1] %}{% macro m() %}{{ split() }}{% endmacro %}{{ m() }}{% endfor %}",
        "{{ split() }}{% set a = 1 %}",
        "{% import 'lib' as lib %}{% from 'lib' import z %}{{ split() }}",
        "{% set a = 1 %}{% include 'inc' %}",
        "{% set a = 1 %}{% include 'bare' %}",
        "{% set a = 1 %}{% for i in [1] %}{% include 'inc' %}{% endfor %}",
        "{% set w = 'shadow' %}{{ split() }}",
    ],
)
def test_context_splits_top_level_names_from_arguments(source):
    # Context.vars holds the template's top-level names, Context.parent its arguments and globals, and a loop variable,
    # a with name or a macro's local is in neither (task 0416)
    both(make_split_env, source, w=2)


def make_body_env(module):
    @module.pass_context
    def p(ctx):
        names = ("a", "b", "c", "i", "w", "loop", "_b")
        return (
            f"vars={sorted(k for k in ctx.vars if k != 'p')} i={ctx.get('i')!r} b={ctx.get('b')!r} loop={'loop' in ctx} "
            f"parent={sorted(k for k in ctx.parent if k in names)} keys={sorted(k for k in ctx.keys() if k in names)}"
        )

    @module.pass_context
    def pf(ctx, value):
        return f"{value}:{sorted(k for k in ctx.vars if k not in ('p', 'pf'))}:{ctx.get('b')!r}"

    env = module.Environment(loader=module.DictLoader({"base": "{% for i in [1] %}{% block x scoped %}{% endblock %}{% endfor %}"}))
    env.globals["p"] = p
    env.filters["pf"] = pf
    return env


@pytest.mark.parametrize(
    "source",
    [
        # The task's probe rows
        "{% set a = 1 %}{% for i in [1] %}{{ p() }}{% endfor %}",
        "{% set a = 1 %}{% for i in [1] %}{% set b = 2 %}{{ p() }}{% endfor %}",
        "{% set a = 1 %}{% block x %}{% set b = 1 %}{{ p() }}{% endblock %}",
        "{% with w = 3 %}{% set b = 2 %}{{ p() }}{% endwith %}",
        # A body's names count from its `set` on, in each pass
        "{% set b = 9 %}{% for i in [1, 2] %}{{ p() }}{% set b = i %}{{ p() }}{% endfor %}",
        "{% for i in [1] %}{% if true %}{% set b = 2 %}{% endif %}{{ p() }}{% endfor %}",
        "{% for i in [1] %}{% set b = 2 %}{% if true %}{{ p() }}{% endif %}{{ p() if true }}{% endfor %}",
        "{% for i in [1] %}{% set b = 2 %}{% with %}{{ p() }}{% endwith %}{% filter upper %}{{ p() }}{% endfilter %}{% endfor %}",
        "{% for i in [1] %}{% set i = 5 %}{{ p() }}{% endfor %}",
        "{% for i in [1] %}{% set _b = 2 %}{{ p() }}{% endfor %}",
        "{% for i in [1] %}{% set b, c = 1, 2 %}{{ p() }}{% endfor %}",
        "{% for i in [1] %}{% set b %}x{% endset %}{{ p() }}{% endfor %}",
        "{% block x %}{% set b = 1 %}{% for i in [1] %}{{ p() }}{% endfor %}{% endblock %}",
        "{% macro m() %}{% for i in [1] %}{% set b = 1 %}{{ p() }}{% endfor %}{% endmacro %}{{ m() }}",
        "{% macro m(i) %}{% set b = 1 %}{{ p() }}{% endmacro %}{{ m(3) }}",
        "{% for i in [1] %}{% set b = 2 %}{% for j in [1] %}{{ p() }}{% endfor %}{% endfor %}",
        "{% for i in [1] %}{% set b = 2 %}{{ 'v'|pf }}{% endfor %}",
        "{% set loop = 6 %}{{ p() }}",
        # A scoped block's context is derived from the names its call site sees
        "{% set a = 1 %}{% block x scoped %}{{ p() }}{% endblock %}",
        "{% for i in [1] %}{% block x scoped %}{{ p() }}{% endblock %}{% endfor %}",
        "{% for i in [1] %}{% set b = 2 %}{% block x scoped %}{{ p() }}{% endblock %}{% endfor %}",
        "{% for i in [1] %}{% block x scoped %}{% set b = 3 %}{{ p() }}{% endblock %}{% endfor %}",
        "{% extends 'base' %}{% block x %}{{ p() }}{% endblock %}",
    ],
)
def test_context_of_calls_in_loop_and_block_bodies(source):
    # Jinja2's Context.call hands a call written in a loop or block body, after a `set` there, context.derived(the names
    # those stored), and a scoped block gets context.derived(locals()) (task 0558)
    both(make_body_env, source, w=2)


def test_context_in_included_and_imported_templates():
    templates = {
        "inc": "[{{ show('a', 'z', 'w') }}]",
        "imp": "{% set z = 9 %}{% macro m() %}{{ show('z', 'a', 'w') }}{% endmacro %}",
        "main": "{% set a = 1 %}{% include 'inc' %}{% import 'imp' as i %}{{ i.m() }}{{ i.z }}{{ show('z') }}"
        "{% from 'imp' import m %}{{ m() }}{% include 'inc' without context %}",
    }
    out = []
    for module in MODULES:
        env = make_env(module, templates)
        out.append(env.get_template("main").render(w=2))
    assert out[0] == out[1]
    assert "name=inc" in out[0] and "name=imp" in out[0]


def test_name_of_a_template_from_a_string_is_none():
    assert both(make_env, "{{ show() }}") == " name=None"


def test_eval_context_follows_autoescape():
    source = "{{ ev() }}{% autoescape true %}{{ ev() }}{% autoescape false %}{{ ev() }}{% endautoescape %}{% endautoescape %}"
    assert both(make_env, source) == "False,FalseTrue,FalseFalse,False"
    assert both(lambda m: make_env(m, autoescape=True), source) == "True,FalseTrue,FalseFalse,False"
    assert both(make_env, "{% autoescape flag %}{{ ev() }}{% endautoescape %}", flag=True) == "True,False"


def test_pass_environment_filter_and_global():
    assert both(make_env, "{{ 4|pe }} {{ pe(5) }}") == "7 8"


def test_environment_is_the_render_environment():
    seen = []

    @jinja2cpp.pass_environment
    def probe(environment):
        seen.append(environment)
        return ""

    env = jinja2cpp.Environment()
    env.globals["probe"] = probe
    env.from_string("{{ probe() }}").render()
    assert seen == [env] and seen[0] is env
    # a render value is honoured as well, and so is a filter and a test
    env.filters["probe"] = jinja2cpp.pass_environment(lambda e, v: v)
    env.tests["probe"] = jinja2cpp.pass_environment(lambda e, v: e is env)
    assert env.from_string("{{ f() }}{{ 1|probe }}{{ 1 is probe }}").render(f=probe) == "1True"
    assert seen[1:] == [env]


def test_jinja2_decorators_are_honoured():
    @jinja2.pass_context
    def f(ctx, name):
        return ctx[name]

    env = jinja2cpp.Environment()
    env.globals["f"] = f
    assert env.from_string("{% set x = 5 %}{{ f('x') }}").render() == "5"


def test_pass_arg_of_methods_and_callable_objects():
    def make(module):
        class Thing:
            @module.pass_context
            def method(self, ctx, name):
                return ctx.get(name)

            @module.pass_eval_context
            def __call__(self, eval_ctx):
                return eval_ctx.autoescape

        env = module.Environment()
        env.globals["thing"] = Thing()
        return env

    assert both(make, "{% set x = 'v' %}{{ thing.method('x') }}{{ thing() }}") == "vFalse"


def test_pass_arg_with_call_arguments():
    def make(module):
        @module.pass_context
        def f(ctx, a, b=2, *rest, k=0):
            return f"{a}{b}{rest}{k}{ctx.get('x')}"

        env = module.Environment()
        env.globals["f"] = f
        return env

    both(make, "{% set x = 1 %}{{ f(1) }}{{ f(1, 3, 4, k=5) }}{{ f(a=7) }}")
    both(make, "{{ f() }}")


def test_context_mapping_api():
    results = []

    def make(module):
        @module.pass_context
        def probe(ctx):
            results.append(
                (
                    ctx.get("a"),
                    ctx["a"],
                    "a" in ctx,
                    "nope" in ctx,
                    ctx.resolve("a"),
                    ctx.resolve_or_missing("a"),
                    type(ctx.resolve("nope")).__name__,
                    ctx.get("nope", 7),
                    ctx.environment,
                    ctx.eval_ctx.environment,
                    ctx.get_all()["a"],
                )
            )
            try:
                ctx["nope"]
            except KeyError as error:
                results.append(error.args)
            return ""

        env = module.Environment()
        env.globals["probe"] = probe
        return env

    both(make, "{% set a = [1] %}{{ probe() }}")
    assert results[0][:8] == results[2][:8]
    assert results[1] == results[3] == ("nope",)
    assert results[0][8] is not results[2][8]


def test_context_listing():
    seen = {}

    @jinja2cpp.pass_context
    def probe(ctx):
        seen["keys"] = set(ctx.keys())
        seen["items"] = dict(ctx.items())
        seen["values"] = list(ctx.values())
        seen["vars"] = dict(ctx.vars)
        seen["parent"] = dict(ctx.parent)
        seen["len"] = len(ctx)
        return ""

    env = jinja2cpp.Environment()
    env.globals.update(probe=probe, g=1)
    env.from_string("{% set top = 2 %}{{ probe() }}").render(arg=3)
    assert {"top", "arg", "g", "probe", "range", "namespace"} <= seen["keys"]
    assert seen["items"]["top"] == 2 and seen["items"]["arg"] == 3 and seen["vars"]["top"] == 2 and seen["parent"]["g"] == 1
    assert seen["len"] == len(seen["keys"]) == len(seen["values"])
    assert "loop" not in seen["keys"]


def test_template_globals_are_seen():
    @jinja2cpp.pass_context
    def probe(ctx):
        return str(ctx.get("tg")) + str("tg" in ctx.get_all())

    env = jinja2cpp.Environment()
    env.globals["probe"] = probe
    assert env.from_string("{{ probe() }}", globals={"tg": 5}).render() == "5True"


def test_context_after_the_call_raises():
    kept = []

    @jinja2cpp.pass_context
    def keep(ctx):
        kept.append(ctx)
        return ctx.get("a")

    env = jinja2cpp.Environment()
    env.globals["keep"] = keep
    assert env.from_string("{% set a = 4 %}{{ keep() }}").render() == "4"
    ctx = kept[0]
    gc.collect()
    for use in (
        lambda: ctx.get("a"),
        lambda: ctx["a"],
        lambda: "a" in ctx,
        lambda: ctx.resolve("a"),
        lambda: ctx.resolve_or_missing("a"),
        lambda: ctx.get_all(),
        lambda: ctx.parent,
        lambda: ctx.vars,
        lambda: list(ctx.keys()),
        lambda: list(ctx),
        lambda: len(ctx),
    ):
        with pytest.raises(jinja2cpp.TemplateRuntimeError):
            use()
    # what the context itself holds stays usable
    assert ctx.environment is env and ctx.eval_ctx.autoescape is False and ctx.name is None
    kept.clear()
    del ctx
    gc.collect()
    # a second render is not disturbed by the dead one
    assert env.from_string("{% set a = 5 %}{{ keep() }}").render() == "5"


def test_values_read_through_a_context_are_lent_too():
    kept = []

    @jinja2cpp.pass_context
    def keep(ctx):
        kept.append(ctx.get("m"))
        kept.append(ctx.get("cnt"))
        return kept[0]("x")

    env = jinja2cpp.Environment()
    env.globals["keep"] = keep
    assert env.from_string("{% macro m(v) %}<{{ v }}>{% endmacro %}{% set cnt = namespace(a=1) %}{{ keep() }}").render() == "<x>"
    gc.collect()
    with pytest.raises(jinja2cpp.TemplateRuntimeError):
        kept[0]("y")
    # A namespace is a snapshot of its attributes, which stays readable (docs/tasks/0357, D5)
    assert kept[1].a == 1


def test_template_objects_print_as_in_jinja2():
    source = ("{% set ns = namespace(a=1) %}{% for i in [1, 2] %}"
              "{{ r(loop) }}|{{ s(loop) }}|{{ r(ns) }}|{{ s(ns) }};{% endfor %}")
    out = [module.Template(source).render(r=repr, s=str) for module in MODULES]
    assert out[0] == out[1]
    assert out[0].startswith("<LoopContext 1/2>|<LoopContext 1/2>|<Namespace {'a': 1}>")


def test_context_survives_nested_renders_and_errors():
    env = jinja2cpp.Environment()
    inner = env.from_string("{% set q = 3 %}{{ probe() }}")
    env.globals["inner"] = jinja2cpp.pass_context(lambda ctx: inner.render())

    @jinja2cpp.pass_context
    def probe(ctx):
        return str(ctx.get("q")) + str(ctx.get("outer"))

    env.globals["probe"] = probe
    assert env.from_string("{% set outer = 1 %}{{ inner() }}|{{ probe() }}").render() == "3None|None1"

    @jinja2cpp.pass_context
    def boom(ctx):
        raise ValueError(ctx.get("x"))

    env.globals["boom"] = boom
    with pytest.raises(ValueError):
        env.from_string("{% set x = 1 %}{{ boom() }}").render()
    assert env.from_string("{% set x = 2 %}{{ probe() }}").render() == "NoneNone"


def test_pass_arg_markers_reading_by_name():
    class Marker:
        name = "context"

    def f(ctx):
        return ctx.get("x")

    f.jinja_pass_arg = Marker()
    env = jinja2cpp.Environment()
    env.globals["f"] = f
    assert env.from_string("{% set x = 1 %}{{ f() }}").render() == "1"
    f.jinja_pass_arg = "context"  # a marker that is no enum member
    assert jinja2cpp.Environment().from_string("{{ 1 }}").render() == "1"


def test_filters_applied_by_map_select_and_finalize():
    def make(module):
        @module.pass_context
        def f(ctx, value):
            return f"{value}{ctx.get('x')}"

        @module.pass_environment
        def fin(environment, value):
            return f"<{value}{type(environment).__name__}>"

        @module.pass_eval_context
        def is_big(eval_ctx, value):
            return value > 1 and eval_ctx.autoescape is False

        env = module.Environment(finalize=fin)
        env.filters["f"] = f
        env.tests["big"] = is_big
        return env

    both(make, "{% set x = 7 %}{{ 1 }}{{ [1, 2]|map('f')|list }}{{ [1, 2, 3]|select('big')|list }}{{ 2|f }}{{ 3 is big }}")


_STASHED = []


def test_context_stored_in_a_global_is_dead_after_the_render():
    @jinja2cpp.pass_context
    def stash(ctx):
        _STASHED.append(ctx)
        # the loop variable and a macro are lent values: a value taken from the context is too
        return ctx.get("loop") is None

    env = jinja2cpp.Environment()
    env.globals["stash"] = stash
    for _ in range(3):
        env.from_string("{% macro m() %}{{ stash() }}{% endmacro %}{% for i in range(3) %}{{ m() }}{% endfor %}").render()
    gc.collect()
    for ctx in _STASHED:
        with pytest.raises(jinja2cpp.TemplateRuntimeError):
            ctx.get("i")
        with pytest.raises(jinja2cpp.TemplateRuntimeError):
            ctx.get_all()
    _STASHED.clear()
