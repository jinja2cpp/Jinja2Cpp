"""Values crossing between Python and the engine (0248 D4, D6), against Python Jinja2."""

import array
import collections
import collections.abc
import dataclasses
import datetime
import decimal
import fractions
import itertools
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


@dataclasses.dataclass(frozen=True)
class Key:
    """Orders by __lt__ only, as a host object that has no other operator"""

    value: int

    def __lt__(self, other):
        return self.value < other.value

    def __repr__(self):
        return f"Key({self.value})"


@dataclasses.dataclass
class Holder:
    key: Key


class Stops:
    def __add__(self, other):
        raise StopIteration


class Index:
    """Has __index__ and __float__ only: np.int64's way to be a number to %d, %x and %f"""

    def __index__(self):
        return 5

    def __float__(self):
        return 2.5


DEC = decimal.Decimal
FRAC = fractions.Fraction

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
    # operators of host objects are Python's: Decimal, Fraction, datetime, a user __lt__ (task 0381)
    ("{{ d + 1 }} {{ 1 + d }} {{ d - 1 }} {{ 10 - d }} {{ d * 2 }} {{ 2 * d }} {{ d / 2 }} {{ 1 / d }} {{ d // 2 }} {{ d % 2 }} {{ d ** 2 }}", {"d": DEC("3")}),
    ("{{ d + d }} {{ d * d }} {{ d - e }} {{ d < e }} {{ e < d }} {{ d >= 3 }} {{ 4 > d }} {{ 3 <= d }}", {"d": DEC("3.5"), "e": DEC("1.25")}),
    ("{{ -d }} {{ +d }} {{ d is odd }} {{ d is even }} {{ d is divisibleby 3 }} {{ d < 4 }}", {"d": DEC("3")}),
    ("{{ ds|max }} {{ ds|min }} {{ ds|sum }} {{ ds|sort }} {{ ds|sort(reverse=true) }} {{ ds|sum(start=1) }}", {"ds": [DEC("2.5"), DEC("1"), DEC("3")]}),
    ("{{ a + b }} {{ a - b }} {{ a * b }} {{ a / b }} {{ a // b }} {{ a % b }} {{ a ** 2 }} {{ -a }} {{ a < b }} {{ a + 1 }} {{ 2 * a }}", {"a": FRAC(1, 2), "b": FRAC(1, 3)}),
    ("{{ [a, b]|max }} {{ [a, b]|sort }} {{ [a, b]|sum }}", {"a": FRAC(1, 2), "b": FRAC(1, 3)}),
    ("{{ t + d }} {{ d + t }} {{ t - d }} {{ t - u }} {{ d - d }} {{ d * 2 }} {{ -d }} {{ t < u }} {{ u >= t }}",
     {"t": datetime.datetime(2020, 1, 1), "u": datetime.datetime(2021, 6, 7, 8), "d": datetime.timedelta(days=1, hours=2)}),
    ("{{ [t, u]|max }} {{ [u, t]|min }} {{ [u, t]|sort }} {{ [d, d + d]|sum }}",
     {"t": datetime.datetime(2020, 1, 1), "u": datetime.datetime(2021, 6, 7, 8), "d": datetime.timedelta(days=1)}),
    ("{{ ks|sort }} {{ ks|sort(reverse=true) }} {{ ks|max }} {{ ks|min }} {{ ks[0] < ks[1] }} {{ ks[0] > ks[1] }}", {"ks": [Key(2), Key(1), Key(3)]}),
    ("{% for k, g in hs|groupby('key') %}{{ k }}:{{ g|map(attribute='key')|list|length }};{% endfor %}", {"hs": [Holder(Key(2)), Holder(Key(1)), Holder(Key(2))]}),
    ("{{ hs|sort(attribute='key')|map(attribute='key')|list }} {{ hs|max(attribute='key') }}", {"hs": [Holder(Key(2)), Holder(Key(1))]}),
    # a host number is the number to `%` formatting, as a tuple item too (task 0582)
    ("{{ '%d' % f }} {{ '%i' % d }} {{ '%f' % f }} {{ '%5.1f' % d }} {{ '%e' % d }} {{ '%g' % f }} {{ '%s' % f }}", {"f": FRAC(7, 2), "d": DEC("3.7")}),
    ("{{ '%d %d' % (f, 1) }} {{ '%d %f' % (d, d) }} {{ '%d' % x }} {{ '%x' % x }} {{ '%f' % x }} {{ '%o' % x }}", {"f": FRAC(7, 2), "d": DEC("3.7"), "x": Index()}),
    # abs and round ask the host: __abs__ and __round__ (task 0582)
    ("{{ f|abs }} {{ (-f)|abs }} {{ d|abs }} {{ (-d)|abs }} {{ f|round }} {{ f|round(1) }} {{ f|round(none) }}", {"f": FRAC(-7, 2), "d": DEC("-3.75")}),
    ("{{ d|round }} {{ d|round(1) }} {{ d|round(none) }} {{ d|round(precision=2, method='common') }} {{ (d|round) + 1 }}", {"d": DEC("3.75")}),
    # pprint orders host keys by their own `<` (task 0582)
    ("{{ {f: 1, g: 2, h: 3}|pprint }}", {"f": FRAC(7, 2), "g": FRAC(1, 3), "h": FRAC(-1)}),
    ("{{ {d: 1, e: 2, 1: 3}|pprint }}", {"d": DEC("3.7"), "e": DEC("-1")}),
    # ... and by type where `<` fails, as _safe_key does (task 0582)
    ("{{ {'a': 1, f: 2}|pprint }}", {"f": FRAC(7, 2)}),
    # bytes, bytearray, array and deque repeat as Python repeats them
    ("{{ b * 3 }} {{ 3 * b }} {{ ba * 2 }} {{ ar * 2 }} {{ dq * 2 }} {{ b * 0 }} {{ b + b }}",
     {"b": b"ab", "ba": bytearray(b"a"), "ar": array.array("i", [1]), "dq": collections.deque([1])}),
    # a StopIteration of an operator is Jinja2's RuntimeError, not the end of an iteration (PEP 479)
    ("{{ s + 1 }}", {"s": Stops()}),
    ("{% for x in [1] %}{{ s + x }}{% endfor %}", {"s": Stops()}),
]


@pytest.mark.parametrize("source,context", CASES)
def test_matches_jinja2(source, context):
    assert_same(source, context, {"extensions": ["jinja2.ext.do"]})


TYPE_ERRORS = [
    ("{{ a + t }}", {"a": [1], "t": (2,)}),
    ("{{ a - a }}", {"a": [1]}),
    ("{{ d < d }}", {"d": {}}),
    # `%` formatting names its class (0510)
    ("{{ '%s' % t }}", {"t": (1, 2)}, TypeError),
    # a host object's operators raise Python's own TypeError (task 0381)
    ("{{ d + 1.0 }}", {"d": DEC("1")}, TypeError),
    ("{{ d < 'a' }}", {"d": DEC("1")}, TypeError),
    ("{{ 'ab' * d }}", {"d": DEC("1")}, TypeError),
    ("{{ d - 'a' }}", {"d": DEC("1")}, TypeError),
    ("{{ k <= k }}", {"k": Key(1)}, TypeError),
    ("{{ k + 1 }}", {"k": Key(1)}, TypeError),
    # list and tuple concatenation names the left type, as Python's does (task 0582)
    ("{{ [1] + d }}", {"d": DEC("1")}, TypeError),
    ("{{ (1, 2) + d }}", {"d": DEC("1")}, TypeError),
    ("{{ l + k }}", {"l": [1], "k": Key(1)}, TypeError),
    ("{{ t + k }}", {"t": (1,), "k": Key(1)}, TypeError),
    # a host that is no number does not format as one; the name of a C type is its tp_name
    ("{{ '%d' % k }}", {"k": Key(1)}, TypeError),
    ("{{ '%f' % k }}", {"k": Key(1)}, TypeError),
    ("{{ -k }}", {"k": Key(1)}, TypeError),
]


@pytest.mark.parametrize("source,context,raised", [row if len(row) == 3 else (*row, jinja2cpp.TemplateRuntimeError) for row in TYPE_ERRORS])
def test_type_errors_of_lists_and_tuples(source, context, raised):
    # Jinja2's TypeError is TemplateRuntimeError here unless the engine names the class, with Python's message
    # (docs/python.md)
    with pytest.raises(TypeError) as expected:
        jinja2.Environment().from_string(source).render(**context)
    with pytest.raises(raised) as got:
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


@pytest.mark.parametrize("source", ["{{ b * 1000000000 }}", "{{ 1000000000 * b }}", "{{ ba * 10**9 }}", "{{ ar * 10**9 }}", "{{ dq * 10**9 }}", "{{ b * 10**30 }}"])
def test_a_host_sequence_repeated_past_the_cap_fails_before_python_allocates(source):
    # bytes, bytearray, array and deque repeat in Python: the length is checked against the cap first
    env = jinja2cpp.Environment()
    context = {"b": b"ab", "ba": bytearray(b"ab"), "ar": array.array("i", [1, 2]), "dq": collections.deque([1, 2])}
    with pytest.raises(jinja2cpp.TemplateRuntimeError, match="too long|longer than"):
        env.from_string(source).render(**context)
    assert env.from_string("{{ b * 3 }}").render(**context) == str(b"ab" * 3)


@pytest.mark.parametrize("name", ["b", "ba", "ar", "dq"])
def test_a_host_sequence_doubled_in_a_loop_stops_at_the_cap(name):
    # `+` grows bytes, bytearray, array and deque as it grows str and list: each step is checked against the cap
    env = jinja2cpp.Environment()
    context = {"b": b"ab", "ba": bytearray(b"ab"), "ar": array.array("i", [1, 2]), "dq": collections.deque([1, 2])}
    source = "{% set ns = namespace(v=v) %}{% for i in range(40) %}{% set ns.v = ns.v + ns.v %}{% endfor %}{{ ns.v|length }}"
    with pytest.raises(jinja2cpp.TemplateRuntimeError, match="too long|longer than"):
        env.from_string(source).render(v=context[name])
    assert env.from_string("{{ (v + v)|length }}").render(v=context[name]) == "4"


def test_an_array_times_a_list_is_no_repeat():
    np = pytest.importorskip("numpy")
    for source in ("{{ a * [1, 2, 3] }}", "{{ [1, 2, 3] * a }}", "{{ a * (1, 2, 3) }}"):
        assert_same(source, {"a": np.array([1, 2, 3])})


def test_a_failing_len_of_a_host_sequence_is_not_skipped_by_the_cap_check():
    class Broken(bytes):
        def __len__(self):
            raise KeyboardInterrupt

    with pytest.raises(KeyboardInterrupt):
        jinja2cpp.Environment().from_string("{{ (b * 10**9)|length }}").render(b=Broken(b"ab"))


@pytest.mark.parametrize("source", ["{{ (b * 10**9)|length }}", "{{ (b + b + b)|length }}", "{{ (dq * 10**9)|length }}"])
def test_a_subclass_that_hides_its_length_is_capped_by_its_storage(source):
    # Inherited repetition and concatenation read the built-in storage, not an overridden __len__ (a list subclass
    # is a sequence the engine reads and repeats itself)
    class Hidden(bytes):
        def __len__(self):
            return 0

    class HiddenDeque(collections.deque):
        def __len__(self):
            return 0

    # Three of these pass the 2**26 cap only together
    big = b"a" * (25 * 2**20)
    context = {"b": Hidden(big if "+" in source else b"ab"), "dq": HiddenDeque([1, 2])}
    with pytest.raises(jinja2cpp.TemplateRuntimeError, match="too long|longer than"):
        jinja2cpp.Environment().from_string(source).render(**context)


def test_a_deque_with_a_maxlen_repeats_as_python_bounds_it():
    # A bounded deque keeps maxlen items whatever the count: no cap error, Python's own result
    for source in ("{{ dq * 10**9 }}", "{{ 10**9 * dq }}", "{{ dq + dq }}", "{{ (dq * 10**30)|list }}"):
        assert_same(source, {"dq": collections.deque([1, 2], maxlen=3)})
    # The left deque's maxlen bounds a concatenation, whatever the right one holds
    big = collections.deque(itertools.repeat(None, 2**26 + 1))
    assert jinja2cpp.Environment().from_string("{{ (s + b)|length }}").render(s=collections.deque([1, 2, 3], maxlen=3), b=big) == "3"
    with pytest.raises(jinja2cpp.TemplateRuntimeError, match="too long|longer than"):
        jinja2cpp.Environment().from_string("{{ (b + s)|length }}").render(s=collections.deque([1, 2, 3], maxlen=3), b=big)
    with pytest.raises(jinja2cpp.TemplateRuntimeError, match="too long|longer than"):
        jinja2cpp.Environment().from_string("{{ (dq * 10**9)|length }}").render(dq=collections.deque([1, 2], maxlen=2**27))


@pytest.mark.parametrize("source", ["{{ (f % (10**9, b))|length }}", "{{ (f % (-10**9, b))|length }}", "{{ (p % (10**9, 1))|length }}",
                                    "{{ (w % b)|length }}", "{{ (pd % 1)|length }}", "{{ (f % (10**30, b))|length }}", "{{ (n % (2**26 + 1, 1))|length }}"])
def test_a_format_padded_past_the_cap_fails_before_python_allocates(source):
    # printf-style widths and numeric precisions of bytes and bytearray formats build their padding in Python: checked
    # against the cap first
    context = {"f": b"%*s", "p": bytearray(b"%.*d"), "w": b"%1000000000s", "pd": b"%.100000000d", "b": b"x", "n": b"%*.3d"}
    with pytest.raises(jinja2cpp.TemplateRuntimeError, match="too long|longer than"):
        jinja2cpp.Environment().from_string(source).render(**context)


@pytest.mark.parametrize("fmt", [b"%s%s", b"%b%b", b"%r%r"])
def test_a_format_that_doubles_its_value_stops_at_the_cap(fmt):
    # The bytes each conversion copies count too: a loop of `fmt % (v, v)` doubles v and stops at the cap
    source = "{% set ns = namespace(v=v) %}{% for i in range(40) %}{% set ns.v = f % (ns.v, ns.v) %}{% endfor %}{{ ns.v|length }}"
    with pytest.raises(jinja2cpp.TemplateRuntimeError, match="too long|longer than"):
        jinja2cpp.Environment().from_string(source).render(v=b"x", f=fmt)


def test_a_format_counts_the_values_its_mapping_keys_name():
    m = {b"a": b"x" * (40 * 2**20)}
    with pytest.raises(jinja2cpp.TemplateRuntimeError, match="too long|longer than"):
        jinja2cpp.Environment().from_string("{{ (f % m)|length }}").render(f=b"%(a)s%(a)s", m=m)
    assert_same("{{ f % m }}", {"f": b"%(a)s-%(a).2s", "m": {b"a": b"xyz"}})


def test_a_format_near_the_cap_is_counted_as_python_pads_it():
    # A width pads to the larger of itself and the copied value; a precision truncates %r too; a memoryview's repr is
    # short. None of these passes the cap in Python
    big = b"x" * (20 * 2**20)
    for fmt, value in ((b"%40000000s", b"x" * (30 * 2**20)), (b"%r", big), (b"%a", big), (b"%.5r", big), (b"%r", memoryview(big))):
        got = jinja2cpp.Environment().from_string("{{ (f % (v,))|length }}").render(f=fmt, v=value)
        assert got == str(len(fmt % (value,))), fmt


def test_an_interrupt_in_a_format_value_lookup_propagates():
    class Once(collections.abc.Mapping):
        calls = 0

        def __getitem__(self, key):
            Once.calls += 1
            if Once.calls == 1:
                raise KeyboardInterrupt
            return b"v"

        def __iter__(self):
            return iter([b"a"])

        def __len__(self):
            return 1

    with pytest.raises(KeyboardInterrupt):
        jinja2cpp.Environment().from_string("{{ f % m }}").render(f=b"%(a)s", m=Once())


def test_bytes_plus_a_wide_buffer_counts_its_bytes():
    # bytes + memoryview copies the view's bytes: an array of 8-byte items counts eight per item
    view = memoryview(array.array("Q", [0]) * 10_000_000)
    for source in ("{{ (b + v)|length }}", "{{ (ba + v)|length }}"):
        with pytest.raises(jinja2cpp.TemplateRuntimeError, match="too long|longer than"):
            jinja2cpp.Environment().from_string(source).render(b=b"x", ba=bytearray(b"x"), v=view)
    assert_same("{{ (b + v)|length }}", {"b": b"x", "v": memoryview(array.array("Q", [1, 2]))})


def test_a_format_within_the_cap_renders_as_python_formats_it():
    context = {"f": b"%*s|%-*d|%%|%.3s|%5.2f|%ld", "b": b"xyz", "fd": b"%(a)s-%(b)05d", "p": bytearray(b"%.*d")}
    for source in ("{{ f % (4, b, 3, 7, b, 2.5, 9) }}", "{{ fd % {'a': b, 'b': 42} }}", "{{ p % (5, 42) }}", "{{ b % () }}",
                   "{{ f % (1, 2) }}", "{{ p % (3,) }}", "{{ p % ('x', 1) }}", "{{ '%*s|%s' % t }}",
                   # A string conversion truncates to its precision, a number pads to the larger of width and
                   # precision, and a negative precision is ignored: none of these nears the cap
                   "{{ (ts % b)|length }}", "{{ (tr % b)|length }}", "{{ (np % (40000000, 40000002, 1))|length }}",
                   "{{ ng % (-10**9, 1.5) }}"):
        assert_same(source, {**context, "t": type("T", (tuple,), {})((3, "a", "b")), "ts": b"%.100000000s",
                             "tr": b"%.100000000r", "np": b"%-*.*d", "ng": b"%.*f"})


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


class _Fmt:
    """A host object with a __format__ of its own, as a Python object that has one."""

    def __format__(self, spec):
        return "F<" + spec + ">"

    def __str__(self):
        return "S"


class _FmtRaises:
    def __init__(self, error):
        self.error = error

    def __format__(self, spec):
        raise self.error


class _FmtInt:
    def __format__(self, spec):
        return 1


@pytest.mark.parametrize(
    "source",
    [
        "{{ '{0}'.format(o) }}",
        "{{ '{}|{:>4}'.format(o, o) }}",
        "{{ '{0:>4}'.format(o) }}",
        "{{ '{0:{1}}'.format(o, 3) }}",
        "{{ '{0!s:>3}'.format(o) }}",
        "{{ '{0!r}'.format(o)|length > 0 }}",
        "{{ '{x:>4}'.format_map({'x': o}) }}",
        "{{ '<{0}>'.format(o) }}",
    ],
)
def test_str_format_asks_a_host_object_for_its_format(source):
    assert_same(source, {"o": _Fmt()})


def test_markup_format_escapes_a_host_objects_format():
    class Amp:
        def __format__(self, spec):
            return "<" + spec + ">"

    assert_same("{{ '{0:a&b}'.format(o) }}|{{ ('<{0}>'|safe).format(o) }}|{{ ('{0:x}'|safe).format(o) }}", {"o": Amp()}, {"autoescape": True})


class _Html:
    """__html__ gives a str, which Markup.format escapes (markupsafe's EscapeFormatter)."""

    def __html__(self):
        return "<b>h</b>"

    def __format__(self, spec):
        return "F" + spec


class _HtmlMarkup:
    def __html__(self):
        return markupsafe.Markup("<b>m</b>")


class _HtmlFormat:
    def __html__(self):
        return "<i>"

    def __html_format__(self, spec):
        return "<" + spec + ">"


class _OnlyHtmlFormat:
    def __html_format__(self, spec):
        return markupsafe.Markup("<u" + spec + ">")


@pytest.mark.parametrize("cls", [_Html, _HtmlMarkup, _HtmlFormat, _OnlyHtmlFormat])
@pytest.mark.parametrize(
    "source",
    [
        "{{ ('{0}'|safe).format(o) }}",
        "{{ ('<{0}>'|safe).format(o) }}",
        "{{ ('{0:>5}'|safe).format(o) }}",
        "{{ ('{x:a&b}'|safe).format_map({'x': o}) }}",
        "{{ '{0}'.format(o)|length }}",
    ],
)
def test_markup_format_of_a_host_object_with_html(cls, source):
    results = []
    for module in (jinja2, jinja2cpp):
        try:
            results.append(module.Environment(autoescape=True).from_string(source).render(o=cls()))
        except (ValueError, module.TemplateRuntimeError) as error:
            results.append("error: " + str(error).split(" given, but")[0])
    assert results[0] == results[1], results


def test_markup_format_of_a_host_object_with_html_and_a_spec_is_a_value_error():
    for module in (jinja2, jinja2cpp):
        template = module.Environment(autoescape=True).from_string("{{ ('{0:>5}'|safe).format(o) }}")
        with pytest.raises(Exception, match="Format specifier >5 given, but .*_Html.* does not define __html_format__"):
            template.render(o=_Html())


def test_format_is_looked_up_on_the_type():
    # str.format resolves __format__ as a special method: an instance attribute of that name is not it
    class Shadowed:
        def __str__(self):
            return "S"

    o = Shadowed()
    o.__format__ = lambda spec: "shadow"
    assert_same("{{ '{0}'.format(o) }}|{{ '{0!s}'.format(o) }}", {"o": o})


@pytest.mark.parametrize("source", ["{{ '{0}'.format(o) }}", "{{ '{0:>4}'.format(o) }}", "{{ '{x}'.format_map({'x': o}) }}"])
def test_a_raising_format_is_undefined_for_stop_iteration_and_an_error_otherwise(source):
    assert_same("[" + source + "]", {"o": _FmtRaises(StopIteration())})
    for module in (jinja2cpp, jinja2):
        with pytest.raises(ValueError, match="boom"):
            module.Environment().from_string(source).render(o=_FmtRaises(ValueError("boom")))
        with pytest.raises((TypeError, module.TemplateRuntimeError), match="__format__ must return a str, not int"):
            module.Environment().from_string(source).render(o=_FmtInt())


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
    env.globals["stopfn"] = _stop
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
    # Python's builtin map ends where the attribute raises StopIteration: sum and join keep what came before
    "{% macro m() %}a{{ [{'stopping': 1}, page, {'stopping': 5}]|sum(attribute='stopping') }}b{% endmacro %}[{{ m() }}]",
    "{% macro m() %}a{{ [{'stopping': 1}, page]|join(attribute='stopping') }}b{% endmacro %}[{{ m() }}]",
    "{% macro m() %}a{{ [page]|sort(attribute='stopping') }}b{% endmacro %}[{{ m() }}]",
    "{% macro m() %}a{{ [page]|max(attribute='stopping') }}b{% endmacro %}[{{ m() }}]",
    "{% macro m() %}a{% for q in [1, 2] if stopfn() %}{{ q }}{% endfor %}b{% endmacro %}[{{ m() }}]",
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


@pytest.mark.parametrize(
    "source",
    [
        "{% macro m() %}{{ 'x' * 600 }}{{ 1|stop }}{% endmacro %}[{{ m() }}]",
        # Where the buffer boundary falls does not matter (0499)
        "{% macro m() %}ab{{ 1|stop }}{% endmacro %}{{ 'p' * 510 }}[{{ m() }}]",
    ],
)
def test_stop_iteration_out_of_a_long_macro_in_generate_and_stream(source):
    # The output a call writes into a generator's consumer is held back until the call returns (0499)
    for mode in ("generate", "stream"):
        expected = "".join(_stop_outcome(jinja2, source, mode=mode))
        assert "".join(_stop_outcome(jinja2cpp, source, mode=mode)) == expected, mode
    assert _stop_outcome(jinja2cpp, source) == _stop_outcome(jinja2, source)


# What Jinja2 does not give undefined: the call is not Context.call, or a generator ends the render. Jinja2's
# generators (map, select, reject and their attribute forms, unique, the filter of a loop) make a StopIteration out
# of the filter, test or attribute they apply a RuntimeError, even inside a macro
_STOP_ERRORS = [
    (_STOP_MACRO + "{{ m().x }}", "UndefinedError"),
    # The call block's call stops: Jinja2 yields the undefined value into the output, a TypeError
    ("{% macro m() %}{{ caller() }}{{ 1|stop }}{% endmacro %}[{% call m() %}x{% endcall %}]", "TypeError"),
    ("[{% call stopfn() %}x{% endcall %}]", "TypeError"),
    ("{{ f(1|stop) }}", "RuntimeError"),
    ("{% macro m() %}a{% include 'inc' %}b{% endmacro %}[{{ m() }}]", "RuntimeError"),
    ("{% macro m() %}a{% import 'inc' as b %}b{% endmacro %}[{{ m() }}]", "RuntimeError"),
    ("{% macro m() %}a{% block b %}{{ 1|stop }}{% endblock %}z{% endmacro %}[{{ m() }}]", "RuntimeError"),
    (
        "{% block b %}{% if flag %}{{ 1|stop }}{% endif %}{% endblock %}{% set flag = true %}"
        "{% macro m() %}x{{ self.b() }}y{% endmacro %}[{{ m() }}]",
        "RuntimeError",
    ),
    ("{% macro m() %}{{ [1]|map('stop')|list }}{% endmacro %}[{{ m() }}]", "RuntimeError"),
    ("{% macro m() %}{{ [1]|map('stop')|join }}{% endmacro %}[{{ m() }}]", "RuntimeError"),
    ("{% macro m() %}{{ [1]|map('stop')|first }}{% endmacro %}[{{ m() }}]", "RuntimeError"),
    ("{% macro m() %}{{ [1]|select('stop')|list }}{% endmacro %}[{{ m() }}]", "RuntimeError"),
    ("{% macro m() %}{{ [1]|reject('stop')|list }}{% endmacro %}[{{ m() }}]", "RuntimeError"),
    ("{% macro m() %}{{ [page]|map(attribute='stopping')|list }}{% endmacro %}[{{ m() }}]", "RuntimeError"),
    ("{% macro m() %}{{ [page]|selectattr('stopping')|list }}{% endmacro %}[{{ m() }}]", "RuntimeError"),
    ("{% macro m() %}{{ [page]|unique(attribute='stopping')|list }}{% endmacro %}[{{ m() }}]", "RuntimeError"),
    ("{% macro m() %}{% for q in [1]|map('stop') %}{{ q }}{% endfor %}{% endmacro %}[{{ m() }}]", "RuntimeError"),
    ("{% macro m() %}{% for q in [1, 2] if q is stop %}{{ q }}{% endfor %}{% endmacro %}[{{ m() }}]", "RuntimeError"),
    ("{% macro m() %}{% for q in [1, 2] if q|stop %}{{ q }}{% endfor %}{% endmacro %}[{{ m() }}]", "RuntimeError"),
    ("{% macro m() %}{% for q in [page] if q.stopping %}{{ q }}{% endfor %}{% endmacro %}[{{ m() }}]", "RuntimeError"),
]


@pytest.mark.parametrize("source,error", _STOP_ERRORS)
def test_stop_iteration_where_jinja2_does_not_stop_the_call(source, error):
    for module in (jinja2cpp, jinja2):
        outcome = _stop_outcome(module, source)
        # Jinja2's TypeError is this module's TemplateRuntimeError (docs/python.md)
        expected = "TemplateRuntimeError" if error == "TypeError" and module is jinja2cpp else error
        assert type(outcome).__name__ == expected, (module.__name__, source, outcome)
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


# A dict view or set of the engine reaches a Python callable as that Python type, a snapshot of its items (0545)
@pytest.mark.parametrize(
    ("source", "expected"),
    [
        ("{{ f({'a': 1, 'b': 2}.keys()) }}", "dict_keys|dict_keys(['a', 'b'])|['a', 'b']"),
        ("{{ f({'a': 1, 'b': 2}.values()) }}", "dict_values|dict_values([1, 2])|[1, 2]"),
        ("{{ f({'a': 1, 'b': 2}.items()) }}", "dict_items|dict_items([('a', 1), ('b', 2)])|[('a', 1), ('b', 2)]"),
        ("{{ f({'a': 1, 'b': 2}.keys() - ['a']) }}", "set|{'b'}|['b']"),
        ("{{ f({'a': 1}.keys() - []) }}", "set|{'a'}|['a']"),
    ],
)
def test_an_engine_view_or_set_reaches_python_as_its_type(source, expected):
    def f(view):
        return f"{type(view).__name__}|{view!r}|{sorted(view, key=repr)!r}"

    env = jinja2cpp.Environment()
    env.globals["f"] = f
    assert env.from_string(source).render() == expected


def _outcome_in(env, source, context=None):
    try:
        return ("ok", env.from_string(source).render(**(context or {})))
    except Exception as error:  # noqa: BLE001
        return ("error",)


@pytest.mark.parametrize(
    "source",
    [
        "{{ keep({'a': 1}.keys()) | pprint }}",
        "{{ keep({'a': 1}.items()) | pprint }}",
        "{{ keep({'a': 1}.values()) | pprint }}",
        "{{ (keep({'a': 1}.keys()) - ['x']) | pprint }}",
        "{{ keep({'a': 1}.keys()) | tojson }}",
        "{{ keep({'a': 1}.keys()) - ['a'] }}",
        "{{ ['a', 'b'] - keep({'a': 1}.keys()) }}",
    ],
)
def test_an_engine_view_given_back_by_python_is_a_python_view(source):
    ours = jinja2cpp.Environment()
    theirs = jinja2.Environment()
    for env in (ours, theirs):
        env.globals["keep"] = lambda view: view
    assert _outcome_in(ours, source) == _outcome_in(theirs, source), source


@pytest.mark.parametrize(
    "source",
    [
        "{{ d.keys() - ['a'] }}",
        "{{ ['a', 'b', 'a'] - d.keys() }}",
        "{{ d.items() - [('a', 1)] }}",
        "{{ d.keys() - d.keys() }}",
        "{{ d.keys() - {'a': 0} }}",
        "{{ {'a': 0} - d.keys() }}",
        "{{ d.keys() - [1, 2] - ['a'] }}",
        "{% for x in d.keys() - ['b'] %}{{ x }}{% endfor %}",
        "{{ d.keys().isdisjoint(['z']) }}|{{ d.keys().isdisjoint(['a']) }}",
        "{{ d.keys() < d.keys() }}|{{ d.keys() <= d.keys() }}",
        "{{ d.keys() + [] }}",
        "{{ d.keys() - 5 }}",
        "{{ d.values() - [] }}",
        "{{ d.keys().mapping | pprint }}|{{ d.keys().mapping }}|{{ d.keys().mapping | length }}",
        "{{ d.keys().mapping | tojson }}",
        "{{ d | random }}",
        "{{ d.keys().mapping | random }}",
    ],
)
def test_a_host_dict_view_has_the_set_operators_of_the_engine_view(source):
    context = {"d": {"a": 1, "b": 2}}
    assert _outcome_in(jinja2cpp.Environment(), source, context) == _outcome_in(jinja2.Environment(), source, context), source


def test_random_of_a_host_dict_indexes_it_like_python():
    env = jinja2cpp.Environment()
    with pytest.raises(KeyError):
        env.from_string("{{ d | random }}").render(d={"a": 1})
    assert env.from_string("{{ d | random }}").render(d={0: "z"}) == "z"
