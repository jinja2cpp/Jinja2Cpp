# Migrating from Jinja2C++ 1.x to 2.0

This file lists the changes a 1.x user has to act on, with the task that made each one.
Most C++ renames are mechanical and `scripts/jinja2cpp_migrate.py` does them; template
behaviour changes need a look at your templates, and the examples below show the before
and after. The API decisions behind them are in [docs/api-2.0.md](docs/api-2.0.md).

Requirements for 2.0: a C++17 compiler (CI builds with GCC 12 and 14, Clang 18 and 20, MSVC 2022 and
Apple Clang on macOS 15 and 26, at C++17, 20, 23 and 26), CMake 3.23 or newer, and expected-lite,
fmt and fast_float (fetched by the default internal dependency mode). Boost is no longer a
dependency of the library.

## Migrating your code with the script

`scripts/jinja2cpp_migrate.py` (Python 3.8+, no packages) uses your compiler as the type
checker: every 1.x name that 2.0 keeps is `[[deprecated]]` with a message that starts with
`jinja2cpp-2:` and names the replacement, so the build log already says which call is a
`jinja2::Value` member and where it is. GCC, Clang and MSVC logs are read.

```sh
# 1. Fix what no longer compiles (renamed Settings fields, removed headers, expected-lite spellings)
python3 jinja2cpp/scripts/jinja2cpp_migrate.py --rewrite src/ include/
# 2. Build once with deprecation warnings on and keep the log (not -Werror for this build)
cmake --build build 2>&1 | tee build.log          # MSVC: C4996 is on at /W3; add /diagnostics:column
# 3. Rename at every reported location
python3 jinja2cpp/scripts/jinja2cpp_migrate.py --log build.log --root .
# 4. Rebuild; repeat 2-3 until no jinja2cpp-2 warning remains (MSVC can report only one
#    of several deprecated calls on a line, so a line may need a second pass)
```

Add `--dry-run` to see a diff instead of writing. The script prints each change and a list
of what it left for a human: a name produced by a macro, a diagnostic it could not place,
and the rows below marked *by hand*. It never edits the library's own headers. A program
that wants to upgrade first and rename later defines `JINJA2CPP_NO_DEPRECATION_WARNINGS`;
the 1.x names are removed in 3.0. CI runs the script on a 1.x-style copy of our own tests
(`test/migration/`) with every compiler it builds with.

## C++ API

### Renamed, deprecated until 3.0 (the script renames them)

*Tasks 0072, 0074 ([docs/api-2.0.md](docs/api-2.0.md) section 3).*

| 1.x | 2.0 | Note |
|---|---|---|
| `v.isString()`, `isWString()`, `isList()`, `isMap()` | `IsString()`, `IsWString()`, `IsList()`, `IsMap()` | |
| `v.isEmpty()` | `IsNone()` | an empty `Value` is Python's `None` (below) |
| `v.asString()`, `asWString()`, `asList()`, `asMap()` | `AsString()`, `AsWString()`, `AsList()`, `AsMap()` | |
| `v.get<T>()` | `As<T>()` | `As` returns a reference; `get` copied |
| `v.getPtr<T>()` | `GetIf<T>()` | |
| `v.data()` | `GetData()` | |
| free `AsString(x)`, `AsWString(x)` (strings, string views, `Value`) | `ToString(x)`, `ToWString(x)` | on a non-string `Value` the old functions return `""`; `ToString` prints it as `{{ v }}` does |
| `EmptyValue` | `NoneValue` | its implicit conversion to any type is gone |
| `UserCallable::UserCallableFunctionPtr` | `UserCallable::Function` | |
| `ErrorInfoTpl<CharT>` | `BasicErrorInfo<CharT>` | Clang does not warn about a deprecated alias template, so `--rewrite` renames this one |

### Removed or renamed without a deprecation (compile errors)

*Tasks 0028, 0043, 0074, 0076.*

| 1.x | 2.0 | Script |
|---|---|---|
| `settings.m_defaultMetadataType` | `settings.defaultMetadataType` | `--rewrite` |
| `settings.extensions.Do` | `settings.extensions.doStatement` | `--rewrite` |
| `settings.useLineStatements = true` | `settings.lineStatementPrefix = "#"` | `--rewrite` (`= false` becomes `lineStatementPrefix = ""`) |
| `settings.jinja2CompatMode`, `Jinja2CompatMode` | removed: the library never read them | by hand: delete the line |
| `#include <jinja2cpp/error_handler.h>`, `IErrorHandler` | removed: an empty class used nowhere | `--rewrite` deletes the include |
| `#include <jinja2cpp/polymorphic_value.h>` | removed; `Value` holds `jinja2::detail` value types (`value_ptr.h`) | by hand |
| `#include <jinja2cpp/binding/rapid_json.h>` | removed; see [RapidJSON binding removed](#rapidjson-binding-removed) | by hand |
| `TemplateEnv::IsEqual`, `TimePoint`, `TimeStamp` | implementation details, gone | by hand |
| `ApplyGlobals(ValuesMap&)` callbacks | take `const ValuesMap&` | by hand |
| `ValuesMap` as a `std::unordered_map` | `jinja2::OrderedMap`: insertion order, no bucket API, node handles, `hash_function`, `key_eq`, `get_allocator` | by hand |
| `namespace jinja2 { class Value; }` forward declarations | `#include <jinja2cpp/fwd.h>` (everything is in the inline namespace `jinja2::v2`) | by hand |
| `namespace jinja2 { namespace detail { template<> struct Reflector<T> ... } }` | `template<> struct jinja2::detail::Reflector<T>` (MSVC does not see through the inline namespace) | by hand |

The ABI changes (inline namespace `v2`, SOVERSION 2, install layout) are in
[README.md, "Compatibility and versioning"](README.md#compatibility-and-versioning).

### `Value(float)` stores a `float`

*Task 0039.* `jinja2::Value` has a `float` alternative. `IsDouble()`, `AsDouble()` and
`GetIf<double>()` do not see a value built from a C++ `float`; code that read it back as a
`double` tests `IsFloat()` too, or converts first. No compiler warning points at this.

```cpp
jinja2::Value v(1.5f);
v.IsDouble();                     // 1.x: true; 2.0: false
v.IsFloat() ? double(v.AsFloat()) : v.AsDouble();   // 2.0: either kind
```

Values a template computes are `double`, as before.

### `Result<T>` and expected-lite

*Task 0070.* `Load` and `Render` return `jinja2::Result<T>` (`ResultW<T>` for wide
templates), which is `nonstd::expected<T, BasicErrorInfo<CharT>>` from expected-lite,
pinned. That is not a 2.0 break, but code that names the expected-lite type or uses its
extensions ties itself to expected-lite. Keep to the subset `std::expected` has, so that a
later switch of `Result<T>` to `std::expected` (once C++23 is the floor) is a recompile:

| Instead of | Write | Script |
|---|---|---|
| `nonstd::expected<T, jinja2::ErrorInfo>` | `jinja2::Result<T>` | `--rewrite` |
| `r.get_unexpected()` | `nonstd::make_unexpected(r.error())` | `--rewrite` |
| other expected-lite extensions | the `std::expected` members: `has_value()`, `value()`, `error()`, `value_or()`, `and_then()`, `transform()`, `or_else()` | by hand |

`nonstd::make_unexpected` itself stays until that switch (our own tests use it).

## Template behaviour

These need no code change but render differently. Each example is what Python Jinja2 at the
pinned version renders, which 2.0 now matches ([docs/parity.md](docs/parity.md)).

### `None` and undefined are different

*Tasks 0026, 0034.* An empty `jinja2::Value()` in the context, in globals or returned by a
callable is Python's `None`; a missing name, attribute or item is undefined.

| Template | 1.x | 2.0 |
|---|---|---|
| `{{ x }}` with `x = jinja2::Value()` | (empty) | `None` |
| `{{ x is defined }}` with `x = jinja2::Value()` | `false` | `True` |
| `{{ missing }}` | (empty) | (empty) |
| `{{ missing == none }}` | `true` | `False` |
| `{{ user.adress.city }}` (typo) | (empty) | an error, `ErrorCode::UndefinedError` |

To keep 1.x's forgiving lookups for a template, select `Settings::undefinedPolicy =
UndefinedPolicy::Chainable` (attributes of undefined stay undefined); `Strict` and `Debug`
follow Jinja2's other `Undefined` classes. Print an empty string for `None` with
`{{ x if x is not none }}` or `{{ x | default('', true) }}`.

### Values print as Python prints them

*Task 0012.*

| Template | 1.x | 2.0 |
|---|---|---|
| `{{ true }}` | `true` | `True` |
| `{{ 2.0 }}`, `{{ 0.1 + 0.2 }}` | `2`, `0.3` | `2.0`, `0.30000000000000004` |
| `{{ [1, 'a'] }}`, `{{ {'a': 1} }}` | (empty) | `[1, 'a']`, `{'a': 1}` |
| `{{ [1, 2] \| join(',') }}` with non-string items | items dropped | `1,2` |
| `{{ 10 % 3 }}` | `1.0` | `1` |

Templates that compared printed booleans with `"true"` compare the value instead
(`{% if flag %}`), and `{{ x | lower }}` gives `true` if a lower-case word is needed.
Dicts print in insertion order (see `ValuesMap` above).

### Whitespace

*Task 0024.*

| Template | 1.x | 2.0 |
|---|---|---|
| `"Hello\n"` (trailing newline) | `Hello\n` | `Hello` (set `Settings::keepTrailingNewline` to keep it) |
| `"a\r\nb"` | `a\r\nb` | `a\nb` (or `Settings::newlineSequence`) |
| `{% if true %}  \nA{% endif %}` with `trimBlocks` | `A` (spaces and newline removed) | `  \nA`: only a newline directly after the tag is removed |
| `{% if true -%}\n\n\nb{% endif %}` | stopped at the first newline | all following whitespace removed: `b` |

### `format` is printf-style only

*Task 0048.* As in Jinja2, the `format` filter is Python `%`-formatting. 1.x also accepted
`{}` placeholders; 2.0 raises "not all arguments converted" for them.

```jinja
{{ "%s-%d" | format("a", 1) }}   {# a-1, both versions #}
{{ "{}-{}" | format(1, 2) }}     {# 1.x: 1-2; 2.0: error #}
{{ "{}-{}".format(1, 2) }}       {# 2.0: 1-2, the str.format method #}
```

### Macros see the names where they are defined

*Task 0038.* A macro body, a `{% call %}` body, a recursive `loop(...)` and `super()` resolve
names in the scope that defines them, as Jinja2 does, not in the caller's. A macro that read
its caller's loop variable or a `{% set %}` from the calling template takes it as an argument:

```jinja
{# 1.x: worked by accident; 2.0: item is undefined inside row #}
{% macro row() %}{{ item }}{% endmacro %}{% for item in items %}{{ row() }}{% endfor %}
{# both #}
{% macro row(item) %}{{ item }}{% endmacro %}{% for item in items %}{{ row(item) }}{% endfor %}
```

A `{% call %}` body reads the names around the `{% call %}`, not the macro's locals; pass
values to it with `caller(value)`.

### Unknown filters and tests fail at load

*[docs/api-2.0.md](docs/api-2.0.md) section 5.6, task 0193.* `{{ x | nosuch }}` fails `Load`
with `ErrorCode::UnknownFilter`, as in Jinja2 (inside an `if` it fails when it runs). 1.x let
a `UserCallable` in the render parameters or globals act as a filter or test; register them
with `TemplateEnv::AddFilter` and `AddTest` instead, or set
`Settings::userCallablesAsFilters = true` to keep the old lookup.

## Template syntax 2.0 no longer accepts

*Tasks 0094 (decided 2026-10-12), 0294.* Jinja2 rejects each of these too; 1.x accepted them.

| 1.x | 2.0 |
|---|---|
| `{% macro m %}...{% endmacro %}` | `{% macro m() %}...{% endmacro %}` |
| `{% call m %}...{% endcall %}` | `{% call m() %}...{% endcall %}` |
| `{'a' = 1}` | `{'a': 1}` |
| `{{ x +}}` (a `+` meaning "keep whitespace" before `}}`) | `{{ x }}`: in an expression `+` is the operator. `+%}` on statements is unchanged |
| `f(a=1, 2)`, `applymacro(macro='m', x)` | positional arguments first: `f(2, a=1)`, `applymacro(x, macro='m')` |

A template with any of these fails to load with a syntax error that names the line; nothing
renders differently.

## Build and packaging

### JSON bindings are package components

*Task [0311](docs/tasks/0311-json-bindings-as-components.md), decisions B1 and 2.*

1.x picked one JSON library at configure time (`JINJA2CPP_WITH_JSON_BINDINGS`) and packaged
the library around it. In 2.0 the core package needs no JSON library, and each binding is a
header-only component that brings its own dependency:

| Component | Header | Links |
|---|---|---|
| `jinja2cpp::nlohmann_json` | `jinja2cpp/binding/nlohmann_json.h` | `nlohmann_json::nlohmann_json` |
| `jinja2cpp::boost_json` | `jinja2cpp/binding/boost_json.h` | `Boost::json` |
| `jinja2cpp::boost_multiprecision` | `jinja2cpp/big_int/boost_cpp_int.h` | `Boost::headers` |

The include paths are unchanged, and every component is installed whatever the build tested.

- **find_package users** ask for the component and link its target:

  ```cmake
  find_package(jinja2cpp 2 REQUIRED COMPONENTS nlohmann_json)
  target_link_libraries(app PRIVATE jinja2cpp::nlohmann_json)
  ```

  `jinja2cpp::jinja2cpp` alone no longer finds or links a JSON library. A component whose
  library is not found fails `REQUIRED` with a message naming the missing target; with
  `OPTIONAL_COMPONENTS` the core target is still found.
- **pkg-config users** use `jinja2cpp-nlohmann_json`, `jinja2cpp-boost_json` or
  `jinja2cpp-boost_multiprecision` instead of `jinja2cpp` (each requires `jinja2cpp`).
- **add_subdirectory and FetchContent users** link `jinja2cpp::nlohmann_json` (or another
  component) as well: the binding headers moved to `bindings/<component>/include/` and are on
  the include path only through the component. Find or fetch the JSON library before
  `add_subdirectory`, so the component can link its target.
- **`JINJA2CPP_WITH_JSON_BINDINGS`** is ignored with a warning (`rapid` is an error, below).
  `JINJA2CPP_TEST_BINDINGS` (default `nlohmann_json`, also `boost_json`) picks the bindings the
  tests check, and so which JSON libraries the build fetches.
- Your own JSON library needs no component: specialise `jinja2::JsonTraits` from
  `jinja2cpp/json_adapter.h`, as the RapidJSON entry below shows.

### RapidJSON binding removed

*Task [0303](docs/tasks/0303-retire-rapidjson-backend.md), decision D20 in
[docs/release-2.0.md](docs/release-2.0.md).*

Jinja2C++ 2.0 no longer ships `jinja2cpp/binding/rapid_json.h`, no longer fetches a patched
RapidJSON, and rejects `-DJINJA2CPP_WITH_JSON_BINDINGS=rapid` (and `rapidjson` in
`JINJA2CPP_TEST_BINDINGS`) at configure time. `{% meta %}`
blocks are read by the library's own JSON reader, so nothing in the library needs RapidJSON.

What to do instead, either:

- switch to the nlohmann binding (`#include <jinja2cpp/binding/nlohmann_json.h>`) or the
  Boost.JSON one (`jinja2cpp/binding/boost_json.h`), components of the package (above); or
- keep your own RapidJSON and add the traits below to your code. `jinja2::Reflect(value)` and
  `jinja2::ToValue(doc)` then accept `rapidjson::Value` and `rapidjson::Document` as before.
  They are written against the JSON contract in `jinja2cpp/json_adapter.h` (task 0311) and need
  an unpatched RapidJSON only (checked against commit 24b5e7a8, the one 1.x fetched).

```cpp
#include <jinja2cpp/json_adapter.h>
#include <rapidjson/document.h>

template<>
struct jinja2::JsonTraits<rapidjson::Value> : jinja2::DomJsonTraits<rapidjson::Value>
{
    static JsonKind Kind(Node n)
    {
        const auto& v = n.get();
        switch (v.GetType())
        {
        case rapidjson::kNullType: return JsonKind::Null;
        case rapidjson::kFalseType:
        case rapidjson::kTrueType: return JsonKind::Bool;
        case rapidjson::kStringType: return JsonKind::String;
        case rapidjson::kArrayType: return JsonKind::Array;
        case rapidjson::kObjectType: return JsonKind::Object;
        case rapidjson::kNumberType: return v.IsInt64() ? JsonKind::Int : v.IsUint64() ? JsonKind::UInt : JsonKind::Double;
        }
        return JsonKind::Null;
    }
    static bool AsBool(Node n) { return n.get().GetBool(); }
    static std::int64_t AsInt(Node n) { return n.get().GetInt64(); }
    static std::uint64_t AsUInt(Node n) { return n.get().GetUint64(); }
    static double AsDouble(Node n) { return n.get().GetDouble(); }
    static std::string_view AsString(Node n) { return { n.get().GetString(), n.get().GetStringLength() }; }
    static std::size_t Size(Node n) { return n.get().IsArray() ? n.get().Size() : n.get().MemberCount(); }
    static Node At(Node n, std::size_t i) { return std::cref(n.get()[static_cast<rapidjson::SizeType>(i)]); }
    static std::optional<Node> Find(Node n, std::string_view key)
    {
        auto it = n.get().FindMember(rapidjson::Value(rapidjson::StringRef(key.data(), static_cast<rapidjson::SizeType>(key.size()))));
        if (it == n.get().MemberEnd()) return std::nullopt;
        return std::cref(it->value);
    }
    template<typename F>
    static void ForEachMember(Node n, F&& f)
    {
        for (auto it = n.get().MemberBegin(); it != n.get().MemberEnd(); ++it)
            if (!f(std::string_view(it->name.GetString(), it->name.GetStringLength()), std::cref(it->value))) break;
    }
};
// A Document is a Value: let Reflect(doc) and ToValue(doc) take it directly
template<>
struct jinja2::JsonTraits<rapidjson::Document> : jinja2::JsonTraits<rapidjson::Value>
{
    static Node Root(const rapidjson::Document& d) { return std::cref<rapidjson::Value>(d); }
};
```

The 1.x binding always borrowed the document. In 2.0, `Reflect` borrows it when given a const
reference or a pointer (`Reflect(std::as_const(doc))`, `Reflect(&doc)`): keep the document alive
while templates render from the reflected value. `Reflect(std::move(doc))` hands the document
to the value, which keeps it alive. A RapidJSON value cannot be copied, so `Reflect(doc)` on a
non-const document, which borrowed in 1.x, no longer compiles: pick one of the two forms.
`ToValue(doc)` copies the document into `jinja2::ValuesMap` and `jinja2::ValuesList`.

With UTF-16 RapidJSON (`GenericValue<UTF16<>>`), specialise the traits for that type, return
`std::wstring` from `AsString` and convert keys to UTF-8.
