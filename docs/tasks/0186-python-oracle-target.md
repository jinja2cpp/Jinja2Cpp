---
status: open
priority: medium
area: parity
touches: [test/parity/requirements.txt, .github/workflows/parity-expectations.yml, docs/parity.md]
---
# Which Python and Jinja2 the parity oracle targets, and how users choose

**Problem.** "Python Jinja2 is the oracle" leaves the version implicit. Today it is pinned
in three places that can drift apart: `jinja2==3.1.6` in `test/parity/requirements.txt`,
Python 3.12 in `parity-expectations.yml` (and `fuzz.yml`, `benchmark.yml`), and the Unicode
version baked into the generated headers (`src/unicode_printable.h`, `unicode_tables.h`,
`unicode_case.h`, `html_entities.h`; Python 3.12 = Unicode 15.0, while 3.13 ships 15.1).
Users cannot tell which Python their output matches, and cannot ask for another one
(raised by Ruslan 2026-10-12/13 in the 0048 thread, PR #502).

**Questions to decide.**
- Policy: follow the latest stable Python and Jinja2 release, the oldest supported one, or
  a pinned pair moved deliberately (who bumps it, and how a bump reaches the corpus and
  the tables in one PR).
- Whether users may target another pair: a CMake option that selects one of several
  pre-generated table sets (`-DJINJA2CPP_PYTHON_TARGET=3.12|3.13`), so builds stay
  reproducible and need no Python, versus a single target documented per release.
- Behaviour that differs between Jinja2 versions (not only Unicode data): how a
  version-dependent case is expressed in the corpus.
- Where the chosen pair is stated for users (`docs/parity.md`, the docs site, a
  `JINJA2CPP_PYTHON_TARGET` macro in `version.h`).

**Related.** 0187 (acquiring generated and third-party data). The alternative of deriving
tables from pinned Unicode data files instead of from Python itself is recorded there.

**Done when.** The policy is written down in `docs/parity.md`, one place names the target
pair and every pin follows it, and a bump is a documented one-PR procedure.

**Release decision (Ruslan 2026-10-18, D7 in docs/release-2.0.md).** 2.0 ships one pinned
pair (Python 3.12 + Jinja2 3.1.6) as the default. Ruslan wants a separate design pass on CMake
machinery that lets a build target a Jinja2 version other than the bundled one; that is the
open half of this task.
