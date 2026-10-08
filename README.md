# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (54 so far, latest [f3cc856](https://github.com/jinja2cpp/Jinja2Cpp/commit/f3cc85623e41d223bf4c7db6acd06d2994f3ef19) on 2026-10-08). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 581,664 | +0.30% |  | 101 | -15.83% |  | 23,232 | -0.21% | -40.55% |
| [Render/chat_llama](#renderchat_llama) | 411,411 | +0.22% |  | 348 | 0% |  | 13,216 | -0.12% | -7.97% |
| [Load/chat_mistral](#loadchat_mistral) | 682,751 | +0.08% |  | 86 | -23.21% |  | 29,032 | -0.22% | -34.98% |
| [Render/chat_mistral](#renderchat_mistral) | 711,636 | -0.02% |  | 474 | 0% |  | 11,632 | -0.14% | -11.93% |
| [Load/chat_qwen](#loadchat_qwen) | 436,328 | +0.02% |  | 78 | -17.89% |  | 18,344 | -0.35% | -32.87% |
| [Render/chat_qwen](#renderchat_qwen) | 397,551 | -0.02% |  | 337 | 0% |  | 7,360 | 0% | -13.45% |
| [Load/config_file](#loadconfig_file) | 150,366 | -0.58% |  | 34 | -15.00% |  | 6,456 | -4.04% | -38.86% |
| [Render/config_file](#renderconfig_file) | 1,913,233 | 0% |  | 1,703 | 0% |  | 21,528 | 0% | -9.58% |
| [Load/dict_ops](#loaddict_ops) | 53,271 | -0.88% | -63.95% | 15 | -16.67% | -88.55% | 2,752 | -3.91% | -35.34% |
| [Render/dict_ops](#renderdict_ops) | 401,104 | +0.01% | -41.94% | 315 | 0% | -30.00% | 34,032 | +0.05% | -1.91% |
| [Load/expressions](#loadexpressions) | 71,640 | -0.20% | -65.25% | 21 | -8.70% | -85.62% | 4,336 | -1.09% | -29.34% |
| [Render/expressions](#renderexpressions) | 532,368 | 0% | -41.25% | 3 | 0% | -90.00% | 2,472 | 0% | -30.25% |
| [Load/filters](#loadfilters) | 99,605 | -0.57% | -63.71% | 32 | -3.03% | -85.96% | 4,216 | -5.39% | -63.07% |
| [Render/filters](#renderfilters) | 60,076 | 0% | -32.25% | 46 | 0% | -37.84% | 4,232 | 0% | -13.84% |
| [Load/for_filter_if](#loadfor_filter_if) | 42,473 | -3.53% | -68.63% | 10 | -33.33% | -91.87% | 2,176 | -4.90% | -36.89% |
| [Render/for_filter_if](#renderfor_filter_if) | 463,905 | +0.06% | -46.40% | 206 | 0% | -8.85% | 2,568 | 0% | -29.45% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 58,440 | -1.62% | -67.65% | 13 | -23.53% | -91.72% | 2,776 | -3.88% | -38.58% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 276,247 | +0.01% | -74.25% | 2 | 0% | -99.62% | 1,552 | 0% | -43.93% |
| [Load/for_range](#loadfor_range) | 21,347 | -0.46% | -62.34% | 10 | -16.67% | -82.14% | 1,352 | -5.59% | -32.40% |
| [Render/for_range](#renderfor_range) | 46,749 | 0% | -49.30% | 3 | 0% | -88.89% | 440 | 0% | -70.90% |
| [Load/html_autoescape](#loadhtml_autoescape) | 297,191 | -0.62% |  | 65 | -8.45% |  | 15,184 | -5.10% | -38.48% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,614,242 | +0.01% |  | 1,011 | 0% |  | 38,752 | 0% | -4.04% |
| [Load/inheritance](#loadinheritance) | 36,146 | -5.52% | -68.33% | 10 | -41.18% | -90.65% | 1,632 | -16.73% | -43.18% |
| [Render/inheritance](#renderinheritance) | 213,488 | +0.17% | -71.45% | 76 | 0% | -87.78% | 3,328 | 0% | -44.68% |
| [Load/large_static](#loadlarge_static) | 240,857 | -0.70% | -72.81% | 28 | -3.45% | -93.64% | 44,128 | -2.41% | -17.34% |
| [Render/large_static](#renderlarge_static) | 27,370 | 0% | -63.57% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [Load/macros](#loadmacros) | 83,845 | -0.64% | -63.22% | 31 | -8.82% | -83.33% | 4,416 | -4.50% | -28.96% |
| [Render/macros](#rendermacros) | 1,017,294 | +0.01% | -60.31% | 407 | 0% | -85.09% | 13,592 | 0% | -7.66% |
| [Load/many_tags](#loadmany_tags) | 13,741,982 | -0.42% | -68.88% | 958 | -38.59% | -97.20% | 530,800 | -2.24% | -49.70% |
| [Render/many_tags](#rendermany_tags) | 1,269,151 | 0% | -39.07% | 2 | 0% | -90.48% | 5,632 | 0% | -10.77% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 83,519 | -1.28% | -64.84% | 28 | -12.50% | -84.78% | 4,024 | -7.02% | -33.64% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 6,653,397 | 0% | -46.15% | 1,025 | 0% | -74.71% | 345,864 | 0% | -0.33% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 102,897 | -1.05% |  | 63 | -5.97% |  | 6,208 | -4.67% | -24.88% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 7,024,403 | 0% |  | 1,046 | 0% |  | 1,379,848 | 0% | -0.08% |
| [Load/plain_text](#loadplain_text) | 4,077 | -8.63% | -57.87% | 5 | -16.67% | -72.22% | 688 | -8.51% | -38.57% |
| [Render/plain_text](#renderplain_text) | 986 | +0.10% | -85.49% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [Load/strings](#loadstrings) | 107,570 | -0.48% | -63.14% | 34 | -8.11% | -84.82% | 5,248 | -2.38% | -41.95% |
| [Render/strings](#renderstrings) | 1,062,637 | +0.01% | -49.34% | 975 | 0% | -53.88% | 17,248 | 0% | -3.79% |
| [Load/substitute](#loadsubstitute) | 10,952 | -2.91% | -63.32% | 8 | -11.11% | -78.38% | 880 | -6.78% | -42.11% |
| [Render/substitute](#rendersubstitute) | 2,666 | +0.04% | -69.76% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 1,604 | 0% | 1,657 | +0.49% | 0 |  | 8,720 | +2.66% |
| Render/chat_llama | 1,332 | +0.76% | 476 | +7.45% | 0 |  | 4,771 | -5.73% |
| Load/chat_mistral | 2,314 | +1.58% | 2,312 | +2.76% | 0 |  | 9,985 | +2.27% |
| Render/chat_mistral | 2,528 | +1.98% | 716 | +6.07% | 0 |  | 13,720 | -6.56% |
| Load/chat_qwen | 1,084 | +2.36% | 1,379 | +0.73% | 0 |  | 5,794 | +10.05% |
| Render/chat_qwen | 1,274 | -0.16% | 334 | +0.30% | 0 |  | 4,173 | -6.22% |
| Load/config_file | 388 | -3.96% | 568 | -4.54% | 0 |  | 3,577 | +3.98% |
| Render/config_file | 7,091 | -6.05% | 2,099 | -0.90% | 0 |  | 71,006 | +1.10% |
| Load/dict_ops | 107 | +10.31% | 97 | -12.61% | 0 |  | 1,779 | +2.65% |
| Render/dict_ops | 1,043 | +1.56% | 621 | -0.64% | 0 |  | 1,139 | +0.35% |
| Load/expressions | 155 | +2.65% | 242 | +5.68% | 0 |  | 1,555 | +0.06% |
| Render/expressions | 13 | +44.44% | 4 | +33.33% | 0 |  | 483 | +7.10% |
| Load/filters | 220 | +7.32% | 167 | +1.21% | 0 |  | 2,377 | +2.77% |
| Render/filters | 82 | -16.33% | 74 | +2.78% | 0 |  | 1,206 | -0.08% |
| Load/for_filter_if | 50 | +31.58% | 32 | +10.34% | 0 |  | 1,276 | +6.42% |
| Render/for_filter_if | 1,305 | +0.08% | 54 | +3.85% | 0 |  | 2,057 | +368.56% |
| Load/for_loop_vars | 71 | +20.34% | 86 | +2.38% | 0 |  | 1,480 | +7.48% |
| Render/for_loop_vars | 11 | +83.33% | 6 | +200.00% | 0 |  | 275 | +3.38% |
| Load/for_range | 31 | +40.91% | 26 | +8.33% | 0 |  | 1,061 | +4.53% |
| Render/for_range | 4 | +100.00% | 2 | 0% | 0 |  | 256 | +17.97% |
| Load/html_autoescape | 833 | -5.88% | 983 | -0.61% | 0 |  | 4,920 | +10.09% |
| Render/html_autoescape | 4,783 | -30.07% | 1,795 | -18.00% | 0 |  | 52,162 | +4.15% |
| Load/inheritance | 21 | -22.22% | 19 | +5.56% | 0 |  | 1,044 | -4.13% |
| Render/inheritance | 509 | +0.20% | 99 | +1.02% | 0 |  | 813 | -33.31% |
| Load/large_static | 2,630 | +0.61% | 1,226 | -1.84% | 0 |  | 464 | +6.91% |
| Render/large_static | 737 | -1.73% | 582 | 0% | 0 |  | 4 | -20.00% |
| Load/macros | 227 | +7.08% | 286 | -2.39% | 0 |  | 2,003 | +2.93% |
| Render/macros | 676 | -5.45% | 213 | -0.47% | 0 |  | 16,034 | +116.94% |
| Load/many_tags | 45,498 | -2.51% | 33,447 | -3.04% | 0 |  | 298,247 | +6.88% |
| Render/many_tags | 8,173 | -1.59% | 1,789 | +1.65% | 0 |  | 7,421 | +99.65% |
| Load/mitsuhiko_table | 98 | -16.24% | 200 | -11.11% | 0 |  | 1,416 | +2.61% |
| Render/mitsuhiko_table | 8,024 | -0.06% | 5,009 | 0% | 0 |  | 460 | -3.36% |
| Load/mitsuhiko_table_wide | 218 | +5.83% | 341 | +1.19% | 0 |  | 2,005 | +5.53% |
| Render/mitsuhiko_table_wide | 8,079 | -0.02% | 21,460 | +0.01% | 0 |  | 513 | +1.58% |
| Load/plain_text | 0 |  | 2 | +100.00% | 0 |  | 47 | +683.33% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 4 | -20.00% |
| Load/strings | 284 | +3.65% | 305 | -3.17% | 0 |  | 3,263 | +1.02% |
| Render/strings | 929 | 0% | 1,339 | -0.37% | 0 |  | 2,774 | -21.55% |
| Load/substitute | 8 | +166.67% | 10 | +66.67% | 0 |  | 422 | +20.92% |
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
