---
status: open
priority: low
area: parity
depends: [0377]
touches: [src/statements.cpp, test/chat_template_preset_test.cpp]
shares: []
---
# `caller()` inside a `{% generation %}` block of a macro must be undefined

**Problem.** transformers parses `{% generation %}...{% endgeneration %}` as a `CallBlock`, so its
body is a macro of its own with no caller. In

```jinja
{% macro a() %}{% generation %}{{ caller() }}{% endgeneration %}{% endmacro %}p{% call a() %}x{% endcall %}
```

transformers 5.22.1 fails with `UndefinedError: No caller defined`; Jinja2C++'s
`GenerationStatement` (src/statements.cpp, a `{% with %}`-like scope) sees the macro's `caller` and
renders `px`. Found by the 0377 verifier; it predates 0377 (the tag was a `WithStatement`).

**Proposal.** Shadow `caller` in the generation body's scope with an undefined value (the message
Jinja2 gives: `No caller defined`), as a call block's body does for a nested macro, so it fails as
in transformers. Keep the other names of the enclosing macro visible (transformers' call-block body
sees them).

**Done when.** A row in test/chat_template_preset_test.cpp renders the template above narrow and
wide and fails with ErrorCode::UndefinedError, and a generation body that does not use `caller`
still sees the macro's arguments.
