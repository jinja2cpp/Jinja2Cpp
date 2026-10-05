# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (16 so far, latest [b31499d](https://github.com/jinja2cpp/Jinja2Cpp/commit/b31499d07f4f45b0e253ab732c478377f3c1086e) on 2026-10-05). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 855,064 |  |  | 585 |  |  |
| [Render/chat_llama](#renderchat_llama) | 980,345 |  |  | 980 |  |  |
| [Load/chat_mistral](#loadchat_mistral) | 1,012,128 |  |  | 687 |  |  |
| [Render/chat_mistral](#renderchat_mistral) | 1,208,249 |  |  | 1,046 |  |  |
| [Load/chat_qwen](#loadchat_qwen) | 635,644 |  |  | 458 |  |  |
| [Render/chat_qwen](#renderchat_qwen) | 721,167 |  |  | 848 |  |  |
| [Load/config_file](#loadconfig_file) | 235,579 |  |  | 202 |  |  |
| [Render/config_file](#renderconfig_file) | 2,795,355 |  |  | 2,713 |  |  |
| [Load/dict_ops](#loaddict_ops) | 81,874 | -0.10% | -44.60% | 88 | 0% | -32.82% |
| [Render/dict_ops](#renderdict_ops) | 476,692 | -0.09% | -31.00% | 323 | 0% | -28.22% |
| [Load/expressions](#loadexpressions) | 105,279 | +0.17% | -48.93% | 103 | 0% | -29.45% |
| [Render/expressions](#renderexpressions) | 640,229 | -0.03% | -29.35% | 10 | 0% | -66.67% |
| [Load/filters](#loadfilters) | 163,223 | +0.25% | -40.52% | 153 | 0% | -32.89% |
| [Render/filters](#renderfilters) | 81,077 | -0.15% | -8.56% | 63 | 0% | -14.86% |
| [Load/for_filter_if](#loadfor_filter_if) | 71,227 | +0.25% | -47.39% | 80 | 0% | -34.96% |
| [Render/for_filter_if](#renderfor_filter_if) | 667,726 | +0.01% | -22.85% | 210 | 0% | -7.08% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 89,392 | +0.34% | -50.52% | 92 | 0% | -41.40% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 784,818 | -0.22% | -26.84% | 507 | 0% | -2.87% |
| [Load/for_range](#loadfor_range) | 34,288 | -0.02% | -39.51% | 47 | 0% | -16.07% |
| [Render/for_range](#renderfor_range) | 67,490 | -0.04% | -26.81% | 10 | 0% | -62.96% |
| [Load/html_autoescape](#loadhtml_autoescape) | 459,449 |  |  | 379 |  |  |
| [Render/html_autoescape](#renderhtml_autoescape) | 3,269,443 |  |  | 2,998 |  |  |
| [Load/inheritance](#loadinheritance) | 59,483 | +0.11% | -47.88% | 68 | 0% | -36.45% |
| [Render/inheritance](#renderinheritance) | 503,252 | 0% | -32.71% | 315 | 0% | -49.36% |
| [Load/large_static](#loadlarge_static) | 606,858 | +0.25% | -31.50% | 244 | 0% | -44.55% |
| [Render/large_static](#renderlarge_static) | 31,027 | +0.11% | -58.71% | 4 | 0% | -77.78% |
| [Load/macros](#loadmacros) | 123,491 | +0.46% | -45.83% | 127 | 0% | -31.72% |
| [Render/macros](#rendermacros) | 1,427,515 | -0.11% | -44.30% | 412 | 0% | -84.90% |
| [Load/many_tags](#loadmany_tags) | 22,089,272 | -0.02% | -49.98% | 16,559 | 0% | -51.65% |
| [Render/many_tags](#rendermany_tags) | 1,861,649 | 0% | -10.63% | 5 | 0% | -76.19% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 125,744 | -0.70% | -47.07% | 133 | 0% | -27.72% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 9,627,481 | 0% | -22.08% | 3,030 | 0% | -25.24% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 178,682 |  |  | 176 |  |  |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 10,575,762 |  |  | 3,051 |  |  |
| [Load/plain_text](#loadplain_text) | 9,581 | 0% | -0.98% | 18 | 0% | 0% |
| [Render/plain_text](#renderplain_text) | 2,656 | 0% | -60.91% | 4 | 0% | -66.67% |
| [Load/strings](#loadstrings) | 158,756 | +0.31% | -45.60% | 142 | 0% | -36.61% |
| [Render/strings](#renderstrings) | 1,877,869 | -1.07% | -10.47% | 1,990 | 0% | -5.87% |
| [Load/substitute](#loadsubstitute) | 19,951 | 0% | -33.18% | 33 | 0% | -10.81% |
| [Render/substitute](#rendersubstitute) | 4,599 | 0% | -47.82% | 4 | 0% | -66.67% |

## Charts

### Load/chat_llama

![Load/chat_llama](charts/Load_chat_llama.svg)

### Render/chat_llama

![Render/chat_llama](charts/Render_chat_llama.svg)

### Load/chat_mistral

![Load/chat_mistral](charts/Load_chat_mistral.svg)

### Render/chat_mistral

![Render/chat_mistral](charts/Render_chat_mistral.svg)

### Load/chat_qwen

![Load/chat_qwen](charts/Load_chat_qwen.svg)

### Render/chat_qwen

![Render/chat_qwen](charts/Render_chat_qwen.svg)

### Load/config_file

![Load/config_file](charts/Load_config_file.svg)

### Render/config_file

![Render/config_file](charts/Render_config_file.svg)

### Load/dict_ops

![Load/dict_ops](charts/Load_dict_ops.svg)

### Render/dict_ops

![Render/dict_ops](charts/Render_dict_ops.svg)

### Load/expressions

![Load/expressions](charts/Load_expressions.svg)

### Render/expressions

![Render/expressions](charts/Render_expressions.svg)

### Load/filters

![Load/filters](charts/Load_filters.svg)

### Render/filters

![Render/filters](charts/Render_filters.svg)

### Load/for_filter_if

![Load/for_filter_if](charts/Load_for_filter_if.svg)

### Render/for_filter_if

![Render/for_filter_if](charts/Render_for_filter_if.svg)

### Load/for_loop_vars

![Load/for_loop_vars](charts/Load_for_loop_vars.svg)

### Render/for_loop_vars

![Render/for_loop_vars](charts/Render_for_loop_vars.svg)

### Load/for_range

![Load/for_range](charts/Load_for_range.svg)

### Render/for_range

![Render/for_range](charts/Render_for_range.svg)

### Load/html_autoescape

![Load/html_autoescape](charts/Load_html_autoescape.svg)

### Render/html_autoescape

![Render/html_autoescape](charts/Render_html_autoescape.svg)

### Load/inheritance

![Load/inheritance](charts/Load_inheritance.svg)

### Render/inheritance

![Render/inheritance](charts/Render_inheritance.svg)

### Load/large_static

![Load/large_static](charts/Load_large_static.svg)

### Render/large_static

![Render/large_static](charts/Render_large_static.svg)

### Load/macros

![Load/macros](charts/Load_macros.svg)

### Render/macros

![Render/macros](charts/Render_macros.svg)

### Load/many_tags

![Load/many_tags](charts/Load_many_tags.svg)

### Render/many_tags

![Render/many_tags](charts/Render_many_tags.svg)

### Load/mitsuhiko_table

![Load/mitsuhiko_table](charts/Load_mitsuhiko_table.svg)

### Render/mitsuhiko_table

![Render/mitsuhiko_table](charts/Render_mitsuhiko_table.svg)

### Load/mitsuhiko_table_wide

![Load/mitsuhiko_table_wide](charts/Load_mitsuhiko_table_wide.svg)

### Render/mitsuhiko_table_wide

![Render/mitsuhiko_table_wide](charts/Render_mitsuhiko_table_wide.svg)

### Load/plain_text

![Load/plain_text](charts/Load_plain_text.svg)

### Render/plain_text

![Render/plain_text](charts/Render_plain_text.svg)

### Load/strings

![Load/strings](charts/Load_strings.svg)

### Render/strings

![Render/strings](charts/Render_strings.svg)

### Load/substitute

![Load/substitute](charts/Load_substitute.svg)

### Render/substitute

![Render/substitute](charts/Render_substitute.svg)
