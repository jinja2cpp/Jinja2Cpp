---
status: done
priority: medium
area: perf
depends: [0011]
touches: [src/template_parser.h#FindNextMatch, src/template_parser.h#MakeDelimiters]
---
# The template splitter tries every delimiter at every byte of text

**Problem.** Found with the benchmark suite (0011) after 0086-0089 and 0088 landed.
`Load/large_static` (36 KB of text with 50 `{{ }}`) loaded only 9x faster than Python
Jinja2 while every other case was 47-150x faster. In raw text `FindNextMatch` called
`MatchTagAt` at every position, which compares each begin delimiter (`memcmp`): about
260 instructions per byte, 95% of the load.

**Resolution.** Without line statement or line comment prefixes a tag can only start
on the first character of a begin delimiter, so the scan jumps there with
`find` (one distinct first character, `memchr`) or `find_first_of`. With line prefixes
set it keeps trying every position, since those match at line starts and after spaces.
`Load/large_static` -91% instructions, `Load/mitsuhiko_table` -26%, `Load/plain_text`
-35%; renders unchanged. Test: `BasicTests.MixedDelimiterStarts`.

**Next.** `Load/many_tags` (300 statements, 38 KB) still costs 44M instructions:
`GetKeyword` is 10% self time at about 1500 instructions per call, so it seems to be
called far more often than once per identifier; worth a look together with the
expression parser's allocations (malloc/free 13%).
