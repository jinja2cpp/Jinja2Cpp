---
status: open
priority: low
area: robustness
touches: [src/template_env.cpp]
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
