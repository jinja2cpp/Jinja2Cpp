# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (61 so far, latest [ec9e7b4](https://github.com/jinja2cpp/Jinja2Cpp/commit/ec9e7b461076e9b15e72934303ad039517289ff9) on 2026-10-10). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 554,035 | +0.06% |  | 92 | 0% |  | 20,128 | 0% | -48.50% |
| [Render/chat_llama](#renderchat_llama) | 408,581 | -0.58% |  | 348 | 0% |  | 13,216 | 0% | -7.97% |
| [Load/chat_mistral](#loadchat_mistral) | 653,635 | +0.04% |  | 74 | 0% |  | 24,200 | 0% | -45.80% |
| [Render/chat_mistral](#renderchat_mistral) | 656,667 | -0.73% |  | 426 | 0% |  | 10,440 | -0.46% | -20.96% |
| [Load/chat_qwen](#loadchat_qwen) | 423,009 | -0.03% |  | 68 | 0% |  | 16,000 | 0% | -41.45% |
| [Render/chat_qwen](#renderchat_qwen) | 396,387 | -0.24% |  | 337 | 0% |  | 7,360 | 0% | -13.45% |
| [Load/config_file](#loadconfig_file) | 145,211 | +0.17% |  | 34 | 0% |  | 5,656 | 0% | -46.44% |
| [Render/config_file](#renderconfig_file) | 1,801,733 | -5.92% |  | 1,169 | -31.36% |  | 21,656 | +0.52% | -9.04% |
| [Load/dict_ops](#loaddict_ops) | 47,412 | +0.11% | -67.92% | 12 | 0% | -90.84% | 2,240 | 0% | -47.37% |
| [Render/dict_ops](#renderdict_ops) | 351,893 | -12.06% | -49.06% | 112 | -64.44% | -75.11% | 35,608 | +4.68% | +2.63% |
| [Load/expressions](#loadexpressions) | 66,938 | +0.03% | -67.53% | 18 | 0% | -87.67% | 3,592 | 0% | -41.46% |
| [Render/expressions](#renderexpressions) | 518,630 | -0.07% | -42.76% | 3 | 0% | -90.00% | 2,472 | 0% | -30.25% |
| [Load/filters](#loadfilters) | 97,259 | +0.51% | -64.56% | 32 | 0% | -85.96% | 3,960 | 0% | -65.31% |
| [Render/filters](#renderfilters) | 53,643 | -0.70% | -39.50% | 35 | 0% | -52.70% | 1,992 | 0% | -59.45% |
| [Load/for_filter_if](#loadfor_filter_if) | 40,899 | +0.01% | -69.79% | 9 | 0% | -92.68% | 1,952 | 0% | -43.39% |
| [Render/for_filter_if](#renderfor_filter_if) | 447,975 | +0.10% | -48.24% | 206 | 0% | -8.85% | 2,568 | 0% | -29.45% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 56,909 | +0.06% | -68.50% | 13 | 0% | -91.72% | 2,448 | 0% | -45.84% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 272,844 | -0.80% | -74.57% | 2 | 0% | -99.62% | 1,552 | 0% | -43.93% |
| [Load/for_range](#loadfor_range) | 19,490 | +0.04% | -65.62% | 10 | 0% | -82.14% | 1,136 | 0% | -43.20% |
| [Render/for_range](#renderfor_range) | 46,535 | -0.52% | -49.53% | 3 | 0% | -88.89% | 440 | 0% | -70.90% |
| [Load/html_autoescape](#loadhtml_autoescape) | 287,082 | +0.13% |  | 61 | 0% |  | 13,152 | 0% | -46.71% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,600,858 | -0.69% |  | 1,003 | -0.79% |  | 38,816 | +0.17% | -3.88% |
| [Load/inheritance](#loadinheritance) | 34,760 | +0.06% | -69.54% | 10 | 0% | -90.65% | 1,488 | 0% | -48.19% |
| [Render/inheritance](#renderinheritance) | 214,000 | -0.31% | -71.38% | 73 | -3.95% | -88.26% | 3,296 | +0.49% | -45.21% |
| [Load/large_static](#loadlarge_static) | 236,867 | +0.03% | -73.27% | 28 | 0% | -93.64% | 43,120 | 0% | -19.23% |
| [Render/large_static](#renderlarge_static) | 27,520 | 0% | -63.37% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [Load/macros](#loadmacros) | 81,013 | 0% | -64.46% | 29 | 0% | -84.41% | 3,744 | 0% | -39.77% |
| [Render/macros](#rendermacros) | 1,008,591 | -1.09% | -60.65% | 406 | -0.25% | -85.12% | 13,608 | +0.12% | -7.55% |
| [Load/many_tags](#loadmany_tags) | 13,216,589 | +0.23% | -70.07% | 958 | 0% | -97.20% | 457,856 | 0% | -56.61% |
| [Render/many_tags](#rendermany_tags) | 1,255,721 | -0.54% | -39.72% | 2 | 0% | -90.48% | 5,632 | 0% | -10.77% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 77,686 | -0.09% | -67.30% | 26 | 0% | -85.87% | 3,568 | 0% | -41.16% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 6,634,919 | -0.34% | -46.30% | 1,025 | 0% | -74.71% | 345,864 | 0% | -0.33% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 96,201 | -0.97% |  | 57 | -6.56% |  | 5,752 | 0% | -30.40% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 7,005,563 | -0.32% |  | 1,043 | -0.29% |  | 1,379,792 | 0% | -0.09% |
| [Load/plain_text](#loadplain_text) | 4,044 | +0.75% | -58.21% | 5 | 0% | -72.22% | 688 | 0% | -38.57% |
| [Render/plain_text](#renderplain_text) | 986 | 0% | -85.49% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [Load/strings](#loadstrings) | 100,998 | +0.19% | -65.39% | 33 | 0% | -85.27% | 4,552 | 0% | -49.65% |
| [Render/strings](#renderstrings) | 1,055,320 | -0.59% | -49.69% | 975 | 0% | -53.88% | 17,248 | 0% | -3.79% |
| [Load/substitute](#loadsubstitute) | 10,702 | +0.07% | -64.15% | 8 | 0% | -78.38% | 832 | 0% | -45.26% |
| [Render/substitute](#rendersubstitute) | 2,672 | 0% | -69.69% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 1,227 | -1.60% | 1,399 | -0.43% | 0 |  | 8,295 | -1.31% |
| Render/chat_llama | 1,277 | +2.16% | 511 | +0.79% | 0 |  | 4,763 | +2.34% |
| Load/chat_mistral | 1,806 | +0.84% | 2,020 | +0.90% | 0 |  | 9,618 | -0.71% |
| Render/chat_mistral | 2,210 | -4.29% | 544 | +0.55% | 0 |  | 13,944 | +2.51% |
| Load/chat_qwen | 868 | +2.12% | 1,273 | +1.52% | 0 |  | 5,295 | -2.41% |
| Render/chat_qwen | 1,203 | +1.26% | 380 | +7.04% | 0 |  | 4,422 | +5.74% |
| Load/config_file | 381 | +0.79% | 543 | +0.37% | 0 |  | 3,421 | +0.18% |
| Render/config_file | 7,338 | +5.29% | 1,963 | -3.68% | 0 |  | 70,068 | +0.70% |
| Load/dict_ops | 98 | +10.11% | 92 | +6.98% | 0 |  | 1,520 | -0.52% |
| Render/dict_ops | 955 | -0.42% | 728 | +19.15% | 0 |  | 1,077 | -1.55% |
| Load/expressions | 143 | +6.72% | 209 | -1.88% | 0 |  | 1,453 | -1.29% |
| Render/expressions | 10 | +100.00% | 3 | +50.00% | 0 |  | 478 | +6.94% |
| Load/filters | 214 | +2.88% | 154 | +4.05% | 0 |  | 2,361 | -2.11% |
| Render/filters | 60 | -6.25% | 37 | 0% | 0 |  | 1,118 | -2.78% |
| Load/for_filter_if | 48 | +9.09% | 27 | +17.39% | 0 |  | 1,179 | +0.51% |
| Render/for_filter_if | 1,308 | +0.15% | 57 | +5.56% | 0 |  | 406 | +11.54% |
| Load/for_loop_vars | 59 | +7.27% | 69 | +4.55% | 0 |  | 1,322 | +3.44% |
| Render/for_loop_vars | 15 | +15.38% | 10 | +66.67% | 0 |  | 266 | +9.47% |
| Load/for_range | 24 | -17.24% | 24 | +4.35% | 0 |  | 925 | -0.11% |
| Render/for_range | 2 | -33.33% | 2 | 0% | 0 |  | 245 | +12.90% |
| Load/html_autoescape | 722 | -1.77% | 890 | +1.02% | 0 |  | 4,605 | +0.70% |
| Render/html_autoescape | 4,332 | +25.31% | 1,568 | +21.17% | 0 |  | 49,836 | -2.07% |
| Load/inheritance | 18 | -5.26% | 23 | +35.29% | 0 |  | 921 | +3.14% |
| Render/inheritance | 513 | +1.38% | 98 | -7.55% | 0 |  | 810 | -0.74% |
| Load/large_static | 2,620 | +0.15% | 1,198 | -0.17% | 0 |  | 425 | -4.28% |
| Render/large_static | 726 | 0% | 582 | 0% | 0 |  | 4 | 0% |
| Load/macros | 207 | 0% | 260 | +1.96% | 0 |  | 1,937 | -0.72% |
| Render/macros | 667 | -1.62% | 207 | -1.43% | 0 |  | 9,580 | +2.38% |
| Load/many_tags | 37,813 | -2.34% | 30,465 | -1.56% | 0 |  | 285,884 | -5.91% |
| Render/many_tags | 7,270 | +0.04% | 1,737 | +0.40% | 0 |  | 557 | -93.33% |
| Load/mitsuhiko_table | 102 | +12.09% | 190 | +2.15% | 0 |  | 1,242 | +7.07% |
| Render/mitsuhiko_table | 8,020 | 0% | 5,013 | +0.04% | 0 |  | 7,459 | +1688.73% |
| Load/mitsuhiko_table_wide | 204 | +0.49% | 323 | -0.92% | 0 |  | 1,629 | +5.85% |
| Render/mitsuhiko_table_wide | 8,085 | +0.19% | 21,462 | +0.02% | 0 |  | 7,478 | +1536.32% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 12 | -45.45% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 5 | 0% |
| Load/strings | 255 | +2.00% | 284 | 0% | 0 |  | 3,081 | -2.03% |
| Render/strings | 930 | +1.75% | 1,339 | +0.75% | 0 |  | 2,656 | -5.35% |
| Load/substitute | 7 | -12.50% | 11 | +10.00% | 0 |  | 352 | +0.86% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 4 | +33.33% |

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
