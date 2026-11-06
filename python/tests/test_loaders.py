"""DictLoader and FileSystemLoader (through ConfinedFileSystem), and a custom get_source loader."""

import os

import jinja2
import jinja2cpp
import pytest

TEMPLATES = {
    "base.html": "<title>{% block title %}base{% endblock %}</title>{% block body %}{% endblock %}",
    "page.html": "{% extends 'base.html' %}{% block title %}{{ super() }}-page{% endblock %}{% block body %}{% include 'part.html' %}{% endblock %}",
    "part.html": "[{{ who|default('nobody') }}]",
    "macros.html": "{% macro hi(n) %}hi {{ n }}{% endmacro %}",
    "use.html": "{% from 'macros.html' import hi %}{% import 'macros.html' as m %}{{ hi(1) }} {{ m.hi(2) }}",
}


def both(loader_of, name, **context):
    out = []
    for module in (jinja2cpp, jinja2):
        env = module.Environment(loader=loader_of(module))
        out.append(env.get_template(name).render(**context))
    return out


@pytest.mark.parametrize("name", ["page.html", "use.html", "part.html"])
def test_dict_loader(name):
    got, expected = both(lambda m: m.DictLoader(TEMPLATES), name, who="me")
    assert got == expected


def test_dict_loader_reloads_a_changed_source():
    mapping = {"a": "one"}
    env = jinja2cpp.Environment(loader=jinja2cpp.DictLoader(mapping))
    assert env.get_template("a").render() == "one"
    mapping["a"] = "two"
    assert env.get_template("a").render() == "two"
    tpl = env.get_template("a")
    assert tpl.name == "a" and tpl.filename is None and tpl.environment is env


@pytest.fixture
def tree(tmp_path):
    for name, source in TEMPLATES.items():
        (tmp_path / "t" / name).parent.mkdir(parents=True, exist_ok=True)
        (tmp_path / "t" / name).write_text(source, encoding="utf-8")
    (tmp_path / "t" / "sub").mkdir()
    (tmp_path / "t" / "sub" / "x.txt").write_text("sub {{ 1 }}", encoding="utf-8")
    (tmp_path / "outside.txt").write_text("SECRET", encoding="utf-8")
    (tmp_path / "t2").mkdir()
    (tmp_path / "t2" / "only2.txt").write_text("second", encoding="utf-8")
    return tmp_path


@pytest.mark.parametrize("name", ["page.html", "use.html", "sub/x.txt", "only2.txt"])
def test_file_system_loader(tree, name):
    got, expected = both(lambda m: m.FileSystemLoader([tree / "t", str(tree / "t2")]), name)
    assert got == expected


def test_file_system_loader_filename_and_not_found(tree):
    env = jinja2cpp.Environment(loader=jinja2cpp.FileSystemLoader(tree / "t"))
    tpl = env.get_template("sub/x.txt")
    assert tpl.filename == os.path.join(str(tree / "t"), "sub/x.txt") or os.path.normpath(tpl.filename) == str(tree / "t" / "sub" / "x.txt")
    for name in ("nope.txt", "../outside.txt", "sub/../../outside.txt"):
        with pytest.raises(jinja2cpp.TemplateNotFound):
            env.get_template(name)


def test_file_system_loader_does_not_follow_a_link_out(tree):
    link = tree / "t" / "link.txt"
    try:
        link.symlink_to(tree / "outside.txt")
    except OSError:
        pytest.skip("no symbolic links here")
    env = jinja2cpp.Environment(loader=jinja2cpp.FileSystemLoader(tree / "t"))
    with pytest.raises(jinja2cpp.TemplateNotFound):
        env.get_template("link.txt").render()


def test_file_system_loader_encoding(tree):
    (tree / "t" / "latin.txt").write_bytes("caf\xe9 {{ 1 }}".encode("latin-1"))
    env = jinja2cpp.Environment(loader=jinja2cpp.FileSystemLoader(tree / "t", encoding="latin-1"))
    assert env.get_template("latin.txt").render() == "caf\xe9 1"


def test_custom_and_jinja2_loaders(tree):
    class Upper(jinja2cpp.BaseLoader):
        def get_source(self, environment, template):
            if template != "x":
                raise jinja2cpp.TemplateNotFound(template)
            return "X{{ 1 }}", None, lambda: True

    assert jinja2cpp.Environment(loader=Upper()).get_template("x").render() == "X1"
    with pytest.raises(jinja2cpp.TemplateNotFound):
        jinja2cpp.Environment(loader=Upper()).get_template("y")
    # Jinja2's own loader classes work as ours
    env = jinja2cpp.Environment(loader=jinja2.DictLoader({"a": "{% include 'b' %}", "b": "B"}))
    assert env.get_template("a").render() == "B"
    env = jinja2cpp.Environment(loader=jinja2.FileSystemLoader(str(tree / "t")))
    assert env.get_template("part.html").render() == "[nobody]"
    env = jinja2cpp.Environment(loader=jinja2.FunctionLoader(lambda name: "fn " + name))
    assert env.get_template("q").render() == "fn q"


def test_list_templates(tree):
    assert jinja2cpp.DictLoader(TEMPLATES).list_templates() == jinja2.DictLoader(TEMPLATES).list_templates()
    assert jinja2cpp.FileSystemLoader(tree / "t").list_templates() == jinja2.FileSystemLoader(tree / "t").list_templates()


def test_syntax_error_source_never_comes_from_outside_the_search_path(tmp_path):
    # dir1/x.html links out of the root (the confined loader rejects it); dir2/x.html is the template
    # that loads, and its source is the diagnostic's, not the file outside
    outside = tmp_path / "outside.html"
    outside.write_text("OUTSIDE {% if %}")
    dir1 = tmp_path / "dir1"
    dir2 = tmp_path / "dir2"
    dir1.mkdir()
    dir2.mkdir()
    try:
        (dir1 / "x.html").symlink_to(outside)
    except OSError:
        pytest.skip("no symlinks")
    (dir2 / "x.html").write_text("INSIDE {% if %}")
    env = jinja2cpp.Environment(loader=jinja2cpp.FileSystemLoader([str(dir1), str(dir2)]))
    with pytest.raises(jinja2cpp.TemplateSyntaxError) as info:
        env.get_template("x.html")
    assert info.value.source in (None, "INSIDE {% if %}")
    assert "OUTSIDE" not in str(info.value.source)
    assert info.value.filename in (None, os.path.join(str(dir2), "x.html"))


def test_uptodate_callback_may_load_another_template():
    # The freshness check ran under the engine's cache lock, so a nested load deadlocked
    import subprocess
    import sys
    import textwrap

    code = textwrap.dedent(
        """
        import jinja2cpp

        class Loader(jinja2cpp.BaseLoader):
            calls = 0

            def get_source(self, environment, template):
                def uptodate():
                    if template == "a":
                        # A template not cached yet, on each check
                        Loader.calls += 1
                        environment.get_template("b%d" % Loader.calls)
                    return True
                return template.upper(), None, uptodate

        env = jinja2cpp.Environment(loader=Loader(), auto_reload=True)
        env.get_template("a")
        print(env.get_template("a").render())
        """
    )
    done = subprocess.run([sys.executable, "-c", code], capture_output=True, text=True, timeout=60, env=dict(os.environ))
    assert done.returncode == 0, done.stderr
    assert done.stdout.strip() == "A"
