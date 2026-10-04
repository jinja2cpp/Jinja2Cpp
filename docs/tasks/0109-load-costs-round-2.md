---
status: open
priority: low
area: perf
depends: [0103]
touches: [src/filters.cpp#CreateFilter, src/function_base.h#ParseParams, src/expression_evaluator.cpp#ExpressionFilter, src/expression_parser.cpp#ParseFullExpression, src/lexertk.h]
---
# Load costs after 0103: filter construction, wrappers, the lexer

**Problem.** After 0103 `Load/many_tags` costs 21.99M instructions and 16,559
allocations per load (300 lines, each with an `if`/`else`, four expressions, two filters
and a `set`). The callgrind and gperftools profiles (bench/README.md) split it roughly as:

- allocation itself (`malloc`/`free`/`operator new`/`delete`): about a quarter. Most
  allocations are expression nodes, which are needed, but some are not:
  - every `ParseFullExpression` allocates a `FullExpressionEvaluator`, also when there is
    no inline `if` (about 2,100 per load); `{{ }}` adds an `ExpressionRenderer` on top.
    The wrapper carries the render-time `CheckStack`, so dropping it needs a look at
    the recursion limits (0098);
  - each filter costs three or more allocations and about 2,300 instructions at load:
    `CreateFilter` copies `CallParamsInfo` by value through a `std::function`,
    `FunctionBase::ParseParams` builds `ArgumentInfo` strings (some longer than the SSO
    buffer) and a hash map with buckets for `default(...)`;
  - string constants are copied (`InternalValue` copy allocates) from the token into the
    `ConstantExpression`, because parsers may backtrack over the token list.
- the lexer: `lexertk::generator::process` plus `Lexer::Preprocess` are about 13%, about
  300 instructions per token, including a `std::string` and an `InternalValue` for each
  identifier.
- destroying the template (part of the `Load/` loop): about 9%, proportional to the
  number of nodes.
- `nonstd::expected` construction and destruction across the precedence chain: about 3%.
- glibc `malloc_consolidate`: about 3%, triggered by the large reallocations of the block
  list and the root composition while the fast bins hold the previous template's nodes.
  Reserving from a count of the template costs more than it saves on text-heavy
  templates (`Load/large_static` got 25% slower in a trial).

**Ideas.** Create filters without copying the call parameters and bind their arguments
from a static table; skip the `FullExpressionEvaluator` when there is no `if`, once the
stack check has another home; keep identifier names as source ranges in tokens and make
the string only where a node needs it.

**Done when.** `Load/many_tags` is measured before and after with
`bench/count.py --baseline` and costs at least 25% fewer instructions than after 0103.
