---
status: done
priority: low
area: perf
depends: []
touches: [src/template_parser.h#GetLocationDescr, src/template_parser.h#ParseErrorsToErrorInfo, test/errors_test.cpp]
shares: [src/template_parser.h]
---
# Error reporting is quadratic for many errors on one long line

**Problem.** Found by the verifier on PR #316 (task 0028), present on master before it.
`ParseErrorsToErrorInfo` builds a location description for every parse error, and
`GetLocationDescr` copies the whole source line into it and streams it character by
character. A template with many errors on one line costs O(errors × line length):
`{% ( %}` repeated 10 000 times on one line (70 KB) takes about 4.5 s to fail loading,
20 000 times about 29 s, with the same timings on master 5c6e1fa and on #316.
Separate lines (`{% ( %}\n` repeated) stay fast. The splitter itself is linear (#316
also stops `FindBlockEnd` and `FindStringEnd` rescanning to the end per tag).

**Proposal.** Cap the reported errors (Jinja2 stops at the first syntax error), or
print a bounded window of the line around the column instead of the whole line, and
write the line with one `write` instead of per-character `operator<<`. Keep the
existing message format for short lines so `errors_test.cpp` stays as is.

**Done when.** Loading `{% ( %}` × 20 000 on one line fails in well under a second.

**Resolution.** `ParseErrorsToErrorInfo` describes only the first error, the only one
`Template::Load` returns (Jinja2 also stops at the first syntax error), and
`GetLocationDescr` builds the string with bulk appends instead of per-character streaming.
A line longer than 160 characters is shown as a 120-character window around the column with
`...` at the cut ends; shorter lines keep the old format. `{% ( %}` x 40 000 on one line now
fails in about 0.4 s in a Debug build (`ErrorsLongLineTest` in `test/errors_test.cpp`).
