---
status: done
priority: medium
area: robustness
depends: [0054]
touches: [src/internal_value.cpp#Value2IntValue, src/render_context.h#IRendererCallback, src/template_impl.h#ThrowRuntimeError, src/template_parser.h, src/value_methods.cpp#FormatSpec, src/filters.cpp#Attr, src/error_handling.h, src/statements.cpp]
---
# Bug-class findings from the clang-tidy survey (0054, batch 1)

**Problem.** The clang-tidy 22 survey of master b0991e3 (task 0054) reported a handful of
diagnostics that point at latent defects rather than style. They need a person to read
them, not an automatic fix, and they should be gone before the bug-finding checks gate
pull requests.

- **`Value2IntValue(Value&&)` keeps a reference to a dying value**
  (`clang-analyzer-core.StackAddressEscape`, `src/internal_value.cpp:1124`, reached from
  lines 671, 1151 and 1428). When `InputValueConvertor` returns no result, the rvalue
  overload falls back to `InternalValue(ValueRef(val))`, a reference to the caller's
  temporary or local. Today every alternative of `Value`'s variant converts, so the
  fallback is unreachable; a new alternative without a converter would turn it into a
  use-after-free. Fix: make the fallback unreachable by construction (convert by value,
  or assert), keeping the lvalue overload's borrowing as it is.
- **`ThrowRuntimeError` is not `[[noreturn]]`** (`src/render_context.h:45`,
  `src/template_impl.h:382`). Callers such as `filesizeformat` (`src/filters.cpp:1555`)
  dereference an empty optional right after calling it; the C++17 run reports these as
  `bugprone-unchecked-optional-access`. Marking the interface and its overriders
  `[[noreturn]]` documents the contract and silences them correctly.
- **Uninitialised members and locals** (`cppcoreguidelines-pro-type-member-init`,
  `init-variables`): `ParseError::errorCode` (`src/error_handling.h:14`), `ConverterParams::mode`
  (`src/filters.cpp:1031`), `StatementInfo::type` (`src/template_parser.h:231`),
  `operation` in `src/expression_parser.cpp:256,300`, `comparator` and `actualList` in
  `src/filters.cpp:370,764`.
- **Smaller ones:** `template_parser.h` includes itself (`misc-header-include-cycle`); an
  unused `padding` string in `src/value_methods.cpp:914`
  (`bugprone-unused-local-non-trivial-variable`); `const auto result` blocks the move in
  `src/filters.cpp:332` (`performance-no-automatic-move`); `ErrorConverter::Convert`
  returns its `const&` parameter (`bugprone-return-const-ref-from-parameter`,
  `src/template_impl.h:132`); `throw tpl.error()` throws an `ErrorInfoTpl`, whose copy constructor can throw
  (`bugprone-exception-copy-constructor-throws`, `src/statements.cpp:603`); the
  override-visibility changes in `src/template_parser.h:1478-1512`; the
  `signed char` to `uint32_t` conversions (`bugprone-signed-char-misuse`, 7 sites in
  `src/value_visitors.h`, `src/internal_value.h`, `src/string_converter_filter.cpp`,
  `src/value_methods.cpp`): each needs a look at the narrow instantiation, where a UTF-8
  byte above 0x7F sign-extends.

**Proposal.** One PR that fixes each item by hand or marks it `NOLINT(<check>)` with a
reason when the code is right, plus a unit test for any behaviour change. Run
`.clang-tidy`'s `bugprone-*`, `clang-analyzer-*` and the two init checks over the tree
afterwards. The optional-access hits the `[[noreturn]]` fix leaves are 0066's, after the
C++17 floor (0070).

**Done when** those checks report nothing on `src/` and `include/` at C++14.

## Outcome

Done together with 0066 (PR link below), on master 850f797 at C++17. Beyond the list
above, the bug-finding checks had grown 40 more hits since the survey, fixed in the same PR:

- `InputValueConvertor` now returns `InternalValue` instead of an optional: every
  alternative converts, so `Value2IntValue` has no fallback left that could borrow from a
  dying value, and the `if (!converted)` branches in `template_impl.h` and
  `value_visitors.h` went with it.
- `ThrowRuntimeError` is `[[noreturn]]` on the interface and both implementations.
- The `even`/`odd` tests computed `static_cast<int64_t>(double)`, undefined for a float
  outside int64_t's range (`1e300 is even`); they now use `fmod` as Python's `%` does.
  Python counts `bool` as a number there, so `false is even` and `true is number` now
  hold (corpus cases `tests.bool_is_number`, `tests.float_even_odd`).
- `CodeUnit(char)`/`CodeUnit(wchar_t)` in `internal_value.h` replace the
  `static_cast<uint32_t>` of code units (`bugprone-signed-char-misuse`, 7 sites).
- The five container adapters' `U&&` constructors are constrained so they never take a
  copy (`bugprone-forwarding-reference-overload`).
- `bugprone-crtp-constructor-accessibility` is off in `.clang-tidy`: the CRTP bases are
  internal and their derived classes inherit constructors (`using Base::Base`), which
  keep the base's access, so private constructors would make them unconstructible.
- NOLINT with a reason: the four static tables (`throwing-static-initialization`), the
  `throw` of the public error type, `EatIfEqual(char)`'s enum cast (the lexer stores
  one-character operators without enumerators) and one sized `string_view::data()`.

`clang-analyzer-*` and `cppcoreguidelines-pro-type-member-init` enter `WarningsAsErrors`
here. `bugprone-*` and `cppcoreguidelines-init-variables` still have 17 hits in `test/`
and enter with 0063.
