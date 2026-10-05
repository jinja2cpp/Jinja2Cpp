---
status: done
priority: low
area: perf
touches: [src/python_format.cpp, src/python_format.h, src/serialize_filters.cpp#StringFormat, src/filters.h#StringFormat, src/expression_evaluator.cpp#ApplyPercentFormat, src/expression_evaluator.h]
---
# Parse a constant `%` format once, at Load

**Status (2026-10-05).** Done in the PR for this task. `ParsePercentFormat` splits a
format into directives that do not depend on the arguments; `Formatter` runs them. A
narrow, non-Markup literal on the left of `%` or piped into `format` is parsed at Load
into a `CompiledPercentFormat`, which `BinaryExpression` and `StringFormat` own; a format
from a variable is parsed as it goes, by the same code. A format that ends in an error
keeps the error in its last directive and raises it where the old parser did, so the
messages and their order are unchanged (`PercentFormatTest` in test/filters_test.cpp
compares literal and variable formats, errors included). Expressions find the literal
through a new `GetConstant()` virtual: a `dynamic_cast` per filtered expression cost
Load/filters +3.7%. Measured with `bench/count.py --baseline`: `Render/strings`
1,153,357 to 1,136,102 instructions (-1.5%), every Load case within ±0.9%. The parse
was a smaller share than estimated; what remains per call is argument evaluation,
the conversions and the result string. Wide-string literals keep the general path.

**Problem.** This is 0114's step 2, which was left out of #391. `PythonPercentFormat`
re-parses the spec of `'%s:%d' | format(...)` and of `'%s' % x` on every call, even
when the format string is a template literal. After #391 the whole formatter costs
about 970 instructions per call on `Render/strings`. The parse is only a part of
that, worth roughly 2-3% of the case.

**Proposal.** When the format operand is a `ConstantExpression`, have the `format`
filter and `BinaryExpression` (`%`) compile it at Load into a list of literal pieces
and conversions (key, flags, width or `*`, precision or `*`, conversion char). Render
then walks that list. Keep the compiled form immutable and owned by the expression,
so concurrent renders of one template share it without locks. A non-constant format
keeps the current path.

**Done when.** `Render/strings` instructions drop measurably (`bench/count.py
--baseline`). The parity corpus is unchanged, and error messages, including the
"unsupported format character ... at index N" message, stay as they are.

**Next.** With 0113's output writer, a compiled spec can write `{{ '%s' % x }}`
straight into the render output.
