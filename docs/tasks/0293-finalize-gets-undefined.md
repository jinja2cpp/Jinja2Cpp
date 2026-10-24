---
status: done
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/647
priority: medium
area: parity
touches: [src/renderer.h#FinalizedExpressionRenderer]
---
# `finalize` gets None for an undefined value

Found by the 0288 verifier. Jinja2 passes the undefined object itself to `finalize`; an
identity finalize then renders it as empty, `f"[{v}]"` gives `[]`, and with `StrictUndefined`
stringifying it raises `UndefinedError`. Jinja2C++ converts the undefined value to an empty
`Value` for the user callable and what comes back renders as `None`:

- `undefined.finalize_gets_undefined`: `{{ nope }}|{{ d.zz }}` with `brackets`: Python
  `[]|[]`, Jinja2C++ `[None]|[None]`.
- `undefined.strict_finalize_gets_undefined`: `{{ nope }}` with `brackets` and strict
  undefined: Python raises, Jinja2C++ renders. With finalize set, strict checking of `{{ }}`
  output is bypassed.

**Proposal.** Hand the user callable a value that keeps the undefined (and its policy, see
0026/0052), or apply the undefined policy to the expression's value before calling finalize
when the policy raises on output; render an undefined result of finalize as Jinja2 does.
Check what a host `Value` can carry for undefined before choosing.

**Done when.** Both cases match and their lines are gone from
`test/parity/divergences/undefined.txt`.
