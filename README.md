# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (42 so far, latest [0892811](https://github.com/jinja2cpp/Jinja2Cpp/commit/0892811d53a0664e3cbe798ffcf8639ea6be5bdd) on 2026-10-06). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 650,471 | 0% |  | 390 | 0% |  | 31,160 | 0% | -20.27% |
| [Render/chat_llama](#renderchat_llama) | 420,599 | +0.06% |  | 348 | 0% |  | 13,216 | 0% | -7.97% |
| [Load/chat_mistral](#loadchat_mistral) | 760,876 | 0% |  | 442 | 0% |  | 38,608 | 0% | -13.53% |
| [Render/chat_mistral](#renderchat_mistral) | 740,952 | +0.06% |  | 503 | 0% |  | 11,616 | 0% | -12.05% |
| [Load/chat_qwen](#loadchat_qwen) | 485,566 | 0% |  | 291 | 0% |  | 24,608 | 0% | -9.95% |
| [Render/chat_qwen](#renderchat_qwen) | 416,741 | +0.09% |  | 358 | 0% |  | 7,360 | 0% | -13.45% |
| [Load/config_file](#loadconfig_file) | 163,609 | 0% |  | 116 | 0% |  | 9,064 | 0% | -14.17% |
| [Render/config_file](#renderconfig_file) | 2,017,818 | +0.28% |  | 1,770 | 0% |  | 21,416 | 0% | -10.05% |
| [Load/dict_ops](#loaddict_ops) | 51,492 | 0% | -65.16% | 42 | 0% | -67.94% | 3,376 | 0% | -20.68% |
| [Render/dict_ops](#renderdict_ops) | 404,184 | 0% | -41.49% | 316 | 0% | -29.78% | 34,016 | 0% | -1.96% |
| [Load/expressions](#loadexpressions) | 74,021 | 0% | -64.09% | 62 | 0% | -57.53% | 5,368 | 0% | -12.52% |
| [Render/expressions](#renderexpressions) | 531,392 | 0% | -41.36% | 3 | 0% | -90.00% | 2,472 | 0% | -30.25% |
| [Load/filters](#loadfilters) | 117,625 | 0% | -57.14% | 106 | 0% | -53.51% | 6,288 | 0% | -44.92% |
| [Render/filters](#renderfilters) | 60,075 | +0.12% | -32.25% | 46 | 0% | -37.84% | 4,232 | 0% | -13.84% |
| [Load/for_filter_if](#loadfor_filter_if) | 42,262 | 0% | -68.78% | 36 | 0% | -70.73% | 3,016 | 0% | -12.53% |
| [Render/for_filter_if](#renderfor_filter_if) | 514,402 | +0.24% | -40.57% | 206 | 0% | -8.85% | 2,552 | 0% | -29.89% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 59,590 | 0% | -67.02% | 48 | 0% | -69.43% | 3,928 | 0% | -13.10% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 603,720 | +0.22% | -43.72% | 502 | 0% | -3.83% | 1,624 | 0% | -41.33% |
| [Load/for_range](#loadfor_range) | 20,469 | 0% | -63.89% | 20 | 0% | -64.29% | 1,648 | 0% | -17.60% |
| [Render/for_range](#renderfor_range) | 47,424 | -0.04% | -48.57% | 3 | 0% | -88.89% | 440 | 0% | -70.90% |
| [Load/html_autoescape](#loadhtml_autoescape) | 342,274 | 0% |  | 254 | 0% |  | 20,912 | 0% | -15.27% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,771,951 | +0.15% |  | 1,072 | 0% |  | 38,664 | 0% | -4.26% |
| [Load/inheritance](#loadinheritance) | 37,608 | 0% | -67.04% | 33 | 0% | -69.16% | 2,568 | 0% | -10.58% |
| [Render/inheritance](#renderinheritance) | 202,576 | -0.07% | -72.91% | 78 | 0% | -87.46% | 3,248 | 0% | -46.01% |
| [Load/large_static](#loadlarge_static) | 287,501 | 0% | -67.55% | 178 | 0% | -59.55% | 48,472 | 0% | -9.20% |
| [Render/large_static](#renderlarge_static) | 26,730 | -0.19% | -64.42% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [Load/macros](#loadmacros) | 86,268 | 0% | -62.16% | 72 | 0% | -61.29% | 5,464 | 0% | -12.10% |
| [Render/macros](#rendermacros) | 1,149,841 | +0.27% | -55.14% | 407 | 0% | -85.09% | 13,576 | 0% | -7.77% |
| [Load/many_tags](#loadmany_tags) | 16,121,785 | 0% | -63.50% | 10,238 | 0% | -70.11% | 784,040 | 0% | -25.70% |
| [Render/many_tags](#rendermany_tags) | 1,262,551 | +0.44% | -39.39% | 2 | 0% | -90.48% | 5,632 | 0% | -10.77% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 87,350 | 0% | -63.23% | 72 | 0% | -60.87% | 5,392 | 0% | -11.08% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 6,973,779 | +0.03% | -43.56% | 1,025 | 0% | -74.71% | 345,864 | 0% | -0.33% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 108,032 | 0% |  | 107 | 0% |  | 7,576 | 0% | -8.33% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 7,324,868 | +0.03% |  | 1,046 | 0% |  | 1,379,848 | 0% | -0.08% |
| [Load/plain_text](#loadplain_text) | 4,121 | 0% | -57.41% | 8 | 0% | -55.56% | 848 | 0% | -24.29% |
| [Render/plain_text](#renderplain_text) | 995 | 0% | -85.36% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [Load/strings](#loadstrings) | 112,922 | 0% | -61.31% | 90 | 0% | -59.82% | 6,680 | 0% | -26.11% |
| [Render/strings](#renderstrings) | 1,106,691 | -0.02% | -47.24% | 1,084 | 0% | -48.72% | 17,248 | 0% | -3.79% |
| [Load/substitute](#loadsubstitute) | 11,103 | 0% | -62.81% | 15 | 0% | -59.46% | 1,088 | 0% | -28.42% |
| [Render/substitute](#rendersubstitute) | 2,615 | -0.08% | -70.33% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 1,811 | 0% | 1,246 | 0% | 0 |  | 9,602 | -0.43% |
| Render/chat_llama | 1,773 | 0% | 482 | 0% | 0 |  | 4,758 | -1.06% |
| Load/chat_mistral | 2,177 | 0% | 1,724 | +0.06% | 0 |  | 11,069 | -1.74% |
| Render/chat_mistral | 3,450 | -0.06% | 761 | 0% | 0 |  | 13,794 | -0.80% |
| Load/chat_qwen | 1,316 | 0% | 1,209 | 0% | 0 |  | 5,886 | +1.40% |
| Render/chat_qwen | 1,836 | 0% | 383 | 0% | 0 |  | 4,218 | -5.89% |
| Load/config_file | 628 | 0% | 533 | 0% | 0 |  | 3,652 | +0.66% |
| Render/config_file | 12,370 | -0.02% | 2,543 | +0.08% | 0 |  | 70,393 | +3.13% |
| Load/dict_ops | 138 | 0% | 84 | 0% | 0 |  | 1,544 | +1.05% |
| Render/dict_ops | 1,092 | -0.09% | 616 | 0% | 0 |  | 1,184 | -1.42% |
| Load/expressions | 199 | 0% | 173 | 0% | 0 |  | 1,434 | -0.35% |
| Render/expressions | 6 | -33.33% | 1 | -66.67% | 0 |  | 432 | -0.92% |
| Load/filters | 326 | 0% | 148 | 0% | 0 |  | 2,732 | +3.25% |
| Render/filters | 169 | 0% | 79 | 0% | 0 |  | 1,163 | +3.47% |
| Load/for_filter_if | 82 | 0% | 39 | 0% | 0 |  | 1,027 | -1.53% |
| Render/for_filter_if | 1,256 | 0% | 52 | 0% | 0 |  | 411 | -2.84% |
| Load/for_loop_vars | 120 | 0% | 89 | 0% | 0 |  | 1,227 | -1.29% |
| Render/for_loop_vars | 777 | 0% | 8 | 0% | 0 |  | 3,905 | +72.94% |
| Load/for_range | 37 | 0% | 28 | 0% | 0 |  | 845 | +1.81% |
| Render/for_range | 1 | -80.00% | 1 | -50.00% | 0 |  | 225 | -4.26% |
| Load/html_autoescape | 1,090 | 0% | 798 | 0% | 0 |  | 5,157 | -1.81% |
| Render/html_autoescape | 12,486 | +0.01% | 2,887 | 0% | 0 |  | 59,033 | -0.37% |
| Load/inheritance | 43 | 0% | 27 | 0% | 0 |  | 904 | +0.56% |
| Render/inheritance | 550 | 0% | 102 | 0% | 0 |  | 1,157 | +9.15% |
| Load/large_static | 2,759 | 0% | 1,209 | 0% | 0 |  | 430 | +5.91% |
| Render/large_static | 832 | 0% | 576 | 0% | 0 |  | 14 | +250.00% |
| Load/macros | 258 | 0% | 243 | 0% | 0 |  | 1,796 | -2.44% |
| Render/macros | 668 | +0.15% | 220 | 0% | 0 |  | 7,427 | +78.32% |
| Load/many_tags | 52,267 | 0% | 24,227 | 0% | 0 |  | 321,015 | +1.79% |
| Render/many_tags | 11,831 | 0% | 1,868 | 0% | 0 |  | 2,036 | +67.99% |
| Load/mitsuhiko_table | 183 | 0% | 201 | 0% | 0 |  | 1,170 | +1.30% |
| Render/mitsuhiko_table | 8,062 | 0% | 5,122 | 0% | 0 |  | 9,526 | +1548.10% |
| Load/mitsuhiko_table_wide | 316 | 0% | 337 | 0% | 0 |  | 1,795 | -1.10% |
| Render/mitsuhiko_table_wide | 8,125 | 0% | 21,518 | 0% | 0 |  | 10,267 | -0.63% |
| Load/plain_text | 0 |  | 0 |  | 0 |  | 8 | -55.56% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 5 | -28.57% |
| Load/strings | 412 | 0% | 265 | 0% | 0 |  | 3,229 | +3.86% |
| Render/strings | 937 | -0.11% | 1,333 | 0% | 0 |  | 2,178 | -36.70% |
| Load/substitute | 4 | 0% | 4 | 0% | 0 |  | 273 | +7.06% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 6 | +50.00% |

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
