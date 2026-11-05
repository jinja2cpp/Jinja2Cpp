"""The four Undefined policies, by jinja2cpp's classes and by Jinja2's own."""

import jinja2
import jinja2cpp
import pytest

from conftest import render_both

NAMES = ["Undefined", "ChainableUndefined", "DebugUndefined", "StrictUndefined"]

SOURCES = [
    "[{{ x }}]",
    "[{{ x.y }}]",
    "[{{ x.y.z }}]",
    "[{{ x['k'] }}]",
    "{{ x is defined }} {{ x is undefined }}",
    "{% if x %}t{% else %}f{% endif %}",
    "{% for i in x %}{{ i }}{% endfor %}.",
    "{{ x|default('d') }} {{ x|length }}",
    "{{ x == x }}",
    "{{ x + 1 }}",
    "{{ d.missing }}",
    "{{ l[5] }}",
]


@pytest.mark.parametrize("name", NAMES)
@pytest.mark.parametrize("source", SOURCES)
@pytest.mark.parametrize("ours", [True, False], ids=["jinja2cpp-class", "jinja2-class"])
def test_policy(name, source, ours):
    def options(module):
        # jinja2cpp takes its own classes and Jinja2's
        cls_module = module if ours or module is jinja2 else jinja2
        return {"undefined": getattr(cls_module, name)}

    got, expected = render_both(source, {"d": {}, "l": []}, options)
    assert got == expected, f"{name} {source!r}: jinja2cpp {got!r}, Jinja2 {expected!r}"


def test_subclass_takes_the_nearest_policy():
    class Loud(jinja2cpp.StrictUndefined):
        pass

    with pytest.raises(jinja2cpp.UndefinedError):
        jinja2cpp.Environment(undefined=Loud).from_string("{{ x }}").render()
    env = jinja2cpp.Environment(undefined=Loud)
    assert env.undefined is Loud


def test_bad_undefined_class():
    with pytest.raises(TypeError):
        jinja2cpp.Environment(undefined=int)
    with pytest.raises(TypeError):
        jinja2cpp.Environment(undefined=1)


def test_undefined_from_python_is_undefined():
    for value in (jinja2cpp.Undefined(name="v"), jinja2.Undefined(name="v")):
        assert jinja2cpp.Template("[{{ u }}] {{ u is defined }}").render(u=value) == "[] False"
    with pytest.raises(jinja2cpp.UndefinedError):
        jinja2cpp.Template("{{ u }}").render(u=jinja2cpp.StrictUndefined(name="v"))
    assert jinja2cpp.is_undefined(jinja2.Undefined()) and jinja2cpp.is_undefined(jinja2cpp.Undefined())
    assert not jinja2cpp.is_undefined(None)
