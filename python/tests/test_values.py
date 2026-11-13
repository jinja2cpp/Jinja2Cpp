"""Values crossing between Python and the engine (0248 D4, D6), against Python Jinja2."""

import collections
import dataclasses
import decimal
import fractions
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



class OwnIterList(list):
    """A list subclass with its own iteration: Python loops through __iter__, items stay the list's (task 0441)"""

    def __iter__(self):
        return iter(["x", "y"])


class OwnIterTuple(tuple):
    def __iter__(self):
        return iter(["x", "y"])


class OwnItemList(list):
    """Overrides only __getitem__: Python's list iterator still gives the list's own items"""

    def __getitem__(self, index):
        return "G"


class OwnItemTuple(tuple):
    def __getitem__(self, index):
        return "G"


class PlainSubList(list):
    pass


class SizedIterable:
    """Not a sequence: a length of its own and an iteration of another length"""

    def __len__(self):
        return 7

    def __iter__(self):
        return iter("ab")

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
    # operators, sort, min, max and `%` read a list or tuple as Jinja2's own (verifier N1)
    ("{{ a + b }} {{ a + [3] }} {{ [0] + a }} {{ t + (3,) }} {{ a * 2 }} {{ 2 * t }}", {"a": [1], "b": [2], "t": (1, 2)}),
    ("{{ a < b }} {{ t >= (1, 2) }} {{ a == [1] }} {{ t == (1, 2) }} {{ a != t }}", {"a": [1], "b": [2], "t": (1, 2)}),
    ("{{ l|sort }} {{ l|min }} {{ l|max }} {{ n|sort(reverse=true) }} {{ n|max }}", {"l": [(2, "b"), (1, "a")], "n": [[2], [1], [3]]}),
    ("{{ '%s' % (1,) }} {{ '%s' % t }} {{ '%s' % l }} {{ 'x' % l }} {{ '%s-%s' % u }} {{ '%s-%s' % p }} {{ '%(a)s' % d }}",
     {"t": (1,), "l": [1], "u": (1, 2), "p": Point(1, 2), "d": {"a": 1}}),
    ("{{ d|urlencode }} {{ l|urlencode }} {{ t|reverse|list }}", {"d": {"a": 1, "b": "x y"}, "l": [("a", 1)], "t": (1, 2)}),
    # a loop follows the live end of a list the template changes (verifier N3)
    ("{% for x in l %}{{ loop.length }}{{ x }}{% do l.pop() %}{% endfor %}", {"l": [1, 2, 3, 4]}),
    ("{% for x in l %}{{ x }}{% if l|length < 4 %}{% do l.append(9) %}{% endif %}{% endfor %}", {"l": [1, 2]}),
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


TYPE_ERRORS = [
    ("{{ a + t }}", {"a": [1], "t": (2,)}),
    ("{{ a - a }}", {"a": [1]}),
    ("{{ d < d }}", {"d": {}}),
    ("{{ '%s' % t }}", {"t": (1, 2)}),
]


@pytest.mark.parametrize("source,context", TYPE_ERRORS)
def test_type_errors_of_lists_and_tuples(source, context):
    # Jinja2's TypeError is TemplateRuntimeError here, with Python's message (docs/python.md)
    with pytest.raises(TypeError) as expected:
        jinja2.Environment().from_string(source).render(**context)
    with pytest.raises(jinja2cpp.TemplateRuntimeError) as got:
        jinja2cpp.Environment().from_string(source).render(**context)
    assert str(got.value) == str(expected.value)


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


def test_deeply_nested_list_to_callable_raises_not_crashes():
    # A loop nests lists without nested syntax; converting the result for a Python callable
    # used to overflow the native stack
    env = jinja2cpp.Environment()
    template = env.from_string(
        "{% set ns = namespace(x=[]) %}{% for i in range(100000) %}{% set ns.x = [ns.x] %}{% endfor %}{{ f(ns.x) }}"
    )
    with pytest.raises((RecursionError, jinja2cpp.TemplateError)):
        template.render(f=lambda value: "ok")


def test_deeply_nested_python_list_in_is_safe():
    deep = []
    for _ in range(100000):
        deep = [deep]
    env = jinja2cpp.Environment()
    try:
        env.from_string("{{ x }}{{ x|length }}{{ x|tojson }}").render(x=deep)
    except (RecursionError, jinja2cpp.TemplateError):
        pass


def test_a_huge_range_passed_to_python_counts_against_the_sequence_cap():
    # A range reaches Python as a list (an accepted divergence of 0357); a lazy one is not built past the cap
    env = jinja2cpp.Environment()
    with pytest.raises(jinja2cpp.TemplateRuntimeError, match="too long"):
        env.from_string("{{ f(range(1000000000000)) }}").render(f=len)
    assert env.from_string("{{ f(range(5)) }}").render(f=len) == "5"


def test_undefined_items_and_dictsort_pairs_reach_python_as_in_jinja2():
    cases = [
        ("{{ f([missing]) }}", lambda xs: type(xs[0]).__name__),
        ("{{ f({'m': missing}) }}", lambda d: type(d["m"]).__name__),
        ("{{ f({'a': 1}|dictsort) }}", repr),
        ("{{ f({'a': 1}.items()|list) }}", repr),
        # Each pair is made as its list is read: a pair released early must not reuse an earlier pair's place
        ("{{ f({'a': 1, 'b': 2, 'c': 3, 'd': 4, 'e': 5}|dictsort) }}", repr),
        ("{{ f([{'a': 1, 'b': 2, 'c': 3}|dictsort, {'d': 4, 'e': 5}|dictsort]) }}", repr),
    ]
    for source, f in cases:
        assert jinja2cpp.Template(source).render(f=f) == jinja2.Template(source).render(f=f), source


# Built afresh for each engine: render_both deep-copies its context, and a deep copy of a list subclass is rebuilt from
# its __iter__, which would hide what these rows check (task 0441)
SUBCLASS_CASES = [
    "{% for i in l %}{{ i }}{% endfor %}|{% for i in t %}{{ i }}{% endfor %}|{% for i in p %}{{ i }}{% endfor %}",
    "{{ l|join('-') }} {{ l|list }} {{ l[1] }} {{ l|length }} {{ t|list }} {{ 'x' in l }} {{ l|first }} {{ l|sort }}",
    "{% for i in g %}{{ i }}{% endfor %}|{% for i in gt %}{{ i }}{% endfor %}|{{ g|list }} {{ g|join }} {{ g|first }}",
    "{{ g|sort }} {{ g|sum }} {{ g|reverse|list }} {{ g[0] }} {{ gt[0] }}",
    "{% for i in p %}{{ i }}{{ p.pop() if loop.first else '' }}{% endfor %}",
    # loop.length is len() of the object; operators and `%` read the stored items
    "{% for i in l %}{{ loop.length }}{{ loop.revindex }}{{ loop.revindex0 }}{{ loop.last }};{% endfor %}",
    "{% for i in t %}{{ loop.length }}{% endfor %}|{% for i in n %}{{ i }}{{ loop.length }}{{ loop.last }}{% endfor %}",
    "{{ l + ['z'] }} {{ l * 2 }} {{ t + ('z',) }} {{ l < [1, 3] }} {{ t > (1,) }} {{ '%s-%s-%s' % t }} {{ l|length }}",
]


def _subclass_context():
    return {
        "l": OwnIterList([1, 2, 3]),
        "t": OwnIterTuple((1, 2, 3)),
        "p": PlainSubList([1, 2, 3]),
        "g": OwnItemList([1, 2, 3]),
        "gt": OwnItemTuple((1, 2, 3)),
        "n": SizedIterable(),
    }


@pytest.mark.parametrize("source", SUBCLASS_CASES)
def test_list_and_tuple_subclasses_iterate_as_python_does(source):
    results = []
    for module in (jinja2cpp, jinja2):
        env = module.Environment()
        results.append(env.from_string(source).render(**_subclass_context()))
    assert results[0] == results[1], f"{source!r}: jinja2cpp {results[0]!r}, Jinja2 {results[1]!r}"


class SecretList(list):
    def __repr__(self):
        return "<redacted>"


# pprint breaks a Python list or tuple as pformat does (0434); a subclass prints as its repr, which may be its own (a
# namedtuple, one that hides its items). A subclass that keeps list.__repr__ is docs/tasks/0453
@pytest.mark.parametrize(
    "value",
    [
        list(range(40)),
        tuple(range(40)),
        (list(range(40)),),
        [[1] * 40, {"a": (1,) * 40}],
        PlainSubList(range(3)),
        Point(list(range(40)), 1),
        SecretList(range(40)),
        [SecretList(["pw"])] * 30,
        {"k": SecretList(["pw"])},
        list(range(1000)),
    ],
)
def test_pprint_breaks_python_sequences_as_pformat(value):
    assert_same("{{ v|pprint }}", {"v": value})


# A call that raises StopIteration is undefined, as Jinja2's Context.call makes it (0434)
def test_stop_iteration_from_a_call_is_undefined():
    def stop():
        raise StopIteration

    assert_same("A{{ f() }}B|{{ f() is defined }}", {"f": stop})
    for module in (jinja2cpp, jinja2):
        with pytest.raises(module.UndefinedError):
            module.Environment().from_string("{{ f().x }}").render(f=stop)
    # A filter, test or finalize is not called through Context.call: the render fails with the RuntimeError Python
    # makes of a StopIteration out of a generator, which Jinja2's render is (PEP 479, docs/tasks/0477)
    for module in (jinja2cpp, jinja2):
        env = module.Environment(finalize=lambda value: stop() if value == "f" else value)
        env.filters["stop"] = lambda value: stop()
        env.tests["stop"] = lambda value: stop()
        env.globals["f"] = stop
        sources = ["{{ 1|stop }}", "{{ 1 is stop }}", "{{ 'f' }}", "{% for x in [1]|stop %}{% endfor %}"]
        for source in sources:
            template = env.from_string(source)
            calls = (template.render, lambda: list(template.generate()), lambda: template.module, lambda: list(template.stream()))
            for call in calls:
                with pytest.raises(RuntimeError, match="^generator raised StopIteration$") as raised:
                    call()
                assert isinstance(raised.value.__cause__, StopIteration), source
            try:  # an exception the caller is handling is not the context: the StopIteration is
                raise ValueError
            except ValueError:
                with pytest.raises(RuntimeError) as raised:
                    template.render()
            assert isinstance(raised.value.__context__, StopIteration), source
    # A macro the template calls goes through Context.call too: undefined, in both engines (docs/tasks/0479)
    for module in (jinja2cpp, jinja2):
        env = module.Environment()
        env.filters["stop"] = lambda value: stop()
        template = env.from_string("{% macro m() %}{{ 1|stop }}{% endmacro %}[{{ m() }}]")
        assert template.render() == "[]"


class _Page:
    """A host object whose property raises StopIteration, as an attribute lookup in a macro."""

    @property
    def stopping(self):
        raise StopIteration

    def __getitem__(self, key):
        raise StopIteration


def _stop(*args, **kwargs):
    raise StopIteration


def _stop_env(module, undefined=None, **kwargs):
    env = module.Environment(
        undefined=undefined or module.Undefined,
        loader=module.DictLoader(
            {
                "inc": "{{ 1|stop }}",
                "imp": "{% macro y() %}a{{ 1|stop }}b{% endmacro %}",
                "par": "{% block b %}{% if flag %}{{ 1|stop }}{% endif %}{% endblock %}",
                "kid": "{% extends 'par' %}{% block b %}{% macro m() %}<{{ super() }}>{% endmacro %}[{{ m() }}]{% endblock %}",
            }
        ),
        **kwargs,
    )
    env.filters["stop"] = _stop
    env.tests["stop"] = _stop
    env.globals["f"] = lambda *args: "F"
    env.globals["page"] = _Page()
    return env


def _stop_outcome(module, source, undefined=None, mode="render", **context):
    env = _stop_env(module, undefined)
    try:
        template = env.from_string(source)
        if mode == "generate":
            return list(template.generate(**context))
        if mode == "stream":
            return list(template.stream(**context))
        return template.render(**context)
    except Exception as error:  # noqa: BLE001 - compared below
        return error


_STOP_MACRO = "{% macro m() %}a{{ 1|stop }}b{% endmacro %}"
_STOP_UNDEFINED = ["Undefined", "StrictUndefined", "DebugUndefined", "ChainableUndefined"]

# Rendered the same by both engines under every undefined class (the expected text is Jinja2's)
_STOP_SAME = [
    _STOP_MACRO + "[{{ m() }}]",
    _STOP_MACRO + "[{{ m()|upper }}]",
    _STOP_MACRO + "{% set x = m() %}[{{ x }}]",
    _STOP_MACRO + "{{ m() is defined }}|{{ m() is undefined }}|{{ m()|default('D') }}|{{ [m()]|length }}",
    _STOP_MACRO + "[{{ m() ~ 'z' }}]",
    _STOP_MACRO + "{% for x in [1] %}[{{ m() }}]{% endfor %}",
    "{% macro c() %}<{{ caller() }}>{% endmacro %}{% call c() %}x{{ 1|stop }}{% endcall %}|after",
    "{% macro m(caller=none) %}{{ caller() }}{% endmacro %}[{% call m() %}{{ 1|stop }}{% endcall %}]",
    "{% macro d(a=1|stop) %}{{ a }}{% endmacro %}[{{ d() }}]",
    "{% macro i() %}{{ 1|stop }}{% endmacro %}{% macro o() %}a{{ i() }}b{% endmacro %}[{{ o() }}]",
    "{% macro t() %}x{{ 1 is stop }}{% endmacro %}[{{ t() }}]",
    "{% for i in [[1]] recursive %}<{% if i is iterable %}{{ loop(i) }}{% else %}{{ i|stop }}{% endif %}>{% endfor %}",
    "{% macro m() %}a{{ page.stopping }}b{% endmacro %}[{{ m() }}]",
    "{% macro m() %}a{{ page['k'] }}b{% endmacro %}[{{ m() }}]",
    "{% import 'imp' as x %}[{{ x.y() }}]",
    "{% from 'imp' import y %}[{{ y() }}]",
    "{% macro m(x) %}{% set v = x %}{% for q in [1, 2] %}{{ q }}{{ 1|stop }}{% endfor %}{% endmacro %}"
    "{% set v = 'o' %}{% for q in 'ab' %}[{{ m(q) }}{{ v }}{{ q }}]{% endfor %}{{ v }}",
    "{% autoescape true %}{% macro m() %}<b>{{ 1|stop }}{% endmacro %}[{{ m() }}]{% endautoescape %}",
]


@pytest.mark.parametrize("source", _STOP_SAME)
@pytest.mark.parametrize("undefined", _STOP_UNDEFINED)
def test_stop_iteration_out_of_a_macro_is_undefined(source, undefined):
    # A macro, caller(), loop(...) and a default go through Context.call (docs/tasks/0479)
    results = []
    for module in (jinja2cpp, jinja2):
        outcome = _stop_outcome(module, source, getattr(module, undefined))
        results.append(type(outcome).__name__ if isinstance(outcome, Exception) else outcome)
    assert results[0] == results[1], f"{source!r}: jinja2cpp {results[0]!r}, Jinja2 {results[1]!r}"


def test_stop_iteration_out_of_a_macro_in_generate_and_stream():
    source = _STOP_MACRO + "[{{ m() }}]"
    for mode in ("generate", "stream"):
        outcomes = [_stop_outcome(module, source, mode=mode) for module in (jinja2cpp, jinja2)]
        # Jinja2 yields the stopped call's empty piece as an item of its own
        assert [item for item in outcomes[0] if item] == [item for item in outcomes[1] if item] == ["[", "]"], mode


def test_stop_iteration_out_of_a_long_macro_in_generate_keeps_the_runtime_error():
    # The macro's output left the buffer before it stopped: it cannot be taken back (docs/python.md)
    source = "{% macro m() %}{{ 'x' * 600 }}{{ 1|stop }}{% endmacro %}[{{ m() }}]"
    assert "".join(_stop_outcome(jinja2, source, mode="generate")) == "[]"
    with pytest.raises(RuntimeError, match="^generator raised StopIteration$"):
        raise _stop_outcome(jinja2cpp, source, mode="generate")
    assert _stop_outcome(jinja2cpp, source) == "[]"


# What Jinja2 does not give undefined: the call is not Context.call, or a generator ends the render
_STOP_ERRORS = [
    (_STOP_MACRO + "{{ m().x }}", "UndefinedError"),
    (_STOP_MACRO + "[{% call m() %}x{% endcall %}]", "TypeError"),
    ("{{ f(1|stop) }}", "RuntimeError"),
    ("{% macro m() %}a{% include 'inc' %}b{% endmacro %}[{{ m() }}]", "RuntimeError"),
    ("{% macro m() %}a{% import 'inc' as b %}b{% endmacro %}[{{ m() }}]", "RuntimeError"),
    ("{% macro m() %}a{% block b %}{{ 1|stop }}{% endblock %}z{% endmacro %}[{{ m() }}]", "RuntimeError"),
    (
        "{% block b %}{% if flag %}{{ 1|stop }}{% endif %}{% endblock %}{% set flag = true %}"
        "{% macro m() %}x{{ self.b() }}y{% endmacro %}[{{ m() }}]",
        "RuntimeError",
    ),
]


@pytest.mark.parametrize("source,error", _STOP_ERRORS)
def test_stop_iteration_where_jinja2_does_not_stop_the_call(source, error):
    for module in (jinja2cpp, jinja2):
        outcome = _stop_outcome(module, source)
        assert type(outcome).__name__ == error, (module.__name__, source, outcome)
        if error == "RuntimeError":
            assert str(outcome) == "generator raised StopIteration"
            assert isinstance(outcome.__cause__, StopIteration)


def test_stop_iteration_out_of_super_in_a_macro_is_a_runtime_error():
    # super() renders the parent's block through a generator, as a block does
    for module in (jinja2cpp, jinja2):
        template = _stop_env(module).get_template("kid")
        assert template.render(flag=False) == "[<>]"
        with pytest.raises(RuntimeError, match="^generator raised StopIteration$"):
            template.render(flag=True)


def test_stop_iteration_from_a_module_macro_reaches_the_host():
    # A host's own call of a module's macro is not Context.call: the exception is the caller's
    for module in (jinja2cpp, jinja2):
        env = _stop_env(module)
        built = env.from_string(_STOP_MACRO).module
        with pytest.raises(StopIteration):
            built.m()


class IntOnly:
    def __int__(self):
        return 42


class FloatOnly:
    def __float__(self):
        return 3.75


class IntAndFloat:
    def __int__(self):
        return 7

    def __float__(self):
        return 2.5


class Raises:
    def __int__(self):
        raise RuntimeError("no int")

    def __float__(self):
        raise RuntimeError("no float")


@pytest.mark.parametrize(
    "value",
    [IntOnly(), FloatOnly(), IntAndFloat(), object(), complex(1, 2), decimal.Decimal("3.9"), decimal.Decimal("NaN"), fractions.Fraction(7, 2), [1], {"a": 1}],
)
def test_numbers_of_an_object(value):
    for source in ("{{ x is number }}", "{{ x|int }}", "{{ x|int(default=7) }}", "{{ x|float }}", "{{ x|float(default=1.5) }}"):
        assert_same(source, {"x": value})


class FloatOverflows:
    def __int__(self):
        raise ValueError("no int")

    def __float__(self):
        raise OverflowError("too large")


def test_int_of_an_object_whose_float_overflows():
    # do_int catches OverflowError around int(float(x)); do_float lets it through
    for source in ("{{ x|int }}", "{{ x|int(default=7) }}"):
        assert_same(source, {"x": FloatOverflows()})
    for module in (jinja2cpp, jinja2):
        with pytest.raises(OverflowError):
            module.Environment().from_string("{{ x|float }}").render(x=FloatOverflows())


def test_numbers_of_an_object_that_raises():
    for source in ("{{ x|int }}", "{{ x|float }}"):
        for module in (jinja2cpp, jinja2):
            with pytest.raises(RuntimeError, match="no "):
                module.Environment().from_string(source).render(x=Raises())
