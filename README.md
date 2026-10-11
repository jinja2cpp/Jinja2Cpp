# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (65 so far, latest [5219f69](https://github.com/jinja2cpp/Jinja2Cpp/commit/5219f698bd20219025565a75ff0e11dd3225c0af) on 2026-10-11). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 555,170 | 0% |  | 92 | 0% |  | 19,856 | 0% | -49.19% |
| [Render/chat_llama](#renderchat_llama) | 407,267 | +0.06% |  | 353 | 0% |  | 15,176 | 0% | +5.68% |
| [Load/chat_mistral](#loadchat_mistral) | 657,629 | 0% |  | 75 | 0% |  | 23,832 | 0% | -46.62% |
| [Render/chat_mistral](#renderchat_mistral) | 647,005 | -1.15% |  | 419 | -2.78% |  | 12,080 | -2.20% | -8.54% |
| [Load/chat_qwen](#loadchat_qwen) | 424,516 | 0% |  | 68 | 0% |  | 15,760 | 0% | -42.33% |
| [Render/chat_qwen](#renderchat_qwen) | 394,034 | +0.07% |  | 339 | 0% |  | 7,920 | 0% | -6.87% |
| [Load/config_file](#loadconfig_file) | 145,958 | 0% |  | 34 | 0% |  | 5,576 | 0% | -47.20% |
| [Render/config_file](#renderconfig_file) | 1,738,178 | +0.14% |  | 1,179 | 0% |  | 23,912 | +0.07% | +0.44% |
| [Load/dict_ops](#loaddict_ops) | 47,706 | 0% | -67.72% | 12 | 0% | -90.84% | 2,224 | 0% | -47.74% |
| [Render/dict_ops](#renderdict_ops) | 351,405 | -0.11% | -49.13% | 114 | 0% | -74.67% | 36,168 | 0% | +4.24% |
| [Load/expressions](#loadexpressions) | 67,192 | 0% | -67.41% | 18 | 0% | -87.67% | 3,560 | 0% | -41.98% |
| [Render/expressions](#renderexpressions) | 517,323 | -0.07% | -42.91% | 3 | 0% | -90.00% | 2,472 | 0% | -30.25% |
| [Load/filters](#loadfilters) | 97,655 | 0% | -64.42% | 32 | 0% | -85.96% | 3,896 | 0% | -65.87% |
| [Render/filters](#renderfilters) | 52,803 | 0% | -40.45% | 37 | 0% | -50.00% | 2,552 | 0% | -48.05% |
| [Load/for_filter_if](#loadfor_filter_if) | 41,106 | 0% | -69.64% | 9 | 0% | -92.68% | 1,936 | 0% | -43.85% |
| [Render/for_filter_if](#renderfor_filter_if) | 380,521 | -14.99% | -56.03% | 204 | -1.92% | -9.73% | 2,840 | -9.21% | -21.98% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 57,243 | 0% | -68.31% | 13 | 0% | -91.72% | 2,416 | 0% | -46.55% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 272,337 | +0.02% | -74.61% | 4 | 0% | -99.23% | 2,112 | 0% | -23.70% |
| [Load/for_range](#loadfor_range) | 19,594 | 0% | -65.43% | 10 | 0% | -82.14% | 1,136 | 0% | -43.20% |
| [Render/for_range](#renderfor_range) | 46,279 | -0.33% | -49.81% | 3 | 0% | -88.89% | 440 | 0% | -70.90% |
| [Load/html_autoescape](#loadhtml_autoescape) | 289,624 | 0% |  | 61 | 0% |  | 12,944 | 0% | -47.55% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,598,894 | -0.04% |  | 1,013 | 0% |  | 41,600 | 0% | +3.01% |
| [Load/inheritance](#loadinheritance) | 34,687 | 0% | -69.60% | 10 | 0% | -90.65% | 1,488 | 0% | -48.19% |
| [Render/inheritance](#renderinheritance) | 193,660 | -0.03% | -74.10% | 81 | 0% | -86.98% | 4,416 | 0% | -26.60% |
| [Load/large_static](#loadlarge_static) | 237,668 | 0% | -73.17% | 28 | 0% | -93.64% | 42,720 | 0% | -19.98% |
| [Render/large_static](#renderlarge_static) | 21,486 | 0% | -71.40% | 3 | 0% | -83.33% | 37,112 | 0% | -0.32% |
| [Load/macros](#loadmacros) | 81,731 | 0% | -64.15% | 32 | 0% | -82.80% | 5,128 | 0% | -17.50% |
| [Render/macros](#rendermacros) | 1,005,629 | -0.19% | -60.76% | 410 | 0% | -84.98% | 14,728 | 0% | +0.05% |
| [Load/many_tags](#loadmany_tags) | 13,310,876 | 0% | -69.86% | 962 | 0% | -97.19% | 445,680 | 0% | -57.76% |
| [Render/many_tags](#rendermany_tags) | 1,165,906 | 0% | -44.03% | 15 | 0% | -28.57% | 82,520 | 0% | +1207.35% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 78,111 | 0% | -67.12% | 26 | 0% | -85.87% | 3,552 | 0% | -41.42% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 6,650,919 | +0.41% | -46.17% | 1,027 | 0% | -74.66% | 346,424 | 0% | -0.17% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 96,464 | 0% |  | 57 | 0% |  | 5,736 | 0% | -30.59% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 7,021,338 | +0.39% |  | 1,045 | 0% |  | 1,380,352 | 0% | -0.05% |
| [Load/plain_text](#loadplain_text) | 4,068 | 0% | -57.96% | 5 | 0% | -72.22% | 688 | 0% | -38.57% |
| [Render/plain_text](#renderplain_text) | 986 | 0% | -85.49% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [Load/strings](#loadstrings) | 101,400 | 0% | -65.26% | 33 | 0% | -85.27% | 4,504 | 0% | -50.18% |
| [Render/strings](#renderstrings) | 1,054,902 | -0.03% | -49.71% | 979 | 0% | -53.69% | 18,368 | 0% | +2.45% |
| [Load/substitute](#loadsubstitute) | 10,865 | 0% | -63.61% | 8 | 0% | -78.38% | 832 | 0% | -45.26% |
| [Render/substitute](#rendersubstitute) | 2,711 | 0% | -69.25% | 3 | 0% | -75.00% | 600 | 0% | -16.67% |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 1,189 | +0.08% | 1,388 | +0.22% | 0 |  | 8,788 | +4.92% |
| Render/chat_llama | 1,277 | +0.16% | 499 | -1.77% | 0 |  | 4,546 | -3.73% |
| Load/chat_mistral | 1,792 | -1.21% | 2,078 | +0.58% | 0 |  | 10,336 | +5.28% |
| Render/chat_mistral | 2,175 | -0.05% | 531 | -1.30% | 0 |  | 13,017 | -3.60% |
| Load/chat_qwen | 827 | -1.08% | 1,240 | 0% | 0 |  | 5,822 | +5.93% |
| Render/chat_qwen | 1,161 | -3.97% | 381 | +0.26% | 0 |  | 4,189 | +0.31% |
| Load/config_file | 380 | 0% | 564 | +0.36% | 0 |  | 3,636 | +4.33% |
| Render/config_file | 7,093 | +0.58% | 2,012 | -1.66% | 0 |  | 67,946 | -1.84% |
| Load/dict_ops | 103 | -2.83% | 103 | -5.50% | 0 |  | 1,610 | +2.68% |
| Render/dict_ops | 949 | 0% | 732 | +0.55% | 0 |  | 1,065 | -3.27% |
| Load/expressions | 124 | -4.62% | 208 | +1.96% | 0 |  | 1,545 | +2.52% |
| Render/expressions | 10 | 0% | 2 | -33.33% | 0 |  | 428 | -4.46% |
| Load/filters | 205 | -1.44% | 152 | +0.66% | 0 |  | 2,671 | +7.53% |
| Render/filters | 49 | -12.50% | 30 | 0% | 0 |  | 1,162 | +1.93% |
| Load/for_filter_if | 50 | -5.66% | 44 | +4.76% | 0 |  | 1,223 | +3.73% |
| Render/for_filter_if | 1,298 | -0.92% | 61 | +5.17% | 0 |  | 244 | -36.13% |
| Load/for_loop_vars | 54 | -1.82% | 79 | +5.33% | 0 |  | 1,399 | +5.11% |
| Render/for_loop_vars | 10 | +11.11% | 6 | 0% | 0 |  | 272 | -3.89% |
| Load/for_range | 26 | +23.81% | 28 | +7.69% | 0 |  | 949 | +0.32% |
| Render/for_range | 2 | 0% | 2 | 0% | 0 |  | 203 | -15.06% |
| Load/html_autoescape | 650 | -0.61% | 882 | +0.23% | 0 |  | 5,081 | +2.54% |
| Render/html_autoescape | 4,042 | -3.07% | 1,494 | +0.61% | 0 |  | 49,049 | -4.03% |
| Load/inheritance | 23 | -8.00% | 23 | 0% | 0 |  | 959 | +5.27% |
| Render/inheritance | 509 | +0.79% | 100 | -0.99% | 0 |  | 1,188 | -0.83% |
| Load/large_static | 2,622 | +0.11% | 1,190 | 0% | 0 |  | 485 | -8.32% |
| Render/large_static | 711 | -0.14% | 579 | 0% | 0 |  | 12 | +200.00% |
| Load/macros | 213 | -0.47% | 275 | +0.36% | 0 |  | 2,038 | +2.77% |
| Render/macros | 681 | +2.25% | 207 | -0.48% | 0 |  | 6,165 | -35.49% |
| Load/many_tags | 39,829 | -0.80% | 33,278 | +0.06% | 0 |  | 321,929 | +5.34% |
| Render/many_tags | 6,883 | +0.03% | 1,465 | +0.14% | 0 |  | 4,267 | +712.76% |
| Load/mitsuhiko_table | 114 | -5.79% | 202 | -1.46% | 0 |  | 1,290 | +2.38% |
| Render/mitsuhiko_table | 8,019 | -0.02% | 5,015 | +0.02% | 0 |  | 435 | -7.64% |
| Load/mitsuhiko_table_wide | 195 | -1.52% | 335 | 0% | 0 |  | 1,697 | +10.48% |
| Render/mitsuhiko_table_wide | 8,089 | +0.15% | 21,466 | +0.03% | 0 |  | 488 | -1.81% |
| Load/plain_text | 0 |  | 3 | 0% | 0 |  | 21 | -44.74% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 6 | +50.00% |
| Load/strings | 252 | -1.56% | 290 | +2.84% | 0 |  | 3,246 | +2.79% |
| Render/strings | 923 | +1.10% | 1,349 | 0% | 0 |  | 3,925 | -3.82% |
| Load/substitute | 7 | 0% | 12 | 0% | 0 |  | 376 | +2.73% |
| Render/substitute | 0 |  | 1 | 0% | 0 |  | 5 | 0% |

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
