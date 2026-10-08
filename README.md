# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (51 so far, latest [5a037d0](https://github.com/jinja2cpp/Jinja2Cpp/commit/5a037d0975a7942c101b00e1976fa0c16dd859cc) on 2026-10-08). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 582,911 | +0.06% |  | 121 | 0% |  | 23,408 | 0% | -40.10% |
| [Render/chat_llama](#renderchat_llama) | 410,569 | 0% |  | 348 | 0% |  | 13,232 | 0% | -7.86% |
| [Load/chat_mistral](#loadchat_mistral) | 684,849 | +0.14% |  | 113 | 0% |  | 29,112 | 0% | -34.80% |
| [Render/chat_mistral](#renderchat_mistral) | 711,760 | 0% |  | 474 | 0% |  | 11,648 | 0% | -11.81% |
| [Load/chat_qwen](#loadchat_qwen) | 438,236 | +0.09% |  | 96 | 0% |  | 18,440 | 0% | -32.52% |
| [Render/chat_qwen](#renderchat_qwen) | 397,655 | 0% |  | 337 | 0% |  | 7,360 | 0% | -13.45% |
| [Load/config_file](#loadconfig_file) | 152,093 | +0.17% |  | 41 | 0% |  | 6,728 | 0% | -36.29% |
| [Render/config_file](#renderconfig_file) | 1,913,750 | -4.35% |  | 1,703 | 0% |  | 21,528 | 0% | -9.58% |
| [Load/dict_ops](#loaddict_ops) | 54,283 | +0.21% | -63.27% | 18 | 0% | -86.26% | 2,864 | 0% | -32.71% |
| [Render/dict_ops](#renderdict_ops) | 401,080 | 0% | -41.94% | 315 | 0% | -30.00% | 34,016 | 0% | -1.96% |
| [Load/expressions](#loadexpressions) | 72,326 | +0.14% | -64.92% | 23 | 0% | -84.25% | 4,384 | 0% | -28.55% |
| [Render/expressions](#renderexpressions) | 532,368 | 0% | -41.25% | 3 | 0% | -90.00% | 2,472 | 0% | -30.25% |
| [Load/filters](#loadfilters) | 100,595 | 0% | -63.35% | 33 | 0% | -85.53% | 4,488 | 0% | -60.69% |
| [Render/filters](#renderfilters) | 60,076 | 0% | -32.25% | 46 | 0% | -37.84% | 4,232 | 0% | -13.84% |
| [Load/for_filter_if](#loadfor_filter_if) | 44,502 | +0.19% | -67.13% | 15 | 0% | -87.80% | 2,304 | 0% | -33.18% |
| [Render/for_filter_if](#renderfor_filter_if) | 463,772 | 0% | -46.42% | 206 | 0% | -8.85% | 2,568 | 0% | -29.45% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 59,966 | +0.19% | -66.81% | 17 | 0% | -89.17% | 2,904 | 0% | -35.75% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 276,232 | 0% | -74.25% | 2 | 0% | -99.62% | 1,552 | 0% | -43.93% |
| [Load/for_range](#loadfor_range) | 21,820 | +0.15% | -61.51% | 12 | 0% | -78.57% | 1,416 | 0% | -29.20% |
| [Render/for_range](#renderfor_range) | 46,749 | 0% | -49.30% | 3 | 0% | -88.89% | 440 | 0% | -70.90% |
| [Load/html_autoescape](#loadhtml_autoescape) | 300,169 | +0.13% |  | 72 | 0% |  | 15,968 | 0% | -35.30% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,614,165 | -4.16% |  | 1,011 | 0% |  | 38,752 | 0% | -4.04% |
| [Load/inheritance](#loadinheritance) | 38,721 | +0.04% | -66.07% | 17 | 0% | -84.11% | 1,960 | 0% | -31.75% |
| [Render/inheritance](#renderinheritance) | 213,313 | 0% | -71.48% | 76 | 0% | -87.78% | 3,328 | 0% | -44.68% |
| [Load/large_static](#loadlarge_static) | 244,343 | 0% | -72.42% | 30 | 0% | -93.18% | 45,312 | 0% | -15.12% |
| [Render/large_static](#renderlarge_static) | 27,370 | 0% | -63.57% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [Load/macros](#loadmacros) | 84,892 | +2.16% | -62.76% | 34 | 0% | -81.72% | 4,624 | +2.48% | -25.61% |
| [Render/macros](#rendermacros) | 1,017,222 | -13.47% | -60.31% | 407 | 0% | -85.09% | 13,592 | 0% | -7.66% |
| [Load/many_tags](#loadmany_tags) | 13,899,297 | 0% | -68.53% | 1,561 | 0% | -95.44% | 547,696 | 0% | -48.10% |
| [Render/many_tags](#rendermany_tags) | 1,269,151 | 0% | -39.07% | 2 | 0% | -90.48% | 5,632 | 0% | -10.77% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 85,318 | +0.12% | -64.09% | 32 | 0% | -82.61% | 4,328 | 0% | -28.63% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 6,653,397 | 0% | -46.15% | 1,025 | 0% | -74.71% | 345,864 | 0% | -0.33% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 104,651 | +0.10% |  | 67 | 0% |  | 6,512 | 0% | -21.20% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 7,024,443 | 0% |  | 1,046 | 0% |  | 1,379,848 | 0% | -0.08% |
| [Load/plain_text](#loadplain_text) | 4,780 | 0% | -50.60% | 6 | 0% | -66.67% | 752 | 0% | -32.86% |
| [Render/plain_text](#renderplain_text) | 986 | 0% | -85.49% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [Load/strings](#loadstrings) | 108,429 | +0.10% | -62.85% | 37 | 0% | -83.48% | 5,360 | 0% | -40.71% |
| [Render/strings](#renderstrings) | 1,062,607 | 0% | -49.34% | 975 | 0% | -53.88% | 17,248 | 0% | -3.79% |
| [Load/substitute](#loadsubstitute) | 11,656 | 0% | -60.96% | 9 | 0% | -75.68% | 960 | 0% | -36.84% |
| [Render/substitute](#rendersubstitute) | 2,666 | 0% | -69.76% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 1,761 | -0.06% | 1,665 | +0.06% | 0 |  | 9,093 | +0.81% |
| Render/chat_llama | 1,387 | -0.29% | 457 | -0.22% | 0 |  | 4,963 | -2.76% |
| Load/chat_mistral | 2,465 | -0.68% | 2,271 | -0.22% | 0 |  | 10,534 | +2.33% |
| Render/chat_mistral | 2,548 | -0.12% | 714 | +0.28% | 0 |  | 14,204 | +0.74% |
| Load/chat_qwen | 1,152 | -2.21% | 1,394 | -0.92% | 0 |  | 5,742 | +1.25% |
| Render/chat_qwen | 1,253 | +0.24% | 320 | +0.95% | 0 |  | 4,282 | -1.56% |
| Load/config_file | 414 | -1.43% | 583 | -0.34% | 0 |  | 3,680 | +0.96% |
| Render/config_file | 8,462 | +3.13% | 2,163 | -4.55% | 0 |  | 70,432 | -5.61% |
| Load/dict_ops | 105 | +0.96% | 104 | 0% | 0 |  | 1,810 | -0.22% |
| Render/dict_ops | 1,037 | 0% | 624 | 0% | 0 |  | 1,128 | +0.98% |
| Load/expressions | 161 | -0.62% | 240 | -0.41% | 0 |  | 1,632 | +0.18% |
| Render/expressions | 14 | 0% | 5 | 0% | 0 |  | 460 | +2.91% |
| Load/filters | 223 | +0.45% | 173 | -0.57% | 0 |  | 2,521 | -3.67% |
| Render/filters | 101 | -1.94% | 77 | 0% | 0 |  | 1,177 | +1.73% |
| Load/for_filter_if | 49 | +6.52% | 35 | +16.67% | 0 |  | 1,316 | +2.09% |
| Render/for_filter_if | 1,307 | 0% | 52 | 0% | 0 |  | 1,805 | +320.75% |
| Load/for_loop_vars | 88 | +7.32% | 99 | +1.02% | 0 |  | 1,534 | +0.46% |
| Render/for_loop_vars | 16 | -23.81% | 4 | 0% | 0 |  | 316 | +6.76% |
| Load/for_range | 42 | +7.69% | 36 | +5.88% | 0 |  | 1,043 | -0.76% |
| Render/for_range | 3 | 0% | 2 | 0% | 0 |  | 209 | -7.93% |
| Load/html_autoescape | 968 | 0% | 1,007 | -0.30% | 0 |  | 5,171 | -2.54% |
| Render/html_autoescape | 6,765 | +8.80% | 2,210 | +13.10% | 0 |  | 50,552 | -8.97% |
| Load/inheritance | 30 | +15.38% | 22 | 0% | 0 |  | 1,140 | +1.51% |
| Render/inheritance | 519 | -0.57% | 100 | 0% | 0 |  | 846 | +0.71% |
| Load/large_static | 2,654 | 0% | 1,257 | 0% | 0 |  | 944 | -32.28% |
| Render/large_static | 753 | 0% | 582 | 0% | 0 |  | 4 | 0% |
| Load/macros | 233 | +3.56% | 298 | +6.05% | 0 |  | 2,059 | -0.48% |
| Render/macros | 698 | -4.12% | 216 | +0.93% | 0 |  | 6,730 | -44.95% |
| Load/many_tags | 50,105 | +0.13% | 34,486 | +0.28% | 0 |  | 304,215 | -0.88% |
| Render/many_tags | 8,316 | -0.06% | 1,769 | -0.11% | 0 |  | 696 | -86.63% |
| Load/mitsuhiko_table | 121 | 0% | 238 | -0.42% | 0 |  | 1,489 | -1.39% |
| Render/mitsuhiko_table | 8,036 | -0.02% | 5,009 | 0% | 0 |  | 4,016 | +773.04% |
| Load/mitsuhiko_table_wide | 238 | -0.42% | 341 | 0% | 0 |  | 1,859 | -1.01% |
| Render/mitsuhiko_table_wide | 8,303 | -0.01% | 21,461 | 0% | 0 |  | 13,203 | +2535.33% |
| Load/plain_text | 0 |  | 1 | 0% | 0 |  | 18 | +28.57% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 5 | 0% |
| Load/strings | 288 | 0% | 311 | -0.64% | 0 |  | 3,408 | +3.09% |
| Render/strings | 944 | 0% | 1,341 | -0.07% | 0 |  | 3,624 | +13.39% |
| Load/substitute | 4 | 0% | 6 | 0% | 0 |  | 407 | -3.55% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 5 | 0% |

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
