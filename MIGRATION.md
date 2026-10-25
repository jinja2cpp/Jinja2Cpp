# Migrating from Jinja2C++ 1.x to 2.0

This file lists the changes a 1.x user has to act on. Task 0077 collects the full list and a
migration script; each change below names the task that made it.

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
