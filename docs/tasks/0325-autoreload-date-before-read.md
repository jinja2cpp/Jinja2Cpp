---
status: in-progress
priority: low
area: robustness
touches: [src/template_env.cpp]
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/863
---
# A reload can cache the new date with the old text

**Problem.** Found by the 0223 scenario suite (row C6, about 1 run in 40 under TSan).
`LoadTemplate` with `autoReload` reads the file, then asks the handler for its date
(src/template_env.cpp). A change that lands between the two is cached as "up to date" with the
old text, and stays unseen until the file changes again. Python's `FileSystemLoader.get_source`
has the same window (it stats after reading), so this is not a parity bug.

**Proposal.** Ask for the date before opening the stream, so that a change during the read makes
the next lookup reload. A handler without dates is unaffected.

**Done when.** The C6 row can assert that every change is eventually seen without the final
extra change, and still passes 50 runs under TSan.

**Resolution.** `LoadTemplate` asks the handler for the date before it opens the stream (only when the cache is
on), so a cached date is never newer than the cached text. The C6 row now asserts that the last change of the loop
is seen without an extra change. `A5_ChangeDuringReadIsSeenNext` makes the change from the
open hook deterministically (v0, then v1). The date is now asked of every handler whose prefix matches, before the
open, also of one that lacks the file: a handler must answer `nullopt` for a name it does not have (the built-in
ones do), as one that throws fails the lookup. A handler that has no date before the read (the Python module's loader,
whose `uptodate` comes with `get_source`) is asked again after it, as before, and keeps the old window.
