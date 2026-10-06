# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (45 so far, latest [1ad80c3](https://github.com/jinja2cpp/Jinja2Cpp/commit/1ad80c3cb162c6ea6160e0120183e37358e3f9e1) on 2026-10-07). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 527,615 | -0.09% |  | 129 | 0% |  | 39,000 | 0% | -0.20% |
| [Render/chat_llama](#renderchat_llama) | 416,318 | -0.03% |  | 348 | 0% |  | 13,216 | 0% | -7.97% |
| [Load/chat_mistral](#loadchat_mistral) | 600,436 | -0.01% |  | 116 | 0% |  | 38,560 | 0% | -13.64% |
| [Render/chat_mistral](#renderchat_mistral) | 724,216 | -1.71% |  | 474 | -5.77% |  | 11,584 | -0.41% | -12.30% |
| [Load/chat_qwen](#loadchat_qwen) | 387,803 | +0.07% |  | 94 | 0% |  | 36,976 | 0% | +35.30% |
| [Render/chat_qwen](#renderchat_qwen) | 403,943 | -2.05% |  | 337 | -5.87% |  | 7,360 | 0% | -13.45% |
| [Load/config_file](#loadconfig_file) | 130,691 | +0.26% |  | 41 | 0% |  | 9,208 | 0% | -12.80% |
| [Render/config_file](#renderconfig_file) | 1,982,346 | -1.34% |  | 1,710 | -3.39% |  | 21,432 | -0.07% | -9.98% |
| [Load/dict_ops](#loaddict_ops) | 45,606 | +0.06% | -69.14% | 19 | 0% | -85.50% | 4,632 | 0% | +8.83% |
| [Render/dict_ops](#renderdict_ops) | 404,684 | 0% | -41.42% | 316 | 0% | -29.78% | 34,048 | 0% | -1.87% |
| [Load/expressions](#loadexpressions) | 61,571 | -0.07% | -70.13% | 24 | 0% | -83.56% | 8,976 | 0% | +46.28% |
| [Render/expressions](#renderexpressions) | 530,929 | 0% | -41.41% | 3 | 0% | -90.00% | 2,472 | 0% | -30.25% |
| [Load/filters](#loadfilters) | 92,016 | -0.35% | -66.47% | 46 | 0% | -79.82% | 9,504 | 0% | -16.75% |
| [Render/filters](#renderfilters) | 59,806 | 0% | -32.55% | 46 | 0% | -37.84% | 4,232 | 0% | -13.84% |
| [Load/for_filter_if](#loadfor_filter_if) | 36,811 | +0.36% | -72.81% | 15 | 0% | -87.80% | 4,336 | 0% | +25.75% |
| [Render/for_filter_if](#renderfor_filter_if) | 515,669 | 0% | -40.42% | 206 | 0% | -8.85% | 2,552 | 0% | -29.89% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 50,195 | +0.45% | -72.22% | 17 | 0% | -89.17% | 4,408 | 0% | -2.48% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 277,001 | -53.86% | -74.18% | 2 | -99.60% | -99.62% | 1,552 | -4.43% | -43.93% |
| [Load/for_range](#loadfor_range) | 18,698 | 0% | -67.01% | 12 | 0% | -78.57% | 2,224 | 0% | +11.20% |
| [Render/for_range](#renderfor_range) | 47,334 | 0% | -48.67% | 3 | 0% | -88.89% | 440 | 0% | -70.90% |
| [Load/html_autoescape](#loadhtml_autoescape) | 255,504 | +0.26% |  | 73 | 0% |  | 35,976 | 0% | +45.77% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,679,439 | -5.05% |  | 1,012 | -5.60% |  | 38,680 | 0% | -4.22% |
| [Load/inheritance](#loadinheritance) | 33,948 | -0.13% | -70.25% | 17 | 0% | -84.11% | 2,528 | 0% | -11.98% |
| [Render/inheritance](#renderinheritance) | 202,734 | 0% | -72.89% | 78 | 0% | -87.46% | 3,264 | 0% | -45.74% |
| [Load/large_static](#loadlarge_static) | 218,623 | 0% | -75.32% | 28 | 0% | -93.64% | 53,352 | 0% | -0.06% |
| [Render/large_static](#renderlarge_static) | 26,629 | 0% | -64.56% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [Load/macros](#loadmacros) | 72,855 | +0.02% | -68.04% | 34 | 0% | -81.72% | 9,368 | 0% | +50.71% |
| [Render/macros](#rendermacros) | 1,150,403 | 0% | -55.12% | 407 | 0% | -85.09% | 13,576 | 0% | -7.77% |
| [Load/many_tags](#loadmany_tags) | 12,050,426 | +0.04% | -72.71% | 2,155 | 0% | -93.71% | 801,528 | 0% | -24.04% |
| [Render/many_tags](#rendermany_tags) | 1,265,428 | 0% | -39.25% | 2 | 0% | -90.48% | 5,632 | 0% | -10.77% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 74,307 | -0.17% | -68.72% | 32 | 0% | -82.61% | 9,136 | 0% | +50.66% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 6,973,771 | 0% | -43.56% | 1,025 | 0% | -74.71% | 345,864 | 0% | -0.33% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 93,810 | -0.13% |  | 67 | 0% |  | 11,320 | 0% | +36.98% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 7,324,773 | 0% |  | 1,046 | 0% |  | 1,379,848 | 0% | -0.08% |
| [Load/plain_text](#loadplain_text) | 3,833 | 0% | -60.39% | 6 | 0% | -66.67% | 1,152 | 0% | +2.86% |
| [Render/plain_text](#renderplain_text) | 994 | 0% | -85.37% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [Load/strings](#loadstrings) | 95,421 | -0.16% | -67.30% | 43 | 0% | -80.80% | 9,712 | 0% | +7.43% |
| [Render/strings](#renderstrings) | 1,055,765 | -4.44% | -49.67% | 976 | -9.96% | -53.83% | 17,248 | 0% | -3.79% |
| [Load/substitute](#loadsubstitute) | 10,226 | 0% | -65.75% | 10 | 0% | -72.97% | 2,200 | 0% | +44.74% |
| [Render/substitute](#rendersubstitute) | 2,614 | 0% | -70.35% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 1,133 | -0.87% | 1,223 | +0.41% | 0 |  | 7,698 | -6.35% |
| Render/chat_llama | 1,670 | +4.64% | 523 | +4.81% | 0 |  | 5,008 | -3.30% |
| Load/chat_mistral | 1,313 | +0.08% | 1,585 | +0.83% | 0 |  | 8,455 | -5.26% |
| Render/chat_mistral | 3,164 | -2.71% | 786 | -5.76% | 0 |  | 13,609 | -3.90% |
| Load/chat_qwen | 826 | -1.31% | 1,148 | +2.04% | 0 |  | 4,544 | -9.50% |
| Render/chat_qwen | 1,723 | -1.54% | 386 | +1.05% | 0 |  | 4,361 | -5.22% |
| Load/config_file | 379 | +3.55% | 452 | +0.67% | 0 |  | 2,925 | -1.32% |
| Render/config_file | 10,926 | -6.09% | 2,505 | -4.28% | 0 |  | 70,301 | +2.94% |
| Load/dict_ops | 100 | +8.70% | 61 | +12.96% | 0 |  | 1,406 | -0.21% |
| Render/dict_ops | 1,173 | +0.43% | 624 | 0% | 0 |  | 1,198 | +2.13% |
| Load/expressions | 122 | +7.96% | 133 | +9.92% | 0 |  | 1,238 | -9.96% |
| Render/expressions | 15 | +50.00% | 3 | 0% | 0 |  | 467 | -2.51% |
| Load/filters | 233 | +10.95% | 136 | +3.82% | 0 |  | 2,158 | -6.05% |
| Render/filters | 158 | -5.39% | 85 | -6.59% | 0 |  | 1,133 | -3.33% |
| Load/for_filter_if | 37 | -2.63% | 24 | +14.29% | 0 |  | 940 | -3.19% |
| Render/for_filter_if | 1,250 | +0.56% | 50 | -1.96% | 0 |  | 2,111 | +407.45% |
| Load/for_loop_vars | 61 | +15.09% | 55 | +1.85% | 0 |  | 1,070 | -1.56% |
| Render/for_loop_vars | 24 | -66.20% | 10 | -50.00% | 0 |  | 290 | -90.12% |
| Load/for_range | 21 | +5.00% | 18 | 0% | 0 |  | 769 | -2.16% |
| Render/for_range | 1 | 0% | 0 | -100.00% | 0 |  | 202 | -8.60% |
| Load/html_autoescape | 702 | +1.45% | 796 | 0% | 0 |  | 3,893 | -6.19% |
| Render/html_autoescape | 9,110 | -23.32% | 2,129 | -13.84% | 0 |  | 51,804 | -14.17% |
| Load/inheritance | 28 | +27.27% | 25 | +8.70% | 0 |  | 811 | -0.12% |
| Render/inheritance | 533 | -1.84% | 102 | +3.03% | 0 |  | 784 | -46.30% |
| Load/large_static | 2,598 | -0.04% | 1,178 | 0% | 0 |  | 330 | -20.86% |
| Render/large_static | 820 | 0% | 576 | 0% | 0 |  | 3 | -72.73% |
| Load/macros | 151 | +1.34% | 140 | +2.94% | 0 |  | 1,616 | -2.24% |
| Render/macros | 667 | 0% | 224 | +0.45% | 0 |  | 12,494 | +24.73% |
| Load/many_tags | 25,179 | -0.21% | 21,967 | -1.97% | 0 |  | 274,767 | -1.99% |
| Render/many_tags | 11,582 | +0.17% | 1,852 | +0.27% | 0 |  | 5,017 | -21.45% |
| Load/mitsuhiko_table | 86 | +21.13% | 122 | +22.00% | 0 |  | 1,057 | +0.19% |
| Render/mitsuhiko_table | 8,058 | +0.14% | 5,121 | 0% | 0 |  | 543 | +0.37% |
| Load/mitsuhiko_table_wide | 202 | +12.22% | 282 | +4.44% | 0 |  | 1,443 | -1.10% |
| Render/mitsuhiko_table_wide | 8,153 | -0.01% | 21,521 | +0.02% | 0 |  | 572 | -88.40% |
| Load/plain_text | 0 |  | 0 |  | 0 |  | 7 | 0% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 4 | +33.33% |
| Load/strings | 274 | -1.44% | 219 | +3.30% | 0 |  | 2,688 | -0.19% |
| Render/strings | 1,054 | +3.64% | 1,345 | +0.37% | 0 |  | 2,148 | -62.22% |
| Load/substitute | 3 | 0% | 5 | 0% | 0 |  | 278 | -6.71% |
| Render/substitute | 0 | -100.00% | 0 |  | 0 |  | 3 | +50.00% |

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
