# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (85 so far, latest [4cbbb8e](https://github.com/jinja2cpp/Jinja2Cpp/commit/4cbbb8e2ee4e4f762157bcc37b7b550ce8798b1c) on 2026-10-12). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 353,637 | +0.07% |  | 59 | 0% |  | 18,064 | 0% | -53.78% |
| [Render/chat_llama](#renderchat_llama) | 310,234 | 0% |  | 262 | 0% |  | 10,232 | 0% | -28.75% |
| [Load/chat_mistral](#loadchat_mistral) | 422,290 | +0.15% |  | 56 | 0% |  | 21,464 | 0% | -51.93% |
| [Render/chat_mistral](#renderchat_mistral) | 496,920 | +0.01% |  | 360 | 0% |  | 8,536 | 0% | -35.37% |
| [Load/chat_qwen](#loadchat_qwen) | 274,489 | +0.05% |  | 45 | 0% |  | 14,304 | 0% | -47.66% |
| [Render/chat_qwen](#renderchat_qwen) | 297,377 | +0.01% |  | 249 | 0% |  | 7,824 | 0% | -8.00% |
| [Load/config_file](#loadconfig_file) | 99,718 | +0.05% |  | 27 | 0% |  | 5,320 | 0% | -49.62% |
| [Render/config_file](#renderconfig_file) | 1,335,328 | -0.02% |  | 1,071 | 0% |  | 23,384 | 0% | -1.78% |
| [Load/dict_ops](#loaddict_ops) | 33,417 | +0.16% | -77.39% | 9 | 0% | -93.13% | 2,144 | 0% | -49.62% |
| [Render/dict_ops](#renderdict_ops) | 301,037 | -0.02% | -56.42% | 112 | 0% | -75.11% | 24,408 | 0% | -29.65% |
| [Load/expressions](#loadexpressions) | 44,383 | +0.05% | -78.47% | 16 | 0% | -89.04% | 2,712 | 0% | -55.80% |
| [Render/expressions](#renderexpressions) | 446,120 | +0.04% | -50.77% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 71,763 | +0.01% | -73.85% | 29 | 0% | -87.28% | 3,656 | 0% | -67.97% |
| [Render/filters](#renderfilters) | 31,527 | +0.08% | -64.44% | 29 | 0% | -60.81% | 1,376 | 0% | -71.99% |
| [Load/for_filter_if](#loadfor_filter_if) | 30,336 | +0.47% | -77.59% | 7 | 0% | -94.31% | 1,888 | 0% | -45.24% |
| [Render/for_filter_if](#renderfor_filter_if) | 276,168 | +0.20% | -68.09% | 4 | 0% | -98.23% | 13,328 | 0% | +266.15% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 40,708 | +0.06% | -77.47% | 10 | 0% | -93.63% | 2,336 | 0% | -48.32% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 193,693 | +0.05% | -81.94% | 2 | 0% | -99.62% | 1,552 | 0% | -43.93% |
| [Load/for_range](#loadfor_range) | 13,778 | +0.19% | -75.69% | 7 | 0% | -87.50% | 1,088 | 0% | -45.60% |
| [Render/for_range](#renderfor_range) | 38,126 | +0.25% | -58.65% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/html_autoescape](#loadhtml_autoescape) | 197,662 | +0.04% |  | 50 | 0% |  | 12,304 | 0% | -50.15% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,254,160 | +0.01% |  | 748 | 0% |  | 42,984 | 0% | +6.44% |
| [Load/inheritance](#loadinheritance) | 23,287 | +0.01% | -79.59% | 7 | 0% | -93.46% | 1,408 | 0% | -50.97% |
| [Render/inheritance](#renderinheritance) | 136,240 | +0.03% | -81.78% | 32 | 0% | -94.86% | 6,440 | 0% | +7.05% |
| [Load/large_static](#loadlarge_static) | 155,313 | 0% | -82.47% | 18 | 0% | -95.91% | 42,720 | 0% | -19.98% |
| [Render/large_static](#renderlarge_static) | 21,095 | 0% | -71.92% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [Load/macros](#loadmacros) | 55,588 | +0.05% | -75.61% | 28 | 0% | -84.95% | 4,280 | 0% | -31.15% |
| [Render/macros](#rendermacros) | 637,185 | +0.02% | -75.14% | 308 | 0% | -88.71% | 18,944 | 0% | +28.70% |
| [Load/many_tags](#loadmany_tags) | 9,110,823 | +0.05% | -79.37% | 945 | 0% | -97.24% | 421,280 | 0% | -60.08% |
| [Render/many_tags](#rendermany_tags) | 723,599 | 0% | -65.26% | 2 | 0% | -90.48% | 5,632 | 0% | -10.77% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 47,145 | +0.17% | -80.15% | 20 | 0% | -89.13% | 3,312 | 0% | -45.38% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,111,214 | +0.14% | -58.63% | 33 | 0% | -99.19% | 373,296 | 0% | +7.58% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 66,499 | +0.12% |  | 45 | 0% |  | 5,640 | 0% | -31.75% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,457,059 | +0.13% |  | 33 | 0% |  | 1,406,688 | 0% | +1.86% |
| [Load/plain_text](#loadplain_text) | 3,668 | 0% | -62.09% | 4 | 0% | -77.78% | 688 | 0% | -38.57% |
| [Render/plain_text](#renderplain_text) | 996 | 0% | -85.35% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [Load/strings](#loadstrings) | 72,393 | +0.13% | -75.19% | 30 | 0% | -86.61% | 4,216 | 0% | -53.36% |
| [Render/strings](#renderstrings) | 584,620 | +0.04% | -72.13% | 449 | 0% | -78.76% | 9,104 | 0% | -49.22% |
| [Load/substitute](#loadsubstitute) | 7,731 | 0% | -74.11% | 6 | 0% | -83.78% | 832 | 0% | -45.26% |
| [Render/substitute](#rendersubstitute) | 2,208 | 0% | -74.95% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 678 | +0.44% | 699 | +0.29% | 0 |  | 6,520 | -5.01% |
| Render/chat_llama | 1,064 | +0.09% | 427 | +1.18% | 0 |  | 3,324 | -1.22% |
| Load/chat_mistral | 1,077 | +0.19% | 1,019 | 0% | 0 |  | 7,329 | -5.69% |
| Render/chat_mistral | 1,766 | +0.80% | 470 | +3.75% | 0 |  | 8,754 | -4.62% |
| Load/chat_qwen | 550 | +0.18% | 593 | +0.17% | 0 |  | 3,948 | -8.19% |
| Render/chat_qwen | 926 | -0.11% | 305 | +2.69% | 0 |  | 2,745 | -3.14% |
| Load/config_file | 167 | +1.83% | 147 | +1.38% | 0 |  | 2,606 | -5.44% |
| Render/config_file | 5,485 | -0.07% | 1,460 | +1.81% | 0 |  | 39,984 | +2.40% |
| Load/dict_ops | 24 | -7.69% | 21 | -4.55% | 0 |  | 1,209 | +1.00% |
| Render/dict_ops | 608 | 0% | 409 | -0.24% | 0 |  | 829 | +2.22% |
| Load/expressions | 27 | +28.57% | 22 | +22.22% | 0 |  | 1,180 | +1.99% |
| Render/expressions | 4 | 0% | 2 | 0% | 0 |  | 261 | -10.92% |
| Load/filters | 113 | +4.63% | 84 | +1.20% | 0 |  | 1,765 | -7.64% |
| Render/filters | 34 | 0% | 7 | 0% | 0 |  | 843 | +1.44% |
| Load/for_filter_if | 10 | 0% | 5 | 0% | 0 |  | 1,017 | -2.59% |
| Render/for_filter_if | 1,580 | +0.06% | 60 | -1.64% | 0 |  | 141 | -13.50% |
| Load/for_loop_vars | 21 | -4.55% | 11 | 0% | 0 |  | 986 | -3.62% |
| Render/for_loop_vars | 12 | 0% | 5 | -16.67% | 0 |  | 105 | -2.78% |
| Load/for_range | 4 | 0% | 5 | +25.00% | 0 |  | 712 | +0.42% |
| Render/for_range | 1 | 0% | 1 | 0% | 0 |  | 101 | +2.02% |
| Load/html_autoescape | 437 | 0% | 496 | +0.20% | 0 |  | 3,486 | -9.27% |
| Render/html_autoescape | 4,153 | -1.49% | 1,279 | +4.58% | 0 |  | 38,372 | +3.83% |
| Load/inheritance | 1 | 0% | 3 | 0% | 0 |  | 581 | -6.29% |
| Render/inheritance | 568 | +0.71% | 118 | +0.85% | 0 |  | 710 | +1.00% |
| Load/large_static | 1,963 | 0% | 887 | 0% | 0 |  | 259 | +6.15% |
| Render/large_static | 705 | 0% | 573 | 0% | 0 |  | 3 | 0% |
| Load/macros | 75 | -1.32% | 62 | -1.59% | 0 |  | 1,504 | -2.59% |
| Render/macros | 855 | -0.12% | 242 | +0.41% | 0 |  | 505 | -61.51% |
| Load/many_tags | 27,068 | +0.58% | 20,604 | +0.05% | 0 |  | 220,479 | -5.57% |
| Render/many_tags | 6,038 | -0.02% | 857 | +0.12% | 0 |  | 104 | -6.31% |
| Load/mitsuhiko_table | 25 | -10.71% | 29 | 0% | 0 |  | 742 | -3.26% |
| Render/mitsuhiko_table | 9,301 | -0.01% | 6,509 | +0.09% | 0 |  | 305 | +4.10% |
| Load/mitsuhiko_table_wide | 96 | +2.13% | 113 | +2.73% | 0 |  | 1,164 | -5.83% |
| Render/mitsuhiko_table_wide | 9,396 | 0% | 22,973 | +0.01% | 0 |  | 299 | +9.52% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 21 | +61.54% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 5 | 0% |
| Load/strings | 144 | +0.70% | 117 | 0% | 0 |  | 2,579 | -0.85% |
| Render/strings | 460 | +0.22% | 602 | +0.17% | 0 |  | 1,390 | -4.01% |
| Load/substitute | 1 | 0% | 3 | 0% | 0 |  | 121 | +7.08% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 3 | 0% |

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
