---
status: in-progress
priority: high
area: robustness
depends: [0003]
touches: [fuzz/]
shares: [.github/workflows/fuzz.yml, fuzz/CMakeLists.txt, test/parity/cases/fuzz.py, test/parity/divergences/fuzz.txt]
---
# Structure-aware fuzzing: mutate templates, not bytes

**Problem.** The fuzzers of 0003 mutate bytes, guided by coverage and a token dictionary.
Most mutants of a template break its syntax, so they stop in the lexer or the parser's
error paths. Deep, well-formed templates (a macro calling a macro inside a loop over a
filtered list, with blocks inherited from a parent) are reached only when a seed already
holds them. The differential check against Python (`fuzz/differential.py`) suffers most:
an input both engines reject teaches nothing, and byte mutants are mostly rejected.

**Proposal.** Three steps, each useful alone:
1. **Custom mutator in the existing targets** (`LLVMFuzzerCustomMutator` and
   `LLVMFuzzerCustomCrossOver`, no new dependency). Split the input with the Jinja2C++
   lexer into text, `{{ }}`, `{% %}` and `{# #}` pieces and mutate at that level: swap,
   duplicate or nest whole tags; replace an expression with one taken from the corpus or
   the dictionary; wrap a body in `if`/`for`/`with`/`macro`/`call`/`filter`; close the
   statements left open. Fall back to `LLVMFuzzerMutate` for the bytes inside a piece, so
   byte-level coverage is kept.
2. **Grammar generator for the differential check.** A Python generator of well-formed
   templates over the context in `fuzz/differential.py` (Hypothesis strategies or a
   Grammarinator grammar), run nightly against both engines. Every input is valid for
   Python, so every difference is a real divergence: it lands as a parity case, as the
   findings in 0094 did.
3. **Protobuf AST with libprotobuf-mutator**, if steps 1 and 2 plateau: a `.proto` for
   the template AST, a printer to Jinja2 text, and a target per engine entry point. This
   is the strongest structure-aware option and the one OSS-Fuzz supports directly, but
   it adds protobuf and libprotobuf-mutator to the fuzz build only.

**Done when.** The custom mutator runs in the PR and nightly jobs, and in the same time
budget reaches more coverage (`cov:` and `ft:` in the libFuzzer log) than byte mutation
alone. The nightly differential run includes generated templates, with the share of
inputs that both engines render recorded in its report.

**Release role (Ruslan 2026-10-18, D11 in docs/release-2.0.md).** Ruslan prefers extensive
structure-aware fuzz sessions to a longer RC window, so gate G8 requires an extended campaign
with this task's mutator and grammar generator on the release candidate.

**Done (steps 1 and 2).** `fuzz/template_mutator.cpp` is the custom mutator and crossover of
the seven template targets (`parse`, `render`, `render_wide`, `untrusted`, `host`, `loader`,
`alloc_failure`), with no new dependency. It splits the input with the delimiters the target's
settings byte picks (a small splitter of its own rather than the internal lexer, so it reads
unterminated and malformed tags too) and mutates whole pieces: it duplicates, swaps and deletes
balanced runs of statements, replaces expressions (from the input, the crossover partner or a
pool over the fuzz context), wraps runs in the block statements, inserts `extends`, `include`,
`import`, `break` and similar, and closes statements left open. A quarter of the mutations, and
one kind of piece mutation, go to `LLVMFuzzerMutate`. `JINJA2CPP_FUZZ_MUTATOR=bytes` (the
`mutator` input of a manual workflow run) turns it off. Pull-request fuzz jobs stay replay only
(0185), so the mutator runs there as ctest `jinja2cpp_fuzz_mutator_smoke` (`jinja2cpp_fuzz_replay
--mutate`), in every build job under every compiler; nightly it drives generation.

Measured on 2026-11-11 (clang 18, ASan+UBSan, `-fork=1`, the seeds of `make_corpus.py`, same
seed, 20 minutes per run, the byte run with `-len_control=100` as on master):

| target | mode | execs | cov | ft | ft gained |
|---|---|---|---|---|---|
| render | bytes | 735 815 | 175 640 | 171 256 | 34 235 |
| render | structure | 196 388 | 177 205 | 197 196 | 60 210 |
| untrusted | bytes | 758 780 | 208 478 | 206 261 | 44 552 |
| untrusted | structure | 232 971 | 209 286 | 232 608 | 71 092 |

With a third of the executions (its mutants are deeper templates that run longer) the
structure-aware run reaches more edges (+0.5 to 0.9 % `cov`) and many more features (+13 to
15 % `ft`, +60 to 76 % of what each run gained over the seeds). The `render` run also found 7
more timeouts than the byte run, all templates with legitimately unbounded work (nested
`range(5000)` loops built by duplication and wrapping, a loop appending to its own list); the
limits-on `untrusted` target found none in either mode.

`fuzz/generate_templates.py` is the grammar generator (plain `random`, no Hypothesis): typed
expressions over the differential context, every statement (includes and imports of the support templates, not of the input itself),
macros with defaults, `varargs` and `caller`, inheritance with block overrides, loop variables,
`namespace`, `break` and `continue`. Every template it keeps parses in Python. The nightly
differential job renders 5000 of them (the run number as seed) and records the share both
engines render; 500 also seed every fuzz job's corpus (`make_corpus.py --generated`, fixed
seed). Over 20 000 generated templates both engines rendered about 97 %, and every divergence
fell into two classes, pinned in `test/parity/cases/fuzz.py` and filed: `lower`/`upper` tests
on strings with no cased letter (0464) and dotted filter and test names (0465). The run also
showed that `differential.py` still assumed sorted `ValuesMap` keys (0043 made them ordered):
its context now keeps MakeContext's order, which removed about 200 false divergences.

**Next.** Step 3 (protobuf AST with libprotobuf-mutator) waits for steps 1 and 2 to plateau
on the nightly numbers. The grammar covers no host data and no custom delimiters yet; both are
cheap to add when its divergences dry up.
