"""Subclassing, template_class, select_template, list_templates, join_path, generate and stream."""

import copy
import gc
import io
import weakref

import jinja2
import jinja2cpp
import pytest

TEMPLATES = {"a.html": "A{{ x }}", "b.html": "B{{ x }}", "sub/c.html": "C", "d.txt": "D"}


def modules():
    return (jinja2cpp, jinja2)


def test_python_subclass_of_environment():
    class MyEnv(jinja2cpp.Environment):
        def __init__(self, **kwargs):
            kwargs.setdefault("trim_blocks", True)
            super().__init__(**kwargs)
            self.extra = "kept"

    env = MyEnv(loader=jinja2cpp.DictLoader(TEMPLATES))
    assert env.trim_blocks is True and env.extra == "kept"
    env.extra = "changed"
    env.undefined = jinja2cpp.StrictUndefined
    assert env.get_template("a.html").render(x=1) == "A1"
    assert env.from_string("{% if 1 %}\nx{% endif %}").render() == "x"
    assert isinstance(env, jinja2cpp.Environment) and type(env.get_template("a.html").environment) is MyEnv
    env.filters["twice"] = lambda s: s * 2
    assert env.from_string("{{ 'ab'|twice }}").render() == "abab"


def test_environment_arbitrary_attributes_as_jinja2():
    for module in modules():
        env = module.Environment()
        env.my_flag = 5
        assert env.my_flag == 5


def test_subclass_cycle_is_collected():
    class MyEnv(jinja2cpp.Environment):
        pass

    env = MyEnv()
    env.self_ref = env
    env.filters["f"] = lambda s, env=env: s
    tpl = env.from_string("{{ 1|f }}")
    assert tpl.render() == "1"
    refs = (weakref.ref(env), weakref.ref(tpl))
    del env, tpl
    gc.collect()
    assert [ref() for ref in refs] == [None, None]


def test_template_class():
    class MyTemplate(jinja2cpp.Template):
        def render(self, *args, **kwargs):
            return "<" + super().render(*args, **kwargs) + ">"

    for module in modules():
        template_class = MyTemplate if module is jinja2cpp else type("T", (jinja2.Template,), {"render": lambda s, *a, **k: "<" + jinja2.Template.render(s, *a, **k) + ">"})
        env = module.Environment(loader=module.DictLoader(TEMPLATES))
        assert env.template_class is module.Template
        env.template_class = template_class
        assert type(env.from_string("{{ 1 }}")) is template_class
        assert type(env.get_template("a.html")) is template_class
        assert env.get_template("a.html").render(x=2) == "<A2>"
        assert type(env.select_template(["nope", "b.html"])) is template_class
        assert env.from_string("1", template_class=module.Template).render() == "1"
    class SubEnv(jinja2cpp.Environment):
        template_class = MyTemplate

    assert SubEnv().from_string("{{ 3 }}").render() == "<3>"
    with pytest.raises(TypeError):
        jinja2cpp.Environment().from_string("x", template_class=int)


def test_template_constructor_uses_environment_class():
    class MyEnv(jinja2cpp.Environment):
        pass

    class MyTemplate(jinja2cpp.Template):
        environment_class = MyEnv

    tpl = MyTemplate("{{ a }}", trim_blocks=True)
    assert type(tpl.environment) is MyEnv and tpl.environment.trim_blocks
    assert tpl.render(a=1) == "1"
    assert jinja2cpp.Template("{{ 1 }}").render() == "1"
    assert repr(jinja2cpp.Template("x")).startswith("<Template memory:")


def test_get_template_calls_join_path():
    seen = []

    def make(module):
        class Env(module.Environment):
            def join_path(self, template, parent):
                seen.append((template, parent))
                return "sub/" + template if parent else template

        return Env(loader=module.DictLoader({"sub/c.html": "C", "c.html": "top"}))

    for module in modules():
        env = make(module)
        assert env.get_template("c.html", parent="x/y").render() == "C"
        assert env.get_template("c.html").render() == "top"
        assert env.select_template(["nope", "c.html"], parent="p").render() == "C"
        assert env.get_or_select_template("c.html", parent="p").render() == "C"
        assert env.get_or_select_template(["c.html"], parent="p").render() == "C"
        assert module.Environment().join_path("t", "p") == "t"
    assert seen[: len(seen) // 2] == seen[len(seen) // 2 :]


JOIN_TEMPLATES = {
    "a/main": "{% include './x' %}|{% import './mac' as m %}{{ m.f() }}|{% from './mac' import g %}{{ g() }}",
    "a/x": "AX",
    "a/y": "AY",
    "a/mac": "{% macro f() %}{% include './y' %}{% endmacro %}{% macro g() %}G{% endmacro %}",
    "a/child": "{% extends './base' %}{% block c %}{% include './y' %}{% endblock %}",
    "a/base": "<{% block c %}{% endblock %}{% include './x' %}>",
    "a/list": "{% include ['./none', './x'] %}{% include ['./none', './y'] ignore missing %}",
    "a/loop": "{% for i in range(2) %}{% include './x' %}{% endfor %}",
    "a/missing": "{% include './nope' %}",
    "a/missing_list": "{% include ['./nope', './nope2'] %}",
    "a/ignored": "[{% include './nope' ignore missing %}]",
    "a/extends_missing": "{% extends './nope' %}",
    "a/p": "{% include './x' %}",
    "b/p": "{% include './x' %}",
    "b/x": "BX",
    "top": "{% include 'a/p' %}{% include 'b/p' %}",
    "from_top": "{% include 'a/main' %}",
    "caller": "{% import 'a/mac' as m %}{% call m.f() %}{% endcall %}{% include './x' ignore missing %}",
}


def join_env(module, calls, join=None):
    def default_join(self, template, parent):
        calls.append((template, parent))
        if template.startswith("./"):
            return parent.rsplit("/", 1)[0] + "/" + template[2:]
        return template

    class Env(module.Environment):
        join_path = join or default_join

    return Env(loader=module.DictLoader(JOIN_TEMPLATES))


def run_join(module, name):
    calls = []
    env = join_env(module, calls)
    try:
        result = env.get_template(name).render()
    except Exception as error:  # noqa: BLE001 - compared by class and message
        result = (type(error).__name__, str(error))
    return result, calls


@pytest.mark.parametrize("name", sorted(JOIN_TEMPLATES))
def test_tags_go_through_join_path(name):
    assert run_join(jinja2cpp, name) == run_join(jinja2, name)


def test_join_path_joined_name_is_the_cache_key():
    result, calls = run_join(jinja2cpp, "top")
    assert result == "AXBX"
    assert calls == run_join(jinja2, "top")[1]


def test_join_path_not_called_for_from_string():
    for module in modules():
        calls = []
        env = join_env(module, calls)
        assert env.from_string("{% include 'a/x' %}").render() == "AX"
        assert calls == []


def test_join_path_exception_propagates():
    class Boom(Exception):
        pass

    def join(self, template, parent):
        raise Boom(template)

    outcomes = []
    for module in modules():
        env = join_env(module, [], join=join)
        with pytest.raises(Boom) as info:
            env.get_template("a/main").render()
        outcomes.append(str(info.value))
    assert outcomes[0] == outcomes[1]


def test_join_path_template_not_found_raised_by_the_function():
    results = []
    for module in modules():

        def join(self, template, parent, module=module):
            raise module.TemplateNotFound(template)

        env = join_env(module, [], join=join)
        # Not `ignore missing` or a list: there Jinja2 skips the name, and the engine fails with the exception (docs/python.md)
        for source in ("{% include './x' %}",):
            try:
                results.append(env.from_string(source).render())
            except Exception as error:  # noqa: BLE001
                results.append((type(error).__name__, str(error)))
    assert results[0] == results[1]


@pytest.mark.parametrize(
    "source, joined",
    [
        ("{% import './nope' as x %}", None),
        ("{% from './nope' import f %}", None),
        ("{% include './nope' %}", None),
        ("{% extends './nope' %}", None),
        ("{% include 'x' %}", ""),
    ],
)
def test_join_path_names_the_joined_template_when_it_is_missing(source, joined):
    results = []
    for module in modules():

        def join(self, template, parent, joined=joined):
            if joined is not None:
                return joined
            return parent.rsplit("/", 1)[0] + "/" + template[2:] if template.startswith("./") else template

        env = join_env(module, [], join=join)
        env.loader = module.DictLoader({"a/main": source})
        try:
            results.append(env.get_template("a/main").render())
        except module.TemplateNotFound as error:
            results.append((type(error).__name__, str(error), error.name))
    assert results[0] == results[1]


def test_join_path_that_loads_a_template():
    def join(self, template, parent):
        self.get_template("a/x")
        return "a/" + template[2:] if template.startswith("./") else template

    outputs = [join_env(module, [], join=join).get_template("top").render() for module in modules()]
    assert outputs[0] == outputs[1]


def test_join_path_set_on_the_instance():
    for module in modules():
        env = module.Environment(loader=module.DictLoader(JOIN_TEMPLATES))
        env.join_path = lambda template, parent: "b/x" if template == "./x" else template
        assert env.get_template("a/p").render() == "BX"


def collect(module, call):
    env = module.Environment(loader=module.DictLoader(TEMPLATES))
    try:
        result = call(env, module)
    except Exception as error:  # noqa: BLE001 - compared by class and message
        return type(error).__name__, str(error)
    return result


@pytest.mark.parametrize(
    "call",
    [
        lambda env, m: env.select_template(["nope", "b.html", "a.html"]).render(x=1),
        lambda env, m: env.select_template(("a.html",)).name,
        lambda env, m: env.select_template(["nope", "none"]),
        lambda env, m: env.select_template([]),
        lambda env, m: env.select_template(m.Undefined()),
        lambda env, m: env.select_template([m.Undefined(), "a.html"]).render(x=3) if m is jinja2 else env.select_template([m.Undefined(), "a.html"]).render(x=3),
        lambda env, m: env.select_template([env.from_string("tpl")]).render(),
        lambda env, m: env.select_template(["a.html"], globals={"x": 9}).render(),
        lambda env, m: env.get_or_select_template("a.html").render(x=1),
        lambda env, m: env.get_or_select_template(["nope", "a.html"]).render(x=1),
        lambda env, m: env.get_or_select_template(env.from_string("t")).render(),
        lambda env, m: env.get_or_select_template(["nope"]),
        lambda env, m: env.get_template("missing"),
        lambda env, m: env.get_template(env.from_string("same")).render(),
        lambda env, m: env.get_template(m.Undefined()),
        lambda env, m: env.get_template(5),
        lambda env, m: m.Environment().get_template("a"),
        lambda env, m: m.Environment().select_template(["a"]),
    ],
)
def test_select_template_like_jinja2(call):
    ours, theirs = collect(jinja2cpp, call), collect(jinja2, call)
    assert ours == theirs, (ours, theirs)


def test_select_template_not_found_attributes():
    env = jinja2cpp.Environment(loader=jinja2cpp.DictLoader(TEMPLATES))
    with pytest.raises(jinja2.TemplatesNotFound) as info:
        env.select_template(["x", "y"])
    assert info.value.templates == ["x", "y"] and isinstance(info.value, jinja2cpp.TemplatesNotFound)


@pytest.mark.parametrize(
    "kwargs",
    [{}, {"extensions": ["html"]}, {"extensions": ("txt", "html")}, {"extensions": []}, {"filter_func": lambda n: n.startswith("sub")}, {"extensions": ["html"], "filter_func": len}],
)
def test_list_templates(kwargs):
    ours, theirs = collect(jinja2cpp, lambda e, m: e.list_templates(**kwargs)), collect(jinja2, lambda e, m: e.list_templates(**kwargs))
    assert ours == theirs


def test_list_templates_without_loader_or_listing():
    for module in modules():
        with pytest.raises(AssertionError):
            module.Environment().list_templates()
        env = module.Environment(loader=module.FunctionLoader(lambda n: None))
        with pytest.raises(TypeError):
            env.list_templates()


GENERATE_SOURCES = ["Hello {{ name }}!", "", "{% for i in range(3) %}{{ i }},{% endfor %}", "{{ missing.x }}"]


@pytest.mark.parametrize("source", GENERATE_SOURCES)
def test_generate_and_stream(source):
    results = []
    for module in modules():
        tpl = module.Environment().from_string(source)
        try:
            results.append((concat(tpl.generate(name="w")), concat(tpl.stream(name="w")), tpl.render(name="w")))
        except Exception as error:  # noqa: BLE001
            results.append(type(error).__name__)
    assert results[0] == results[1]


def concat(chunks):
    return "".join(chunks)


def test_stream_dump_and_buffering():
    for module in modules():
        stream = module.Environment().from_string("a{{ x }}é").stream(x=1)
        assert stream.buffered is False
        stream.enable_buffering(2)
        assert stream.buffered is True
        with pytest.raises(ValueError):
            stream.enable_buffering(1)
        stream.disable_buffering()
        buffer = io.BytesIO()
        stream.dump(buffer, encoding="utf-8")
        assert buffer.getvalue() == "a1é".encode()
        text = io.StringIO()
        module.Environment().from_string("t{{ 2 }}").stream().dump(text)
        assert text.getvalue() == "t2"


def test_stream_dump_to_file_name(tmp_path):
    jinja2cpp.Template("hi {{ n }}").stream(n="there").dump(str(tmp_path / "out.html"))
    assert (tmp_path / "out.html").read_text(encoding="utf-8") == "hi there"


def test_generate_error_comes_on_first_item():
    env = jinja2cpp.Environment(undefined=jinja2cpp.StrictUndefined)
    generator = env.from_string("{{ nope }}").generate()
    with pytest.raises(jinja2cpp.UndefinedError):
        next(generator)


def test_positional_arguments_as_jinja2():
    for module in modules():
        env = module.Environment("<%", "%>", "<<", ">>")
        assert env.from_string("<% if 1 %><< 2 >><% endif %>").render() == "2"
        with pytest.raises(TypeError):
            module.Environment("<%", block_start_string="x")
        with pytest.raises(TypeError):
            module.Environment(*range(30))


def test_none_and_non_str_arguments_as_jinja2():
    for module in modules():
        env = module.Environment(loader=module.DictLoader(TEMPLATES))
        with pytest.raises(jinja2.TemplateNotFound):
            env.get_template(None)
        assert env.select_template([None, "a.html"]).render(x=1) == "A1"
        for bad in (5, b"x"):
            with pytest.raises(TypeError):
                env.from_string(bad)
        env.loader = None
        env.finalize = None
        env.line_statement_prefix = None
        env.line_comment_prefix = None
        env.bytecode_cache = None
        assert env.loader is None and env.bytecode_cache is None


def test_uninstall_gettext_translations_accepts_none():
    env = jinja2cpp.Environment(extensions=[jinja2cpp.ext.i18n])
    env.install_null_translations()
    env.uninstall_gettext_translations(None)


def test_jinja2cpp_loaders_work_in_a_jinja2_environment():
    env = jinja2.Environment(loader=jinja2cpp.ChoiceLoader([jinja2cpp.DictLoader({"a": "A{{ 1 }}"}), jinja2cpp.PrefixLoader({"p": jinja2cpp.FunctionLoader(lambda n: "fn " + n)})]))
    assert env.get_template("a").render() == "A1"
    assert env.get_template("p/x").render() == "fn x"
    assert env.loader.load(env, "a").render() == "A1"


def test_generate_yields_jinja2s_items():
    # One item for each `{{ }}` and each run of text, as Jinja2 (test_generate.py has more)
    tpl = jinja2cpp.Environment().from_string("{% for i in range(300) %}{{ i }}-{% endfor %}")
    items = list(tpl.generate())
    assert items[:4] == ["0", "-", "1", "-"]
    assert "".join(items) == tpl.render()


def test_generate_error_comes_after_the_chunks_before_it():
    # As Jinja2: the items written before the error, then the error
    big = "x" * 2000
    source = big + "{{ fail() }}"

    def fail():
        raise ZeroDivisionError("boom")

    for module in modules():
        generator = module.Environment().from_string(source).generate(fail=fail)
        got = []
        with pytest.raises(ZeroDivisionError, match="boom"):
            for chunk in generator:
                got.append(chunk)
        assert "".join(got) == big


@pytest.mark.parametrize("source", ["{{ 1 }}|{{ x }}", "{{ 'a' }}{{ 1 + 1 }}"])
def test_folded_output_with_raising_finalize_under_autoescape(source):
    # Jinja2 folds a constant {{ }} as finalize(escape(value)); when that raises it evaluates the
    # value at render time instead, with finalize of the plain value (docs/tasks/0355)
    def finalize(value):
        if isinstance(value, jinja2cpp.Markup):
            raise ValueError("no Markup")
        return value

    results = []
    for module in modules():
        env = module.Environment(autoescape=True, finalize=finalize)
        results.append(env.from_string(source).render(x="<"))
    assert results[0] == results[1]


@pytest.mark.parametrize("error", [KeyboardInterrupt, SystemExit, GeneratorExit])
def test_folded_output_finalize_base_exception_ends_the_render(error):
    # Jinja2's fold catches Exception only: KeyboardInterrupt and the other BaseExceptions go through
    def finalize(value):
        if isinstance(value, jinja2cpp.Markup):
            raise error("stop")
        return value

    for module in modules():
        env = module.Environment(autoescape=True, finalize=finalize)
        with pytest.raises(error, match="stop"):
            env.from_string("{{ 1 }}").render()


def test_base_exception_through_a_lent_macro_reaches_the_python_caller():
    # A KeyboardInterrupt raised inside a macro a Python callable calls goes to that callable, which may catch it
    def boom():
        raise KeyboardInterrupt("stop")

    def swallow(m):
        try:
            return m()
        except KeyboardInterrupt:
            return "swallowed"

    source = "{% macro m() %}{{ boom() }}{% endmacro %}{{ outer(m) }}"
    for module in modules():
        env = module.Environment()
        env.globals.update(boom=boom, outer=lambda m: m())
        with pytest.raises(KeyboardInterrupt, match="stop"):
            env.from_string(source).render()
        env.globals["outer"] = swallow
        assert env.from_string(source).render() == "swallowed"


# --- env.policies (docs/tasks/0435) ---------------------------------------------------------


@pytest.fixture(autouse=True)
def _restore_jinja2_default_policies():
    # Jinja2's Environment copies DEFAULT_POLICIES shallowly: an in-place edit of json.dumps_kwargs would reach every
    # later environment of the process
    saved = copy.deepcopy(jinja2.defaults.DEFAULT_POLICIES)
    yield
    jinja2.defaults.DEFAULT_POLICIES.clear()
    jinja2.defaults.DEFAULT_POLICIES.update(saved)


def _set(key, value):
    def apply(env):
        env.policies[key] = value

    return apply


def _set_kwargs(**kwargs):
    def apply(env):
        env.policies["json.dumps_kwargs"] = kwargs

    return apply


def _dumps_marker(value, **kwargs):
    import json

    return "<%s|%s>" % (json.dumps(value, sort_keys=True), sorted(kwargs))


def _dumps_not_a_str(value, **kwargs):
    return 42


def _dumps_with_kwargs(env):
    env.policies.update({"json.dumps_function": _dumps_marker, "json.dumps_kwargs": {"x": 1}})


def _ignored_keys(env):
    env.policies.update({"compiler.ascii_str": False, "no.such": 1})


URLIZE = "{{ t|urlize }}"
TEXT = {"t": "see example.com and a@b.org"}
TRANS = "{% trans %}  Hello\n   {{ name }}  {% endtrans %}"
TRANS_PLURAL = "{% trans n=n %}\n  one  item\n  {% pluralize %}\n  {{ n }}   items\n{% endtrans %}"
DATA = {"b": 1, "a": [1, 2]}

# (id, mutation of the environment, source, context): each row runs with the policy set before the template loads and
# after it, as Jinja2 does (a filter reads the policies when it runs; {% trans %} when the template compiles)
POLICY_CASES = [
    ("urlize_rel", _set("urlize.rel", "nofollow ugc"), URLIZE, TEXT),
    ("urlize_rel_none", _set("urlize.rel", None), URLIZE, TEXT),
    ("urlize_rel_with_argument", _set("urlize.rel", "noopener"), "{{ t|urlize(rel='x') }}", TEXT),
    ("urlize_target", _set("urlize.target", "_blank"), URLIZE, TEXT),
    ("urlize_target_argument", _set("urlize.target", "_blank"), "{{ t|urlize(target='_top') }}", TEXT),
    ("urlize_extra_schemes", _set("urlize.extra_schemes", ["git+ssh://", "xy:"]), "{{ t|urlize }}", {"t": "git+ssh://h/r and xy:z and see example.com"}),
    ("urlize_extra_schemes_tuple", _set("urlize.extra_schemes", ("tel:",)), "{{ t|urlize }}", {"t": "tel:123 x"}),
    ("urlize_extra_schemes_str", _set("urlize.extra_schemes", "ab"), "{{ t|urlize }}", TEXT),
    ("urlize_extra_schemes_bad", _set("urlize.extra_schemes", ["1x"]), URLIZE, TEXT),
    ("urlize_extra_schemes_argument", _set("urlize.extra_schemes", ["tel:"]), "{{ t|urlize(extra_schemes=['xy:']) }}", {"t": "tel:1 xy:2"}),
    ("truncate_leeway", _set("truncate.leeway", 0), "{{ s|truncate(8) }}", {"s": "abcdefghijkl"}),
    ("truncate_leeway_large", _set("truncate.leeway", 50), "{{ s|truncate(8) }}", {"s": "abcdefghijkl"}),
    ("truncate_leeway_argument", _set("truncate.leeway", 0), "{{ s|truncate(8, leeway=9) }}", {"s": "abcdefghijkl"}),
    ("truncate_leeway_bool", _set("truncate.leeway", True), "{{ s|truncate(8) }}", {"s": "abcdefghij"}),
    ("truncate_leeway_negative", _set("truncate.leeway", -1), "{{ s|truncate(8) }}", {"s": "abcdefghijkl"}),
    ("tojson_sort_keys_off", _set_kwargs(), "{{ d|tojson }}", {"d": DATA}),
    ("tojson_ensure_ascii_off", _set_kwargs(ensure_ascii=False), "{{ d|tojson }}", {"d": {"k": "é<&"}}),
    ("tojson_ensure_ascii_default", _set_kwargs(sort_keys=True), "{{ d|tojson }}", {"d": {"k": "é"}}),
    ("tojson_indent_int", _set_kwargs(sort_keys=True, indent=2), "{{ d|tojson }}", {"d": DATA}),
    ("tojson_indent_str", _set_kwargs(indent="--"), "{{ d|tojson }}", {"d": DATA}),
    ("tojson_indent_zero", _set_kwargs(indent=0), "{{ d|tojson }}", {"d": [1, {"a": 2}]}),
    ("tojson_indent_argument", _set_kwargs(indent=4), "{{ d|tojson(indent=1) }}", {"d": DATA}),
    ("tojson_separators", _set_kwargs(separators=(";", "=")), "{{ d|tojson }}", {"d": DATA}),
    ("tojson_separators_and_indent", _set_kwargs(separators=("; ", "="), indent=1), "{{ d|tojson }}", {"d": DATA}),
    ("tojson_default_kwarg", _set_kwargs(default=str, sort_keys=True), "{{ d|tojson }}", {"d": {"k": {1, 2}}}),
    ("tojson_skipkeys", _set_kwargs(skipkeys=True), "{{ d|tojson }}", {"d": {(1, 2): 1, "a": 2}}),
    ("tojson_allow_nan_off", _set_kwargs(allow_nan=False), "{{ d|tojson }}", {"d": float("nan")}),
    ("tojson_unknown_kwarg", _set_kwargs(no_such=1), "{{ d|tojson }}", {"d": 1}),
    ("tojson_dumps_function", _set("json.dumps_function", _dumps_marker), "{{ d|tojson }}", {"d": {"b": 1, "a": "<"}}),
    ("tojson_dumps_function_indent", _set("json.dumps_function", _dumps_marker), "{{ d|tojson(indent=2) }}", {"d": [1]}),
    ("tojson_dumps_function_kwargs", _dumps_with_kwargs, "{{ d|tojson }}", {"d": 1}),
    ("tojson_dumps_function_not_a_str", _set("json.dumps_function", _dumps_not_a_str), "{{ d|tojson }}", {"d": 1}),
    ("trimmed", _set("ext.i18n.trimmed", True), TRANS, {"name": "N"}),
    ("trimmed_plural", _set("ext.i18n.trimmed", True), TRANS_PLURAL, {"n": 3}),
    ("trimmed_block_override_off", _set("ext.i18n.trimmed", True), "{% trans notrimmed %}  A\n  B {% endtrans %}", {}),
    ("trimmed_block_override_on", _set("ext.i18n.trimmed", False), "{% trans trimmed %}  A\n  B {% endtrans %}", {}),
    ("trimmed_truthy", _set("ext.i18n.trimmed", 1), TRANS, {"name": "N"}),
    ("ascii_str_and_unknown_keys_ignored", _ignored_keys, "{{ 'x'|upper }}", {}),
]


def _render_policy_case(module, mutate, source, context, after_load):
    env = module.Environment(extensions=["jinja2.ext.i18n"])
    env.install_null_translations(newstyle=True)
    if after_load:
        template = env.from_string(source)
        mutate(env)
    else:
        mutate(env)
        template = env.from_string(source)
    try:
        return "ok", template.render(**context)
    except Exception:  # noqa: BLE001
        # that it fails, not with which class: the engine's TemplateRuntimeError for a filter's argument that Jinja2
        # asserts or rejects with FilterArgumentError is the same difference as for the filter's own argument
        return ("error",)


@pytest.mark.parametrize("after_load", [False, True], ids=["set_before_load", "set_after_load"])
@pytest.mark.parametrize("case", POLICY_CASES, ids=[row[0] for row in POLICY_CASES])
def test_policies_like_jinja2(case, after_load):
    _, mutate, source, context = case
    expected = _render_policy_case(jinja2, mutate, source, context, after_load)
    fallbacks = jinja2cpp.stats()["loads_fallback"]
    assert _render_policy_case(jinja2cpp, mutate, source, context, after_load) == expected
    # The engine takes every row itself: none goes to Jinja2
    assert jinja2cpp.stats()["loads_fallback"] == fallbacks


def _renders(edit, source="{{ d|tojson }}|{{ s|truncate(8) }}", **context):
    """What Jinja2 and the module render before and after `edit(env)`, which changes the policies of the environment."""
    results = {}
    for module in (jinja2, jinja2cpp):
        env = module.Environment()
        template = env.from_string(source)
        first = template.render(**context)
        edit(env)
        results[module.__name__] = (first, template.render(**context))
    return results


def test_a_policy_edited_in_place_after_a_render_is_seen_by_the_next_render():
    def edit(env):
        env.policies["json.dumps_kwargs"]["indent"] = 2

    results = _renders(edit, d=DATA, s="abcdefghijkl")
    assert results["jinja2cpp"] == results["jinja2"]
    assert results["jinja2"][0] != results["jinja2"][1]


def test_a_policy_edited_deep_inside_a_value_is_seen_by_the_next_render():
    def edit(env):
        env.policies["json.dumps_kwargs"]["separators"][1] = "="

    for module in (jinja2, jinja2cpp):
        env = module.Environment()
        env.policies["json.dumps_kwargs"] = {"separators": [",", ":"]}
        template = env.from_string("{{ d|tojson }}")
        first = template.render(d=DATA)
        edit(env)
        second = template.render(d=DATA)
        assert first != second
        if module is jinja2:
            expected = (first, second)
    assert (first, second) == expected


def test_a_list_of_extra_schemes_appended_to_after_a_render_is_seen():
    for module in (jinja2, jinja2cpp):
        env = module.Environment()
        env.policies["urlize.extra_schemes"] = ["tel:"]
        template = env.from_string("{{ t|urlize }}")
        first = template.render(t="tel:1 xy:2")
        env.policies["urlize.extra_schemes"].append("xy:")
        second = template.render(t="tel:1 xy:2")
        assert first != second
        if module is jinja2:
            expected = (first, second)
    assert (first, second) == expected


def test_policies_replaced_as_a_whole_are_seen_by_the_next_render():
    def edit(env):
        env.policies = dict(env.policies, **{"truncate.leeway": 0})

    results = _renders(edit, d=DATA, s="abcdefghijkl")
    assert results["jinja2cpp"] == results["jinja2"]
    assert results["jinja2"][0] != results["jinja2"][1]


def test_an_include_sees_the_policies_of_the_render():
    for module in (jinja2, jinja2cpp):
        loader = module.DictLoader({"main": "{% include 'inc' %}", "inc": "{{ s|truncate(8) }}|{{ d|tojson }}"})
        env = module.Environment(loader=loader)
        template = env.get_template("main")
        first = template.render(s="abcdefghijkl", d=DATA)
        env.policies["truncate.leeway"] = 0
        env.policies["json.dumps_kwargs"] = {"indent": 1}
        second = template.render(s="abcdefghijkl", d=DATA)
        assert first != second
        # an include that the render itself loads
        env.cache.clear()
        third = env.get_template("main").render(s="abcdefghijkl", d=DATA)
        if module is jinja2:
            expected = (first, second, third)
    assert (first, second, third) == expected


def test_a_policy_value_the_engine_cannot_take_set_after_a_native_load_raises_at_render():
    env = jinja2cpp.Environment(fallback="never")
    template = env.from_string("{{ 1 }}|{{ s|truncate(8) }}")
    assert template.render(s="x") == "1|x"
    env.policies["urlize.rel"] = 5
    with pytest.raises(jinja2cpp.UnsupportedError, match="urlize.rel"):
        template.render(s="x")
    with pytest.raises(jinja2cpp.UnsupportedError, match="urlize.rel"):
        env.from_string("{{ 1 }}")
    env.policies["urlize.rel"] = "noopener"
    assert template.render(s="x") == "1|x"
    env.policies["truncate.leeway"] = 1.5
    with pytest.raises(jinja2cpp.UnsupportedError, match="truncate.leeway"):
        template.render(s="x")
    del env.policies["truncate.leeway"]
    with pytest.raises(jinja2cpp.UnsupportedError, match="truncate.leeway"):
        template.render(s="x")


@pytest.mark.parametrize("value", [5, ["a"], object()], ids=["int", "list", "object"])
def test_a_policy_value_the_engine_cannot_take_sends_the_loads_to_jinja2(value):
    source = "{{ 'example.com'|urlize }}"
    env = jinja2cpp.Environment()
    env.policies["urlize.target"] = value
    before = jinja2cpp.stats()["loads_fallback"]
    template = env.from_string(source)
    assert jinja2cpp.stats()["loads_fallback"] == before + 1
    oracle = jinja2.Environment()
    oracle.policies["urlize.target"] = value
    outcomes = []
    for tpl in (oracle.from_string(source), template):
        try:
            outcomes.append(("ok", tpl.render()))
        except Exception as error:  # noqa: BLE001
            outcomes.append(("error", type(error).__name__))
    assert outcomes[0] == outcomes[1]


def test_a_one_shot_iterator_of_extra_schemes_goes_to_jinja2():
    env = jinja2cpp.Environment()
    env.policies["urlize.extra_schemes"] = iter(["tel"])
    before = jinja2cpp.stats()["loads_fallback"]
    env.from_string("{{ 'tel:1'|urlize }}")
    assert jinja2cpp.stats()["loads_fallback"] == before + 1


def test_the_macros_of_a_module_keep_the_policies_of_the_load():
    # Jinja2's module macros read the environment's policies at each call; the engine's keep those of the load
    # (docs/python.md, Differences)
    env = jinja2cpp.Environment()
    template = env.from_string("{% macro m(s) %}{{ s|truncate(8) }}{% endmacro %}")
    module = template.module
    assert module.m("abcdefghijkl") == "abcdefghijkl"
    env.policies["truncate.leeway"] = 0
    assert module.m("abcdefghijkl") == "abcdefghijkl"
    assert env.from_string("{{ s|truncate(8) }}").render(s="abcdefghijkl") == "abcde..."


def test_a_dumps_function_is_collected_with_the_environment():
    class Dumper:
        def __call__(self, value, **kwargs):
            return "x"

    env = jinja2cpp.Environment()
    dumper = Dumper()
    dumper.env = env
    env.policies["json.dumps_function"] = dumper
    template = env.from_string("{{ 1|tojson }}")
    assert template.render() == "x"
    refs = (weakref.ref(env), weakref.ref(dumper), weakref.ref(template))
    del env, dumper, template
    gc.collect()
    assert [ref() for ref in refs] == [None, None, None]


def test_replaced_dumps_functions_are_not_kept():
    # Only the current json.dumps_function stays; a template loaded before keeps the one it was loaded with
    class Dumper:
        def __init__(self, tag):
            self.tag = tag

        def __call__(self, value, **kwargs):
            return self.tag

    env = jinja2cpp.Environment()
    refs = []
    old = env.from_string("{{ 1|tojson }}")
    first = Dumper("0")
    refs.append(weakref.ref(first))
    env.policies["json.dumps_function"] = first
    template = env.from_string("{{ 1|tojson }}")
    assert template.render() == "0"
    del first
    for i in range(1, 200):
        dumper = Dumper(str(i))
        refs.append(weakref.ref(dumper))
        env.policies["json.dumps_function"] = dumper
        del dumper
        assert env.from_string("{{ 1|tojson }}").render() == str(i)
        assert old.render() == str(i)
    gc.collect()
    assert sum(ref() is not None for ref in refs) <= 3
    assert template.render() == "199"
    env.policies["json.dumps_function"] = None
    gc.collect()
    assert sum(ref() is not None for ref in refs) <= 2
