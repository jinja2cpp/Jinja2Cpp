# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (38 so far, latest [0990e3a](https://github.com/jinja2cpp/Jinja2Cpp/commit/0990e3ab13072f4582c2d5fb870ab9eb8beca228) on 2026-10-06). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 668,570 | 0% |  | 403 | 0% |  | 39,456 | 0% | +0.96% |
| [Render/chat_llama](#renderchat_llama) | 427,273 | -0.08% |  | 349 | 0% |  | 13,320 | 0% | -7.24% |
| [Load/chat_mistral](#loadchat_mistral) | 779,775 | 0% |  | 459 | 0% |  | 45,408 | 0% | +1.70% |
| [Render/chat_mistral](#renderchat_mistral) | 747,427 | -0.11% |  | 502 | 0% |  | 11,752 | 0% | -11.02% |
| [Load/chat_qwen](#loadchat_qwen) | 501,142 | 0% |  | 312 | 0% |  | 27,832 | 0% | +1.84% |
| [Render/chat_qwen](#renderchat_qwen) | 423,436 | -0.11% |  | 359 | 0% |  | 7,464 | 0% | -12.23% |
| [Load/config_file](#loadconfig_file) | 173,768 | 0% |  | 133 | 0% |  | 10,632 | 0% | +0.68% |
| [Render/config_file](#renderconfig_file) | 2,143,633 | -0.23% |  | 1,845 | 0% |  | 21,496 | 0% | -9.71% |
| [Load/dict_ops](#loaddict_ops) | 62,782 | 0% | -57.52% | 61 | 0% | -53.44% | 4,312 | 0% | +1.32% |
| [Render/dict_ops](#renderdict_ops) | 414,123 | +0.06% | -40.06% | 317 | 0% | -29.56% | 34,120 | 0% | -1.66% |
| [Load/expressions](#loadexpressions) | 84,485 | 0% | -59.02% | 82 | 0% | -43.84% | 6,304 | 0% | +2.74% |
| [Render/expressions](#renderexpressions) | 556,094 | +0.17% | -38.63% | 4 | 0% | -86.67% | 2,576 | 0% | -27.31% |
| [Load/filters](#loadfilters) | 128,718 | 0% | -53.10% | 114 | 0% | -50.00% | 11,488 | 0% | +0.63% |
| [Render/filters](#renderfilters) | 63,162 | -0.27% | -28.77% | 47 | 0% | -36.49% | 4,336 | 0% | -11.73% |
| [Load/for_filter_if](#loadfor_filter_if) | 52,703 | 0% | -61.07% | 56 | 0% | -54.47% | 3,504 | 0% | +1.62% |
| [Render/for_filter_if](#renderfor_filter_if) | 519,236 | +0.17% | -40.01% | 207 | 0% | -8.41% | 2,656 | 0% | -27.03% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 68,612 | 0% | -62.02% | 67 | 0% | -57.32% | 4,560 | 0% | +0.88% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 618,790 | +0.11% | -42.32% | 503 | 0% | -3.64% | 1,728 | 0% | -37.57% |
| [Load/for_range](#loadfor_range) | 28,133 | 0% | -50.37% | 37 | 0% | -33.93% | 2,040 | 0% | +2.00% |
| [Render/for_range](#renderfor_range) | 51,704 | +0.19% | -43.93% | 4 | 0% | -85.19% | 544 | 0% | -64.02% |
| [Load/html_autoescape](#loadhtml_autoescape) | 358,258 | 0% |  | 274 | 0% |  | 25,008 | 0% | +1.33% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,823,685 | -0.08% |  | 1,074 | 0% |  | 38,688 | 0% | -4.20% |
| [Load/inheritance](#loadinheritance) | 45,560 | 0% | -60.08% | 51 | 0% | -52.34% | 2,912 | 0% | +1.39% |
| [Render/inheritance](#renderinheritance) | 281,751 | -0.55% | -62.33% | 155 | 0% | -75.08% | 3,824 | 0% | -36.44% |
| [Load/large_static](#loadlarge_static) | 301,884 | 0% | -65.93% | 193 | 0% | -56.14% | 53,376 | 0% | -0.01% |
| [Render/large_static](#renderlarge_static) | 31,788 | -0.63% | -57.69% | 2 | 0% | -88.89% | 36,656 | 0% | -1.55% |
| [Load/macros](#loadmacros) | 97,571 | 0% | -57.20% | 95 | 0% | -48.92% | 6,272 | 0% | +0.90% |
| [Render/macros](#rendermacros) | 1,177,285 | -0.27% | -54.07% | 408 | 0% | -85.05% | 13,680 | 0% | -7.07% |
| [Load/many_tags](#loadmany_tags) | 16,460,925 | 0% | -62.73% | 9,659 | 0% | -71.80% | 1,060,064 | 0% | +0.46% |
| [Render/many_tags](#rendermany_tags) | 1,285,364 | -0.66% | -38.29% | 3 | 0% | -85.71% | 5,736 | 0% | -9.13% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 98,083 | 0% | -58.71% | 95 | 0% | -48.37% | 6,136 | 0% | +1.19% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 7,265,472 | +0.03% | -41.20% | 1,026 | 0% | -74.69% | 345,968 | 0% | -0.30% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 125,756 | 0% |  | 132 | 0% |  | 8,320 | 0% | +0.68% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 7,616,834 | +0.03% |  | 1,047 | 0% |  | 1,379,952 | 0% | -0.08% |
| [Load/plain_text](#loadplain_text) | 9,006 | 0% | -6.92% | 16 | 0% | -11.11% | 1,144 | 0% | +2.14% |
| [Render/plain_text](#renderplain_text) | 2,333 | 0% | -65.66% | 2 | 0% | -83.33% | 144 | 0% | -80.00% |
| [Load/strings](#loadstrings) | 123,918 | 0% | -57.54% | 106 | 0% | -52.68% | 9,368 | 0% | +3.63% |
| [Render/strings](#renderstrings) | 1,114,471 | +0.03% | -46.87% | 1,083 | 0% | -48.77% | 17,352 | 0% | -3.21% |
| [Load/substitute](#loadsubstitute) | 17,144 | 0% | -42.58% | 27 | 0% | -27.03% | 1,544 | 0% | +1.58% |
| [Render/substitute](#rendersubstitute) | 4,103 | -0.19% | -53.45% | 2 | 0% | -83.33% | 144 | 0% | -80.00% |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 2,024 | +0.45% | 1,431 | -0.07% | 0 |  | 9,986 | -4.44% |
| Render/chat_llama | 1,905 | +0.11% | 545 | -1.45% | 0 |  | 5,168 | -4.42% |
| Load/chat_mistral | 2,404 | +0.21% | 1,849 | +0.22% | 0 |  | 11,378 | -5.16% |
| Render/chat_mistral | 3,581 | +0.08% | 853 | 0% | 0 |  | 14,320 | -7.84% |
| Load/chat_qwen | 1,390 | -0.07% | 1,292 | -0.15% | 0 |  | 6,232 | -4.29% |
| Render/chat_qwen | 2,089 | +0.72% | 396 | -0.25% | 0 |  | 4,635 | +0.76% |
| Load/config_file | 664 | +0.61% | 577 | +0.35% | 0 |  | 3,968 | -4.96% |
| Render/config_file | 12,350 | -2.59% | 2,490 | -1.66% | 0 |  | 78,553 | +0.77% |
| Load/dict_ops | 188 | +1.08% | 128 | -3.03% | 0 |  | 1,720 | -2.99% |
| Render/dict_ops | 1,117 | 0% | 640 | -0.47% | 0 |  | 1,284 | -2.06% |
| Load/expressions | 254 | +0.40% | 219 | -0.45% | 0 |  | 1,553 | -3.42% |
| Render/expressions | 31 | 0% | 13 | 0% | 0 |  | 598 | +1.01% |
| Load/filters | 483 | +0.42% | 285 | 0% | 0 |  | 3,443 | -0.12% |
| Render/filters | 211 | +0.48% | 103 | -2.83% | 0 |  | 1,372 | +3.31% |
| Load/for_filter_if | 129 | 0% | 66 | 0% | 0 |  | 1,193 | -4.71% |
| Render/for_filter_if | 1,350 | +0.07% | 64 | -3.03% | 0 |  | 561 | -0.88% |
| Load/for_loop_vars | 148 | +0.68% | 125 | -0.79% | 0 |  | 1,355 | -7.95% |
| Render/for_loop_vars | 118 | +22.92% | 17 | 0% | 0 |  | 3,574 | +172.82% |
| Load/for_range | 43 | +7.50% | 45 | +2.27% | 0 |  | 893 | -2.93% |
| Render/for_range | 6 | 0% | 4 | 0% | 0 |  | 368 | +1.38% |
| Load/html_autoescape | 1,167 | +0.52% | 858 | +0.12% | 0 |  | 5,506 | -7.63% |
| Render/html_autoescape | 11,318 | +1.77% | 2,624 | +1.23% | 0 |  | 59,748 | -5.50% |
| Load/inheritance | 33 | +17.86% | 21 | 0% | 0 |  | 1,010 | -2.04% |
| Render/inheritance | 876 | +0.11% | 146 | +3.55% | 0 |  | 1,881 | -51.67% |
| Load/large_static | 2,847 | +0.14% | 1,259 | -0.16% | 0 |  | 558 | -37.58% |
| Render/large_static | 920 | 0% | 580 | 0% | 0 |  | 22 | +10.00% |
| Load/macros | 299 | -1.32% | 275 | +0.36% | 0 |  | 1,874 | -3.85% |
| Render/macros | 710 | -1.39% | 205 | -0.97% | 0 |  | 9,240 | +28.80% |
| Load/many_tags | 56,884 | +1.38% | 29,342 | -0.10% | 0 |  | 337,424 | -6.80% |
| Render/many_tags | 13,634 | +0.07% | 1,661 | -0.12% | 0 |  | 748 | -68.64% |
| Load/mitsuhiko_table | 220 | +1.38% | 233 | 0% | 0 |  | 1,204 | -6.16% |
| Render/mitsuhiko_table | 8,082 | +0.02% | 5,188 | 0% | 0 |  | 655 | -8.01% |
| Load/mitsuhiko_table_wide | 394 | +0.51% | 387 | +0.78% | 0 |  | 1,807 | -5.29% |
| Render/mitsuhiko_table_wide | 8,390 | +0.01% | 21,464 | -0.23% | 0 |  | 709 | -87.23% |
| Load/plain_text | 0 |  | 0 |  | 0 |  | 34 | +183.33% |
| Render/plain_text | 0 |  | 0 | -100.00% | 0 |  | 11 | +22.22% |
| Load/strings | 453 | +1.12% | 352 | +0.28% | 0 |  | 3,607 | -1.58% |
| Render/strings | 995 | +0.71% | 1,399 | +0.14% | 0 |  | 2,374 | -37.53% |
| Load/substitute | 7 | 0% | 7 | 0% | 0 |  | 453 | +5.35% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 7 | +40.00% |

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
