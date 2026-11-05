"""Values crossing between Python and the engine (0248 D4, D6), against Python Jinja2."""

import collections
import dataclasses
import types

import jinja2
import jinja2cpp
import markupsafe
import pytest

from conftest import assert_same

Point = collections.namedtuple("Point", "x y")


@dataclasses.dataclass
class User:
    name: str
    age: int

    def greet(self, other="you"):
        return f"{self.name} greets {other}"


class Html:
    def __html__(self):
        return "<b>html</b>"

    def __str__(self):
        return "<plain>"


class Items:
    """An object with items but no attributes of that name: getattr first, then getitem"""

    title = "attribute"

    def __getitem__(self, key):
        return f"item {key}"


class Box(collections.abc.Mapping):
    """A Mapping that is no dict"""

    def __init__(self, **items):
        self._items = items

    def __getitem__(self, key):
        return self._items[key]

    def __iter__(self):
        return iter(self._items)

    def __len__(self):
        return len(self._items)


CASES = [
    # dict: items, attributes, methods, non-string keys, filters that want a mapping
    ("{{ d.a }} {{ d['a'] }} {{ d.get('z', 0) }} {{ d.items()|list }} {{ d|length }}", {"d": {"a": 1, "b": 2}}),
    ("{{ d[1] }} {{ d[(1, 2)] }} {{ d[none] }} {{ 1 in d }} {{ 'x' in d }}", {"d": {1: "one", (1, 2): "pair", None: "none"}}),
    ("{{ d is mapping }} {{ d|dictsort }} {{ d|dictsort(by='value', reverse=true) }}", {"d": {"b": 1, "a": 2}}),
    ("{{ d|tojson }} {{ d|items|list }} {{ d|list }} {{ d|xmlattr }}", {"d": {"b": [1, (2, None)], "a": "x"}}),
    ("{% for k, v in d.items() %}{{ k }}={{ v }};{% endfor %}{% for k in d %}{{ k }}{% endfor %}", {"d": {"a": 1, "b": 2}}),
    ("{{ 'items' in d }} {{ 'keys' in d }} {{ d.keys() }}", {"d": {"a": 1}}),
    ("{{ m is mapping }} {{ m.a }} {{ m|dictsort }} {{ m|items|list }} {{ 'a' in m }} {{ m|length }}", {"m": Box(a=1)}),
    # list and tuple: printing, indexing, slicing, methods, namedtuple fields
    ("{{ l }} {{ t }} {{ (t,) }} {{ e }} {{ l[1:] }} {{ t[::-1] }} {{ l.index(2) }} {{ t.count(1) }}", {"l": [1, 2], "t": (1, "a"), "e": ()}),
    ("{{ p }} {{ p.x }} {{ p[1] }} {{ p|list }} {{ p|tojson }} {{ p|length }}", {"p": Point(1, 2)}),
    ("{{ ps|map(attribute='y')|list }} {{ ps|sort(attribute='x')|map(attribute='x')|list }}", {"ps": [Point(3, 1), Point(1, 2)]}),
    ("{{ l is sequence }} {{ t is iterable }} {{ l|tojson }} {{ l|sum }} {{ l|max }} {{ t|join('-') }}", {"l": [3, 1, 2], "t": ("a", "b")}),
    # mutation is seen both ways
    ("{% do l.append(3) %}{% do d.update(z=1) %}{{ l }} {{ d }}", {"l": [1], "d": {"a": 0}}),
    # objects: attributes, methods, str/repr, __html__, getattr before getitem
    ("{{ u }} {{ u.name }} {{ u.greet() }} {{ u.greet(other='me') }} {{ [u] }}", {"u": User("ann", 3)}),
    ("{{ u.missing is defined }} {{ u.age + 1 }} {{ u|attr('name') }}", {"u": User("ann", 3)}),
    ("{{ h }} {{ h|e }} {{ [h]|join }} {{ h is escaped }}", {"h": Html()}),
    ("{{ i.title }} {{ i['title'] }} {{ i.other }} {{ i[3] }}", {"i": Items()}),
    ("{{ ns.a }} {{ ns.b|default('d') }}", {"ns": types.SimpleNamespace(a=1)}),
    # generators and iterators are read once
    ("{% for x in g %}{{ x }}{% endfor %}|{% for x in g %}{{ x }}{% endfor %}", {"g": (i * i for i in range(4))}),
    ("{{ g|list }} {{ g|list }}", {"g": iter([1, 2])}),
    # scalars
    ("{{ big }} {{ big + 1 }} {{ -big }} {{ big * big }} {{ big // 3 }} {{ big is integer }}", {"big": 2**80}),
    ("{{ n }} {{ n + 1 }} {{ n is integer }}", {"n": -(2**63)}),
    ("{{ f }} {{ f|round(1) }} {{ b }} {{ b + 1 }} {{ none }} {{ none is none }}", {"f": 2.25, "b": True, "none": None}),
    ("{{ s }} {{ s|length }} {{ s|reverse }} {{ s[2:] }}", {"s": "żółw 🐢 \x00 end"}),
    ("{{ e }} {{ e|int }}", {"e": __import__("enum").IntEnum("E", "A B").B}),
]


@pytest.mark.parametrize("source,context", CASES)
def test_matches_jinja2(source, context):
    assert_same(source, context, {"extensions": ["jinja2.ext.do"]})


@pytest.mark.parametrize("autoescape", [False, True])
def test_markup(autoescape):
    assert jinja2cpp.Markup is markupsafe.Markup
    assert jinja2cpp.escape is markupsafe.escape
    context = {"m": markupsafe.Markup("<i>x</i>"), "s": "<i>"}
    assert_same("{{ m }} {{ s }} {{ m ~ s }} {{ s|e }} {{ m|length }} {{ (m ~ '')|e }}", context, {"autoescape": autoescape})


def test_outbound_types_and_identity():
    seen = []

    def record(*args, **kwargs):
        seen.append((args, kwargs))
        return args[0] if args else None

    obj = User("ann", 3)
    data = {"k": [1]}
    env = jinja2cpp.Environment(autoescape=True)
    env.globals["record"] = record
    env.from_string(
        "{{ record(obj, data, data.k, 1, 2.5, 'x', none, true, 2**70, [1, (2,)], {'a': 1}, 'a'|safe, missing, kw=obj) }}"
    ).render(obj=obj, data=data)
    args, kwargs = seen[0]
    assert args[0] is obj and kwargs["kw"] is obj
    assert args[1] is data and args[2] is data["k"]
    assert args[3:9] == (1, 2.5, "x", None, True, 2**70)
    assert type(args[3]) is int and type(args[4]) is float and type(args[5]) is str
    assert args[9] == [1, (2,)] and type(args[9]) is list and type(args[9][1]) is tuple
    assert args[10] == {"a": 1} and type(args[10]) is dict
    assert type(args[11]) is markupsafe.Markup and args[11] == "a"
    assert isinstance(args[12], jinja2cpp.Undefined)
    # A Python object a filter returns comes back as itself
    env.filters["same"] = lambda value: value
    assert env.from_string("{{ (o|same).name }}").render(o=obj) == "ann"


def test_undefined_argument_is_the_environments_class():
    got = []
    for module in (jinja2cpp, jinja2):
        for cls in ("Undefined", "ChainableUndefined", "DebugUndefined"):
            env = module.Environment(undefined=getattr(module, cls))
            env.filters["kind"] = lambda v: f"{type(v).__name__}:{str(v)}"
            got.append(env.from_string("{{ nope|kind }}").render())
    assert got[:3] == got[3:]
    env = jinja2cpp.Environment(undefined=jinja2.StrictUndefined)
    env.filters["kind"] = lambda v: type(v).__name__
    assert env.from_string("{{ nope|kind }}").render() == "StrictUndefined"


def test_lone_surrogate_raises_unicode_encode_error():
    with pytest.raises(UnicodeEncodeError):
        jinja2cpp.Template("{{ s }}").render(s="a\ud800b")
    with pytest.raises(UnicodeEncodeError):
        jinja2cpp.Template("\ud800")


def test_python_callables():
    def f(a, b=2, *args, c=3, **kwargs):
        return f"{a} {b} {args} {c} {sorted(kwargs.items())}"

    assert_same("{{ f(1) }} | {{ f(1, 5, 6, 7, c=8, d=9) }} | {{ f(b=1, a=2) }}", {"f": f})
    assert_same("{{ m.upper() }} {{ len(l) }} {{ sorted(l, reverse=true) }}", {"m": "ab", "len": len, "l": [2, 1], "sorted": sorted})


def test_macro_passed_to_python():
    env = jinja2cpp.Environment()
    kept = []

    def call_twice(macro, value):
        kept.append(macro)
        return macro(value) + macro(value=value + 1)

    env.globals["call_twice"] = call_twice
    tpl = env.from_string("{% macro m(value) %}<{{ value }}>{% endmacro %}{{ call_twice(m, 1) }}")
    assert tpl.render() == "<1><2>"
    # The macro is lent for the call: afterwards it raises rather than reading a finished render
    with pytest.raises(jinja2cpp.TemplateRuntimeError):
        kept[0](1)


def test_caller_and_loop_passed_to_python():
    env = jinja2cpp.Environment()
    env.globals["wrap"] = lambda caller: "[" + caller() + "]"
    env.globals["idx"] = lambda loop: loop.index
    assert env.from_string("{% call wrap() %}body{% endcall %}").render() == "[body]"
    assert env.from_string("{% for x in 'ab' %}{{ idx(loop) }}{% endfor %}").render() == "12"
