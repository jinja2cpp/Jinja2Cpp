# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (151 so far, latest [8d00811](https://github.com/jinja2cpp/Jinja2Cpp/commit/8d008112d0913de31fcc5cdaee4d2fb5358ce87e) on 2026-10-28). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 344,972 | -0.04% |  | 55 | 0% |  | 18,256 | 0% | -53.29% |
| [Render/chat_llama](#renderchat_llama) | 307,511 | 0% |  | 251 | 0% |  | 11,136 | 0% | -22.45% |
| [Load/chat_mistral](#loadchat_mistral) | 394,034 | -0.02% |  | 52 | 0% |  | 20,728 | 0% | -53.57% |
| [Render/chat_mistral](#renderchat_mistral) | 510,243 | 0% |  | 424 | 0% |  | 9,520 | 0% | -27.92% |
| [Load/chat_qwen](#loadchat_qwen) | 267,084 | -0.03% |  | 44 | 0% |  | 14,400 | 0% | -47.31% |
| [Render/chat_qwen](#renderchat_qwen) | 294,955 | 0% |  | 238 | 0% |  | 7,824 | 0% | -8.00% |
| [Load/config_file](#loadconfig_file) | 99,262 | -0.03% |  | 25 | 0% |  | 5,448 | 0% | -48.41% |
| [Render/config_file](#renderconfig_file) | 1,258,106 | 0% |  | 908 | 0% |  | 23,416 | 0% | -1.65% |
| [Load/dict_ops](#loaddict_ops) | 33,186 | -0.03% | -77.54% | 9 | 0% | -93.13% | 2,192 | 0% | -48.50% |
| [Render/dict_ops](#renderdict_ops) | 296,099 | +0.01% | -57.14% | 112 | 0% | -75.11% | 24,424 | 0% | -29.61% |
| [Load/expressions](#loadexpressions) | 45,367 | -0.01% | -77.99% | 16 | 0% | -89.04% | 2,744 | 0% | -55.28% |
| [Render/expressions](#renderexpressions) | 424,637 | 0% | -53.14% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 71,823 | +0.01% | -73.83% | 27 | 0% | -88.16% | 3,736 | 0% | -67.27% |
| [Render/filters](#renderfilters) | 30,274 | -0.01% | -65.86% | 26 | 0% | -64.86% | 1,080 | 0% | -78.01% |
| [Load/for_filter_if](#loadfor_filter_if) | 30,075 | -0.03% | -77.79% | 7 | 0% | -94.31% | 1,920 | 0% | -44.32% |
| [Render/for_filter_if](#renderfor_filter_if) | 273,512 | 0% | -68.40% | 4 | 0% | -98.23% | 13,328 | 0% | +266.15% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 40,520 | -0.01% | -77.57% | 10 | 0% | -93.63% | 2,400 | 0% | -46.90% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 192,341 | 0% | -82.07% | 2 | 0% | -99.62% | 1,552 | 0% | -43.93% |
| [Load/for_range](#loadfor_range) | 13,573 | -0.04% | -76.05% | 7 | 0% | -87.50% | 1,104 | 0% | -44.80% |
| [Render/for_range](#renderfor_range) | 38,774 | +0.04% | -57.95% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/html_autoescape](#loadhtml_autoescape) | 195,569 | 0% |  | 43 | 0% |  | 12,608 | 0% | -48.91% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,216,422 | 0% |  | 674 | 0% |  | 43,744 | 0% | +8.32% |
| [Load/inheritance](#loadinheritance) | 22,748 | -0.11% | -80.07% | 7 | 0% | -93.46% | 1,344 | 0% | -53.20% |
| [Render/inheritance](#renderinheritance) | 136,635 | 0% | -81.73% | 35 | 0% | -94.37% | 6,488 | 0% | +7.85% |
| [Load/int_arith](#loadint_arith) | 66,081 | -0.02% |  | 20 | 0% |  | 3,856 | 0% | 0% |
| [Render/int_arith](#renderint_arith) | 825,984 | 0% |  | 12 | 0% |  | 3,288 | 0% | 0% |
| [Load/large_static](#loadlarge_static) | 155,237 | 0% | -82.48% | 14 | 0% | -96.82% | 43,136 | 0% | -19.20% |
| [Render/large_static](#renderlarge_static) | 20,884 | 0% | -72.20% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [RenderSink/large_static](#rendersinklarge_static) | 12,686 | -0.02% |  | 0 |  |  | 0 |  |  |
| [RenderTo/large_static](#rendertolarge_static) | 20,263 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/macros](#loadmacros) | 55,661 | -0.02% | -75.58% | 27 | 0% | -85.48% | 4,376 | 0% | -29.60% |
| [Render/macros](#rendermacros) | 649,936 | 0% | -74.64% | 308 | 0% | -88.71% | 18,864 | 0% | +28.15% |
| [Load/many_tags](#loadmany_tags) | 9,087,216 | -0.04% | -79.42% | 951 | 0% | -97.22% | 431,152 | 0% | -59.14% |
| [Render/many_tags](#rendermany_tags) | 721,445 | 0% | -65.37% | 2 | 0% | -90.48% | 5,632 | 0% | -10.77% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 46,344 | -0.03% | -80.49% | 19 | 0% | -89.67% | 3,376 | 0% | -44.33% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,187,191 | 0% | -58.02% | 33 | 0% | -99.19% | 373,296 | 0% | +7.58% |
| [RenderSink/mitsuhiko_table](#rendersinkmitsuhiko_table) | 5,114,464 | 0% |  | 32 | 0% |  | 28,824 | 0% | 0% |
| [RenderTo/mitsuhiko_table](#rendertomitsuhiko_table) | 5,186,741 | 0% |  | 32 | 0% |  | 28,824 | 0% | 0% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 55,003 | +0.01% |  | 35 | 0% |  | 5,704 | 0% | -30.98% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,533,180 | 0% |  | 33 | 0% |  | 1,406,688 | 0% | +1.86% |
| [RenderSink/mitsuhiko_table_wide](#rendersinkmitsuhiko_table_wide) | 5,356,491 | 0% |  | 32 | 0% |  | 28,824 | 0% | 0% |
| [RenderTo/mitsuhiko_table_wide](#rendertomitsuhiko_table_wide) | 5,532,749 | 0% |  | 32 | 0% |  | 28,824 | 0% | 0% |
| [Load/plain_text](#loadplain_text) | 3,692 | 0% | -61.84% | 4 | 0% | -77.78% | 688 | 0% | -38.57% |
| [Render/plain_text](#renderplain_text) | 1,017 | 0% | -85.03% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/plain_text](#rendersinkplain_text) | 717 | -0.42% |  | 0 |  |  | 0 |  |  |
| [RenderTo/plain_text](#rendertoplain_text) | 818 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/strings](#loadstrings) | 72,303 | 0% | -75.23% | 30 | 0% | -86.61% | 4,264 | 0% | -52.83% |
| [Render/strings](#renderstrings) | 577,967 | 0% | -72.45% | 344 | 0% | -83.73% | 9,104 | 0% | -49.22% |
| [Load/substitute](#loadsubstitute) | 7,715 | 0% | -74.16% | 6 | 0% | -83.78% | 848 | 0% | -44.21% |
| [Render/substitute](#rendersubstitute) | 2,233 | 0% | -74.66% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/substitute](#rendersinksubstitute) | 1,936 | -0.15% |  | 0 |  |  | 0 |  |  |
| [RenderTo/substitute](#rendertosubstitute) | 2,038 | 0% |  | 0 |  |  | 0 |  |  |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 739 | +0.27% | 732 | -0.27% | 0 |  | 7,503 | +3.63% |
| Render/chat_llama | 1,136 | -0.26% | 488 | -0.20% | 0 |  | 3,548 | -2.31% |
| Load/chat_mistral | 1,130 | +3.29% | 1,022 | +0.49% | 0 |  | 8,410 | +3.84% |
| Render/chat_mistral | 1,809 | -1.09% | 545 | -1.45% | 0 |  | 9,527 | +2.85% |
| Load/chat_qwen | 483 | +2.99% | 593 | -0.17% | 0 |  | 4,429 | +5.91% |
| Render/chat_qwen | 1,065 | -1.57% | 401 | -0.74% | 0 |  | 3,155 | -1.96% |
| Load/config_file | 187 | +8.09% | 164 | -4.65% | 0 |  | 3,091 | +2.05% |
| Render/config_file | 5,974 | +2.98% | 1,339 | -3.46% | 0 |  | 38,011 | -4.02% |
| Load/dict_ops | 46 | +15.00% | 24 | +4.35% | 0 |  | 1,357 | +3.59% |
| Render/dict_ops | 740 | -0.40% | 415 | -0.72% | 0 |  | 833 | -1.42% |
| Load/expressions | 24 | +71.43% | 13 | +85.71% | 0 |  | 1,361 | -0.07% |
| Render/expressions | 2 | 0% | 1 | 0% | 0 |  | 297 | -0.34% |
| Load/filters | 148 | +10.45% | 79 | +3.95% | 0 |  | 2,053 | -4.69% |
| Render/filters | 23 | +76.92% | 5 | +66.67% | 0 |  | 723 | +1.83% |
| Load/for_filter_if | 10 | 0% | 4 | +33.33% | 0 |  | 1,079 | +6.20% |
| Render/for_filter_if | 1,658 | -0.12% | 59 | +1.72% | 0 |  | 174 | -4.40% |
| Load/for_loop_vars | 20 | +25.00% | 13 | +30.00% | 0 |  | 1,128 | +4.44% |
| Render/for_loop_vars | 15 | +87.50% | 2 | +100.00% | 0 |  | 134 | +28.85% |
| Load/for_range | 2 | -33.33% | 3 | 0% | 0 |  | 767 | +4.21% |
| Render/for_range | 1 | 0% | 1 | 0% | 0 |  | 111 | +73.44% |
| Load/html_autoescape | 466 | +1.30% | 528 | +0.76% | 0 |  | 4,281 | +9.32% |
| Render/html_autoescape | 4,019 | +2.45% | 1,199 | +3.45% | 0 |  | 33,541 | -4.21% |
| Load/inheritance | 2 | -66.67% | 2 | -50.00% | 0 |  | 685 | +7.70% |
| Render/inheritance | 570 | -0.70% | 96 | +1.05% | 0 |  | 845 | -1.97% |
| Load/int_arith | 74 | +7.25% | 48 | +4.35% | 0 |  | 2,105 | -1.41% |
| Render/int_arith | 9 | -18.18% | 3 | -40.00% | 0 |  | 718 | +0.56% |
| Load/large_static | 1,942 | +0.10% | 909 | 0% | 0 |  | 263 | +6.05% |
| Render/large_static | 706 | 0% | 569 | 0% | 0 |  | 4 | -20.00% |
| RenderSink/large_static | 1 | 0% | 1 |  | 0 |  | 3 | 0% |
| RenderTo/large_static | 686 | 0% | 569 | 0% | 0 |  | 3 | -40.00% |
| Load/macros | 59 | -10.61% | 44 | -4.35% | 0 |  | 1,696 | +0.89% |
| Render/macros | 736 | +0.14% | 218 | 0% | 0 |  | 556 | -40.72% |
| Load/many_tags | 29,161 | +2.02% | 22,674 | +0.72% | 0 |  | 259,391 | +3.16% |
| Render/many_tags | 6,136 | -0.03% | 860 | -0.58% | 0 |  | 158 | +13.67% |
| Load/mitsuhiko_table | 19 | -13.64% | 17 | -10.53% | 0 |  | 855 | +3.26% |
| Render/mitsuhiko_table | 9,083 | +0.19% | 6,623 | +0.18% | 0 |  | 285 | +5.95% |
| RenderSink/mitsuhiko_table | 9,032 | +0.11% | 1,486 | -0.87% | 0 |  | 231 | +14.93% |
| RenderTo/mitsuhiko_table | 9,084 | +0.32% | 6,612 | -0.11% | 0 |  | 263 | +5.20% |
| Load/mitsuhiko_table_wide | 75 | +7.14% | 63 | +8.62% | 0 |  | 950 | -11.55% |
| Render/mitsuhiko_table_wide | 9,263 | 0% | 23,082 | +0.04% | 0 |  | 301 | +12.73% |
| RenderSink/mitsuhiko_table_wide | 9,153 | +0.38% | 1,601 | +1.52% | 0 |  | 219 | +4.29% |
| RenderTo/mitsuhiko_table_wide | 9,251 | +0.11% | 23,012 | +0.01% | 0 |  | 263 | +14.85% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 31 | +121.43% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 5 | -16.67% |
| RenderSink/plain_text | 0 |  | 0 |  | 0 |  | 4 | -20.00% |
| RenderTo/plain_text | 0 |  | 1 | 0% | 0 |  | 5 | -16.67% |
| Load/strings | 181 | +7.10% | 122 | +5.17% | 0 |  | 2,868 | +0.35% |
| Render/strings | 426 | -3.84% | 458 | -0.87% | 0 |  | 1,471 | +0.96% |
| Load/substitute | 2 | 0% | 3 | 0% | 0 |  | 174 | +32.82% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 4 | -20.00% |
| RenderSink/substitute | 0 | -100.00% | 0 |  | 0 |  | 3 | 0% |
| RenderTo/substitute | 0 |  | 0 |  | 0 |  | 3 | -25.00% |

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

### Load/int_arith

![Load/int_arith](charts/Load_int_arith.svg)

### Render/int_arith

![Render/int_arith](charts/Render_int_arith.svg)

### Load/large_static

![Load/large_static](charts/Load_large_static.svg)

### Render/large_static

![Render/large_static](charts/Render_large_static.svg)

### RenderSink/large_static

![RenderSink/large_static](charts/RenderSink_large_static.svg)

### RenderTo/large_static

![RenderTo/large_static](charts/RenderTo_large_static.svg)

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

### RenderSink/mitsuhiko_table

![RenderSink/mitsuhiko_table](charts/RenderSink_mitsuhiko_table.svg)

### RenderTo/mitsuhiko_table

![RenderTo/mitsuhiko_table](charts/RenderTo_mitsuhiko_table.svg)

### Load/mitsuhiko_table_wide

![Load/mitsuhiko_table_wide](charts/Load_mitsuhiko_table_wide.svg)

### Render/mitsuhiko_table_wide

![Render/mitsuhiko_table_wide](charts/Render_mitsuhiko_table_wide.svg)

### RenderSink/mitsuhiko_table_wide

![RenderSink/mitsuhiko_table_wide](charts/RenderSink_mitsuhiko_table_wide.svg)

### RenderTo/mitsuhiko_table_wide

![RenderTo/mitsuhiko_table_wide](charts/RenderTo_mitsuhiko_table_wide.svg)

### Load/plain_text

![Load/plain_text](charts/Load_plain_text.svg)

### Render/plain_text

![Render/plain_text](charts/Render_plain_text.svg)

### RenderSink/plain_text

![RenderSink/plain_text](charts/RenderSink_plain_text.svg)

### RenderTo/plain_text

![RenderTo/plain_text](charts/RenderTo_plain_text.svg)

### Load/strings

![Load/strings](charts/Load_strings.svg)

### Render/strings

![Render/strings](charts/Render_strings.svg)

### Load/substitute

![Load/substitute](charts/Load_substitute.svg)

### Render/substitute

![Render/substitute](charts/Render_substitute.svg)

### RenderSink/substitute

![RenderSink/substitute](charts/RenderSink_substitute.svg)

### RenderTo/substitute

![RenderTo/substitute](charts/RenderTo_substitute.svg)
