---
status: done
priority: low
area: style
depends: [0055]
touches: [src/ast_visitor.h, src/lexer.h, src/template_parser.h, src/expression_evaluator.cpp, src/statements.cpp, src/filters.cpp, src/testers.cpp, src/value_methods.cpp, src/value_visitors.h, src/generic_adapters.h, src/string_converter_filter.cpp, src/render_context.h, src/internal_value.cpp, src/ordered_map.h, src/value.cpp, src/value_helpers.h, src/filters.h, src/binding/boost_json_serializer.cpp, include/jinja2cpp/]
---
# clang-tidy hits left after the batches

**Problem.** After batches 0057-0062 and 0055, a whole-tree run (clang-tidy 22.1.8, C++17,
master 850f797) still reports about 55 hits in `src/` and `include/` from checks that
are on in `.clang-tidy` but not yet in `WarningsAsErrors`, so nothing stops them growing:
`cppcoreguidelines-virtual-class-destructor` 18 (the `VisitorIfaceImpl` chain in
`src/ast_visitor.h`, `LexerHelper`, `TemplateParser`), `modernize-use-auto` 9,
`readability-inconsistent-declaration-parameter-name` 4,
`portability-template-virtual-member-function` 4, `performance-inefficient-string-concatenation` 4,
`modernize-avoid-c-style-cast` 3, and one or two each of `misc-const-correctness`,
`readability-enum-initial-value`, `readability-use-concise-preprocessor-directives`,
`readability-avoid-unconditional-preprocessor-if`, `readability-const-return-type`
(`include/jinja2cpp/string_helpers.h`), `readability-duplicate-include`
(`include/jinja2cpp/value.h`), `performance-inefficient-vector-operation`,
`performance-move-constructor-init`, `performance-avoid-endl`, `misc-unused-parameters`,
`readability-suspicious-call-argument`, `readability-static-accessed-through-instance`,
`readability-use-anyofallof`, `modernize-avoid-variadic-functions`, `modernize-macro-to-enum`
(`JINJA2CPP_VERSION`, which stays a macro).

**Proposal.** One PR: fix each, or NOLINT with the reason where the code is right (a
public header's signature, the version macro), then move every check that is clean on
the whole tree into `WarningsAsErrors`.

**Done when** the whole-tree job reports nothing on `src/` and `include/` outside the
checks owned by 0061, 0064 and 0065.

**Outcome.** Done in this PR. Every hit is fixed or carries a NOLINT with its reason:
- `VisitorIfaceImpl<void, T>` and `LexerHelper` get virtual destructors, which silences the
  whole chain. `DoVisit`'s empty bodies keep `portability-template-virtual-member-function`
  NOLINTs: they compile for every `Type`, so instantiation order does not matter.
- Fix-its: `use-auto`, C-style casts to `static_cast`, concise preprocessor directives,
  `endl`, const locals, matching parameter names, `reserve`. Also the `const` return
  types of `AsString`/`AsWString` in `string_helpers.h` and the duplicate `<utility>`
  include in `value.h`.
- Manual changes:
  - `Token::Type` keeps implicit enumerator values with a NOLINT, because the
    one-character operators are their own character.
  - The explicit `RM_*` values are removed. The fix-it would have set them all to 0.
  - `ordered_map::operator==` uses `std::all_of`.
  - The pattern and urlize link strings are built with `append`.
  - Boost JSON's `SizeVisitor` catch-all is a template instead of a C variadic.
- NOLINT with reason:
  - `JINJA2CPP_VERSION` stays a macro, because users test it in `#if`.
  - The `RenderContext` move constructor (with `performance-move-constructor-init`).
  - One `suspicious-call-argument`.
  - The `#if 0` bodies of `src/value.cpp` and `src/value_helpers.h`, whose removal is 0084.

With the whole tree clean, `WarningsAsErrors` becomes `*`, minus the checks still being
cleaned: cognitive complexity (0061), include-cleaner (0064) and identifier naming (0065).
Each of those PRs removes its exclusion.
