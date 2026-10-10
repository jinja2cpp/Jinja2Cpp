# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (58 so far, latest [1734c9b](https://github.com/jinja2cpp/Jinja2Cpp/commit/1734c9b4f2e54ccbd678684ab24c875df06c9224) on 2026-10-10). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 554,173 | 0% |  | 92 | 0% |  | 20,128 | 0% | -48.50% |
| [Render/chat_llama](#renderchat_llama) | 410,946 | 0% |  | 348 | 0% |  | 13,216 | 0% | -7.97% |
| [Load/chat_mistral](#loadchat_mistral) | 653,764 | 0% |  | 74 | 0% |  | 24,200 | 0% | -45.80% |
| [Render/chat_mistral](#renderchat_mistral) | 708,694 | 0% |  | 474 | 0% |  | 11,664 | 0% | -11.69% |
| [Load/chat_qwen](#loadchat_qwen) | 423,529 | 0% |  | 68 | 0% |  | 16,000 | 0% | -41.45% |
| [Render/chat_qwen](#renderchat_qwen) | 397,330 | 0% |  | 337 | 0% |  | 7,360 | 0% | -13.45% |
| [Load/config_file](#loadconfig_file) | 145,763 | 0% |  | 34 | 0% |  | 5,720 | 0% | -45.83% |
| [Render/config_file](#renderconfig_file) | 1,916,381 | 0% |  | 1,703 | 0% |  | 21,528 | 0% | -9.58% |
| [Load/dict_ops](#loaddict_ops) | 47,355 | 0% | -67.96% | 12 | 0% | -90.84% | 2,240 | 0% | -47.37% |
| [Render/dict_ops](#renderdict_ops) | 400,135 | 0% | -42.08% | 315 | 0% | -30.00% | 34,016 | 0% | -1.96% |
| [Load/expressions](#loadexpressions) | 66,930 | 0% | -67.53% | 18 | 0% | -87.67% | 3,592 | 0% | -41.46% |
| [Render/expressions](#renderexpressions) | 518,971 | 0% | -42.73% | 3 | 0% | -90.00% | 2,472 | 0% | -30.25% |
| [Load/filters](#loadfilters) | 96,795 | 0% | -64.73% | 32 | 0% | -85.96% | 3,960 | 0% | -65.31% |
| [Render/filters](#renderfilters) | 60,297 | 0% | -32.00% | 46 | 0% | -37.84% | 4,232 | 0% | -13.84% |
| [Load/for_filter_if](#loadfor_filter_if) | 40,925 | 0% | -69.77% | 9 | 0% | -92.68% | 1,952 | 0% | -43.39% |
| [Render/for_filter_if](#renderfor_filter_if) | 447,512 | 0% | -48.29% | 206 | 0% | -8.85% | 2,568 | 0% | -29.45% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 56,956 | 0% | -68.47% | 13 | 0% | -91.72% | 2,448 | 0% | -45.84% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 275,055 | 0% | -74.36% | 2 | 0% | -99.62% | 1,552 | 0% | -43.93% |
| [Load/for_range](#loadfor_range) | 19,493 | 0% | -65.61% | 10 | 0% | -82.14% | 1,136 | 0% | -43.20% |
| [Render/for_range](#renderfor_range) | 46,776 | 0% | -49.27% | 3 | 0% | -88.89% | 440 | 0% | -70.90% |
| [Load/html_autoescape](#loadhtml_autoescape) | 287,383 | 0% |  | 63 | 0% |  | 13,416 | 0% | -45.64% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,613,474 | 0% |  | 1,011 | 0% |  | 38,752 | 0% | -4.04% |
| [Load/inheritance](#loadinheritance) | 34,956 | 0% | -69.37% | 10 | 0% | -90.65% | 1,552 | 0% | -45.96% |
| [Render/inheritance](#renderinheritance) | 214,360 | 0% | -71.34% | 76 | 0% | -87.78% | 3,328 | 0% | -44.68% |
| [Load/large_static](#loadlarge_static) | 236,792 | 0% | -73.27% | 28 | 0% | -93.64% | 43,120 | 0% | -19.23% |
| [Render/large_static](#renderlarge_static) | 27,520 | 0% | -63.37% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [Load/macros](#loadmacros) | 81,769 | 0% | -64.13% | 31 | 0% | -83.33% | 3,832 | 0% | -38.35% |
| [Render/macros](#rendermacros) | 1,021,772 | 0% | -60.14% | 407 | 0% | -85.09% | 13,592 | 0% | -7.66% |
| [Load/many_tags](#loadmany_tags) | 13,199,788 | 0% | -70.11% | 958 | 0% | -97.20% | 457,856 | 0% | -56.61% |
| [Render/many_tags](#rendermany_tags) | 1,262,516 | 0% | -39.39% | 2 | 0% | -90.48% | 5,632 | 0% | -10.77% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 77,769 | 0% | -67.26% | 26 | 0% | -85.87% | 3,568 | 0% | -41.16% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 6,657,472 | 0% | -46.12% | 1,025 | 0% | -74.71% | 345,864 | 0% | -0.33% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 97,154 | 0% |  | 61 | 0% |  | 5,752 | 0% | -30.40% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 7,028,360 | 0% |  | 1,046 | 0% |  | 1,379,848 | 0% | -0.08% |
| [Load/plain_text](#loadplain_text) | 4,014 | 0% | -58.52% | 5 | 0% | -72.22% | 688 | 0% | -38.57% |
| [Render/plain_text](#renderplain_text) | 986 | 0% | -85.49% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [Load/strings](#loadstrings) | 100,814 | 0% | -65.46% | 33 | 0% | -85.27% | 4,552 | 0% | -49.65% |
| [Render/strings](#renderstrings) | 1,061,558 | 0% | -49.39% | 975 | 0% | -53.88% | 17,248 | 0% | -3.79% |
| [Load/substitute](#loadsubstitute) | 10,694 | 0% | -64.18% | 8 | 0% | -78.38% | 832 | 0% | -45.26% |
| [Render/substitute](#rendersubstitute) | 2,672 | 0% | -69.69% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 1,229 | 0% | 1,399 | 0% | 0 |  | 8,265 | 0% |
| Render/chat_llama | 1,247 | 0% | 502 | 0% | 0 |  | 4,631 | 0% |
| Load/chat_mistral | 1,807 | 0% | 2,010 | 0% | 0 |  | 9,521 | 0% |
| Render/chat_mistral | 2,400 | 0% | 726 | 0% | 0 |  | 13,666 | 0% |
| Load/chat_qwen | 850 | 0% | 1,245 | 0% | 0 |  | 5,426 | 0% |
| Render/chat_qwen | 1,167 | 0% | 361 | 0% | 0 |  | 4,222 | 0% |
| Load/config_file | 371 | 0% | 554 | 0% | 0 |  | 3,423 | 0% |
| Render/config_file | 6,684 | 0% | 1,983 | 0% | 0 |  | 71,045 | 0% |
| Load/dict_ops | 85 | 0% | 83 | 0% | 0 |  | 1,493 | 0% |
| Render/dict_ops | 960 | 0% | 610 | 0% | 0 |  | 1,128 | 0% |
| Load/expressions | 132 | 0% | 211 | 0% | 0 |  | 1,427 | 0% |
| Render/expressions | 3 | 0% | 1 | 0% | 0 |  | 469 | 0% |
| Load/filters | 210 | 0% | 145 | 0% | 0 |  | 2,332 | 0% |
| Render/filters | 96 | 0% | 72 | 0% | 0 |  | 1,190 | 0% |
| Load/for_filter_if | 45 | 0% | 20 | 0% | 0 |  | 1,209 | 0% |
| Render/for_filter_if | 1,304 | 0% | 53 | 0% | 0 |  | 384 | 0% |
| Load/for_loop_vars | 50 | 0% | 62 | 0% | 0 |  | 1,321 | 0% |
| Render/for_loop_vars | 14 | 0% | 6 | 0% | 0 |  | 278 | 0% |
| Load/for_range | 26 | 0% | 20 | 0% | 0 |  | 940 | 0% |
| Render/for_range | 2 | 0% | 1 | 0% | 0 |  | 215 | 0% |
| Load/html_autoescape | 744 | 0% | 900 | 0% | 0 |  | 4,598 | 0% |
| Render/html_autoescape | 4,741 | 0% | 1,711 | 0% | 0 |  | 51,651 | 0% |
| Load/inheritance | 16 | 0% | 17 | 0% | 0 |  | 920 | 0% |
| Render/inheritance | 517 | 0% | 111 | 0% | 0 |  | 1,040 | 0% |
| Load/large_static | 2,616 | 0% | 1,200 | 0% | 0 |  | 430 | 0% |
| Render/large_static | 725 | 0% | 582 | 0% | 0 |  | 12 | 0% |
| Load/macros | 206 | 0% | 269 | 0% | 0 |  | 1,962 | 0% |
| Render/macros | 678 | 0% | 210 | 0% | 0 |  | 9,868 | 0% |
| Load/many_tags | 38,999 | 0% | 31,087 | 0% | 0 |  | 294,899 | 0% |
| Render/many_tags | 7,267 | 0% | 1,725 | 0% | 0 |  | 4,128 | 0% |
| Load/mitsuhiko_table | 94 | 0% | 187 | 0% | 0 |  | 1,111 | 0% |
| Render/mitsuhiko_table | 8,022 | 0% | 5,011 | 0% | 0 |  | 445 | 0% |
| Load/mitsuhiko_table_wide | 206 | 0% | 334 | 0% | 0 |  | 1,562 | 0% |
| Render/mitsuhiko_table_wide | 8,072 | 0% | 21,459 | 0% | 0 |  | 497 | 0% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 34 | 0% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 4 | 0% |
| Load/strings | 257 | 0% | 277 | 0% | 0 |  | 3,134 | 0% |
| Render/strings | 923 | 0% | 1,329 | 0% | 0 |  | 3,411 | 0% |
| Load/substitute | 8 | 0% | 10 | 0% | 0 |  | 340 | 0% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 12 | 0% |

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
