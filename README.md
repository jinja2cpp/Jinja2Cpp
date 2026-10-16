# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (92 so far, latest [72e7e8e](https://github.com/jinja2cpp/Jinja2Cpp/commit/72e7e8e73ee27f830476790f467f0c20297a7dfb) on 2026-10-16). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 355,900 | 0% |  | 59 | 0% |  | 18,064 | 0% | -53.78% |
| [Render/chat_llama](#renderchat_llama) | 307,330 | -0.03% |  | 262 | 0% |  | 10,232 | 0% | -28.75% |
| [Load/chat_mistral](#loadchat_mistral) | 411,403 | 0% |  | 52 | 0% |  | 20,664 | 0% | -53.72% |
| [Render/chat_mistral](#renderchat_mistral) | 515,713 | -0.09% |  | 450 | 0% |  | 9,072 | 0% | -31.31% |
| [Load/chat_qwen](#loadchat_qwen) | 276,038 | -0.02% |  | 45 | 0% |  | 14,304 | 0% | -47.66% |
| [Render/chat_qwen](#renderchat_qwen) | 292,894 | -0.03% |  | 249 | 0% |  | 7,824 | 0% | -8.00% |
| [Load/config_file](#loadconfig_file) | 100,537 | -0.02% |  | 27 | 0% |  | 5,320 | 0% | -49.62% |
| [Render/config_file](#renderconfig_file) | 1,250,411 | -6.07% |  | 1,046 | -2.15% |  | 23,752 | +1.92% | -0.24% |
| [Load/dict_ops](#loaddict_ops) | 33,638 | -0.02% | -77.24% | 9 | 0% | -93.13% | 2,144 | 0% | -49.62% |
| [Render/dict_ops](#renderdict_ops) | 301,230 | -0.18% | -56.40% | 112 | 0% | -75.11% | 24,408 | 0% | -29.65% |
| [Load/expressions](#loadexpressions) | 45,253 | +0.01% | -78.05% | 16 | 0% | -89.04% | 2,712 | 0% | -55.80% |
| [Render/expressions](#renderexpressions) | 447,708 | 0% | -50.59% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 72,917 | -0.03% | -73.43% | 29 | 0% | -87.28% | 3,656 | 0% | -67.97% |
| [Render/filters](#renderfilters) | 30,971 | +0.02% | -65.07% | 29 | 0% | -60.81% | 1,376 | 0% | -71.99% |
| [Load/for_filter_if](#loadfor_filter_if) | 30,372 | +0.02% | -77.57% | 7 | 0% | -94.31% | 1,888 | 0% | -45.24% |
| [Render/for_filter_if](#renderfor_filter_if) | 269,537 | -0.37% | -68.86% | 4 | 0% | -98.23% | 13,328 | 0% | +266.15% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 40,848 | 0% | -77.39% | 10 | 0% | -93.63% | 2,336 | 0% | -48.32% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 194,234 | -0.01% | -81.89% | 2 | 0% | -99.62% | 1,552 | 0% | -43.93% |
| [Load/for_range](#loadfor_range) | 13,750 | +0.04% | -75.74% | 7 | 0% | -87.50% | 1,088 | 0% | -45.60% |
| [Render/for_range](#renderfor_range) | 38,627 | -0.04% | -58.11% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/html_autoescape](#loadhtml_autoescape) | 198,903 | -0.01% |  | 50 | 0% |  | 12,304 | 0% | -50.15% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,220,945 | -3.11% |  | 734 | -1.74% |  | 43,728 | +2.02% | +8.28% |
| [Load/inheritance](#loadinheritance) | 23,281 | +0.03% | -79.60% | 7 | 0% | -93.46% | 1,408 | 0% | -50.97% |
| [Render/inheritance](#renderinheritance) | 131,300 | -2.78% | -82.44% | 34 | 0% | -94.53% | 6,560 | +0.74% | +9.04% |
| [Load/large_static](#loadlarge_static) | 156,388 | 0% | -82.35% | 18 | 0% | -95.91% | 42,720 | 0% | -19.98% |
| [Render/large_static](#renderlarge_static) | 21,046 | -0.26% | -71.99% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [Load/macros](#loadmacros) | 56,046 | +0.01% | -75.41% | 28 | 0% | -84.95% | 4,296 | 0% | -30.89% |
| [Render/macros](#rendermacros) | 642,427 | -0.28% | -74.94% | 308 | 0% | -88.71% | 18,848 | 0% | +28.04% |
| [Load/many_tags](#loadmany_tags) | 9,194,937 | 0% | -79.18% | 945 | 0% | -97.24% | 421,312 | 0% | -60.07% |
| [Render/many_tags](#rendermany_tags) | 729,661 | +0.03% | -64.97% | 2 | 0% | -90.48% | 5,632 | 0% | -10.77% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 47,459 | +0.01% | -80.02% | 20 | 0% | -89.13% | 3,312 | 0% | -45.38% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,107,009 | -0.20% | -58.67% | 33 | 0% | -99.19% | 373,296 | 0% | +7.58% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 66,496 | 0% |  | 45 | 0% |  | 5,640 | 0% | -31.75% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,452,372 | -0.18% |  | 33 | 0% |  | 1,406,688 | 0% | +1.86% |
| [Load/plain_text](#loadplain_text) | 3,695 | +0.16% | -61.81% | 4 | 0% | -77.78% | 688 | 0% | -38.57% |
| [Render/plain_text](#renderplain_text) | 1,000 | -0.10% | -85.28% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [Load/strings](#loadstrings) | 73,090 | -0.02% | -74.96% | 30 | 0% | -86.61% | 4,216 | 0% | -53.36% |
| [Render/strings](#renderstrings) | 599,658 | -0.12% | -71.41% | 449 | 0% | -78.76% | 9,104 | 0% | -49.22% |
| [Load/substitute](#loadsubstitute) | 7,790 | +0.08% | -73.91% | 6 | 0% | -83.78% | 832 | 0% | -45.26% |
| [Render/substitute](#rendersubstitute) | 2,219 | -0.40% | -74.83% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 698 | -0.85% | 693 | -0.29% | 0 |  | 6,907 | -9.08% |
| Render/chat_llama | 1,102 | -0.09% | 432 | -0.46% | 0 |  | 3,185 | -1.39% |
| Load/chat_mistral | 1,017 | -0.59% | 979 | +0.41% | 0 |  | 7,469 | -12.15% |
| Render/chat_mistral | 1,762 | +4.01% | 478 | 0% | 0 |  | 8,659 | -7.24% |
| Load/chat_qwen | 520 | -2.07% | 597 | -0.17% | 0 |  | 4,195 | -10.54% |
| Render/chat_qwen | 1,034 | -2.73% | 368 | 0% | 0 |  | 2,678 | -5.34% |
| Load/config_file | 188 | -1.05% | 174 | -0.57% | 0 |  | 2,797 | -2.00% |
| Render/config_file | 5,236 | -10.25% | 1,272 | -21.43% | 0 |  | 36,218 | -8.69% |
| Load/dict_ops | 27 | -3.57% | 20 | +5.26% | 0 |  | 1,237 | -0.96% |
| Render/dict_ops | 656 | -0.61% | 427 | +0.47% | 0 |  | 833 | -1.30% |
| Load/expressions | 40 | -2.44% | 34 | -5.56% | 0 |  | 1,186 | +0.76% |
| Render/expressions | 11 | +37.50% | 6 | +100.00% | 0 |  | 305 | +9.32% |
| Load/filters | 124 | -5.34% | 73 | -1.35% | 0 |  | 2,133 | +3.75% |
| Render/filters | 15 | +15.38% | 4 | +33.33% | 0 |  | 801 | -0.37% |
| Load/for_filter_if | 9 | -43.75% | 9 | -10.00% | 0 |  | 1,004 | -3.83% |
| Render/for_filter_if | 1,722 | +0.17% | 66 | +1.54% | 0 |  | 151 | -2.58% |
| Load/for_loop_vars | 27 | -30.77% | 15 | -6.25% | 0 |  | 1,058 | +0.76% |
| Render/for_loop_vars | 5 | 0% | 1 | -50.00% | 0 |  | 114 | -35.96% |
| Load/for_range | 5 | 0% | 8 | 0% | 0 |  | 704 | +0.86% |
| Render/for_range | 1 | 0% | 1 | 0% | 0 |  | 132 | +48.31% |
| Load/html_autoescape | 442 | -0.45% | 498 | +0.61% | 0 |  | 4,037 | +3.94% |
| Render/html_autoescape | 3,258 | -33.27% | 1,174 | -24.21% | 0 |  | 33,007 | -10.78% |
| Load/inheritance | 3 | 0% | 2 | 0% | 0 |  | 606 | -2.88% |
| Render/inheritance | 589 | 0% | 128 | -3.03% | 0 |  | 788 | +4.65% |
| Load/large_static | 1,948 | 0% | 886 | 0% | 0 |  | 288 | +18.03% |
| Render/large_static | 706 | -0.42% | 574 | +0.53% | 0 |  | 3 | -81.25% |
| Load/macros | 65 | -12.16% | 47 | -6.00% | 0 |  | 1,570 | +0.32% |
| Render/macros | 754 | -0.40% | 240 | -0.83% | 0 |  | 494 | -55.13% |
| Load/many_tags | 26,752 | -1.56% | 20,615 | -0.20% | 0 |  | 241,329 | -4.72% |
| Render/many_tags | 6,050 | -0.03% | 871 | -0.91% | 0 |  | 126 | -14.29% |
| Load/mitsuhiko_table | 22 | -4.35% | 20 | 0% | 0 |  | 761 | -0.52% |
| Render/mitsuhiko_table | 9,289 | -0.09% | 6,621 | -0.02% | 0 |  | 298 | +9.96% |
| Load/mitsuhiko_table_wide | 85 | +3.66% | 113 | -3.42% | 0 |  | 1,261 | +11.89% |
| Render/mitsuhiko_table_wide | 9,387 | +0.02% | 22,914 | 0% | 0 |  | 309 | +11.96% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 6 | +20.00% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 5 | 0% |
| Load/strings | 168 | -3.45% | 106 | -2.75% | 0 |  | 2,667 | -0.78% |
| Render/strings | 502 | +1.62% | 605 | -0.17% | 0 |  | 1,409 | -3.03% |
| Load/substitute | 2 | 0% | 3 | 0% | 0 |  | 133 | +37.11% |
| Render/substitute | 0 |  | 0 | -100.00% | 0 |  | 3 | 0% |

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
