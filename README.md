# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (72 so far, latest [89aa3b7](https://github.com/jinja2cpp/Jinja2Cpp/commit/89aa3b78ade94442649fd3343d0c46b56a87c659) on 2026-10-11). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 372,168 | 0% |  | 59 | 0% |  | 19,920 | 0% | -49.03% |
| [Render/chat_llama](#renderchat_llama) | 406,195 | -0.27% |  | 350 | -1.41% |  | 13,352 | -12.80% | -7.02% |
| [Load/chat_mistral](#loadchat_mistral) | 451,269 | 0% |  | 57 | 0% |  | 23,832 | 0% | -46.62% |
| [Render/chat_mistral](#renderchat_mistral) | 638,517 | -0.15% |  | 416 | -1.19% |  | 10,224 | -16.09% | -22.59% |
| [Load/chat_qwen](#loadchat_qwen) | 292,499 | 0% |  | 45 | 0% |  | 15,872 | 0% | -41.92% |
| [Render/chat_qwen](#renderchat_qwen) | 389,259 | -0.08% |  | 318 | -0.62% |  | 8,840 | -5.96% | +3.95% |
| [Load/config_file](#loadconfig_file) | 102,743 | 0% |  | 27 | 0% |  | 5,576 | 0% | -47.20% |
| [Render/config_file](#renderconfig_file) | 1,649,513 | -0.02% |  | 1,120 | -0.36% |  | 24,272 | -4.35% | +1.95% |
| [Load/dict_ops](#loaddict_ops) | 34,140 | 0% | -76.90% | 9 | 0% | -93.13% | 2,224 | 0% | -47.74% |
| [Render/dict_ops](#renderdict_ops) | 346,049 | -0.09% | -49.91% | 112 | -1.75% | -75.11% | 35,608 | -1.55% | +2.63% |
| [Load/expressions](#loadexpressions) | 47,348 | 0% | -77.03% | 16 | 0% | -89.04% | 3,560 | 0% | -41.98% |
| [Render/expressions](#renderexpressions) | 507,199 | 0% | -44.03% | 3 | 0% | -90.00% | 2,472 | 0% | -30.25% |
| [Load/filters](#loadfilters) | 74,724 | 0% | -72.77% | 29 | 0% | -87.28% | 3,896 | 0% | -65.87% |
| [Render/filters](#renderfilters) | 51,270 | -0.69% | -42.18% | 35 | -5.41% | -52.70% | 2,008 | -21.81% | -59.12% |
| [Load/for_filter_if](#loadfor_filter_if) | 30,457 | 0% | -77.50% | 7 | 0% | -94.31% | 1,936 | 0% | -43.85% |
| [Render/for_filter_if](#renderfor_filter_if) | 333,115 | -0.11% | -61.51% | 4 | -33.33% | -98.23% | 13,344 | -4.03% | +266.59% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 41,884 | 0% | -76.82% | 10 | 0% | -93.63% | 2,416 | 0% | -46.55% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 264,350 | -0.14% | -75.36% | 2 | -50.00% | -99.62% | 1,568 | -26.32% | -43.35% |
| [Load/for_range](#loadfor_range) | 14,303 | 0% | -74.77% | 8 | 0% | -85.71% | 1,136 | 0% | -43.20% |
| [Render/for_range](#renderfor_range) | 46,021 | +0.03% | -50.09% | 3 | 0% | -88.89% | 440 | 0% | -70.90% |
| [Load/html_autoescape](#loadhtml_autoescape) | 205,043 | 0% |  | 50 | 0% |  | 12,944 | 0% | -47.55% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,518,945 | -0.12% |  | 932 | -0.64% |  | 43,600 | -4.85% | +7.96% |
| [Load/inheritance](#loadinheritance) | 24,135 | 0% | -78.85% | 7 | 0% | -93.46% | 1,488 | 0% | -48.19% |
| [Render/inheritance](#renderinheritance) | 148,940 | -0.24% | -80.08% | 32 | -5.88% | -94.86% | 6,616 | -7.80% | +9.97% |
| [Load/large_static](#loadlarge_static) | 156,948 | 0% | -82.29% | 18 | 0% | -95.91% | 42,720 | 0% | -19.98% |
| [Render/large_static](#renderlarge_static) | 21,069 | -1.70% | -71.96% | 1 | -66.67% | -94.44% | 36,552 | -1.51% | -1.83% |
| [Load/macros](#loadmacros) | 58,084 | 0% | -74.52% | 28 | 0% | -84.95% | 5,128 | 0% | -17.50% |
| [Render/macros](#rendermacros) | 861,348 | -0.08% | -66.39% | 308 | -1.28% | -88.71% | 19,184 | -5.52% | +30.33% |
| [Load/many_tags](#loadmany_tags) | 9,478,437 | 0% | -78.54% | 945 | 0% | -97.24% | 445,648 | 0% | -57.77% |
| [Render/many_tags](#rendermany_tags) | 1,044,186 | -5.22% | -49.87% | 2 | -86.67% | -90.48% | 5,632 | -93.17% | -10.77% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 49,647 | 0% | -79.10% | 20 | 0% | -89.13% | 3,552 | 0% | -41.42% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 6,452,855 | -0.01% | -47.78% | 33 | -5.71% | -99.19% | 373,312 | -0.15% | +7.58% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 68,567 | 0% |  | 45 | 0% |  | 5,736 | 0% | -30.59% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 6,803,545 | -0.01% |  | 51 | -3.77% |  | 1,406,704 | -0.04% | +1.86% |
| [Load/plain_text](#loadplain_text) | 3,655 | 0% | -62.23% | 4 | 0% | -77.78% | 688 | 0% | -38.57% |
| [Render/plain_text](#renderplain_text) | 1,002 | +1.52% | -85.26% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [Load/strings](#loadstrings) | 75,655 | 0% | -74.08% | 30 | 0% | -86.61% | 4,504 | 0% | -50.18% |
| [Render/strings](#renderstrings) | 1,029,506 | -0.06% | -50.92% | 975 | -0.41% | -53.88% | 17,264 | -6.09% | -3.70% |
| [Load/substitute](#loadsubstitute) | 7,780 | 0% | -73.94% | 6 | 0% | -83.78% | 832 | 0% | -45.26% |
| [Render/substitute](#rendersubstitute) | 2,347 | -13.27% | -73.38% | 1 | -66.67% | -91.67% | 40 | -93.33% | -94.44% |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 879 | -0.57% | 858 | -0.12% | 0 |  | 7,283 | -12.54% |
| Render/chat_llama | 1,280 | +0.23% | 489 | -2.00% | 0 |  | 4,620 | -2.24% |
| Load/chat_mistral | 1,303 | -0.15% | 1,197 | 0% | 0 |  | 8,487 | -11.91% |
| Render/chat_mistral | 2,206 | +0.91% | 522 | -2.97% | 0 |  | 13,049 | -1.33% |
| Load/chat_qwen | 578 | +0.35% | 680 | 0% | 0 |  | 4,575 | -12.92% |
| Render/chat_qwen | 1,210 | -0.41% | 387 | +1.31% | 0 |  | 3,977 | -1.00% |
| Load/config_file | 172 | -0.58% | 153 | -1.29% | 0 |  | 2,891 | -4.87% |
| Render/config_file | 6,892 | -0.81% | 1,930 | -2.38% | 0 |  | 56,639 | +2.33% |
| Load/dict_ops | 18 | -18.18% | 16 | 0% | 0 |  | 1,270 | -5.01% |
| Render/dict_ops | 976 | -0.41% | 731 | +0.27% | 0 |  | 1,004 | -2.81% |
| Load/expressions | 35 | -5.41% | 28 | -3.45% | 0 |  | 1,213 | -3.12% |
| Render/expressions | 4 | 0% | 3 | 0% | 0 |  | 418 | -1.42% |
| Load/filters | 118 | 0% | 86 | 0% | 0 |  | 1,978 | -7.05% |
| Render/filters | 82 | +12.33% | 38 | +22.58% | 0 |  | 1,067 | -3.00% |
| Load/for_filter_if | 11 | -21.43% | 6 | -14.29% | 0 |  | 1,027 | -6.38% |
| Render/for_filter_if | 1,599 | -0.37% | 66 | 0% | 0 |  | 272 | +12.40% |
| Load/for_loop_vars | 16 | -20.00% | 13 | 0% | 0 |  | 1,072 | -5.22% |
| Render/for_loop_vars | 5 | 0% | 2 | 0% | 0 |  | 223 | -22.03% |
| Load/for_range | 6 | 0% | 5 | 0% | 0 |  | 730 | -2.67% |
| Render/for_range | 2 | 0% | 1 | 0% | 0 |  | 177 | -22.37% |
| Load/html_autoescape | 456 | +0.22% | 532 | +0.57% | 0 |  | 3,969 | -9.28% |
| Render/html_autoescape | 5,553 | -1.93% | 1,566 | +8.90% | 0 |  | 45,738 | +0.02% |
| Load/inheritance | 8 | -11.11% | 5 | 0% | 0 |  | 595 | -6.30% |
| Render/inheritance | 597 | +0.34% | 125 | +1.63% | 0 |  | 842 | -2.66% |
| Load/large_static | 1,960 | 0% | 887 | 0% | 0 |  | 252 | +0.80% |
| Render/large_static | 706 | -0.70% | 576 | -0.35% | 0 |  | 6 | +50.00% |
| Load/macros | 101 | -2.88% | 85 | -1.16% | 0 |  | 1,651 | -2.42% |
| Render/macros | 778 | -0.89% | 217 | -0.91% | 0 |  | 1,963 | -61.78% |
| Load/many_tags | 28,455 | -0.02% | 21,890 | 0% | 0 |  | 238,476 | -5.23% |
| Render/many_tags | 6,490 | -5.74% | 867 | -41.62% | 0 |  | 223 | -95.23% |
| Load/mitsuhiko_table | 40 | 0% | 39 | 0% | 0 |  | 787 | -11.57% |
| Render/mitsuhiko_table | 9,286 | +0.04% | 6,499 | 0% | 0 |  | 483 | -92.56% |
| Load/mitsuhiko_table_wide | 89 | -1.11% | 115 | 0% | 0 |  | 1,315 | -5.80% |
| Render/mitsuhiko_table_wide | 9,407 | +0.06% | 23,010 | +0.02% | 0 |  | 481 | -92.61% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 6 | -62.50% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 6 | +20.00% |
| Load/strings | 156 | +0.65% | 122 | 0% | 0 |  | 2,758 | -5.29% |
| Render/strings | 918 | -0.97% | 1,365 | +0.66% | 0 |  | 1,868 | +0.43% |
| Load/substitute | 2 | 0% | 3 | 0% | 0 |  | 118 | +5.36% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 4 | 0% |

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
