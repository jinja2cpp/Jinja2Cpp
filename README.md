# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (36 so far, latest [c05ee13](https://github.com/jinja2cpp/Jinja2Cpp/commit/c05ee135c37c84f0c62e93963e7e0a5e2e7921c0) on 2026-10-06). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 669,737 | +0.08% |  | 403 | 0% |  | 39,456 | -0.04% | +0.96% |
| [Render/chat_llama](#renderchat_llama) | 427,693 | +0.03% |  | 349 | 0% |  | 13,320 | 0% | -7.24% |
| [Load/chat_mistral](#loadchat_mistral) | 778,902 | +0.09% |  | 459 | 0% |  | 45,456 | +0.07% | +1.81% |
| [Render/chat_mistral](#renderchat_mistral) | 746,710 | -0.04% |  | 502 | 0% |  | 11,688 | -0.14% | -11.51% |
| [Load/chat_qwen](#loadchat_qwen) | 500,921 | -0.14% |  | 312 | 0% |  | 27,816 | -0.06% | +1.79% |
| [Render/chat_qwen](#renderchat_qwen) | 421,782 | -0.04% |  | 359 | 0% |  | 7,464 | 0% | -12.23% |
| [Load/config_file](#loadconfig_file) | 174,074 | -0.20% |  | 133 | 0% |  | 10,632 | -0.15% | +0.68% |
| [Render/config_file](#renderconfig_file) | 2,144,316 | -0.10% |  | 1,845 | 0% |  | 21,464 | 0% | -9.85% |
| [Load/dict_ops](#loaddict_ops) | 62,521 | +0.84% | -57.69% | 61 | 0% | -53.44% | 4,312 | 0% | +1.32% |
| [Render/dict_ops](#renderdict_ops) | 413,759 | -0.07% | -40.11% | 317 | 0% | -29.56% | 34,120 | -0.05% | -1.66% |
| [Load/expressions](#loadexpressions) | 85,023 | +0.38% | -58.76% | 82 | 0% | -43.84% | 6,288 | 0% | +2.48% |
| [Render/expressions](#renderexpressions) | 555,170 | 0% | -38.73% | 4 | 0% | -86.67% | 2,576 | 0% | -27.31% |
| [Load/filters](#loadfilters) | 128,028 | -0.89% | -53.35% | 114 | 0% | -50.00% | 11,520 | +0.70% | +0.91% |
| [Render/filters](#renderfilters) | 63,507 | +0.02% | -28.38% | 47 | 0% | -36.49% | 4,336 | 0% | -11.73% |
| [Load/for_filter_if](#loadfor_filter_if) | 52,699 | -0.56% | -61.08% | 56 | 0% | -54.47% | 3,504 | 0% | +1.62% |
| [Render/for_filter_if](#renderfor_filter_if) | 518,250 | 0% | -40.12% | 207 | 0% | -8.41% | 2,656 | 0% | -27.03% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 68,618 | -0.01% | -62.02% | 67 | 0% | -57.32% | 4,560 | 0% | +0.88% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 616,618 | +0.24% | -42.52% | 503 | 0% | -3.64% | 1,728 | 0% | -37.57% |
| [Load/for_range](#loadfor_range) | 28,098 | 0% | -50.43% | 37 | 0% | -33.93% | 2,040 | 0% | +2.00% |
| [Render/for_range](#renderfor_range) | 51,607 | 0% | -44.03% | 4 | 0% | -85.19% | 544 | 0% | -64.02% |
| [Load/html_autoescape](#loadhtml_autoescape) | 358,224 | -0.20% |  | 274 | 0% |  | 25,008 | +0.06% | +1.33% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,824,109 | -0.49% |  | 1,074 | 0% |  | 38,688 | 0% | -4.20% |
| [Load/inheritance](#loadinheritance) | 45,509 | +0.05% | -60.12% | 51 | 0% | -52.34% | 2,912 | 0% | +1.39% |
| [Render/inheritance](#renderinheritance) | 283,630 | +0.18% | -62.07% | 155 | 0% | -75.08% | 3,824 | 0% | -36.44% |
| [Load/large_static](#loadlarge_static) | 301,011 | -0.15% | -66.03% | 193 | 0% | -56.14% | 53,376 | -0.03% | -0.01% |
| [Render/large_static](#renderlarge_static) | 32,000 | +0.07% | -57.41% | 2 | 0% | -88.89% | 36,656 | 0% | -1.55% |
| [Load/macros](#loadmacros) | 97,855 | -0.15% | -57.07% | 95 | 0% | -48.92% | 6,272 | 0% | +0.90% |
| [Render/macros](#rendermacros) | 1,180,746 | +0.03% | -53.93% | 408 | 0% | -85.05% | 13,680 | 0% | -7.07% |
| [Load/many_tags](#loadmany_tags) | 16,461,395 | -0.69% | -62.73% | 9,659 | 0% | -71.80% | 1,060,048 | -0.01% | +0.46% |
| [Render/many_tags](#rendermany_tags) | 1,293,880 | 0% | -37.88% | 3 | 0% | -85.71% | 5,736 | 0% | -9.13% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 98,608 | -0.23% | -58.49% | 95 | 0% | -48.37% | 6,152 | 0% | +1.45% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 7,263,506 | 0% | -41.22% | 1,026 | 0% | -74.69% | 345,968 | 0% | -0.30% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 125,856 | +0.02% |  | 132 | 0% |  | 8,320 | 0% | +0.68% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 7,614,756 | 0% |  | 1,047 | 0% |  | 1,379,952 | 0% | -0.08% |
| [Load/plain_text](#loadplain_text) | 9,006 | 0% | -6.92% | 16 | 0% | -11.11% | 1,144 | 0% | +2.14% |
| [Render/plain_text](#renderplain_text) | 2,333 | 0% | -65.66% | 2 | 0% | -83.33% | 144 | 0% | -80.00% |
| [Load/strings](#loadstrings) | 123,920 | +0.28% | -57.54% | 106 | 0% | -52.68% | 9,368 | 0% | +3.63% |
| [Render/strings](#renderstrings) | 1,114,040 | +0.01% | -46.89% | 1,083 | 0% | -48.77% | 17,352 | 0% | -3.21% |
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
