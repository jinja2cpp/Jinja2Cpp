---
status: open
priority: medium
area: perf
touches: [src/out_stream.h, src/template_impl.h#GenericStreamWriter, src/renderer.h#RawTextRenderer]
---
# Output: one virtual call and one append per fragment

**Problem.** After 0088/0100 the evaluator is no longer the largest cost of a table-like
render; writing the output is. On `Render/mitsuhiko_table` (1.30x Python, the closest
case) callgrind on master cf927ef puts about 27% of instructions under output:
`RawTextRenderer::Render` 14%, `GenericStreamWriter::WriteValue` -> `ValueRenderer` 13%,
both ending in `OutStream::WriteBuffer` (a virtual call through `StreamWriter`) and
`std::string::_M_append` (capacity check, then `memcpy`) for every fragment, most of
them a few bytes. The contradiction: the writer is an interface so that a render can
target a narrow string, a wide string or a user stream, but the common case (render to
`std::string`) pays for that generality per fragment.

**Proposal.** Measure each step with `bench/count.py --baseline`:
1. A concrete fast path for the string target: `OutStream` holds the target buffer
   directly when the writer is the string writer (no virtual call), keeping the
   interface for other targets.
2. Write through a growable buffer with a cheap bump-pointer append (for example
   `fmt::basic_memory_buffer<CharT, N>` or a hand-rolled one): the output reserve from
   0100 already sizes it, so most appends are a bounds check and a `memcpy`.
3. Raw text known at Load: store it as a view into the template source rather than a
   separate string, and merge adjacent static fragments.
4. Values: format numbers straight into the buffer (`fmt::format_to` with a back
   inserter of the buffer), no temporary string (see 0114 for the formatting side).

**Done when.** `Render/mitsuhiko_table` instructions drop by at least 10% with no other
case slower than 1%, rendering to a user stream unchanged.

**Next.** With output cheap, the remaining mitsuhiko cost is name lookup and the user
data adapter (0115); slot resolution (0117) addresses the first.
