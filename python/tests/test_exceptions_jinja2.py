"""jinja2cpp's exceptions are also Jinja2's when Jinja2 is importable, and stand alone without it."""

import subprocess
import sys
import textwrap

import jinja2
import jinja2cpp
import pytest

CLASSES = ["TemplateError", "TemplateNotFound", "TemplatesNotFound", "TemplateSyntaxError", "TemplateAssertionError", "TemplateRuntimeError", "UndefinedError", "SecurityError", "FilterArgumentError"]


@pytest.mark.parametrize("name", CLASSES)
def test_class_is_a_subclass_of_jinja2s(name):
    assert issubclass(getattr(jinja2cpp, name), getattr(jinja2.exceptions, name))


def test_jinja2_handlers_catch_ours():
    env = jinja2cpp.Environment(loader=jinja2cpp.DictLoader({}), undefined=jinja2cpp.StrictUndefined)
    with pytest.raises(jinja2.TemplateNotFound):
        env.get_template("nope")
    with pytest.raises(jinja2.TemplatesNotFound):
        env.select_template(["a", "b"])
    with pytest.raises(jinja2.TemplateSyntaxError) as info:
        env.from_string("{% if %}")
    assert str(info.value).startswith("Expected an expression") or "line 1" in str(info.value)
    with pytest.raises(jinja2.UndefinedError):
        env.from_string("{{ x }}").render()
    with pytest.raises(jinja2.TemplateAssertionError):
        env.from_string("{{ 1|nofilter }}")
    with pytest.raises(jinja2.exceptions.TemplateError):
        env.from_string("{{ 1 // 0 }}").render()
    with pytest.raises(LookupError):
        env.get_template("nope")


def test_ours_still_behave():
    exc = jinja2cpp.TemplateNotFound("n")
    assert exc.name == "n" and exc.templates == ["n"] and str(exc) == "n"
    syntax = jinja2cpp.TemplateSyntaxError("bad", 3, "name", "file")
    assert (syntax.lineno, syntax.name, syntax.filename) == (3, "name", "file") and "line 3" in str(syntax)
    many = jinja2cpp.TemplatesNotFound(["a", "b"])
    assert many.templates == ["a", "b"] and "none of the templates given were found: a, b" in str(many)
    import pickle

    assert pickle.loads(pickle.dumps(syntax)).lineno == 3


def test_unsupported_error():
    assert issubclass(jinja2cpp.UnsupportedError, jinja2cpp.TemplateError)
    assert issubclass(jinja2cpp.UnsupportedError, NotImplementedError)
    assert issubclass(jinja2cpp.UnsupportedError, jinja2.TemplateError)
    assert not issubclass(jinja2cpp.UnsupportedError, jinja2cpp.TemplateRuntimeError)
    with pytest.raises(NotImplementedError):
        raise jinja2cpp.UnsupportedError("not in the subset")


def test_without_jinja2():
    script = textwrap.dedent(
        """
        import sys

        class Block:
            def find_spec(self, name, path=None, target=None):
                if name == "jinja2" or name.startswith("jinja2."):
                    raise ImportError("jinja2 is blocked")

        sys.meta_path.insert(0, Block())
        import jinja2cpp

        try:
            import jinja2
        except ImportError:
            pass
        else:
            raise SystemExit("jinja2 imported")
        for name in %r:
            cls = getattr(jinja2cpp, name)
            assert not any(base.__module__.startswith("jinja2.") for base in cls.__mro__), name
        assert issubclass(jinja2cpp.TemplateNotFound, LookupError) and issubclass(jinja2cpp.TemplateNotFound, OSError)
        env = jinja2cpp.Environment(loader=jinja2cpp.DictLoader({"a": "{{ 1 + 1 }}"}))
        assert env.get_template("a").render() == "2"
        try:
            env.get_template("missing")
        except jinja2cpp.TemplateNotFound as error:
            assert error.name == "missing"
        else:
            raise SystemExit("no error")
        assert issubclass(jinja2cpp.UnsupportedError, NotImplementedError)
        print("ok")
        """
        % CLASSES
    )
    result = subprocess.run([sys.executable, "-c", script], capture_output=True, text=True, check=False, env={"PYTHONPATH": ":".join(p for p in sys.path if p), "PATH": ""})
    assert result.returncode == 0 and result.stdout.strip() == "ok", result.stderr
