# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (91 so far, latest [2c45ff2](https://github.com/jinja2cpp/Jinja2Cpp/commit/2c45ff20531a6af0b39cae56a2267e85937d280f) on 2026-10-16). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 355,893 | +0.02% |  | 59 | 0% |  | 18,064 | 0% | -53.78% |
| [Render/chat_llama](#renderchat_llama) | 307,426 | +0.01% |  | 262 | 0% |  | 10,232 | 0% | -28.75% |
| [Load/chat_mistral](#loadchat_mistral) | 411,404 | 0% |  | 52 | 0% |  | 20,664 | 0% | -53.72% |
| [Render/chat_mistral](#renderchat_mistral) | 516,181 | 0% |  | 450 | 0% |  | 9,072 | 0% | -31.31% |
| [Load/chat_qwen](#loadchat_qwen) | 276,081 | +0.01% |  | 45 | 0% |  | 14,304 | 0% | -47.66% |
| [Render/chat_qwen](#renderchat_qwen) | 292,972 | 0% |  | 249 | 0% |  | 7,824 | 0% | -8.00% |
| [Load/config_file](#loadconfig_file) | 100,557 | -0.02% |  | 27 | 0% |  | 5,320 | 0% | -49.62% |
| [Render/config_file](#renderconfig_file) | 1,331,274 | +0.02% |  | 1,069 | 0% |  | 23,304 | 0% | -2.12% |
| [Load/dict_ops](#loaddict_ops) | 33,646 | +0.04% | -77.23% | 9 | 0% | -93.13% | 2,144 | 0% | -49.62% |
| [Render/dict_ops](#renderdict_ops) | 301,769 | 0% | -56.32% | 112 | 0% | -75.11% | 24,408 | 0% | -29.65% |
| [Load/expressions](#loadexpressions) | 45,247 | 0% | -78.05% | 16 | 0% | -89.04% | 2,712 | 0% | -55.80% |
| [Render/expressions](#renderexpressions) | 447,723 | 0% | -50.59% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 72,941 | +0.08% | -73.42% | 29 | 0% | -87.28% | 3,656 | 0% | -67.97% |
| [Render/filters](#renderfilters) | 30,965 | +0.02% | -65.08% | 29 | 0% | -60.81% | 1,376 | 0% | -71.99% |
| [Load/for_filter_if](#loadfor_filter_if) | 30,366 | -0.02% | -77.57% | 7 | 0% | -94.31% | 1,888 | 0% | -45.24% |
| [Render/for_filter_if](#renderfor_filter_if) | 270,529 | 0% | -68.74% | 4 | 0% | -98.23% | 13,328 | 0% | +266.15% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 40,849 | -0.09% | -77.39% | 10 | 0% | -93.63% | 2,336 | 0% | -48.32% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 194,246 | 0% | -81.89% | 2 | 0% | -99.62% | 1,552 | 0% | -43.93% |
| [Load/for_range](#loadfor_range) | 13,744 | 0% | -75.75% | 7 | 0% | -87.50% | 1,088 | 0% | -45.60% |
| [Render/for_range](#renderfor_range) | 38,642 | 0% | -58.09% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/html_autoescape](#loadhtml_autoescape) | 198,925 | +0.01% |  | 50 | 0% |  | 12,304 | 0% | -50.15% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,260,179 | +0.02% |  | 747 | 0% |  | 42,864 | 0% | +6.14% |
| [Load/inheritance](#loadinheritance) | 23,275 | 0% | -79.60% | 7 | 0% | -93.46% | 1,408 | 0% | -50.97% |
| [Render/inheritance](#renderinheritance) | 135,048 | 0% | -81.94% | 34 | 0% | -94.53% | 6,512 | 0% | +8.24% |
| [Load/large_static](#loadlarge_static) | 156,382 | 0% | -82.35% | 18 | 0% | -95.91% | 42,720 | 0% | -19.98% |
| [Render/large_static](#renderlarge_static) | 21,100 | 0% | -71.92% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [Load/macros](#loadmacros) | 56,040 | -0.01% | -75.42% | 28 | 0% | -84.95% | 4,296 | 0% | -30.89% |
| [Render/macros](#rendermacros) | 644,241 | 0% | -74.86% | 308 | 0% | -88.71% | 18,848 | 0% | +28.04% |
| [Load/many_tags](#loadmany_tags) | 9,194,931 | +0.02% | -79.18% | 945 | 0% | -97.24% | 421,312 | 0% | -60.07% |
| [Render/many_tags](#rendermany_tags) | 729,430 | +0.08% | -64.98% | 2 | 0% | -90.48% | 5,632 | 0% | -10.77% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 47,453 | 0% | -80.03% | 20 | 0% | -89.13% | 3,312 | 0% | -45.38% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,117,039 | 0% | -58.59% | 33 | 0% | -99.19% | 373,296 | 0% | +7.58% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 66,496 | 0% |  | 45 | 0% |  | 5,640 | 0% | -31.75% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,462,402 | 0% |  | 33 | 0% |  | 1,406,688 | 0% | +1.86% |
| [Load/plain_text](#loadplain_text) | 3,689 | 0% | -61.88% | 4 | 0% | -77.78% | 688 | 0% | -38.57% |
| [Render/plain_text](#renderplain_text) | 1,001 | 0% | -85.27% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [Load/strings](#loadstrings) | 73,107 | +0.03% | -74.95% | 30 | 0% | -86.61% | 4,216 | 0% | -53.36% |
| [Render/strings](#renderstrings) | 600,394 | +0.05% | -71.38% | 449 | 0% | -78.76% | 9,104 | 0% | -49.22% |
| [Load/substitute](#loadsubstitute) | 7,784 | 0% | -73.93% | 6 | 0% | -83.78% | 832 | 0% | -45.26% |
| [Render/substitute](#rendersubstitute) | 2,228 | 0% | -74.72% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 704 | +3.07% | 695 | +0.29% | 0 |  | 7,597 | +8.93% |
| Render/chat_llama | 1,103 | -3.50% | 434 | -1.59% | 0 |  | 3,230 | -8.52% |
| Load/chat_mistral | 1,023 | +1.59% | 975 | +0.21% | 0 |  | 8,502 | +6.77% |
| Render/chat_mistral | 1,694 | -5.68% | 478 | +1.06% | 0 |  | 9,335 | -4.45% |
| Load/chat_qwen | 531 | +1.34% | 598 | -0.33% | 0 |  | 4,689 | +9.92% |
| Render/chat_qwen | 1,063 | +0.95% | 368 | -0.54% | 0 |  | 2,829 | -1.74% |
| Load/config_file | 190 | +0.53% | 175 | +4.17% | 0 |  | 2,854 | +4.54% |
| Render/config_file | 5,834 | -5.95% | 1,619 | -0.67% | 0 |  | 39,663 | -5.64% |
| Load/dict_ops | 28 | -3.45% | 19 | -5.00% | 0 |  | 1,249 | +2.38% |
| Render/dict_ops | 660 | +2.64% | 425 | +0.71% | 0 |  | 844 | +0.24% |
| Load/expressions | 41 | +24.24% | 36 | +24.14% | 0 |  | 1,177 | -4.15% |
| Render/expressions | 8 | -27.27% | 3 | -40.00% | 0 |  | 279 | +4.10% |
| Load/filters | 131 | -7.09% | 74 | 0% | 0 |  | 2,056 | +10.60% |
| Render/filters | 13 | -27.78% | 3 | -40.00% | 0 |  | 804 | +1.90% |
| Load/for_filter_if | 16 | +14.29% | 10 | -9.09% | 0 |  | 1,044 | +2.86% |
| Render/for_filter_if | 1,719 | -0.12% | 65 | -1.52% | 0 |  | 155 | -10.40% |
| Load/for_loop_vars | 39 | +21.88% | 16 | -15.79% | 0 |  | 1,050 | +2.94% |
| Render/for_loop_vars | 5 | -28.57% | 2 | -33.33% | 0 |  | 178 | +35.88% |
| Load/for_range | 5 | -37.50% | 8 | -20.00% | 0 |  | 698 | -1.13% |
| Render/for_range | 1 | 0% | 1 | 0% | 0 |  | 89 | -25.21% |
| Load/html_autoescape | 444 | -0.89% | 495 | 0% | 0 |  | 3,884 | +1.36% |
| Render/html_autoescape | 4,882 | -5.00% | 1,549 | -12.78% | 0 |  | 36,996 | +2.12% |
| Load/inheritance | 3 | 0% | 2 | 0% | 0 |  | 624 | +5.58% |
| Render/inheritance | 589 | -0.51% | 132 | +2.33% | 0 |  | 753 | -1.95% |
| Load/large_static | 1,948 | -0.41% | 886 | 0% | 0 |  | 244 | -8.96% |
| Render/large_static | 709 | 0% | 571 | 0% | 0 |  | 16 | +700.00% |
| Load/macros | 74 | 0% | 50 | -7.41% | 0 |  | 1,565 | 0% |
| Render/macros | 757 | +0.53% | 242 | +0.83% | 0 |  | 1,101 | +32.81% |
| Load/many_tags | 27,177 | +1.30% | 20,657 | +0.21% | 0 |  | 253,275 | +6.44% |
| Render/many_tags | 6,052 | +0.05% | 879 | +0.57% | 0 |  | 147 | -24.23% |
| Load/mitsuhiko_table | 23 | +15.00% | 20 | +5.26% | 0 |  | 765 | -0.52% |
| Render/mitsuhiko_table | 9,297 | +0.05% | 6,622 | +0.17% | 0 |  | 271 | -10.86% |
| Load/mitsuhiko_table_wide | 82 | -13.68% | 117 | +4.46% | 0 |  | 1,127 | -3.01% |
| Render/mitsuhiko_table_wide | 9,385 | -0.03% | 22,915 | -0.01% | 0 |  | 276 | -3.16% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 5 | -16.67% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 5 | -16.67% |
| Load/strings | 174 | +1.16% | 109 | -3.54% | 0 |  | 2,688 | +0.22% |
| Render/strings | 494 | -1.00% | 606 | 0% | 0 |  | 1,453 | +1.25% |
| Load/substitute | 2 | 0% | 3 | 0% | 0 |  | 97 | -21.77% |
| Render/substitute | 0 |  | 1 | 0% | 0 |  | 3 | 0% |

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
