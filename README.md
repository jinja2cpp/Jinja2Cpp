# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (120 so far, latest [0e6ff37](https://github.com/jinja2cpp/Jinja2Cpp/commit/0e6ff3703c224eb7d37d88a41e774bd84876d85b) on 2026-10-23). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 344,366 | -1.47% |  | 55 | -3.51% |  | 18,272 | +1.06% | -53.24% |
| [Render/chat_llama](#renderchat_llama) | 305,956 | -0.07% |  | 251 | 0% |  | 11,136 | 0% | -22.45% |
| [Load/chat_mistral](#loadchat_mistral) | 393,297 | -1.71% |  | 50 | 0% |  | 20,824 | +0.70% | -53.36% |
| [Render/chat_mistral](#renderchat_mistral) | 511,833 | -0.42% |  | 428 | 0% |  | 9,520 | -0.17% | -27.92% |
| [Load/chat_qwen](#loadchat_qwen) | 266,677 | -1.00% |  | 44 | 0% |  | 14,432 | +0.78% | -47.19% |
| [Render/chat_qwen](#renderchat_qwen) | 293,272 | -0.04% |  | 238 | 0% |  | 7,824 | 0% | -8.00% |
| [Load/config_file](#loadconfig_file) | 98,674 | -1.53% |  | 25 | -7.41% |  | 5,464 | +2.40% | -48.26% |
| [Render/config_file](#renderconfig_file) | 1,254,967 | -0.13% |  | 908 | 0% |  | 23,304 | 0% | -2.12% |
| [Load/dict_ops](#loaddict_ops) | 33,133 | -0.85% | -77.58% | 9 | 0% | -93.13% | 2,208 | +2.22% | -48.12% |
| [Render/dict_ops](#renderdict_ops) | 297,573 | -0.57% | -56.93% | 112 | 0% | -75.11% | 24,408 | 0% | -29.65% |
| [Load/expressions](#loadexpressions) | 45,426 | -1.19% | -77.96% | 16 | 0% | -89.04% | 2,760 | +1.17% | -55.02% |
| [Render/expressions](#renderexpressions) | 425,751 | -6.28% | -53.01% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 71,857 | -1.51% | -73.82% | 27 | -6.90% | -88.16% | 3,768 | +2.61% | -66.99% |
| [Render/filters](#renderfilters) | 31,009 | -0.49% | -65.03% | 29 | 0% | -60.81% | 1,376 | 0% | -71.99% |
| [Load/for_filter_if](#loadfor_filter_if) | 29,932 | -1.30% | -77.89% | 7 | 0% | -94.31% | 1,952 | +2.52% | -43.39% |
| [Render/for_filter_if](#renderfor_filter_if) | 271,417 | +0.01% | -68.64% | 4 | 0% | -98.23% | 13,328 | 0% | +266.15% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 40,362 | -1.03% | -77.66% | 10 | 0% | -93.63% | 2,400 | +1.35% | -46.90% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 194,714 | +0.10% | -81.85% | 2 | 0% | -99.62% | 1,552 | 0% | -43.93% |
| [Load/for_range](#loadfor_range) | 13,624 | -0.84% | -75.96% | 7 | 0% | -87.50% | 1,136 | +2.90% | -43.20% |
| [Render/for_range](#renderfor_range) | 38,831 | -1.03% | -57.89% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/html_autoescape](#loadhtml_autoescape) | 194,381 | -1.50% |  | 43 | -14.00% |  | 12,608 | +2.34% | -48.91% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,231,224 | -0.13% |  | 734 | 0% |  | 43,664 | 0% | +8.12% |
| [Load/inheritance](#loadinheritance) | 23,179 | -0.50% | -79.69% | 7 | 0% | -93.46% | 1,472 | +3.37% | -48.75% |
| [Render/inheritance](#renderinheritance) | 136,285 | -0.08% | -81.78% | 35 | 0% | -94.37% | 6,456 | 0% | +7.31% |
| [Load/large_static](#loadlarge_static) | 154,793 | +0.15% | -82.53% | 14 | -22.22% | -96.82% | 43,168 | +1.01% | -19.14% |
| [Render/large_static](#renderlarge_static) | 21,057 | -0.02% | -71.97% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [Load/macros](#loadmacros) | 55,312 | -1.41% | -75.74% | 27 | -3.57% | -85.48% | 4,392 | +1.86% | -29.34% |
| [Render/macros](#rendermacros) | 647,764 | -0.18% | -74.73% | 308 | 0% | -88.71% | 18,848 | 0% | +28.04% |
| [Load/many_tags](#loadmany_tags) | 9,070,914 | -0.80% | -79.46% | 951 | -0.52% | -97.22% | 428,752 | +1.74% | -59.37% |
| [Render/many_tags](#rendermany_tags) | 724,620 | -1.11% | -65.21% | 2 | 0% | -90.48% | 5,632 | 0% | -10.77% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 46,181 | -1.68% | -80.56% | 19 | -5.00% | -89.67% | 3,408 | +2.40% | -43.80% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,183,620 | -0.27% | -58.05% | 33 | 0% | -99.19% | 373,296 | 0% | +7.58% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 55,709 | -0.97% |  | 35 | -2.78% |  | 5,736 | +1.41% | -30.59% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,528,869 | -0.25% |  | 33 | 0% |  | 1,406,688 | 0% | +1.86% |
| [Load/plain_text](#loadplain_text) | 3,775 | +0.51% | -60.99% | 4 | 0% | -77.78% | 720 | +2.27% | -35.71% |
| [Render/plain_text](#renderplain_text) | 1,031 | -0.48% | -84.82% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [Load/strings](#loadstrings) | 72,162 | -1.56% | -75.27% | 29 | -3.33% | -87.05% | 4,296 | +1.51% | -52.48% |
| [Render/strings](#renderstrings) | 615,749 | -2.82% | -70.64% | 449 | 0% | -78.76% | 9,104 | 0% | -49.22% |
| [Load/substitute](#loadsubstitute) | 7,794 | +0.30% | -73.90% | 6 | 0% | -83.78% | 880 | +3.77% | -42.11% |
| [Render/substitute](#rendersubstitute) | 2,269 | -0.22% | -74.26% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 704 | -4.61% | 719 | -1.91% | 0 |  | 7,468 | -1.62% |
| Render/chat_llama | 1,157 | +2.57% | 506 | +9.29% | 0 |  | 3,436 | -7.88% |
| Load/chat_mistral | 1,095 | +2.82% | 1,024 | +0.29% | 0 |  | 8,257 | -6.67% |
| Render/chat_mistral | 1,852 | +6.81% | 549 | +13.20% | 0 |  | 8,788 | -0.11% |
| Load/chat_qwen | 495 | -0.40% | 609 | +0.83% | 0 |  | 4,569 | -3.73% |
| Render/chat_qwen | 1,096 | +10.37% | 421 | +5.78% | 0 |  | 2,956 | -8.48% |
| Load/config_file | 156 | -7.69% | 160 | +6.67% | 0 |  | 2,970 | -3.26% |
| Render/config_file | 6,457 | +11.17% | 1,882 | -1.77% | 0 |  | 39,219 | +3.41% |
| Load/dict_ops | 31 | 0% | 19 | +26.67% | 0 |  | 1,290 | -3.01% |
| Render/dict_ops | 772 | +6.78% | 412 | -1.67% | 0 |  | 826 | -4.29% |
| Load/expressions | 23 | +9.52% | 20 | +66.67% | 0 |  | 1,323 | -1.19% |
| Render/expressions | 7 | +133.33% | 4 | +300.00% | 0 |  | 319 | +16.42% |
| Load/filters | 139 | +13.01% | 72 | +12.50% | 0 |  | 2,107 | +7.12% |
| Render/filters | 13 | -23.53% | 5 | +25.00% | 0 |  | 752 | -1.18% |
| Load/for_filter_if | 11 | +37.50% | 5 | +66.67% | 0 |  | 1,053 | -9.61% |
| Render/for_filter_if | 1,657 | +0.12% | 69 | +13.11% | 0 |  | 163 | -8.43% |
| Load/for_loop_vars | 18 | -35.71% | 11 | +10.00% | 0 |  | 1,102 | -7.86% |
| Render/for_loop_vars | 8 | +100.00% | 4 | -20.00% | 0 |  | 140 | -5.41% |
| Load/for_range | 2 | -60.00% | 3 | -25.00% | 0 |  | 714 | -1.79% |
| Render/for_range | 1 | 0% | 1 | 0% | 0 |  | 146 | +33.94% |
| Load/html_autoescape | 449 | +3.94% | 511 | +6.68% | 0 |  | 4,202 | -8.27% |
| Render/html_autoescape | 5,301 | +14.47% | 1,602 | +18.23% | 0 |  | 33,150 | +2.96% |
| Load/inheritance | 3 | +50.00% | 3 | +50.00% | 0 |  | 613 | -3.01% |
| Render/inheritance | 575 | +0.17% | 147 | +22.50% | 0 |  | 826 | +6.03% |
| Load/large_static | 1,950 | +0.31% | 907 | +2.37% | 0 |  | 328 | +63.18% |
| Render/large_static | 703 | -0.28% | 571 | +0.18% | 0 |  | 2 | 0% |
| Load/macros | 85 | +26.87% | 52 | 0% | 0 |  | 1,640 | -4.15% |
| Render/macros | 714 | -3.51% | 229 | -1.29% | 0 |  | 552 | -68.07% |
| Load/many_tags | 28,716 | -0.46% | 22,195 | +2.55% | 0 |  | 249,532 | -4.47% |
| Render/many_tags | 6,131 | +0.76% | 881 | -2.76% | 0 |  | 162 | -1.22% |
| Load/mitsuhiko_table | 32 | +23.08% | 44 | +131.58% | 0 |  | 823 | -4.08% |
| Render/mitsuhiko_table | 9,054 | +0.22% | 6,612 | +0.21% | 0 |  | 300 | +8.70% |
| Load/mitsuhiko_table_wide | 85 | +11.84% | 80 | +9.59% | 0 |  | 1,092 | +12.46% |
| Render/mitsuhiko_table_wide | 9,121 | -0.40% | 22,907 | -0.50% | 0 |  | 304 | +10.95% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 37 | +640.00% |
| Render/plain_text | 0 |  | 1 | 0% | 0 |  | 4 | 0% |
| Load/strings | 188 | +13.25% | 100 | +5.26% | 0 |  | 2,857 | +3.78% |
| Render/strings | 464 | -2.32% | 607 | -0.65% | 0 |  | 1,458 | -0.95% |
| Load/substitute | 1 | 0% | 3 | +50.00% | 0 |  | 223 | +110.38% |
| Render/substitute | 0 |  | 1 | 0% | 0 |  | 2 | 0% |

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
