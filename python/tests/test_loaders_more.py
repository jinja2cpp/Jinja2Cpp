"""ChoiceLoader, PrefixLoader, FunctionLoader, PackageLoader, ModuleLoader and BaseLoader.load."""

import sys
import zipfile

import jinja2
import jinja2cpp
import pytest

FILES = {"index.html": "index {{ x|default('-') }}", "inc.html": "[inc]", "page.html": "{% extends 'index.html' %}", "deep/leaf.txt": "leaf"}


def render_pair(make_loader, name, **context):
    out = []
    for module in (jinja2cpp, jinja2):
        env = module.Environment(loader=make_loader(module))
        try:
            out.append(env.get_template(name).render(**context))
        except Exception as error:  # noqa: BLE001 - compared by class name
            out.append(type(error).__name__)
    return out


@pytest.mark.parametrize("name", ["a.html", "b.html", "both.html", "missing.html", "inc.html"])
def test_choice_loader(name):
    def make(module):
        return module.ChoiceLoader(
            [
                module.DictLoader({"a.html": "A1", "both.html": "first"}),
                module.FunctionLoader(lambda n: {"b.html": "B2", "both.html": "second"}.get(n)),
                module.DictLoader({"inc.html": "{% include 'a.html' %}"}),
            ]
        )

    ours, theirs = render_pair(make, name)
    assert ours == theirs


@pytest.mark.parametrize("name", ["app1/x.html", "app2/x.html", "app1/inc", "app3/x.html", "x.html", "app1/"])
def test_prefix_loader(name):
    def make(module):
        return module.PrefixLoader({"app1": module.DictLoader({"x.html": "one", "inc": "{% include 'app2/x.html' %}"}), "app2": module.DictLoader({"x.html": "two"})})

    ours, theirs = render_pair(make, name)
    assert ours == theirs


def test_prefix_loader_delimiter_and_not_found_name():
    loader = jinja2cpp.PrefixLoader({"p": jinja2cpp.DictLoader({"t": "T"})}, delimiter=":")
    env = jinja2cpp.Environment(loader=loader)
    assert env.get_template("p:t").render() == "T"
    with pytest.raises(jinja2.TemplateNotFound) as info:
        env.get_template("p:zz")
    assert info.value.name == "p:zz"
    assert loader.list_templates() == ["p:t"]


def test_nested_choice_and_prefix_with_file_system(tmp_path):
    (tmp_path / "fs.html").write_text("from disk {{ 1 + 1 }}", encoding="utf-8")
    for module in (jinja2cpp, jinja2):
        loader = module.ChoiceLoader([module.PrefixLoader({"d": module.ChoiceLoader([module.DictLoader({"x": "dx"}), module.FileSystemLoader(str(tmp_path))])}), module.DictLoader({"y": "{% include 'd/fs.html' %}"})])
        env = module.Environment(loader=loader)
        assert env.get_template("d/x").render() == "dx"
        assert env.get_template("y").render() == "from disk 2"
        assert env.list_templates() == sorted(["d/x", "d/fs.html", "y"])


def test_jinja2_loaders_work_in_our_environment(tmp_path):
    (tmp_path / "t.html").write_text("t{{ v }}", encoding="utf-8")
    loader = jinja2.ChoiceLoader([jinja2.PrefixLoader({"p": jinja2.FunctionLoader(lambda n: "fn:" + n)}), jinja2.FileSystemLoader(str(tmp_path))])
    env = jinja2cpp.Environment(loader=loader)
    assert env.get_template("p/q").render() == "fn:q"
    assert env.get_template("t.html").render(v=1) == "t1"


def test_function_loader_forms():
    def load(name):
        if name == "str":
            return "just {{ 1 }}"
        if name == "tuple":
            return "tuple", "/virtual/tuple.txt", None
        return None

    for module in (jinja2cpp, jinja2):
        env = module.Environment(loader=module.FunctionLoader(load))
        assert env.get_template("str").render() == "just 1"
        assert env.get_template("tuple").filename == "/virtual/tuple.txt"
        with pytest.raises(jinja2.TemplateNotFound):
            env.get_template("none")


@pytest.mark.parametrize("module", [jinja2cpp, jinja2], ids=["jinja2cpp", "jinja2"])
def test_uptodate_is_honoured_with_auto_reload(module):
    state = {"source": "one", "fresh": True, "asked": 0}

    def load(name):
        source = state["source"]

        def uptodate():
            state["asked"] += 1
            return state["fresh"]

        return source, None, uptodate

    for loader_of in (lambda: module.FunctionLoader(load), lambda: module.ChoiceLoader([module.FunctionLoader(load)]), lambda: module.PrefixLoader({"p": module.FunctionLoader(load)})):
        state.update(source="one", fresh=True, asked=0)
        name = "p/t" if "Prefix" in type(loader_of()).__name__ else "t"
        env = module.Environment(loader=loader_of())
        assert env.get_template(name).render() == "one"
        state["source"], state["fresh"] = "two", False
        assert env.get_template(name).render() == "two"
        assert state["asked"] >= 1
        # up to date: the cached template stays
        state.update(source="three", fresh=True)
        assert env.get_template(name).render() == "two"


@pytest.mark.parametrize("module", [jinja2cpp, jinja2], ids=["jinja2cpp", "jinja2"])
def test_auto_reload_off_never_asks(module):
    state = {"asked": 0, "source": "one"}

    def load(name):
        def uptodate():
            state["asked"] += 1
            return False

        return state["source"], None, uptodate

    env = module.Environment(loader=module.FunctionLoader(load), auto_reload=False)
    assert env.get_template("t").render() == "one"
    state["source"] = "two"
    assert env.get_template("t").render() == "one"
    assert state["asked"] == 0


def test_base_loader_load():
    class Mine(jinja2cpp.BaseLoader):
        def get_source(self, environment, template):
            if template == "x":
                return "x says {{ v }}", "/mine/x", None
            raise jinja2.TemplateNotFound(template)

    env = jinja2cpp.Environment(loader=Mine(), autoescape=lambda name: bool(name and name.endswith(".html")))
    loader = env.loader
    tpl = loader.load(env, "x", {"v": "g<"})
    assert tpl.render() == "x says g<" and tpl.name == "x" and tpl.filename == "/mine/x"
    assert env.get_template("x").render(v=1) == "x says 1"
    with pytest.raises(jinja2.TemplateNotFound):
        loader.load(env, "y")
    with pytest.raises(jinja2.TemplateNotFound):
        jinja2cpp.BaseLoader().get_source(env, "z")
    # the autoescape selector sees the name loader.load gives
    class Html(jinja2cpp.BaseLoader):
        def get_source(self, environment, template):
            return "{{ v }}", None, None

    html_env = jinja2cpp.Environment(loader=Html(), autoescape=lambda name: bool(name and name.endswith(".html")))
    assert html_env.loader.load(html_env, "a.html").render(v="<") == "&lt;"
    assert html_env.loader.load(html_env, "a.txt").render(v="<") == "<"


def test_module_loader_is_unsupported():
    with pytest.raises(jinja2cpp.UnsupportedError) as info:
        jinja2cpp.ModuleLoader("/tmp")
    assert isinstance(info.value, NotImplementedError) and isinstance(info.value, jinja2cpp.TemplateError)
    assert jinja2cpp.ModuleLoader.has_source_access is False


_counter = [0]


def unique(prefix):
    # A package is imported once per process: each test gets a name of its own
    _counter[0] += 1
    return f"{prefix}{_counter[0]}"


@pytest.fixture
def package_dir(tmp_path, monkeypatch):
    root = tmp_path / "site"
    pkg = root / unique("ldr_dirpkg")
    (pkg / "templates" / "deep").mkdir(parents=True)
    (pkg / "__init__.py").write_text("", encoding="utf-8")
    for name, source in FILES.items():
        (pkg / "templates" / name).write_text(source, encoding="utf-8")
    (pkg / "other").mkdir()
    (pkg / "other" / "o.html").write_text("other {{ 1 }}", encoding="utf-8")
    monkeypatch.syspath_prepend(str(root))
    return pkg


@pytest.fixture
def package_zip(tmp_path, monkeypatch):
    archive = tmp_path / "ldr_zip.zip"
    package = unique("ldr_zippkg")
    with zipfile.ZipFile(archive, "w") as z:
        z.writestr(f"{package}/__init__.py", "")
        for name, source in FILES.items():
            z.writestr(f"{package}/templates/{name}", source)
    monkeypatch.syspath_prepend(str(archive))
    return package


@pytest.mark.parametrize("name", ["index.html", "page.html", "deep/leaf.txt", "missing", "../__init__.py", "inc.html"])
def test_package_loader_directory(package_dir, name):
    ours, theirs = render_pair(lambda m: m.PackageLoader(package_dir.name), name, x=3)
    assert ours == theirs


@pytest.mark.parametrize("name", ["index.html", "page.html", "deep/leaf.txt", "missing"])
def test_package_loader_zip(package_zip, name):
    ours, theirs = render_pair(lambda m: m.PackageLoader(package_zip), name, x=3)
    assert ours == theirs


def test_package_loader_details(package_dir):
    ours, theirs = jinja2cpp.PackageLoader(package_dir.name), jinja2.PackageLoader(package_dir.name)
    assert ours.list_templates() == theirs.list_templates() == sorted(FILES)
    assert jinja2cpp.PackageLoader(package_dir.name, "other").list_templates() == ["o.html"]
    env = jinja2cpp.Environment(loader=ours)
    assert env.get_template("index.html").filename == jinja2.Environment(loader=theirs).get_template("index.html").filename
    assert env.list_templates(extensions=["txt"]) == ["deep/leaf.txt"]
    with pytest.raises(ValueError):
        jinja2cpp.PackageLoader(package_dir.name, "nonexistent")
    with pytest.raises(ImportError):
        jinja2cpp.PackageLoader("no_such_package_ldr")


def test_package_loader_zip_listing_and_reload(package_zip):
    assert jinja2cpp.PackageLoader(package_zip).list_templates() == jinja2.PackageLoader(package_zip).list_templates() == sorted(FILES)
    loader = jinja2cpp.PackageLoader(package_zip)
    assert loader.get_source(None, "inc.html")[2] is None


def test_package_loader_reloads_a_changed_file(package_dir):
    env = jinja2cpp.Environment(loader=jinja2cpp.PackageLoader(package_dir.name))
    assert env.get_template("inc.html").render() == "[inc]"
    path = package_dir / "templates" / "inc.html"
    path.write_text("[changed]", encoding="utf-8")
    import os

    os.utime(path, (os.path.getmtime(path) + 5, os.path.getmtime(path) + 5))
    assert env.get_template("inc.html").render() == "[changed]"
