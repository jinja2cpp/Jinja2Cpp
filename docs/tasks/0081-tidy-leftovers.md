---
status: open
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
