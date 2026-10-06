# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (35 so far, latest [04c111d](https://github.com/jinja2cpp/Jinja2Cpp/commit/04c111dccf3a128b98c19959569f8b9ffec37e22) on 2026-10-06). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 669,227 | +0.03% |  | 403 | 0% |  | 39,472 | +0.16% | +1.00% |
| [Render/chat_llama](#renderchat_llama) | 427,547 | -1.08% |  | 349 | -2.79% |  | 13,320 | -3.37% | -7.24% |
| [Load/chat_mistral](#loadchat_mistral) | 778,183 | -0.02% |  | 459 | 0% |  | 45,424 | +0.25% | +1.74% |
| [Render/chat_mistral](#renderchat_mistral) | 746,980 | -1.69% |  | 502 | -3.28% |  | 11,704 | -7.23% | -11.39% |
| [Load/chat_qwen](#loadchat_qwen) | 501,616 | -0.01% |  | 312 | 0% |  | 27,832 | +0.06% | +1.84% |
| [Render/chat_qwen](#renderchat_qwen) | 421,966 | -1.32% |  | 359 | -3.23% |  | 7,464 | -5.85% | -12.23% |
| [Load/config_file](#loadconfig_file) | 174,420 | +0.31% |  | 133 | 0% |  | 10,648 | +0.60% | +0.83% |
| [Render/config_file](#renderconfig_file) | 2,146,453 | -3.21% |  | 1,845 | -9.25% |  | 21,464 | -5.13% | -9.85% |
| [Load/dict_ops](#loaddict_ops) | 62,000 | -0.74% | -58.04% | 61 | 0% | -53.44% | 4,312 | +0.75% | +1.32% |
| [Render/dict_ops](#renderdict_ops) | 414,046 | +0.29% | -40.07% | 317 | -0.63% | -29.56% | 34,136 | 0% | -1.61% |
| [Load/expressions](#loadexpressions) | 84,704 | +0.03% | -58.91% | 82 | 0% | -43.84% | 6,288 | +0.26% | +2.48% |
| [Render/expressions](#renderexpressions) | 555,175 | -0.07% | -38.73% | 4 | -33.33% | -86.67% | 2,576 | -13.21% | -27.31% |
| [Load/filters](#loadfilters) | 129,178 | 0% | -52.93% | 114 | 0% | -50.00% | 11,440 | 0% | +0.21% |
| [Render/filters](#renderfilters) | 63,497 | +0.05% | -28.39% | 47 | 0% | -36.49% | 4,336 | 0% | -11.73% |
| [Load/for_filter_if](#loadfor_filter_if) | 52,993 | +0.10% | -60.86% | 56 | 0% | -54.47% | 3,504 | +0.46% | +1.62% |
| [Render/for_filter_if](#renderfor_filter_if) | 518,225 | -0.19% | -40.12% | 207 | -0.48% | -8.41% | 2,656 | -13.32% | -27.03% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 68,625 | +0.44% | -62.01% | 67 | 0% | -57.32% | 4,560 | 0% | +0.88% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 615,120 | -0.69% | -42.66% | 503 | -0.40% | -3.64% | 1,728 | -21.17% | -37.57% |
| [Load/for_range](#loadfor_range) | 28,098 | +0.09% | -50.43% | 37 | 0% | -33.93% | 2,040 | +0.79% | +2.00% |
| [Render/for_range](#renderfor_range) | 51,607 | -1.38% | -44.03% | 4 | -33.33% | -85.19% | 544 | -41.88% | -64.02% |
| [Load/html_autoescape](#loadhtml_autoescape) | 358,958 | +0.08% |  | 274 | 0% |  | 24,992 | +0.32% | +1.26% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,833,092 | +0.36% |  | 1,074 | -0.56% |  | 38,688 | -1.19% | -4.20% |
| [Load/inheritance](#loadinheritance) | 45,487 | +0.11% | -60.14% | 51 | 0% | -52.34% | 2,912 | +0.55% | +1.39% |
| [Render/inheritance](#renderinheritance) | 283,121 | -0.41% | -62.14% | 155 | -1.27% | -75.08% | 3,824 | -10.82% | -36.44% |
| [Load/large_static](#loadlarge_static) | 301,465 | 0% | -65.97% | 193 | 0% | -56.14% | 53,392 | 0% | +0.01% |
| [Render/large_static](#renderlarge_static) | 31,979 | 0% | -57.44% | 2 | 0% | -88.89% | 36,656 | 0% | -1.55% |
| [Load/macros](#loadmacros) | 98,006 | +0.04% | -57.01% | 95 | 0% | -48.92% | 6,272 | +0.26% | +0.90% |
| [Render/macros](#rendermacros) | 1,180,426 | -0.11% | -53.95% | 408 | -0.49% | -85.05% | 13,680 | -3.28% | -7.07% |
| [Load/many_tags](#loadmany_tags) | 16,575,519 | +0.05% | -62.47% | 9,659 | 0% | -71.80% | 1,060,112 | 0% | +0.46% |
| [Render/many_tags](#rendermany_tags) | 1,293,876 | 0% | -37.88% | 3 | 0% | -85.71% | 5,736 | 0% | -9.13% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 98,833 | +0.18% | -58.40% | 95 | 0% | -48.37% | 6,152 | +0.79% | +1.45% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 7,263,543 | -6.54% | -41.22% | 1,026 | -66.12% | -74.69% | 345,968 | -0.13% | -0.30% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 125,833 | +0.05% |  | 132 | 0% |  | 8,320 | +0.58% | +0.68% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 7,614,488 | -6.26% |  | 1,047 | -65.66% |  | 1,379,952 | -0.03% | -0.08% |
| [Load/plain_text](#loadplain_text) | 9,006 | 0% | -6.92% | 16 | 0% | -11.11% | 1,144 | 0% | +2.14% |
| [Render/plain_text](#renderplain_text) | 2,333 | 0% | -65.66% | 2 | 0% | -83.33% | 144 | 0% | -80.00% |
| [Load/strings](#loadstrings) | 123,577 | -0.11% | -57.66% | 106 | 0% | -52.68% | 9,368 | +0.34% | +3.63% |
| [Render/strings](#renderstrings) | 1,113,910 | -0.10% | -46.89% | 1,083 | -0.37% | -48.77% | 17,352 | 0% | -3.21% |
| [Load/substitute](#loadsubstitute) | 17,109 | 0% | -42.70% | 27 | 0% | -27.03% | 1,544 | 0% | +1.58% |
| [Render/substitute](#rendersubstitute) | 4,111 | 0% | -53.36% | 2 | 0% | -83.33% | 144 | 0% | -80.00% |

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
