# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (40 so far, latest [260ab16](https://github.com/jinja2cpp/Jinja2Cpp/commit/260ab169ce3412a1d3175da4747e2281a3cdb1c9) on 2026-10-06). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 652,559 | -1.02% |  | 390 | +2.63% |  | 31,032 | -20.63% | -20.59% |
| [Render/chat_llama](#renderchat_llama) | 422,107 | -0.94% |  | 349 | 0% |  | 13,320 | 0% | -7.24% |
| [Load/chat_mistral](#loadchat_mistral) | 761,740 | -1.00% |  | 442 | +1.38% |  | 38,480 | -14.43% | -13.81% |
| [Render/chat_mistral](#renderchat_mistral) | 743,114 | -0.40% |  | 504 | +0.40% |  | 11,736 | +0.27% | -11.14% |
| [Load/chat_qwen](#loadchat_qwen) | 485,796 | -1.59% |  | 291 | +0.69% |  | 24,464 | -10.95% | -10.48% |
| [Render/chat_qwen](#renderchat_qwen) | 418,628 | -0.74% |  | 359 | 0% |  | 7,464 | 0% | -12.23% |
| [Load/config_file](#loadconfig_file) | 163,610 | -0.40% |  | 116 | +5.45% |  | 8,936 | -12.60% | -15.38% |
| [Render/config_file](#renderconfig_file) | 2,129,248 | -0.62% |  | 1,845 | 0% |  | 21,464 | 0% | -9.85% |
| [Load/dict_ops](#loaddict_ops) | 52,090 | -2.42% | -64.75% | 42 | +2.44% | -67.94% | 3,248 | -16.46% | -23.68% |
| [Render/dict_ops](#renderdict_ops) | 406,739 | -2.01% | -41.12% | 317 | 0% | -29.56% | 34,136 | 0% | -1.61% |
| [Load/expressions](#loadexpressions) | 73,971 | -2.70% | -64.12% | 62 | 0% | -57.53% | 5,256 | -10.37% | -14.34% |
| [Render/expressions](#renderexpressions) | 534,533 | -3.90% | -41.01% | 4 | 0% | -86.67% | 2,576 | 0% | -27.31% |
| [Load/filters](#loadfilters) | 117,237 | -0.95% | -57.28% | 106 | +12.77% | -53.51% | 6,160 | -44.16% | -46.04% |
| [Render/filters](#renderfilters) | 62,060 | -1.80% | -30.01% | 47 | 0% | -36.49% | 4,336 | 0% | -11.73% |
| [Load/for_filter_if](#loadfor_filter_if) | 42,557 | -2.62% | -68.57% | 36 | 0% | -70.73% | 2,888 | -6.23% | -16.24% |
| [Render/for_filter_if](#renderfor_filter_if) | 514,813 | -0.77% | -40.52% | 207 | 0% | -8.41% | 2,656 | 0% | -27.03% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 59,640 | -1.86% | -66.99% | 48 | 0% | -69.43% | 3,800 | -8.12% | -15.93% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 605,734 | -1.64% | -43.53% | 503 | 0% | -3.64% | 1,728 | 0% | -37.57% |
| [Load/for_range](#loadfor_range) | 20,691 | -1.21% | -63.50% | 20 | 0% | -64.29% | 1,520 | -5.94% | -24.00% |
| [Render/for_range](#renderfor_range) | 49,027 | -5.18% | -46.83% | 4 | 0% | -85.19% | 544 | 0% | -64.02% |
| [Load/html_autoescape](#loadhtml_autoescape) | 342,843 | -1.40% |  | 254 | +1.20% |  | 20,800 | -15.45% | -15.72% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,799,038 | -1.85% |  | 1,074 | 0% |  | 38,688 | 0% | -4.20% |
| [Load/inheritance](#loadinheritance) | 37,831 | +0.17% | -66.85% | 33 | +3.12% | -69.16% | 2,440 | -1.93% | -15.04% |
| [Render/inheritance](#renderinheritance) | 277,180 | -1.57% | -62.94% | 155 | 0% | -75.08% | 3,824 | 0% | -36.44% |
| [Load/large_static](#loadlarge_static) | 287,489 | -2.58% | -67.55% | 178 | +0.56% | -59.55% | 48,344 | -8.70% | -9.44% |
| [Render/large_static](#renderlarge_static) | 30,435 | -4.15% | -59.49% | 2 | 0% | -88.89% | 36,656 | 0% | -1.55% |
| [Load/macros](#loadmacros) | 86,449 | -2.09% | -62.08% | 72 | 0% | -61.29% | 5,336 | -8.76% | -14.16% |
| [Render/macros](#rendermacros) | 1,150,632 | -2.34% | -55.11% | 408 | 0% | -85.05% | 13,680 | 0% | -7.07% |
| [Load/many_tags](#loadmany_tags) | 16,132,095 | -2.00% | -63.47% | 10,238 | +6.24% | -70.11% | 783,864 | -26.03% | -25.72% |
| [Render/many_tags](#rendermany_tags) | 1,262,201 | -1.80% | -39.40% | 3 | 0% | -85.71% | 5,736 | 0% | -9.13% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 86,921 | -1.22% | -63.41% | 72 | 0% | -60.87% | 5,264 | -7.84% | -13.19% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 6,995,256 | -3.72% | -43.39% | 1,026 | 0% | -74.69% | 345,968 | 0% | -0.30% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 107,901 | -2.42% |  | 107 | 0% |  | 7,448 | -5.67% | -9.87% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 7,346,422 | -3.55% |  | 1,047 | 0% |  | 1,379,952 | 0% | -0.08% |
| [Load/plain_text](#loadplain_text) | 4,287 | +0.28% | -55.69% | 8 | 0% | -55.56% | 720 | 0% | -35.71% |
| [Render/plain_text](#renderplain_text) | 2,334 | 0% | -65.65% | 2 | 0% | -83.33% | 144 | 0% | -80.00% |
| [Load/strings](#loadstrings) | 113,147 | +0.16% | -61.23% | 90 | +7.14% | -59.82% | 6,552 | -26.74% | -27.52% |
| [Render/strings](#renderstrings) | 1,109,081 | -0.40% | -47.12% | 1,085 | +0.18% | -48.68% | 17,352 | 0% | -3.21% |
| [Load/substitute](#loadsubstitute) | 11,312 | -2.71% | -62.11% | 15 | 0% | -59.46% | 960 | -14.29% | -36.84% |
| [Render/substitute](#rendersubstitute) | 4,050 | -1.32% | -54.05% | 2 | 0% | -83.33% | 144 | 0% | -80.00% |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 1,820 | -7.24% | 1,260 | -8.36% | 0 |  | 9,385 | -8.76% |
| Render/chat_llama | 1,775 | -5.84% | 509 | -0.20% | 0 |  | 4,904 | -7.07% |
| Load/chat_mistral | 2,174 | -5.23% | 1,698 | -5.72% | 0 |  | 10,795 | -8.46% |
| Render/chat_mistral | 3,456 | -10.00% | 807 | -1.82% | 0 |  | 14,087 | +1.27% |
| Load/chat_qwen | 1,327 | -5.75% | 1,197 | -5.97% | 0 |  | 5,768 | -12.13% |
| Render/chat_qwen | 1,906 | -6.48% | 354 | -7.09% | 0 |  | 4,380 | -4.99% |
| Load/config_file | 616 | -4.05% | 529 | -7.03% | 0 |  | 3,548 | -7.05% |
| Render/config_file | 12,569 | -3.61% | 2,537 | -2.12% | 0 |  | 77,129 | -0.05% |
| Load/dict_ops | 138 | -9.21% | 85 | -21.30% | 0 |  | 1,540 | -4.35% |
| Render/dict_ops | 1,075 | -2.54% | 609 | -3.33% | 0 |  | 1,290 | +0.47% |
| Load/expressions | 205 | -6.82% | 194 | +6.59% | 0 |  | 1,429 | -0.76% |
| Render/expressions | 34 | -10.53% | 13 | -23.53% | 0 |  | 575 | -0.35% |
| Load/filters | 316 | -26.34% | 145 | -41.53% | 0 |  | 2,406 | -23.16% |
| Render/filters | 200 | -8.26% | 102 | +6.25% | 0 |  | 1,239 | -9.83% |
| Load/for_filter_if | 89 | -4.30% | 44 | -18.52% | 0 |  | 987 | -7.15% |
| Render/for_filter_if | 1,336 | +0.45% | 62 | +6.90% | 0 |  | 1,906 | +47.75% |
| Load/for_loop_vars | 114 | -3.39% | 80 | -6.98% | 0 |  | 1,164 | -7.69% |
| Render/for_loop_vars | 83 | +69.39% | 10 | +11.11% | 0 |  | 3,481 | +66.32% |
| Load/for_range | 35 | +6.06% | 29 | -3.33% | 0 |  | 823 | -0.36% |
| Render/for_range | 11 | +266.67% | 4 | +100.00% | 0 |  | 380 | +7.65% |
| Load/html_autoescape | 1,115 | +1.73% | 800 | -2.08% | 0 |  | 4,927 | -13.01% |
| Render/html_autoescape | 13,502 | -17.50% | 2,385 | -8.87% | 0 |  | 60,068 | -4.71% |
| Load/inheritance | 24 | -36.84% | 22 | -35.29% | 0 |  | 888 | +3.14% |
| Render/inheritance | 606 | -3.66% | 125 | -3.10% | 0 |  | 1,018 | -54.08% |
| Load/large_static | 2,857 | +0.18% | 1,207 | -1.87% | 0 |  | 371 | -24.13% |
| Render/large_static | 879 | -3.93% | 582 | +0.34% | 0 |  | 21 | -60.38% |
| Load/macros | 277 | -4.15% | 239 | -9.13% | 0 |  | 1,777 | -3.48% |
| Render/macros | 681 | -3.54% | 201 | -2.90% | 0 |  | 7,397 | -10.74% |
| Load/many_tags | 50,943 | -12.27% | 25,090 | -15.68% | 0 |  | 308,949 | -9.11% |
| Render/many_tags | 11,854 | -12.48% | 1,516 | -9.17% | 0 |  | 4,038 | -10.33% |
| Load/mitsuhiko_table | 168 | -7.18% | 204 | -0.49% | 0 |  | 1,194 | +3.92% |
| Render/mitsuhiko_table | 8,095 | -0.01% | 5,190 | +0.02% | 0 |  | 661 | -2.79% |
| Load/mitsuhiko_table_wide | 317 | -8.65% | 344 | -9.23% | 0 |  | 1,730 | +8.26% |
| Render/mitsuhiko_table_wide | 8,148 | -0.83% | 21,462 | -0.05% | 0 |  | 711 | -94.51% |
| Load/plain_text | 3 | 0% | 4 | 0% | 0 |  | 11 | 0% |
| Render/plain_text | 1 | 0% | 0 |  | 0 |  | 7 | -56.25% |
| Load/strings | 420 | +0.96% | 271 | -7.19% | 0 |  | 3,044 | -11.92% |
| Render/strings | 1,001 | +3.41% | 1,375 | -1.01% | 0 |  | 2,960 | -9.29% |
| Load/substitute | 6 | 0% | 10 | 0% | 0 |  | 278 | +8.59% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 6 | -57.14% |

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
