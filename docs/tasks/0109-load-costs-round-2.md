---
status: done
priority: low
area: perf
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/405
depends: [0103]
touches: [src/filters.cpp, src/filters.h, src/function_base.h, src/expression_evaluator.cpp, src/expression_evaluator.h, src/expression_parser.cpp, src/expression_parser.h, src/lexer.cpp, src/lexer.h, src/lexertk.h, src/renderer.h, src/statements.h, src/string_converter_filter.cpp, src/template_parser.cpp, src/template_parser.h, src/testers.cpp, src/value_methods.cpp]
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

**Result.** `Load/many_tags` costs 16.40M instructions and 9,658 allocations per load
(`bench/count.py --baseline` against master 698f881: 22.08M and 16,559), 25.7% fewer
instructions; every other `Load/` case is 9% to 51% cheaper and `Render/` is flat or
slightly cheaper. In order of what they saved:

- the lexer no longer copies identifier names into `Token::value`; the parser reads them
  from the source with `LexScanner::GetAsString` where a node needs one;
- statement bodies, subscripts and else branches keep their first entries in place
  (`small_vector`), `StatementInfo::compositions` is gone (it always held
  `currentComposition`), `set x =` and `for x in` build no target vector;
- `lexertk` classifies ASCII without the locale (a facet lookup per character before);
- `FullExpressionEvaluator` is made only for an inline `if`; Compare, Slice, Tuple and
  Dict nodes now check the stack themselves, as the other compound nodes already did;
- parsed call arguments live in a flat `ArgumentsMap` instead of an `unordered_map`;
  builtin filters are created through function pointers found by binary search, without
  copying the call parameters; filter and tester parameters come from static tables whose
  default nodes every instance shares (`MakeArgumentsTable`);
- the splitter finds newlines with `find` (`Load/large_static` -50%), classifies block
  characters by a table and compares delimiters without `memcmp` calls; `IsMethodName`
  buckets the names by length.

Tried and dropped: a `std::deque` for the statement stack saved 300 allocations but cost
0.5% more instructions.

**Next.** What is left of `Load/many_tags`: `nonstd::expected` through the eleven
levels of the precedence chain (about 2.5%), clearing the token buffer per tag (about
1.5%), glibc `malloc_consolidate` on the copy of the template text (about 2.5%), and
`lexertk::generator::process` itself (about 5%).
