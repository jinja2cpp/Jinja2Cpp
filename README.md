# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (60 so far, latest [2486c22](https://github.com/jinja2cpp/Jinja2Cpp/commit/2486c22abf9438c076232ae4003134475617a31a) on 2026-10-10). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 553,683 | -0.09% |  | 92 | 0% |  | 20,128 | 0% | -48.50% |
| [Render/chat_llama](#renderchat_llama) | 410,946 | 0% |  | 348 | 0% |  | 13,216 | 0% | -7.97% |
| [Load/chat_mistral](#loadchat_mistral) | 653,374 | -0.05% |  | 74 | 0% |  | 24,200 | 0% | -45.80% |
| [Render/chat_mistral](#renderchat_mistral) | 661,493 | 0% |  | 426 | 0% |  | 10,488 | 0% | -20.59% |
| [Load/chat_qwen](#loadchat_qwen) | 423,129 | -0.10% |  | 68 | 0% |  | 16,000 | 0% | -41.45% |
| [Render/chat_qwen](#renderchat_qwen) | 397,330 | 0% |  | 337 | 0% |  | 7,360 | 0% | -13.45% |
| [Load/config_file](#loadconfig_file) | 144,958 | -0.56% |  | 34 | 0% |  | 5,656 | -1.12% | -46.44% |
| [Render/config_file](#renderconfig_file) | 1,915,162 | -0.06% |  | 1,703 | 0% |  | 21,544 | +0.07% | -9.51% |
| [Load/dict_ops](#loaddict_ops) | 47,361 | 0% | -67.95% | 12 | 0% | -90.84% | 2,240 | 0% | -47.37% |
| [Render/dict_ops](#renderdict_ops) | 400,135 | 0% | -42.08% | 315 | 0% | -30.00% | 34,016 | 0% | -1.96% |
| [Load/expressions](#loadexpressions) | 66,919 | -0.02% | -67.54% | 18 | 0% | -87.67% | 3,592 | 0% | -41.46% |
| [Render/expressions](#renderexpressions) | 518,971 | 0% | -42.73% | 3 | 0% | -90.00% | 2,472 | 0% | -30.25% |
| [Load/filters](#loadfilters) | 96,765 | 0% | -64.74% | 32 | 0% | -85.96% | 3,960 | 0% | -65.31% |
| [Render/filters](#renderfilters) | 54,020 | 0% | -39.08% | 35 | 0% | -52.70% | 1,992 | 0% | -59.45% |
| [Load/for_filter_if](#loadfor_filter_if) | 40,894 | -0.11% | -69.80% | 9 | 0% | -92.68% | 1,952 | 0% | -43.39% |
| [Render/for_filter_if](#renderfor_filter_if) | 447,512 | 0% | -48.29% | 206 | 0% | -8.85% | 2,568 | 0% | -29.45% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 56,873 | -0.11% | -68.52% | 13 | 0% | -91.72% | 2,448 | 0% | -45.84% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 275,055 | 0% | -74.36% | 2 | 0% | -99.62% | 1,552 | 0% | -43.93% |
| [Load/for_range](#loadfor_range) | 19,482 | -0.06% | -65.63% | 10 | 0% | -82.14% | 1,136 | 0% | -43.20% |
| [Render/for_range](#renderfor_range) | 46,776 | 0% | -49.27% | 3 | 0% | -88.89% | 440 | 0% | -70.90% |
| [Load/html_autoescape](#loadhtml_autoescape) | 286,698 | -0.24% |  | 61 | -3.17% |  | 13,152 | -1.97% | -46.71% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,611,985 | -0.06% |  | 1,011 | 0% |  | 38,752 | 0% | -4.04% |
| [Load/inheritance](#loadinheritance) | 34,740 | -0.62% | -69.56% | 10 | 0% | -90.65% | 1,488 | -4.12% | -48.19% |
| [Render/inheritance](#renderinheritance) | 214,663 | +0.19% | -71.30% | 76 | 0% | -87.78% | 3,280 | -1.44% | -45.48% |
| [Load/large_static](#loadlarge_static) | 236,792 | 0% | -73.27% | 28 | 0% | -93.64% | 43,120 | 0% | -19.23% |
| [Render/large_static](#renderlarge_static) | 27,520 | 0% | -63.37% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [Load/macros](#loadmacros) | 81,017 | -0.93% | -64.46% | 29 | -6.45% | -84.41% | 3,744 | -2.30% | -39.77% |
| [Render/macros](#rendermacros) | 1,019,700 | -0.20% | -60.22% | 407 | 0% | -85.09% | 13,592 | 0% | -7.66% |
| [Load/many_tags](#loadmany_tags) | 13,186,888 | -0.10% | -70.14% | 958 | 0% | -97.20% | 457,856 | 0% | -56.61% |
| [Render/many_tags](#rendermany_tags) | 1,262,516 | 0% | -39.39% | 2 | 0% | -90.48% | 5,632 | 0% | -10.77% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 77,757 | -0.02% | -67.27% | 26 | 0% | -85.87% | 3,568 | 0% | -41.16% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 6,657,472 | 0% | -46.12% | 1,025 | 0% | -74.71% | 345,864 | 0% | -0.33% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 97,142 | -0.01% |  | 61 | 0% |  | 5,752 | 0% | -30.40% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 7,028,360 | 0% |  | 1,046 | 0% |  | 1,379,848 | 0% | -0.08% |
| [Load/plain_text](#loadplain_text) | 4,014 | 0% | -58.52% | 5 | 0% | -72.22% | 688 | 0% | -38.57% |
| [Render/plain_text](#renderplain_text) | 986 | 0% | -85.49% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [Load/strings](#loadstrings) | 100,804 | +0.01% | -65.46% | 33 | 0% | -85.27% | 4,552 | 0% | -49.65% |
| [Render/strings](#renderstrings) | 1,061,556 | 0% | -49.39% | 975 | 0% | -53.88% | 17,248 | 0% | -3.79% |
| [Load/substitute](#loadsubstitute) | 10,694 | 0% | -64.18% | 8 | 0% | -78.38% | 832 | 0% | -45.26% |
| [Render/substitute](#rendersubstitute) | 2,672 | 0% | -69.69% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 1,247 | +1.38% | 1,405 | +0.50% | 0 |  | 8,405 | +3.26% |
| Render/chat_llama | 1,250 | +0.24% | 507 | +1.00% | 0 |  | 4,654 | -2.08% |
| Load/chat_mistral | 1,791 | -0.78% | 2,002 | -0.30% | 0 |  | 9,687 | +2.97% |
| Render/chat_mistral | 2,309 | +2.53% | 541 | +0.74% | 0 |  | 13,603 | -3.89% |
| Load/chat_qwen | 850 | 0% | 1,254 | +0.72% | 0 |  | 5,426 | +2.22% |
| Render/chat_qwen | 1,188 | +1.80% | 355 | -1.66% | 0 |  | 4,182 | -6.59% |
| Load/config_file | 378 | +1.89% | 541 | -2.17% | 0 |  | 3,415 | +1.07% |
| Render/config_file | 6,969 | +4.23% | 2,038 | +2.88% | 0 |  | 69,579 | +1.42% |
| Load/dict_ops | 89 | 0% | 86 | +1.18% | 0 |  | 1,528 | +1.80% |
| Render/dict_ops | 959 | -0.10% | 611 | +0.16% | 0 |  | 1,094 | -4.62% |
| Load/expressions | 134 | +1.52% | 213 | +0.95% | 0 |  | 1,472 | +1.03% |
| Render/expressions | 5 | +66.67% | 2 | +100.00% | 0 |  | 447 | +0.22% |
| Load/filters | 208 | -0.95% | 148 | +2.07% | 0 |  | 2,412 | +5.24% |
| Render/filters | 64 | +1.59% | 37 | -2.63% | 0 |  | 1,150 | -2.54% |
| Load/for_filter_if | 44 | -12.00% | 23 | +15.00% | 0 |  | 1,173 | +1.82% |
| Render/for_filter_if | 1,306 | +0.15% | 54 | +1.89% | 0 |  | 364 | -70.31% |
| Load/for_loop_vars | 55 | +10.00% | 66 | +6.45% | 0 |  | 1,278 | -1.99% |
| Render/for_loop_vars | 13 | -7.14% | 6 | 0% | 0 |  | 243 | -10.66% |
| Load/for_range | 29 | +11.54% | 23 | +15.00% | 0 |  | 926 | -3.04% |
| Render/for_range | 3 | +50.00% | 2 | +100.00% | 0 |  | 217 | -3.98% |
| Load/html_autoescape | 735 | -1.21% | 881 | -2.11% | 0 |  | 4,573 | +3.25% |
| Render/html_autoescape | 3,457 | -27.08% | 1,294 | -24.37% | 0 |  | 50,889 | -0.15% |
| Load/inheritance | 19 | +18.75% | 17 | 0% | 0 |  | 893 | -2.62% |
| Render/inheritance | 506 | -2.13% | 106 | -4.50% | 0 |  | 816 | -5.01% |
| Load/large_static | 2,616 | 0% | 1,200 | 0% | 0 |  | 444 | +4.23% |
| Render/large_static | 726 | +0.14% | 582 | 0% | 0 |  | 4 | -60.00% |
| Load/macros | 207 | +2.48% | 255 | -4.14% | 0 |  | 1,951 | +0.31% |
| Render/macros | 678 | 0% | 210 | 0% | 0 |  | 9,357 | -17.76% |
| Load/many_tags | 38,720 | -0.72% | 30,947 | -0.45% | 0 |  | 303,826 | +7.52% |
| Render/many_tags | 7,267 | 0% | 1,730 | +0.29% | 0 |  | 8,355 | +520.27% |
| Load/mitsuhiko_table | 91 | -3.19% | 186 | -0.53% | 0 |  | 1,160 | -0.77% |
| Render/mitsuhiko_table | 8,020 | -0.02% | 5,011 | 0% | 0 |  | 417 | -12.94% |
| Load/mitsuhiko_table_wide | 203 | -1.46% | 326 | -2.40% | 0 |  | 1,539 | -1.79% |
| Render/mitsuhiko_table_wide | 8,070 | -0.02% | 21,458 | 0% | 0 |  | 457 | -96.74% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 22 | +15.79% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 5 | +25.00% |
| Load/strings | 250 | -1.96% | 284 | +2.53% | 0 |  | 3,145 | +1.98% |
| Render/strings | 914 | -1.08% | 1,329 | 0% | 0 |  | 2,806 | -64.81% |
| Load/substitute | 8 | 0% | 10 | 0% | 0 |  | 349 | +2.05% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 3 | +50.00% |

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
