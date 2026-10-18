# 0250 plan: one error contract for the 2.0 public API

**Decided (Ruslan 2026-10-18 12:24, "go recommended").** D1 a (wrap host exceptions, keep the
original in `GetException()`), D2 a (one `jinja2::Exception : std::runtime_error`), D3 b
(`Value::TryFrom(I, Value&) noexcept`, revised in §9.3), D4 a (one `TemplateRuntimeError` code),
D5 a refined by §9.5 (`bad_alloc` only for system out-of-memory; every limit is
`ResourceLimitExceeded`), D6 a (`#error` without exceptions), D7 a, D8 a (`std::error_code`
interop), D9 a (`ErrorInfo` as one pointer to a shared immutable payload), D10 a
(`__builtin_FILE`/`__builtin_LINE` on `jinja2::Exception`, no `std::source_location`). The
contract is written in docs/api-2.0.md §2.1; phases are tasks 0253 (P1), 0254 (P2), 0255 (P3).
Sections 0-8 are the original plan; section 9 revises it after Ruslan's questions and wins
where they differ.


## 0. The tension

**Requirement A: one predictable, Result-shaped API.** The host should get a `Result` with a specific `ErrorCode` and a template location for every failure in Load/Render. That is where our design deliberately differs from Python.

**Requirement B: host code must not be hidden, and the render hot path must not pay.**
- Host callables, reflectors and accessors throw arbitrary exceptions. Python lets them through unchanged, with the template line in the traceback.
- Classifying an exception and giving it a location means catching it somewhere inside the engine.
- The renderer has no notion of a "current location". Nodes store no source positions, and the parser drops its line table in `ReleaseParseState` (template_parser.h:509).

What the probe and the code show today:
- **Host and engine errors look the same.** A `runtime_error` from a user callable and the engine's own `1 + 'a'` both come back as `UnexpectedException` at `noname.j2tpl:1:1`. Every render error uses the placeholder location 1:1 (template_impl.h:393-424).
- **Engine errors are plain `std::runtime_error`.** The engine raises Python's TypeError/ValueError as plain `std::runtime_error` (about 130 sites in 11 files), so the boundary cannot tell them from host exceptions. The engine's own `catch (const std::runtime_error&)` sites (filters.cpp:1014, expression_parser.cpp:737/1232/1276) also swallow a host `runtime_error` that passes through them.
- **Non-`std::exception` throws escape Render.** The catch chain has no `catch(...)` (template_impl.h:385-424); the probe confirmed a thrown `MyErr{}` reaches the host.
- **`std::bad_alloc` becomes `UnexpectedException`.** The handler at template_impl.h:414 builds an ErrorInfo for it, which itself allocates.
- **Bug: `LoadFromFile` of a missing file returns success.** template.cpp:96-99 does `if (!file.good()) return {};`.
- **The wide conversion drops data.** `ErrorConverter` (template_impl.h:162-174) loses `relatedLocs`; a new field would be lost the same way.
- **`ToChars` (value.cpp:112) reports everything as out of memory.** It is `noexcept` and maps any exception, including a host accessor's, to `errc::not_enough_memory`.

## 1. The contract, as rules

R1. **Load\*, Render\*, `GetMetadata*` and `TemplateEnv::LoadTemplate*` report every failure through `Result<T>`, except out of memory.** This includes exceptions from host callables, reflectors, accessors and filesystem handlers, and non-`std::exception` throws.

R2. **`std::bad_alloc` (and `bad_array_new_length`) propagates unchanged from every API call.** Same rule as 0252. Matches Python's `MemoryError`, and avoids allocating an error object after an allocation failed. A failed Load keeps the previous template (strong guarantee, already true: template_impl.h:258-299). A failed Render leaves Template and TemplateEnv unchanged.

R3. **Output on failure.** `RenderAsString` returns no string. What a caller-supplied stream or sink received is unspecified (today `Render(os)` writes only on success, template.cpp:107-112, but the contract does not promise it, so 0252 may stream). Exceptions from a host stream whose exception mask the host enabled propagate.

R4. **Outside Load/Render, calls that run host code let its exceptions through unchanged** (e.g. `GenericList`/`GenericMap` access from the host, `Value::ToString` of a reflected container).

R5. **Documented throwing calls.** Nothing else throws except `bad_alloc` and R4.
- `Value::AsX()` / `As<T>()` throw `std::bad_variant_access` (decided, api-2.0.md §2).
- `OrderedMap::at` throws `std::out_of_range` (std-container protocol).
- `Result::value()` throws `bad_expected_access<ErrorInfo>`, spelled `jinja2::BadResultAccess<CharT>` so the type survives a switch to `std::expected`.
- `Value(I)` / `Reflect` / JSON bindings throw `jinja2::Exception{IntegerOutOfRange}` for an integer outside int64 (0172).

R6. **`noexcept` guarantees**, written into the doc as a table:
- `Value`: move ctor/assign, `IsX`, `bool AsX(out)`, `GetIf`, `GetData`, `ToChars`, `Value(nullptr)`, `Value(I)` for every I that fits int64, `Value(F)`.
- `BasicErrorInfo`: move ctor/assign and all getters (add `noexcept` at error_info.h:141-181).
- `OrderedMap`: move, iterators, size, empty, clear, swap. Destructors.
- Host-supplied destructors, `IComparable::IsEqual` and filesystem-handler destructors must not throw.
- `ToChars` returns `errc::not_enough_memory` only for `bad_alloc`, `errc::invalid_argument` for any other failure (fixes value.cpp:112).

R7. **Exceptions are required.** `-fno-exceptions` is unsupported. config.h gets `#if !defined(__cpp_exceptions) && !defined(_CPPUNWIND)` → `#error "Jinja2C++ requires C++ exceptions"`.

R8. **What a render error carries:** a specific `ErrorCode`; `srcLoc` of the innermost statement or `{{ }}` tag that was running, in the template that owns it (included, imported or parent); `locationDescr` with the caret line; `GetException()` set iff the failure began as an exception the engine did not raise. Errors raised before any statement runs (e.g. converting params at Render start) keep the template name with 1:1.

### Python: match and deliberate differences
- **Match:** `TemplateSyntaxError`, `UndefinedError`, `TemplateNotFound` map to existing codes with location; `MemoryError` propagates as `bad_alloc` will; a host `jinja2::Exception{UndefinedError}` passes through with its code.
- **Differ:** errors arrive as `Result`, not raised (unchanged since 1.x). Host exceptions are wrapped, not propagated; the original is kept, so Python's behaviour is one line: `if (auto& p = e.GetException()) std::rethrow_exception(p);`. Python's TypeError/ValueError/ZeroDivisionError become one code `TemplateRuntimeError`. int64 overflow raises `IntegerOutOfRange` (until 0178). No sandbox `SecurityError`; `ResourceLimitExceeded` covers hard caps and 0097's limits.

## 2. Answers to the task's questions

**Q1. What throws / returns Result; throwing `Value` ctor.** R1-R5. Keep the throwing ctor (D1a in release-2.0.md). No checked factory in 2.0: it is additive later, 0178's backend makes it always succeed, and it does not reach the case that matters (lazily reflected data converted mid-render, which R8 covers). Rejected: `Value::TryFrom(I)` now (open decision D3); storing `double` (already rejected in D1).

**Q2. Codes with location.** Append codes (§3a). Location from one `try` per statement body (§3c). Rejected: a "current node" store per statement (hot-path store); a source offset in every node (changes layouts 0118/0140 sized); wrapping each host accessor call (~74 call sites, several per item).

**Q3. Own exception type.** Exactly one: `class JINJA2CPP_EXPORT Exception : public std::runtime_error` with `ErrorCode Code() const noexcept`. Thrown by our headers for host-data errors, and by hosts from callables/accessors to report a specific code that passes through the boundary with location. Not used for Load/Render, `As<T>`, `OrderedMap::at`. Rejected: std types only (boundary can't tell our overflow from a host's); a Python-like hierarchy (duplicates `ErrorCode`, more exported typeinfo); a code mixin alongside std types (two `std::exception` bases make `catch (const std::exception&)` ambiguous); an exception carrying full `ErrorInfo` (error_info.h includes value.h, so value.h can't depend on it).

**Q4. Host exception translation, original kept.** Add `std::exception_ptr` to `ErrorInfo`. Costs: 8 B (Itanium) / 16 B (MSVC x64) per ErrorInfo; copy = atomic refcount; the object is shared across copies/threads (doc: catch by `const&`, treat as immutable); an `ErrorInfo` outliving `dlclose` of the throwing plugin is UB (documented). Rejected: storing only `what()` + type name (loses rethrow); `std::throw_with_nested` (the boundary returns `Result`).

**Q5. Docs.** New docs/api-2.0.md section, README section, Doxygen on `Result`/`ErrorCode`/`Exception`/`GetException` (§6).

## 3. Concrete changes

### a) `ErrorCode` (append, explicit numbers)

| Value | Name | Raised for | `GetException()` |
|---|---|---|---|
| 15 | `UserCallableException` | anything thrown out of a `UserCallable` (context/global value, `AddFilter`/`AddTest`, `Settings::finalize`, gettext) | yes |
| 16 | `IntegerOutOfRange` | host integer outside int64 (Value/Reflect/bindings, 0172) and int64 overflow in template arithmetic (`ThrowIntegerOverflow`, value_visitors.h:1000) | host data yes, arithmetic no |
| 17 | `TemplateRuntimeError` | the template's own operation failed (Python TypeError/ValueError/…); replaces `UnexpectedException` for engine-raised errors | no |
| 18 | `ResourceLimitExceeded` (D7) | the `MaxSequenceSize` cap today, 0097's limits later | no |

`extraParams[0]` is the message (`what()` or `"unknown exception"`). `UnexpectedException` (1) keeps its number and narrows to "an exception the engine did not raise and could not attribute" (host accessor, reflector, filesystem handler, std library, non-std type), now with location and `GetException()`. `InvalidValueType` (8) is not repurposed.

### b) New public types and fields
- **`include/jinja2cpp/exception.h` (new):** only `<stdexcept>` plus opaque `enum class ErrorCode;`. `jinja2::Exception(ErrorCode, const std::string&)`, `Code()`. Exported, destructor out of line in src/error_info.cpp as key function so typeinfo is unique under 0076's hidden visibility (else a host `catch` fails across a shared-library boundary on macOS/libc++). error_info.h includes it.
- **value.h:** no dependency on it; declares `[[noreturn]] JINJA2CPP_EXPORT void detail::ThrowIntegerOutOfRange(...)` which `ToStoredInt` (#573) calls instead of `throw std::overflow_error` (cold path out of line).
- **`BasicErrorInfo::Data`:** trailing `std::exception_ptr exception;` (positional aggregate init stays source-compatible); `[[nodiscard]] const std::exception_ptr& GetException() const noexcept`.
- **`BasicErrorInfo` moves:** replace the hand-written swap move-assign (error_info.h:124-138) and move ctor with `= default` (still noexcept; the hand-written swap would silently skip the new field).
- **`ErrorConverter`:** copies `exception` and `relatedLocs` (template_impl.h:166-172).
- **`ToString` texts** for codes 15-18.
- **template.h:** `template<class CharT = char> using BadResultAccess = nonstd::bad_expected_access<BasicErrorInfo<CharT>>;`.
- **fwd.h:** `class Exception;`; **jinja2cpp.h:** add exception.h (0076-plan §7).

### c) Render-time location (no hot-path stores)
- **Source offset per statement.** Preferred: when the parser closes a body (`TakeBody` template_parser.h:394, `TakeFrameBody` template_parser.cpp:45, root template_parser.h:1513), record each child's opening-tag offset in the same arena block after the children: 4 B per statement, no new allocation, resolved only on the error path. Fallback: a sorted `vector<pair<uint32,uint32>>` in `TemplateImpl` (one Load allocation). The sealed arena keeps a back-pointer to its template's source and name so the catch site computes `line:col` and `locationDescr`.
- **`ComposedRenderer::Render` (renderer.h:63-74):** one `try` around the loop, current child held outside it:
  ```
  catch (const std::bad_alloc&) { throw; }
  catch (const detail::RenderFailure&) { throw; }   // innermost location wins
  catch (...) { detail::ThrowRenderFailure(values.Nodes(), child, std::current_exception()); }
  ```
  `ThrowRenderFailure` (new src/render_failure.{h,cpp}) classifies and throws `detail::RenderFailure{ErrorInfo}`:

  | Caught | Becomes |
  |---|---|
  | `jinja2::Exception` | its `Code()` + ptr |
  | `detail::HostCallableFailure` | 15 + inner ptr |
  | `detail::TemplateRuntimeError` | 17 |
  | `UndefinedError` | 11 |
  | `RecursionLimitError` | 12 |
  | thrown `BasicErrorInfo<…>` | keep code, fill location if placeholder |
  | `std::exception` / `...` | 1 + ptr |

- **Render boundary (template_impl.h:384-424):** first `catch (const std::bad_alloc&) { throw; }` and `catch (const RenderFailure& f) { return Convert(f.info); }`; existing clauses stay for errors outside bodies and gain the ptr; final `catch (...)` → 1 + ptr.
- **Same boundary on `TemplateImpl::Load`** (around `parser.Parse()`, template_impl.h:271) and `TemplateEnv::LoadTemplate` around handler calls (template_env.cpp:212-227).
- **Internal catches crossing a body now see `RenderFailure`.** Only one: the include `FileNotFound` catch (statements.cpp:1945-1958) wraps the include's rendering; Python's `ignore missing` covers only the lookup (checked), so P2 narrows that try to the load. Reviewer rule: no exceptions as control flow across statement bodies.

### d) Host-callable translation
`UserCallableBody::operator()` (internal_value.cpp:2201-2205) is the single funnel for context callables, filters, tests, finalize and gettext. Wrap only `val.callable(ucParams)`: pass engine-origin types through (`RenderFailure`, `TemplateRuntimeError`, `UndefinedError`, `RecursionLimitError`, `BasicErrorInfo`, `jinja2::Exception`, `bad_alloc`) so a callback into a macro keeps its own error; `catch (...)` → `throw detail::HostCallableFailure{std::current_exception(), msg}`. That function already builds a `ValuesMap` and two `Value` lists per call, so the try region is unmeasurable. Reflectors and accessors are classified by type at the body catch.

### e) Engine runtime errors
- `detail::TemplateRuntimeError : std::runtime_error` in src/error_handling.h (unconverted internal catches keep working).
- Scripted rewrite of `throw std::runtime_error(` in src/ (~130 sites).
- `ThrowIntegerOverflow` → `IntegerOutOfRange`; `MaxSequenceSize` throws → `ResourceLimitExceeded` (D7).
- Narrow filters.cpp:1014 and expression_parser.cpp:737/1232/1276 to `TemplateRuntimeError`; expression_parser.cpp:1276 also prints to `std::cout` on a parse failure: replace with a parse error.
- Rejected: classifying by `typeid(e) == typeid(std::runtime_error)` without the rewrite (misfiles the most common host throw as an engine error).

### Perf: what the bench gate watches
- Happy path: zero extra instructions with table-based EH (Itanium, Windows x64).
- `bench/count.py --cache-sim`: Render cases Ir within ±0.1%, I1mr unchanged; `.text` of `ComposedRenderer::Render` (landing pads); Load Ir and allocations (+0 with arena offsets, +1/template with fallback; ~+4 B per tag in tree bytes).
- No bench case uses a user callable, so (d) is unmeasured by design.
- `Value(uint64)`: one compare and a cold call; unchanged for types that fit.

## 4. Interaction with other tasks
- **0172 (#573):** 0250 depends on it. P1 changes its `std::overflow_error` to `jinja2::Exception{IntegerOutOfRange}` (still a `std::runtime_error`); updates #573's rows in test/basic_tests.cpp and test/binding/*; rewrites value.h/reflected_value.h doc comments.
- **0178:** with a big-int backend code 16 is never raised; default build raises it for host data and arithmetic. D1a "revisit after 0250" resolves as "keep the throw, now typed".
- **0252 (#575):** shared R2 (`bad_alloc` propagates; no `OutOfMemory` code) and R3 (partial sink output unspecified). Any caller buffer / `memory_resource` overload follows R1. The thread-local workspace is RAII-restored on unwinding (`WorkspaceMaps`, template_impl.h:329); P2's rethrows add paths the sanitizer run must cover.
- **0076:** exception.h joins jinja2cpp.h; fwd.h gets `class Exception;`; P1 extends test/fwd_test.cpp. `v2` tag unaffected; must land before the RC.
- **0097:** takes code 18 if D7a; otherwise appends it.
- **0077 MIGRATION.md:** new codes for former `UnexpectedException` errors; `LoadFromFile` now fails for a missing file; the 0172 exception type.

## 5. Phases (PR-sized)

**P0, docs only:** write the contract into docs/api-2.0.md; file P1-P3 as tasks (0253+; re-check master and open PRs); 0250 → `planned`.

**P1 "public error types and boundary"** (ABI-relevant, before the RC, after #573):
- include: exception.h (new), error_info.h, value.h, config.h, template.h, reflected_value.h and binding/*.h (comments), fwd.h, jinja2cpp.h; src: error_info.cpp, template_impl.h (boundary clauses, ErrorConverter, Load guard), template.cpp (`LoadFromFile`), template_env.cpp (handler guard), value.cpp (`ToChars`).
- Tests: errors_test.cpp `ErrorContractTest` (callable throws `int` → 1 + ptr; throws `bad_alloc` → `EXPECT_THROW`; throws `jinja2::Exception{UndefinedError}` → 11; static_asserts on 15-18, `is_nothrow_move_constructible_v<ErrorInfo>`, `is_base_of_v<std::runtime_error, Exception>`, `noexcept(Value(int{}))`, `!noexcept(Value(uint64_t{}))`); template_api_test.cpp (`LoadFromFile("missing")` → `FileNotFound`; copy/move/wide conversion keep `GetException()`); filesystem_handler_test.cpp (throwing `OpenStream` → 1 + ptr); #573's rows switch to `jinja2::Exception`; fwd_test.cpp; a `ToChars` row (accessor throw → `invalid_argument`).
- Verifier + preflight `--perf`; Windows CI matters for the exported type.

**P2 "render location and callable boundary"** (behaviour, before the RC):
- src/renderer.h, src/render_failure.{h,cpp} (new), node arena (offset tail, back-pointer), template_parser.{h,cpp}, internal_value.cpp:2201, template_impl.h, statements.cpp:1945.
- Tests: errors_test.cpp `RenderErrorLocationTest` over {template, context, code, line, col, hasPtr}: `"a\n{{ f() }}"` throwing callable → 15 @2:1 ptr; `"{% for i in xs %}\n  {{ i.y.z }}{% endfor %}"` → 11 @2:3; reflected `uint64_t` field `UINT64_MAX` → 16 + ptr; throwing `GenericList` accessor → 1 + ptr; errors inside included/imported/parent templates → that template's fileName; recursion → 12 with location; throwing `AddFilter` filter → 15. user_callable_test.cpp: `rethrow_exception(GetException())` yields the original type. includes_test.cpp: `ignore missing` with a missing nested include → `TemplateNotFound`. Update errors_test.cpp:77-81, 103-107.
- Sanitizers (address+undefined), fuzz smoke, bench gate.

**P3 "engine runtime errors"** (behaviour; at a wave boundary with the merge steward, parity-hot files):
- Commit A: scripted rewrite (into `.git-blame-ignore-revs`). Commit B: narrowed catches, 16/17/18 mapping, tests (`{{ 1 + 'a' }}` → 17; `{{ 9223372036854775807 + 1 }}` → 16; `{{ ('x' * 100000000)|length }}` → 18; errors_test.cpp:128 → 17; re-check recursion_limits_test.cpp, containers_api_test.cpp).
- Parity corpus unaffected (errors match regardless of type); a follow-up can check Python's recorded exception type against the code.

P2 and P3 change no public header and may follow P1 in either order. Only P1 is freeze-critical.

## 6. Docs
- **docs/api-2.0.md:** replace the paragraph at line 106 with a new §2.1 "Errors and exceptions" (R1-R8, noexcept table, code table, Python match/differ list, `exception_ptr` lifetime and thread notes); update §3.4 rows; §7/§8 entries for P1-P3.
- **README:** "Error handling" section after "Getting started" (switching on codes, rethrowing a host exception, `bad_alloc`, exceptions required); changelog breaking changes.
- **docs/release-2.0.md:** D1 note "revisited by 0250: kept, typed as `jinja2::Exception{IntegerOutOfRange}`".
- **MIGRATION.md (0077):** the behaviour changes in §4.

## 7. Open decisions for Ruslan
- **D1. Host exceptions at the render boundary.** a) wrap into `Result`, original in `GetException()` (rec.: one contract, original one `rethrow_exception` away); b) propagate unchanged as Python does; c) a `Settings` toggle (doubles the test matrix).
- **D2. Exception type.** a) one `jinja2::Exception : std::runtime_error` with `Code()` (rec.); b) standard types only; c) code mixin plus std types.
- **D3. Checked `Value` factory.** a) none in 2.0, additive later (rec.); b) `static std::optional<Value> Value::TryFrom(I) noexcept` now.
- **D4. Granularity of engine runtime errors.** a) one `TemplateRuntimeError` (rec.); b) Python-class codes (TypeError, ValueError, ZeroDivisionError, KeyError), hand-classifying ~130 sites; c) leave as `UnexpectedException`.
- **D5. Out of memory.** a) `bad_alloc` propagates from every call, same in 0252 (rec.); b) `ErrorCode::OutOfMemory` in `Result`.
- **D6. `-fno-exceptions`.** a) unsupported, documented, `#error` in config.h (rec.); b) documented only.
- **D7. `ResourceLimitExceeded` (18) for the existing `MaxSequenceSize` cap.** a) in P3 now (rec.); b) leave to 0097.

## 8. What this makes harder next (the next contradictions)
- Every new way for host code to run during a render must go through `UserCallableBody` or be classified at the body catch (0180 and 0183 already go through it). Internal code may not use exceptions as control flow across statement bodies.
- The code set becomes a contract: splitting `TemplateRuntimeError` into Python classes later (D4b) changes what switch statements see.
- `exception_ptr` in a public struct ties `ErrorInfo`'s lifetime to the module that threw and pins `<exception>` in error_info.h.
- Render locations depend on per-statement offsets: arena changes (0118 successors) and load-time folding (0149) must keep them; a folded node reports its parent's location.
- R3 leaves room for 0252 to stream, but hosts relying on today's "nothing written on failure" will see partial output once it streams.
- With 0178, the same template yields `IntegerOutOfRange` or a big integer depending on the build.
- Switching to `std::expected` changes the bad-access type; `BadResultAccess` hides it only for code that uses the alias.

## 9. Revision after Ruslan's questions (2026-10-18 11:28)

Ruslan asked about `std::source_location`, the ned14/status-code design, and a noexcept-like public API mode (internals keep exceptions, `bad_alloc` excepted).

### 9.1 `std::source_location`
- It records the C++ call site (file:line:function of C++ code). The location users need is in the *template*, which is not C++, so it cannot replace R8.
- Public headers have a C++17 floor. A field present only under C++20 would give C++17 and C++20 consumers different `ErrorInfo` layouts, the same ABI-split class as the 0002 segfault.
- Where it helps: the C++ origin of a failure, for us (fuzz triage, engine bugs) and for hosts (which line of their callable threw). This needs no C++20: `__builtin_FILE()`/`__builtin_LINE()`/`__builtin_FUNCTION()` work as default arguments in C++17 on GCC, Clang and recent MSVC.
- Proposal (D10): `jinja2::Exception`'s constructor takes `const char* file = __builtin_FILE(), unsigned line = __builtin_LINE()` and stores them (two fields, fixed layout in every standard). The engine's internal throw helpers capture their origin the same way, reported in debug builds only. No `source_location` type in the public ABI.

### 9.2 status-code (P1028 `std::error`)
Its ideas, and what we take:

| status-code idea | Take it? | How in jinja2cpp |
|---|---|---|
| A *domain* identifies the code set; codes from different libraries compare semantically (`== errc::no_such_file_or_directory`) | yes, via the standard | `ErrorCode` becomes a `std::error_code` enum: `jinja2::error_category()` (exported) + `std::is_error_code_enum<ErrorCode>` + `default_error_condition`/`equivalent` mapping (`FileNotFound`/`TemplateNotFound` ≡ `errc::no_such_file_or_directory`, `ResourceLimitExceeded` ≡ `errc::value_too_large`, …). Hosts can put our codes into their existing `error_code` plumbing. The DSO address-comparison weakness that status-code fixes with 64-bit domain ids does not bite: the category singleton lives only in libjinja2cpp. |
| The error object is cheap: trivially relocatable, register-sized, no-throw copy | yes | `BasicErrorInfo` becomes one pointer to an immutable shared payload (code, locations, params, descr, `exception_ptr`). Today it is about 128 B on 64-bit libstdc++ (code + SourceLocation + two vectors + string), so every `Result<T>` carries those bytes. After: 8 B, copy = refcount, copy and move `noexcept`. `GetCode()` stays inline. Only the freeze window allows this layout change. |
| A static, never-allocated payload for out-of-memory | keep in reserve | With 9.2's pointer layout an `OutOfMemory` error costs no allocation (an immortal static payload), so D5 b becomes viable later without an ABI change. |
| Take the library itself (`system_code`, `erased<T>`) | no | Not standardised (P1028 stalled); erased codes require trivially copyable payloads that fit in two words, while ours carries strings and locations; a new vendored dependency against the Boost-free / small-SBOM direction; `Result` is already decided as `nonstd::expected<T, ErrorInfo>`. |

### 9.3 The noexcept-like public mode
- Two worlds, as Ruslan says: internals keep throwing (about 130 throw sites, recursion unwinding, user callables); the public boundary converts.
- The `noexcept` keyword cannot go on Load/Render: with `bad_alloc` allowed out, `noexcept` turns it into `std::terminate`. So the contract is stated as **"throws only `std::bad_alloc`"**, documented on every function and tested, not spelled with the keyword. (Where the keyword is honest it is used: R6.)
- Rule R9: **every public call that may throw something other than `bad_alloc` has a non-throwing sibling**, in the `bool X(out) noexcept` style already decided for `AsX` (api-2.0.md 3c):

  | Throwing | Non-throwing sibling |
  |---|---|
  | `Value::AsX()`, `As<T>()` | `bool AsX(out) noexcept`, `GetIf<T>()` (exist) |
  | `Value(I)` for unsigned above `INT64_MAX` (0172) | **new:** `static bool Value::TryFrom(I, Value& out) noexcept` (flips D3 to b) |
  | `OrderedMap::at` | `find` (exists) |
  | `Result::value()` | `has_value()` / `operator*` (exist) |
  | `Reflect(big uint64)` eagerly, JSON bindings eagerly | lazy reflection reports through `Result` at render (R8); eager scalar path uses `TryFrom` |
  | host-code calls outside Render (R4) | none: the host's own exceptions are the host's |

- A test (`test/nothrow_contract_test.cpp`, P1) static_asserts `noexcept` on every sibling, and a throw-injection row per Result-returning entry point checks that only `bad_alloc` escapes.

### 9.4 Changed and new decisions
- **D3 → b (changed):** `static bool Value::TryFrom(I, Value&) noexcept` in 2.0, as the R9 sibling of the throwing constructor.
- **D8 (new): `std::error_code` interop for `ErrorCode`.** a) yes, category + equivalence table in P1 (rec.); b) later (additive).
- **D9 (new): `ErrorInfo` as a pointer to a shared immutable payload.** a) yes, in P1, before the freeze (rec.: 128 B → 8 B per `Result`, noexcept copy, enables static OOM); b) keep the value layout.
- **D10 (new): C++ origin capture.** a) `__builtin_FILE/LINE` defaults on `jinja2::Exception` plus debug-only engine origin (rec.); b) none; c) `std::source_location` under C++20 only (rejected: ABI split between standards).

### 9.5 Challenge of R2 (`bad_alloc` propagates), 2026-10-18 11:35
Verdict: R2 holds for genuine system out-of-memory; it is refined for memory *budgets*.
- **Why it holds.** The internals throw anyway, so the engine must be exception-safe at every allocation whatever the boundary does; letting `bad_alloc` through costs no code. Catching it would mean building an error while memory is exhausted, swallowing it out of host callables, and handing the host a "continue" signal in a process that usually cannot continue. Hosts already treat `bad_alloc` from `std::vector` the same way, at a top-level handler. Python agrees (`MemoryError` propagates). On Linux with overcommit a true `bad_alloc` is rare; the OOM killer acts first.
- **Where it breaks.** `bad_alloc` is not always "the machine is out of memory":
  1. *Template-driven sizes:* `'x' * 10**10`, `range(10**10)|list`, a huge `join`. Untrusted input must not be able to raise a process-level exception in a multi-tenant host.
  2. *Budgets:* a bounded `memory_resource` (0252) or a 0097 memory limit throws when one render's budget is spent. That is a per-render policy decision, expected and routine.
  3. `std::length_error` (string or vector too long) is not `bad_alloc` but comes from the same place.
- **Refinement R2'.**
  - `bad_alloc` propagates only as "system out of memory".
  - Every limit is reported in `Result` as `ResourceLimitExceeded` (18): the engine checks sizes before allocating (as `MaxSequenceSize` already does; P3 extends it to string repeat, `range`, `join`, `format` widths), and `std::length_error` maps to 18.
  - A host's bounded resource reports budget exhaustion by throwing `jinja2::Exception{ResourceLimitExceeded}` (not derived from `bad_alloc`), which arrives as `Result`. 0252 documents this for its resource interface and its own arena budget.
- **Proof obligation (P1/P2).** A fault-injection test: a counting global `operator new` fails the Nth allocation for N = 1.. over Load and Render of a few representative templates; each run must either succeed or throw `bad_alloc`, leave the Template reusable (a second render matches), and leak nothing under ASan. This is what the trade-off costs; without it R2 is a promise, not a property.
