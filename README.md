# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (220 so far, latest [4ee1ca8](https://github.com/jinja2cpp/Jinja2Cpp/commit/4ee1ca8a06c2de7cec8c4db96c4a2af077db7103) on 2026-11-13). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 335,442 | +0.01% |  | 55 | 0% |  | 18,480 | 0% | -52.71% |
| [Render/chat_llama](#renderchat_llama) | 295,351 | +0.10% |  | 225 | 0% |  | 11,616 | 0% | -19.11% |
| [Load/chat_mistral](#loadchat_mistral) | 385,101 | -0.01% |  | 52 | 0% |  | 20,904 | 0% | -53.18% |
| [Render/chat_mistral](#renderchat_mistral) | 501,949 | +0.06% |  | 402 | 0% |  | 10,032 | 0% | -24.05% |
| [Load/chat_qwen](#loadchat_qwen) | 261,145 | -0.01% |  | 44 | 0% |  | 14,432 | 0% | -47.19% |
| [Render/chat_qwen](#renderchat_qwen) | 278,783 | +0.14% |  | 212 | 0% |  | 8,304 | 0% | -2.35% |
| [Load/config_file](#loadconfig_file) | 94,253 | 0% |  | 25 | 0% |  | 5,480 | 0% | -48.11% |
| [Render/config_file](#renderconfig_file) | 1,275,337 | -0.01% |  | 908 | 0% |  | 24,440 | 0% | +2.65% |
| [Load/dict_ops](#loaddict_ops) | 32,235 | 0% | -78.19% | 9 | 0% | -93.13% | 2,192 | 0% | -48.50% |
| [Render/dict_ops](#renderdict_ops) | 296,037 | +0.03% | -57.15% | 112 | 0% | -75.11% | 24,408 | 0% | -29.65% |
| [Load/expressions](#loadexpressions) | 44,235 | 0% | -78.54% | 16 | 0% | -89.04% | 2,744 | 0% | -55.28% |
| [Render/expressions](#renderexpressions) | 426,998 | 0% | -52.88% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 67,515 | 0% | -75.40% | 27 | 0% | -88.16% | 3,864 | 0% | -66.15% |
| [Render/filters](#renderfilters) | 27,794 | +0.15% | -68.65% | 23 | 0% | -68.92% | 752 | 0% | -84.69% |
| [Load/for_filter_if](#loadfor_filter_if) | 29,057 | 0% | -78.54% | 7 | 0% | -94.31% | 1,920 | 0% | -44.32% |
| [Render/for_filter_if](#renderfor_filter_if) | 275,002 | +0.22% | -68.23% | 4 | 0% | -98.23% | 16,560 | 0% | +354.95% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 38,841 | 0% | -78.50% | 10 | 0% | -93.63% | 2,384 | 0% | -47.26% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 180,586 | 0% | -83.17% | 2 | 0% | -99.62% | 1,568 | 0% | -43.35% |
| [Load/for_range](#loadfor_range) | 13,259 | 0% | -76.61% | 7 | 0% | -87.50% | 1,104 | 0% | -44.80% |
| [Render/for_range](#renderfor_range) | 38,449 | +0.01% | -58.30% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/hf_deepseek_v31](#loadhf_deepseek_v31) | 382,898 | 0% |  | 55 | 0% |  | 21,992 | 0% | +0.81% |
| [Render/hf_deepseek_v31](#renderhf_deepseek_v31) | 93,185 | +0.21% |  | 43 | 0% |  | 2,480 | 0% | 0% |
| [Load/hf_gpt_oss](#loadhf_gpt_oss) | 1,522,258 | -0.01% |  | 287 | 0% |  | 74,688 | 0% | +0.63% |
| [Render/hf_gpt_oss](#renderhf_gpt_oss) | 173,518 | +0.07% |  | 155 | 0% |  | 5,080 | 0% | +2.92% |
| [Load/hf_mistral_small](#loadhf_mistral_small) | 591,634 | 0% |  | 88 | 0% |  | 33,064 | 0% | +0.39% |
| [Render/hf_mistral_small](#renderhf_mistral_small) | 142,759 | +0.05% |  | 99 | 0% |  | 6,120 | 0% | -0.26% |
| [Load/hf_qwen3](#loadhf_qwen3) | 475,747 | -0.01% |  | 82 | 0% |  | 25,576 | 0% | +0.38% |
| [Render/hf_qwen3](#renderhf_qwen3) | 121,388 | +0.11% |  | 100 | 0% |  | 3,248 | 0% | 0% |
| [Load/html_autoescape](#loadhtml_autoescape) | 188,619 | 0% |  | 43 | 0% |  | 12,688 | 0% | -48.59% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,211,369 | +0.06% |  | 674 | 0% |  | 45,408 | 0% | +12.44% |
| [Load/inheritance](#loadinheritance) | 21,939 | 0% | -80.78% | 7 | 0% | -93.46% | 1,360 | 0% | -52.65% |
| [Render/inheritance](#renderinheritance) | 137,894 | +0.10% | -81.56% | 35 | 0% | -94.37% | 7,384 | 0% | +22.74% |
| [Load/int_arith](#loadint_arith) | 65,009 | 0% |  | 20 | 0% |  | 3,888 | 0% | +0.83% |
| [Render/int_arith](#renderint_arith) | 829,431 | +0.14% |  | 12 | 0% |  | 3,272 | 0% | -0.49% |
| [Module/keep](#modulekeep) | 7,133 | +0.07% |  | 7 | 0% |  |  |  |  |
| [Module/keep_written](#modulekeep_written) | 29,078 | -0.03% |  | 11 | 0% |  |  |  |  |
| [Load/large_static](#loadlarge_static) | 147,742 | 0% | -83.32% | 14 | 0% | -96.82% | 43,136 | 0% | -19.20% |
| [Render/large_static](#renderlarge_static) | 20,740 | 0% | -72.40% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [RenderSink/large_static](#rendersinklarge_static) | 12,727 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/large_static](#rendertolarge_static) | 20,303 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/macros](#loadmacros) | 54,940 | -0.03% | -75.90% | 27 | 0% | -85.48% | 4,408 | 0% | -29.09% |
| [Render/macros](#rendermacros) | 659,710 | +0.03% | -74.26% | 308 | 0% | -88.71% | 20,560 | 0% | +39.67% |
| [Load/many_tags](#loadmany_tags) | 8,714,972 | 0% | -80.27% | 951 | 0% | -97.22% | 438,176 | 0% | -58.48% |
| [Render/many_tags](#rendermany_tags) | 729,972 | 0% | -64.96% | 2 | 0% | -90.48% | 5,648 | 0% | -10.52% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 45,097 | 0% | -81.02% | 19 | 0% | -89.67% | 3,376 | 0% | -44.33% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,205,028 | +0.04% | -57.88% | 33 | 0% | -99.19% | 381,536 | 0% | +9.95% |
| [RenderSink/mitsuhiko_table](#rendersinkmitsuhiko_table) | 5,132,293 | +0.04% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table](#rendertomitsuhiko_table) | 5,204,581 | +0.04% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 52,802 | +0.03% |  | 35 | 0% |  | 5,704 | 0% | -30.98% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,551,074 | +0.04% |  | 33 | 0% |  | 1,414,928 | 0% | +2.46% |
| [RenderSink/mitsuhiko_table_wide](#rendersinkmitsuhiko_table_wide) | 5,374,376 | +0.04% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table_wide](#rendertomitsuhiko_table_wide) | 5,550,641 | +0.04% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/plain_text](#loadplain_text) | 3,752 | 0% | -61.22% | 4 | 0% | -77.78% | 688 | 0% | -38.57% |
| [Render/plain_text](#renderplain_text) | 1,048 | 0% | -84.57% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/plain_text](#rendersinkplain_text) | 749 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/plain_text](#rendertoplain_text) | 849 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/prompt_budget](#loadprompt_budget) | 34,462 | 0% |  | 18 | 0% |  | 2,240 | 0% | +0.72% |
| [Render/prompt_budget](#renderprompt_budget) | 1,320,291 | +0.05% |  | 76 | 0% |  | 36,416 | 0% | 0% |
| [Load/strings](#loadstrings) | 68,994 | +0.01% | -76.36% | 30 | 0% | -86.61% | 4,360 | 0% | -51.77% |
| [Render/strings](#renderstrings) | 584,306 | +0.14% | -72.14% | 344 | 0% | -83.73% | 9,088 | 0% | -49.31% |
| [Load/substitute](#loadsubstitute) | 7,531 | 0% | -74.78% | 6 | 0% | -83.78% | 848 | 0% | -44.21% |
| [Render/substitute](#rendersubstitute) | 2,282 | 0% | -74.11% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/substitute](#rendersinksubstitute) | 1,986 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/substitute](#rendertosubstitute) | 2,087 | 0% |  | 0 |  |  | 0 |  |  |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 736 | -0.41% | 752 | -0.27% | 0 |  | 7,828 | +1.90% |
| Render/chat_llama | 1,198 | +0.50% | 512 | +0.39% | 0 |  | 4,244 | +2.49% |
| Load/chat_mistral | 1,110 | +0.73% | 1,085 | +0.18% | 0 |  | 8,892 | -0.24% |
| Render/chat_mistral | 1,940 | +1.15% | 587 | -0.17% | 0 |  | 10,316 | -2.90% |
| Load/chat_qwen | 573 | -1.38% | 627 | -0.63% | 0 |  | 4,640 | -4.37% |
| Render/chat_qwen | 1,078 | +0.84% | 396 | +2.33% | 0 |  | 3,617 | +2.15% |
| Load/config_file | 238 | +3.03% | 184 | +1.10% | 0 |  | 2,726 | -4.65% |
| Render/config_file | 6,440 | +1.87% | 1,594 | +0.06% | 0 |  | 42,922 | +1.44% |
| Load/dict_ops | 59 | +9.26% | 30 | +3.45% | 0 |  | 1,394 | +2.50% |
| Render/dict_ops | 684 | +0.74% | 422 | -0.71% | 0 |  | 894 | -1.97% |
| Load/expressions | 47 | +11.90% | 35 | +6.06% | 0 |  | 1,353 | +2.34% |
| Render/expressions | 11 | +266.67% | 4 | +300.00% | 0 |  | 321 | -7.76% |
| Load/filters | 100 | 0% | 38 | -9.52% | 0 |  | 1,566 | -9.16% |
| Render/filters | 13 | +62.50% | 4 | +33.33% | 0 |  | 791 | +1.80% |
| Load/for_filter_if | 30 | -21.05% | 15 | -6.25% | 0 |  | 1,076 | -5.11% |
| Render/for_filter_if | 1,742 | +0.29% | 131 | 0% | 0 |  | 199 | -7.44% |
| Load/for_loop_vars | 55 | +5.77% | 20 | +5.26% | 0 |  | 1,093 | -7.29% |
| Render/for_loop_vars | 17 | +54.55% | 2 | +100.00% | 0 |  | 190 | +6.15% |
| Load/for_range | 3 | 0% | 4 | 0% | 0 |  | 757 | -1.05% |
| Render/for_range | 1 | 0% | 0 | -100.00% | 0 |  | 146 | -9.88% |
| Load/hf_deepseek_v31 | 1,012 | -0.39% | 985 | -0.51% | 0 |  | 6,529 | +0.02% |
| Render/hf_deepseek_v31 | 360 | +2.56% | 70 | -2.78% | 0 |  | 1,412 | -3.88% |
| Load/hf_gpt_oss | 4,702 | +0.56% | 3,399 | +0.21% | 0 |  | 22,394 | -5.29% |
| Render/hf_gpt_oss | 1,243 | +0.89% | 392 | 0% | 0 |  | 4,668 | +0.93% |
| Load/hf_mistral_small | 1,894 | +0.48% | 1,635 | +0.18% | 0 |  | 11,467 | +0.75% |
| Render/hf_mistral_small | 951 | -1.86% | 303 | +0.33% | 0 |  | 3,627 | -4.60% |
| Load/hf_qwen3 | 1,523 | -0.72% | 1,343 | -0.07% | 0 |  | 8,945 | -5.67% |
| Render/hf_qwen3 | 701 | +1.74% | 225 | -0.88% | 0 |  | 2,647 | -3.85% |
| Load/html_autoescape | 510 | -0.58% | 533 | -0.56% | 0 |  | 3,883 | -14.28% |
| Render/html_autoescape | 3,927 | -3.91% | 1,062 | +2.91% | 0 |  | 36,894 | +5.47% |
| Load/inheritance | 4 | -50.00% | 3 | -25.00% | 0 |  | 699 | -3.32% |
| Render/inheritance | 603 | -1.31% | 107 | +0.94% | 0 |  | 967 | -1.83% |
| Load/int_arith | 82 | 0% | 54 | +3.85% | 0 |  | 2,015 | -8.03% |
| Render/int_arith | 16 | +6.67% | 5 | -16.67% | 0 |  | 750 | -3.35% |
| Module/keep | 1 | -80.00% | 2 | -33.33% | 0 |  | 433 | +7.44% |
| Module/keep_written | 1,135 | -0.18% | 559 | 0% | 0 |  | 757 | +2.44% |
| Load/large_static | 1,923 | -0.05% | 904 | 0% | 0 |  | 345 | +7.48% |
| Render/large_static | 708 | 0% | 568 | 0% | 0 |  | 6 | -80.00% |
| RenderSink/large_static | 1 | 0% | 0 |  | 0 |  | 4 | -20.00% |
| RenderTo/large_static | 684 | 0% | 572 | 0% | 0 |  | 5 | -28.57% |
| Load/macros | 98 | +4.26% | 72 | +2.86% | 0 |  | 1,711 | -3.33% |
| Render/macros | 755 | +0.67% | 233 | 0% | 0 |  | 624 | +2.46% |
| Load/many_tags | 27,993 | +0.43% | 22,570 | +0.04% | 0 |  | 232,720 | -4.50% |
| Render/many_tags | 6,253 | +0.02% | 867 | -0.23% | 0 |  | 159 | +9.66% |
| Load/mitsuhiko_table | 48 | +4.35% | 61 | +3.39% | 0 |  | 882 | +3.76% |
| Render/mitsuhiko_table | 10,053 | -0.04% | 6,279 | +0.02% | 0 |  | 367 | -2.39% |
| RenderSink/mitsuhiko_table | 9,977 | -0.04% | 1,214 | 0% | 0 |  | 308 | -0.65% |
| RenderTo/mitsuhiko_table | 10,051 | -0.03% | 6,291 | -0.03% | 0 |  | 332 | -6.21% |
| Load/mitsuhiko_table_wide | 70 | -4.11% | 75 | -3.85% | 0 |  | 1,060 | -2.12% |
| Render/mitsuhiko_table_wide | 10,219 | -0.03% | 22,662 | 0% | 0 |  | 383 | -1.29% |
| RenderSink/mitsuhiko_table_wide | 10,061 | +0.03% | 1,278 | -0.70% | 0 |  | 311 | -4.60% |
| RenderTo/mitsuhiko_table_wide | 10,235 | -0.04% | 22,670 | 0% | 0 |  | 362 | -0.82% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 42 | +500.00% |
| Render/plain_text | 1 | 0% | 0 |  | 0 |  | 7 | +16.67% |
| RenderSink/plain_text | 1 | 0% | 0 |  | 0 |  | 5 | 0% |
| RenderTo/plain_text | 1 | 0% | 0 |  | 0 |  | 6 | 0% |
| Load/prompt_budget | 86 | +2.38% | 45 | 0% | 0 |  | 1,393 | -0.29% |
| Render/prompt_budget | 4,815 | -0.02% | 544 | 0% | 0 |  | 898 | -3.13% |
| Load/strings | 147 | -1.34% | 107 | 0% | 0 |  | 2,782 | +0.43% |
| Render/strings | 417 | -1.65% | 485 | +0.41% | 0 |  | 1,584 | -1.06% |
| Load/substitute | 2 | -66.67% | 3 | 0% | 0 |  | 217 | +5.85% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 5 | -16.67% |
| RenderSink/substitute | 0 |  | 0 |  | 0 |  | 5 | 0% |
| RenderTo/substitute | 0 |  | 0 |  | 0 |  | 5 | 0% |

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

### Load/prompt_budget

![Load/prompt_budget](charts/Load_prompt_budget.svg)

### Render/prompt_budget

![Render/prompt_budget](charts/Render_prompt_budget.svg)

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
