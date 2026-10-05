---
status: open
priority: low
area: perf
touches: [src/python_format.cpp, src/python_format.h, src/serialize_filters.cpp#StringFormat, src/expression_evaluator.cpp#ApplyPercentFormat]
---
# Parse a constant `%` format once, at Load

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
