# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (67 so far, latest [84f2ea0](https://github.com/jinja2cpp/Jinja2Cpp/commit/84f2ea0ffb30a7a3460c758ed1f5133b34201d64) on 2026-10-11). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 516,769 | -6.79% |  | 92 | 0% |  | 19,856 | 0% | -49.19% |
| [Render/chat_llama](#renderchat_llama) | 405,775 | 0% |  | 355 | 0% |  | 15,312 | 0% | +6.63% |
| [Load/chat_mistral](#loadchat_mistral) | 611,136 | -7.04% |  | 75 | 0% |  | 23,832 | 0% | -46.62% |
| [Render/chat_mistral](#renderchat_mistral) | 643,059 | 0% |  | 421 | 0% |  | 12,184 | 0% | -7.75% |
| [Load/chat_qwen](#loadchat_qwen) | 395,383 | -6.80% |  | 68 | 0% |  | 15,760 | 0% | -42.33% |
| [Render/chat_qwen](#renderchat_qwen) | 387,943 | 0% |  | 320 | 0% |  | 9,400 | 0% | +10.54% |
| [Load/config_file](#loadconfig_file) | 136,693 | -6.37% |  | 34 | 0% |  | 5,576 | 0% | -47.20% |
| [Render/config_file](#renderconfig_file) | 1,690,298 | 0% |  | 1,124 | 0% |  | 25,312 | 0% | +6.32% |
| [Load/dict_ops](#loaddict_ops) | 44,503 | -6.64% | -69.89% | 12 | 0% | -90.84% | 2,224 | 0% | -47.74% |
| [Render/dict_ops](#renderdict_ops) | 350,090 | 0% | -49.32% | 114 | 0% | -74.67% | 36,168 | 0% | +4.24% |
| [Load/expressions](#loadexpressions) | 61,193 | -8.79% | -70.32% | 18 | 0% | -87.67% | 3,560 | 0% | -41.98% |
| [Render/expressions](#renderexpressions) | 517,323 | 0% | -42.91% | 3 | 0% | -90.00% | 2,472 | 0% | -30.25% |
| [Load/filters](#loadfilters) | 91,015 | -6.54% | -66.84% | 32 | 0% | -85.96% | 3,896 | 0% | -65.87% |
| [Render/filters](#renderfilters) | 52,811 | 0% | -40.44% | 37 | 0% | -50.00% | 2,568 | 0% | -47.72% |
| [Load/for_filter_if](#loadfor_filter_if) | 39,072 | -5.02% | -71.14% | 9 | 0% | -92.68% | 1,936 | 0% | -43.85% |
| [Render/for_filter_if](#renderfor_filter_if) | 340,600 | 0% | -60.65% | 6 | 0% | -97.35% | 13,904 | 0% | +281.98% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 53,739 | -6.33% | -70.25% | 13 | 0% | -91.72% | 2,416 | 0% | -46.55% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 272,737 | 0% | -74.58% | 4 | 0% | -99.23% | 2,128 | 0% | -23.12% |
| [Load/for_range](#loadfor_range) | 18,390 | -6.03% | -67.56% | 10 | 0% | -82.14% | 1,136 | 0% | -43.20% |
| [Render/for_range](#renderfor_range) | 46,279 | 0% | -49.81% | 3 | 0% | -88.89% | 440 | 0% | -70.90% |
| [Load/html_autoescape](#loadhtml_autoescape) | 269,676 | -6.81% |  | 61 | 0% |  | 12,944 | 0% | -47.55% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,563,404 | 0% |  | 938 | 0% |  | 45,760 | 0% | +13.31% |
| [Load/inheritance](#loadinheritance) | 32,893 | -5.31% | -71.18% | 10 | 0% | -90.65% | 1,488 | 0% | -48.19% |
| [Render/inheritance](#renderinheritance) | 149,869 | 0% | -79.96% | 34 | 0% | -94.53% | 7,176 | 0% | +19.28% |
| [Load/large_static](#loadlarge_static) | 220,869 | -6.71% | -75.07% | 28 | 0% | -93.64% | 42,720 | 0% | -19.98% |
| [Render/large_static](#renderlarge_static) | 21,471 | 0% | -71.42% | 3 | 0% | -83.33% | 37,112 | 0% | -0.32% |
| [Load/macros](#loadmacros) | 76,056 | -6.82% | -66.64% | 32 | 0% | -82.80% | 5,128 | 0% | -17.50% |
| [Render/macros](#rendermacros) | 984,002 | 0% | -61.61% | 312 | 0% | -88.57% | 20,272 | 0% | +37.72% |
| [Load/many_tags](#loadmany_tags) | 12,357,046 | -7.06% | -72.02% | 962 | 0% | -97.19% | 445,680 | 0% | -57.76% |
| [Render/many_tags](#rendermany_tags) | 1,138,879 | 0% | -45.33% | 15 | 0% | -28.57% | 82,520 | 0% | +1207.35% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 70,988 | -9.04% | -70.12% | 26 | 0% | -85.87% | 3,552 | 0% | -41.42% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 6,557,215 | 0% | -46.93% | 35 | 0% | -99.14% | 373,872 | 0% | +7.74% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 89,416 | -7.25% |  | 57 | 0% |  | 5,736 | 0% | -30.59% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 6,927,680 | 0% |  | 53 | 0% |  | 1,407,264 | 0% | +1.90% |
| [Load/plain_text](#loadplain_text) | 4,015 | -1.06% | -58.51% | 5 | 0% | -72.22% | 688 | 0% | -38.57% |
| [Render/plain_text](#renderplain_text) | 986 | 0% | -85.49% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [Load/strings](#loadstrings) | 94,415 | -6.83% | -67.65% | 33 | 0% | -85.27% | 4,504 | 0% | -50.18% |
| [Render/strings](#renderstrings) | 1,055,690 | 0% | -49.67% | 979 | 0% | -53.69% | 18,384 | 0% | +2.54% |
| [Load/substitute](#loadsubstitute) | 10,129 | -6.26% | -66.07% | 8 | 0% | -78.38% | 832 | 0% | -45.26% |
| [Render/substitute](#rendersubstitute) | 2,684 | 0% | -69.55% | 3 | 0% | -75.00% | 600 | 0% | -16.67% |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 1,075 | -9.44% | 1,157 | -16.52% | 0 |  | 7,858 | -8.46% |
| Render/chat_llama | 1,297 | +0.31% | 511 | +0.59% | 0 |  | 4,906 | +7.54% |
| Load/chat_mistral | 1,675 | -6.89% | 1,686 | -18.20% | 0 |  | 9,093 | -8.05% |
| Render/chat_mistral | 2,191 | +0.83% | 547 | +0.55% | 0 |  | 13,476 | +3.27% |
| Load/chat_qwen | 798 | -4.77% | 1,090 | -11.60% | 0 |  | 5,189 | -5.29% |
| Render/chat_qwen | 1,247 | +0.40% | 395 | 0% | 0 |  | 4,369 | -0.64% |
| Load/config_file | 316 | -16.40% | 432 | -23.54% | 0 |  | 3,223 | -9.57% |
| Render/config_file | 7,491 | +1.77% | 2,091 | -0.90% | 0 |  | 62,642 | +0.30% |
| Load/dict_ops | 64 | -32.63% | 47 | -57.66% | 0 |  | 1,448 | -9.39% |
| Render/dict_ops | 959 | -0.10% | 730 | 0% | 0 |  | 1,103 | +2.13% |
| Load/expressions | 82 | -30.51% | 96 | -53.85% | 0 |  | 1,407 | -5.38% |
| Render/expressions | 9 | +12.50% | 2 | 0% | 0 |  | 481 | +1.05% |
| Load/filters | 186 | -7.00% | 118 | -19.18% | 0 |  | 2,335 | -8.72% |
| Render/filters | 60 | 0% | 33 | -5.71% | 0 |  | 1,147 | +1.24% |
| Load/for_filter_if | 26 | -36.59% | 18 | -48.57% | 0 |  | 1,147 | -0.26% |
| Render/for_filter_if | 1,590 | -0.06% | 69 | +1.47% | 0 |  | 258 | -4.80% |
| Load/for_loop_vars | 40 | -27.27% | 38 | -49.33% | 0 |  | 1,259 | -7.15% |
| Render/for_loop_vars | 12 | -7.69% | 9 | +12.50% | 0 |  | 277 | +6.95% |
| Load/for_range | 10 | -50.00% | 10 | -64.29% | 0 |  | 885 | -6.15% |
| Render/for_range | 2 | 0% | 2 | 0% | 0 |  | 210 | -4.98% |
| Load/html_autoescape | 613 | -5.55% | 779 | -11.88% | 0 |  | 4,609 | -8.06% |
| Render/html_autoescape | 5,278 | -2.67% | 1,382 | -7.81% | 0 |  | 49,089 | -0.72% |
| Load/inheritance | 14 | -12.50% | 13 | -27.78% | 0 |  | 830 | -11.04% |
| Render/inheritance | 583 | -0.17% | 127 | +0.79% | 0 |  | 906 | +2.37% |
| Load/large_static | 2,602 | -0.61% | 1,171 | -1.60% | 0 |  | 360 | -22.91% |
| Render/large_static | 711 | 0% | 579 | 0% | 0 |  | 4 | +33.33% |
| Load/macros | 174 | -13.43% | 221 | -19.34% | 0 |  | 1,856 | -7.85% |
| Render/macros | 786 | +0.26% | 229 | +0.44% | 0 |  | 7,580 | +38.73% |
| Load/many_tags | 36,735 | -7.29% | 28,814 | -12.22% | 0 |  | 268,794 | -10.67% |
| Render/many_tags | 6,880 | -0.01% | 1,460 | +0.14% | 0 |  | 2,095 | +38.01% |
| Load/mitsuhiko_table | 76 | -33.91% | 121 | -40.10% | 0 |  | 1,061 | -11.51% |
| Render/mitsuhiko_table | 9,313 | 0% | 6,503 | 0% | 0 |  | 572 | +10.00% |
| Load/mitsuhiko_table_wide | 167 | -13.02% | 253 | -24.25% | 0 |  | 1,441 | -21.56% |
| Render/mitsuhiko_table_wide | 9,412 | -0.06% | 22,994 | 0% | 0 |  | 582 | +7.78% |
| Load/plain_text | 0 |  | 3 | 0% | 0 |  | 14 | +100.00% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 4 | -20.00% |
| Load/strings | 219 | -14.45% | 209 | -28.42% | 0 |  | 2,929 | -8.47% |
| Render/strings | 928 | 0% | 1,345 | +0.07% | 0 |  | 3,384 | +72.21% |
| Load/substitute | 5 | -37.50% | 10 | -28.57% | 0 |  | 295 | -20.27% |
| Render/substitute | 0 |  | 1 | 0% | 0 |  | 4 | +33.33% |

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
