---
status: open
priority: high
area: parity
depends: [0001, 0012]
touches: [src/template_parser.cpp#splitter, test/forloop_test.cpp, test/macro_test.cpp, test/statements_tets.cpp, test/user_callable_test.cpp, test/expressions_test.cpp, test/filters_test.cpp, test/if_test.cpp, test/extends_test.cpp]
shares: [src/template_parser.cpp, src/template_parser.h, src/lexer.cpp, src/template_impl.h, include/jinja2cpp/template_env.h]
---
# Trailing newline, `-` modifiers, newline normalisation

**Problem.** Jinja2 removes a single trailing newline from the template source unless
`keep_trailing_newline=True`; Jinja2C++ always keeps it, so almost every template loaded
from a file renders one extra newline. Also: `{%-` does not strip across several
newlines, `trim_blocks` applies inside `raw`, and `\r\n` is not normalised to
`newline_sequence` (8 cases).

**Proposal.** Add `keepTrailingNewline` (default false, like Jinja2) and `newlineSequence`
to `Settings`, apply both in the lexer as Jinja2 does, and fix the two stripping rules.
Changing the default changes output for existing users: note it in the release notes, or
gate it on `Jinja2CompatMode` if the maintainers prefer.

**Scheduling.** Dropping the trailing newline by default changes about 110 existing unit tests across eight test files (measured by stripping it in `TemplateImpl::Load`); update them here, in a wave where no other task edits those files. Runs after 0012 so the two test-expectation sweeps do not collide.

**Done when.** No line of `test/parity/divergences.txt` names task 0024, and `ctest -R parity` passes.
