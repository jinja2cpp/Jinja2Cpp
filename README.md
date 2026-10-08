# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (53 so far, latest [425822a](https://github.com/jinja2cpp/Jinja2Cpp/commit/425822ae5834fb61ccfd65c866d6eb062f1cd6f4) on 2026-10-08). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 579,913 | -0.47% |  | 120 | -0.83% |  | 23,280 | -0.55% | -40.43% |
| [Render/chat_llama](#renderchat_llama) | 410,512 | -0.01% |  | 348 | 0% |  | 13,232 | 0% | -7.86% |
| [Load/chat_mistral](#loadchat_mistral) | 682,195 | -0.35% |  | 112 | -0.88% |  | 29,096 | -0.05% | -34.83% |
| [Render/chat_mistral](#renderchat_mistral) | 711,788 | 0% |  | 474 | 0% |  | 11,648 | 0% | -11.81% |
| [Load/chat_qwen](#loadchat_qwen) | 436,227 | -0.40% |  | 95 | -1.04% |  | 18,408 | -0.17% | -32.64% |
| [Render/chat_qwen](#renderchat_qwen) | 397,619 | -0.01% |  | 337 | 0% |  | 7,360 | 0% | -13.45% |
| [Load/config_file](#loadconfig_file) | 151,249 | -0.37% |  | 40 | -2.44% |  | 6,728 | 0% | -36.29% |
| [Render/config_file](#renderconfig_file) | 1,913,254 | -0.03% |  | 1,703 | 0% |  | 21,528 | 0% | -9.58% |
| [Load/dict_ops](#loaddict_ops) | 53,744 | -0.49% | -63.63% | 18 | 0% | -86.26% | 2,864 | 0% | -32.71% |
| [Render/dict_ops](#renderdict_ops) | 401,079 | 0% | -41.94% | 315 | 0% | -30.00% | 34,016 | 0% | -1.96% |
| [Load/expressions](#loadexpressions) | 71,786 | -0.36% | -65.18% | 23 | 0% | -84.25% | 4,384 | 0% | -28.55% |
| [Render/expressions](#renderexpressions) | 532,367 | 0% | -41.25% | 3 | 0% | -90.00% | 2,472 | 0% | -30.25% |
| [Load/filters](#loadfilters) | 100,177 | -0.14% | -63.50% | 33 | 0% | -85.53% | 4,456 | -0.71% | -60.97% |
| [Render/filters](#renderfilters) | 60,075 | 0% | -32.25% | 46 | 0% | -37.84% | 4,232 | 0% | -13.84% |
| [Load/for_filter_if](#loadfor_filter_if) | 44,029 | -0.45% | -67.48% | 15 | 0% | -87.80% | 2,288 | -0.69% | -33.64% |
| [Render/for_filter_if](#renderfor_filter_if) | 463,615 | -0.03% | -46.43% | 206 | 0% | -8.85% | 2,568 | 0% | -29.45% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 59,403 | -0.48% | -67.12% | 17 | 0% | -89.17% | 2,888 | -0.55% | -36.11% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 276,231 | 0% | -74.25% | 2 | 0% | -99.62% | 1,552 | 0% | -43.93% |
| [Load/for_range](#loadfor_range) | 21,446 | -0.46% | -62.17% | 12 | 0% | -78.57% | 1,432 | +1.13% | -28.40% |
| [Render/for_range](#renderfor_range) | 46,748 | 0% | -49.30% | 3 | 0% | -88.89% | 440 | 0% | -70.90% |
| [Load/html_autoescape](#loadhtml_autoescape) | 299,052 | -0.28% |  | 71 | -1.39% |  | 16,000 | +0.20% | -35.17% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,614,157 | 0% |  | 1,011 | 0% |  | 38,752 | 0% | -4.04% |
| [Load/inheritance](#loadinheritance) | 38,257 | -0.49% | -66.48% | 17 | 0% | -84.11% | 1,960 | 0% | -31.75% |
| [Render/inheritance](#renderinheritance) | 213,120 | -0.09% | -71.50% | 76 | 0% | -87.78% | 3,328 | 0% | -44.68% |
| [Load/large_static](#loadlarge_static) | 242,544 | -0.62% | -72.62% | 29 | -3.33% | -93.41% | 45,216 | -0.21% | -15.30% |
| [Render/large_static](#renderlarge_static) | 27,369 | 0% | -63.57% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [Load/macros](#loadmacros) | 84,383 | -0.27% | -62.98% | 34 | 0% | -81.72% | 4,624 | 0% | -25.61% |
| [Render/macros](#rendermacros) | 1,017,221 | 0% | -60.31% | 407 | 0% | -85.09% | 13,592 | 0% | -7.66% |
| [Load/many_tags](#loadmany_tags) | 13,800,119 | -0.71% | -68.75% | 1,560 | -0.06% | -95.45% | 542,976 | -0.86% | -48.54% |
| [Render/many_tags](#rendermany_tags) | 1,269,150 | 0% | -39.07% | 2 | 0% | -90.48% | 5,632 | 0% | -10.77% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 84,600 | -0.52% | -64.39% | 32 | 0% | -82.61% | 4,328 | 0% | -28.63% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 6,653,396 | 0% | -46.15% | 1,025 | 0% | -74.71% | 345,864 | 0% | -0.33% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 103,986 | -0.37% |  | 67 | 0% |  | 6,512 | 0% | -21.20% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 7,024,442 | 0% |  | 1,046 | 0% |  | 1,379,848 | 0% | -0.08% |
| [Load/plain_text](#loadplain_text) | 4,462 | -0.95% | -53.89% | 6 | 0% | -66.67% | 752 | 0% | -32.86% |
| [Render/plain_text](#renderplain_text) | 985 | -0.10% | -85.51% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [Load/strings](#loadstrings) | 108,085 | -0.06% | -62.97% | 37 | 0% | -83.48% | 5,376 | +0.30% | -40.53% |
| [Render/strings](#renderstrings) | 1,062,573 | 0% | -49.34% | 975 | 0% | -53.88% | 17,248 | 0% | -3.79% |
| [Load/substitute](#loadsubstitute) | 11,280 | -0.89% | -62.22% | 9 | 0% | -75.68% | 944 | -1.67% | -37.89% |
| [Render/substitute](#rendersubstitute) | 2,665 | -0.04% | -69.77% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 1,604 | -8.92% | 1,649 | -1.02% | 0 |  | 8,494 | -2.08% |
| Render/chat_llama | 1,322 | -4.69% | 443 | -3.06% | 0 |  | 5,061 | -2.62% |
| Load/chat_mistral | 2,278 | -7.55% | 2,250 | -0.92% | 0 |  | 9,763 | -2.81% |
| Render/chat_mistral | 2,479 | -2.71% | 675 | -5.46% | 0 |  | 14,684 | -2.68% |
| Load/chat_qwen | 1,059 | -8.07% | 1,369 | -1.79% | 0 |  | 5,265 | -2.36% |
| Render/chat_qwen | 1,276 | +1.84% | 333 | +4.06% | 0 |  | 4,450 | -3.34% |
| Load/config_file | 404 | -2.65% | 595 | +1.88% | 0 |  | 3,440 | -2.55% |
| Render/config_file | 7,548 | -10.80% | 2,118 | -2.08% | 0 |  | 70,236 | +1.13% |
| Load/dict_ops | 97 | -7.62% | 111 | +6.73% | 0 |  | 1,733 | -1.59% |
| Render/dict_ops | 1,027 | -0.96% | 625 | +0.16% | 0 |  | 1,135 | -0.53% |
| Load/expressions | 151 | -6.21% | 229 | -4.58% | 0 |  | 1,554 | -2.57% |
| Render/expressions | 9 | -35.71% | 3 | -40.00% | 0 |  | 451 | -3.84% |
| Load/filters | 205 | -8.07% | 165 | -4.62% | 0 |  | 2,313 | -4.54% |
| Render/filters | 98 | -2.97% | 72 | -6.49% | 0 |  | 1,207 | +3.69% |
| Load/for_filter_if | 38 | -22.45% | 29 | -17.14% | 0 |  | 1,199 | -5.81% |
| Render/for_filter_if | 1,304 | -0.23% | 52 | 0% | 0 |  | 439 | -1.79% |
| Load/for_loop_vars | 59 | -32.95% | 84 | -15.15% | 0 |  | 1,377 | -5.88% |
| Render/for_loop_vars | 6 | -62.50% | 2 | -50.00% | 0 |  | 266 | -66.16% |
| Load/for_range | 22 | -47.62% | 24 | -33.33% | 0 |  | 1,015 | -1.74% |
| Render/for_range | 2 | -33.33% | 2 | 0% | 0 |  | 217 | -6.87% |
| Load/html_autoescape | 885 | -8.67% | 989 | -1.79% | 0 |  | 4,469 | -4.61% |
| Render/html_autoescape | 6,840 | +1.11% | 2,189 | -0.95% | 0 |  | 50,085 | -1.38% |
| Load/inheritance | 27 | -10.00% | 18 | -21.74% | 0 |  | 1,089 | -1.27% |
| Render/inheritance | 508 | -2.12% | 98 | -2.00% | 0 |  | 1,219 | +43.92% |
| Load/large_static | 2,614 | -1.47% | 1,249 | -0.64% | 0 |  | 434 | -4.19% |
| Render/large_static | 750 | -0.40% | 582 | 0% | 0 |  | 5 | +25.00% |
| Load/macros | 212 | -9.01% | 293 | -1.68% | 0 |  | 1,946 | -1.32% |
| Render/macros | 715 | +2.44% | 214 | -0.93% | 0 |  | 7,391 | -25.62% |
| Load/many_tags | 46,669 | -6.86% | 34,494 | +0.02% | 0 |  | 279,037 | -5.55% |
| Render/many_tags | 8,305 | -0.13% | 1,760 | -0.51% | 0 |  | 3,717 | -15.96% |
| Load/mitsuhiko_table | 117 | -4.10% | 225 | -5.46% | 0 |  | 1,380 | -6.44% |
| Render/mitsuhiko_table | 8,029 | -0.09% | 5,009 | 0% | 0 |  | 476 | -89.65% |
| Load/mitsuhiko_table_wide | 206 | -13.45% | 337 | -1.17% | 0 |  | 1,900 | +0.96% |
| Render/mitsuhiko_table_wide | 8,081 | -2.67% | 21,457 | -0.02% | 0 |  | 505 | -89.01% |
| Load/plain_text | 0 |  | 1 | 0% | 0 |  | 6 | -50.00% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 5 | 0% |
| Load/strings | 274 | -4.86% | 315 | +1.29% | 0 |  | 3,230 | -0.89% |
| Render/strings | 929 | -1.59% | 1,344 | +0.22% | 0 |  | 3,536 | -12.24% |
| Load/substitute | 3 | -25.00% | 6 | 0% | 0 |  | 349 | -13.40% |
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
