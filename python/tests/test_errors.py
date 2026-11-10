"""Exceptions: Jinja2's hierarchy and attributes, and a callable's own exception raised as itself."""

import os

import jinja2
import jinja2cpp
import pytest


def raised(module, source, loader=None, **context):
    env = module.Environment(loader=loader(module) if loader else None)
    with pytest.raises(Exception) as info:
        env.get_template(source).render(**context) if loader else env.from_string(source).render(**context)
    return info.value


def test_hierarchy_matches_jinja2():
    for name in ("TemplateNotFound", "TemplatesNotFound", "TemplateSyntaxError", "TemplateAssertionError", "TemplateRuntimeError", "UndefinedError", "SecurityError", "FilterArgumentError"):
        ours, theirs = getattr(jinja2cpp, name), getattr(jinja2.exceptions, name)
        # Without Jinja2's own classes (they are bases too), the order is Jinja2's
        assert [c.__name__ for c in ours.__mro__ if not c.__module__.startswith("jinja2.")] == [c.__name__ for c in theirs.__mro__], name
        assert issubclass(ours, theirs)


@pytest.mark.parametrize("source", ["{% if %}", "{{ 1 +* 2 }}", "a\nb\n{% for x in %}", "{% endif %}", "{{ x "])
def test_syntax_error_attributes(source):
    ours, theirs = raised(jinja2cpp, source), raised(jinja2, source)
    assert type(ours).__name__ == type(theirs).__name__ == "TemplateSyntaxError"
    assert isinstance(ours, jinja2cpp.TemplateError)
    assert ours.lineno == theirs.lineno
    assert ours.name is None and ours.filename is None
    assert ours.source == source


def test_syntax_error_from_a_loader_names_the_template():
    templates = {"bad.html": "ok\n{% for %}"}
    ours = raised(jinja2cpp, "bad.html", lambda m: m.DictLoader(templates))
    theirs = raised(jinja2, "bad.html", lambda m: m.DictLoader(templates))
    assert (ours.lineno, ours.name, ours.filename) == (theirs.lineno, theirs.name, theirs.filename)
    assert 'line 2' in str(ours)


def test_unknown_filter_and_test_are_assertion_errors():
    for source in ("{{ 1|nosuch }}", "{{ 1 is nosuch }}"):
        ours, theirs = raised(jinja2cpp, source), raised(jinja2, source)
        assert type(ours).__name__ == type(theirs).__name__ == "TemplateAssertionError"
        assert str(ours).splitlines()[0] == str(theirs).splitlines()[0]


def test_undefined_error_message():
    for source, context in (("{{ x.y }}", {}), ("{{ d.k.z }}", {"d": {}}), ("{{ x + 1 }}", {})):
        ours, theirs = raised(jinja2cpp, source, **context), raised(jinja2, source, **context)
        assert type(ours) is jinja2cpp.UndefinedError and type(theirs) is jinja2.UndefinedError
        assert str(ours) == str(theirs)


def test_template_not_found():
    for module in (jinja2cpp, jinja2):
        env = module.Environment(loader=module.DictLoader({"a": "{% include 'missing' %}"}))
        with pytest.raises(module.TemplateNotFound) as info:
            env.get_template("nope")
        assert info.value.name == "nope" and str(info.value) == "nope"
        with pytest.raises(module.TemplateNotFound):
            env.get_template("a").render()
    with pytest.raises(TypeError):
        jinja2cpp.Environment().get_template("x")


def test_callable_exception_is_raised_as_itself():
    class Custom(Exception):
        pass

    error = Custom("boom")

    def fail(*args):
        raise error

    for where in ("{{ f() }}", "{{ 1|f }}", "{{ 1 is f }}", "{% for x in [1] %}{{ o.attr }}{% endfor %}"):
        env = jinja2cpp.Environment()
        env.globals["f"] = fail
        env.filters["f"] = fail
        env.tests["f"] = fail

        class Obj:
            @property
            def attr(self):
                raise error

        with pytest.raises(Custom) as info:
            env.from_string(where).render(o=Obj())
        assert info.value is error
        assert info.value.__traceback__ is not None
    # The traceback refers to this frame, which refers to the error: a cycle the interpreter
    # would collect only after nanobind's leak check at exit
    error.__traceback__ = None


def test_exceptions_of_python_operations():
    class Bad:
        def __str__(self):
            raise ValueError("no str")

    with pytest.raises(ValueError, match="no str"):
        jinja2cpp.Template("{{ b }}").render(b=Bad())

    def gen():
        yield 1
        raise KeyError("in generator")

    with pytest.raises(KeyError):
        jinja2cpp.Template("{% for x in g %}{{ x }}{% endfor %}").render(g=gen())


def test_template_error_from_python_keeps_its_message():
    def raise_exception(message):
        raise jinja2cpp.TemplateError(message)

    with pytest.raises(jinja2cpp.TemplateError, match="custom message"):
        jinja2cpp.Template("{{ raise_exception('custom message') }}").render(raise_exception=raise_exception)


def test_runtime_errors():
    # Jinja2 raises Python's own ZeroDivisionError and TypeError here; jinja2cpp raises
    # TemplateRuntimeError with the same message (docs/python.md, Differences)
    for source in ("{{ 1 / 0 }}", "{{ 'a' + 1 }}", "{{ 1 < 'a' }}"):
        ours, theirs = raised(jinja2cpp, source), raised(jinja2, source)
        assert isinstance(ours, jinja2cpp.TemplateRuntimeError), source
        assert str(ours) == str(theirs), source


def test_include_of_several_missing_templates_is_templates_not_found():
    for module in (jinja2cpp, jinja2):
        env = module.Environment(loader=module.DictLoader({"header": "H"}))
        with pytest.raises(module.TemplatesNotFound) as info:
            env.from_string('{% include ["missing", "missing2"] %}').render()
        assert info.value.templates == ["missing", "missing2"]
        assert info.value.name == "missing2"
        assert str(info.value) == "none of the templates given were found: missing, missing2"


def test_template_not_found_names_the_search_path(tmp_path):
    for module in (jinja2cpp, jinja2):
        loader = module.FileSystemLoader(str(tmp_path))
        env = module.Environment(loader=loader)
        with pytest.raises(module.TemplateNotFound) as info:
            env.get_template("missing")
        assert str(info.value) == f"'missing' not found in search path: {str(tmp_path)!r}"
        loader.searchpath.append("other")
        with pytest.raises(module.TemplateNotFound) as info:
            env.from_string("{% include 'missing' %}").render()
        assert str(info.value) == f"'missing' not found in search paths: {str(tmp_path)!r}, 'other'"


def test_syntax_error_of_an_included_template_has_its_source():
    source = "a\n{% for x in y %}\n{% endif %}"
    for module in (jinja2cpp, jinja2):
        env = module.Environment(loader=module.ChoiceLoader([module.DictLoader({"inc": "{% include 'bad' %}"}), module.FunctionLoader(lambda name: source if name == "bad" else None)]))
        with pytest.raises(module.TemplateSyntaxError) as info:
            env.get_template("inc").render()
        assert (info.value.name, info.value.lineno, info.value.source) == ("bad", 3, source)


def test_tracebacks_show_the_templates_line(tmp_path):
    # As Jinja2's rewritten tracebacks: the template's file and line between the caller and the callable
    import traceback

    (tmp_path / "broken.html").write_text("Before\n{{ fail() }}\nAfter\n", encoding="utf-8")
    (tmp_path / "syntax.html").write_text("a\n{% for x in y %}\n{% endif %}\n", encoding="utf-8")
    env = jinja2cpp.Environment(loader=jinja2cpp.FileSystemLoader(str(tmp_path)))
    with pytest.raises(ZeroDivisionError) as info:
        env.get_template("broken.html").render(fail=lambda: 1 / 0)
    frames = traceback.extract_tb(info.tb)
    assert [(os.path.basename(f.filename), f.lineno, f.line) for f in frames[-2:-1]] == [("broken.html", 2, "{{ fail() }}")]
    assert frames[-1].name == "<lambda>"
    with pytest.raises(jinja2cpp.TemplateSyntaxError) as info:
        env.get_template("syntax.html")
    last = traceback.extract_tb(info.tb)[-1]
    assert (os.path.basename(last.filename), last.lineno, last.name, last.line) == ("syntax.html", 3, "template", "{% endif %}")
