---
status: open
priority: medium
area: perf
touches: [bench/cases/, bench/jinja2cpp_bench.cpp, bench/python_bench.py, bench/README.md]
---
# Benchmark workloads people actually render

**Problem.** The 14 cases in `bench/cases/` were written to isolate engine features,
so every optimisation so far was steered by synthetic templates. The largest real use
of Jinja in C++ today is LLM chat templates (Hugging Face tokenizer `chat_template`s
rendered per request by inference servers), and none of the cases looks like one. Wide
templates (`MULTISTR` is half the API) and autoescape-on HTML are not measured at all.

**Proposal.** Add cases, each shared with the Python driver:
- 3-4 real chat templates (for example Llama 3, Qwen, Mistral style, including tool
  calls), rendered over a 20-message conversation;
- an autoescaped HTML page (`autoescape=True`, Markup, `|e`);
- a wide-string variant of one table case;
- a config-file style template (many small includes and macros).
Check that the Python driver produces identical output (run.py does), and add the new
cases to the trend.

**Done when.** The cases are in `bench/cases/`, run.py passes, and the summary table in
bench/README.md lists them against Python.

**Next.** Re-profile on the new cases: they decide the order of 0113-0118.
