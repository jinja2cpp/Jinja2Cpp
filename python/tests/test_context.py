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
    both(make_env, "{{ show('w') }}{{ show('g', 'range') }}", w=[1])


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
    with pytest.raises(jinja2cpp.TemplateRuntimeError):
        kept[1].a


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
