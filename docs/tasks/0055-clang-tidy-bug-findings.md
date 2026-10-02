---
status: open
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
  dereference an empty optional right after calling it; the C++17 and C++23 runs report these as
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
C++23 floor.

**Done when** those checks report nothing on `src/` and `include/` at C++14.
