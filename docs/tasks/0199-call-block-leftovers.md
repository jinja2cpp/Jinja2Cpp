---
status: open
priority: low
area: parity
touches: [src/template_parser.cpp#CallStatement, src/value_visitors.h]
shares: [test/parity/cases/statements.py]
---
# Call block leftovers

Found while verifying 0169 (PR #526). All three predate it.

1. **Caller repr.** `{{ caller }}` inside a macro called from a call block prints
   nothing; Jinja2 prints `<Macro anonymous>`, and a named macro prints `<Macro 'm'>`
   (`statements.call_block_caller_repr`). The general callable repr is missing.
2. **Repeated caller keyword.** `{% call m(caller=c) %}` loads and runs; Jinja2 raises
   `TemplateSyntaxError: keyword argument repeated: caller` at load
   (`statements.call_block_explicit_caller_repeated`).
3. **Dotted callee.** `{% call ns.m() %}` fails to parse ("Expected: '('"); Jinja2 accepts
   any call expression after `call`, including attribute and subscript callees
   (`statements.call_block_dotted_callee`).

**Done when.** No divergences line names 0199 and `ctest -R parity` passes.
