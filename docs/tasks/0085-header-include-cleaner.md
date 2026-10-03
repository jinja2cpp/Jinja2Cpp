---
status: done
priority: low
area: build
depends: [0064]
touches: [src/*.h, include/jinja2cpp/*.h]
---
# include-cleaner on headers analysed on their own

**Problem.** 0064 cleaned includes by running clang-tidy over the `.cpp` files, where
`misc-include-cleaner` checks only the main file. The pull-request job is different:
`clang-tidy-diff` runs clang-tidy on every changed header as a file of its own, and
include-cleaner then checks that header. Run that way, 33 project headers report 266
missing includes (master after 0065; `src/value_visitors.h` 48, `template_parser.h` 23,
`template_impl.h` 23, `internal_value.h` 17, `markup.h` 15, `statements.h` 14). The
vendored `robin_hood.h`, `lexertk.h` and `unicode_tables.h` report more, but nobody edits
them. Since the check is in `WarningsAsErrors`, any PR that edits a line of such a
header fails on that line until the header names what it uses. #351 hit this with
`std::numeric_limits` in `src/generic_adapters.h`.

**Proposal.** Apply the same edits to the headers that 0064 applied to the `.cpp` files:
run clang-tidy on each header with `-checks=-*,misc-include-cleaner`, merge the
same-offset insertions, and regroup the includes. Then extend the weekly whole-tree job
so it also runs over `src/*.h` and `include/jinja2cpp/*.h`, which keeps them clean.

**Done when** clang-tidy run on each project header reports nothing, and the weekly
job covers headers.

**Outcome.** Done in this PR:
- Include-cleaner's insertions are applied to 32 headers, including `binding/boost_json.h` and the Boost JSON parser and serializer headers. The includes are regrouped the same way as 0064.
- 15 unused includes are removed from `src/` headers. One `.cpp` file had relied on one of them (`error_info.cpp` on `string_helpers.h`) and now includes it directly.
- Public headers keep what the check calls unused, because user code may rely on it. `template.h`, `make_generic_list.h` and `error_info.h` are marked `// IWYU pragma: export`, and six standard headers carry `// IWYU pragma: keep`.
- The weekly whole-tree job now also runs clang-tidy on each header, skipping the vendored headers, `value_helpers.h` (0084) and the RapidJSON and nlohmann bindings. Run locally, it reports nothing.
