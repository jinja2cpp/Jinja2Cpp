# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (175 so far, latest [0d55e47](https://github.com/jinja2cpp/Jinja2Cpp/commit/0d55e477a365f5bdb1928f3720af553a064b8afc) on 2026-11-05). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 343,306 | +0.04% |  | 55 | 0% |  | 18,464 | 0% | -52.75% |
| [Render/chat_llama](#renderchat_llama) | 295,377 | +0.01% |  | 225 | 0% |  | 11,616 | 0% | -19.11% |
| [Load/chat_mistral](#loadchat_mistral) | 392,445 | +0.02% |  | 52 | 0% |  | 20,888 | 0% | -53.22% |
| [Render/chat_mistral](#renderchat_mistral) | 499,048 | +0.11% |  | 402 | 0% |  | 10,016 | 0% | -24.17% |
| [Load/chat_qwen](#loadchat_qwen) | 265,208 | +0.09% |  | 44 | 0% |  | 14,416 | 0% | -47.25% |
| [Render/chat_qwen](#renderchat_qwen) | 277,075 | +0.01% |  | 212 | 0% |  | 8,304 | 0% | -2.35% |
| [Load/config_file](#loadconfig_file) | 96,675 | +0.09% |  | 25 | 0% |  | 5,464 | 0% | -48.26% |
| [Render/config_file](#renderconfig_file) | 1,266,366 | +0.05% |  | 908 | 0% |  | 24,440 | 0% | +2.65% |
| [Load/dict_ops](#loaddict_ops) | 33,172 | +0.02% | -77.55% | 9 | 0% | -93.13% | 2,176 | 0% | -48.87% |
| [Render/dict_ops](#renderdict_ops) | 293,427 | +0.01% | -57.53% | 112 | 0% | -75.11% | 24,408 | 0% | -29.65% |
| [Load/expressions](#loadexpressions) | 44,894 | 0% | -78.22% | 16 | 0% | -89.04% | 2,728 | 0% | -55.54% |
| [Render/expressions](#renderexpressions) | 427,194 | +0.01% | -52.86% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 68,911 | 0% | -74.89% | 27 | 0% | -88.16% | 3,848 | 0% | -66.29% |
| [Render/filters](#renderfilters) | 30,368 | +0.41% | -65.75% | 26 | 0% | -64.86% | 1,080 | 0% | -78.01% |
| [Load/for_filter_if](#loadfor_filter_if) | 29,877 | +0.08% | -77.93% | 7 | 0% | -94.31% | 1,904 | 0% | -44.78% |
| [Render/for_filter_if](#renderfor_filter_if) | 275,089 | 0% | -68.22% | 4 | 0% | -98.23% | 16,560 | 0% | +354.95% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 40,112 | +0.07% | -77.80% | 10 | 0% | -93.63% | 2,368 | 0% | -47.61% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 192,299 | 0% | -82.07% | 2 | 0% | -99.62% | 1,568 | 0% | -43.35% |
| [Load/for_range](#loadfor_range) | 13,571 | 0% | -76.06% | 7 | 0% | -87.50% | 1,088 | 0% | -45.60% |
| [Render/for_range](#renderfor_range) | 38,846 | +0.06% | -57.87% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/hf_deepseek_v31](#loadhf_deepseek_v31) | 385,572 | -0.11% |  | 55 | 0% |  | 21,976 | 0% | +0.73% |
| [Render/hf_deepseek_v31](#renderhf_deepseek_v31) | 90,371 | +0.06% |  | 43 | 0% |  | 2,480 | 0% | 0% |
| [Load/hf_gpt_oss](#loadhf_gpt_oss) | 1,534,405 | +0.14% |  | 285 | 0% |  | 74,416 | +0.04% | +0.26% |
| [Render/hf_gpt_oss](#renderhf_gpt_oss) | 170,482 | +0.12% |  | 155 | 0% |  | 5,064 | 0% | +2.59% |
| [Load/hf_mistral_small](#loadhf_mistral_small) | 595,654 | +0.03% |  | 88 | 0% |  | 33,048 | 0% | +0.34% |
| [Render/hf_mistral_small](#renderhf_mistral_small) | 144,687 | +0.07% |  | 105 | 0% |  | 6,120 | 0% | -0.26% |
| [Load/hf_qwen3](#loadhf_qwen3) | 483,905 | +0.11% |  | 82 | 0% |  | 25,560 | 0% | +0.31% |
| [Render/hf_qwen3](#renderhf_qwen3) | 120,160 | +0.11% |  | 100 | 0% |  | 3,248 | 0% | 0% |
| [Load/html_autoescape](#loadhtml_autoescape) | 193,083 | +0.11% |  | 43 | 0% |  | 12,640 | 0% | -48.78% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,211,254 | 0% |  | 674 | 0% |  | 45,408 | 0% | +12.44% |
| [Load/inheritance](#loadinheritance) | 22,740 | 0% | -80.07% | 7 | 0% | -93.46% | 1,328 | 0% | -53.76% |
| [Render/inheritance](#renderinheritance) | 137,619 | 0% | -81.60% | 35 | 0% | -94.37% | 7,384 | 0% | +22.74% |
| [Load/int_arith](#loadint_arith) | 65,834 | +0.01% |  | 20 | 0% |  | 3,872 | 0% | +0.41% |
| [Render/int_arith](#renderint_arith) | 825,070 | +0.01% |  | 12 | 0% |  | 3,272 | 0% | -0.49% |
| [Module/keep](#modulekeep) | 6,960 | 0% |  | 7 | 0% |  |  |  |  |
| [Module/keep_written](#modulekeep_written) | 36,351 | +0.10% |  | 11 | 0% |  |  |  |  |
| [Load/large_static](#loadlarge_static) | 154,163 | 0% | -82.60% | 14 | 0% | -96.82% | 43,120 | 0% | -19.23% |
| [Render/large_static](#renderlarge_static) | 20,700 | 0% | -72.45% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [RenderSink/large_static](#rendersinklarge_static) | 12,686 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/large_static](#rendertolarge_static) | 20,263 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/macros](#loadmacros) | 55,711 | +0.06% | -75.56% | 27 | 0% | -85.48% | 4,344 | 0% | -30.12% |
| [Render/macros](#rendermacros) | 656,763 | 0% | -74.38% | 308 | 0% | -88.71% | 20,560 | 0% | +39.67% |
| [Load/many_tags](#loadmany_tags) | 8,972,987 | +0.06% | -79.68% | 951 | 0% | -97.22% | 438,144 | -0.01% | -58.48% |
| [Render/many_tags](#rendermany_tags) | 725,698 | 0% | -65.16% | 2 | 0% | -90.48% | 5,648 | 0% | -10.52% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 46,294 | 0% | -80.51% | 19 | 0% | -89.67% | 3,360 | 0% | -44.59% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,235,872 | 0% | -57.63% | 33 | 0% | -99.19% | 381,536 | 0% | +9.95% |
| [RenderSink/mitsuhiko_table](#rendersinkmitsuhiko_table) | 5,163,041 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table](#rendertomitsuhiko_table) | 5,235,355 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 54,532 | 0% |  | 35 | 0% |  | 5,688 | 0% | -31.17% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,581,817 | 0% |  | 33 | 0% |  | 1,414,928 | 0% | +2.46% |
| [RenderSink/mitsuhiko_table_wide](#rendersinkmitsuhiko_table_wide) | 5,405,104 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table_wide](#rendertomitsuhiko_table_wide) | 5,581,388 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/plain_text](#loadplain_text) | 3,693 | 0% | -61.83% | 4 | 0% | -77.78% | 672 | 0% | -40.00% |
| [Render/plain_text](#renderplain_text) | 1,017 | 0% | -85.03% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/plain_text](#rendersinkplain_text) | 717 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/plain_text](#rendertoplain_text) | 818 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/strings](#loadstrings) | 70,057 | +0.03% | -76.00% | 30 | 0% | -86.61% | 4,344 | 0% | -51.95% |
| [Render/strings](#renderstrings) | 582,971 | +0.06% | -72.21% | 344 | 0% | -83.73% | 9,088 | 0% | -49.31% |
| [Load/substitute](#loadsubstitute) | 7,710 | 0% | -74.18% | 6 | 0% | -83.78% | 832 | 0% | -45.26% |
| [Render/substitute](#rendersubstitute) | 2,233 | 0% | -74.66% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/substitute](#rendersinksubstitute) | 1,936 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/substitute](#rendertosubstitute) | 2,038 | 0% |  | 0 |  |  | 0 |  |  |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 750 | +1.08% | 743 | +1.50% | 0 |  | 7,467 | +2.53% |
| Render/chat_llama | 1,173 | +0.69% | 522 | +3.57% | 0 |  | 3,763 | +5.58% |
| Load/chat_mistral | 1,111 | -3.05% | 1,074 | +1.03% | 0 |  | 8,517 | +2.32% |
| Render/chat_mistral | 1,842 | -1.86% | 533 | -2.56% | 0 |  | 9,827 | +4.92% |
| Load/chat_qwen | 497 | +2.90% | 592 | +1.02% | 0 |  | 4,553 | +0.02% |
| Render/chat_qwen | 1,072 | -1.83% | 402 | +1.01% | 0 |  | 3,230 | +1.60% |
| Load/config_file | 181 | -2.69% | 154 | -1.28% | 0 |  | 2,684 | +1.78% |
| Render/config_file | 5,956 | -7.23% | 1,684 | +5.25% | 0 |  | 40,650 | -2.32% |
| Load/dict_ops | 30 | -16.67% | 13 | -38.10% | 0 |  | 1,281 | -1.39% |
| Render/dict_ops | 660 | +0.76% | 425 | -0.47% | 0 |  | 870 | -0.57% |
| Load/expressions | 34 | -19.05% | 24 | -20.00% | 0 |  | 1,291 | +4.03% |
| Render/expressions | 2 | 0% | 1 |  | 0 |  | 333 | +24.25% |
| Load/filters | 90 | +7.14% | 43 | -14.00% | 0 |  | 1,716 | -4.56% |
| Render/filters | 17 | +41.67% | 5 | +400.00% | 0 |  | 767 | -1.67% |
| Load/for_filter_if | 21 | +31.25% | 10 | -9.09% | 0 |  | 1,069 | +1.04% |
| Render/for_filter_if | 1,731 | +0.23% | 93 | +6.90% | 0 |  | 185 | -2.63% |
| Load/for_loop_vars | 37 | -17.78% | 22 | -24.14% | 0 |  | 1,108 | +2.03% |
| Render/for_loop_vars | 6 | +50.00% | 1 | 0% | 0 |  | 88 | -49.13% |
| Load/for_range | 7 | 0% | 5 | 0% | 0 |  | 739 | -0.14% |
| Render/for_range | 1 | 0% | 0 |  | 0 |  | 94 | +4.44% |
| Load/hf_deepseek_v31 | 991 | +3.23% | 976 | +1.24% | 0 |  | 5,983 | -3.48% |
| Render/hf_deepseek_v31 | 357 | 0% | 86 | -1.15% | 0 |  | 1,345 | +5.00% |
| Load/hf_gpt_oss | 4,600 | +0.97% | 3,240 | -0.18% | 0 |  | 21,236 | -0.94% |
| Render/hf_gpt_oss | 1,208 | +0.33% | 380 | 0% | 0 |  | 4,569 | +5.20% |
| Load/hf_mistral_small | 1,778 | +1.83% | 1,575 | +0.70% | 0 |  | 11,128 | +2.84% |
| Render/hf_mistral_small | 956 | +0.53% | 313 | +13.00% | 0 |  | 3,707 | +3.46% |
| Load/hf_qwen3 | 1,443 | +1.62% | 1,248 | +0.16% | 0 |  | 8,805 | +0.79% |
| Render/hf_qwen3 | 672 | -1.90% | 216 | +9.09% | 0 |  | 2,560 | +1.95% |
| Load/html_autoescape | 443 | +0.91% | 515 | -0.19% | 0 |  | 4,011 | +3.67% |
| Render/html_autoescape | 3,685 | -13.50% | 1,289 | -16.35% | 0 |  | 38,843 | +3.90% |
| Load/inheritance | 3 | 0% | 3 | -25.00% | 0 |  | 659 | +1.70% |
| Render/inheritance | 583 | -0.17% | 94 | +2.17% | 0 |  | 903 | +0.67% |
| Load/int_arith | 85 | -2.30% | 51 | -15.00% | 0 |  | 2,091 | +7.34% |
| Render/int_arith | 7 | +133.33% | 4 | +300.00% | 0 |  | 775 | +4.59% |
| Module/keep | 2 | +100.00% | 2 | 0% | 0 |  | 433 | +4.09% |
| Module/keep_written | 2,928 | -0.03% | 559 | +0.36% | 0 |  | 759 | +7.81% |
| Load/large_static | 1,929 | 0% | 913 | 0% | 0 |  | 231 | -12.17% |
| Render/large_static | 710 | 0% | 568 | 0% | 0 |  | 4 | -63.64% |
| RenderSink/large_static | 0 |  | 0 |  | 0 |  | 2 | 0% |
| RenderTo/large_static | 687 | 0% | 572 | 0% | 0 |  | 4 | 0% |
| Load/macros | 70 | -2.78% | 50 | -3.85% | 0 |  | 1,673 | +1.64% |
| Render/macros | 765 | +1.19% | 216 | -0.92% | 0 |  | 1,419 | -7.07% |
| Load/many_tags | 28,191 | -0.45% | 22,588 | +0.06% | 0 |  | 224,798 | -2.39% |
| Render/many_tags | 6,227 | -0.10% | 844 | -1.75% | 0 |  | 133 | -30.73% |
| Load/mitsuhiko_table | 25 | +19.05% | 24 | +14.29% | 0 |  | 829 | -2.70% |
| Render/mitsuhiko_table | 10,087 | +0.09% | 6,264 | +0.02% | 0 |  | 340 | 0% |
| RenderSink/mitsuhiko_table | 9,965 | +0.17% | 1,203 | 0% | 0 |  | 260 | -1.14% |
| RenderTo/mitsuhiko_table | 10,055 | +0.12% | 6,262 | +0.10% | 0 |  | 320 | +0.31% |
| Load/mitsuhiko_table_wide | 51 | +2.00% | 50 | -5.66% | 0 |  | 962 | +5.25% |
| Render/mitsuhiko_table_wide | 10,472 | +1.40% | 22,734 | +0.29% | 0 |  | 326 | +1.88% |
| RenderSink/mitsuhiko_table_wide | 10,117 | +0.10% | 1,295 | +0.08% | 0 |  | 258 | -2.64% |
| RenderTo/mitsuhiko_table_wide | 10,474 | +1.56% | 22,678 | +0.05% | 0 |  | 297 | +4.95% |
| Load/plain_text | 0 | -100.00% | 2 | 0% | 0 |  | 7 | +16.67% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 6 | 0% |
| RenderSink/plain_text | 0 |  | 0 |  | 0 |  | 4 | +33.33% |
| RenderTo/plain_text | 0 |  | 0 |  | 0 |  | 5 | 0% |
| Load/strings | 116 | -6.45% | 77 | -4.94% | 0 |  | 2,698 | +1.62% |
| Render/strings | 412 | +3.78% | 473 | +3.50% | 0 |  | 1,554 | +0.65% |
| Load/substitute | 3 | +50.00% | 4 | +33.33% | 0 |  | 140 | +6.87% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 3 | -25.00% |
| RenderSink/substitute | 0 |  | 0 |  | 0 |  | 2 | -33.33% |
| RenderTo/substitute | 0 |  | 0 |  | 0 |  | 2 | -33.33% |

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

### Load/hf_deepseek_v31

![Load/hf_deepseek_v31](charts/Load_hf_deepseek_v31.svg)

### Render/hf_deepseek_v31

![Render/hf_deepseek_v31](charts/Render_hf_deepseek_v31.svg)

### Load/hf_gpt_oss

![Load/hf_gpt_oss](charts/Load_hf_gpt_oss.svg)

### Render/hf_gpt_oss

![Render/hf_gpt_oss](charts/Render_hf_gpt_oss.svg)

### Load/hf_mistral_small

![Load/hf_mistral_small](charts/Load_hf_mistral_small.svg)

### Render/hf_mistral_small

![Render/hf_mistral_small](charts/Render_hf_mistral_small.svg)

### Load/hf_qwen3

![Load/hf_qwen3](charts/Load_hf_qwen3.svg)

### Render/hf_qwen3

![Render/hf_qwen3](charts/Render_hf_qwen3.svg)

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

### Module/keep

![Module/keep](charts/Module_keep.svg)

### Module/keep_written

![Module/keep_written](charts/Module_keep_written.svg)

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
