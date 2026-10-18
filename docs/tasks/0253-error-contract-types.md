---
status: in-progress
priority: high
area: api
depends: [0250, 0172]
touches: [include/jinja2cpp/exception.h, include/jinja2cpp/error_info.h, include/jinja2cpp/value.h, include/jinja2cpp/config.h, include/jinja2cpp/template.h, include/jinja2cpp/fwd.h, include/jinja2cpp/jinja2cpp.h, src/error_info.cpp, src/template.cpp, src/template_env.cpp, src/value.cpp, test/errors_test.cpp, test/fwd_test.cpp, test/allocation_failure_test.cpp]
shares: [src/template_impl.h, include/jinja2cpp/reflected_value.h, include/jinja2cpp/binding/, test/basic_tests.cpp, test/binding/]
---
# Error contract P1: public error types and the Load/Render boundary

**In progress in [#597](https://github.com/jinja2cpp/Jinja2Cpp/pull/597).** Left for it: the
`fwd.h`/`jinja2cpp.h` lines and `fwd_test.cpp`, which wait for #580 (0076). Filed from it: the
nlohmann binding's allocating destructor (0279) and cycle leaks on a failed allocation (0282).

**Release impact (0221).** Freeze-critical (gate G1): changes public types and `ErrorInfo`'s
layout. Lands after #573 (0172), in the API wave. Contract: docs/api-2.0.md §2.1; plan and
rejected alternatives: docs/design/0250-error-contract-plan.md (§3a, §3b, §9).

**Scope.**
- `include/jinja2cpp/exception.h`: exported `jinja2::Exception : std::runtime_error` with
  `ErrorCode Code() const noexcept` and C++ origin via `__builtin_FILE()`/`__builtin_LINE()`
  defaults; destructor out of line (key function, unique typeinfo under hidden visibility).
  Add to `fwd.h` and `jinja2cpp.h` (0076).
- `ErrorCode` 15-18 (`UserCallableException`, `IntegerOutOfRange`, `TemplateRuntimeError`,
  `ResourceLimitExceeded`), `ToString` texts; `jinja2::error_category()`,
  `std::is_error_code_enum<ErrorCode>`, `make_error_code`, and an `equivalent` table to
  `std::errc` (not found, value too large, invalid argument).
- `BasicErrorInfo` becomes one pointer to an immutable shared payload (code, `srcLoc`,
  `relatedLocs`, `extraParams`, `locationDescr`, `std::exception_ptr`); `BasicErrorInfo(Data)`
  and every getter keep their signatures; copy and move `noexcept`; add `GetException()`.
  The wide conversion in `template_impl.h` keeps `relatedLocs` and the exception.
- `Value`: 0172's `std::overflow_error` becomes `jinja2::Exception{IntegerOutOfRange}` thrown
  from an out-of-line `[[noreturn]] detail::ThrowIntegerOutOfRange`; add
  `static bool Value::TryFrom(I, Value& out) noexcept`; doc comments in value.h,
  reflected_value.h and the bindings.
- `template.h`: `template<class CharT = char> using BadResultAccess = ...`.
- `config.h`: `#error` when exceptions are disabled.
- Render and Load boundary (`template_impl.h`, `TemplateEnv::LoadTemplate`): rethrow
  `bad_alloc`; `std::length_error` to `ResourceLimitExceeded`; a final `catch (...)` to
  `UnexpectedException` with the exception kept; filesystem-handler exceptions the same.
- `LoadFromFile` of a missing file returns `FileNotFound` (it returned success).
- `ToChars` returns `not_enough_memory` only for `bad_alloc`, `invalid_argument` otherwise.
- `noexcept` on the `BasicErrorInfo` getters.
- REVIEW.md: its Security rule describes today's catch-all error boundary and defers to 0250;
  rewrite it to the contract each phase lands (0254 and 0255 update it again).

**Tests.** errors_test.cpp `ErrorContractTest`: a callable throwing `int` gives 1 with the
exception kept; throwing `std::bad_alloc` propagates; throwing `jinja2::Exception{UndefinedError}`
gives 11; `static_assert`s on the code numbers, `is_nothrow_copy_constructible_v<ErrorInfo>`,
`is_base_of_v<std::runtime_error, Exception>`, `noexcept(Value::TryFrom(...))`; `error_code`
equivalence rows. template_api_test.cpp: missing `LoadFromFile`; copies, moves and the wide
conversion keep `GetException()`. filesystem_handler_test.cpp: a throwing `OpenStream`.
#573's rows switch to `jinja2::Exception` (still caught as `std::runtime_error`). fwd_test.cpp.
New allocation_failure_test.cpp: a counting `operator new` fails the Nth allocation over Load
and Render of a few templates; each run succeeds or throws `bad_alloc`, the template renders
again afterwards, nothing leaks under ASan.

**Done when.** The rows pass on every CI platform (Windows matters for the exported type) and
`sizeof(ErrorInfo)` is one pointer.
