# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (113 so far, latest [f0bb0cf](https://github.com/jinja2cpp/Jinja2Cpp/commit/f0bb0cf07aa38e670a6fd5446d0c9cb7b288aaaa) on 2026-10-18). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 349,498 | 0% |  | 57 | 0% |  | 18,080 | 0% | -53.74% |
| [Render/chat_llama](#renderchat_llama) | 314,888 | 0% |  | 262 | 0% |  | 10,232 | 0% | -28.75% |
| [Load/chat_mistral](#loadchat_mistral) | 400,319 | 0% |  | 50 | 0% |  | 20,696 | 0% | -53.65% |
| [Render/chat_mistral](#renderchat_mistral) | 526,448 | 0% |  | 448 | 0% |  | 9,072 | 0% | -31.31% |
| [Load/chat_qwen](#loadchat_qwen) | 268,821 | 0% |  | 44 | 0% |  | 14,320 | 0% | -47.60% |
| [Render/chat_qwen](#renderchat_qwen) | 301,135 | 0% |  | 249 | 0% |  | 7,824 | 0% | -8.00% |
| [Load/config_file](#loadconfig_file) | 100,603 | 0% |  | 27 | 0% |  | 5,336 | 0% | -49.47% |
| [Render/config_file](#renderconfig_file) | 1,291,125 | 0% |  | 1,046 | 0% |  | 23,800 | 0% | -0.03% |
| [Load/dict_ops](#loaddict_ops) | 33,712 | 0% | -77.19% | 9 | 0% | -93.13% | 2,160 | 0% | -49.25% |
| [Render/dict_ops](#renderdict_ops) | 307,555 | 0% | -55.48% | 112 | 0% | -75.11% | 24,424 | 0% | -29.61% |
| [Load/expressions](#loadexpressions) | 45,606 | 0% | -77.88% | 16 | 0% | -89.04% | 2,728 | 0% | -55.54% |
| [Render/expressions](#renderexpressions) | 451,081 | 0% | -50.22% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 73,319 | 0% | -73.28% | 29 | 0% | -87.28% | 3,672 | 0% | -67.83% |
| [Render/filters](#renderfilters) | 31,747 | 0% | -64.20% | 29 | 0% | -60.81% | 1,376 | 0% | -71.99% |
| [Load/for_filter_if](#loadfor_filter_if) | 30,411 | 0% | -77.54% | 7 | 0% | -94.31% | 1,904 | 0% | -44.78% |
| [Render/for_filter_if](#renderfor_filter_if) | 272,680 | 0% | -68.49% | 4 | 0% | -98.23% | 13,328 | 0% | +266.15% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 40,905 | 0% | -77.36% | 10 | 0% | -93.63% | 2,352 | 0% | -47.96% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 196,459 | 0% | -81.69% | 2 | 0% | -99.62% | 1,552 | 0% | -43.93% |
| [Load/for_range](#loadfor_range) | 13,790 | 0% | -75.67% | 7 | 0% | -87.50% | 1,104 | 0% | -44.80% |
| [Render/for_range](#renderfor_range) | 39,303 | 0% | -57.38% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/html_autoescape](#loadhtml_autoescape) | 198,625 | 0% |  | 50 | 0% |  | 12,320 | 0% | -50.08% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,242,690 | 0% |  | 734 | 0% |  | 43,776 | 0% | +8.40% |
| [Load/inheritance](#loadinheritance) | 23,301 | 0% | -79.58% | 7 | 0% | -93.46% | 1,424 | 0% | -50.42% |
| [Render/inheritance](#renderinheritance) | 136,490 | 0% | -81.75% | 35 | 0% | -94.37% | 6,680 | 0% | +11.04% |
| [Load/large_static](#loadlarge_static) | 156,170 | 0% | -82.37% | 18 | 0% | -95.91% | 42,736 | 0% | -19.95% |
| [Render/large_static](#renderlarge_static) | 21,065 | 0% | -71.96% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [Load/macros](#loadmacros) | 56,175 | 0% | -75.36% | 28 | 0% | -84.95% | 4,312 | 0% | -30.63% |
| [Render/macros](#rendermacros) | 649,415 | 0% | -74.66% | 308 | 0% | -88.71% | 18,848 | 0% | +28.04% |
| [Load/many_tags](#loadmany_tags) | 9,211,197 | 0% | -79.14% | 956 | 0% | -97.21% | 421,424 | 0% | -60.06% |
| [Render/many_tags](#rendermany_tags) | 734,484 | 0% | -64.74% | 2 | 0% | -90.48% | 5,632 | 0% | -10.77% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 47,556 | 0% | -79.98% | 20 | 0% | -89.13% | 3,328 | 0% | -45.12% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,185,624 | 0% | -58.03% | 33 | 0% | -99.19% | 373,296 | 0% | +7.58% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 56,454 | 0% |  | 36 | 0% |  | 5,656 | 0% | -31.56% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,530,868 | 0% |  | 33 | 0% |  | 1,406,688 | 0% | +1.86% |
| [Load/plain_text](#loadplain_text) | 3,720 | 0% | -61.56% | 4 | 0% | -77.78% | 704 | 0% | -37.14% |
| [Render/plain_text](#renderplain_text) | 1,044 | 0% | -84.64% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [Load/strings](#loadstrings) | 73,484 | 0% | -74.82% | 30 | 0% | -86.61% | 4,232 | 0% | -53.19% |
| [Render/strings](#renderstrings) | 624,503 | 0% | -70.23% | 449 | 0% | -78.76% | 9,104 | 0% | -49.22% |
| [Load/substitute](#loadsubstitute) | 7,807 | 0% | -73.85% | 6 | 0% | -83.78% | 848 | 0% | -44.21% |
| [Render/substitute](#rendersubstitute) | 2,272 | 0% | -74.23% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 713 | 0% | 723 | 0% | 0 |  | 7,210 | -2.14% |
| Render/chat_llama | 1,071 | 0% | 424 | 0% | 0 |  | 3,359 | -14.33% |
| Load/chat_mistral | 1,064 | 0% | 1,022 | 0% | 0 |  | 8,058 | +1.69% |
| Render/chat_mistral | 1,789 | 0% | 417 | 0% | 0 |  | 9,015 | -7.63% |
| Load/chat_qwen | 495 | 0% | 596 | 0% | 0 |  | 4,196 | +1.21% |
| Render/chat_qwen | 1,022 | 0% | 308 | 0% | 0 |  | 2,871 | -6.21% |
| Load/config_file | 172 | 0% | 178 | 0% | 0 |  | 2,862 | +0.10% |
| Render/config_file | 6,341 | 0% | 1,827 | 0% | 0 |  | 39,549 | +3.82% |
| Load/dict_ops | 35 | 0% | 18 | 0% | 0 |  | 1,262 | -1.87% |
| Render/dict_ops | 688 | 0% | 428 | 0% | 0 |  | 860 | 0% |
| Load/expressions | 38 | 0% | 22 | 0% | 0 |  | 1,282 | -0.54% |
| Render/expressions | 3 | 0% | 1 | 0% | 0 |  | 360 | +17.65% |
| Load/filters | 111 | 0% | 61 | 0% | 0 |  | 1,945 | -4.52% |
| Render/filters | 29 | 0% | 8 | 0% | 0 |  | 807 | -0.98% |
| Load/for_filter_if | 15 | 0% | 9 | 0% | 0 |  | 1,067 | +1.81% |
| Render/for_filter_if | 1,711 | 0% | 65 | 0% | 0 |  | 167 | +10.60% |
| Load/for_loop_vars | 34 | 0% | 24 | 0% | 0 |  | 1,033 | +1.57% |
| Render/for_loop_vars | 7 | 0% | 5 | 0% | 0 |  | 160 | +30.08% |
| Load/for_range | 3 | 0% | 5 | 0% | 0 |  | 702 | -3.04% |
| Render/for_range | 1 | 0% | 1 | 0% | 0 |  | 119 | -19.05% |
| Load/html_autoescape | 477 | 0% | 505 | 0% | 0 |  | 4,027 | +4.11% |
| Render/html_autoescape | 3,952 | 0% | 1,261 | 0% | 0 |  | 32,387 | -2.17% |
| Load/inheritance | 3 | 0% | 4 | 0% | 0 |  | 616 | -1.60% |
| Render/inheritance | 568 | 0% | 125 | 0% | 0 |  | 855 | +4.65% |
| Load/large_static | 1,946 | 0% | 893 | 0% | 0 |  | 294 | -0.34% |
| Render/large_static | 704 | 0% | 575 | 0% | 0 |  | 4 | 0% |
| Load/macros | 72 | 0% | 58 | 0% | 0 |  | 1,609 | 0% |
| Render/macros | 726 | 0% | 248 | 0% | 0 |  | 524 | -8.87% |
| Load/many_tags | 27,523 | 0% | 21,436 | 0% | 0 |  | 243,353 | -3.45% |
| Render/many_tags | 6,043 | 0% | 874 | 0% | 0 |  | 111 | -16.54% |
| Load/mitsuhiko_table | 26 | 0% | 37 | 0% | 0 |  | 814 | +1.37% |
| Render/mitsuhiko_table | 9,026 | 0% | 6,614 | 0% | 0 |  | 364 | +9.64% |
| Load/mitsuhiko_table_wide | 89 | 0% | 79 | 0% | 0 |  | 949 | -0.84% |
| Render/mitsuhiko_table_wide | 9,155 | 0% | 22,923 | 0% | 0 |  | 341 | -0.87% |
| Load/plain_text | 0 |  | 3 | 0% | 0 |  | 5 | 0% |
| Render/plain_text | 0 |  | 1 | 0% | 0 |  | 7 | 0% |
| Load/strings | 155 | 0% | 112 | 0% | 0 |  | 2,714 | -3.35% |
| Render/strings | 477 | 0% | 617 | 0% | 0 |  | 1,451 | -1.49% |
| Load/substitute | 3 | 0% | 4 | 0% | 0 |  | 182 | +30.00% |
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
