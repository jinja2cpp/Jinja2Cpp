---
status: open
priority: low
area: parity
depends: []
touches: [src/value_visitors.h#PythonTypeName, test/parity/cases/operators.py]
---
# Python type names of callables in error messages

**Problem.** Found in 0297. Every callable reports Python's type name `function`
(`visitors::PythonTypeName(const Callable&)`), so the TypeErrors that name a type say
`argument of type 'function' is not iterable` where Jinja2 3.1.6 says:

| Value | Python type name |
|---|---|
| a macro | `Macro` |
| `range`, `dict` | `type` |
| `lipsum`, a host function | `function` |
| `(1).bit_length`, `'a'.upper` | `builtin_function_or_method` |

The parity suite compares only that both engines fail (`operators.in_macro_raises`,
`operators.in_function_raises`), so the corpus does not catch it; a user reading the message
does. The same name appears in `unsupported operand`, `not iterable` and format errors.

**Proposal.** Let a callable report its kind (macro, builtin class, bound method, global
function, host callable) and map each to Python's name in `PythonTypeName`. Check the 0305
work on builtin class reprs first, which already distinguishes some of these.

**Done when** a unit test asserts the messages above for `1 in m`, `1 in range`, `1 in
'a'.upper` and `1 + m`, narrow and wide.

**Next.** The verifier of 0297/0307 found the same gap in output: a bound method or function prints
empty (`{{ lipsum }}`, `{{ 'a'.upper }}`, `{{ 'a'|attr('upper') }}`) where Python prints its repr
(`<function generate_lorem_ipsum at 0x...>`, `<built-in method upper of str object at 0x...>`).
The addresses make exact parity impossible; decide with 0305's reprs what to print.
