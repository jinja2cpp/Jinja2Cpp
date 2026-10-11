# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (74 so far, latest [5ce1786](https://github.com/jinja2cpp/Jinja2Cpp/commit/5ce178646df92b299da77ba4e9715613e338b159) on 2026-10-12). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 349,444 | -4.00% |  | 59 | -22.37% |  | 18,064 | -3.30% | -53.78% |
| [Render/chat_llama](#renderchat_llama) | 311,034 | -16.60% |  | 262 | -37.77% |  | 10,232 | -19.00% | -28.75% |
| [Load/chat_mistral](#loadchat_mistral) | 418,411 | -2.09% |  | 56 | -11.11% |  | 21,464 | -0.92% | -51.93% |
| [Render/chat_mistral](#renderchat_mistral) | 500,901 | -10.63% |  | 360 | -26.68% |  | 8,536 | -3.35% | -35.37% |
| [Load/chat_qwen](#loadchat_qwen) | 270,303 | -4.21% |  | 45 | -21.05% |  | 14,304 | -3.35% | -47.66% |
| [Render/chat_qwen](#renderchat_qwen) | 298,371 | -16.01% |  | 249 | -36.96% |  | 7,824 | -11.73% | -8.00% |
| [Load/config_file](#loadconfig_file) | 99,535 | -0.66% |  | 27 | 0% |  | 5,320 | 0% | -49.62% |
| [Render/config_file](#renderconfig_file) | 1,335,687 | -5.98% |  | 1,071 | -16.39% |  | 23,384 | -0.14% | -1.78% |
| [Load/dict_ops](#loaddict_ops) | 33,237 | -0.46% | -77.51% | 9 | 0% | -93.13% | 2,144 | 0% | -49.62% |
| [Render/dict_ops](#renderdict_ops) | 300,824 | -1.20% | -56.46% | 112 | 0% | -75.11% | 24,408 | 0% | -29.65% |
| [Load/expressions](#loadexpressions) | 44,015 | -0.85% | -78.65% | 16 | 0% | -89.04% | 2,712 | 0% | -55.80% |
| [Render/expressions](#renderexpressions) | 446,287 | -0.01% | -50.75% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 71,527 | -0.98% | -73.94% | 29 | 0% | -87.28% | 3,656 | 0% | -67.97% |
| [Render/filters](#renderfilters) | 34,339 | -26.50% | -61.27% | 29 | -43.14% | -60.81% | 1,376 | 0% | -71.99% |
| [Load/for_filter_if](#loadfor_filter_if) | 30,079 | 0% | -77.78% | 7 | 0% | -94.31% | 1,888 | 0% | -45.24% |
| [Render/for_filter_if](#renderfor_filter_if) | 275,621 | -0.83% | -68.15% | 4 | 0% | -98.23% | 13,328 | 0% | +266.15% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 40,673 | -0.56% | -77.49% | 10 | 0% | -93.63% | 2,336 | 0% | -48.32% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 193,760 | -0.11% | -81.94% | 2 | 0% | -99.62% | 1,552 | 0% | -43.93% |
| [Load/for_range](#loadfor_range) | 13,712 | -0.07% | -75.81% | 7 | 0% | -87.50% | 1,088 | 0% | -45.60% |
| [Render/for_range](#renderfor_range) | 38,193 | -0.12% | -58.58% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/html_autoescape](#loadhtml_autoescape) | 197,087 | -1.00% |  | 50 | -3.85% |  | 12,304 | -0.52% | -50.15% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,251,405 | -10.46% |  | 748 | -31.63% |  | 42,984 | 0% | +6.44% |
| [Load/inheritance](#loadinheritance) | 23,179 | -1.38% | -79.69% | 7 | 0% | -93.46% | 1,408 | 0% | -50.97% |
| [Render/inheritance](#renderinheritance) | 135,545 | -0.53% | -81.88% | 32 | 0% | -94.86% | 6,440 | 0% | +7.05% |
| [Load/large_static](#loadlarge_static) | 154,211 | 0% | -82.59% | 18 | 0% | -95.91% | 42,720 | 0% | -19.98% |
| [Render/large_static](#renderlarge_static) | 21,090 | -0.05% | -71.93% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [Load/macros](#loadmacros) | 55,299 | -0.86% | -75.74% | 28 | 0% | -84.95% | 4,280 | 0% | -31.15% |
| [Render/macros](#rendermacros) | 653,434 | -4.98% | -74.51% | 308 | 0% | -88.71% | 18,944 | 0% | +28.70% |
| [Load/many_tags](#loadmany_tags) | 9,069,799 | -0.88% | -79.46% | 945 | 0% | -97.24% | 421,280 | 0% | -60.08% |
| [Render/many_tags](#rendermany_tags) | 810,831 | -11.41% | -61.07% | 2 | 0% | -90.48% | 5,632 | 0% | -10.77% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 46,758 | -1.86% | -80.32% | 20 | 0% | -89.13% | 3,312 | 0% | -45.38% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,090,130 | -0.47% | -58.81% | 33 | 0% | -99.19% | 373,296 | 0% | +7.58% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 66,105 | -3.47% |  | 45 | -11.76% |  | 5,640 | -3.29% | -31.75% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,435,975 | -0.44% |  | 33 | 0% |  | 1,406,688 | 0% | +1.86% |
| [Load/plain_text](#loadplain_text) | 3,667 | 0% | -62.10% | 4 | 0% | -77.78% | 688 | 0% | -38.57% |
| [Render/plain_text](#renderplain_text) | 995 | -0.50% | -85.36% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [Load/strings](#loadstrings) | 72,002 | -1.26% | -75.33% | 30 | 0% | -86.61% | 4,216 | 0% | -53.36% |
| [Render/strings](#renderstrings) | 583,336 | -36.80% | -72.19% | 449 | -65.06% | -78.76% | 9,104 | -5.87% | -49.22% |
| [Load/substitute](#loadsubstitute) | 7,684 | 0% | -74.26% | 6 | 0% | -83.78% | 832 | 0% | -45.26% |
| [Render/substitute](#rendersubstitute) | 2,199 | -0.45% | -75.05% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 670 | -11.26% | 694 | -1.00% | 0 |  | 7,181 | -2.55% |
| Render/chat_llama | 1,084 | -3.21% | 407 | +9.70% | 0 |  | 3,197 | -16.79% |
| Load/chat_mistral | 1,081 | -2.79% | 1,024 | 0% | 0 |  | 8,435 | +3.64% |
| Render/chat_mistral | 1,757 | -3.88% | 419 | -1.41% | 0 |  | 8,598 | -5.13% |
| Load/chat_qwen | 531 | -9.23% | 587 | -2.17% | 0 |  | 4,373 | -0.55% |
| Render/chat_qwen | 1,012 | -4.98% | 298 | -9.97% | 0 |  | 2,999 | -4.55% |
| Load/config_file | 160 | -1.23% | 135 | +2.27% | 0 |  | 2,754 | +1.85% |
| Render/config_file | 5,367 | -3.82% | 1,333 | -15.15% | 0 |  | 39,044 | -5.03% |
| Load/dict_ops | 22 | +29.41% | 14 | 0% | 0 |  | 1,243 | +2.14% |
| Render/dict_ops | 600 | +0.33% | 399 | +0.25% | 0 |  | 826 | -0.84% |
| Load/expressions | 16 | -11.11% | 14 | -22.22% | 0 |  | 1,158 | +1.67% |
| Render/expressions | 6 | +100.00% | 2 | +100.00% | 0 |  | 290 | +14.17% |
| Load/filters | 112 | -3.45% | 82 | -7.87% | 0 |  | 1,871 | -4.44% |
| Render/filters | 18 | +38.46% | 3 | +50.00% | 0 |  | 849 | -8.41% |
| Load/for_filter_if | 11 | -15.38% | 6 | 0% | 0 |  | 1,042 | +10.03% |
| Render/for_filter_if | 1,584 | +0.32% | 61 | +7.02% | 0 |  | 145 | -2.68% |
| Load/for_loop_vars | 20 | +42.86% | 9 | +28.57% | 0 |  | 1,116 | +9.09% |
| Render/for_loop_vars | 8 | +100.00% | 3 | +200.00% | 0 |  | 113 | -4.24% |
| Load/for_range | 5 | +25.00% | 5 | +25.00% | 0 |  | 697 | -1.27% |
| Render/for_range | 1 | 0% | 1 | 0% | 0 |  | 74 | +2.78% |
| Load/html_autoescape | 433 | -2.26% | 494 | +0.82% | 0 |  | 4,181 | +8.06% |
| Render/html_autoescape | 2,765 | -2.12% | 1,065 | -5.25% | 0 |  | 35,756 | -9.69% |
| Load/inheritance | 5 | +400.00% | 4 | +33.33% | 0 |  | 559 | -5.73% |
| Render/inheritance | 547 | -1.44% | 117 | +2.63% | 0 |  | 759 | +4.40% |
| Load/large_static | 1,952 | -0.15% | 881 | 0% | 0 |  | 260 | +1.17% |
| Render/large_static | 706 | 0% | 573 | -0.35% | 0 |  | 4 | +33.33% |
| Load/macros | 74 | -1.33% | 69 | +4.55% | 0 |  | 1,579 | -1.44% |
| Render/macros | 754 | +0.40% | 233 | -0.43% | 0 |  | 679 | -55.03% |
| Load/many_tags | 26,505 | -0.21% | 20,566 | -0.14% | 0 |  | 235,392 | -0.40% |
| Render/many_tags | 6,037 | +0.27% | 689 | +0.15% | 0 |  | 138 | -95.98% |
| Load/mitsuhiko_table | 23 | -17.86% | 19 | -24.00% | 0 |  | 762 | +2.56% |
| Render/mitsuhiko_table | 9,274 | +0.09% | 6,497 | +0.08% | 0 |  | 292 | +9.77% |
| Load/mitsuhiko_table_wide | 92 | -14.02% | 111 | -11.20% | 0 |  | 1,138 | -3.72% |
| Render/mitsuhiko_table_wide | 9,357 | -0.29% | 22,964 | -0.01% | 0 |  | 285 | -9.24% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 7 | -53.33% |
| Render/plain_text | 0 |  | 0 | -100.00% | 0 |  | 5 | 0% |
| Load/strings | 144 | +5.11% | 108 | +4.85% | 0 |  | 2,673 | +0.83% |
| Render/strings | 473 | -42.39% | 601 | -50.90% | 0 |  | 1,432 | -6.89% |
| Load/substitute | 1 | 0% | 3 | 0% | 0 |  | 118 | -14.49% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 3 | -25.00% |

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
