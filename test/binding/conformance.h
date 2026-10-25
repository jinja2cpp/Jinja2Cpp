#ifndef JINJA2CPP_TEST_BINDING_CONFORMANCE_H
#define JINJA2CPP_TEST_BINDING_CONFORMANCE_H

// A typed suite that every JSON binding of Jinja2C++ runs (docs/tasks/0311). A binding supplies a struct:
//
//   struct NlohmannOrdered
//   {
//       using Json = nlohmann::ordered_json;
//       static Json Parse(std::string_view text);         // the library's own parser
//       static constexpr bool kSortedKeys = false;        // objects iterate in key order (nlohmann::json)
//       static constexpr bool kViewable = true;           // false for copy-only traits: the view modes are skipped
//       static constexpr bool kBigIntText = false;        // integers beyond uint64 are exact (JsonKind::BigInt)
//       static constexpr bool kViewDuplicateKeys = false; // the view of {"a":1,"a":2} lists the key twice
//   };
//   INSTANTIATE_TYPED_TEST_SUITE_P(Nlohmann, JsonConformance, ::testing::Types<NlohmannOrdered>);
//
// Every row renders in four modes (a borrowed view, an owned view, a shared view and the copy of jinja2::ToValue),
// with a narrow and a wide template. Expected strings are what Python Jinja2 renders for json.loads of the document
// (test/binding/conformance_expect.py prints them; for a binding with kSortedKeys, every object is sorted first).
//
// The header needs only GoogleTest and the public headers, and is meant to be included by one translation unit.

#include <jinja2cpp/error_info.h>
#include <jinja2cpp/generic_list.h>
#include <jinja2cpp/json_adapter.h>
#include <jinja2cpp/reflected_value.h>
#include <jinja2cpp/template.h>
#include <jinja2cpp/user_callable.h>
#include <jinja2cpp/value.h>

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

namespace jinja2_conformance
{
//! A document, a template, and the rendering of Python Jinja2 (`expectedSorted` when it differs for sorted keys)
struct Row
{
    const char* name;
    const char* doc;
    const char* tpl;
    const char* expected;
    const char* expectedSorted;  //!< null: same as `expected`
    const wchar_t* expectedWide; //!< null: `expected` widened; given for the rows that are not ASCII
};

// Pasted from `python3 test/binding/conformance_expect.py --cpp` (Python Jinja2 is the oracle)

inline const std::vector<Row>& ScalarsRows()
{
    static const std::vector<Row> rows = {
        { "all", R"J2({"t":true,"f":false,"i":42,"n":-7,"l":100500100500100,"d":100.5,"s":"str","e":""})J2",
          R"J2({{ j.t }}|{{ j.f }}|{{ j.i }}|{{ j.n }}|{{ j.l }}|{{ j.d }}|{{ j.s }}|[{{ j.e }}])J2",
          R"J2(True|False|42|-7|100500100500100|100.5|str|[])J2", nullptr, nullptr },
        { "arithmetic", R"J2({"t":true,"f":false,"i":42,"n":-7,"l":100500100500100,"d":100.5,"s":"str","e":""})J2",
          R"J2({{ j.i + 1 }}|{{ j.n * 2 }}|{{ j.d + 0.5 }}|{{ j.l - 100500100500000 }}|{{ j.t and 'y' }}|{{ j.f or 'n' }}|{{ j.s ~ j.i }})J2",
          R"J2(43|-14|101.0|100|y|n|str42)J2", nullptr, nullptr },
        { "tests", R"J2({"t":true,"f":false,"i":42,"n":-7,"l":100500100500100,"d":100.5,"s":"str","e":""})J2",
          R"J2({{ j.i is integer }}|{{ j.d is float }}|{{ j.s is string }}|{{ j.t is boolean }}|{{ j.i is number }}|{{ j.d is integer }}|{{ j.i is float }})J2",
          R"J2(True|True|True|True|True|False|False)J2", nullptr, nullptr },
        { "non_ascii", R"J2({"s":"H\u00e9","k":"\u65e5\u672c"})J2",
          R"J2({{ j.s }}|{{ j.s|length }}|{{ j.k }}|{{ j.k|length }}|{{ j.s|upper }})J2",
          "H\303\251|2|\346\227\245\346\234\254|2|H\303\211", nullptr, L"H\u00e9|2|\u65e5\u672c|2|H\u00c9" },
        { "null", R"J2({"n":null})J2",
          R"J2({{ j.n is none }}|{{ j.n is defined }}|{{ j.n }}|{{ j.m is defined }})J2",
          R"J2(True|True|None|False)J2", nullptr, nullptr },
        { "root_int", R"J2(42)J2",
          R"J2({{ j + 1 }}|{{ j is number }})J2",
          R"J2(43|True)J2", nullptr, nullptr },
        { "root_string", R"J2("abc")J2",
          R"J2({{ j|upper }}|{{ j|length }})J2",
          R"J2(ABC|3)J2", nullptr, nullptr },
        { "root_bool", R"J2(true)J2",
          R"J2({{ j and 'y' }}|{{ j is boolean }})J2",
          R"J2(y|True)J2", nullptr, nullptr },
        { "root_double", R"J2(2.5)J2",
          R"J2({{ j * 2 }})J2",
          R"J2(5.0)J2", nullptr, nullptr },
        { "root_null", R"J2(null)J2",
          R"J2({{ j is none }}|{{ j is defined }})J2",
          R"J2(True|True)J2", nullptr, nullptr },
    };
    return rows;
}

inline const std::vector<Row>& BigIntegersRows()
{
    static const std::vector<Row> rows = {
        { "edges", R"J2({"max":9223372036854775807,"min":-9223372036854775808,"b1":9223372036854775808,"b2":18446744073709551615})J2",
          R"J2({{ j.max }}|{{ j.min }}|{{ j.b1 }}|{{ j.b2 }})J2",
          R"J2(9223372036854775807|-9223372036854775808|9223372036854775808|18446744073709551615)J2", nullptr, nullptr },
        { "compare", R"J2({"max":9223372036854775807,"min":-9223372036854775808,"b1":9223372036854775808,"b2":18446744073709551615})J2",
          R"J2({{ j.b1 > j.max }}|{{ j.b2 > j.b1 }}|{{ j.min < 0 }}|{{ j.b2 == 18446744073709551615 }}|{{ j.b1 is number }}|{{ j.b1 is integer }})J2",
          R"J2(True|True|True|True|True|True)J2", nullptr, nullptr },
        { "in_list", R"J2({"l":[9223372036854775808,1,18446744073709551615]})J2",
          R"J2({{ j.l }}|{{ j.l|first }}|{{ j|tojson }})J2",
          R"J2([9223372036854775808, 1, 18446744073709551615]|9223372036854775808|{"l": [9223372036854775808, 1, 18446744073709551615]})J2", nullptr, nullptr },
        { "beyond_uint64", R"J2({"h":123456789012345678901234567890,"n":-123456789012345678901234567890,"u":18446744073709551616})J2",
          R"J2({{ j.h }}|{{ j.n }}|{{ j.u }}|{{ j.h > j.u }}|{{ j|tojson }})J2",
          R"J2(123456789012345678901234567890|-123456789012345678901234567890|18446744073709551616|True|{"h": 123456789012345678901234567890, "n": -123456789012345678901234567890, "u": 18446744073709551616})J2", nullptr, nullptr },
    };
    return rows;
}

inline const std::vector<Row>& NestingRows()
{
    static const std::vector<Row> rows = {
        { "path", R"J2({"a":[{"b":{"c":"deep"}},2],"x":{"p":1,"q":[1,2]}})J2",
          R"J2({{ j.a[0].b.c }}|{{ j['a'][0]['b']['c'] }}|{{ j.a|length }}|{{ j.x.q[1] }}|{{ j['x']['p'] }})J2",
          R"J2(deep|deep|2|2|1)J2", nullptr, nullptr },
        { "repr", R"J2({"a":[{"b":{"c":"deep"}},2],"x":{"p":1,"q":[1,2]}})J2",
          R"J2({{ j }})J2",
          R"J2({'a': [{'b': {'c': 'deep'}}, 2], 'x': {'p': 1, 'q': [1, 2]}})J2", nullptr, nullptr },
        { "tojson", R"J2({"a":[{"b":{"c":"deep"}},2],"x":{"p":1,"q":[1,2]}})J2",
          R"J2({{ j|tojson }})J2",
          R"J2({"a": [{"b": {"c": "deep"}}, 2], "x": {"p": 1, "q": [1, 2]}})J2", nullptr, nullptr },
        { "get", R"J2({"a":[{"b":{"c":"deep"}},2],"x":{"p":1,"q":[1,2]}})J2",
          R"J2({{ j.get('x') }}|{{ j.get('zz') }}|{{ j['a'] }})J2",
          R"J2({'p': 1, 'q': [1, 2]}|None|[{'b': {'c': 'deep'}}, 2])J2", nullptr, nullptr },
        { "items", R"J2({"a":[{"b":{"c":"deep"}},2],"x":{"p":1,"q":[1,2]}})J2",
          R"J2({{ j.items()|list }})J2",
          R"J2([('a', [{'b': {'c': 'deep'}}, 2]), ('x', {'p': 1, 'q': [1, 2]})])J2", nullptr, nullptr },
        { "dictsort", R"J2({"b":1,"a":[2],"c":{"z":1}})J2",
          R"J2({{ j|dictsort }})J2",
          R"J2([('a', [2]), ('b', 1), ('c', {'z': 1})])J2", nullptr, nullptr },
        { "pprint", R"J2({"a":[{"b":{"c":"deep"}},2],"x":{"p":1,"q":[1,2]}})J2",
          R"J2({{ j|pprint }})J2",
          R"J2({'a': [{'b': {'c': 'deep'}}, 2], 'x': {'p': 1, 'q': [1, 2]}})J2", nullptr, nullptr },
    };
    return rows;
}

inline const std::vector<Row>& KeyOrderRows()
{
    static const std::vector<Row> rows = {
        { "list", R"J2({"b":1,"a":2,"c":{"z":1,"y":2,"x":{"n":0,"m":1}},"d":[{"q":1,"p":2}]})J2",
          R"J2({{ j|list }})J2",
          R"J2(['b', 'a', 'c', 'd'])J2", R"J2(['a', 'b', 'c', 'd'])J2", nullptr },
        { "items", R"J2({"b":1,"a":2,"c":{"z":1,"y":2,"x":{"n":0,"m":1}},"d":[{"q":1,"p":2}]})J2",
          R"J2({{ j.items()|list }})J2",
          R"J2([('b', 1), ('a', 2), ('c', {'z': 1, 'y': 2, 'x': {'n': 0, 'm': 1}}), ('d', [{'q': 1, 'p': 2}])])J2", R"J2([('a', 2), ('b', 1), ('c', {'x': {'m': 1, 'n': 0}, 'y': 2, 'z': 1}), ('d', [{'p': 2, 'q': 1}])])J2", nullptr },
        { "repr", R"J2({"b":1,"a":2,"c":{"z":1,"y":2,"x":{"n":0,"m":1}},"d":[{"q":1,"p":2}]})J2",
          R"J2({{ j }})J2",
          R"J2({'b': 1, 'a': 2, 'c': {'z': 1, 'y': 2, 'x': {'n': 0, 'm': 1}}, 'd': [{'q': 1, 'p': 2}]})J2", R"J2({'a': 2, 'b': 1, 'c': {'x': {'m': 1, 'n': 0}, 'y': 2, 'z': 1}, 'd': [{'p': 2, 'q': 1}]})J2", nullptr },
        { "nested", R"J2({"b":1,"a":2,"c":{"z":1,"y":2,"x":{"n":0,"m":1}},"d":[{"q":1,"p":2}]})J2",
          R"J2({{ j.c }}|{{ j.d[0] }}|{{ j.c.x|list }})J2",
          R"J2({'z': 1, 'y': 2, 'x': {'n': 0, 'm': 1}}|{'q': 1, 'p': 2}|['n', 'm'])J2", R"J2({'x': {'m': 1, 'n': 0}, 'y': 2, 'z': 1}|{'p': 2, 'q': 1}|['m', 'n'])J2", nullptr },
        { "loop", R"J2({"b":1,"a":2,"c":{"z":1,"y":2,"x":{"n":0,"m":1}},"d":[{"q":1,"p":2}]})J2",
          R"J2({% for k, v in j.c.items() %}{{ k }}={{ v }};{% endfor %})J2",
          R"J2(z=1;y=2;x={'n': 0, 'm': 1};)J2", R"J2(x={'m': 1, 'n': 0};y=2;z=1;)J2", nullptr },
        { "keys", R"J2({"b":1,"a":2,"c":{"z":1,"y":2,"x":{"n":0,"m":1}},"d":[{"q":1,"p":2}]})J2",
          R"J2({{ j.keys()|list }}|{{ j.values()|list|length }}|{{ j|join(',') }}|{{ j|first }}|{{ j|last }})J2",
          R"J2(['b', 'a', 'c', 'd']|4|b,a,c,d|b|d)J2", R"J2(['a', 'b', 'c', 'd']|4|a,b,c,d|a|d)J2", nullptr },
    };
    return rows;
}

inline const std::vector<Row>& MissingRows()
{
    static const std::vector<Row> rows = {
        { "keys", R"J2({"l":[10,20,30],"o":{"k":1}})J2",
          R"J2({{ j.x is defined }}|{{ j.get('x', 1) }}|{{ j.o.zz is defined }}|{{ j.o.k }}|{{ 'k' in j.o }}|{{ 'zz' in j.o }})J2",
          R"J2(False|1|False|1|True|False)J2", nullptr, nullptr },
        { "index", R"J2({"l":[10,20,30],"o":{"k":1}})J2",
          R"J2({{ j.l[9] is defined }}|{{ j.l[-1] }}|{{ j.l[-3] }}|{{ j.l[-4] is defined }}|{{ j.l[3] is defined }}|{{ j.l[0] }})J2",
          R"J2(False|30|10|False|False|10)J2", nullptr, nullptr },
        { "default", R"J2({"l":[10,20,30],"o":{"k":1}})J2",
          R"J2({{ j.x|default('d') }}|{{ j.l[9]|default('d') }}|{{ j.o.zz|default('d') }})J2",
          R"J2(d|d|d)J2", nullptr, nullptr },
    };
    return rows;
}

inline const std::vector<Row>& EmptyRows()
{
    static const std::vector<Row> rows = {
        { "length", R"J2({"e":{},"a":[],"s":""})J2",
          R"J2({{ j.e|length }}|{{ j.a|length }}|{{ j.e is mapping }}|{{ j.a is mapping }}|{{ j.a is sequence }}|{{ j.e is iterable }})J2",
          R"J2(0|0|True|False|True|True)J2", nullptr, nullptr },
        { "for_else", R"J2({"e":{},"a":[],"s":""})J2",
          R"J2({% for x in j.a %}x{% else %}empty-list{% endfor %}|{% for k in j.e %}x{% else %}empty-map{% endfor %})J2",
          R"J2(empty-list|empty-map)J2", nullptr, nullptr },
        { "truthy", R"J2({"e":{},"a":[],"s":""})J2",
          R"J2({{ j.e or 'falsy' }}|{{ j.a or 'falsy' }}|{{ j.s or 'falsy' }}|{{ 'T' if j.e else 'F' }}|{{ j.a|list }}|{{ j.e|list }}|{{ j }})J2",
          R"J2(falsy|falsy|falsy|F|[]|[]|{'e': {}, 'a': [], 's': ''})J2", R"J2(falsy|falsy|falsy|F|[]|[]|{'a': [], 'e': {}, 's': ''})J2", nullptr },
        { "root_object", R"J2({})J2",
          R"J2({{ j|length }}|{{ j }}|{{ j or 'falsy' }}|{{ j is mapping }})J2",
          R"J2(0|{}|falsy|True)J2", nullptr, nullptr },
        { "root_array", R"J2([])J2",
          R"J2({{ j|length }}|{{ j }}|{{ j or 'falsy' }}|{{ j is mapping }})J2",
          R"J2(0|[]|falsy|False)J2", nullptr, nullptr },
    };
    return rows;
}

inline const std::vector<Row>& NullsRows()
{
    static const std::vector<Row> rows = {
        { "member", R"J2({"x":null,"l":[1,null,3],"o":{"n":null}})J2",
          R"J2({{ j.x is defined }}|{{ j.x is none }}|[{{ j.x }}]|{{ j.x|string }}|{{ j.y is defined }}|{{ j.o.n is none }})J2",
          R"J2(True|True|[None]|None|False|True)J2", nullptr, nullptr },
        { "item", R"J2({"x":null,"l":[1,null,3],"o":{"n":null}})J2",
          R"J2({{ j.l }}|{{ j.l[1] is none }}|{{ j.l[1] is defined }}|{% for v in j.l %}{{ v is none }},{% endfor %}|{{ j.l|length }})J2",
          R"J2([1, None, 3]|True|True|False,True,False,|3)J2", nullptr, nullptr },
        { "default", R"J2({"x":null,"l":[1,null,3],"o":{"n":null}})J2",
          R"J2({{ j.x|default('d') }}|{{ j.y|default('d') }}|{{ j.x|default('d', true) }})J2",
          R"J2(None|d|d)J2", nullptr, nullptr },
        { "repr", R"J2({"x":null,"l":[1,null,3],"o":{"n":null}})J2",
          R"J2({{ j }}|{{ j|tojson }})J2",
          R"J2({'x': None, 'l': [1, None, 3], 'o': {'n': None}}|{"l": [1, null, 3], "o": {"n": null}, "x": null})J2", R"J2({'l': [1, None, 3], 'o': {'n': None}, 'x': None}|{"l": [1, null, 3], "o": {"n": null}, "x": null})J2", nullptr },
    };
    return rows;
}

inline const std::vector<Row>& StringsRows()
{
    static const std::vector<Row> rows = {
        { "escapes", R"J2({"e":"a\"b\\c\/d\n\t","c":"\u0001"})J2",
          R"J2({{ j.e|length }}|{{ j.e|tojson }}|{{ j.c|length }})J2",
          R"J2(9|"a\"b\\c/d\n\t"|1)J2", nullptr, nullptr },
        { "unicode", R"J2({"u":"\u00e9","bmp":"\u2603"})J2",
          R"J2({{ j.u|length }}|{{ j.bmp|length }}|{{ j.u }}{{ j.bmp }})J2",
          "1|1|\303\251\342\230\203", nullptr, L"1|1|\u00e9\u2603" },
        { "surrogate_pair", R"J2({"sp":"\ud83d\ude00"})J2",
          R"J2({{ j.sp|length }}|{{ j.sp }})J2",
          "1|\360\237\230\200", nullptr, L"1|\U0001f600" },
        { "nul", R"J2({"nul":"a\u0000b"})J2",
          R"J2({{ j.nul|length }}|{{ j.nul|tojson }}|{{ j.nul|list|length }})J2",
          R"J2(3|"a\u0000b"|3)J2", nullptr, nullptr },
        { "unicode_key", R"J2({"k\u00e9y":1})J2",
          R"J2({{ j|list }}|{{ j|length }}|{% for k in j %}{{ k|length }}{% endfor %})J2",
          "['k\303\251y']|1|3", nullptr, L"['k\u00e9y']|1|3" },
    };
    return rows;
}

inline const std::vector<Row>& IterationRows()
{
    static const std::vector<Row> rows = {
        { "sort", R"J2({"l":[3,1,2],"w":["b","a","c"],"o":[{"n":"x","v":2},{"n":"y","v":1},{"n":"z","v":3}],"f":[1.5,0.5],"e":[]})J2",
          R"J2({{ j.l|sort }}|{{ j.l|sort(reverse=true) }}|{{ j.w|sort }}|{{ j.o|sort(attribute='v')|map(attribute='n')|join }})J2",
          R"J2([1, 2, 3]|[3, 2, 1]|['a', 'b', 'c']|yxz)J2", nullptr, nullptr },
        { "map_join", R"J2({"l":[3,1,2],"w":["b","a","c"],"o":[{"n":"x","v":2},{"n":"y","v":1},{"n":"z","v":3}],"f":[1.5,0.5],"e":[]})J2",
          R"J2({{ j.o|map(attribute='n')|join(',') }}|{{ j.w|join('-') }}|{{ j.l|map('string')|join('+') }})J2",
          R"J2(x,y,z|b-a-c|3+1+2)J2", nullptr, nullptr },
        { "first_last", R"J2({"l":[3,1,2],"w":["b","a","c"],"o":[{"n":"x","v":2},{"n":"y","v":1},{"n":"z","v":3}],"f":[1.5,0.5],"e":[]})J2",
          R"J2({{ j.l|first }}|{{ j.l|last }}|{{ j.e|first }}|{{ j.l|length }}|{{ j.l|sum }}|{{ j.f|sum }}|{{ j.l|max }}|{{ j.l|min }})J2",
          R"J2(3|2||3|6|2.0|3|1)J2", nullptr, nullptr },
        { "slice", R"J2({"l":[3,1,2],"w":["b","a","c"],"o":[{"n":"x","v":2},{"n":"y","v":1},{"n":"z","v":3}],"f":[1.5,0.5],"e":[]})J2",
          R"J2({{ j.l[1:] }}|{{ j.l[:2] }}|{{ j.l[::-1] }}|{{ j.l[-2:] }}|{{ j.l|reverse|list }}|{{ j.l|batch(2)|list }}|{{ j.l|slice(2)|list }})J2",
          R"J2([1, 2]|[3, 1]|[2, 1, 3]|[1, 2]|[2, 1, 3]|[[3, 1], [2]]|[[3, 1], [2]])J2", nullptr, nullptr },
        { "loop", R"J2({"l":[3,1,2],"w":["b","a","c"],"o":[{"n":"x","v":2},{"n":"y","v":1},{"n":"z","v":3}],"f":[1.5,0.5],"e":[]})J2",
          R"J2({% for x in j.l %}{{ loop.index }}:{{ x }}{% if not loop.last %},{% endif %}{% endfor %}|{% for o in j.o %}{{ loop.index0 }}{{ o.n }}{{ loop.revindex }};{% endfor %})J2",
          R"J2(1:3,2:1,3:2|0x3;1y2;2z1;)J2", nullptr, nullptr },
        { "select", R"J2({"l":[3,1,2],"w":["b","a","c"],"o":[{"n":"x","v":2},{"n":"y","v":1},{"n":"z","v":3}],"f":[1.5,0.5],"e":[]})J2",
          R"J2({{ j.o|selectattr('v','gt',1)|map(attribute='n')|list }}|{{ j.o|rejectattr('v','eq',2)|map(attribute='n')|join }}|{{ j.l|select('odd')|list }}|{{ j.o|map(attribute='v')|sum }})J2",
          R"J2(['x', 'z']|yz|[3, 1]|6)J2", nullptr, nullptr },
        { "membership", R"J2({"l":[3,1,2],"w":["b","a","c"],"o":[{"n":"x","v":2},{"n":"y","v":1},{"n":"z","v":3}],"f":[1.5,0.5],"e":[]})J2",
          R"J2({{ 2 in j.l }}|{{ 'b' in j.w }}|{{ 'n' in j.o[0] }}|{{ 5 in j.l }}|{% for k in j.o[0] %}{{ k }}{% endfor %})J2",
          R"J2(True|True|True|False|nv)J2", nullptr, nullptr },
        { "nested_loop", R"J2({"o":[{"n":"xy","v":2},{"n":"z","v":1}]})J2",
          R"J2({% for o in j.o %}{% for c in o.n %}{{ c }}{% endfor %};{% endfor %}|{% for k, v in j.o[0].items() %}{{ k }}{{ v }}{% endfor %})J2",
          R"J2(xy;z;|nxyv2)J2", nullptr, nullptr },
    };
    return rows;
}

inline const std::vector<Row>& EqualityRows()
{
    static const std::vector<Row> rows = {
        { "structural", R"J2({"a":{"x":[1,2],"y":{"z":null}},"b":{"x":[1,2],"y":{"z":null}},"c":{"x":[1,3]},"l":[1,2]})J2",
          R"J2({{ j.a == j.b }}|{{ j.a == j.c }}|{{ j.a != j.c }}|{{ j.a == j.a }}|{{ j.a.x == j.b.x }}|{{ j.a.y == j.b.y }})J2",
          R"J2(True|False|True|True|True|True)J2", nullptr, nullptr },
        { "list", R"J2({"a":{"x":[1,2],"y":{"z":null}},"b":{"x":[1,2],"y":{"z":null}},"c":{"x":[1,3]},"l":[1,2]})J2",
          R"J2({{ j.l == [1, 2] }}|{{ j.l == [2, 1] }}|{{ j.l == j.a.x }}|{{ j.a.x in [j.b.x] }}|{{ j.l != [1] }})J2",
          R"J2(True|False|True|True|True)J2", nullptr, nullptr },
        { "literal", R"J2({"a":{"x":[1,2],"y":{"z":null}},"b":{"x":[1,2],"y":{"z":null}},"c":{"x":[1,3]},"l":[1,2]})J2",
          R"J2({{ j.c == {'x': [1, 3]} }}|{{ j.a.y == {'z': none} }})J2",
          R"J2(True|True)J2", nullptr, nullptr },
    };
    return rows;
}

inline const std::vector<Row>& LifetimeRows()
{
    static const std::vector<Row> rows = {
        { "sort", R"J2({"list":[3,1,2],"obj":{"a":1,"b":[1,2]}})J2",
          R"J2({% set r = rjson().list | sort %}{{ r }})J2",
          R"J2([1, 2, 3])J2", nullptr, nullptr },
        { "items_tojson", R"J2({"list":[3,1,2],"obj":{"a":1,"b":[1,2]}})J2",
          R"J2({{ rjson().obj.items()|list|tojson }})J2",
          R"J2([["a", 1], ["b", [1, 2]]])J2", nullptr, nullptr },
        { "slice", R"J2({"list":[3,1,2],"obj":{"a":1,"b":[1,2]}})J2",
          R"J2({{ rjson().list[1:] }})J2",
          R"J2([1, 2])J2", nullptr, nullptr },
        { "namespace_list", R"J2({"list":[3,1,2],"obj":{"a":1,"b":[1,2]}})J2",
          R"J2({% set ns = namespace(items=[]) %}{% for x in rjson().list %}{% set ns.items = ns.items + [x] %}{% endfor %}{{ ns.items }})J2",
          R"J2([3, 1, 2])J2", nullptr, nullptr },
        { "namespace_object", R"J2({"list":[3,1,2],"obj":{"a":1,"b":[1,2]}})J2",
          R"J2({% set ns = namespace(o=none) %}{% for i in [1] %}{% set ns.o = rjson().obj %}{% endfor %}{{ ns.o }}|{{ ns.o.b[1] }}|{{ ns.o.b|sum }})J2",
          R"J2({'a': 1, 'b': [1, 2]}|2|3)J2", nullptr, nullptr },
        { "set_chain", R"J2({"list":[3,1,2],"obj":{"a":1,"b":[1,2]}})J2",
          R"J2({% set o = rjson().obj %}{% set b = o.b %}{{ b }}|{{ o.a }})J2",
          R"J2([1, 2]|1)J2", nullptr, nullptr },
        { "macro", R"J2({"list":[3,1,2],"obj":{"a":1,"b":[1,2]}})J2",
          R"J2({% macro m(x) %}{{ x.b|join('+') }}{% endmacro %}{{ m(rjson().obj) }})J2",
          R"J2(1+2)J2", nullptr, nullptr },
        { "filters", R"J2({"list":[3,1,2],"obj":{"a":1,"b":[1,2]}})J2",
          R"J2({{ rjson().obj.b|map('string')|join(',') }}|{{ (rjson().list|sort)[0] }})J2",
          R"J2(1,2|1)J2", nullptr, nullptr },
        { "list_of_roots", R"J2({"list":[3,1,2],"obj":{"a":1,"b":[1,2]}})J2",
          R"J2({% set l = [rjson().obj, rjson().obj] %}{{ l[0].b }}|{{ l|length }}|{{ l[1].a }})J2",
          R"J2([1, 2]|2|1)J2", nullptr, nullptr },
    };
    return rows;
}

inline const std::vector<Row>& RepeatedKeysRows()
{
    static const std::vector<Row> rows = {
        { "full", R"J2({"a":1,"b":2,"a":3})J2",
          R"J2({{ j }}|{{ j.a }}|{{ j|length }}|{{ j|list }})J2",
          R"J2({'a': 3, 'b': 2}|3|2|['a', 'b'])J2", nullptr, nullptr },
        { "nested", R"J2({"o":{"k":1,"k":2,"m":0}})J2",
          R"J2({{ j }})J2",
          R"J2({'o': {'k': 2, 'm': 0}})J2", nullptr, nullptr },
        { "other_key", R"J2({"a":1,"b":2,"a":3})J2",
          R"J2({{ j.b }})J2",
          R"J2(2)J2", nullptr, nullptr },
    };
    return rows;
}

// Runs a row in the given modes, narrow and wide
constexpr int kViewModes = 1;
constexpr int kCopyMode = 2;
constexpr int kAllModes = kViewModes | kCopyMode;

inline std::wstring Widen(const std::string& text)
{
    return std::wstring(text.begin(), text.end()); // the rows are ASCII except where a wide expectation is given
}

inline std::string Narrow(const std::wstring& text)
{
    std::string result;
    for (wchar_t ch : text)
    {
        result += ch < 128 ? static_cast<char>(ch) : '?';
    }
    return result;
}

inline std::string Describe(const jinja2::ErrorInfo& err)
{
    return err.ToString();
}

inline std::string Describe(const jinja2::ErrorInfoW& err)
{
    return Narrow(err.ToString());
}

// Renders `tpl` with a narrow and a wide template and compares with the expectation
inline void RenderBoth(const jinja2::ValuesMap& params, const char* tpl, const std::string& expected, const wchar_t* expectedWide = nullptr)
{
    {
        jinja2::Template narrow;
        auto loaded = narrow.Load(std::string(tpl));
        ASSERT_TRUE(loaded.has_value()) << Describe(loaded.error()) << "\n"
                                        << tpl;
        auto out = narrow.RenderAsString(params);
        ASSERT_TRUE(out.has_value()) << Describe(out.error()) << "\n"
                                     << tpl;
        EXPECT_EQ(expected, *out) << "narrow template: " << tpl;
    }
    {
        jinja2::TemplateW wide;
        auto loaded = wide.Load(Widen(tpl));
        ASSERT_TRUE(loaded.has_value()) << Describe(loaded.error()) << "\n"
                                        << tpl;
        auto out = wide.RenderAsString(params);
        ASSERT_TRUE(out.has_value()) << Describe(out.error()) << "\n"
                                     << tpl;
        const std::wstring want = expectedWide ? std::wstring(expectedWide) : Widen(expected);
        EXPECT_TRUE(want == *out) << "wide template: " << tpl << "\nexpected: " << Narrow(want) << "\nactual:   " << Narrow(*out);
    }
}

// Calls fn(const Value& j) with the document in every way in: a borrowed view, an owned view, a shared view and a copy
// NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward): called for each mode, never moved
template<typename B, typename Fn>
void ForEachMode(std::string_view text, int modes, Fn&& fn)
{
    using Json = typename B::Json;
    if constexpr (B::kViewable)
    {
        if (modes & kViewModes)
        {
            {
                SCOPED_TRACE("view-borrowed");
                const Json doc = B::Parse(text);
                const jinja2::Value value = jinja2::Reflect(std::as_const(doc));
                fn(value);
            }
            {
                SCOPED_TRACE("view-owned");
                const jinja2::Value value = jinja2::Reflect(B::Parse(text));
                fn(value);
            }
            {
                SCOPED_TRACE("view-shared");
                const jinja2::Value value = jinja2::Reflect(std::make_shared<Json>(B::Parse(text)));
                fn(value);
            }
        }
    }
    if (modes & kCopyMode)
    {
        SCOPED_TRACE("copy");
        auto copy = jinja2::ToValue(B::Parse(text));
        ASSERT_TRUE(copy.has_value()) << Describe(copy.error());
        fn(*copy);
    }
}

template<typename B>
void RunRows(const std::vector<Row>& rows, int modes, const char* skipName = nullptr)
{
    for (const Row& row : rows)
    {
        if (skipName && std::string_view(row.name) == skipName)
        {
            continue;
        }
        SCOPED_TRACE(row.name);
        ForEachMode<B>(row.doc, modes, [&row](const jinja2::Value& j) {
            const bool sorted = B::kSortedKeys && row.expectedSorted;
            RenderBoth({ { "j", j } }, row.tpl, sorted ? row.expectedSorted : row.expected, sorted ? nullptr : row.expectedWide);
        });
    }
}

inline std::string Nest(std::size_t depth)
{
    return std::string(depth, '[') + std::string(depth, ']');
}

inline std::string DeepPath(std::size_t depth)
{
    std::string path = "$";
    for (std::size_t n = 0; n < depth; ++n)
    {
        path += "[0]";
    }
    return path;
}

inline std::string AsText(const jinja2::Value& val)
{
    std::string_view text;
    return val.AsString(text) ? std::string(text) : std::string("<not a string>");
}

// What a failed ToValue reports: code, the limit's or traits' message, and the JSON path last
struct FailureCheck
{
    jinja2::ErrorCode code;
    const char* limit; // ExtraParams[1] of a limit error, or null
    std::string path;
};

inline void ExpectFailure(const nonstd::expected<jinja2::Value, jinja2::ErrorInfo>& result, const FailureCheck& want)
{
    ASSERT_FALSE(result.has_value());
    const jinja2::ErrorInfo& err = result.error();
    EXPECT_EQ(want.code, err.GetCode());
    const auto& extra = err.GetExtraParams();
    ASSERT_GE(extra.size(), 2u);
    if (want.limit)
    {
        ASSERT_GE(extra.size(), 3u);
        EXPECT_EQ(want.limit, AsText(extra[1]));
    }
    EXPECT_EQ(want.path, AsText(extra.back()));
    EXPECT_EQ(0u, err.GetLocationDescr().find("JSON " + want.path)) << err.GetLocationDescr();
}

// Makes the owned and the shared view of a document
template<typename B>
std::vector<std::function<jinja2::Value()>> OwningFactories(std::string text)
{
    using Json = typename B::Json;
    return { [text] { return jinja2::Reflect(B::Parse(text)); }, [text] { return jinja2::Reflect(std::make_shared<Json>(B::Parse(text))); } };
}

} // namespace jinja2_conformance

template<typename B>
class JsonConformance : public ::testing::Test
{
};

TYPED_TEST_SUITE_P(JsonConformance);

namespace jc = jinja2_conformance;

TYPED_TEST_P(JsonConformance, Scalars)
{
    jc::RunRows<TypeParam>(jc::ScalarsRows(), jc::kAllModes);
}

TYPED_TEST_P(JsonConformance, BigIntegers)
{
    // Beyond uint64 is exact only for traits that supply the integer's text
    jc::RunRows<TypeParam>(jc::BigIntegersRows(), jc::kAllModes, TypeParam::kBigIntText ? nullptr : "beyond_uint64");

    jc::ForEachMode<TypeParam>(R"({"max":9223372036854775807,"min":-9223372036854775808,"b1":9223372036854775808,"b2":18446744073709551615,"l":[18446744073709551615]})", jc::kAllModes, [](const jinja2::Value& j) {
        const jinja2::GenericMap map = j.ToGenericMap();
        const jinja2::Value max = map.GetValueByName("max");
        const jinja2::Value min = map.GetValueByName("min");
        const jinja2::Value b1 = map.GetValueByName("b1");
        const jinja2::Value b2 = map.GetValueByName("b2");
        EXPECT_FALSE(max.IsBigInt());
        EXPECT_FALSE(min.IsBigInt());
        ASSERT_TRUE(max.IsInt());
        ASSERT_TRUE(min.IsInt());
        EXPECT_EQ(INT64_MAX, max.AsInt());
        EXPECT_EQ(INT64_MIN, min.AsInt());
        ASSERT_TRUE(b1.IsBigInt());
        ASSERT_TRUE(b2.IsBigInt());
        EXPECT_EQ("9223372036854775808", b1.AsBigInt().ToString());
        EXPECT_EQ("18446744073709551615", b2.AsBigInt().ToString());
        // An item of a list is a BigInt too
        const jinja2::Value inList = map.GetValueByName("l").ToGenericList().GetAccessor()->GetIndexer()->GetItemByIndex(0);
        ASSERT_TRUE(inList.IsBigInt());
        EXPECT_EQ("18446744073709551615", inList.AsBigInt().ToString());
    });

    if constexpr (TypeParam::kBigIntText)
    {
        jc::ForEachMode<TypeParam>(R"({"h":123456789012345678901234567890,"n":-123456789012345678901234567890})", jc::kAllModes, [](const jinja2::Value& j) {
            const jinja2::GenericMap map = j.ToGenericMap();
            const jinja2::Value huge = map.GetValueByName("h");
            ASSERT_TRUE(huge.IsBigInt());
            EXPECT_EQ("123456789012345678901234567890", huge.AsBigInt().ToString());
            ASSERT_TRUE(map.GetValueByName("n").IsBigInt());
            EXPECT_EQ("-123456789012345678901234567890", map.GetValueByName("n").AsBigInt().ToString());
        });
    }
}

TYPED_TEST_P(JsonConformance, Nesting)
{
    jc::RunRows<TypeParam>(jc::NestingRows(), jc::kAllModes);
}

TYPED_TEST_P(JsonConformance, KeyOrder)
{
    jc::RunRows<TypeParam>(jc::KeyOrderRows(), jc::kAllModes);

    // GetKeys follows the library's order too: C++ callers see what a template sees
    jc::ForEachMode<TypeParam>(R"({"b":1,"a":2,"c":{"z":1,"y":2}})", jc::kAllModes, [](const jinja2::Value& j) {
        const std::vector<std::string> expected = TypeParam::kSortedKeys ? std::vector<std::string>{ "a", "b", "c" } : std::vector<std::string>{ "b", "a", "c" };
        EXPECT_EQ(expected, j.ToGenericMap().GetKeys());
        const std::vector<std::string> inner = TypeParam::kSortedKeys ? std::vector<std::string>{ "y", "z" } : std::vector<std::string>{ "z", "y" };
        EXPECT_EQ(inner, j.ToGenericMap().GetValueByName("c").ToGenericMap().GetKeys());
    });
}

TYPED_TEST_P(JsonConformance, MissingKeysAndIndexes)
{
    jc::RunRows<TypeParam>(jc::MissingRows(), jc::kAllModes);

    // From C++ the indexer is bounds-checked: an index outside the array gives an empty Value, not undefined behaviour
    jc::ForEachMode<TypeParam>(R"({"l":[10,20,30],"o":{"k":1},"e":[]})", jc::kViewModes, [](const jinja2::Value& j) {
        const jinja2::GenericMap map = j.ToGenericMap();
        const jinja2::GenericList list = map.GetValueByName("l").ToGenericList();
        ASSERT_TRUE(list.IsValid());
        const jinja2::IIndexBasedAccessor* indexer = list.GetAccessor()->GetIndexer();
        ASSERT_NE(nullptr, indexer);
        EXPECT_EQ(3u, list.GetSize().value_or(0));
        EXPECT_TRUE(indexer->GetItemByIndex(99).IsNone());
        EXPECT_TRUE(indexer->GetItemByIndex(-1).IsNone());
        EXPECT_TRUE(indexer->GetItemByIndex(3).IsNone());
        EXPECT_TRUE(indexer->GetItemByIndex(INT64_MAX).IsNone());
        EXPECT_TRUE(indexer->GetItemByIndex(INT64_MIN).IsNone());
        ASSERT_TRUE(indexer->GetItemByIndex(2).IsInt());
        EXPECT_EQ(30, indexer->GetItemByIndex(2).AsInt());

        const jinja2::GenericList empty = map.GetValueByName("e").ToGenericList();
        ASSERT_NE(nullptr, empty.GetAccessor()->GetIndexer());
        EXPECT_TRUE(empty.GetAccessor()->GetIndexer()->GetItemByIndex(0).IsNone());
        EXPECT_FALSE(map.Find("zz").has_value());
        EXPECT_FALSE(map.HasValue("zz"));
        EXPECT_TRUE(map.GetValueByName("zz").IsNone());
    });
}

TYPED_TEST_P(JsonConformance, EmptyContainers)
{
    jc::RunRows<TypeParam>(jc::EmptyRows(), jc::kAllModes);
}

TYPED_TEST_P(JsonConformance, NullMemberAndNullItem)
{
    jc::RunRows<TypeParam>(jc::NullsRows(), jc::kAllModes);

    jc::ForEachMode<TypeParam>(R"({"x":null,"l":[1,null,3]})", jc::kAllModes, [](const jinja2::Value& j) {
        const jinja2::GenericMap map = j.ToGenericMap();
        EXPECT_EQ(2u, map.GetSize());
        EXPECT_TRUE(map.HasValue("x")); // present, though null
        EXPECT_FALSE(map.HasValue("y"));
        EXPECT_TRUE(map.GetValueByName("x").IsNone());
        const jinja2::Value list = map.GetValueByName("l");
        EXPECT_EQ(3u, list.ToGenericList().GetSize().value_or(0));
        EXPECT_TRUE(list.ToGenericList().GetAccessor()->GetIndexer()->GetItemByIndex(1).IsNone());
    });
}

TYPED_TEST_P(JsonConformance, Strings)
{
    jc::RunRows<TypeParam>(jc::StringsRows(), jc::kAllModes);

    // The NUL is kept, in the value and in the length
    jc::ForEachMode<TypeParam>(R"({"nul":"a\u0000b","sp":"\ud83d\ude00"})", jc::kAllModes, [](const jinja2::Value& j) {
        EXPECT_EQ(std::string("a\0b", 3), j.ToGenericMap().GetValueByName("nul").ToString());
        EXPECT_EQ("\xF0\x9F\x98\x80", j.ToGenericMap().GetValueByName("sp").ToString()); // UTF-8
    });
}

TYPED_TEST_P(JsonConformance, IterationAndFilters)
{
    jc::RunRows<TypeParam>(jc::IterationRows(), jc::kAllModes);

    // Enumerating a list from C++: every item in order, again after Reset, and a Clone continues on its own
    jc::ForEachMode<TypeParam>(R"({"l":[3,1,2]})", jc::kAllModes, [](const jinja2::Value& j) {
        const jinja2::GenericList list = j.ToGenericMap().GetValueByName("l").ToGenericList();
        auto created = list.GetAccessor()->CreateEnumerator();
        ASSERT_TRUE(created.has_value());
        jinja2::ListEnumeratorPtr& en = *created;
        std::vector<int64_t> seen;
        for (int pass = 0; pass < 2; ++pass)
        {
            seen.clear();
            while (en->MoveNext())
            {
                seen.push_back(en->GetCurrent().AsInt());
            }
            EXPECT_EQ((std::vector<int64_t>{ 3, 1, 2 }), seen);
            en->Reset();
        }
        ASSERT_TRUE(en->MoveNext());
        auto clone = en->Clone();
        ASSERT_FALSE(clone.valueless_after_move());
        EXPECT_EQ(3, en->GetCurrent().AsInt());
        ASSERT_TRUE(clone->MoveNext());
        EXPECT_EQ(1, clone->GetCurrent().AsInt());
        EXPECT_EQ(3, en->GetCurrent().AsInt());
    });
}

TYPED_TEST_P(JsonConformance, EnumeratorPastTheEnd)
{
    // The enumerator of a view gives an empty Value before the first and past the last item (the interface's contract)
    jc::ForEachMode<TypeParam>(R"({"l":[3,1,2]})", jc::kViewModes, [](const jinja2::Value& j) {
        const jinja2::GenericList list = j.ToGenericMap().GetValueByName("l").ToGenericList();
        auto created = list.GetAccessor()->CreateEnumerator();
        ASSERT_TRUE(created.has_value());
        EXPECT_TRUE((*created)->GetCurrent().IsNone());
        while ((*created)->MoveNext())
        {
        }
        EXPECT_FALSE((*created)->MoveNext());
        EXPECT_TRUE((*created)->GetCurrent().IsNone());
    });
}

TYPED_TEST_P(JsonConformance, Equality)
{
    jc::RunRows<TypeParam>(jc::EqualityRows(), jc::kViewModes);

    if constexpr (TypeParam::kViewable)
    {
        // Copies of one reflected Value share the document and compare equal; another document is another node
        const std::string text = R"({"a":{"x":[1,2]},"l":[1,2]})";
        for (auto& make : jc::OwningFactories<TypeParam>(text))
        {
            const jinja2::Value first = make();
            const jinja2::Value second = first; // NOLINT(performance-unnecessary-copy-initialization): the copy is the subject
            EXPECT_TRUE(first == second);
            EXPECT_TRUE(first.ToGenericMap() == second.ToGenericMap());
            EXPECT_TRUE(first.ToGenericMap().GetValueByName("l").ToGenericList() == second.ToGenericMap().GetValueByName("l").ToGenericList());
            EXPECT_TRUE(first.ToGenericMap().GetValueByName("a").ToGenericMap() == second.ToGenericMap().GetValueByName("a").ToGenericMap());
            const jinja2::Value other = make();
            EXPECT_FALSE(first.ToGenericMap() == other.ToGenericMap());
        }
        const typename TypeParam::Json doc = TypeParam::Parse(text);
        const jinja2::Value borrowed = jinja2::Reflect(std::as_const(doc));
        const jinja2::Value again = borrowed; // NOLINT(performance-unnecessary-copy-initialization): the copy is the subject
        EXPECT_TRUE(borrowed.ToGenericMap() == again.ToGenericMap());
        EXPECT_TRUE(borrowed.ToGenericMap() == jinja2::Reflect(std::as_const(doc)).ToGenericMap());
    }
}

TYPED_TEST_P(JsonConformance, LifetimeInTemplates)
{
    // Task 0125: a callable returns a reflected document that nothing else holds; its items outlive the call
    if constexpr (TypeParam::kViewable)
    {
        for (const jc::Row& row : jc::LifetimeRows())
        {
            SCOPED_TRACE(row.name);
            for (auto& make : jc::OwningFactories<TypeParam>(row.doc))
            {
                jinja2::ValuesMap params;
                params["rjson"] = jinja2::MakeCallable([make] { return make(); });
                jc::RenderBoth(params, row.tpl, TypeParam::kSortedKeys && row.expectedSorted ? row.expectedSorted : row.expected);
            }
        }
    }
}

// Nested items, enumerators and indexed items must read right after the root and every copy of it are gone
TYPED_TEST_P(JsonConformance, LifetimeOfItemsWithoutEngine)
{
    if constexpr (TypeParam::kViewable)
    {
        const std::string text = R"({"m":{"k":[10,20,30],"s":"str"},"l":[[1,2],[3,4]]})";
        for (auto& make : jc::OwningFactories<TypeParam>(text))
        {
            std::optional<jinja2::Value> map;
            std::optional<jinja2::Value> item;
            std::optional<jinja2::ListEnumeratorPtr> enumerator;
            {
                auto root = std::make_unique<jinja2::Value>(make());
                auto rootCopy = std::make_unique<jinja2::Value>(*root);
                auto rootMap = std::make_unique<jinja2::GenericMap>(root->ToGenericMap());
                map = rootMap->GetValueByName("m");
                {
                    const jinja2::Value inner = map->ToGenericMap().GetValueByName("k");
                    const jinja2::GenericList list = inner.ToGenericList();
                    enumerator = list.GetAccessor()->CreateEnumerator();
                }
                item = rootMap->GetValueByName("l").ToGenericList().GetAccessor()->GetIndexer()->GetItemByIndex(1);
                rootMap.reset();
                rootCopy.reset();
                root.reset();
            }
            ASSERT_TRUE(map && item && enumerator);

            EXPECT_EQ("str", map->ToGenericMap().GetValueByName("s").AsString());
            const jinja2::Value k = map->ToGenericMap().GetValueByName("k");
            const jinja2::GenericList kList = k.ToGenericList();
            ASSERT_EQ(3u, kList.GetSize().value_or(0));
            EXPECT_EQ(30, kList.GetAccessor()->GetIndexer()->GetItemByIndex(2).AsInt());

            int64_t sum = 0;
            int count = 0;
            while ((*enumerator)->MoveNext())
            {
                sum += (*enumerator)->GetCurrent().AsInt();
                ++count;
            }
            EXPECT_EQ(3, count);
            EXPECT_EQ(60, sum);

            const jinja2::GenericList pair = item->ToGenericList();
            ASSERT_EQ(2u, pair.GetSize().value_or(0));
            EXPECT_EQ(3, pair.GetAccessor()->GetIndexer()->GetItemByIndex(0).AsInt());
            EXPECT_EQ(4, pair.GetAccessor()->GetIndexer()->GetItemByIndex(1).AsInt());
        }
    }
}

TYPED_TEST_P(JsonConformance, ConcurrentRenders)
{
    if constexpr (TypeParam::kViewable)
    {
        const std::string text = R"({"l":[3,1,2],"o":[{"n":"x","v":2},{"n":"y","v":1}],"m":{"k":{"z":"deep"}}})";
        const char* tpl = "{% for o in j.o %}{{ o.n }}{{ o.v }}{% endfor %}|{{ j.l|sort }}|{{ j.m.k.z }}|{{ j|length }}";
        const typename TypeParam::Json doc = TypeParam::Parse(text);
        // One borrowed view and one owned view, each rendered by four threads at once
        for (const jinja2::Value& view : { jinja2::Reflect(std::as_const(doc)), jinja2::Reflect(TypeParam::Parse(text)) })
        {
            const jinja2::ValuesMap params{ { "j", view } };
            jinja2::Template compiled;
            ASSERT_TRUE(compiled.Load(std::string(tpl)).has_value());
            constexpr std::size_t kThreads = 4;
            constexpr int kRenders = 200;
            std::vector<std::vector<std::string>> outputs(kThreads);
            std::vector<std::thread> threads;
            for (std::size_t n = 0; n < kThreads; ++n)
            {
                threads.emplace_back([&, n] {
                    for (int pass = 0; pass < kRenders; ++pass)
                    {
                        auto out = compiled.RenderAsString(params);
                        outputs[n].push_back(out.has_value() ? *out : "error: " + out.error().ToString());
                    }
                });
            }
            for (auto& thread : threads)
            {
                thread.join();
            }
            for (const auto& perThread : outputs)
            {
                ASSERT_EQ(static_cast<std::size_t>(kRenders), perThread.size());
                for (const auto& out : perThread)
                {
                    ASSERT_EQ("x2y1|[1, 2, 3]|deep|3", out);
                }
            }
        }
    }
}

TYPED_TEST_P(JsonConformance, CopyLimits)
{
    using jc::ExpectFailure;
    using jinja2::ErrorCode;
    using jinja2::JsonCopyLimits;
    using TypeP = TypeParam;

    // Arrays and objects open at once
    EXPECT_TRUE(jinja2::ToValue(TypeP::Parse(jc::Nest(128))).has_value());
    EXPECT_TRUE(jinja2::ToValue(TypeP::Parse(jc::Nest(128)), JsonCopyLimits{ 128, 3 }).has_value());
    ExpectFailure(jinja2::ToValue(TypeP::Parse(jc::Nest(129))), { ErrorCode::ResourceLimitExceeded, "maxDepth", jc::DeepPath(128) });
    ExpectFailure(jinja2::ToValue(TypeP::Parse(jc::Nest(5)), JsonCopyLimits{ 4, 0 }), { ErrorCode::ResourceLimitExceeded, "maxDepth", jc::DeepPath(4) });
    ExpectFailure(jinja2::ToValue(TypeP::Parse(R"({"a":{"b":[{"c":1}]}})"), JsonCopyLimits{ 3, 0 }), { ErrorCode::ResourceLimitExceeded, "maxDepth", "$.a.b[0]" });

    // Items of one array, members of one object and code units of one string
    const JsonCopyLimits three{ 128, 3 };
    EXPECT_TRUE(jinja2::ToValue(TypeP::Parse(R"({"a":[1,2,3],"o":{"p":1,"q":2,"r":3},"s":"xyz"})"), three).has_value());
    ExpectFailure(jinja2::ToValue(TypeP::Parse(R"({"a":[1,2,3,4]})"), three), { ErrorCode::ResourceLimitExceeded, "maxSequenceSize", "$.a" });
    ExpectFailure(jinja2::ToValue(TypeP::Parse(R"({"a":{"p":1,"q":2,"r":3,"s":4}})"), three), { ErrorCode::ResourceLimitExceeded, "maxSequenceSize", "$.a" });
    ExpectFailure(jinja2::ToValue(TypeP::Parse(R"({"a":["xyz","abcd"]})"), three), { ErrorCode::ResourceLimitExceeded, "maxSequenceSize", "$.a[1]" });
    ExpectFailure(jinja2::ToValue(TypeP::Parse(R"({"a b":"abcd"})"), three), { ErrorCode::ResourceLimitExceeded, "maxSequenceSize", "$[\"a b\"]" });
    ExpectFailure(jinja2::ToValue(TypeP::Parse(R"([[1,2,3,4]])"), three), { ErrorCode::ResourceLimitExceeded, "maxSequenceSize", "$[0]" });
    ExpectFailure(jinja2::ToValue(TypeP::Parse(R"({"x":[{"k\"q":"abcd"}]})"), three), { ErrorCode::ResourceLimitExceeded, "maxSequenceSize", R"($.x[0]["k\"q"])" });
    ExpectFailure(jinja2::ToValue(TypeP::Parse(R"({"1x":[true]})"), JsonCopyLimits{ 1, 0 }), { ErrorCode::ResourceLimitExceeded, "maxDepth", "$[\"1x\"]" });

    // The error has no byte offset unless the traits have a Location
    if constexpr (!jinja2::detail::json::HasLocation<typename TypeP::Json>::value)
    {
        auto err = jinja2::ToValue(TypeP::Parse(R"({"a":[1,2,3,4]})"), three);
        ASSERT_FALSE(err.has_value());
        EXPECT_EQ(std::string::npos, err.error().GetLocationDescr().find("(char"));
    }

    // A large cap does not raise the built-in one
    auto big = jinja2::ToValue(TypeP::Parse(R"({"a":[1,2,3,4]})"), JsonCopyLimits{ 128, ~std::uint64_t{ 0 } });
    EXPECT_TRUE(big.has_value());
}

TYPED_TEST_P(JsonConformance, RepeatedKeys)
{
    const auto& rows = jc::RepeatedKeysRows();
    for (const jc::Row& row : rows)
    {
        SCOPED_TRACE(row.name);
        const bool onlyOtherKey = std::string_view(row.name) == "other_key";
        // ToValue follows json.loads: the first position, the last value. A view shows what the library parsed
        jc::ForEachMode<TypeParam>(row.doc, jc::kCopyMode, [&row](const jinja2::Value& j) {
            jc::RenderBoth({ { "j", j } }, row.tpl, row.expected);
        });
        if constexpr (TypeParam::kViewable)
        {
            if (onlyOtherKey || !TypeParam::kViewDuplicateKeys)
            {
                jc::ForEachMode<TypeParam>(row.doc, jc::kViewModes, [&row](const jinja2::Value& j) {
                    jc::RenderBoth({ { "j", j } }, row.tpl, row.expected);
                });
            }
        }
    }
    // The copy has the key once
    auto copy = jinja2::ToValue(TypeParam::Parse(R"({"a":1,"b":2,"a":3})"));
    ASSERT_TRUE(copy.has_value());
    ASSERT_TRUE(copy->IsMap());
    EXPECT_EQ(2u, copy->ToGenericMap().GetSize());
    EXPECT_EQ(3, copy->ToGenericMap().GetValueByName("a").AsInt());
}

TYPED_TEST_P(JsonConformance, DeepViewRendersOrFailsCleanly)
{
    // 1000 levels through the view: a rendered value or an error Result, never a crash (ASan runs this)
    jc::ForEachMode<TypeParam>(jc::Nest(1000), jc::kViewModes, [](const jinja2::Value& j) {
        jinja2::Template compiled;
        ASSERT_TRUE(compiled.Load(std::string("{{ j }}")).has_value());
        const jinja2::ValuesMap params{ { "j", j } };
        auto out = compiled.RenderAsString(params);
        if (out.has_value())
        {
            EXPECT_EQ(2000u, out->size());
        }
        else
        {
            EXPECT_NE(jinja2::ErrorCode::Unspecified, out.error().GetCode());
        }
    });
}

REGISTER_TYPED_TEST_SUITE_P(JsonConformance, Scalars, BigIntegers, Nesting, KeyOrder, MissingKeysAndIndexes, EmptyContainers, NullMemberAndNullItem, Strings, IterationAndFilters, EnumeratorPastTheEnd, Equality, LifetimeInTemplates, LifetimeOfItemsWithoutEngine, ConcurrentRenders, CopyLimits, RepeatedKeys, DeepViewRendersOrFailsCleanly);

#endif // JINJA2CPP_TEST_BINDING_CONFORMANCE_H
