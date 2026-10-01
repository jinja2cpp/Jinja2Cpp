---
status: done
priority: high
area: process
depends: []
touches: [test/parity/divergences.txt, test/parity/update_divergences.py, test/parity/parity_test.cpp, test/parity/generate.py, docs/parity.md, docs/tasks/README.md, test/parity/README.md]
---
# Parity PRs collide in shared generated files

**Problem.** Every parity PR edits two shared files: `test/parity/divergences.txt` (one
sorted list for all areas, where neighbouring lines belong to different tasks) and the
summary table in `docs/parity.md` (one snapshot of `generate.py --report` that every PR
rewrites). In wave 1 (#297, #298, #299, #300, #301) eight of the ten PR pairs conflicted
in these files, although only two pairs touched the same source lines. `git` cannot merge
the table at all, and in `divergences.txt` one PR rewriting a line's kind or task next to
another PR's deletion is a conflict too, so the README's claim that "line deletions in
different places merge cleanly" does not hold. A naive union of the two sides keeps the
stale line next to the new one. The result is that parity PRs can only land one at a
time (merge master, re-run the corpus, push, a full CI run each) or through a hand-built
integration branch, which is what wave 1 needed.

**Proposal.**
1. Split `divergences.txt` into one file per corpus area (`divergences/<area>.txt`),
   matching `cases/<area>.py` and `expected/<area>.json`. Two PRs then conflict only when
   they change cases in the same area, and the updater keeps working per file.
2. Stop committing the summary snapshot: have `docs/parity.md` say how to produce it
   (`generate.py --report`) or generate it in a post-merge job, and keep only prose and
   per-feature rows in the doc.
3. Give `divergences.txt` (or the per-area files) a `merge=union` attribute only if
   step 1 is not done; union alone keeps stale duplicates, so the updater would also need
   to reject a case listed twice.
4. Correct the claim in `docs/tasks/README.md` and the merge advice in
   `test/parity/README.md`.

**Done when** two parity PRs that fix cases in different areas merge with no textual
conflict, and the parity suite fails on a case id listed twice.

**Resolution.** Steps 1, 2 and 4 landed with the wave 2 integration branch: the
allow-list is `test/parity/divergences/<area>.txt`, `ParityRegistry.DivergencesAreWellFormed`
fails on an id listed twice or filed under the wrong area, parity PRs leave the summary
table and corpus size in `docs/parity.md` to the integration branch, and the README
files describe the wave process. Step 3 (`merge=union`) was not needed.
