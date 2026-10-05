# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (22 so far, latest [580fcbf](https://github.com/jinja2cpp/Jinja2Cpp/commit/580fcbf92a6f179e62dd6a9748f7c4733f284735) on 2026-10-05). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 854,616 | 0% |  | 585 | 0% |  |
| [Render/chat_llama](#renderchat_llama) | 437,870 | -5.75% |  | 361 | 0% |  |
| [Load/chat_mistral](#loadchat_mistral) | 1,011,841 | 0% |  | 687 | 0% |  |
| [Render/chat_mistral](#renderchat_mistral) | 786,130 | -7.68% |  | 529 | -1.67% |  |
| [Load/chat_qwen](#loadchat_qwen) | 635,391 | 0% |  | 458 | 0% |  |
| [Render/chat_qwen](#renderchat_qwen) | 437,209 | -8.19% |  | 373 | 0% |  |
| [Load/config_file](#loadconfig_file) | 235,583 | 0% |  | 202 | 0% |  |
| [Render/config_file](#renderconfig_file) | 2,591,593 | -5.07% |  | 2,712 | 0% |  |
| [Load/dict_ops](#loaddict_ops) | 81,875 | 0% | -44.60% | 88 | 0% | -32.82% |
| [Render/dict_ops](#renderdict_ops) | 425,054 | -5.75% | -38.47% | 323 | 0% | -28.22% |
| [Load/expressions](#loadexpressions) | 105,275 | 0% | -48.93% | 103 | 0% | -29.45% |
| [Render/expressions](#renderexpressions) | 598,184 | 0% | -33.99% | 10 | 0% | -66.67% |
| [Load/filters](#loadfilters) | 163,243 | 0% | -40.52% | 153 | 0% | -32.89% |
| [Render/filters](#renderfilters) | 76,522 | -4.66% | -13.70% | 55 | -12.70% | -25.68% |
| [Load/for_filter_if](#loadfor_filter_if) | 71,207 | 0% | -47.41% | 80 | 0% | -34.96% |
| [Render/for_filter_if](#renderfor_filter_if) | 553,002 | -15.04% | -36.11% | 210 | 0% | -7.08% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 89,419 | 0% | -50.50% | 92 | 0% | -41.40% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 660,296 | -12.07% | -38.45% | 507 | 0% | -2.87% |
| [Load/for_range](#loadfor_range) | 34,292 | 0% | -39.50% | 47 | 0% | -16.07% |
| [Render/for_range](#renderfor_range) | 56,520 | 0% | -38.71% | 10 | 0% | -62.96% |
| [Load/html_autoescape](#loadhtml_autoescape) | 459,439 | 0% |  | 379 | 0% |  |
| [Render/html_autoescape](#renderhtml_autoescape) | 2,444,870 | -6.32% |  | 2,246 | 0% |  |
| [Load/inheritance](#loadinheritance) | 59,491 | 0% | -47.87% | 68 | 0% | -36.45% |
| [Render/inheritance](#renderinheritance) | 451,694 | -6.52% | -39.60% | 315 | 0% | -49.36% |
| [Load/large_static](#loadlarge_static) | 606,962 | 0% | -31.49% | 244 | 0% | -44.55% |
| [Render/large_static](#renderlarge_static) | 31,703 | 0% | -57.81% | 4 | 0% | -77.78% |
| [Load/macros](#loadmacros) | 123,479 | 0% | -45.83% | 127 | 0% | -31.72% |
| [Render/macros](#rendermacros) | 1,228,428 | -5.25% | -52.07% | 412 | 0% | -84.90% |
| [Load/many_tags](#loadmany_tags) | 22,090,472 | 0% | -49.98% | 16,559 | 0% | -51.65% |
| [Render/many_tags](#rendermany_tags) | 1,502,615 | -11.78% | -27.86% | 5 | 0% | -76.19% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 125,770 | 0% | -47.06% | 133 | 0% | -27.72% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 8,076,041 | -1.61% | -34.64% | 3,030 | 0% | -25.24% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 178,707 | 0% |  | 176 | 0% |  |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 9,086,914 | -1.43% |  | 3,051 | 0% |  |
| [Load/plain_text](#loadplain_text) | 9,585 | 0% | -0.94% | 18 | 0% | 0% |
| [Render/plain_text](#renderplain_text) | 2,702 | 0% | -60.23% | 4 | 0% | -66.67% |
| [Load/strings](#loadstrings) | 158,766 | 0% | -45.60% | 142 | 0% | -36.61% |
| [Render/strings](#renderstrings) | 1,215,355 | -8.11% | -42.06% | 1,091 | -1.27% | -48.39% |
| [Load/substitute](#loadsubstitute) | 19,955 | 0% | -33.16% | 33 | 0% | -10.81% |
| [Render/substitute](#rendersubstitute) | 4,465 | 0% | -49.34% | 4 | 0% | -66.67% |

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
