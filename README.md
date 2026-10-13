# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (87 so far, latest [a6e9573](https://github.com/jinja2cpp/Jinja2Cpp/commit/a6e9573fbef068cf80c21dd880ee86b1efe3606d) on 2026-10-13). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 355,752 | -0.29% |  | 59 | 0% |  | 18,064 | 0% | -53.78% |
| [Render/chat_llama](#renderchat_llama) | 308,395 | +0.02% |  | 262 | 0% |  | 10,232 | 0% | -28.75% |
| [Load/chat_mistral](#loadchat_mistral) | 411,428 | -0.32% |  | 52 | 0% |  | 20,664 | 0% | -53.72% |
| [Render/chat_mistral](#renderchat_mistral) | 515,759 | +0.16% |  | 450 | 0% |  | 9,072 | +0.35% | -31.31% |
| [Load/chat_qwen](#loadchat_qwen) | 276,065 | -0.25% |  | 45 | 0% |  | 14,304 | 0% | -47.66% |
| [Render/chat_qwen](#renderchat_qwen) | 293,627 | +0.02% |  | 249 | 0% |  | 7,824 | 0% | -8.00% |
| [Load/config_file](#loadconfig_file) | 100,501 | -0.24% |  | 27 | 0% |  | 5,320 | 0% | -49.62% |
| [Render/config_file](#renderconfig_file) | 1,337,546 | +0.40% |  | 1,072 | +0.09% |  | 23,664 | +1.13% | -0.60% |
| [Load/dict_ops](#loaddict_ops) | 33,623 | -0.21% | -77.25% | 9 | 0% | -93.13% | 2,144 | 0% | -49.62% |
| [Render/dict_ops](#renderdict_ops) | 302,461 | +0.10% | -56.22% | 112 | 0% | -75.11% | 24,408 | 0% | -29.65% |
| [Load/expressions](#loadexpressions) | 45,253 | -0.12% | -78.05% | 16 | 0% | -89.04% | 2,712 | 0% | -55.80% |
| [Render/expressions](#renderexpressions) | 447,715 | 0% | -50.59% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 72,683 | 0% | -73.52% | 29 | 0% | -87.28% | 3,656 | 0% | -67.97% |
| [Render/filters](#renderfilters) | 31,582 | -0.02% | -64.38% | 29 | 0% | -60.81% | 1,376 | 0% | -71.99% |
| [Load/for_filter_if](#loadfor_filter_if) | 30,365 | -0.28% | -77.57% | 7 | 0% | -94.31% | 1,888 | 0% | -45.24% |
| [Render/for_filter_if](#renderfor_filter_if) | 270,529 | -0.05% | -68.74% | 4 | 0% | -98.23% | 13,328 | 0% | +266.15% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 40,841 | -0.26% | -77.39% | 10 | 0% | -93.63% | 2,336 | 0% | -48.32% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 194,246 | 0% | -81.89% | 2 | 0% | -99.62% | 1,552 | 0% | -43.93% |
| [Load/for_range](#loadfor_range) | 13,750 | -0.24% | -75.74% | 7 | 0% | -87.50% | 1,088 | 0% | -45.60% |
| [Render/for_range](#renderfor_range) | 38,634 | +0.02% | -58.10% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/html_autoescape](#loadhtml_autoescape) | 198,875 | -0.09% |  | 50 | 0% |  | 12,304 | 0% | -50.15% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,258,070 | +0.25% |  | 749 | +0.13% |  | 43,136 | +0.35% | +6.81% |
| [Load/inheritance](#loadinheritance) | 23,281 | -0.52% | -79.60% | 7 | 0% | -93.46% | 1,408 | 0% | -50.97% |
| [Render/inheritance](#renderinheritance) | 134,987 | +0.26% | -81.95% | 34 | +6.25% | -94.53% | 6,512 | +1.12% | +8.24% |
| [Load/large_static](#loadlarge_static) | 156,388 | -0.12% | -82.35% | 18 | 0% | -95.91% | 42,720 | 0% | -19.98% |
| [Render/large_static](#renderlarge_static) | 21,100 | +0.02% | -71.92% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [Load/macros](#loadmacros) | 56,046 | +0.14% | -75.41% | 28 | 0% | -84.95% | 4,296 | +0.37% | -30.89% |
| [Render/macros](#rendermacros) | 642,221 | +0.81% | -74.94% | 309 | +0.32% | -88.68% | 19,032 | +0.46% | +29.29% |
| [Load/many_tags](#loadmany_tags) | 9,184,437 | -0.31% | -79.20% | 945 | 0% | -97.24% | 421,312 | 0% | -60.07% |
| [Render/many_tags](#rendermany_tags) | 718,030 | +0.04% | -65.53% | 2 | 0% | -90.48% | 5,632 | 0% | -10.77% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 47,459 | -0.19% | -80.02% | 20 | 0% | -89.13% | 3,312 | 0% | -45.38% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,117,015 | +0.10% | -58.59% | 33 | 0% | -99.19% | 373,296 | 0% | +7.58% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 66,502 | -0.19% |  | 45 | 0% |  | 5,640 | 0% | -31.75% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,462,378 | +0.09% |  | 33 | 0% |  | 1,406,688 | 0% | +1.86% |
| [Load/plain_text](#loadplain_text) | 3,695 | +0.16% | -61.81% | 4 | 0% | -77.78% | 688 | 0% | -38.57% |
| [Render/plain_text](#renderplain_text) | 1,001 | +0.40% | -85.27% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [Load/strings](#loadstrings) | 73,102 | -0.16% | -74.95% | 30 | 0% | -86.61% | 4,216 | 0% | -53.36% |
| [Render/strings](#renderstrings) | 598,136 | +0.28% | -71.48% | 449 | 0% | -78.76% | 9,104 | 0% | -49.22% |
| [Load/substitute](#loadsubstitute) | 7,790 | -0.13% | -73.91% | 6 | 0% | -83.78% | 832 | 0% | -45.26% |
| [Render/substitute](#rendersubstitute) | 2,228 | +0.18% | -74.72% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 668 | 0% | 690 | +1.17% | 0 |  | 7,363 | -1.09% |
| Render/chat_llama | 1,112 | +0.27% | 425 | -0.70% | 0 |  | 3,082 | -8.52% |
| Load/chat_mistral | 979 | -1.41% | 968 | +0.31% | 0 |  | 8,139 | -3.89% |
| Render/chat_mistral | 1,739 | +3.39% | 442 | 0% | 0 |  | 8,904 | -3.43% |
| Load/chat_qwen | 510 | -0.39% | 602 | +0.17% | 0 |  | 4,502 | -3.97% |
| Render/chat_qwen | 1,059 | +4.96% | 356 | +3.79% | 0 |  | 2,801 | +2.38% |
| Load/config_file | 183 | +1.67% | 166 | -1.19% | 0 |  | 2,872 | -3.59% |
| Render/config_file | 6,248 | +9.36% | 1,650 | +13.64% | 0 |  | 42,175 | +6.89% |
| Load/dict_ops | 33 | -10.81% | 23 | -14.81% | 0 |  | 1,264 | +0.48% |
| Render/dict_ops | 648 | -1.07% | 416 | 0% | 0 |  | 854 | +2.52% |
| Load/expressions | 42 | +13.51% | 37 | 0% | 0 |  | 1,237 | +3.43% |
| Render/expressions | 12 | -14.29% | 5 | -28.57% | 0 |  | 286 | +7.52% |
| Load/filters | 131 | 0% | 77 | +5.48% | 0 |  | 1,969 | +2.02% |
| Render/filters | 37 | +32.14% | 6 | +50.00% | 0 |  | 801 | -1.23% |
| Load/for_filter_if | 11 | -21.43% | 9 | -18.18% | 0 |  | 1,045 | -5.26% |
| Render/for_filter_if | 1,726 | +0.29% | 63 | +5.00% | 0 |  | 182 | +17.42% |
| Load/for_loop_vars | 25 | -10.71% | 20 | +11.11% | 0 |  | 1,030 | -6.28% |
| Render/for_loop_vars | 5 | 0% | 2 | +100.00% | 0 |  | 113 | -16.91% |
| Load/for_range | 7 | -36.36% | 8 | -20.00% | 0 |  | 681 | -6.46% |
| Render/for_range | 1 | 0% | 1 | 0% | 0 |  | 71 | -22.83% |
| Load/html_autoescape | 434 | -0.69% | 502 | +1.83% | 0 |  | 4,015 | -8.08% |
| Render/html_autoescape | 3,857 | -7.11% | 1,359 | +1.12% | 0 |  | 37,497 | +5.62% |
| Load/inheritance | 5 | -16.67% | 5 | 0% | 0 |  | 605 | -4.57% |
| Render/inheritance | 587 | +4.63% | 121 | -6.20% | 0 |  | 753 | +3.29% |
| Load/large_static | 1,949 | -0.20% | 885 | -0.67% | 0 |  | 303 | +13.06% |
| Render/large_static | 707 | +0.43% | 571 | -0.17% | 0 |  | 4 | -73.33% |
| Load/macros | 81 | 0% | 50 | -3.85% | 0 |  | 1,600 | -1.84% |
| Render/macros | 765 | +0.66% | 238 | -0.42% | 0 |  | 1,344 | -31.74% |
| Load/many_tags | 26,777 | -1.00% | 20,602 | +0.07% | 0 |  | 236,134 | -7.24% |
| Render/many_tags | 6,045 | -0.20% | 852 | -0.35% | 0 |  | 125 | -2.34% |
| Load/mitsuhiko_table | 25 | +4.17% | 22 | -4.35% | 0 |  | 786 | -5.76% |
| Render/mitsuhiko_table | 9,269 | -0.02% | 6,609 | +0.02% | 0 |  | 291 | -7.03% |
| Load/mitsuhiko_table_wide | 76 | -13.64% | 113 | 0% | 0 |  | 1,290 | +8.04% |
| Render/mitsuhiko_table_wide | 9,376 | +0.15% | 22,908 | +0.01% | 0 |  | 305 | +3.39% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 14 | +133.33% |
| Render/plain_text | 0 |  | 0 | -100.00% | 0 |  | 5 | 0% |
| Load/strings | 150 | -2.60% | 108 | +17.39% | 0 |  | 2,752 | -0.58% |
| Render/strings | 498 | +1.84% | 604 | -0.98% | 0 |  | 1,435 | -36.34% |
| Load/substitute | 2 | 0% | 3 | 0% | 0 |  | 136 | +4.62% |
| Render/substitute | 0 |  | 1 |  | 0 |  | 4 | -63.64% |

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
