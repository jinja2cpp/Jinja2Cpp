"""Subclassing, template_class, select_template, list_templates, join_path, generate and stream."""

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


def test_include_does_not_go_through_join_path():
    # Jinja2 calls environment.get_template(name, parent=template name) for an include; the
    # engine's include reads the loader directly (docs/python.md, known differences)
    calls = []

    class Env(jinja2cpp.Environment):
        def join_path(self, template, parent):
            calls.append((template, parent))
            return template

    env = Env(loader=jinja2cpp.DictLoader({"page": "{% include 'part' %}", "part": "P"}))
    assert env.get_template("page").render() == "P"
    assert calls == []


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
