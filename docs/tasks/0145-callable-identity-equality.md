---
status: open
priority: low
area: parity
touches: [src/value_visitors.h, src/internal_value.h]
---
# A macro or a callable is not equal to itself

**Problem.** `BinaryMathOperation` has no case for `Callable` or `UserCallable`, so `==`
between two callables falls to the default and is false. Jinja2 compares functions by
identity: `{% macro m() %}x{% endmacro %}{{ m == m }}` renders `True` in Python and
`False` in Jinja2C++, and `{{ f == f }}` for a user callable `f` is `True` in Python and
`False` here. Found by the verifier of the 0118 decision 5 PR, which
did not change value comparison (it removed structural equality of parse-tree nodes only).

**Proposal.** Compare callables by identity: a `Callable` made from a macro by the macro
node and its bound frame, a user callable by the address of its stored function object.
`Callable` holds a `std::function`, which has no identity of its own, so the value needs an
identity token when it is made (the 0118 P4 escape handle of a macro is a natural one).

**Done when.** Corpus cases `expressions.macro_equals_itself` and
`expressions.user_callable_equals_itself` match Python.
