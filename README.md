# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (218 so far, latest [0cbdee5](https://github.com/jinja2cpp/Jinja2Cpp/commit/0cbdee59b0d0172f14102ebc0662d78fd6bbffce) on 2026-11-13). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 335,347 | +0.01% |  | 55 | 0% |  | 18,480 | +0.09% | -52.71% |
| [Render/chat_llama](#renderchat_llama) | 294,975 | -0.01% |  | 225 | 0% |  | 11,616 | 0% | -19.11% |
| [Load/chat_mistral](#loadchat_mistral) | 385,136 | +0.01% |  | 52 | 0% |  | 20,904 | +0.08% | -53.18% |
| [Render/chat_mistral](#renderchat_mistral) | 501,604 | +0.03% |  | 402 | 0% |  | 10,032 | +0.32% | -24.05% |
| [Load/chat_qwen](#loadchat_qwen) | 261,156 | +0.02% |  | 44 | 0% |  | 14,432 | +0.11% | -47.19% |
| [Render/chat_qwen](#renderchat_qwen) | 278,310 | -0.18% |  | 212 | 0% |  | 8,304 | 0% | -2.35% |
| [Load/config_file](#loadconfig_file) | 94,239 | +0.05% |  | 25 | 0% |  | 5,480 | +0.29% | -48.11% |
| [Render/config_file](#renderconfig_file) | 1,275,632 | +0.02% |  | 908 | 0% |  | 24,440 | 0% | +2.65% |
| [Load/dict_ops](#loaddict_ops) | 32,235 | +0.14% | -78.19% | 9 | 0% | -93.13% | 2,192 | +0.74% | -48.50% |
| [Render/dict_ops](#renderdict_ops) | 295,947 | +0.14% | -57.16% | 112 | 0% | -75.11% | 24,408 | 0% | -29.65% |
| [Load/expressions](#loadexpressions) | 44,235 | +0.10% | -78.54% | 16 | 0% | -89.04% | 2,744 | +0.59% | -55.28% |
| [Render/expressions](#renderexpressions) | 426,996 | 0% | -52.88% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 67,459 | +0.04% | -75.42% | 27 | 0% | -88.16% | 3,864 | +0.42% | -66.15% |
| [Render/filters](#renderfilters) | 27,732 | +0.08% | -68.72% | 23 | 0% | -68.92% | 752 | 0% | -84.69% |
| [Load/for_filter_if](#loadfor_filter_if) | 29,057 | +0.15% | -78.54% | 7 | 0% | -94.31% | 1,920 | +0.84% | -44.32% |
| [Render/for_filter_if](#renderfor_filter_if) | 274,397 | +0.01% | -68.30% | 4 | 0% | -98.23% | 16,560 | 0% | +354.95% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 38,877 | +0.09% | -78.48% | 10 | 0% | -93.63% | 2,384 | +0.68% | -47.26% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 180,584 | +0.01% | -83.17% | 2 | 0% | -99.62% | 1,568 | 0% | -43.35% |
| [Load/for_range](#loadfor_range) | 13,259 | +0.33% | -76.61% | 7 | 0% | -87.50% | 1,104 | +1.47% | -44.80% |
| [Render/for_range](#renderfor_range) | 38,447 | +0.03% | -58.30% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/hf_deepseek_v31](#loadhf_deepseek_v31) | 382,850 | -0.04% |  | 55 | 0% |  | 21,992 | +0.07% | +0.81% |
| [Render/hf_deepseek_v31](#renderhf_deepseek_v31) | 92,994 | +0.05% |  | 43 | 0% |  | 2,480 | 0% | 0% |
| [Load/hf_gpt_oss](#loadhf_gpt_oss) | 1,522,103 | -0.03% |  | 287 | 0% |  | 74,688 | -0.02% | +0.63% |
| [Render/hf_gpt_oss](#renderhf_gpt_oss) | 173,372 | -0.01% |  | 155 | 0% |  | 5,080 | -0.31% | +2.92% |
| [Load/hf_mistral_small](#loadhf_mistral_small) | 591,509 | +0.01% |  | 88 | 0% |  | 33,064 | +0.05% | +0.39% |
| [Render/hf_mistral_small](#renderhf_mistral_small) | 142,665 | +0.01% |  | 99 | 0% |  | 6,120 | 0% | -0.26% |
| [Load/hf_qwen3](#loadhf_qwen3) | 475,701 | +0.05% |  | 82 | 0% |  | 25,576 | +0.06% | +0.38% |
| [Render/hf_qwen3](#renderhf_qwen3) | 121,196 | +0.04% |  | 100 | 0% |  | 3,248 | 0% | 0% |
| [Load/html_autoescape](#loadhtml_autoescape) | 188,631 | +0.02% |  | 43 | 0% |  | 12,688 | +0.13% | -48.59% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,209,729 | +0.01% |  | 674 | 0% |  | 45,408 | 0% | +12.44% |
| [Load/inheritance](#loadinheritance) | 21,939 | +0.20% | -80.78% | 7 | 0% | -93.46% | 1,360 | +1.19% | -52.65% |
| [Render/inheritance](#renderinheritance) | 137,750 | +0.09% | -81.58% | 35 | 0% | -94.37% | 7,384 | 0% | +22.74% |
| [Load/int_arith](#loadint_arith) | 65,009 | +0.10% |  | 20 | 0% |  | 3,888 | +0.41% | +0.83% |
| [Render/int_arith](#renderint_arith) | 828,243 | 0% |  | 12 | 0% |  | 3,272 | 0% | -0.49% |
| [Module/keep](#modulekeep) | 7,128 | +0.11% |  | 7 | 0% |  |  |  |  |
| [Module/keep_written](#modulekeep_written) | 29,081 | +0.02% |  | 11 | 0% |  |  |  |  |
| [Load/large_static](#loadlarge_static) | 147,742 | +0.03% | -83.32% | 14 | 0% | -96.82% | 43,136 | +0.04% | -19.20% |
| [Render/large_static](#renderlarge_static) | 20,740 | +0.07% | -72.40% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [RenderSink/large_static](#rendersinklarge_static) | 12,727 | +0.09% |  | 0 |  |  | 0 |  |  |
| [RenderTo/large_static](#rendertolarge_static) | 20,303 | +0.07% |  | 0 |  |  | 0 |  |  |
| [Load/macros](#loadmacros) | 54,968 | -0.46% | -75.89% | 27 | 0% | -85.48% | 4,408 | +0.36% | -29.09% |
| [Render/macros](#rendermacros) | 659,503 | +0.10% | -74.27% | 308 | 0% | -88.71% | 20,560 | 0% | +39.67% |
| [Load/many_tags](#loadmany_tags) | 8,714,972 | +0.01% | -80.27% | 951 | 0% | -97.22% | 438,176 | +0.01% | -58.48% |
| [Render/many_tags](#rendermany_tags) | 729,970 | 0% | -64.96% | 2 | 0% | -90.48% | 5,648 | 0% | -10.52% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 45,097 | +0.10% | -81.02% | 19 | 0% | -89.67% | 3,376 | +0.48% | -44.33% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,203,070 | 0% | -57.89% | 33 | 0% | -99.19% | 381,536 | 0% | +9.95% |
| [RenderSink/mitsuhiko_table](#rendersinkmitsuhiko_table) | 5,130,335 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table](#rendertomitsuhiko_table) | 5,202,623 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 52,784 | +0.03% |  | 35 | 0% |  | 5,704 | +0.28% | -30.98% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,549,116 | 0% |  | 33 | 0% |  | 1,414,928 | 0% | +2.46% |
| [RenderSink/mitsuhiko_table_wide](#rendersinkmitsuhiko_table_wide) | 5,372,418 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table_wide](#rendertomitsuhiko_table_wide) | 5,548,683 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/plain_text](#loadplain_text) | 3,752 | +1.19% | -61.22% | 4 | 0% | -77.78% | 688 | +2.38% | -38.57% |
| [Render/plain_text](#renderplain_text) | 1,048 | +1.16% | -84.57% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/plain_text](#rendersinkplain_text) | 749 | +1.49% |  | 0 |  |  | 0 |  |  |
| [RenderTo/plain_text](#rendertoplain_text) | 849 | +1.43% |  | 0 |  |  | 0 |  |  |
| [Load/prompt_budget](#loadprompt_budget) | 34,469 | +0.13% |  | 18 | 0% |  | 2,240 | +0.72% | +0.72% |
| [Render/prompt_budget](#renderprompt_budget) | 1,319,644 | -0.01% |  | 76 | 0% |  | 36,416 | 0% | 0% |
| [Load/strings](#loadstrings) | 68,941 | -0.16% | -76.38% | 30 | 0% | -86.61% | 4,360 | +0.37% | -51.77% |
| [Render/strings](#renderstrings) | 583,371 | 0% | -72.19% | 344 | 0% | -83.73% | 9,088 | 0% | -49.31% |
| [Load/substitute](#loadsubstitute) | 7,531 | +0.59% | -74.78% | 6 | 0% | -83.78% | 848 | +1.92% | -44.21% |
| [Render/substitute](#rendersubstitute) | 2,282 | +0.57% | -74.11% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/substitute](#rendersinksubstitute) | 1,986 | +0.56% |  | 0 |  |  | 0 |  |  |
| [RenderTo/substitute](#rendertosubstitute) | 2,087 | +0.63% |  | 0 |  |  | 0 |  |  |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 751 | 0% | 752 | -0.79% | 0 |  | 7,427 | -0.48% |
| Render/chat_llama | 1,179 | +1.90% | 517 | -3.18% | 0 |  | 4,068 | -2.12% |
| Load/chat_mistral | 1,108 | -0.09% | 1,081 | -1.19% | 0 |  | 8,731 | +1.67% |
| Render/chat_mistral | 1,901 | +1.44% | 570 | -2.73% | 0 |  | 10,251 | +5.55% |
| Load/chat_qwen | 571 | -12.02% | 624 | -1.42% | 0 |  | 4,487 | +0.63% |
| Render/chat_qwen | 1,071 | -3.86% | 401 | -5.87% | 0 |  | 3,503 | -0.65% |
| Load/config_file | 233 | 0% | 194 | -2.02% | 0 |  | 2,879 | +2.46% |
| Render/config_file | 6,278 | -1.88% | 1,558 | +8.19% | 0 |  | 43,373 | +1.13% |
| Load/dict_ops | 60 | +5.26% | 30 | -9.09% | 0 |  | 1,317 | -3.66% |
| Render/dict_ops | 681 | +1.95% | 419 | -1.41% | 0 |  | 902 | +2.15% |
| Load/expressions | 39 | +39.29% | 26 | +30.00% | 0 |  | 1,271 | -4.36% |
| Render/expressions | 4 | -60.00% | 1 | 0% | 0 |  | 305 | -6.15% |
| Load/filters | 105 | +8.25% | 54 | +5.88% | 0 |  | 1,662 | +7.02% |
| Render/filters | 26 | +30.00% | 7 | 0% | 0 |  | 767 | -6.23% |
| Load/for_filter_if | 36 | -16.28% | 20 | -16.67% | 0 |  | 1,087 | +0.65% |
| Render/for_filter_if | 1,743 | +0.23% | 132 | -0.75% | 0 |  | 181 | -9.50% |
| Load/for_loop_vars | 57 | +16.33% | 23 | -11.54% | 0 |  | 1,226 | +5.51% |
| Render/for_loop_vars | 10 | -23.08% | 1 | -66.67% | 0 |  | 154 | +2.67% |
| Load/for_range | 3 | -25.00% | 4 | -33.33% | 0 |  | 766 | -1.03% |
| Render/for_range | 1 | -50.00% | 0 |  | 0 |  | 141 | -2.08% |
| Load/hf_deepseek_v31 | 993 | -0.30% | 981 | -2.10% | 0 |  | 6,674 | +7.68% |
| Render/hf_deepseek_v31 | 359 | -2.45% | 78 | +2.63% | 0 |  | 1,449 | +2.26% |
| Load/hf_gpt_oss | 4,662 | -0.02% | 3,390 | +0.44% | 0 |  | 23,309 | +6.24% |
| Render/hf_gpt_oss | 1,226 | +2.00% | 392 | +2.62% | 0 |  | 4,629 | -0.81% |
| Load/hf_mistral_small | 1,908 | -0.78% | 1,642 | -0.06% | 0 |  | 11,190 | +1.66% |
| Render/hf_mistral_small | 952 | -1.65% | 299 | +0.67% | 0 |  | 3,732 | +1.94% |
| Load/hf_qwen3 | 1,515 | -1.94% | 1,339 | -0.22% | 0 |  | 8,998 | +1.09% |
| Render/hf_qwen3 | 693 | +3.28% | 224 | +2.28% | 0 |  | 2,670 | +0.45% |
| Load/html_autoescape | 505 | -1.17% | 533 | -2.02% | 0 |  | 4,207 | +2.99% |
| Render/html_autoescape | 3,977 | -9.59% | 1,084 | -17.13% | 0 |  | 37,840 | +3.20% |
| Load/inheritance | 5 | +66.67% | 3 | 0% | 0 |  | 703 | +1.15% |
| Render/inheritance | 603 | -0.99% | 108 | +0.93% | 0 |  | 980 | -0.71% |
| Load/int_arith | 85 | -8.60% | 53 | -3.64% | 0 |  | 2,001 | -0.35% |
| Render/int_arith | 16 | -30.43% | 5 | 0% | 0 |  | 762 | -0.39% |
| Module/keep | 1 | 0% | 2 | 0% | 0 |  | 418 | -4.13% |
| Module/keep_written | 1,137 | +0.26% | 559 | -0.36% | 0 |  | 739 | +1.09% |
| Load/large_static | 1,927 | -0.36% | 904 | -0.11% | 0 |  | 308 | -13.73% |
| Render/large_static | 708 | -0.14% | 568 | 0% | 0 |  | 5 | -54.55% |
| RenderSink/large_static | 1 | 0% | 0 |  | 0 |  | 3 | -25.00% |
| RenderTo/large_static | 684 | -0.15% | 572 | 0% | 0 |  | 4 | -42.86% |
| Load/macros | 102 | +0.99% | 75 | -1.32% | 0 |  | 1,751 | +0.34% |
| Render/macros | 751 | +0.54% | 233 | +1.75% | 0 |  | 632 | -29.31% |
| Load/many_tags | 27,911 | -1.75% | 22,584 | +0.06% | 0 |  | 242,653 | +6.37% |
| Render/many_tags | 6,248 | -0.02% | 868 | +0.70% | 0 |  | 142 | -7.19% |
| Load/mitsuhiko_table | 55 | +3.77% | 60 | 0% | 0 |  | 837 | -7.92% |
| Render/mitsuhiko_table | 10,073 | -0.13% | 6,283 | -0.06% | 0 |  | 405 | -5.59% |
| RenderSink/mitsuhiko_table | 9,994 | +0.04% | 1,218 | +1.08% | 0 |  | 329 | -12.96% |
| RenderTo/mitsuhiko_table | 10,064 | -0.15% | 6,295 | +0.05% | 0 |  | 385 | -5.17% |
| Load/mitsuhiko_table_wide | 61 | -31.46% | 81 | -18.18% | 0 |  | 1,016 | -2.78% |
| Render/mitsuhiko_table_wide | 10,234 | +0.31% | 22,666 | +0.08% | 0 |  | 436 | +10.66% |
| RenderSink/mitsuhiko_table_wide | 10,072 | +0.12% | 1,290 | +2.22% | 0 |  | 355 | +1.43% |
| RenderTo/mitsuhiko_table_wide | 10,249 | +0.35% | 22,675 | +0.10% | 0 |  | 420 | +10.24% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 6 | -14.29% |
| Render/plain_text | 1 | 0% | 0 |  | 0 |  | 6 | -14.29% |
| RenderSink/plain_text | 0 |  | 0 |  | 0 |  | 4 | 0% |
| RenderTo/plain_text | 1 | 0% | 0 |  | 0 |  | 5 | -16.67% |
| Load/prompt_budget | 91 | +10.98% | 47 | +4.44% | 0 |  | 1,405 | +0.21% |
| Render/prompt_budget | 4,807 | -0.15% | 546 | -0.18% | 0 |  | 1,042 | +17.74% |
| Load/strings | 159 | -1.85% | 107 | -7.76% | 0 |  | 2,796 | +1.78% |
| Render/strings | 422 | -4.09% | 484 | -0.82% | 0 |  | 1,634 | +1.81% |
| Load/substitute | 2 | 0% | 3 | 0% | 0 |  | 204 | +5.15% |
| Render/substitute | 0 | -100.00% | 0 |  | 0 |  | 4 | -42.86% |
| RenderSink/substitute | 0 | -100.00% | 0 |  | 0 |  | 3 | -25.00% |
| RenderTo/substitute | 0 | -100.00% | 0 |  | 0 |  | 4 | -20.00% |

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
