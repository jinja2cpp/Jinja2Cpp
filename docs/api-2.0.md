# Jinja2C++ 2.0 public API: inventory and proposal

Design for task 0056 (public API review and migration path for 2.0.0). Status: **proposal,
waiting for review**. Nothing here is implemented yet; the decisions in
[section 7](#7-decisions-to-make) need an answer before the work in
[section 8](#8-work-breakdown) is filed.

Surveyed: every header in `include/jinja2cpp/` on master `b0991e3` (parity wave 6 merged),
compared with the last release `1.3.2` (June 2024). Claims marked *probe* were checked with a
small program compiled against the headers (GCC 13, `-std=c++14` unless stated); the rest
come from reading the code.

## 1. Summary

The 0056 plan assumed the next release could be a 1.4 that only adds new names and
deprecates old ones. The survey says otherwise, and that changes the shape of the plan:

1. **Master is already not 1.x-compatible.** Since 1.3.2 it changed `IListItemAccessor::CreateEnumerator`
   to return `nonstd::optional<ListEnumeratorPtr>`, replaced the vendored
   `polymorphic_value.h` (removing `make_polymorphic_value`, `MakeEmptyListEnumeratorPtr`),
   and grew `Settings`, `Settings::Extensions` and `TemplateEnv`, which are laid out inline in
   user code. Any custom list accessor stops compiling, and every user object file built
   against 1.3.2 headers is ABI-incompatible. Whatever is released from master next is a
   major version, with or without renames.
2. **The ABI is not even one ABI today.** `Value`, `Result<T>` and every API taking
   `optional`/`string_view` use nonstd types that resolve to `std::` types when the *user*
   compiles with a newer standard than the library. *Probe:* `Value::ValueData` is
   `nonstd::variants::variant<...>` at C++14 and `std::variant<...>` at C++17; a library
   built at C++14 (the default) linked into a C++17 program disagrees on these types. On top
   of that, `Value(5u)` stores **`bool`** at C++14 and `int64_t` at C++17.
3. **The naming inconsistency is narrower than it looks.** The parts users touch most
   (`Template`, `TemplateEnv`, `MakeCallable`, `Reflect`, filesystem handlers, `ErrorInfo`)
   already follow one convention: `CamelCase` functions, `camelBack` data members. The
   outliers are `Value`'s accessors (`isString`, `asString`, `get`, `getPtr`, `isEmpty`,
   `data`), three `Settings` fields and the misspelt `Jinja2CompatMode::Vesrsion_2_10`.
4. **Many names are not released yet.** `AddFilter`, `AddTester`, `InstallGettextCallables`,
   `UndefinedPolicy`, `Settings::{LoopControls, I18n, keepTrailingNewline, newlineSequence,
   autoescape, finalize, ...}` exist only on master. Renaming them before the release costs
   no user anything.

Recommendation, in one paragraph: release master as **2.0.0** (no 1.4 from master). In 2.0,
add the new names and keep the old *function and type* names next to them, marked
`[[deprecated]]` with a machine-readable message; remove the dead ones. Ship a small script
that reads those compiler warnings and rewrites the call sites, since the compiler has
already resolved which `isString` belongs to `jinja2::Value`. Put the library in an inline
namespace that carries the ABI tag, export the vocabulary-type selection with the CMake
package, and drop the deprecated names in 3.0. An optional 1.4 can be cut from the
`1.3.2` tag if real users ask for a bridge (section 6).

## 2. Conventions for the 2.0 API

One rule set, chosen to match what most of the API already does, so that the fewest
names move.

| Kind | Rule | Example |
|---|---|---|
| Types, enums, enumerators | `CamelCase` | `ValuesMap`, `ErrorCode::FileNotFound` |
| Functions and methods | `CamelCase` | `Template::Load`, `Value::IsString` |
| Standard protocols | as the standard spells them | `begin`, `end`, `cbegin`, `swap`, `value_type`, `iterator` |
| Vocabulary aliases | as `std::` spells them | `jinja2::optional`, `jinja2::string_view` |
| Public data members, parameters | `camelBack`, named after the Jinja2 option where there is one | `Settings::trimBlocks` (`trim_blocks`) |
| Private members | `m_` + `camelBack` | `m_impl` |
| Macros | `JINJA2CPP_` prefix | `JINJA2CPP_EXPORT` |
| Namespaces | `jinja2`, with `jinja2::detail` for everything users must not name | |

Verb rules, so a name says what a call does:

- `IsX()`: a predicate, never throws.
- `GetX()`: exact access to what is stored. On `Value`, throws `bad_variant_access` when the
  value holds something else; elsewhere a plain getter.
- `GetIf<T>()`: pointer or `nullptr`, as `std::get_if`.
- `AsX()`: lenient conversion, never throws, returns an empty result when there is nothing to
  convert. This is what the free `jinja2::AsString(const Value&)` already does.
- `ToX()`: formatting for humans (`ErrorInfo::ToString`).

The rule resolves the worst collision in today's API: `v.asString()` throws on a non-string,
while `jinja2::AsString(v)` converts and returns `""`. Same word, opposite contracts. In 2.0
the member becomes `GetString()`, the free function keeps `AsString`.

Errors: the API reports failures through `Result<T>` (`expected<T, ErrorInfo>`), not
exceptions. The only throwing calls are `Value::GetX()`/`Get<T>()` (documented) and
`Result::value()`.

## 3. Inventory

Each row: what is there now, what kind of problem it is, the 2.0 form, and how users
migrate. Migration kinds:
**alias** (old name kept as a `[[deprecated]]` alias or forwarding function in 2.0, removed
in 3.0; the script rewrites it), **break** (old name removed; compile error, the script or
the migration notes fix it), **fix** (a defect; behaviour changes, no rename),
**unreleased** (only on master, renamed for free), **add** (new, non-breaking).

### 3.1 `Value` (`value.h`, `string_helpers.h`)

| Now | Problem | 2.0 | Migration |
|---|---|---|---|
| `isString()`, `isWString()`, `isList()`, `isMap()`, `isEmpty()` | `camelBack` methods | `IsString()`, `IsWString()`, `IsList()`, `IsMap()`, `IsNone()` | alias |
| `asString()`, `asWString()` | `camelBack`; same word as the converting free `AsString` with the opposite contract | `GetString()`, `GetWString()` | alias |
| `asList()`, `asMap()` | `isList()` is true for a `GenericList` but `asList()` throws for it (same for maps) | `GetList()`/`GetMap()` (exact, as today) plus `AsList()`/`AsMap()` returning a `GenericList`/`GenericMap` view over either representation | alias + add |
| `get<T>()` | returns **by value**: `v.get<std::string>()` copies | `Get<T>()` returning a reference | alias (old one keeps copying) |
| `getPtr<T>()` | `camelBack` | `GetIf<T>()` | alias |
| `data()` | `camelBack`; the variant alternatives expose `RecWrapper<T>` = vendored `xyz::polymorphic<T>` to every visitor | `GetData()`; add `jinja2::Visit(fn, value)` that unwraps `RecWrapper` as `ParamUnwrapper` already does internally | alias + add |
| no `IsBool`, `IsInt`, `IsDouble`, `IsCallable` and getters | incomplete family; users go through `data()` | add them | add |
| `EmptyValue` | since 0034 it means Python `None`; it also has `template<class T> operator T()`, an implicit conversion to *anything* | `NoneValue`; drop the catch-all conversion | alias (`using EmptyValue [[deprecated]] = NoneValue`); the conversion is a break, check `src/` users first |
| `Value(T&&)` for unsigned, `long long`, `short`, `char`, `size_t` | *probe:* stores `bool` at C++14 (`Value(5u).data().index() == 1`), `int64_t` at C++17, and `Value(size_t(5))` does not compile at C++17 | constructors for every integral type storing `int64_t`, as `Reflect` already does; `Value(char)` deleted (ambiguous between a number and a one-letter string) | fix (task 0067) |
| `Value(nullptr)` | *probe:* picks `Value(const char*)` and throws `logic_error` from `std::string(nullptr)` | `Value(std::nullptr_t)` makes `None` | fix (0067) |
| `Value(ValuesList&&) noexcept`, `Value(ValuesMap&&) noexcept` | allocate, so `bad_alloc` becomes `std::terminate` | drop `noexcept` there | fix |
| `IsEqual()` next to `operator==` | redundant | keep both; not worth a break | — |
| `ValuesMap : std::unordered_map` | hash order | ordered map (task 0043) | break, owned by 0043 |
| `const std::string AsString(const std::string&)` | returns a `const` value, defeating moves | return `std::string` | fix |

### 3.2 `Template`, `TemplateW` (`template.h`)

| Now | Problem | 2.0 | Migration |
|---|---|---|---|
| two hand-copied classes `Template`/`TemplateW`, and `Result`/`ResultW` | duplication; a fix to one is easily missed in the other | `template<class CharT> class BasicTemplate`, explicitly instantiated in the library; `using Template = BasicTemplate<char>`, `using TemplateW = BasicTemplate<wchar_t>`; `Result<T, CharT = char>` with `ResultW<T>` kept | none: the old names stay as the aliases |
| `Load(const char*, std::string)`, `Load(const std::string&, std::string)` | two overloads for one job | `Load(basic_string_view<CharT> source, std::string name = {})` | none (both call forms still compile) |
| `Render`, `RenderAsString`, `GetMetadata`, `GetMetadataRaw` are non-`const` | rendering does not change the template; non-`const` hides whether concurrent renders are safe | `const`, once a TSan run shows concurrent rendering of one template is safe; until then, document that it is not | fix |
| context is `const ValuesMap&` only | a reflected struct or a JSON object cannot be the context | add `RenderAsString(const GenericMap&)` and `Render(os, const GenericMap&)` | add (2.x) |
| `Template(TemplateEnv*)` | raw pointer; destroying the environment first leaves a dangling pointer | keep the constructor; with a pimpl'd environment (3.3) the template holds a shared reference to the environment's state | none |
| `MetadataInfo::metadata` is a `string_view` into the template | lifetime not documented | document | — |
| `ITemplateImpl`, `TemplateEnv` forward-declared with `JINJA2CPP_EXPORT` | attribute on a forward declaration | drop | — |

### 3.3 `TemplateEnv`, `Settings` (`template_env.h`)

| Now | Problem | 2.0 | Migration |
|---|---|---|---|
| everything inline: mutex, three callable maps, two caches, `FsHandler` | the whole private layout is ABI; every new member (filters, tests, translations since 1.3.2) breaks it | pimpl: `std::shared_ptr<detail::TemplateEnvImpl>`, methods out of line and exported | none at source level |
| `AddTester`, `RemoveTester`, `FindTester` | Jinja2 calls them *tests* (`env.tests`); "tester" is our internal word | `AddTest`, `RemoveTest`, `FindTest` | unreleased |
| `ApplyGlobals(fn)` passes `ValuesMap&` under a **shared** lock | *probe:* a mutating callback compiles; two concurrent callers race | pass `const ValuesMap&` | fix (task 0069) |
| `Settings& GetSettings()` | mutable reference, no lock | keep, and document Jinja2's own rule: configure the environment before loading templates; changes afterwards are not synchronised | — |
| `AddFilesystemHandler(prefix, IFilesystemHandler&)` | non-owning; lifetime is the caller's | keep, document | — |
| `LoadTemplate` returns `nonstd::expected<Template, ErrorInfo>` | spelled out instead of `Result<Template>` | `Result<Template>` (same type) | none |
| `IsEqual`, `TimePoint`, `TimeStamp` public | implementation details | move into the impl | break (unlikely to be used) |
| no `FromString` | Jinja2's `env.from_string` | add `FromString(source, name)` | add |
| `Settings::m_defaultMetadataType` | private-member prefix on a public field | `defaultMetadataType` | break (one line; script) |
| `Settings::Extensions::Do` | `CamelCase` field (`do` is a keyword) | `doStatement` | break (script) |
| `Settings::Extensions::LoopControls`, `I18n` | `CamelCase` fields | `loopControls`, `i18n` | unreleased |
| `Settings::useLineStatements` | superseded by `lineStatementPrefix = "#"` (0028) | remove | break |
| `Settings::jinja2CompatMode`, `enum Jinja2CompatMode { None, Vesrsion_2_10 }` | misspelt, and **never read** by the library | remove | break |
| `operator==(Settings, Settings)` lists every field by hand | a new field that is not added there compares equal silently | out of line, next to a `static_assert` on the field count or a test that fails when the struct grows | fix |
| every new `Settings` field changes `sizeof(Settings)` | parity work keeps adding options, each an ABI break | accepted: see SOVERSION policy in 5.4 | — |

### 3.4 Errors (`error_info.h`, `error_handler.h`)

| Now | Problem | 2.0 | Migration |
|---|---|---|---|
| `ErrorInfoTpl<CharT>` | odd suffix; the rest of the API would use `Basic*` | `BasicErrorInfo<CharT>`; `ErrorInfo`, `ErrorInfoW` unchanged | alias |
| `ErrorCode` numbered implicitly after `UnexpectedException = 1` and `ExpectedStringLiteral = 1001` | users log and persist codes; inserting an enumerator in the middle renumbers the rest | explicit value on every enumerator, append-only | none |
| doc of `MetadataParseError` copies `InvalidTemplateName`'s | wrong comment | fix | — |
| `IErrorHandler` (`error_handler.h`) | empty class, used nowhere; the header's closing comment names another file | remove the header | break |

### 3.5 User callables (`value.h`, `user_callable.h`)

| Now | Problem | 2.0 | Migration |
|---|---|---|---|
| `UserCallable::UserCallableFunctionPtr` | a `std::function`, not a pointer | `UserCallable::Function` | alias |
| `UserCallableParams::paramsParsed` | engine-internal flag in a public struct | keep (removing it is churn for nothing), mark internal in the docs | — |
| `ArgInfo` names `"*args"`, `"**kwargs"`, `"*context"` | magic strings | named constants `ArgInfo::VarArgs`, `VarKwArgs`, `Context` | add |
| `ArgInfo(std::string name, bool isMandat, Value defVal)` | parameter spelt `isMandat` | `isMandatory` | none (parameter name) |

### 3.6 Generic containers and reflection

| Now | Problem | 2.0 | Migration |
|---|---|---|---|
| `generic_list_impl.h` defines `lists_impl::MakeGeneratedList` and `MakeGenericList(ListGenerator)` without `inline` | *probe:* two translation units including it fail to link (multiple definition); the header with `MakeGenericList` is named `_impl` | `inline`; move `MakeGenericList` to `generic_list.h` (or `make_generic_list.h`), keep `_impl` including it; `lists_impl` becomes `detail` | fix (0069) |
| `MakeGenericList(b, e)` with named iterators | *probe:* does not compile (`iterator_traits<It&>`), only temporaries work | decay the iterator types | fix (0069) |
| `GenericList::cbegin()`, `cend()` declared `auto`, defined in the `.cpp` | *probe:* unusable from user code ("use before deduction of auto") | declare the return type | fix (0069) |
| `GenericMap::GetAccessor()` | *probe:* default-constructed map throws `bad_function_call` (the list version checks) | check, as the list does | fix (0069) |
| `GenericMap` not exported, `GenericList` exported; no iteration over `GenericMap` | asymmetric | export; add key iteration | fix + add |
| `detail::GenericListIterator` returned from public `begin()` | public type in `detail` | `GenericList::iterator` alias | add |
| `IMapItemAccessor : IComparable` vs list interfaces `: virtual IComparable` | inconsistent inheritance | `virtual` everywhere | break for implementers only in theory (compiles unchanged) |
| `IComparable::IsEqual` pure virtual | every user accessor and filesystem handler writes `dynamic_cast` boilerplate | default implementation (identity, `this == &other`) | none (overrides still compile) |
| `IMapItemAccessor::GetSize()` returns `size_t` ("max means unknown"); lists return `optional<size_t>` | inconsistent | keep `size_t`: changing a pure virtual signature breaks every implementer for little gain | — |
| commented-out `IMapItemAccessor::IsEqual` | dead code | remove | — |
| JSON bindings specialise `detail::Reflector` | the extension point for non-struct types lives in `detail`, while structs use public `TypeReflection` | public `jinja2::Reflector<T>`; `detail::Reflector` kept as alias | add |
| `ReflectedMapImpl::GetAccessors()` returns `auto` | copies the whole accessor `unordered_map` on **every** field access, `HasValue` and `GetKeys` | return a reference | fix (0069) |
| `ReflectedMapImplBase::GetValueByName` throws `runtime_error` for an unknown field | the interface says it returns an empty value | return `Value()` | fix (0069) |
| `Reflect(5LL)` | *probe:* does not compile on LP64 (`int64_t` is `long`; no `Reflector<long long>`) | reflect every integral type | fix (0067) |
| `Reflect(const T&)` borrows (stores a pointer), `Reflect(T&&)` owns | surprising, but changing it silently copies or stops compiling for non-copyable types | keep; document; add `ReflectRef`/`ReflectCopy` if asked | — |
| `JINJA2_INT_REFLECTOR` macro | wrong prefix, leaks from the header | `#undef` after use | fix |

### 3.7 Filesystem handlers (`filesystem_handler.h`)

| Now | Problem | 2.0 | Migration |
|---|---|---|---|
| `RealFileSystem::OpenByteStream` doc | copied from `GetLastModificationDate` | fix the comment | — |
| `MemoryFileSystem::m_filesMap` is `mutable`, `AddFile` unsynchronised | adding files while templates load races | document (same rule as `Settings`) | — |
| `IFilesystemHandler` vs Jinja2 "loader" | different word | keep: renaming every handler implementation buys nothing | — |

### 3.8 Headers, macros, ABI (`config.h`, `value_ptr.h`, CMake package)

| Now | Problem | 2.0 | Migration |
|---|---|---|---|
| `JINJA2CPP_VERSION 10100` | the project is 1.3.2: the macro is stale | generate from `project(VERSION)` | fix (0069) |
| `#pragma warning(disable : 4251)` in `config.h` | disables the warning for the rest of every user TU | `push`/`pop` around our declarations | fix (0069) |
| `JINJA2_DECLSPEC` | wrong prefix | `JINJA2CPP_DECLSPEC` | break (internal macro) |
| `value_ptr.h` includes `polymorphic_cxx14.h`, then `polymorphic.h` under `__cplusplus != 201402L` | both share one include guard, so the second include is dead; `using namespace xyz;` inside `jinja2::types` | include one; drop the `using` | fix (0069) |
| vendored `xyz::polymorphic` in the global `xyz` namespace | collides with a user's own copy of the reference implementation | move into `jinja2::detail` | none |
| nonstd `optional`/`variant`/`string_view`/`expected` select `std::` by the **consumer's** standard | library and user disagree on `Value`, `Result`, virtual signatures; silent ODR violation | export the selection the library was built with (`*_CONFIG_SELECT_*` as `INTERFACE` definitions of the installed target) and name the types through `jinja2::optional` etc. | fix (0068) |
| installed `jinja2cpp-config.cmake` | hand-written, always `STATIC IMPORTED`, no namespaced target, sets `PUBLIC` definitions on an imported target | generated export with `jinja2cpp::jinja2cpp` | fix (0068) |
| no umbrella or forward header | users include five headers; inline namespace (5.3) breaks user forward declarations | `jinja2cpp/jinja2cpp.h`, `jinja2cpp/fwd.h` | add |
| `error_handler.h` | see 3.4 | remove | break |

## 4. The 2.0 `Value`, in code

The shape of the largest change, to judge the result rather than the rows:

```c++
namespace jinja2 {
inline namespace JINJA2CPP_ABI_NAMESPACE {   // v2, see 5.3

struct NoneValue {};
using EmptyValue JINJA2CPP_DEPRECATED("jinja2cpp-2: NoneValue") = NoneValue;

class Value
{
public:
    using ValueData = variant<NoneValue, bool, std::string, std::wstring, string_view, wstring_view,
                              int64_t, double, RecWrapper<ValuesList>, RecWrapper<ValuesMap>,
                              GenericList, GenericMap, RecWrapper<UserCallable>>;

    Value() noexcept;
    Value(std::nullptr_t) noexcept;                         // None
    template<class I, std::enable_if_t<detail::IsStoredAsInt<I>::value>* = nullptr>
    Value(I val) noexcept : m_data(static_cast<int64_t>(val)) {}
    Value(char) = delete;                                   // number or one-letter string? say which
    // ... string, double, list, map, callable constructors as today

    bool IsNone() const noexcept;
    bool IsBool() const noexcept;
    bool IsInt() const noexcept;
    bool IsDouble() const noexcept;
    bool IsString() const noexcept;
    bool IsWString() const noexcept;
    bool IsList() const noexcept;      // ValuesList or GenericList
    bool IsMap() const noexcept;       // ValuesMap or GenericMap
    bool IsCallable() const noexcept;

    const std::string& GetString() const;   // throws bad_variant_access
    std::string& GetString();
    const ValuesList& GetList() const;      // the ValuesList alternative only
    GenericList AsList() const;             // a view over either list representation
    GenericMap AsMap() const;
    // GetBool, GetInt, GetDouble, GetWString, GetMap, GetCallable likewise

    template<class T> const T& Get() const; template<class T> T& Get();
    template<class T> const T* GetIf() const noexcept; template<class T> T* GetIf() noexcept;
    const ValueData& GetData() const noexcept; ValueData& GetData() noexcept;

    // 1.x names, removed in 3.0
    JINJA2CPP_DEPRECATED("jinja2cpp-2: IsString") bool isString() const { return IsString(); }
    JINJA2CPP_DEPRECATED("jinja2cpp-2: GetString") auto& asString() { return GetString(); }
    template<class T>
    JINJA2CPP_DEPRECATED("jinja2cpp-2: Get") auto get() const { return Get<T>(); }   // still copies
    // ...
};

template<class Fn> decltype(auto) Visit(Fn&& fn, const Value& v);   // unwraps RecWrapper

}}
```

`JINJA2CPP_DEPRECATED(msg)` expands to `[[deprecated(msg)]]`, or to nothing when the user
defines `JINJA2CPP_NO_DEPRECATION_WARNINGS` (upgrade now, migrate later).

## 5. Migration mechanics

The analogy: renaming streets in a city. The old signs stay up for a while with a plate
saying "now called X" (deprecated aliases), and the taxi drivers get a map update that
knows both (the rewrite script). Streets that led nowhere (`IErrorHandler`,
`jinja2CompatMode`) are simply closed, with a note at the entrance (migration notes).

### 5.1 Deprecated in place instead of an opt-in compat header

0056 proposed `jinja2cpp/compat/v1.h`, opt-in, holding the old names. Old *member*
functions cannot be added to a class from another header, so the compat header would have
to switch them on with a macro inside `value.h`. Then a program where one file includes the
compat header and another does not has two different definitions of `class Value`: a real
ODR violation, harmless in practice only as long as the extra members stay inline and
non-virtual. Keeping the old names in the main headers, always, marked deprecated, has
none of that, needs no opt-in, and the warning points at every call site. The cost is a
noisier class definition for one major version.

What cannot be kept as an alias (renamed data members of `Settings`, removed fields and
types, the `ApplyGlobals` signature) is a compile error with a migration note.

### 5.2 The rewrite script: the compiler is the type checker

A rename tool must know that `x.isString()` is a call on `jinja2::Value` and not on some
other class with the same method. A clang-tidy plugin or a libTooling tool knows that, but
the user would have to build it against their exact Clang version. The deprecation
warnings already carry the answer: the compiler resolved the call and printed where it is.
*Probe* (GCC 13, Clang 18): every call site of a deprecated member produces one warning
with `file:line:col` and the message, including calls inside templates (once per
instantiation; deduplicated by location).

`scripts/jinja2cpp_migrate.py`:

1. The user builds once with `-Wdeprecated-declarations` and saves the log (any compiler:
   GCC, Clang and MSVC formats are parsed).
2. The script keeps warnings whose message starts with `jinja2cpp-2:`, takes the new name
   from the message, and replaces the old identifier at that location. Clang's column
   points at the identifier, GCC's at the call's `(`, so the script searches back from the
   column for the old name on that line.
3. A small table handles what produces errors instead of warnings: the renamed `Settings`
   fields (`m_defaultMetadataType`, `extensions.Do`), `useLineStatements = true` →
   `lineStatementPrefix = "#"`, removed headers.
4. It prints what it changed and what it left for a human (macros, generated code).

No Clang libraries, no compile database, and it works on MSVC-only projects. The same
script migrates our own tests and docs, which is how it gets tested.

### 5.3 Inline namespace as the ABI tag

```c++
namespace jinja2 { inline namespace v2 { ... } }
```

User code still writes `jinja2::Value`, but the mangled name becomes `jinja2::v2::Value`,
so headers of one major version linked against the library of another fail at link time
instead of misbehaving. SOVERSION protects shared libraries only; the inline namespace
also protects static linking and mixed installs. Cost: a user's own forward declaration
`namespace jinja2 { class Value; }` declares a different class. `jinja2cpp/fwd.h` is the
supported way; the migration notes say so.

The namespace name can also carry the vocabulary-type selection (`v2` vs `v2_std`), so a
consumer that somehow ends up with the other selection gets a link error, not an ODR
violation. Exporting the selection with the package (0068) is the primary fix; the tag is
the safety net.

### 5.4 SOVERSION policy

SOVERSION goes to 2 with this release. After that, it tracks the ABI, not the major
version: a minor release that adds a `Settings` field bumps it, because user code
allocates `Settings`. Parity work adds options regularly, so the alternative (freezing
`Settings` or hiding it behind setters) costs more than a SOVERSION number.

## 6. Release options

| Option | What ships | Users get | Cost |
|---|---|---|---|
| **A (recommended)** | master as **2.0.0**: new names, deprecated old ones, removals, ordered `ValuesMap` (0043), inline namespace | one upgrade with compiler-guided migration and a script | none beyond the 2.0 work |
| B | master as 1.4.0 with new names and deprecations, then 2.0 removes | two breaking upgrades in a row: 1.4 already breaks source (custom list accessors) and ABI | a release that lies about being minor |
| C | A, plus a 1.4.0 cut from the `1.3.2` tag that only adds the new `Value` names as inline aliases and deprecates the old ones | users stuck on 1.x can migrate their `Value` calls early | a release branch with its own CI; `Settings` renames cannot be bridged there |

C is cheap in code (the `Value` accessors are header-only) and expensive in process.
Recommend A now, C only if someone asks.

## 7. Decisions to make

1. **Release as 2.0.0 directly** (option A) rather than a 1.4 from master. *Recommended.*
2. **Deprecate in place** (5.1) instead of an opt-in `compat/v1.h`. *Recommended.*
3. **Value accessor verbs**: `Is` / `Get` (throws) / `GetIf` / `As` (lenient), as in section 2.
   Alternative: keep `As` for the throwing members (`v.AsString()`) and rename the free
   lenient function to `ToString`; rejected because the free `AsString` is the documented
   way and is used in examples. *Recommended: Is/Get/GetIf/As.*
4. **C++ standard floor** (task 0008). 2.0 is the one window to raise it. At C++17 the API
   could use `std::optional`, `std::variant`, `std::string_view` directly, leaving only
   `expected` from nonstd (and the 0068 fix still needed for C++23 consumers, whose
   `expected` resolves to `std::expected`). This design works either way: the API names the
   types through `jinja2::optional` etc. Needs data on C++14 users; not decided here.
5. **Renames of names not yet released**: `AddTester`→`AddTest` (and `Remove`/`Find`),
   `LoopControls`→`loopControls`, `I18n`→`i18n`. *Recommended:* yes, before the release.
6. **`Value(char)`**: delete it (force the user to say `Value(int64_t('c'))` or
   `Value(std::string(1, 'c'))`), or store a one-letter string? *Recommended:* delete.
   Today it silently stores `true`.

## 8. Work breakdown

Filed as tasks once section 7 is agreed. `touches` are listed so `scripts/task_batches.py`
can wave them.

| # | Work | touches | Notes |
|---|---|---|---|
| a | `Value` accessors, `NoneValue`, integral constructors, `Visit` | `include/jinja2cpp/value.h`, `string_helpers.h` | builds on 0067; then `src/` and `test/` use the new names |
| b | `BasicTemplate<CharT>`, `Result<T, CharT>`, `Load(string_view)` | `include/jinja2cpp/template.h`, `src/template.cpp` | |
| c | `TemplateEnv` pimpl, `Settings` renames and removals, `AddTest`, `FromString` | `include/jinja2cpp/template_env.h`, `src/template_env.cpp`, `src/template_impl.h` | |
| d | containers and reflection: public `Reflector`, `GenericMap` export and iteration, `IComparable` default, `BasicErrorInfo` | `generic_list*.h`, `reflected_value.h`, `binding/*.h`, `error_info.h` | builds on 0069 |
| e | inline namespace, `fwd.h`, umbrella header, `config.h`, vendored `polymorphic` into `detail`, remove `error_handler.h`, SOVERSION 2, version 2.0.0 | every public header | touches every header's namespace line: run alone, after a-d |
| f | `scripts/jinja2cpp_migrate.py`, `MIGRATION.md`, docs site, README | `scripts/`, docs | migrates our own tests as its test |
| g | ordered `ValuesMap` | per 0043 | exists |
| h | `readability-identifier-naming` for `include/` | `.clang-tidy` | 0054/0065 follow-up |

a, b, c and d touch disjoint headers and can run side by side; e and f go after them.
The 1.x-safe fixes (0067, 0068, 0069) can land now, before any of it.

## 9. Findings filed separately

Defects found during the survey that are worth fixing regardless of the 2.0 design:

- [0067](tasks/0067-value-integral-construction.md): `Value` from unsigned, `long long`,
  `short`, `char` stores `bool` at C++14; `Value(nullptr)` throws; `Reflect(5LL)` does not
  compile on LP64.
- [0068](tasks/0068-package-abi-facts.md): the installed package does not carry the
  vocabulary-type selection or the link type the library was built with.
- [0069](tasks/0069-public-header-defects.md): smaller defects in public headers
  (multiple definitions in `generic_list_impl.h`, `MakeGenericList` with named iterators,
  `cbegin`, `GenericMap::GetAccessor`, `ApplyGlobals`, reflected field lookup copying the
  accessor map, stale `JINJA2CPP_VERSION`, leaked pragma, `value_ptr.h`).

## 10. Considered and not proposed

- **Renaming `IFilesystemHandler` to "Loader"** to match Jinja2: every user handler
  changes, nothing gets clearer.
- **Making `Reflect(const T&)` copy**: a silent semantic change that compiles; borrowing
  stays, documented.
- **Changing `IMapItemAccessor::GetSize` to `optional`**: breaks every implementer for
  symmetry alone.
- **Hiding `Settings` behind setters** to keep its layout stable: loses the plain-struct
  style users and tests rely on; SOVERSION bumps are cheaper (5.4).
- **Exceptions-based API**: `Result<T>::value()` already throws for those who want it.
