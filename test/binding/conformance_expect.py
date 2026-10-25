#!/usr/bin/env python3
"""Prints the Python Jinja2 expectations of the rows in test/binding/conformance.h.

For reviewers: nothing is generated into the tree. The C++ tables in conformance.h were pasted from
`conformance_expect.py --cpp`; run it again and diff to see that they still match the pinned oracle
(Python Jinja2 at the version of test/parity/requirements.txt).

    python3 test/binding/conformance_expect.py          # one block per row
    python3 test/binding/conformance_expect.py --cpp    # the row tables, as C++

Bindings whose objects iterate in key order (nlohmann::json) are compared with `object_pairs_hook` sorting
every object, so each row has an `expected` and, when it differs, a `sorted` string.
"""
import json
import sys

import jinja2

LIFETIME_DOC = '{"list":[3,1,2],"obj":{"a":1,"b":[1,2]}}'
NESTED = '{"a":[{"b":{"c":"deep"}},2],"x":{"p":1,"q":[1,2]}}'
ORDER = '{"b":1,"a":2,"c":{"z":1,"y":2,"x":{"n":0,"m":1}},"d":[{"q":1,"p":2}]}'
SCALARS = '{"t":true,"f":false,"i":42,"n":-7,"l":100500100500100,"d":100.5,"s":"str","e":""}'
BIGS = '{"max":9223372036854775807,"min":-9223372036854775808,"b1":9223372036854775808,"b2":18446744073709551615}'
MISSING = '{"l":[10,20,30],"o":{"k":1}}'
EMPTY = '{"e":{},"a":[],"s":""}'
NULLS = '{"x":null,"l":[1,null,3],"o":{"n":null}}'
ITER = '{"l":[3,1,2],"w":["b","a","c"],"o":[{"n":"x","v":2},{"n":"y","v":1},{"n":"z","v":3}],"f":[1.5,0.5],"e":[]}'
EQUAL = '{"a":{"x":[1,2],"y":{"z":null}},"b":{"x":[1,2],"y":{"z":null}},"c":{"x":[1,3]},"l":[1,2]}'

# group -> [(row name, JSON document, template)]
ROWS = {
    "Scalars": [
        ("all", SCALARS, "{{ j.t }}|{{ j.f }}|{{ j.i }}|{{ j.n }}|{{ j.l }}|{{ j.d }}|{{ j.s }}|[{{ j.e }}]"),
        ("arithmetic", SCALARS,
         "{{ j.i + 1 }}|{{ j.n * 2 }}|{{ j.d + 0.5 }}|{{ j.l - 100500100500000 }}|{{ j.t and 'y' }}|{{ j.f or 'n' }}|{{ j.s ~ j.i }}"),
        ("tests", SCALARS,
         "{{ j.i is integer }}|{{ j.d is float }}|{{ j.s is string }}|{{ j.t is boolean }}|{{ j.i is number }}|{{ j.d is integer }}|{{ j.i is float }}"),
        ("non_ascii", '{"s":"H\\u00e9","k":"\\u65e5\\u672c"}',
         "{{ j.s }}|{{ j.s|length }}|{{ j.k }}|{{ j.k|length }}|{{ j.s|upper }}"),
        ("null", '{"n":null}', "{{ j.n is none }}|{{ j.n is defined }}|{{ j.n }}|{{ j.m is defined }}"),
        ("root_int", "42", "{{ j + 1 }}|{{ j is number }}"),
        ("root_string", '"abc"', "{{ j|upper }}|{{ j|length }}"),
        ("root_bool", "true", "{{ j and 'y' }}|{{ j is boolean }}"),
        ("root_double", "2.5", "{{ j * 2 }}"),
        ("root_null", "null", "{{ j is none }}|{{ j is defined }}"),
    ],
    "BigIntegers": [
        ("edges", BIGS, "{{ j.max }}|{{ j.min }}|{{ j.b1 }}|{{ j.b2 }}"),
        ("compare", BIGS,
         "{{ j.b1 > j.max }}|{{ j.b2 > j.b1 }}|{{ j.min < 0 }}|{{ j.b2 == 18446744073709551615 }}|{{ j.b1 is number }}|{{ j.b1 is integer }}"),
        ("in_list", '{"l":[9223372036854775808,1,18446744073709551615]}', "{{ j.l }}|{{ j.l|first }}|{{ j|tojson }}"),
        ("beyond_uint64", '{"h":123456789012345678901234567890,"n":-123456789012345678901234567890,"u":18446744073709551616}',
         "{{ j.h }}|{{ j.n }}|{{ j.u }}|{{ j.h > j.u }}|{{ j|tojson }}"),
    ],
    "Nesting": [
        ("path", NESTED, "{{ j.a[0].b.c }}|{{ j['a'][0]['b']['c'] }}|{{ j.a|length }}|{{ j.x.q[1] }}|{{ j['x']['p'] }}"),
        ("repr", NESTED, "{{ j }}"),
        ("tojson", NESTED, "{{ j|tojson }}"),
        ("get", NESTED, "{{ j.get('x') }}|{{ j.get('zz') }}|{{ j['a'] }}"),
        ("items", NESTED, "{{ j.items()|list }}"),
        ("dictsort", '{"b":1,"a":[2],"c":{"z":1}}', "{{ j|dictsort }}"),
        ("pprint", NESTED, "{{ j|pprint }}"),
    ],
    "KeyOrder": [
        ("list", ORDER, "{{ j|list }}"),
        ("items", ORDER, "{{ j.items()|list }}"),
        ("repr", ORDER, "{{ j }}"),
        ("nested", ORDER, "{{ j.c }}|{{ j.d[0] }}|{{ j.c.x|list }}"),
        ("loop", ORDER, "{% for k, v in j.c.items() %}{{ k }}={{ v }};{% endfor %}"),
        ("keys", ORDER, "{{ j.keys()|list }}|{{ j.values()|list|length }}|{{ j|join(',') }}|{{ j|first }}|{{ j|last }}"),
    ],
    "Missing": [
        ("keys", MISSING,
         "{{ j.x is defined }}|{{ j.get('x', 1) }}|{{ j.o.zz is defined }}|{{ j.o.k }}|{{ 'k' in j.o }}|{{ 'zz' in j.o }}"),
        ("index", MISSING,
         "{{ j.l[9] is defined }}|{{ j.l[-1] }}|{{ j.l[-3] }}|{{ j.l[-4] is defined }}|{{ j.l[3] is defined }}|{{ j.l[0] }}"),
        ("default", MISSING, "{{ j.x|default('d') }}|{{ j.l[9]|default('d') }}|{{ j.o.zz|default('d') }}"),
    ],
    "Empty": [
        ("length", EMPTY,
         "{{ j.e|length }}|{{ j.a|length }}|{{ j.e is mapping }}|{{ j.a is mapping }}|{{ j.a is sequence }}|{{ j.e is iterable }}"),
        ("for_else", EMPTY,
         "{% for x in j.a %}x{% else %}empty-list{% endfor %}|{% for k in j.e %}x{% else %}empty-map{% endfor %}"),
        ("truthy", EMPTY,
         "{{ j.e or 'falsy' }}|{{ j.a or 'falsy' }}|{{ j.s or 'falsy' }}|{{ 'T' if j.e else 'F' }}|{{ j.a|list }}|{{ j.e|list }}|{{ j }}"),
        ("root_object", "{}", "{{ j|length }}|{{ j }}|{{ j or 'falsy' }}|{{ j is mapping }}"),
        ("root_array", "[]", "{{ j|length }}|{{ j }}|{{ j or 'falsy' }}|{{ j is mapping }}"),
    ],
    "Nulls": [
        ("member", NULLS,
         "{{ j.x is defined }}|{{ j.x is none }}|[{{ j.x }}]|{{ j.x|string }}|{{ j.y is defined }}|{{ j.o.n is none }}"),
        ("item", NULLS,
         "{{ j.l }}|{{ j.l[1] is none }}|{{ j.l[1] is defined }}|{% for v in j.l %}{{ v is none }},{% endfor %}|{{ j.l|length }}"),
        ("default", NULLS, "{{ j.x|default('d') }}|{{ j.y|default('d') }}|{{ j.x|default('d', true) }}"),
        ("repr", NULLS, "{{ j }}|{{ j|tojson }}"),
        ("attribute", NULLS, "[{{ j.x.attr }}]|[{{ j.l[1].attr }}]|{{ j.x.attr is defined }}|{{ j.o.n.k|default('d') }}"),
    ],
    "Strings": [
        ("escapes", '{"e":"a\\"b\\\\c\\/d\\n\\t","c":"\\u0001"}', "{{ j.e|length }}|{{ j.e|tojson }}|{{ j.c|length }}"),
        ("unicode", '{"u":"\\u00e9","bmp":"\\u2603"}', "{{ j.u|length }}|{{ j.bmp|length }}|{{ j.u }}{{ j.bmp }}"),
        ("surrogate_pair", '{"sp":"\\ud83d\\ude00"}', "{{ j.sp|length }}|{{ j.sp }}"),
        ("nul", '{"nul":"a\\u0000b"}', "{{ j.nul|length }}|{{ j.nul|tojson }}|{{ j.nul|list|length }}"),
        ("unicode_key", '{"k\\u00e9y":1}', "{{ j|list }}|{{ j|length }}|{% for k in j %}{{ k|length }}{% endfor %}"),
    ],
    "Iteration": [
        ("sort", ITER,
         "{{ j.l|sort }}|{{ j.l|sort(reverse=true) }}|{{ j.w|sort }}|{{ j.o|sort(attribute='v')|map(attribute='n')|join }}"),
        ("map_join", ITER,
         "{{ j.o|map(attribute='n')|join(',') }}|{{ j.w|join('-') }}|{{ j.l|map('string')|join('+') }}"),
        ("first_last", ITER,
         "{{ j.l|first }}|{{ j.l|last }}|{{ j.e|first }}|{{ j.l|length }}|{{ j.l|sum }}|{{ j.f|sum }}|{{ j.l|max }}|{{ j.l|min }}"),
        ("slice", ITER,
         "{{ j.l[1:] }}|{{ j.l[:2] }}|{{ j.l[::-1] }}|{{ j.l[-2:] }}|{{ j.l|reverse|list }}|{{ j.l|batch(2)|list }}|{{ j.l|slice(2)|list }}"),
        ("loop", ITER,
         "{% for x in j.l %}{{ loop.index }}:{{ x }}{% if not loop.last %},{% endif %}{% endfor %}|{% for o in j.o %}{{ loop.index0 }}{{ o.n }}{{ loop.revindex }};{% endfor %}"),
        ("select", ITER,
         "{{ j.o|selectattr('v','gt',1)|map(attribute='n')|list }}|{{ j.o|rejectattr('v','eq',2)|map(attribute='n')|join }}|{{ j.l|select('odd')|list }}|{{ j.o|map(attribute='v')|sum }}"),
        ("membership", ITER,
         "{{ 2 in j.l }}|{{ 'b' in j.w }}|{{ 'n' in j.o[0] }}|{{ 5 in j.l }}|{% for k in j.o[0] %}{{ k }}{% endfor %}"),
        ("nested_loop", '{"o":[{"n":"xy","v":2},{"n":"z","v":1}]}',
         "{% for o in j.o %}{% for c in o.n %}{{ c }}{% endfor %};{% endfor %}|{% for k, v in j.o[0].items() %}{{ k }}{{ v }}{% endfor %}"),
    ],
    "Equality": [
        ("structural", EQUAL,
         "{{ j.a == j.b }}|{{ j.a == j.c }}|{{ j.a != j.c }}|{{ j.a == j.a }}|{{ j.a.x == j.b.x }}|{{ j.a.y == j.b.y }}"),
        ("list", EQUAL, "{{ j.l == [1, 2] }}|{{ j.l == [2, 1] }}|{{ j.l == j.a.x }}|{{ j.a.x in [j.b.x] }}|{{ j.l != [1] }}"),
        ("literal", EQUAL, "{{ j.c == {'x': [1, 3]} }}|{{ j.a.y == {'z': none} }}"),
    ],
    "Lifetime": [
        ("sort", LIFETIME_DOC, "{% set r = rjson().list | sort %}{{ r }}"),
        ("items_tojson", LIFETIME_DOC, "{{ rjson().obj.items()|list|tojson }}"),
        ("slice", LIFETIME_DOC, "{{ rjson().list[1:] }}"),
        ("namespace_list", LIFETIME_DOC,
         "{% set ns = namespace(items=[]) %}{% for x in rjson().list %}{% set ns.items = ns.items + [x] %}{% endfor %}{{ ns.items }}"),
        ("namespace_object", LIFETIME_DOC,
         "{% set ns = namespace(o=none) %}{% for i in [1] %}{% set ns.o = rjson().obj %}{% endfor %}{{ ns.o }}|{{ ns.o.b[1] }}|{{ ns.o.b|sum }}"),
        ("set_chain", LIFETIME_DOC, "{% set o = rjson().obj %}{% set b = o.b %}{{ b }}|{{ o.a }}"),
        ("macro", LIFETIME_DOC, "{% macro m(x) %}{{ x.b|join('+') }}{% endmacro %}{{ m(rjson().obj) }}"),
        ("filters", LIFETIME_DOC, "{{ rjson().obj.b|map('string')|join(',') }}|{{ (rjson().list|sort)[0] }}"),
        ("list_of_roots", LIFETIME_DOC, "{% set l = [rjson().obj, rjson().obj] %}{{ l[0].b }}|{{ l|length }}|{{ l[1].a }}"),
    ],
    "RepeatedKeys": [
        ("full", '{"a":1,"b":2,"a":3}', "{{ j }}|{{ j.a }}|{{ j|length }}|{{ j|list }}"),
        ("nested", '{"o":{"k":1,"k":2,"m":0}}', "{{ j }}"),
        ("other_key", '{"a":1,"b":2,"a":3}', "{{ j.b }}"),
    ],
}


def load(doc, sort_keys):
    if sort_keys:
        return json.loads(doc, object_pairs_hook=lambda pairs: dict(sorted(pairs)))
    return json.loads(doc)


def render(doc, tpl, sort_keys):
    env = jinja2.Environment()
    try:
        return env.from_string(tpl).render(j=load(doc, sort_keys), rjson=lambda: load(doc, sort_keys))
    except Exception as ex:  # a row the oracle rejects is a bug in this script
        return "ORACLE ERROR: %s: %s" % (type(ex).__name__, ex)


def ascii_only(text):
    return all(ord(ch) < 128 for ch in text)


def escape(text, wide):
    out = []
    for ch in text:
        code = ord(ch)
        if ch in '"\\':
            out.append("\\" + ch)
        elif ch == "?":
            out.append("\\?")  # no trigraphs
        elif 32 <= code < 127:
            out.append(ch)
        elif wide:
            out.append("\\u%04x" % code if code < 0x10000 else "\\U%08x" % code)
        else:
            out.extend("\\%03o" % byte for byte in ch.encode("utf-8"))
    return ("L" if wide else "") + '"' + "".join(out) + '"'


def raw(text):
    assert ')J2"' not in text
    return 'R"J2(' + text + ')J2"'


def cpp():
    print("// Pasted from `python3 test/binding/conformance_expect.py --cpp` (Python Jinja2 is the oracle)")
    for group, rows in ROWS.items():
        print("\ninline const std::vector<Row>& %sRows()\n{\n    static const std::vector<Row> rows = {" % group)
        for name, doc, tpl in rows:
            plain, sorted_ = render(doc, tpl, False), render(doc, tpl, True)
            assert not plain.startswith("ORACLE ERROR"), (group, name, plain)
            assert ascii_only(tpl) and ascii_only(doc)
            wide = "nullptr" if ascii_only(plain) else escape(plain, True)
            narrow = raw(plain) if ascii_only(plain) else escape(plain, False)
            if plain == sorted_:
                other = "nullptr"
            else:
                assert ascii_only(sorted_)
                other = raw(sorted_)
            print('        { "%s", %s,\n          %s,\n          %s, %s, %s },' % (name, raw(doc), raw(tpl), narrow, other, wide))
        print("    };\n    return rows;\n}")


def listing():
    print("jinja2 %s" % jinja2.__version__)
    for group, rows in ROWS.items():
        for name, doc, tpl in rows:
            print("[%s.%s]\n  doc:      %s\n  template: %s" % (group, name, doc, tpl))
            plain, sorted_ = render(doc, tpl, False), render(doc, tpl, True)
            print("  expected: %s" % plain)
            if plain != sorted_:
                print("  sorted:   %s" % sorted_)


if __name__ == "__main__":
    if "--cpp" in sys.argv[1:]:
        cpp()
    else:
        listing()
