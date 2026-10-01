---
status: open
priority: low
area: parity
depends: [0001]
touches: [src/lexer.cpp, src/lexer.h, src/template_parser.cpp, src/template_parser.h, include/jinja2cpp/template_env.h]
---
# Custom delimiters, line statements

**Problem.** Jinja2 lets the Environment change `{{ }}`, `{% %}`, `{# #}` (used for
LaTeX, C and shell templates where the defaults collide with the target language) and
enables line statements (`# for x in y`) and line comments. Jinja2C++ has fixed
delimiters and an unimplemented `useLineStatements` flag (6 cases). For a C++ code
generator, custom delimiters are a practical need.

**Proposal.** Make the template splitter in `template_parser` take its delimiters from
`Settings`, then add line statement and line comment prefixes. Remove the dead
`useLineStatements` flag or implement it as part of this.

**Done when.** No line of `test/parity/divergences.txt` names task 0028, and `ctest -R parity` passes.
