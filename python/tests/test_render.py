"""Environment, Template, from_string and render, against Python Jinja2."""

import jinja2
import markupsafe
import jinja2cpp
import pytest

from conftest import assert_same, render_both

CASES = [
    ("plain text", {}),
    ("Hello {{ name }}!", {"name": "World"}),
    ("{{ 1 + 2 * 3 }} {{ 7 // 2 }} {{ 7 / 2 }} {{ 2 ** 10 }} {{ 7 % 3 }}", {}),
    ("{% for x in items %}{{ loop.index }}:{{ x }}{% if not loop.last %},{% endif %}{% endfor %}", {"items": ["a", "b", "c"]}),
    ("{% for k, v in d.items() %}{{ k }}={{ v }};{% endfor %}", {"d": {"a": 1, "b": 2}}),
    ("{{ items|join(', ') }} {{ items|length }} {{ items|first }} {{ items|last }}", {"items": [3, 1, 2]}),
    ("{{ items|sort|list }} {{ items|reverse|list }} {{ items|sum }} {{ items|max }}", {"items": [3, 1, 2]}),
    ("{{ name|upper }} {{ name|title }} {{ name|replace('o', '0') }} {{ name|center(11) }}|", {"name": "hello world"}),
    ("{% set x = 3 %}{% if x > 2 %}big{% elif x > 1 %}mid{% else %}small{% endif %}", {}),
    ("{% macro m(a, b=2) %}{{ a }}-{{ b }}{% endmacro %}{{ m(1) }} {{ m(1, b=5) }}", {}),
    ("{{ d }} {{ l }} {{ t }} {{ n }} {{ b }} {{ f }}", {"d": {"a": [1, (2, 3)]}, "l": [1, "x", None], "t": (1,), "n": None, "b": True, "f": 1.5}),
    ("{{ users|map(attribute='name')|join(',') }}", {"users": [{"name": "a"}, {"name": "b"}]}),
    ("{{ users|selectattr('age', 'gt', 18)|map(attribute='name')|list }}", {"users": [{"name": "a", "age": 20}, {"name": "b", "age": 10}]}),
    ("{{ d['key'] }} {{ d.key }} {{ d.get('missing', 'dflt') }} {{ 'key' in d }}", {"d": {"key": "v"}}),
    ("{{ l[0] }} {{ l[-1] }} {{ l[1:] }} {{ l[::-1] }}", {"l": [1, 2, 3]}),
    ("{{ s[1:3] }} {{ s|length }}", {"s": "hello"}),
    ("{{ big }} {{ big + 1 }} {{ -big }}", {"big": 2**80}),
    ("{{ x|default('d') }} {{ y|default('d') }}", {"x": 0}),
    ("{{ text|e }} {{ text|safe }}", {"text": "<b>&</b>"}),
    ("{% for a, b in pairs %}{{ a }}{{ b }}{% endfor %}", {"pairs": [(1, 2), (3, 4)]}),
    ("{{ range(3)|list }} {{ dict(a=1) }} {{ lipsum(1, false, 3, 4)|length > 0 }}", {}),
    ("{% filter upper %}abc{% endfilter %}", {}),
    ("{{ '%s-%d'|format('a', 3) }} {{ 'x{}y'.format(1) }}", {}),
    ("{{ items|groupby('k')|map(attribute='grouper')|list }}", {"items": [{"k": 1}, {"k": 2}, {"k": 1}]}),
    ("{{ x is defined }} {{ y is undefined }} {{ 3 is odd }} {{ l is iterable }} {{ d is mapping }}", {"x": 1, "l": [1], "d": {}}),
    ("{{ d|dictsort }} {{ d|tojson }}", {"d": {"b": 2, "a": [1, "x"]}}),
    ("{% set ns = namespace(n=0) %}{% for i in range(3) %}{% set ns.n = ns.n + i %}{% endfor %}{{ ns.n }}", {}),
]


@pytest.mark.parametrize("source,context", CASES)
def test_matches_jinja2(source, context):
    assert_same(source, context)


def test_render_takes_a_dict_and_keywords():
    for module in (jinja2cpp, jinja2):
        tpl = module.Template("{{ a }}{{ b }}")
        assert tpl.render({"a": 1, "b": 2}, b=3) == "13"
        assert tpl.render([("a", "x")]) == "x"
        with pytest.raises(TypeError):
            tpl.render({}, {})


def test_template_constructor_takes_options():
    tpl = jinja2cpp.Template("<% if x %>[[ x ]]<% endif %>", block_start_string="<%", block_end_string="%>", variable_start_string="[[", variable_end_string="]]")
    assert tpl.render(x=5) == "5"
    assert isinstance(tpl, jinja2cpp.Template)
    assert isinstance(tpl.environment, jinja2cpp.Environment)


OPTIONS = [
    ({"trim_blocks": True}, "{% if x %}\nyes\n{% endif %}\nend", {"x": 1}),
    ({"lstrip_blocks": True}, "  {% if x %}\n  yes\n  {% endif %}", {"x": 1}),
    ({"trim_blocks": True, "lstrip_blocks": True}, "<ul>\n  {% for i in l %}\n  <li>{{ i }}</li>\n  {% endfor %}\n</ul>\n", {"l": [1, 2]}),
    ({"keep_trailing_newline": True}, "line\n", {}),
    ({}, "line\n", {}),
    ({"newline_sequence": "\r\n"}, "a\nb\n{{ 'c\nd' }}", {}),
    ({"line_statement_prefix": "#", "line_comment_prefix": "##"}, "# for x in l\n{{ x }} ## note\n# endfor\n", {"l": [1, 2]}),
    ({"comment_start_string": "<#", "comment_end_string": "#>"}, "a<# hidden #>b", {}),
    ({"autoescape": True}, "{{ x }} {{ x|safe }} {{ m }}", {"x": "<a>", "m": markupsafe.Markup("<i>")}),
    ({"extensions": ["jinja2.ext.do"]}, "{% set l = [] %}{% do l.append(1) %}{{ l }}", {}),
    ({"extensions": ["jinja2.ext.loopcontrols"]}, "{% for i in range(5) %}{% if i == 3 %}{% break %}{% endif %}{{ i }}{% endfor %}", {}),
    ({"finalize": lambda v: "" if v is None else v}, "[{{ none }}][{{ 1 }}]", {"none": None}),
]


@pytest.mark.parametrize("options,source,context", OPTIONS)
def test_environment_options(options, source, context):
    assert_same(source, context, options)


def test_extension_classes():
    import jinja2.ext

    assert_same("{% do x.append(2) %}{{ x }}", {"x": [1]}, lambda m: {"extensions": [jinja2.ext.do]})
    assert jinja2cpp.Environment(extensions=[jinja2cpp.ext.loopcontrols]).from_string("{% for i in [1, 2] %}{% continue %}{% endfor %}").render() == ""
    with pytest.raises(NotImplementedError):
        jinja2cpp.Environment(extensions=["jinja2.ext.debug"])


def test_autoescape_callable():
    seen = []

    def select(name):
        seen.append(name)
        return name is not None and name.endswith(".html")

    env = jinja2cpp.Environment(autoescape=select, loader=jinja2cpp.DictLoader({"a.html": "{{ x }}", "a.txt": "{{ x }}"}))
    assert env.get_template("a.html").render(x="<>") == "&lt;&gt;"
    assert env.get_template("a.txt").render(x="<>") == "<>"
    assert env.from_string("{{ x }}").render(x="<>") == "<>"
    assert seen == ["a.html", "a.txt", None]
    env = jinja2cpp.Environment(autoescape=jinja2cpp.select_autoescape(), loader=jinja2cpp.DictLoader({"a.html": "{{ x }}", "b.txt": "{{ x }}"}))
    assert env.get_template("a.html").render(x="<") == "&lt;"
    assert env.get_template("b.txt").render(x="<") == "<"


def test_unknown_option_and_bad_newline():
    with pytest.raises(TypeError):
        jinja2cpp.Environment(no_such_option=1)
    with pytest.raises(AssertionError):
        jinja2cpp.Environment(newline_sequence="x")
    with pytest.raises(NotImplementedError):
        jinja2cpp.Environment(enable_async=True)
    env = jinja2cpp.Environment(optimized=False, bytecode_cache=None, cache_size=10, auto_reload=False)
    assert env.cache_size == 10 and env.auto_reload is False


def test_options_set_after_construction():
    env = jinja2cpp.Environment()
    env.trim_blocks = True
    assert env.trim_blocks is True
    assert env.from_string("{% if 1 %}\nx{% endif %}").render() == "x"


def test_globals_filters_tests():
    for module in (jinja2cpp, jinja2):
        env = module.Environment()
        env.globals["site"] = {"name": "S"}
        env.globals["greet"] = lambda who: f"hi {who}"
        env.filters["twice"] = lambda v, sep="": f"{v}{sep}{v}"
        env.tests["short"] = lambda v, n=3: len(v) <= n
        tpl = env.from_string("{{ site.name }} {{ greet('x') }} {{ 'ab'|twice('-') }} {{ 'ab' is short }} {{ 'abcd' is short(2) }}")
        assert tpl.render() == "S hi x ab-ab True False"
        # Context over globals, template globals over environment globals
        assert env.from_string("{{ site }}").render(site=1) == "1"
        assert env.from_string("{{ site }}", globals={"site": 2}).render() == "2"


def test_globals_seen_by_imported_template():
    for module in (jinja2cpp, jinja2):
        env = module.Environment(loader=module.DictLoader({"lib": "{% macro m() %}{{ g }}{% endmacro %}", "main": "{% import 'lib' as lib %}{{ lib.m() }}"}))
        env.globals["g"] = "G"
        assert env.get_template("main").render() == "G"


def test_template_globals_seen_by_imported_template():
    # Jinja2 3.1 passes the importer's template globals on to the module it imports (verifier N4)
    for module in (jinja2cpp, jinja2):
        lib = "{% macro f() %}[{{ g }}|{{ e }}]{% endmacro %}{% set top = g %}"
        env = module.Environment(loader=module.DictLoader({"lib": lib, "main": "{% import 'lib' as m %}{{ m.f() }}{{ m.top }}", "from": "{% from 'lib' import f %}{{ f() }}", "inc": "{% include 'leaf' %}", "leaf": "{{ g }}"}))
        env.globals["e"] = "E"
        assert env.get_template("main", globals={"g": "G"}).render() == "[G|E]G"
        assert env.get_template("from", globals={"g": "G"}).render() == "[G|E]"
        assert env.get_template("inc", globals={"g": "G"}).render() == "G"


def test_template_keeps_the_globals_dict_it_was_made_with():
    # Jinja2's template.globals chains the Environment.globals dict of its time: a dict set
    # later is not seen, a change to that dict is (verifier N4)
    for module in (jinja2cpp, jinja2):
        env = module.Environment()
        env.globals["e"] = "E"
        before = env.from_string("{{ e }}")
        env.globals = {"e": "NEW"}
        after = env.from_string("{{ e }}")
        env.globals["e"] = "MUT"
        assert (before.render(), after.render()) == ("E", "MUT"), module.__name__


def test_filter_added_after_load_binds_at_next_load():
    env = jinja2cpp.Environment()
    env.filters["f"] = lambda v: "one"
    first = env.from_string("{{ 1|f }}")
    env.filters["f"] = lambda v: "two"
    assert first.render() == "one"
    assert env.from_string("{{ 1|f }}").render() == "two"
    with pytest.raises(jinja2cpp.TemplateAssertionError):
        env.from_string("{{ 1|nosuch }}")


def test_unicode():
    assert_same("{{ s }} {{ s|upper }} {{ s|length }} {{ s[1] }}", {"s": "żółw 🐢"})


def test_template_globals_is_one_mapping_that_renders_read():
    for module in (jinja2cpp, jinja2):
        env = module.Environment()
        env.globals["e"] = "E"
        tpl = env.from_string("{{ x }}{{ e }}")
        assert tpl.globals["e"] == "E", module.__name__
        tpl.globals["x"] = 1
        assert tpl.render() == "1E", module.__name__
        assert "x" not in env.globals, module.__name__
        env.globals["e"] = "F"
        assert tpl.render() == "1F", module.__name__
