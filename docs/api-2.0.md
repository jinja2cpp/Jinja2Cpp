# Jinja2C++ 2.0 public API: inventory and proposal

Design for task 0056 (public API review and migration path for 2.0.0). Status: **agreed with Ruslan 2026-10-02** (decisions in
[section 7](#7-decisions-to-make)); implementation is tracked by tasks 0070-0077
([section 8](#8-work-breakdown)).

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
   With the C++17 floor (decision 4) 2.0 uses `std::optional`, `std::variant` and
   `std::string_view`, and pins expected-lite to its own `nonstd::expected`, which removes the
   mismatch; 0068 fixes the package.
3. **The naming inconsistency is narrower than it looks.** The parts users touch most
   (`Template`, `TemplateEnv`, `MakeCallable`, `Reflect`, filesystem handlers, `ErrorInfo`)
   already follow one convention: `CamelCase` functions, `camelBack` data members. The
   outliers are `Value`'s accessors (`isString`, `asString`, `get`, `getPtr`, `isEmpty`,
   `data`), three `Settings` fields and the misspelt `Jinja2CompatMode::Vesrsion_2_10`.
   The original API mixes three styles: Qt's `isSomething`/`asSomething`, the standard
   library's `snake_case` (`get`, `data`, `begin`) and the dominant Microsoft-style
   `CamelCase`. Dropping the Qt style leaves one style plus the standard protocols, which is
   less for a user to keep in mind (Ruslan's review, 2026-10-02).
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
| Vocabulary types | the `std::` ones directly (C++17 floor, decision 4); `expected` from expected-lite, pinned | `std::optional`, `std::string_view`, `jinja2::Result<T>` |
| Public data members, parameters | `camelBack`, named after the Jinja2 option where there is one | `Settings::trimBlocks` (`trim_blocks`) |
| Private members | `m_` + `camelBack` | `m_impl` |
| Macros | `JINJA2CPP_` prefix | `JINJA2CPP_EXPORT` |
| Namespaces | `jinja2`, with `jinja2::detail` for everything users must not name | |

Verb rules, so a name says what a call does:

- `IsX()`: a predicate, never throws.
- `AsX()` / `As<T>()`: exact access to what is stored, by reference. Throws
  `bad_variant_access` when the value holds something else. For code that has already
  checked the type, or where a wrong type is a bug.
- Non-throwing access, side by side with `AsX()` so users pick whichever makes their code
  simpler (Ruslan, 2026-10-02, decision 3c):
  - `bool AsX(out) const noexcept`: an overload of the throwing accessor that returns
    whether the value holds that type and, if so, stores it in `out`. `out` never receives a
    copy of anything large: a `std::string_view` (`std::wstring_view`) into the stored string,
    a plain value for `bool`, `int64_t` and `double`, and a pointer for lists, maps and
    callables. Nothing allocates, so it is `noexcept`:
    `std::string_view s; if (v.AsString(s)) use(s);`, `int64_t n; if (v.AsInt(n)) ...`.
    The view lives as long as the `Value` is unchanged, as any view does;
  - `GetIf<T>()`: a pointer to the stored object, or `nullptr`, as `std::get_if`, for
    generic code (`if (auto* s = v.GetIf<std::string>()) use(*s);`).
- `ToX()`: conversion that always succeeds: `v.ToString()` and `v.ToWString()` as members,
  the same as free functions `jinja2::ToString(v)`, and `ErrorInfo::ToString()`. `ToString`
  is the common spelling for this across C++ libraries and other languages, so it replaces
  today's free `jinja2::AsString(const Value&)`. Non-allocating forms are in 3.1.
- `GetX()`: plain getters on everything other than `Value` (`Template::GetMetadata`,
  `GenericList::GetSize`), as today.

The rule resolves the worst collision in today's API: `v.asString()` throws on a non-string,
while `jinja2::AsString(v)` converts and returns `""`. Same word, opposite contracts. In 2.0
the member becomes `AsString()` (throws) with `AsString(std::string_view&)` (`noexcept`)
beside it, and the free function becomes `ToString()`.

`ToString(v)` prints the value the way `{{ v }}` prints it, Python `str()`: `ToString(5)` is
`"5"`, then `True`, `None`, `[1, 2]` (decided by Ruslan, 2026-10-02). The library already has
the renderer. The deprecated free `AsString` keeps returning `""` for non-strings, so no
existing call changes meaning.

`jinja2::Result<T>` is the only public spelling of the result type (decided by Ruslan
2026-10-02): no public signature names `nonstd::expected` or `std::expected`, so the backing
library can change (to `std::expected` once the floor is C++23) without touching user code that
spells `Result`. How errors travel is §2.1.

### 2.1 Errors and exceptions (task 0250, decided by Ruslan 2026-10-18)

The engine throws internally; the public boundary converts. Plan and alternatives:
docs/design/0250-error-contract-plan.md. Implementation: 0253 (types and boundary), 0254 (render
locations), 0255 (engine runtime errors).

**Rules.**
1. `Load*`, `Render*`, `GetMetadata*` and `TemplateEnv::LoadTemplate*` report every failure
   through `Result<T>`: template errors, exceptions from host callables, reflectors, accessors
   and filesystem handlers, and throws of non-`std::exception` types.
2. **Only `std::bad_alloc` escapes them**, and only as "the system is out of memory" (the one
   other exception is rule 3's: a failure of a caller-supplied stream whose exception mask the
   caller enabled). Every
   limit (`Settings::Limits`, 0262; the built-in caps; `std::length_error`) and every budget a
   host enforces is `ErrorCode::ResourceLimitExceeded` in the `Result`; a host allocator or
   memory resource reports a spent budget by throwing `jinja2::Exception{ResourceLimitExceeded}`,
   which does not derive from `bad_alloc`. A failed Load keeps the previous template; a failed
   Render leaves `Template` and `TemplateEnv` unchanged. These calls are not marked `noexcept`
   (that would turn `bad_alloc` into `std::terminate`); the guarantee is documented and tested
   by allocation-failure injection.
3. What a caller-supplied stream received before a failure is unspecified. Exceptions from a
   host stream whose exception mask the host enabled propagate.
4. Outside Load/Render, calls that run host code (`GenericList`/`GenericMap` access from the
   host, `ToString` of a reflected container) let the host's exceptions through unchanged.
5. **Every other throwing call has a non-throwing sibling** (`bool X(out) noexcept`, §2):

   | Throws | Non-throwing sibling |
   |---|---|
   | `Value::AsX()`, `As<T>()`: `std::bad_variant_access` | `bool AsX(out) noexcept`, `GetIf<T>()` |
   | `OrderedMap::at`: `std::out_of_range` | `find` |
   | `Result::value()`: `jinja2::BadResultAccess<CharT>` | `has_value()`, `operator*` |

6. `noexcept`: `Value` moves, `IsX`, `bool AsX(out)`, `GetIf`, `ToChars`, `Value(nullptr)`,
   `Value(I)` for every `I` that fits int64 (wider types may allocate a `BigInt`), `Value(F)`; `BasicErrorInfo` copy, move and
   getters; `OrderedMap` moves, iterators, `size`, `empty`, `clear`, `swap`; `ReleaseThreadCaches()`
   (0275); destructors.
   Host-supplied destructors, `IComparable::IsEqual` and filesystem-handler destructors must
   not throw.
7. C++ exceptions are required: building without them is an `#error` in `config.h`.
8. A render error carries a specific `ErrorCode`, the `file:line:col` of the innermost
   statement or `{{ }}` tag that was running (in the template that owns it), a caret line in
   `GetLocationDescr()`, and `GetException()`: the original `std::exception_ptr`, set if and
   only if the failure began as a C++ exception thrown outside the template engine: by host
   code or by the standard library. Errors the engine raises while evaluating the template
   (undefined values, type errors, limits, recursion, a `BigInt` it cannot hold yet) have none.

**Types.**
- `jinja2::Exception : std::runtime_error` (`exception.h`, in `fwd.h` and `jinja2cpp.h`) with
  `ErrorCode Code() const noexcept` and the C++ origin (`__builtin_FILE()`/`__builtin_LINE()`
  default arguments; no `std::source_location`, so C++17 and C++20 builds share one layout).
  Thrown by hosts to report a specific code, which then passes through the render boundary with
  its location. (`Value(I)` once threw it for an integer outside int64; with `BigInt` (0269) it
  stores the value instead, so `Value::TryFrom` was dropped before release.)
- `BasicErrorInfo` is one pointer to an immutable shared payload (code, locations, parameters,
  description, `exception_ptr`): copies share it. The payload's exception object lives in the
  module that threw it; an `ErrorInfo` must not outlive that module.
- `ErrorCode` is a `std::error_code` enum: `jinja2::error_category()`, so
  `std::error_code ec = ErrorCode::TemplateNotFound;` works and compares equal to
  `std::errc::no_such_file_or_directory`.
- New codes, appended: 15 `UserCallableException` (anything thrown out of a `UserCallable`,
  filter, test, finalize or gettext callable), 16 `IntegerOutOfRange` (a `BigInt` reaching a
  template before 0270; int64 overflow in template arithmetic), 17 `TemplateRuntimeError` (the template's own
  operation failed: Python's `TypeError`, `ValueError`, ...), 18 `ResourceLimitExceeded`
  (`extraParams[1]` names the limit), 19 `TemplateExpired` (a macro of a `Template::MakeModule`
  module called after its template was loaded again; 0284. A Load no longer raises it since 0321:
  `Load` replaces the template for its own object only, and a module keeps its tree, reversing
  the second half of 0284 D1a. 0301 raises it for a stored macro of a module that ended with an
  earlier call; 0300 for a template callable lent to a host's call and called after it), 20 `DataReadError` (`ToValue` could not
  read a JSON document: its `JsonTraits` reported an error; 0311), 21 `SecurityError` (a sandbox refused an attribute or a
  list or dict method: `ISandboxPolicy`, `Settings::immutableContainers`; 0357 B7, until then an `UndefinedError` with a
  marker in `extraParams[1]`). `UnexpectedException` (1) now means "an exception the
  engine did not raise and could not attribute", with location and `GetException()`.

**Against Python.** Same: syntax, undefined and not-found errors keep their codes, with
location; `MemoryError` propagates as `bad_alloc` does. Different: errors arrive as `Result`,
not raised; host exceptions are wrapped, not propagated (`std::rethrow_exception(e.GetException())`
restores Python's behaviour); Python's runtime exception classes share one code.

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
| `asString()`, `asWString()` | `camelBack`; same word as the converting free `AsString` with the opposite contract | `AsString()`, `AsWString()` (throw), plus `bool AsString(std::string_view&)`, `bool AsWString(std::wstring_view&)` (`noexcept`, no copy) | alias + add |
| `asList()`, `asMap()` | `isList()` is true for a `GenericList` but `asList()` throws for it (same for maps) | `AsList()`/`AsMap()` (the `ValuesList`/`ValuesMap` alternative only, as today) plus `ToGenericList()`/`ToGenericMap()` returning a view over either representation, empty for a non-list | alias + add |
| `get<T>()` | returns **by value**: `v.get<std::string>()` copies | `As<T>()` returning a reference | alias (old one keeps copying) |
| `getPtr<T>()` | `camelBack` | `GetIf<T>()` | alias |
| free `AsString(const Value&)`, `AsWString(const Value&)` | lenient conversion under the word the members use for exact access; returns `""` for non-strings | member and free `ToString()`/`ToWString()`, printing as `{{ v }}` does | alias (keeps returning `""`) |
| no way to print a `Value` without allocating | | `v.ToString(std::string& out)` appends to the caller's buffer (no allocation once it has capacity); `v.ToChars(first, last)` writes into a fixed buffer and returns `std::to_chars_result`, as `std::to_chars`; `std::formatter<jinja2::Value>` for `std::format_to`/`format_to_n` where the standard library has `<format>`. All four go through one internal output sink, so they cannot print differently. *As built (0072):* scalars, None and narrow strings print with no allocation; wide strings, containers and (for `ToChars`) text past the small-string buffer still allocate inside the renderer | add |
| no way to pass safe HTML from C++ | a host string is always escaped under autoescape, and a callable's Markup result loses the flag in `IntValue2Value` | `jinja2::Markup` / `WMarkup` (`markup.h`): `BasicMarkup<CharT>`, an immutable wrapper of a `std::basic_string<CharT>` (composition, not inheritance) with explicit constructors, a read-only string API (`str()`, implicit conversions to `const std::basic_string&` and `std::basic_string_view`, comparisons, `operator<<`) and no mutating member, two more `ValueData` alternatives (13, 14; `sizeof(Value)` unchanged). `IsMarkup()`/`IsWMarkup()`, `AsMarkup()`/`AsWMarkup()` (both forms) and `As<Markup>()`/`GetIf<Markup>()` are exact. **A Markup reads as a string** (Python's `Markup` is a `str`): the const `IsString()`, `AsString()`, `As<std::string>()` and `GetIf<std::string>()` accept it, so existing readers keep working. **A Markup is immutable, as in Python** (`markup += user_input` would append unescaped text and keep the safe flag, so it does not compile): mutable access to the text through a non-const `Value` (`AsString()`, `AsWString()`, `As<std::string>()`, `GetIf<std::string>()`, the deprecated `getPtr`) turns the Value into a plain string first, as any change to a Python Markup gives a `str`. Only `GetData()` with `std::get` or `std::holds_alternative` is exact. Template side: a Markup in the context, in a container or returned by a callable renders unescaped, passes `is escaped`, and comes back from `IntValue2Value` (callable arguments, `UserCallableParams`) as a Markup; a typed `MakeCallable` parameter (`const std::string&`) receives its text, and a `Markup` parameter takes a Markup as is and a plain string escaped. `jinja2::Escape(std::string_view)` / `Escape(std::wstring_view)` is `markupsafe.escape` (`& < > ' "`; a Markup argument is returned as is); there is no `Markup + str` operator, the host escapes explicitly: `Markup(m.str() + Escape(s).str())` (0277) | add (0180, 0277) |
| `data()` | `camelBack`; the variant alternatives expose `RecWrapper<T>` = vendored `indirect<T>` to every visitor | `GetData()`; add `jinja2::Visit(fn, value)` that unwraps `RecWrapper` as `ParamUnwrapper` already does internally | alias + add |
| no `IsBool`, `IsInt`, `IsDouble`, `IsCallable` and getters | incomplete family; users go through `data()` | add them | add |
| `EmptyValue` | since 0034 it means Python `None`; it also has `template<class T> operator T()`, an implicit conversion to *anything* | `NoneValue`; drop the catch-all conversion | alias (`using EmptyValue [[deprecated]] = NoneValue`); the conversion is a break, check `src/` users first |
| `Value(T&&)` for unsigned, `long long`, `short`, `char`, `size_t` | *probe:* stores `bool` at C++14 (`Value(5u).data().index() == 1`), `int64_t` at C++17, and `Value(size_t(5))` does not compile at C++17 | constructors for every integral type storing `int64_t`, as `Reflect` already does; `Value(char)` deleted (ambiguous between a number and a one-letter string) | fix (task 0067) |
| `Value(nullptr)` | *probe:* picks `Value(const char*)` and throws `logic_error` from `std::string(nullptr)` | `Value(std::nullptr_t)` makes `None` | fix (0067) |
| `Value(ValuesList&&) noexcept`, `Value(ValuesMap&&) noexcept` | allocate, so `bad_alloc` becomes `std::terminate` | drop `noexcept` there | fix |
| `Value(uint64_t)` above `INT64_MAX` | cannot be held: wrapped to a negative number in 1.x, `std::overflow_error` since 0172 | `jinja2::BigInt` (`big_int.h`): an integer of any size as canonical decimal text, `TryParse(std::string_view, BigInt&) noexcept`, `Sign`, `ToInt64`/`ToUInt64`, comparisons by value; the last `Value` alternative (`sizeof(Value)` unchanged), with `IsBigInt()`, `AsBigInt()`, `bool AsBigInt(const BigInt*&)`; the integral constructor, `Reflect` and the JSON bindings store it for a value outside `int64_t`. `IsInt()` stays `int64_t` only. Until 0270 a `BigInt` that enters a template fails the render (a top-level render parameter fails it even unread), and `ToString()` prints one only at top level, not inside a list or map; arithmetic on it needs a backend (0271, plan docs/design/0178-big-integer-plan.md) | add (0269); an exhaustive `std::visit` over `GetData()` must handle the new alternative |
| `Value(float)` | widened to `double` exactly, so `12.123f` prints `12.123000144958496` | `float` is the last `Value` alternative (after `BigInt`, `sizeof(Value)` unchanged), with `IsFloat()`, `AsFloat()`, `bool AsFloat(float&)`; `IsDouble()` stays false for it and host equality stays structural (`Value(1.5f) != Value(1.5)`). `Value(double)` and `Value(long double)` store `double`; `Reflect` keeps a `float` member a `float`. In a template a `float` is the double nearest its shortest decimal (`12.123f` is `12.123`), and a value coming back from a template is a `double`; a typed `float` callable parameter gets the exact float back (0039, plan docs/design/0039-float-value-plan.md) | add (0039); an exhaustive `std::visit` over `GetData()` must handle the new alternative |
| `IsEqual()` next to `operator==` | redundant | keep both; not worth a break | — |
| `ValuesMap : std::unordered_map` | hash order | ordered map (task 0043) | break, done in 0043 |
| free `AsString`/`AsWString` overloads for `std::string`, `std::wstring` and the views | encoding conversion; the `std::string` one returns a `const` value, defeating moves | `ToString`/`ToWString` returning `std::string`/`std::wstring` | alias |

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
| `LoadTemplate`/`LoadTemplateW` return `nonstd::expected<…>` | spelled out instead of `Result<Template>`/`ResultW<TemplateW>` | `Result<Template>`/`ResultW<TemplateW>` (same types) | none |
| `IsEqual`, `TimePoint`, `TimeStamp` public | implementation details | move into the impl | break (unlikely to be used) |
| no `FromString` | Jinja2's `env.from_string` | add `FromString(source, name)` | add |
| `Settings::m_defaultMetadataType` | private-member prefix on a public field | `defaultMetadataType` | break (one line; script) |
| `Settings::Extensions::Do` | `CamelCase` field (`do` is a keyword) | `doStatement` | break (script) |
| `Settings::Extensions::LoopControls`, `I18n` | `CamelCase` fields | `loopControls`, `i18n` | unreleased |
| `Settings::useLineStatements` | superseded by `lineStatementPrefix = "#"` (0028) | remove | break |
| `Settings::jinja2CompatMode`, `enum Jinja2CompatMode { None, Vesrsion_2_10 }` | misspelt, and **never read** by the library | remove | break |
| `operator==(Settings, Settings)` lists every field by hand | a new field that is not added there compares equal silently | out of line, next to a `static_assert` on the field count or a test that fails when the struct grows | fix |
| every new `Settings` field changes `sizeof(Settings)` | parity work keeps adding options, each an ABI break | accepted: see SOVERSION policy in 5.4 | — |
| no way to bound a render: the depth limits are compile-time constants, nothing bounds time, steps, output or loads | a host rendering templates its users write cannot stop a runaway one | `Settings::Limits limits` (0262), fixed at 2.0, see below | add |

**`Settings::Limits` (task 0262).** One nested struct, every field enforced, 0 meaning no
limit beyond the built-in caps: `maxRange`, `maxSequenceSize` (only lowers the 2^26 cap),
`maxOutputSize`, `maxTemplateSize` (Load, loader-sourced templates included),
`maxTemplateLoads`, `maxRenderSteps`, `maxRenderTime` (polled every 1024 steps and after each
host callable returns), `maxRenderMemory` (bytes of strings, lists, dicts and other objects a render holds at
once, counted when built and given back when freed, 0459, 0478), and the depth limits `maxRenderDepth` (256), `maxExpressionDepth`
(70), `maxExpressionOperators` (491), `maxBlockNesting` (248), which used to be constants (defaults where Python Jinja2 stops, 0098). A
limit fails the call with `ErrorCode::ResourceLimitExceeded`, `extraParams[1]` naming the field;
the depth limits keep `RecursionLimitExceeded`. A render started by a host callable inside
another keeps its own template's limits. Presets: `UntrustedLimits()` (Jinja2
`SandboxedEnvironment` anchors: `range()` 100000, sequences 2^20, 16 MiB of output, 10^7
steps, and 256 MiB of render memory) and `UntrustedSettings()` (the same with `autoescape`). A step is a statement body,
an item a loop's `if` looks at, a render level, or the evaluation of an operator, call or
filter: the chat-template benchmarks take 550-1000. With every limit at its default a render
pays nothing per step: steps are counted in the stack check's slow path, which only a render
with limits takes.

**Big-integer backend (task 0271, plan docs/design/0178-big-integer-plan.md).**
`TemplateEnv::SetBigIntBackend(const BigIntBackend*) noexcept` installs the arithmetic for
integers outside `int64_t` (`big_int_backend.h`: a versioned table of 13 noexcept functions on
objects of at most 64 bytes). It returns false, changing nothing, for a table the library cannot
use. A template keeps the backend its environment had when it was made, as it keeps the settings.
Without a backend (`nullptr`) such arithmetic fails the render with
`ErrorCode::IntegerOutOfRange`. `GetBigIntBackend()` and `GetBuiltinBigIntBackend()` read it back.
Every environment starts with the built-in one: none by default, `cpp_int` with
`-DJINJA2CPP_WITH_BIGINT=boost` (0272). The header-only adapter
`jinja2cpp/big_int/boost_cpp_int.h` (`BoostCppIntBackend()`) works with any build and is the
sample for writing a backend over another library.
Only int64 overflow calls the backend, so int64 arithmetic costs the same with or without one.

**Chat-template preset (task 0360, decided by Ruslan 2026-11-01).** `jinja2cpp/chat_template.h`:
`ConfigureForChatTemplates(TemplateEnv&, ChatTemplateOptions = {})` configures an environment as
transformers' `apply_chat_template` does: trim/lstrip blocks, loop controls, autoescape off,
`limits.maxRange` at most 100000, transformers' `tojson` (`json.dumps` with `ensure_ascii=False`,
keys in insertion order, no HTML escaping, `indent`/`separators`/`sort_keys`), the
`{% generation %}` tag (body rendered; the assistant mask is `RenderOptions::generationSpans`, below),
read-only lists and dicts (`ImmutableSandboxedEnvironment`: calling `append`, `update`, ... fails
with `ErrorCode::SecurityError` and the sandbox's message) and the globals
`raise_exception` (`ErrorCode::TemplateRuntimeError`) and `strftime_now` (clock in
`ChatTemplateOptions::now`). Later knobs are new fields of `ChatTemplateOptions`, not new entry points.
The header and its one source file are built and installed only with the CMake option
`JINJA2CPP_WITH_CHAT_TEMPLATES` (default ON); `JINJA2CPP_HAS_CHAT_TEMPLATES` (generated
`jinja2cpp/features.h`, included by `config.h`) says whether they are, and
`JINJA2CPP_CHAT_TEMPLATES_TRANSFORMERS` names the transformers version the preset follows (the pin
of `test/chat_templates/requirements.txt`, which catches its drift). What the preset switches on is
an internal dialect of the core, always compiled and dormant unless the preset sets it, so the
option changes no other symbol or layout. Considered: a documented `AddGlobal`/`AddFilter` recipe
(a correct transformers `tojson` is too easy to get wrong, and the tag and immutable containers
need the core anyway) and an `ApplyChatTemplate(tpl, messages, ...)` mirror of transformers
(can be added after 2.0 without breaking this one). Known limits: it is not the rest of
`apply_chat_template` (the host builds `messages`, `tools`, `bos_token`, ...) and no security
sandbox beyond what it lists.

**Assistant token mask (task 0377, Ruslan 2026-11-10, before G1).** `RenderOptions::generationSpans`, a
`std::vector<GenerationSpan>*` (`struct GenerationSpan { size_t start, end; }`) the host owns: the
render clears it and appends the `[start, end)` range of each `{% generation %}` body, in the order
the blocks end (inner before outer), as transformers' `generation_indices`. Offsets are in units of
the output (UTF-8 bytes narrow, `wchar_t` units wide) counted from the first character the render
writes; the host converts to code points or tokens (transformers counts `str` characters). A failed
render leaves the vector empty, as it leaves a string target empty. A
buffered part in Jinja2 (a macro's result, a call block, a recursive loop) places its blocks where
the buffer began, as transformers does. Considered: a field of `ChatTemplateOptions` (the mask is a
property of a render, not of the environment) and a render overload returning the pairs (a second
overload set next to the three of `RenderTo`). `RenderOptions` grows by a pointer, an ABI change of
a public struct that must land before G1, after which it cannot grow. The `SecurityError` half of
the task was done by 0357 S2c; only the docs changed.

**Jinja2's error classes and generate items (tasks 0432, 0436, before G1).** `ErrorCode` gains
`TemplateSyntaxError = 1023` and `TemplateAssertionError = 1024` (additive; `ExtraParams[0]` is
Jinja2's message). `UnexpectedStatement` keeps the tag in `ExtraParams[0]` and adds Jinja2's sentence
as `ExtraParams[1]`; `TemplateRuntimeError` adds the Python class (`"TypeError"`, `"ValueError"`) as
`ExtraParams[1]` where the engine knows it. `RenderOptions` grows by `bool writeEachPiece`: a sink
gets one `Write` per piece Jinja2's `generate()` yields (text run, `{{ }}` value, macro or call
output up to the 512-character buffer), so the Python module's `generate()` yields Jinja2's items.
Considered: a separate sink callback per piece (a second sink interface for one flag) and
splitting in the module (the module cannot see node boundaries). Like `generationSpans`, the growth
of `RenderOptions` must land before G1.

**Suspendable renders (task 0457, Ruslan 2026-11-11 D1, before G1).** `RenderOptions` grows by
`const RenderSuspension* suspension` (`struct RenderSuspension { const void* stackLow; size_t stackSize; }`).
A render to a sink with it set may be suspended inside `Write`: its host runs it on a stack of its own (a
fiber) and switches back to its own code there, to resume the render later on the same thread. The engine
then keeps the render's per-thread state (workspace contents, limits, stack bounds, poll, render depth, cycle
collector) in a bundle of its own, swapped with the thread's at entry, around each `Write` and at exit, so
other renders, plain or suspendable, run and interleave on the thread while one is suspended; and it checks
the stack against the given bounds. Null costs nothing (the code is in the hooks instantiation and a cold
translation unit); a render to a string ignores it. The Python module's lazy `Template.generate()` is the
user. Considered: an additive `RenderTo(sink, params, options, const RenderSuspension&)` overload after G1
(a fourth `RenderTo` overload for one pointer), and a public resumable render class owning the fiber (the
core would gain platform assembly and stack management, docs/design/0457-lazy-generate-plan.md option b).
Like `generationSpans`, the growth of `RenderOptions` must land before G1.

**Prompt escaping and token budgets (tasks 0358, 0359, decided by Ruslan 2026-11-05).** A new,
always installed header `jinja2cpp/prompt.h` (core, included by `template_env.h`, not behind
`JINJA2CPP_WITH_CHAT_TEMPLATES`, whose switch must not change the core ABI) adds:
`enum class AutoescapeMode : unsigned char { Html, Prompt }`; `PromptEscapeOptions` (special tokens, tag names,
separator, default a space); `PromptEscaper`, the compiled patterns, immutable and shared by
copies (`shared_ptr<const Impl>`, equality by identity, as `AutoescapeSelector`), with a throwing
constructor and a `noexcept` `IsValid` (2.1 rule 5) and `Neutralize` for host code; and
`Tokenizer`, a host callback pair on UTF-8 text (`count`, required; `prefix(text, maxTokens)`,
optional), with the estimate `ceil(utf8_bytes / 4)` when unset. `Settings` gains
`autoescapeMode`, `promptEscaper` and `tokenizer`. Prompt-safe is the Markup flag: in prompt mode
autoescape neutralizes instead of HTML-escaping, string literals are prompt-safe, `|e` neutralizes,
and one environment escapes for one mode. The filters `neutralize`, `fence`, `count_tokens`,
`truncate_tokens` and `pack` are specified in docs/cpp-only-filters.md; their exact output is part
of the contract. The library keeps no per-model list of special tokens. Cost: renders without
autoescape run the same instructions; HTML-autoescape renders pay one load and branch per escaped
value, shown within count.py's noise or moved out. Considered: a second "prompt-safe" mark beside
Markup (a new bit in every value, and Markup crossing between modes), a pluggable escaper callback
(a call and an allocation per `{{ }}`, and the composition rule left to the host), a zero-width
separator (deleted by BERT-style normalizers, which re-forms the token) and a 1-token-per-byte
estimate (a bound for byte-level BPE, about 4x too high for English). Steps and
test rows: docs/tasks/0358; follow-ups 0392-0396.

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
| `IMapItemAccessor::HasValue`/`GetValueByName` take `const std::string&` and need two lookups for one read | the caller builds an owning key per access; an accessor answers presence and value separately | `Find(std::string_view) -> std::optional<Value>` and `Contains(std::string_view)`, bridged both ways with the 1.x pair, which stays overridable and is not deprecated (compilers warn callers of a deprecated virtual, never its overriders); `GenericMap` takes `std::string_view`; 3.0 makes the 1.x pair non-virtual (0115, decided 2026-10-05) | break (rare): `GenericMap` lookups no longer take a type convertible only to `std::string`, nor bind to a `const std::string&` member pointer; 1.x accessors compile unchanged; `GenericMap::GetValueByName` now asks `HasValue` first, as template lookups already did; an accessor overriding neither lookup gets `std::logic_error` |
| `IMapItemAccessor::GetSize()` returns `size_t` ("max means unknown"); lists return `optional<size_t>` | inconsistent | keep `size_t`: changing a pure virtual signature breaks every implementer for little gain | — |
| commented-out `IMapItemAccessor::IsEqual` | dead code | remove | — |
| JSON bindings specialise `detail::Reflector` | the extension point for non-struct types lives in `detail`, while structs use public `TypeReflection` | public `jinja2::Reflector<T>`; `detail::Reflector` kept as the base of its primary template, so 1.x specialisations still work (an alias template cannot be specialised) | add |
| `ReflectedMapImpl::GetAccessors()` returns `auto` | copies the whole accessor `unordered_map` on **every** field access, `HasValue` and `GetKeys` | return a reference | fix (0069) |
| `ReflectedMapImplBase::GetValueByName` throws `runtime_error` for an unknown field | the interface says it returns an empty value | return `Value()` | fix (0069) |
| `Reflect(5LL)` | *probe:* does not compile on LP64 (`int64_t` is `long`; no `Reflector<long long>`) | reflect every integral type | fix (0067) |
| `Reflect(const T&)` borrows (stores a pointer), `Reflect(T&&)` owns | surprising, but changing it silently copies or stops compiling for non-copyable types | keep; document; add `ReflectRef`/`ReflectCopy` if asked | — |
| `JINJA2_INT_REFLECTOR` macro | wrong prefix, leaks from the header | `#undef` after use | fix |
| `GenericList`, `GenericMap` hold `std::function<const I…ItemAccessor*()>` | each copy copies the callable and the accessor in it (an allocation per copy); 32 bytes | hold `std::shared_ptr<const IListItemAccessor>` (`IMapItemAccessor`): a copy shares the accessor. New `explicit GenericList(std::shared_ptr<const IListItemAccessor>)`, which can point into a larger object (aliasing constructor). The 1.x callable form stays as a constructor template: it stores the callable in one shared allocation and calls it once, when the list is made (0348, decided by Ruslan 2026-10-29) | break (ABI, 16 bytes). 1.x code compiles; the callable must return the same accessor every time; copies share the accessor's state and compare equal |

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
| `JINJA2_DECLSPEC` | wrong prefix | removed: only `config.h` used it (decided 2026-10-18, 0076) | break (internal macro) |
| `value_ptr.h` includes `polymorphic_cxx14.h`, then `polymorphic.h` under `__cplusplus != 201402L` | both share one include guard, so the second include is dead; `using namespace xyz;` inside `jinja2::types` | include one; drop the `using` | fix (0069) |
| vendored `xyz::polymorphic` in the global `xyz` namespace | collides with a user's own copy of the reference implementation | move into `jinja2::detail` | none |
| `RecWrapper<T>` = `polymorphic<T>` for the concrete `ValuesList`, `ValuesMap`, `UserCallable` | pays for type erasure (virtual copy through a control block) that a final type never uses | vendor `indirect<T>` from the same reference implementation (jbcoe/value_types, the source of C++26 `std::indirect`); `RecWrapper<T>` = `detail::indirect<T>`; `polymorphic` stays for the enumerator interfaces (decided by Ruslan 2026-10-02) | break (ABI; source unchanged for code that spells `RecWrapper`) |
| nonstd `optional`/`variant`/`string_view`/`expected` select `std::` by the **consumer's** standard | library and user disagree on `Value`, `Result`, virtual signatures; silent ODR violation | 2.0: `std::` types for `optional`/`variant`/`string_view` (C++17 floor); expected-lite pinned to `nonstd::expected` in the exported target, so `Result<T>` is one type at every standard | fix (0068, 0071) |
| installed `jinja2cpp-config.cmake` | hand-written, always `STATIC IMPORTED`, no namespaced target, sets `PUBLIC` definitions on an imported target | generated export with `jinja2cpp::jinja2cpp` | fixed (0068) |
| no umbrella or forward header | users include five headers; inline namespace (5.3) breaks user forward declarations | `jinja2cpp/jinja2cpp.h`, `jinja2cpp/fwd.h` | add |
| `error_handler.h` | see 3.4 | remove | break |

### 3.9 Memory and allocation

Decided by Ruslan 2026-10-18 (task 0252; plan in
[design/0252-memory-contract-plan.md](design/0252-memory-contract-plan.md)). The error side is
section 2.1 (0250).

- **No allocator parameters in 2.0.** No entry point takes a `memory_resource`, and `Value`,
  `ValuesList` and `ValuesMap` stay on `std::allocator`. A Load upstream would cover about 1 of
  ~958 Load allocations (the parse-tree arena allocates its own blocks), a per-render resource
  is escaped by values that outlive the render, a pmr string would grow `Value` from 40 to 48 B
  against the 0140 work, and Apple's libc++ ships `std::pmr` only from macOS 14 / iOS 17. If a
  hook comes in 2.x, it is an additive `TemplateEnv` method taking `std::pmr::memory_resource&`
  (never Boost's pmr: 0230). Host data enters without copies through `string_view`,
  `Reflect(const T&)` and generic accessors (0115).
- **Out of memory.** `std::bad_alloc` propagates out of the call that hit it, and means only
  that the system is out of memory (2.1). A spent budget is `ResourceLimitExceeded`: the
  library's limits (0262), the arena's size cap at Load, and a host budget signalled by throwing
  `jinja2::Exception{ResourceLimitExceeded}` from a callable, a sink or the host's allocator. A
  failed Load keeps the previous template; a failed render leaves `Template`, `TemplateEnv`
  and the thread caches usable. Until [0256](tasks/0256-survive-allocation-failure.md) lands,
  some allocation failures crash (arena block bookkeeping, vendored robin_hood; fixed by
  #597). With the nlohmann JSON binding, OOM inside `nlohmann::json`'s destructor terminates
  the process (0279).
- **Caller-owned output** ([0257](tasks/0257-render-to-string-and-sink.md)):
  - `RenderTo(StringType& out, params)` appends; on any failure `out` is truncated back to its
    size at entry;
  - `RenderTo(BasicOutputSink<CharT>& sink, params)` streams; chunks already written stay
    written. `BasicOutputSink` has one virtual `Write(basic_string_view<CharT>)`, frozen for 2.x;
    its exceptions are host exceptions under 2.1;
  - header-only `ContainerSink<C>` appends to any container with `append(first, last)`
    (`std::string`, `fmt::basic_memory_buffer`, `std::vector`, `small_vector`). No fmt type
    appears in a signature: fmt stays a private dependency;
  - `Render(ostream&)` stays all-or-nothing: the stream receives output only on success.
  These are overloads rather than an options struct, so every later target is ABI-additive.
- **Thread caches.** Renders on a thread reuse slot frames, loop frames, parameter tables, the
  last environment's converted globals and buffers. `bool ReleaseThreadCaches() noexcept`
  ([0275](tasks/0275-release-thread-caches.md)) frees them on the calling thread and returns
  `false`, freeing nothing, while a render runs there. No caller-passed workspace.
- **User-facing text:** README "Memory and threads" and Doxygen notes
  ([0276](tasks/0276-memory-contract-docs.md)).

### 3.10 Embedding lifecycle

Decided by Ruslan 2026-10-25 (task 0223, decisions D1-D7 of its plan). What a host may rely on
when templates, environments, renders, modules and callbacks overlap in time. The scenario suite
is `test/embedding_lifecycle_test.cpp`; it runs in the sanitizer jobs (ASan/UBSan/LSan, TSan).
The ownership graph: a template holds its environment's state, so the state outlives the
`TemplateEnv` object (`~TemplateEnv` empties the caches and stops caching); copies of a template
share one loaded template; a render holds the template it renders, every template it includes,
extends or imports, and its snapshot of the globals.

**Caching.**
1. `LoadTemplate(name)` returns the cached template while one is cached. With `autoReload` it
   first asks the handler for the file's time; a handler that gives none (`MemoryFileSystem`)
   never reloads. A reload makes a **new** template: templates the host holds, renders in flight
   and modules made earlier keep the old one, as Python's `get_template` returns a new object.
2. `cacheSize` bounds each width's cache, as Jinja2's `cache_size` does (default 400, 0 off, a
   negative size unbounded): past it the least recently used template is dropped
   ([0264](tasks/0264-template-cache-bound.md)). An eviction follows the rule of a reload. The
   key is the name as given, as in Jinja2, so `a.j2` and `./a.j2` are two entries.
3. `include`, `extends` and `import` go through the same cache. `TemplateLookup::OncePerRender`
   (the default) looks a name up once per render; `EveryUse` looks it up on each use and keeps
   every version it got until the render ends.
4. After the `TemplateEnv` object is destroyed nothing is cached: each lookup opens and parses
   the file, and the parse is freed when the render that made it ends.
5. **(D1)** `Load` replaces the template for that `Template` object only. Copies, the
   environment's cache entry, running renders and modules keep the template they had
   ([0321](tasks/0321-load-copies-on-write.md); this reverses the second half of 0284 D1a: a
   module no longer expires).

**Threads.**
1. The `const` members of a template (renders, metadata, `MakeModule`,
   `FindUndeclaredVariables`) may run on many threads at once. Each render needs its own
   params, output and sink.
2. `Load` and `LoadFromFile` must not run concurrently with another call on the **same object**;
   copies and the cache are other objects (D1).
3. **(D2)** `LoadTemplate`, `LoadTemplateW` and `FromString` may run concurrently with each
   other, with renders and with changes to globals, filters, tests and translations.
4. Configuration (`SetSettings`, the mutable `GetSettings()`, `AddFilesystemHandler`,
   `SetBigIntBackend`) is synchronised with nothing: not with loads, not with renders of the
   environment's templates (which read the handlers and `autoReload` when they include), not
   with other configuration. Configure first, as in Jinja2.
5. Globals, filters, tests and translations may change at any time. A render works on the
   snapshot of the globals it took when it started (D22); a nested render on the same thread
   sees what the outer one changed in place (0211). A filter or test change affects templates
   loaded afterwards.
6. Host code (callables, filters, tests, `finalize`, gettext callables, the autoescape selector,
   reflected accessors, filesystem handlers, sinks) runs on the rendering thread, and on several
   threads at once when renders do: it must be thread-safe the way the host uses it. The
   library's handlers are safe for concurrent loads; `MemoryFileSystem::AddFile` concurrent with
   loads is not. A sink belongs to one render.
7. Calls of one module's macros take turns (a recursive mutex), so a macro may call back into
   its own module.
8. `ReleaseThreadCaches()` acts on the calling thread only, and refuses while a render runs
   there.

**Reentrancy (host code a render calls, same thread).** Allowed: render any template, the same
one included, within the nested-render limits of [security.md](security.md); `LoadTemplate`,
`FromString`; change globals, filters, tests or translations (the running render keeps its
snapshot); call module macros; `ReleaseThreadCaches()` (returns `false`); destroy the
`TemplateEnv` object (the render keeps the state); `Load` on the rendering template (D1: the
render finishes on the template it started with); throw (2.1). **(D4)** Not allowed, and not
detected: configuration calls (threads, item 4) and destroying the `Template` object whose
render is running (the C++ rule for a member call). Both are undefined behaviour.

**Values crossing the boundary.**
1. The render borrows the params map and everything in it for the duration of the call. Host
   data reflected by `const&`, by pointer or as a `string_view` must stay alive and unchanged
   while any value that refers to it is in use: the render, and any copy the host kept, module
   exports included.
2. **(D3)** A value a callable receives (`args`, `extraPosArgs`, `extraKwArgs`) stays valid
   after the call, except where it refers to data the host lent (item 1). That includes a list
   or map that came from the globals: it keeps the globals it was read from
   ([0318](tasks/0318-kept-globals-keep-their-snapshot.md)). `context` is valid during the call
   only.
3. A value a callable returns is taken over by the library; borrowed host data inside it
   follows item 1.
4. A thrown exception becomes the error of 2.1; the environment, the template and the thread
   caches stay usable (3.9).
5. `MetadataInfo::metadata` stays valid while the template is alive and not reloaded;
   `GetMetadata()` returns a self-contained map.
6. Module exports are owned by the host. Macros keep their module, template and environment
   alive, also a macro inside an exported list or dict, or in a value a call returns
   ([0300](tasks/0300-module-export-callable-gaps.md)). A list item read from an export is a
   view into that list (the generic-list rule of `value.h`). A value export is a snapshot taken
   when the module was made: a namespace a later macro call changes is not seen through
   `exports` (Python's `module.ns` is the live object), while the calls share the module's
   state. A live view would put the module's mutex on every read and hand the host mutable
   state the module's calls change, which the immutability principle rules out (0300 item 3).
7. **(D6)** One rule for structs and JSON documents (0311 decision 5): `Reflect` of a non-const
   lvalue copies into an owned view that every copy of the `Value` shares;
   `Reflect(std::as_const(x))`, `Reflect(const T&)` and `Reflect(&x)` borrow;
   `Reflect(std::move(x))` and `Reflect(shared_ptr)` own, and so does a `const shared_ptr<T>&`
   whose reflector takes a `shared_ptr`. A handle type (`JsonTraits::IsHandle`) always borrows;
   `ReflectJson` gives it an owner.
8. A macro, `caller` or built-in callable given to a host callable is lent for the duration
   of the call ([0300](tasks/0300-module-export-callable-gaps.md)): called during it, on its
   thread (by the host, or by a module's macro the host calls), it runs in the render that
   passed it. Called later it fails with `TemplateExpired` (`extraParams[0]` is `"lent"`), as
   that render may be gone. A host's own callable comes back as itself.
9. **A container the render hands to a callable** ([0331](tasks/0331-kept-container-contract.md))
   is never mutable state the host shares with a render or a thread: the host holds a
   read-only snapshot. One the render wrote before the call (or one inside a list or dict the
   template made) is the render's copy, frozen when the render ends: the host keeps its final
   state, also after the params are freed (a nested container it never wrote stays a view of
   the host's data). One handed out before any write, or never written,
   is a view of the host's data (item 1 applies), also while the render runs; Python shows the
   render's later writes there, a deliberate divergence. A global's data is handed out as the
   global's original. A module call's container keeps that call's final state: a later call's
   write copies it first; so do the items of a dictsort pair, a namespace's members and the
   items a generator (`select`, `map`) made when the host got it, which it walks then
   ([0347](tasks/0347-module-cells-in-lazy-and-namespace-exports.md)). A module macro the host
   calls with a render's container writes a copy of its own: the render does not see the write
   ([0407](tasks/0407-module-call-leaks-through-exports-and-closures.md)). Reading a kept value on another thread while its render runs is not
   supported, as for any value of a running render.
10. **Copies of a generic container are one container** ([0348](tasks/0348-generic-list-shared-accessor.md)).
   A `GenericList` or `GenericMap` shares its accessor with every copy, including the copies the
   engine makes (a thread's snapshot of a global, a value handed to a callable). Renders only
   read an accessor, possibly on several threads at once (Threads, item 6). A single-pass list
   (`MakeGenericList` of a generator or of input iterators) is used up by whichever copy reads
   it first, and is not safe in concurrent renders. Give each render its own, in its params or
   from a global callable that makes a new one per call. Two params that are copies of one list
   are one list in the template too: `a.append(7)` shows in `b`, as for one Python object. Before
   2.0, a copy restarted from a copy of the generator's state.

**Destruction order.** Supported in any order: the environment before its templates (they keep
rendering, uncached); templates before their modules; environment, templates and handler before
an `ErrorInfo` (from the same module, 2.1); the environment during a render it runs; a thread
exiting with caches. The host keeps alive: a handler passed by reference and a `BigIntBackend`,
for as long as any template of the environment; lent data (values, item 1); and it calls
`ReleaseThreadCaches()` on pool threads before the environment's globals must be gone (each
thread keeps the last globals it rendered with) and before a plugin is unloaded.
**(D5)** Anything stored in the globals that holds a template of the same environment (a module
macro, a callable capturing a template or module) keeps the environment alive until
`RemoveGlobal` removes it; destroying the `TemplateEnv` object does not break that cycle.

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
                              GenericList, GenericMap, RecWrapper<UserCallable>, Markup, WMarkup, BigInt, float>;

    Value();
    Value(std::nullptr_t) noexcept;                         // None
    template<class I, std::enable_if_t<detail::IsStoredAsInt<I>::value>* = nullptr>
    Value(I val) noexcept(!detail::CanExceedInt64<I>)       // outside int64_t: a BigInt (0269)
        : m_data(detail::ToStoredInt<ValueData>(val)) {}
    Value(char) = delete;                                   // number or one-letter string? say which
    template<class F, std::enable_if_t<std::is_floating_point_v<F>>* = nullptr>
    Value(F val) noexcept;                                  // float stores float (0039), others double
    // ... string, double, list, map, callable constructors as today

    bool IsNone() const noexcept;
    bool IsBool() const noexcept;
    bool IsInt() const noexcept;
    bool IsDouble() const noexcept;
    bool IsString() const noexcept;    // std::string or Markup (a Markup reads as a string)
    bool IsWString() const noexcept;
    bool IsMarkup() const noexcept;    // exactly Markup; IsWMarkup likewise
    bool IsList() const noexcept;      // ValuesList or GenericList
    bool IsMap() const noexcept;       // ValuesMap or GenericMap
    bool IsCallable() const noexcept;
    bool IsBigInt() const noexcept;    // an integer outside int64_t
    bool IsFloat() const noexcept;     // a float exactly; IsDouble is false for it

    const std::string& AsString() const;                // throws bad_variant_access
    std::string& AsString();                           // a held Markup becomes a plain string first
    bool AsString(std::string_view& out) const noexcept;    // false if not a string; no copy
    int64_t AsInt() const;
    bool AsInt(int64_t& out) const noexcept;
    const Markup& AsMarkup() const;                     // exactly Markup, throws for a plain string
    const ValuesList& AsList() const;                   // the ValuesList alternative only
    bool AsList(const ValuesList*& out) const noexcept;
    GenericList ToGenericList() const;                  // a view over either list representation
    GenericMap ToGenericMap() const;
    // AsBool, AsDouble, AsFloat, AsWString (std::wstring_view&), AsMap, AsCallable: both forms likewise

    template<class T> const T& As() const; template<class T> T& As();
    template<class T> const T* GetIf() const noexcept; template<class T> T* GetIf() noexcept;
    const ValueData& GetData() const noexcept; ValueData& GetData() noexcept;

    std::string ToString() const;                       // as {{ v }} prints: "5", "True", "None"
    void ToString(std::string& out) const;              // appends, reuses out's capacity
    std::to_chars_result ToChars(char* first, char* last) const noexcept;   // errc::value_too_large if it does not fit
    std::wstring ToWString() const;                     // and the wide forms likewise

    // 1.x names, removed in 3.0
    JINJA2CPP_DEPRECATED("jinja2cpp-2: IsString") bool isString() const { return IsString(); }
    JINJA2CPP_DEPRECATED("jinja2cpp-2: AsString") auto& asString() { return AsString(); }
    template<class T>
    JINJA2CPP_DEPRECATED("jinja2cpp-2: As") auto get() const { return As<T>(); }   // still copies
    // ...
};

std::string ToString(const Value& v);     // same as v.ToString()
std::wstring ToWString(const Value& v);
JINJA2CPP_DEPRECATED("jinja2cpp-2: ToString") std::string AsString(const Value& v);   // "" for non-strings, as in 1.x

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

**Cost cap** (Ruslan asked that keeping the old names not cost a fortune). A 1.x name is kept
only if its alias is a one-line inline forward with exactly the old behaviour; anything
needing more becomes a hard break with a migration note. That bounds the set to about 35
declarations, all in two headers:

| Where | Kept names | Count |
|---|---|---|
| `Value` members | `isString`, `isWString`, `isList`, `isMap`, `isEmpty`, `asString`, `asWString`, `asList`, `asMap`, `get`, `getPtr`, `data` (const and non-const) | ~19 |
| free functions | `AsString`, `AsWString` overloads (already exist; only gain the attribute) | 10 |
| type aliases | `EmptyValue`, `ErrorInfoTpl`, `UserCallable::UserCallableFunctionPtr`, `detail::Reflector` | 4 |

What it costs: no ABI or layout change (inline, non-virtual); no measurable compile time;
one test file, `test/v1_names_test.cpp`, built with deprecation warnings off so the aliases
cannot rot; and removal in 3.0 is deleting one marked block per header. The aliases are
also what the migration script feeds on (5.2), so they pay for themselves.

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

With the C++17 floor the public API names `std::optional`, `std::variant` and
`std::string_view`, and `Result<T>` uses expected-lite pinned to its own implementation
(0071), so the mismatch 0068 describes (nonstd types resolving differently in the library
and in the user's code) cannot happen in 2.0, and the namespace needs no extra tag for it.

### 5.4 SOVERSION policy

SOVERSION goes to 2 with this release. After that, it tracks the ABI, not the major
version: a minor release that adds a `Settings` field bumps it, because user code
allocates `Settings`. Parity work adds options regularly, so the alternative (freezing
`Settings` or hiding it behind setters) costs more than a SOVERSION number. `BigIntBackend` grows
only at its end, behind `structSize` and `version`, so a later version's larger table is not an
ABI break (0271).

### 5.5 `[[nodiscard]]` results (release note)

Functions whose result is the point of the call carry `[[nodiscard]]` (docs/tasks/0059):
the `const` accessors and queries of the public headers (`Value::isString`, `asString`,
`get`, `IsEqual`, `RenderAsString`, `GetMetadata`, the error accessors, about 110 in all)
and `TemplateEnv::LoadTemplate` and `FromString`, and `Template::Load`, `LoadFromFile`
and `Render` (both `Template` and `TemplateW`), whose result is the only report of a parse
or render error (docs/tasks/0080). Code that calls one of them and drops the result now
gets a compiler warning (`-Wunused-result`, MSVC C4834), an error under `-Werror`. The
common case is a bare `tpl.Load(source);` as 1.x examples wrote it: check the result
(`if (auto r = tpl.Load(source); !r) { /* r.error() */ }`), or cast it to `void` where
dropping it is intended.

### 5.6 Unknown filters and tests fail at load (release note)

As in Jinja2, a filter or test is one built in or added with `TemplateEnv::AddFilter` or
`AddTest`. A template that names any other fails `Template::Load` with
`ErrorCode::UnknownFilter` or `UnknownTest` ("No filter named 'x'."), unless the name is
inside an `if` statement or an inline if, where it fails when it runs (docs/tasks/0193).
1.x let a `UserCallable` passed in the render parameters or the globals act as a filter or
test, found when it ran; `Settings::userCallablesAsFilters = true` keeps that behaviour.

### 5.7 C++-only filters render, `toxml` is gone (release note)

In 1.x `camelize`, `underscorize`, `escapecpp`, `toxml` and `toyaml` were registered but
rendered an empty string. In 2.0 the first four render the output that
[cpp-only-filters.md](cpp-only-filters.md) describes, and `toxml` is removed: a template that
uses it fails `Template::Load` with `ErrorCode::UnknownFilter` (docs/tasks/0233,
docs/release-2.0.md D16). Templates that leaned on the empty output drop the filter.

## 6. Release options

| Option | What ships | Users get | Cost |
|---|---|---|---|
| **A (recommended)** | master as **2.0.0**: new names, deprecated old ones, removals, ordered `ValuesMap` (0043), inline namespace | one upgrade with compiler-guided migration and a script | none beyond the 2.0 work |
| B | master as 1.4.0 with new names and deprecations, then 2.0 removes | two breaking upgrades in a row: 1.4 already breaks source (custom list accessors) and ABI | a release that lies about being minor |
| C | A, plus a 1.4.0 cut from the `1.3.2` tag that only adds the new `Value` names as inline aliases and deprecates the old ones | users stuck on 1.x can migrate their `Value` calls early | a release branch with its own CI; `Settings` renames cannot be bridged there |

C is cheap in code (the `Value` accessors are header-only) and expensive in process.
Recommend A now, C only if someone asks.

## 7. Decisions to make

1. **Release as 2.0.0 directly** (option A) rather than a 1.4 from master. *Decided by Ruslan 2026-10-02: yes.*
2. **Deprecate in place** (5.1) instead of an opt-in `compat/v1.h`. *Decided: yes, provided it stays cheap;* the cost cap is in 5.1.
3. **Value accessor verbs**: *agreed with Ruslan 2026-10-02:* `Is` / `As` (throws) /
   non-throwing access / `ToString` (member and free, always succeeds), as in section 2.
   - 3b. *Decided:* `ToString(5)` is `"5"`, the value as `{{ v }}` prints it.
   - 3c. *Decided:* non-throwing access is an overload `bool AsX(out) const noexcept`, with
     `out` a `std::string_view` for strings (no copy), a value for scalars and a pointer for
     containers, plus `GetIf<T>()` for generic code. Considered and dropped: `TryAsX()`
     (Ruslan disliked the name), `AsXOr(fallback)`, an `AsString(std::nothrow)` tag, and
     `bool AsString(std::string& out)`, which copies and cannot be `noexcept`.
4. **C++ standards**: *decided by Ruslan 2026-10-02:* drop C++14; **C++17 is the floor**,
   and C++17, C++20 and C++23 are all supported and tested (task 0008 has the survey and
   the toolchain table). Consequences for this design:
   - the API uses `std::optional`, `std::variant` and `std::string_view`; optional-lite,
     variant-lite and string-view-lite leave the project (0071);
   - `Result<T>` stays on expected-lite, pinned to `nonstd::expected` in the exported
     target (`nsel_CONFIG_SELECT_EXPECTED=nsel_EXPECTED_NONSTD`), so a C++23 user
     and a C++17-built library agree on the type. `std::expected` would need a C++23 floor
     and, with libstdc++, Clang 19 (*probe:* Clang 18 with libstdc++ 13 has no
     `std::expected`), and was the only thing C++23 would have bought;
   - newer features sit behind feature-test macros: `std::formatter<Value>` only where
     `<format>` exists (C++20, GCC 13); everything else in this design needs only C++17.
   - C++26 gets a CI build row (forward-compatibility check, *decided by Ruslan
     2026-10-02*), but the public types never switch on the consumer's standard: the
     vendored `polymorphic`/`indirect` stay pinned exactly like expected-lite, and move
     to `std::polymorphic`/`std::indirect` in one place once the floor reaches C++26.
   An earlier revision of this document read the decision as a C++23 floor; Ruslan
   corrected it the same day.
5. **Renames of names not yet released**: `AddTester`→`AddTest` (and `Remove`/`Find`),
   `LoopControls`→`loopControls`, `I18n`→`i18n`. *Decided: yes.*
6. **`Value(char)`**: delete it (force the user to say `Value(int64_t('c'))` or
   `Value(std::string(1, 'c'))`), or store a one-letter string? *Decided: delete.*
   Today it silently stores `true`.
7. **Memory contract** (0252, 3.9). *Decided by Ruslan 2026-10-18 (all as recommended):*
   R1 no allocator parameters in 2.0; R2 `bad_alloc` only for system OOM, budgets are
   `ResourceLimitExceeded`; R3 `Render(ostream&)` stays all-or-nothing; R4 output targets as
   `RenderTo` overloads (string, sink) plus the header-only `ContainerSink`.
8. **Chat-template preset** (0360, 3.3). *Decided by Ruslan 2026-11-01:* public, one entry point
   `ConfigureForChatTemplates(TemplateEnv&, ChatTemplateOptions)` in its own header behind the CMake
   option `JINJA2CPP_WITH_CHAT_TEMPLATES` (default ON) with a feature macro; the internals it enables
   stay in the core, dormant otherwise.

9. **Prompt escaping and token budgets** (0358, 0359, 3.3). *Decided by Ruslan 2026-11-05 (all as
   recommended):* Markup is the prompt-safe flag with `Settings::autoescapeMode` per environment;
   string literals are prompt-safe in prompt mode; the separator is a visible space after the
   first code point, specials also matched ASCII case-insensitively; `AutoescapeMode`,
   `PromptEscaper` and `Tokenizer` in `jinja2cpp/prompt.h` with no specials registry in the
   library; the estimate `ceil(utf8_bytes / 4)`; `pack(budget, attribute, by, reverse, reserve,
   stop)` with no per-item overhead argument; core, not behind the chat-template switch; one load
   and branch per escaped value on the HTML-autoescape path, if count.py shows it within noise.
10. **The Python module on the public API only** (0357, plan in
   [design/0357-python-module-public-api.md](design/0357-python-module-public-api.md)).
   *Decided by Ruslan 2026-11-06: every recommendation, after the advisor's and Codex's
   reviews.* **Status: implemented** (S1-S4: #800, #811, #813 and the S4 PR). `python/`
   includes only `jinja2cpp/` headers, and CI builds it alone against an installed library
   (`python.yml`, job `installed`, with a check of every header it includes). The engine's
   private entry points for the module are gone: `TemplateImpl::RenderInternal` and
   `ReferencedTemplatesOf`, `host::EngineUserCallable` and `TemplateEnvImpl::SetSandboxPolicy`.
   `src/host_object.h` is the engine's side of the public bridge only.
   - **What is added.** A standalone public `jinja2::IHostObject` in `jinja2cpp/host_object.h`,
     built by composition: `MakeHostValue`, plus an `AsHostObject()` slot on
     `IMapItemAccessor`. It does not derive from the map accessor.
   - **Export views.** The public export views gain lossless entries (non-string keys), a tuple
     flag and a stable identity token.
   - **Undefined values.** Free functions `MakeUndefined` and `UndefinedOf`, and an opt-in
     `UserCallable::keepUndefined`. `None` stays the default.
   - **Rendering.** `RenderOptions`, with per-render globals and a `std::function` poll;
     call-site information for `*context` callables; hex digits for `BigInt`;
     `Template::FindReferencedTemplates`. The globals are a `shared_ptr<const ValuesMap>`, not a
     call-scoped borrow: a value of them that a callable keeps stays valid after the render, as
     an environment global's does (S2b).
   - **Sandbox.** An optional public `ISandboxPolicy`, at no cost when it is unset. Both the
     native attribute path and the direct method-call path honour it.
   - **Growth after the freeze.** Through `IHostObject2 : IHostObject`, found once per object.
     There is no `Query` escape hatch.
   - **Kept template objects.** A kept `loop` or namespace becomes a readable snapshot. Lent
     macros still expire.
   - **Perf gate.** The module must beat Python Jinja2 on every case. No case may lose more
     than 5% of its margin, measured in instruction counts.
   - **Host reads that write into the engine (decided by Ruslan 2026-11-09).** The S1b probe
     lost more than 5% of the margin on host-data cases, so D2 was reopened. Taken: option (c),
     a write-only `HostResult` over the engine's own storage with three fast reads on
     `IHostObject` (`GetAttrTo`, `GetItemTo`, `ItemAtTo`, defaulting to `GetAttr`, `GetItem`
     and `Iterate`), plus one more sink, `HostResult::SetHostObject`: a nested host object
     (`shared_ptr` or `SetHostObject<T>(args...)` built in place) goes into the result
     without a `Value` and a `GenericMap` made and converted back.
   - **D7 relaxed for host-data cases.** "Beats Python Jinja2 on every case" stays the hard
     gate. The 5% margin rule no longer binds cases dominated by host data; their remaining
     gap against the private seam is accepted. Callgrind instructions per render, Python
     module, private seam on the rebased branch / (c) / (c) with the sink:
     for_filter_if 818,610 / 894,552 / 853,131, chat_llama 636,918 / 726,761 / 710,482,
     mitsuhiko_table 8,211,299 / 9,131,276 / 8,890,719. The sink closes 54%, 18% and
     26% of the gap (c) leaves.
   - **Frozen ABI.** `detail::MakeHostValueInPlace` and the private exported
     `HostResult::SetHostObjectInPlace` are called from header templates (`MakeHostValue<T>`,
     `HostResult::SetHostObject<T>`), so their signatures and the `construct` contract are
     frozen ABI for 2.x like any exported function.
   - **Template objects keep their class (decided by Ruslan 2026-11-09).** S3 found that a
     `loop`, namespace, cycler or joiner a callable receives through the public API is only a
     map, so Python saw a dict. Taken: a virtual `IMapItemAccessor::GetObjectClass()`
     (`nullptr` by default) and `GenericMap::GetObjectClass()`, giving `LoopContext`,
     `Namespace`, `Cycler` or `Joiner`. A sandbox policy gets a template macro or function as
     an empty map of class `Macro` or `function`. It adds a vtable slot before the freeze.
   - **A sequence with its own iteration (task 0441, before the freeze).** The engine iterates
     an `IsSequence` object with a `Len` by index only when `IHostObject::IteratesByIndex()`
     (true by default) says so; otherwise through `Iterate`. The module answers true only for an
     exact `list` or `tuple`, so a subclass loops through `iter()` as in Python. `loop.length`
     stays `Len`, and list operators still read the items by index. It
     adds a vtable slot before the freeze.
   - **Numbers of a host object (task 0434, before the freeze; decided by Ruslan 2026-11-12).**
     `IHostObject` gains `IsNumber()` (`isinstance(x, numbers.Number)`, for `is number`; false by
     default), `ToInt()` (`int(x)` for the `int` filter) and `ToFloat()` (`float(x)` for `int` and
     `float`), the last two `std::optional<Value>` with nullopt by default. The `int` filter asks
     `ToInt`, then `ToFloat` truncated, then the default, as Jinja2's `do_int`. The Python module
     answers them with `numbers.Number`, `int()` and `float()` (a `TypeError` or `ValueError` is
     nullopt). It adds three vtable slots before the freeze.
   - **Attributes of a module macro (task 0434, before the freeze; decided by Ruslan 2026-11-12).**
     `jinja2::MacroAttributes(const UserCallable&)` returns the attributes Jinja2's `Macro` has for
     a macro a module exports: `name`, `arguments` (a `ValuesList`), `catch_kwargs`,
     `catch_varargs` and `caller`; an empty map for any other callable. A free function, so it
     adds no vtable slot; the Python `ModuleMacro` exposes them as read-only properties.

## 8. Work breakdown

Filed as tasks 0070-0077 on 2026-10-02, after Ruslan agreed section 7. `touches` are listed so `scripts/task_batches.py`
can wave them.

| # | Work | touches | Notes |
|---|---|---|---|
| a (0072) | `Value` accessors, `NoneValue`, integral constructors, `Visit` | `include/jinja2cpp/value.h`, `string_helpers.h` | builds on 0067; then `src/` and `test/` use the new names |
| b (0073) | `BasicTemplate<CharT>`, `Result<T, CharT>`, `Load(string_view)` | `include/jinja2cpp/template.h`, `src/template.cpp` | |
| c (0074) | `TemplateEnv` pimpl, `Settings` renames and removals, `AddTest`, `FromString` | `include/jinja2cpp/template_env.h`, `src/template_env.cpp`, `src/template_impl.h` | |
| d (0075) | containers and reflection: public `Reflector`, `GenericMap` export and iteration, `IComparable` default, `BasicErrorInfo` | `generic_list*.h`, `reflected_value.h`, `binding/*.h`, `error_info.h` | builds on 0069 |
| e (0076) | inline namespace, `fwd.h`, umbrella header, `config.h`, vendored `polymorphic` into `detail`, remove `error_handler.h`, SOVERSION 2, version 2.0.0 | every public header | touches every header's namespace line: run alone, after a-d |
| f (0077) | `scripts/jinja2cpp_migrate.py`, `MIGRATION.md`, docs site, README | `scripts/`, docs | migrates our own tests as its test |
| g | ordered `ValuesMap` | per 0043 | exists |
| h | `readability-identifier-naming` for `include/` | `.clang-tidy` | 0054/0065 follow-up |

Before all of them: the standard change (0070), then the switch from the nonstd libraries
to `std::` types (0071). 0070: replace the 40 `get_unexpected()` calls that stop the
library compiling at C++23, make C++17 the floor in CMake, rebuild the CI matrix on
C++17/20/23 and run the clang-tidy job at C++17 (the floor). The 0054 tidy batches wait for
it too.

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
- **Switching `Result<T>` from expected-lite to `tl::expected`** (TartanLlama, v1.3.1;
  both projects maintained in 2025): it never selects `std::expected` on its own, but
  expected-lite pinned with `nsel_CONFIG_SELECT_EXPECTED=nsel_EXPECTED_NONSTD`
  gives the same stability, already has `and_then`/`transform`/`or_else`, and stays
  closer to the standard spelling (`tl` adds a non-standard `map`). Users write
  `jinja2::Result<T>`, so the backing library is swappable later; the next swap is to
  `std::expected` once the floor reaches C++23.
- **Allocator-aware API in 2.0** (0252): a `memory_resource` for Load or Render, pmr
  `Value`/containers, a caller-passed render workspace, a per-render byte budget, an
  `OutOfMemory` error code, `RenderTo(fmt::memory_buffer&)` and an options struct for output
  targets. Reasons in [design/0252-memory-contract-plan.md](design/0252-memory-contract-plan.md)
  section 9.
