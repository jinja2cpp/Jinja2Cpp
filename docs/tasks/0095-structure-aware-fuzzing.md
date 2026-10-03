---
status: open
priority: medium
area: robustness
depends: [0003]
touches: [fuzz/]
shares: [.github/workflows/fuzz.yml, fuzz/CMakeLists.txt]
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
