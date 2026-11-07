"""The fallback to real Jinja2 (docs/python.md, "Fallback"): Tier B options, templates the engine cannot
parse, the include of an unparsable template, the three modes, jinja2cpp.stats() and the behavior without Jinja2."""

import asyncio
import os
import subprocess
import sys
import textwrap
import warnings

import jinja2
import jinja2.ext
import jinja2cpp
import jinja2cpp.ext
import jinja2cpp.sandbox
import pytest
from jinja2 import nodes
from jinja2cpp.nativetypes import NativeEnvironment

# A statement of Jinja2 the engine does not parse (a candidate; test_candidate_is_unparsable checks it)
UNPARSABLE = "a{% print 1 %}b"
UNPARSABLE_RESULT = "a1b"


@pytest.fixture(autouse=True)
def default_mode():
    previous = jinja2cpp.set_fallback("auto")
    jinja2cpp.stats(reset=True)
    yield
    jinja2cpp.set_fallback(previous)


class HelloExtension(jinja2.ext.Extension):
    """{% hello "name" %} prints "Hello name"."""

    tags = {"hello"}

    def parse(self, parser):
        lineno = next(parser.stream).lineno
        name = parser.parse_expression()
        return nodes.Output([nodes.Const("Hello "), name]).set_lineno(lineno)


class OurHelloExtension(jinja2cpp.ext.Extension):
    tags = {"hello"}

    def __init__(self, environment):
        super().__init__(environment)

    def parse(self, parser):
        lineno = next(parser.stream).lineno
        return nodes.Output([nodes.Const("Hello "), parser.parse_expression()]).set_lineno(lineno)


def test_candidate_is_unparsable():
    # If the engine learns this statement, pick another one for UNPARSABLE
    with pytest.raises(jinja2cpp.TemplateSyntaxError):
        jinja2cpp.Environment(fallback="never").from_string(UNPARSABLE)
    assert jinja2.Environment().from_string(UNPARSABLE).render() == UNPARSABLE_RESULT


# --- Tier B at construction ----------------------------------------------------------------


@pytest.mark.parametrize("extension", [HelloExtension, OurHelloExtension])
def test_custom_extension_delegates_the_environment(extension):
    env = jinja2cpp.Environment(extensions=[extension], loader=jinja2cpp.DictLoader({"a": '{% hello "a" %}!', "b": '{% include "a" %}{{ x }}'}))
    assert isinstance(env, jinja2cpp.Environment)
    assert env.get_template("a").render() == "Hello a!"
    assert env.get_template("b").render(x=1) == "Hello a!1"
    assert env.from_string('{% hello "w" %}').render() == "Hello w"
    assert env.extensions == [extension]
    assert jinja2cpp.stats()["environments_fallback"] == 1


def test_delegating_environment_has_the_attributes_and_live_dicts():
    loader = jinja2cpp.DictLoader({"t": "{{ x|twice }}{{ g }}"})
    env = jinja2cpp.Environment(extensions=[HelloExtension], loader=loader, autoescape=True, trim_blocks=True, undefined=jinja2cpp.StrictUndefined)
    assert env.loader is loader and env.autoescape is True and env.trim_blocks is True
    assert env.undefined is jinja2cpp.StrictUndefined and env.extensions == [HelloExtension]
    env.filters["twice"] = lambda v: v * 2
    env.globals["g"] = "G"
    template = env.get_template("t")
    assert template.render(x="<") == "&lt;&lt;G"
    assert template.name == "t"
    assert template.environment is env and isinstance(template, jinja2cpp.Template)
    assert template.globals["g"] == "G"
    assert repr(template).startswith("<Fallback")
    with pytest.raises(jinja2cpp.UndefinedError):
        env.from_string("{{ nope }}").render()
    env.autoescape = False
    assert env.from_string("{{ x }}").render(x="<") == "<"
    env.globals = {"g": 2}
    assert env.from_string("{{ g }}").render() == "2"


def test_delegating_environment_loaders_and_lookup():
    env = jinja2cpp.Environment(
        extensions=[HelloExtension],
        loader=jinja2cpp.ChoiceLoader([jinja2cpp.DictLoader({"a": "A"}), jinja2cpp.FunctionLoader(lambda n: "F:" + n if n == "f" else None)]),
    )
    assert env.get_template("a").render() == "A"
    assert env.get_template("f").render() == "F:f"
    assert env.select_template(["zz", "a"]).render() == "A"
    assert env.get_or_select_template(["zz", "f"]).render() == "F:f"
    assert jinja2cpp.Environment(extensions=[HelloExtension], loader=jinja2cpp.DictLoader({"b": "", "a": ""})).list_templates() == ["a", "b"]
    with pytest.raises(jinja2cpp.TemplateNotFound):
        env.get_template("zz")
    with pytest.raises(jinja2.TemplateNotFound):
        env.get_template("zz")
    with pytest.raises(jinja2cpp.TemplatesNotFound):
        env.select_template(["y", "z"])
    # a real Jinja2 loader
    real = jinja2cpp.Environment(extensions=[HelloExtension], loader=jinja2.DictLoader({"r": "R"}))
    assert real.get_template("r").render() == "R"


def test_delegating_environment_template_api():
    env = jinja2cpp.Environment(extensions=[HelloExtension])
    template = env.from_string("{% macro m(a) %}[{{ a }}]{% endmacro %}{% set v = 5 %}{{ m(v) }}")
    assert template.render() == "[5]"
    assert "".join(template.generate()) == "[5]"
    assert "".join(template.stream()) == "[5]"
    assert isinstance(template.stream(), jinja2cpp.TemplateStream)
    module = template.make_module()
    assert module.v == 5 and module.m(1) == "[1]"
    assert str(template.module) == "[5]"
    assert template.blocks == {}
    assert template.root_render_func is not None
    assert template.new_context({"a": 1}).get("a") == 1


def test_delegating_environment_pass_decorators_and_meta():
    @jinja2cpp.pass_context
    def with_context(context, value):
        return f"{context.get('k')}:{value}"

    @jinja2cpp.pass_environment
    def with_env(environment, value):
        return f"{type(environment).__name__}:{value}"

    env = jinja2cpp.Environment(extensions=[HelloExtension])
    env.filters["wc"] = with_context
    env.filters["we"] = with_env
    assert env.from_string("{{ 1|wc }} {{ 2|we }}").render(k="K") == "K:1 Environment:2"
    ast = env.parse("{{ a }}{% include 'b' %}")
    assert jinja2cpp.meta.find_undeclared_variables(ast) == {"a"}
    assert list(jinja2cpp.meta.find_referenced_templates(ast)) == ["b"]


def test_delegating_environment_mapped_exceptions():
    env = jinja2cpp.Environment(extensions=[HelloExtension])
    with pytest.raises(jinja2cpp.TemplateSyntaxError) as info:
        env.from_string("{% if %}")
    assert info.value.lineno == 1 and info.value.message == "Expected an expression, got 'end of statement block'"
    with pytest.raises(ZeroDivisionError):  # Jinja2's own Python exceptions are not changed
        env.from_string("{{ 1 // 0 }}").render()
    with pytest.raises(jinja2cpp.UndefinedError):
        jinja2cpp.Environment(extensions=[HelloExtension], undefined=jinja2cpp.StrictUndefined).from_string("{{ x }}").render()
    with pytest.raises(jinja2.TemplateSyntaxError):  # still Jinja2's class too
        env.from_string("{% if %}")


def test_enable_async():
    env = jinja2cpp.Environment(enable_async=True)
    template = env.from_string("{{ 1 + 1 }}")
    assert asyncio.run(template.render_async()) == "2"
    assert env.is_async is True
    assert jinja2cpp.stats()["environments_fallback"] == 1


def test_custom_context_and_generator_classes():
    class MyContext(jinja2.runtime.Context):
        pass

    class Env(jinja2cpp.Environment):
        context_class = MyContext

    env = Env()
    assert env.from_string("{{ 1 }}").render() == "1"
    assert jinja2cpp.stats()["environments_fallback"] == 1

    class Gen(jinja2.compiler.CodeGenerator):
        pass

    class Env2(jinja2cpp.Environment):
        code_generator_class = Gen

    assert Env2().from_string("{{ 2 }}").render() == "2"
    # Jinja2's own default classes are no reason
    class Env3(jinja2cpp.Environment):
        context_class = jinja2.runtime.Context

    Env3()
    assert jinja2cpp.stats()["environments_native"] == 1


def test_native_environment():
    env = NativeEnvironment()
    assert isinstance(env, jinja2cpp.Environment)
    assert env.from_string("{{ 1 + 2 }}").render() == 3
    assert env.from_string("{{ [1, 2] }}").render() == [1, 2]
    with pytest.raises(jinja2cpp.UnsupportedError):
        NativeEnvironment(fallback="never")


def test_template_constructor():
    template = jinja2cpp.Template('{% hello "t" %}', extensions=[HelloExtension])
    assert template.render() == "Hello t"
    assert isinstance(template, jinja2cpp.Template)
    plain = jinja2cpp.Template("{{ x }}")
    assert plain.render(x=1) == "1" and type(plain) is jinja2cpp.Template
    assert jinja2cpp.Template(UNPARSABLE).render() == UNPARSABLE_RESULT
    assert jinja2cpp.Template("{{ x }}", autoescape=True).render(x="<") == "&lt;"
    with pytest.raises(jinja2cpp.TemplateSyntaxError):
        jinja2cpp.Template("{% if %}")

    class Sub(jinja2cpp.Template):
        pass

    assert type(Sub("{{ 1 }}")) is Sub
    assert isinstance(Sub(UNPARSABLE), Sub) and Sub(UNPARSABLE).render() == UNPARSABLE_RESULT


def test_policies_send_templates_to_jinja2():
    env = jinja2cpp.Environment()
    assert env.policies == jinja2.defaults.DEFAULT_POLICIES
    before = jinja2cpp.stats()["loads_fallback"]
    env.policies["truncate.leeway"] = 0
    template = env.from_string("{{ x|truncate(8) }}")
    expected = jinja2.Environment()
    expected.policies["truncate.leeway"] = 0
    assert template.render(x="abcdefghijkl") == expected.from_string("{{ x|truncate(8) }}").render(x="abcdefghijkl")
    assert jinja2cpp.stats()["loads_fallback"] == before + 1
    env.policies["no.such.policy"] = 1
    assert env.from_string("{{ 1 }}").render() == "1"
    env2 = jinja2cpp.Environment(fallback="never")
    env2.policies["x.y"] = 1
    with pytest.raises(jinja2cpp.UnsupportedError, match="env.policies"):
        env2.from_string("{{ 1 }}")


# --- Tier B methods of a native environment and template -----------------------------------


def test_tier_b_methods_of_a_native_environment():
    env = jinja2cpp.Environment()
    expression = env.compile_expression("x + 1")
    assert expression(x=2) == 3
    assert [token[1] for token in env.lex("{{ a }}")][0] == "variable_begin"
    assert env.preprocess("{{ a }}") == "{{ a }}"
    assert env.compile("{{ a }}") is not None
    template = env.from_string("{% block a %}A{% endblock %}")
    assert list(template.blocks) == ["a"]
    assert template.root_render_func is not None
    assert template.new_context({"q": 1}).get("q") == 1
    with pytest.raises(RuntimeError, match="async"):
        asyncio.run(template.render_async())
    jinja2cpp.set_fallback("never")
    with pytest.raises(jinja2cpp.UnsupportedError):
        env.compile_expression("x")
    with pytest.raises(jinja2cpp.UnsupportedError):
        env.from_string("{% block a %}{% endblock %}").blocks


def test_tier_b_template_needs_its_source():
    env = jinja2cpp.Environment(loader=jinja2cpp.DictLoader({"a": "{% block b %}{% endblock %}"}))
    assert list(env.get_template("a").blocks) == ["b"]


# --- Templates the engine cannot parse ------------------------------------------------------


def test_unparsable_template_is_rendered_by_jinja2():
    env = jinja2cpp.Environment()
    template = env.from_string(UNPARSABLE)
    assert template.render() == UNPARSABLE_RESULT
    assert isinstance(template, jinja2cpp.Template) and template.environment is env
    stats = jinja2cpp.stats()
    assert stats["loads_fallback"] == 1 and stats["renders_fallback"] == 1 and stats["renders_native"] == 0
    loaded = jinja2cpp.Environment(loader=jinja2cpp.DictLoader({"p": UNPARSABLE})).get_template("p")
    assert loaded.render() == UNPARSABLE_RESULT and loaded.name == "p"


def test_unparsable_template_sees_the_environment():
    env = jinja2cpp.Environment(autoescape=True, undefined=jinja2cpp.StrictUndefined, loader=jinja2cpp.DictLoader({"inc": "<{{ q }}>"}))
    env.filters["shout"] = lambda value: str(value).upper() + "!"
    env.globals["gg"] = "G"
    template = env.from_string(UNPARSABLE + "{{ v|shout }}{{ gg }}{% include 'inc' %}")
    assert template.render(v="<x>", q="&") == "a1b&lt;X&gt;!G<&amp;>"
    with pytest.raises(jinja2cpp.UndefinedError):
        template.render(v="x")


def test_both_reject_raises_jinja2s_message_as_ours():
    source = "ok\n{% if %}\n{% endif %}"
    with pytest.raises(jinja2.TemplateSyntaxError) as theirs:
        jinja2.Environment().from_string(source)
    with pytest.raises(jinja2cpp.TemplateSyntaxError) as ours:
        jinja2cpp.Environment().from_string(source)
    assert ours.value.message == theirs.value.message
    assert (ours.value.lineno, ours.value.name, ours.value.filename) == (theirs.value.lineno, theirs.value.name, theirs.value.filename)
    assert ours.value.source == theirs.value.source
    # a message only Jinja2 words
    source = "{% for %}\n{% endfor %}"
    with pytest.raises(jinja2.TemplateSyntaxError) as theirs:
        jinja2.Environment().from_string(source)
    with pytest.raises(jinja2cpp.TemplateSyntaxError) as ours:
        jinja2cpp.Environment().from_string(source)
    assert ours.value.message == theirs.value.message
    with pytest.raises(jinja2cpp.TemplateSyntaxError) as loaded:
        jinja2cpp.Environment(loader=jinja2cpp.DictLoader({"bad": "x\n{% if %}"})).get_template("bad")
    with pytest.raises(jinja2.TemplateSyntaxError) as real:
        jinja2.Environment(loader=jinja2.DictLoader({"bad": "x\n{% if %}"})).get_template("bad")
    assert loaded.value.message == real.value.message and loaded.value.name == "bad"


def test_unknown_filter_message_is_jinja2s():
    with pytest.raises(jinja2cpp.TemplateAssertionError) as ours:
        jinja2cpp.Environment().from_string("{{ 1|nosuch }}")
    with pytest.raises(jinja2.TemplateAssertionError) as theirs:
        jinja2.Environment().from_string("{{ 1|nosuch }}")
    assert ours.value.message == theirs.value.message


def test_a_filter_added_to_the_environment_still_resolves_natively():
    env = jinja2cpp.Environment()
    env.filters["nosuch"] = str.upper
    assert env.from_string("{{ 'a'|nosuch }}").render() == "A"
    assert jinja2cpp.stats()["loads_fallback"] == 0


# --- An include of a template the engine cannot parse -----------------------------------------------------------------------


def make_includer(main, inc=UNPARSABLE, **options):
    return jinja2cpp.Environment(loader=jinja2cpp.DictLoader({"main": main, "inc": inc}), **options)


def test_include_of_an_unparsable_template_raises_the_engines_error():
    # The engine renders the includer and cannot parse the included template: its own error is raised.
    # (Routing the static names at load time is task 0430.)
    for main in ("{% include 'inc' %}", "{% import 'inc' as m %}", "{% from 'inc' import x %}", "{% extends 'inc' %}"):
        template = make_includer(main).get_template("main")
        with pytest.raises(jinja2cpp.TemplateSyntaxError) as info:
            template.render()
        assert "print" in info.value.message  # the engine's own error, as fallback="never" gives
    with pytest.raises(jinja2cpp.TemplateSyntaxError):
        make_includer("{% include 'inc' %}", fallback="never").get_template("main").render()
    stats = jinja2cpp.stats()
    assert stats["renders_fallback"] == 0 and stats["loads_fallback"] == 0 and stats["renders_native"] == 5
    # generate and stream raise it too, and a runtime error is the engine's as before
    template = make_includer("{% include 'inc' %}").get_template("main")
    with pytest.raises(jinja2cpp.TemplateSyntaxError):
        list(template.generate())
    with pytest.raises(jinja2cpp.TemplateRuntimeError):
        make_includer("{{ 1 // 0 }}{% include 'inc' %}").get_template("main").render()
    with pytest.raises(jinja2cpp.TemplateNotFound):
        make_includer("{% include 'missing' %}").get_template("main").render()


def test_include_of_an_unparsable_template_is_silent_in_warn_mode():
    with warnings.catch_warnings():
        warnings.simplefilter("error")
        with pytest.raises(jinja2cpp.TemplateSyntaxError):
            make_includer("{% include 'inc' %}", fallback="warn").get_template("main").render()


# --- Modes ----------------------------------------------------------------------------------


def test_never_raises_unsupported_for_every_tier_b_use():
    for options in ({"enable_async": True}, {"extensions": [HelloExtension]}, {"extensions": ["jinja2.ext.debug"]}):
        with pytest.raises(jinja2cpp.UnsupportedError) as info:
            jinja2cpp.Environment(fallback="never", **options)
        assert "fallback='never'" in str(info.value)
        assert isinstance(info.value, NotImplementedError)
    env = jinja2cpp.Environment(fallback="never")
    with pytest.raises(jinja2cpp.TemplateSyntaxError):
        env.from_string(UNPARSABLE)
    with pytest.raises(jinja2cpp.UnsupportedError):
        env.compile_expression("x")
    included = make_includer("{% include 'inc' %}", fallback="never")
    with pytest.raises(jinja2cpp.TemplateSyntaxError):
        included.get_template("main").render()
    with pytest.raises(jinja2cpp.UnsupportedError):
        jinja2cpp.Template("x", enable_async=True, fallback="never")
    assert jinja2cpp.stats()["loads_fallback"] == 0


def test_module_default_forces_never():
    assert jinja2cpp.get_fallback() == "auto"
    assert jinja2cpp.set_fallback("never") == "auto"
    with pytest.raises(jinja2cpp.UnsupportedError):
        jinja2cpp.Environment(enable_async=True)
    with pytest.raises(jinja2cpp.TemplateSyntaxError):
        jinja2cpp.Environment().from_string(UNPARSABLE)
    # the environment's own setting wins
    assert jinja2cpp.Environment(fallback="auto").from_string(UNPARSABLE).render() == UNPARSABLE_RESULT
    with pytest.raises(ValueError):
        jinja2cpp.set_fallback("sometimes")
    with pytest.raises(ValueError):
        jinja2cpp.Environment(fallback="sometimes")


def test_environment_variable_sets_the_default():
    script = "import jinja2cpp; print(jinja2cpp.get_fallback()); jinja2cpp.Environment(enable_async=True)"
    result = subprocess.run([sys.executable, "-c", script], capture_output=True, text=True, check=False, env=process_env(JINJA2CPP_FALLBACK="never"))
    assert result.stdout.strip() == "never" and "UnsupportedError" in result.stderr
    result = subprocess.run([sys.executable, "-c", "import jinja2cpp; jinja2cpp.get_fallback()"], capture_output=True, text=True, check=False, env=process_env(JINJA2CPP_FALLBACK="bogus"))
    assert "ValueError" in result.stderr


def test_warn_once_per_reason():
    with warnings.catch_warnings(record=True) as caught:
        warnings.simplefilter("always")
        jinja2cpp.Environment(fallback="warn", enable_async=True)
        jinja2cpp.Environment(fallback="warn", enable_async=True)
        jinja2cpp.Environment(fallback="warn", extensions=[HelloExtension])
        env = jinja2cpp.Environment(fallback="warn")
        env.from_string(UNPARSABLE)
        env.from_string(UNPARSABLE)
    messages = [str(w.message) for w in caught if issubclass(w.category, jinja2cpp.FallbackWarning)]
    assert len(messages) == 3, messages
    assert "enable_async=True" in messages[0] and "HelloExtension" in messages[1]
    assert all(message.startswith("jinja2cpp falls back to Jinja2: ") for message in messages)
    # reset forgets what was warned
    jinja2cpp.stats(reset=True)
    with warnings.catch_warnings(record=True) as caught:
        warnings.simplefilter("always")
        jinja2cpp.Environment(fallback="warn", enable_async=True)
    assert len(caught) == 1
    # the warning points at the caller
    with pytest.warns(jinja2cpp.FallbackWarning) as record:
        jinja2cpp.stats(reset=True)
        jinja2cpp.Environment(fallback="warn", enable_async=True)
    assert record[0].filename == __file__


# --- Statistics -----------------------------------------------------------------------------


def test_stats_count_native_and_fallback():
    keys = {"environments_native", "environments_fallback", "loads_native", "loads_fallback", "renders_native", "renders_fallback"}
    assert set(jinja2cpp.stats()) == keys and all(v == 0 for v in jinja2cpp.stats().values())
    env = jinja2cpp.Environment(loader=jinja2cpp.DictLoader({"a": "{{ 1 }}"}))
    env.from_string("{{ 1 }}").render()
    env.get_template("a").render()
    env.get_template("a").render()
    assert jinja2cpp.stats() == {"environments_native": 1, "environments_fallback": 0, "loads_native": 3, "loads_fallback": 0, "renders_native": 3, "renders_fallback": 0}
    jinja2cpp.Environment(enable_async=True).from_string("x").render()
    env.from_string(UNPARSABLE).render()
    assert jinja2cpp.stats() == {"environments_native": 1, "environments_fallback": 1, "loads_native": 3, "loads_fallback": 2, "renders_native": 3, "renders_fallback": 2}
    snapshot = jinja2cpp.stats(reset=True)
    assert snapshot["renders_fallback"] == 2
    assert all(v == 0 for v in jinja2cpp.stats().values())
    env.from_string("{{ 1 }}").render()
    assert jinja2cpp.stats()["renders_native"] == 1


# --- Sandbox --------------------------------------------------------------------------------


def test_sandbox_delegates_to_jinja2s_sandbox():
    env = jinja2cpp.sandbox.SandboxedEnvironment(extensions=[HelloExtension])
    assert env.sandboxed and isinstance(env, jinja2cpp.sandbox.SandboxedEnvironment)
    assert env.from_string('{% hello "s" %}').render() == "Hello s"
    with pytest.raises(jinja2cpp.SecurityError):
        env.from_string("{{ x.__class__() }}").render(x=1)
    assert env.from_string("{{ x.__class__ }}").render(x=1) == ""
    with pytest.raises(OverflowError):
        env.from_string("{{ range(10**6)|list }}").render()
    immutable = jinja2cpp.sandbox.ImmutableSandboxedEnvironment(extensions=[HelloExtension])
    with pytest.raises(jinja2cpp.SecurityError):
        immutable.from_string("{{ l.append(1) }}").render(l=[])


def test_sandbox_parse_fallback_and_stock_policy():
    env = jinja2cpp.sandbox.SandboxedEnvironment()
    assert env.from_string(UNPARSABLE).render() == UNPARSABLE_RESULT
    with pytest.raises(jinja2cpp.SecurityError):
        jinja2cpp.sandbox.SandboxedEnvironment().from_string("{% print x.__class__() %}").render(x=1)


def test_sandbox_with_its_own_policy_never_falls_back():
    class Mine(jinja2cpp.sandbox.SandboxedEnvironment):
        def wrap_str_format(self, value):
            return None

    with pytest.raises(jinja2cpp.UnsupportedError, match="overrides wrap_str_format"):
        Mine(extensions=[HelloExtension])
    with pytest.raises(jinja2cpp.UnsupportedError, match="overrides wrap_str_format"):
        Mine().from_string(UNPARSABLE)
    with pytest.raises(jinja2cpp.TemplateSyntaxError):  # Jinja2 rejects too: the syntax error stands
        Mine().from_string("{% if %}")
    class Intercepting(jinja2cpp.sandbox.SandboxedEnvironment):
        intercepted_binops = frozenset(["+"])

    with pytest.raises(jinja2cpp.UnsupportedError, match="intercepts"):
        Intercepting(enable_async=True)
    env = jinja2cpp.sandbox.SandboxedEnvironment()
    env.call_unop = lambda context, operator, arg: arg
    with pytest.raises(jinja2cpp.UnsupportedError):
        env.from_string(UNPARSABLE)
    included = Mine(loader=jinja2cpp.DictLoader({"main": "{% include 'inc' %}", "inc": UNPARSABLE}))
    with pytest.raises(jinja2cpp.TemplateSyntaxError):
        included.get_template("main").render()


# --- The loaders inside Jinja2 --------------------------------------------------------------


def test_file_system_loader_stays_confined_for_jinja2(tmp_path):
    outside = tmp_path / "outside.html"
    outside.write_text("SECRET")
    root = tmp_path / "root"
    root.mkdir()
    try:
        (root / "link.html").symlink_to(outside)
    except OSError:
        pytest.skip("no symlinks")
    (root / "ok.html").write_text("ok")
    env = jinja2cpp.Environment(extensions=[HelloExtension], loader=jinja2cpp.FileSystemLoader(str(root)))
    assert env.get_template("ok.html").render() == "ok"
    with pytest.raises(jinja2cpp.TemplateNotFound):
        env.get_template("link.html")


def test_loader_with_its_own_load_is_used():
    seen = []

    class Mine(jinja2cpp.BaseLoader):
        def get_source(self, environment, name):
            return "src", None, None

        def load(self, environment, name, globals=None):
            seen.append(name)
            return environment.from_string("mine:" + name)

    env = jinja2cpp.Environment(extensions=[HelloExtension], loader=Mine())
    assert env.get_template("x").render() == "mine:x"
    assert seen == ["x"]


# --- Without Jinja2 -------------------------------------------------------------------------


def run_without_jinja2(body):
    script = (
        textwrap.dedent(
            """
        import sys

        class Block:
            def find_spec(self, name, path=None, target=None):
                if name == "jinja2" or name.startswith("jinja2."):
                    raise ImportError("jinja2 is blocked")

        sys.meta_path.insert(0, Block())
        import jinja2cpp
        import jinja2cpp.ext

        UNPARSABLE = %r
        """
        )
        % UNPARSABLE
        + textwrap.dedent(body)
    )
    result = subprocess.run([sys.executable, "-c", script], capture_output=True, text=True, check=False, env=process_env())
    assert result.returncode == 0 and result.stdout.strip().endswith("ok"), result.stdout + result.stderr


def process_env(**extra):
    env = {**os.environ, "PYTHONPATH": ":".join(p for p in sys.path if p)}
    env.pop("JINJA2CPP_FALLBACK", None)
    env.update(extra)
    return env


def test_without_jinja2_tier_b_raises_unsupported():
    run_without_jinja2(
        """
        class Mine(jinja2cpp.ext.Extension):
            tags = {"x"}

        for mode in (None, "auto", "warn"):
            kwargs = {} if mode is None else {"fallback": mode}
            for options in ({"enable_async": True}, {"extensions": [Mine]}, {"extensions": ["jinja2.ext.debug"]}):
                try:
                    jinja2cpp.Environment(**options, **kwargs)
                except jinja2cpp.UnsupportedError as error:
                    assert "Jinja2 is not installed" in str(error), error
                else:
                    raise SystemExit("no error for %r" % (options,))
        from jinja2cpp.nativetypes import NativeEnvironment
        try:
            NativeEnvironment()
        except jinja2cpp.UnsupportedError as error:
            assert "NativeEnvironment" in str(error)
        else:
            raise SystemExit("no error")
        env = jinja2cpp.Environment()
        for call in (lambda: env.compile_expression("x"), lambda: env.lex("x"), lambda: env.from_string("x").blocks):
            try:
                call()
            except jinja2cpp.UnsupportedError:
                pass
            else:
                raise SystemExit("no error")
        env.policies["x.y"] = 1
        try:
            env.from_string("x")
        except jinja2cpp.UnsupportedError:
            pass
        else:
            raise SystemExit("no error")
        print("ok")
        """
    )


def test_without_jinja2_the_engine_error_stands():
    run_without_jinja2(
        """
        env = jinja2cpp.Environment()
        try:
            env.from_string(UNPARSABLE)
        except jinja2cpp.TemplateSyntaxError:
            pass
        else:
            raise SystemExit("no error")
        env = jinja2cpp.Environment(loader=jinja2cpp.DictLoader({"main": "{% include 'inc' %}", "inc": UNPARSABLE}))
        try:
            env.get_template("main").render()
        except jinja2cpp.TemplateSyntaxError:
            pass
        else:
            raise SystemExit("no error")
        assert env.from_string("{{ 1 }}").render() == "1"
        assert jinja2cpp.Template("{{ 2 }}").render() == "2"
        stats = jinja2cpp.stats()
        assert stats["environments_fallback"] == 0 and stats["loads_fallback"] == 0 and stats["renders_fallback"] == 0, stats
        assert stats["renders_native"] >= 3, stats
        print("ok")
        """
    )


def test_without_jinja2_the_native_extensions_work():
    run_without_jinja2(
        """
        env = jinja2cpp.Environment(extensions=["jinja2.ext.do", jinja2cpp.ext.loopcontrols, "jinja2cpp.ext.i18n"])
        assert env.from_string("{% do 1 %}{% for i in [1, 2] %}{% break %}{% endfor %}x").render() == "x"
        print("ok")
        """
    )


# --- Review round 1: the verifier's findings ------------------------------------------------


class Ext(jinja2.ext.Extension):
    """An extension that adds nothing, to make an environment Tier B."""


class Secret:
    secret = 42
    shown = 1

    def act(self):
        return "acted"


def _oracle_and_ours(make_ours, make_oracle, source, **ctx):
    ours = make_ours().from_string(source).render(**ctx)
    return ours, make_oracle().from_string(source).render(**ctx)


@pytest.mark.parametrize("tier", ["B", "C"])
def test_sandbox_hooks_set_after_construction_are_live(tier):
    # B: a custom extension makes Jinja2 the environment; C: the engine renders, Jinja2 takes a template it cannot parse
    def make(module):
        return module.SandboxedEnvironment(extensions=[Ext]) if tier == "B" else module.SandboxedEnvironment()

    suffix = "" if tier == "B" else "{% print 0 %}"
    ours, oracle = make(jinja2cpp.sandbox), jinja2.sandbox.SandboxedEnvironment(extensions=[Ext])
    early = ours.from_string("{{ x.secret }}{{ x.shown }}" + suffix)  # made before the hook exists
    for env in (ours, oracle):
        env.is_safe_attribute = lambda obj, attr, value: attr != "secret"
    expected = oracle.from_string("{{ x.secret }}{{ x.shown }}{% if 0 %}{% endif %}").render(x=Secret())
    assert expected == "1"
    assert early.render(x=Secret()) == "1" + ("0" if tier == "C" else "")
    assert ours.from_string("{{ x.secret }}{{ x.shown }}" + suffix).render(x=Secret()) == "1" + ("0" if tier == "C" else "")


def test_sandbox_hook_patched_onto_the_class_is_live():
    env = jinja2cpp.sandbox.SandboxedEnvironment(extensions=[Ext])

    class Mine(jinja2cpp.sandbox.SandboxedEnvironment):
        pass

    mine = Mine(extensions=[Ext])
    assert mine.from_string("{{ x.secret }}").render(x=Secret()) == "42"
    Mine.is_safe_attribute = lambda self, obj, attr, value: attr != "secret"
    assert mine.from_string("{{ x.secret }}{{ x.shown }}").render(x=Secret()) == "1"
    assert env.from_string("{{ x.secret }}").render(x=Secret()) == "42"  # not the base class


def test_sandbox_hook_set_in_a_subclass_init_is_live():
    class Mine(jinja2cpp.sandbox.SandboxedEnvironment):
        def __init__(self, *args, **kwargs):
            super().__init__(*args, **kwargs)
            self.is_safe_attribute = lambda obj, attr, value: attr != "secret"

    class Oracle(jinja2.sandbox.SandboxedEnvironment):
        def __init__(self, *args, **kwargs):
            super().__init__(*args, **kwargs)
            self.is_safe_attribute = lambda obj, attr, value: attr != "secret"

    assert Mine(extensions=[Ext]).from_string("{{ x.secret }}{{ x.shown }}").render(x=Secret()) == "1"
    assert Oracle(extensions=[Ext]).from_string("{{ x.secret }}{{ x.shown }}").render(x=Secret()) == "1"


def test_sandbox_is_safe_callable_is_live_and_unsafe_undefined_too():
    ours = jinja2cpp.sandbox.SandboxedEnvironment(extensions=[Ext])
    oracle = jinja2.sandbox.SandboxedEnvironment(extensions=[Ext])
    for env in (ours, oracle):
        template = env.from_string("{{ x.act() }}")
        assert template.render(x=Secret()) == "acted"
        env.is_safe_callable = lambda obj: False
        with pytest.raises(jinja2.exceptions.SecurityError):
            template.render(x=Secret())
        with pytest.raises(jinja2.exceptions.SecurityError):
            env.from_string("{{ x.act() }}").render(x=Secret())
    seen = []

    class Loud(jinja2cpp.sandbox.SandboxedEnvironment):
        def is_safe_attribute(self, obj, attr, value):
            return False

        def unsafe_undefined(self, obj, attribute):
            seen.append(attribute)
            return super().unsafe_undefined(obj, attribute)

    Loud(extensions=[Ext]).from_string("{{ x.shown }}").render(x=Secret())
    assert seen == ["shown"]


def test_sandbox_overrides_that_cannot_be_given_still_refuse():
    env = jinja2cpp.sandbox.SandboxedEnvironment(extensions=[Ext])
    env.getattr = lambda obj, attr: None
    with pytest.raises(jinja2cpp.UnsupportedError):
        env.from_string("{{ 1 }}")


def test_context_class_and_code_generator_class_reach_jinja2():
    class Ctx(jinja2.runtime.Context):
        def resolve_or_missing(self, key):
            if key == "magic":
                return "MAGIC"
            return super().resolve_or_missing(key)

    class Ours(jinja2cpp.Environment):
        context_class = Ctx

    class Theirs(jinja2.Environment):
        context_class = Ctx

    assert Theirs().from_string("{{ magic }}").render() == "MAGIC"
    assert Ours().from_string("{{ magic }}").render() == "MAGIC"
    assert Ours(loader=jinja2cpp.DictLoader({"t": "{{ magic }}{{ x }}"})).get_template("t").render(x=1) == "MAGIC1"

    seen = []

    class Gen(jinja2.compiler.CodeGenerator):
        def visit_Template(self, node, frame=None):
            seen.append(1)
            return super().visit_Template(node, frame)

    class OursGen(jinja2cpp.Environment):
        code_generator_class = Gen

    assert OursGen().from_string("{{ 1 }}").render() == "1"
    assert seen


def test_fallback_templates_are_cached_like_jinja2s():
    loader = jinja2cpp.DictLoader({"a": "{% print 1 %}", "b": "{% print 2 %}"})
    env = jinja2cpp.Environment(loader=loader)
    first = env.get_template("a")
    assert env.get_template("a") is first
    assert env.get_template("b") is not first
    assert jinja2cpp.stats()["loads_fallback"] == 2  # real loads only
    assert env.select_template(["zz", "a"]) is first
    assert jinja2cpp.stats()["loads_fallback"] == 2
    oracle = jinja2.Environment(loader=jinja2.DictLoader({"a": "{% print 1 %}"}))
    assert oracle.get_template("a") is oracle.get_template("a")
    # Tier B wrappers too
    delegating = jinja2cpp.Environment(extensions=[Ext], loader=jinja2cpp.DictLoader({"a": "x"}))
    assert delegating.get_template("a") is delegating.get_template("a")
    # from_string is not cached, as in Jinja2
    assert env.from_string("{% print 1 %}") is not env.from_string("{% print 1 %}")


def test_fallback_template_is_reloaded_when_the_loader_says_it_changed():
    source = ["{% print 1 %}"]
    stamp = [1]
    loader = jinja2cpp.FunctionLoader(lambda name: (source[0], None, lambda: stamp[0] == 1))
    env = jinja2cpp.Environment(loader=loader)
    first = env.get_template("t")
    assert first.render() == "1"
    source[0], stamp[0] = "{% print 2 %}", 2
    second = env.get_template("t")
    assert second is not first and second.render() == "2"


def test_delegate_environment_is_rebuilt_when_the_settings_change():
    env = jinja2cpp.Environment(loader=jinja2cpp.DictLoader({"a": "{% print x %}"}))
    assert env.get_template("a").render(x="<") == "<"
    env.autoescape = True
    assert env.get_template("a").render(x="<") == "&lt;"


def test_fallback_templates_see_the_live_globals_filters_and_tests():
    env = jinja2cpp.Environment()
    env.filters["up"] = str.upper
    env.tests["odd2"] = lambda v: v % 2 == 1
    template = env.from_string("{% print g %}{{ v|up }}{{ 1 is odd2 }}")
    env.globals["g"] = "G"  # after the template was made
    assert template.render(v='a') == "GATrue"
    env.globals["g"] = "H"
    assert template.render(v='a') == "HATrue"
    env.filters["up"] = lambda v: "<" + v + ">"  # a filter replaced after: the template sees it
    assert template.render(v='a') == "H<a>True"
    template.globals["own"] = 1
    assert "own" not in env.globals


def test_template_class_without_overrides_of_render_methods_works_on_fallback_templates():
    class Mine(jinja2cpp.Template):
        def shout(self):
            return "!"

        def __repr__(self):
            return "MINE"

    class Env(jinja2cpp.Environment):
        template_class = Mine

    env = Env(loader=jinja2cpp.DictLoader({"a": "{% print 1 %}", "n": "{{ 1 }}"}))
    assert env.get_template("a").render() == "1"
    assert env.get_template("a").shout() == "!" and isinstance(env.get_template("a"), Mine)
    assert repr(env.get_template("a")) == "MINE"
    assert Env(extensions=[Ext]).from_string("{{ 3 }}").render() == "3"
    assert env.get_template("n").shout() == "!"
    # the engine's own methods are untouched
    assert jinja2cpp.Template.render is jinja2cpp._jinja2cpp.Template.render


def test_template_class_that_overrides_render_is_refused_for_fallback_templates():
    class Mine(jinja2cpp.Template):
        def render(self, *args, **kwargs):
            return "<" + super().render(*args, **kwargs) + ">"

    class Env(jinja2cpp.Environment):
        template_class = Mine

    env = Env(loader=jinja2cpp.DictLoader({"a": "{% print 1 %}", "n": "{{ 1 }}"}))
    assert env.get_template("n").render() == "<1>"  # native templates run the override
    with pytest.raises(jinja2cpp.UnsupportedError, match="render"):
        env.get_template("a")
    with pytest.raises(jinja2cpp.UnsupportedError, match="Mine"):
        Env(extensions=[Ext]).from_string("{{ 3 }}")


def test_custom_undefined_subclass_is_refused_with_its_name():
    class Mine(jinja2cpp.Undefined):
        pass

    env = jinja2cpp.Environment(undefined=Mine)
    assert env.from_string("{{ x }}").render() == ""
    with pytest.raises(jinja2cpp.UnsupportedError, match="Mine"):
        env.from_string(UNPARSABLE)
    with pytest.raises(jinja2cpp.UnsupportedError, match="Mine"):
        jinja2cpp.Environment(undefined=Mine, extensions=[Ext])

    class Theirs(jinja2.Undefined):
        pass

    assert jinja2cpp.Environment(undefined=Theirs, extensions=[Ext]).from_string("[{{ x }}]").render() == "[]"
    assert jinja2cpp.Environment(undefined=jinja2cpp.StrictUndefined, extensions=[Ext]).undefined is jinja2cpp.StrictUndefined


def test_parse_and_meta_for_a_source_the_engine_cannot_parse():
    from jinja2cpp import meta

    env = jinja2cpp.Environment(loader=jinja2cpp.DictLoader({"t": "{% print x %}{% include 'o' %}", "o": "{{ 1 }}"}))
    ast = env.parse("{% print x %}{{ y }}")
    assert meta.find_undeclared_variables(ast) == {"x", "y"}
    ast = env.parse("{% print x %}{% extends 'base' %}{% include 'q' %}")
    assert sorted(meta.find_referenced_templates(ast)) == ["base", "q"]
    template = env.get_template("t")
    assert meta.find_undeclared_variables(template) == {"x"}
    assert list(meta.find_referenced_templates(template)) == ["o"]
    assert meta.find_undeclared_variables(env.from_string("{% print a %}{{ b }}")) == {"a", "b"}
    assert template._undeclared() == {"x"}
    assert template._referenced() == ["o"]
    module = env.from_string("{% print 1 %}{% set v = 5 %}").make_module()
    assert str(module).strip() == "1" and module.v == 5
    with pytest.raises(jinja2cpp.TemplateSyntaxError):
        env.parse("{% if %}")
    with pytest.raises(jinja2cpp.TemplateSyntaxError):
        jinja2cpp.Environment(fallback="never").parse(UNPARSABLE)


def test_stats_counters_are_exact_under_threads():
    import threading

    env = jinja2cpp.Environment()
    template = env.from_string("{% print 1 %}")

    def work():
        for _ in range(500):
            template.render()

    threads = [threading.Thread(target=work) for _ in range(4)]
    for thread in threads:
        thread.start()
    for thread in threads:
        thread.join()
    assert jinja2cpp.stats()["renders_fallback"] == 2000


def test_jinja2_exception_is_the_cause_of_the_mapped_one():
    env = jinja2cpp.Environment(extensions=[Ext])
    with pytest.raises(jinja2cpp.UndefinedError) as info:
        env.from_string("{{ x.y }}", ).render(x=jinja2.Undefined(name="x"))
    assert isinstance(info.value.__cause__, jinja2.exceptions.UndefinedError)
    assert type(info.value.__cause__) is not type(info.value) or info.value.__cause__ is not info.value


def test_warning_points_at_the_caller_of_a_tier_c_load():
    env = jinja2cpp.Environment(fallback="warn", loader=jinja2cpp.DictLoader({"a": UNPARSABLE}))
    for call in (lambda: env.from_string(UNPARSABLE), lambda: env.get_template("a"), lambda: jinja2cpp.Template(UNPARSABLE, fallback="warn"), lambda: env.select_template(["a"])):
        jinja2cpp.stats(reset=True)
        with warnings.catch_warnings(record=True) as caught:
            warnings.simplefilter("always")
            call()
        assert [w.filename for w in caught] == [__file__], [(w.filename, w.lineno) for w in caught]
    jinja2cpp.stats(reset=True)
    with warnings.catch_warnings(record=True) as caught:
        warnings.simplefilter("always")
        jinja2cpp.sandbox.SandboxedEnvironment(fallback="warn").from_string(UNPARSABLE)
    assert [w.filename for w in caught] == [__file__]


# --- Review round 2: the owner reference ------------------------------------------------------


def test_compile_expression_keeps_the_sandbox_policy_of_a_collected_environment():
    import gc

    def factory():
        env = jinja2cpp.sandbox.SandboxedEnvironment(extensions=[Ext])
        env.is_safe_attribute = lambda obj, attr, value: attr != "secret"
        return env.compile_expression("x.secret")

    expression = factory()
    gc.collect()
    assert expression(x=Secret()) is None  # the refused attribute is undefined: None with undefined_to_none
    assert jinja2cpp.sandbox.SandboxedEnvironment(extensions=[Ext]).compile_expression("x.secret")(x=Secret()) == 42


def test_fallback_templates_keep_the_policy_after_the_environment_is_dropped():
    import gc

    def factory():
        env = jinja2cpp.sandbox.SandboxedEnvironment()
        env.is_safe_attribute = lambda obj, attr, value: attr != "secret"
        return env.from_string("{{ x.secret }}{{ x.shown }}{% print 0 %}")

    template = factory()
    gc.collect()
    assert template.render(x=Secret()) == "10"


def test_a_delegate_without_an_owner_fails_closed():
    from jinja2cpp import fallback

    jenv = fallback.build_environment(jinja2cpp.sandbox.SandboxedEnvironment(), None)
    del jenv.__dict__["_jinja2cpp_owner"]
    with pytest.raises(jinja2.exceptions.SecurityError):
        jenv.from_string("{{ x.shown }}").render(x=Secret())


def test_environments_are_freed_by_the_collector():
    import gc
    import weakref

    refs = []
    for make in (
        lambda: jinja2cpp.Environment(extensions=[Ext]),
        lambda: jinja2cpp.sandbox.SandboxedEnvironment(extensions=[Ext]),
        lambda: jinja2cpp.Environment(),
    ):
        env = make()
        template = env.from_string("{{ 1 }}")
        template.render()
        if env.__class__ is jinja2cpp.Environment and "_delegate" not in env.__dict__:
            env.from_string(UNPARSABLE).render()  # builds the Jinja2 environment of a native one
        refs += [weakref.ref(env), weakref.ref(template)]
        del env, template
    gc.collect()
    assert [r() for r in refs] == [None] * len(refs)


# --- Review round 2: small fixes ---------------------------------------------------------------


def test_class_caches_do_not_keep_user_classes_alive():
    import gc
    import weakref

    class Mine(jinja2cpp.Template):
        pass

    class Env(jinja2cpp.sandbox.SandboxedEnvironment):
        template_class = Mine

    env = Env(extensions=[Ext])
    env.from_string("{{ 1 }}").render()
    Env(loader=jinja2cpp.DictLoader({"a": UNPARSABLE})).get_template("a").render()
    refs = [weakref.ref(Mine), weakref.ref(Env)]
    del env, Mine, Env
    gc.collect()
    assert [r() for r in refs] == [None, None]


def test_template_source_keyword_is_the_source():
    assert jinja2cpp.Template(source="{{ 1 }}").render() == "1"
    assert jinja2cpp.Template(source=UNPARSABLE).render() == UNPARSABLE_RESULT
    assert jinja2cpp.Template(source="{{ x }}", autoescape=True).render(x="<") == "&lt;"
    with pytest.raises(jinja2cpp.TemplateSyntaxError):
        jinja2cpp.Template(source=UNPARSABLE, fallback="never")


def test_sandbox_override_set_after_load_is_refused_by_fallback_templates():
    for make in (lambda: jinja2cpp.sandbox.SandboxedEnvironment(), lambda: jinja2cpp.sandbox.SandboxedEnvironment(extensions=[Ext])):
        env = make()
        template = env.from_string("{{ x.shown }}{% print 0 %}")
        assert template.render(x=Secret()) == "10"
        env.call_unop = lambda context, operator, arg: arg
        with pytest.raises(jinja2cpp.UnsupportedError):
            template.render(x=Secret())
        with pytest.raises(jinja2cpp.UnsupportedError):
            list(template.generate(x=Secret()))
