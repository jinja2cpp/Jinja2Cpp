---
status: open
priority: low
area: parity
depends: [0001, 0024, 0027]
touches: [src/template_parser.cpp#splitter, src/lexer.cpp, src/lexer.h]
shares: [src/template_parser.cpp, src/template_parser.h, include/jinja2cpp/template_env.h]
---
# Custom delimiters, line statements

**Problem.** Jinja2 lets the Environment change `{{ }}`, `{% %}`, `{# #}` (used for
LaTeX, C and shell templates where the defaults collide with the target language) and
enables line statements (`# for x in y`) and line comments. The splitter also ends a tag at the first
`}}` even inside an expression, so `{{ {'a': {'b': 1}} }}` fails to parse (Jinja2's lexer
tracks bracket balance). Jinja2C++ has fixed
delimiters and an unimplemented `useLineStatements` flag (6 cases). For a C++ code
generator, custom delimiters are a practical need.

**Proposal.** Make the template splitter in `template_parser` take its delimiters from
`Settings`, then add line statement and line comment prefixes. Remove the dead
`useLineStatements` flag or implement it as part of this.

**Scheduling.** Rewrites the template splitter that 0024 and 0027 also change; it goes last of the three.

**Done when.** No line of `test/parity/divergences/` names task 0028, and `ctest -R parity` passes.
