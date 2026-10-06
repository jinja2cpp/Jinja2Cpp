---
status: done
priority: low
area: perf
depends: [0120]
touches: [bench/, .github/workflows/benchmark.yml]
---
# Compare with the Rust template engines

**Problem.** The comparison covers Python Jinja2 and the C++ engines (0120). The Rust
engines are the other natively compiled Jinja-family implementations an embedder weighs:
MiniJinja (by Jinja2's author, used by Hugging Face's Rust LLM servers for chat templates)
and Tera (Jinja2/Django-like, v2 is a bytecode VM). We do not know where we stand
against them.

**Proposal.** An optional driver, off by default, built by cargo, that runs the cases each
engine supports and reports Load and Render next to the others in run.py.

**Done when.** bench/README.md has a table of the shared cases with the Rust engines.

**Done** in the PR that adds this line: `-DJINJA2CPP_BENCH_WITH_RUST_ENGINES=ON` builds
`bench/rust` (MiniJinja 2.24 with minijinja-contrib, Tera 2.4, pinned in `Cargo.lock`) into
`rust_engines_bench`, which takes engines_bench's flags and prints Google Benchmark JSON;
`run.py --engines` now repeats. Tera's dialect needs `<name>.tera` translations, added for
the seven cases that translate without macros or namespaces. Results and what each engine
cannot run: bench/README.md "Rust engines".
