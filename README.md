# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (219 so far, latest [f8d45aa](https://github.com/jinja2cpp/Jinja2Cpp/commit/f8d45aa90c707459b741609e1c72007c4b2be641) on 2026-11-13). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 335,408 | +0.02% |  | 55 | 0% |  | 18,480 | 0% | -52.71% |
| [Render/chat_llama](#renderchat_llama) | 295,047 | +0.02% |  | 225 | 0% |  | 11,616 | 0% | -19.11% |
| [Load/chat_mistral](#loadchat_mistral) | 385,150 | 0% |  | 52 | 0% |  | 20,904 | 0% | -53.18% |
| [Render/chat_mistral](#renderchat_mistral) | 501,649 | +0.01% |  | 402 | 0% |  | 10,032 | 0% | -24.05% |
| [Load/chat_qwen](#loadchat_qwen) | 261,165 | 0% |  | 44 | 0% |  | 14,432 | 0% | -47.19% |
| [Render/chat_qwen](#renderchat_qwen) | 278,382 | +0.03% |  | 212 | 0% |  | 8,304 | 0% | -2.35% |
| [Load/config_file](#loadconfig_file) | 94,253 | +0.01% |  | 25 | 0% |  | 5,480 | 0% | -48.11% |
| [Render/config_file](#renderconfig_file) | 1,275,474 | -0.01% |  | 908 | 0% |  | 24,440 | 0% | +2.65% |
| [Load/dict_ops](#loaddict_ops) | 32,235 | 0% | -78.19% | 9 | 0% | -93.13% | 2,192 | 0% | -48.50% |
| [Render/dict_ops](#renderdict_ops) | 295,947 | 0% | -57.16% | 112 | 0% | -75.11% | 24,408 | 0% | -29.65% |
| [Load/expressions](#loadexpressions) | 44,235 | 0% | -78.54% | 16 | 0% | -89.04% | 2,744 | 0% | -55.28% |
| [Render/expressions](#renderexpressions) | 426,996 | 0% | -52.88% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 67,515 | +0.08% | -75.40% | 27 | 0% | -88.16% | 3,864 | 0% | -66.15% |
| [Render/filters](#renderfilters) | 27,753 | +0.08% | -68.70% | 23 | 0% | -68.92% | 752 | 0% | -84.69% |
| [Load/for_filter_if](#loadfor_filter_if) | 29,057 | 0% | -78.54% | 7 | 0% | -94.31% | 1,920 | 0% | -44.32% |
| [Render/for_filter_if](#renderfor_filter_if) | 274,397 | 0% | -68.30% | 4 | 0% | -98.23% | 16,560 | 0% | +354.95% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 38,841 | -0.09% | -78.50% | 10 | 0% | -93.63% | 2,384 | 0% | -47.26% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 180,584 | 0% | -83.17% | 2 | 0% | -99.62% | 1,568 | 0% | -43.35% |
| [Load/for_range](#loadfor_range) | 13,259 | 0% | -76.61% | 7 | 0% | -87.50% | 1,104 | 0% | -44.80% |
| [Render/for_range](#renderfor_range) | 38,447 | 0% | -58.30% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/hf_deepseek_v31](#loadhf_deepseek_v31) | 382,884 | +0.01% |  | 55 | 0% |  | 21,992 | 0% | +0.81% |
| [Render/hf_deepseek_v31](#renderhf_deepseek_v31) | 92,990 | 0% |  | 43 | 0% |  | 2,480 | 0% | 0% |
| [Load/hf_gpt_oss](#loadhf_gpt_oss) | 1,522,359 | +0.02% |  | 287 | 0% |  | 74,688 | 0% | +0.63% |
| [Render/hf_gpt_oss](#renderhf_gpt_oss) | 173,395 | +0.01% |  | 155 | 0% |  | 5,080 | 0% | +2.92% |
| [Load/hf_mistral_small](#loadhf_mistral_small) | 591,628 | +0.02% |  | 88 | 0% |  | 33,064 | 0% | +0.39% |
| [Render/hf_mistral_small](#renderhf_mistral_small) | 142,690 | +0.02% |  | 99 | 0% |  | 6,120 | 0% | -0.26% |
| [Load/hf_qwen3](#loadhf_qwen3) | 475,810 | +0.02% |  | 82 | 0% |  | 25,576 | 0% | +0.38% |
| [Render/hf_qwen3](#renderhf_qwen3) | 121,249 | +0.04% |  | 100 | 0% |  | 3,248 | 0% | 0% |
| [Load/html_autoescape](#loadhtml_autoescape) | 188,615 | -0.01% |  | 43 | 0% |  | 12,688 | 0% | -48.59% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,210,689 | +0.08% |  | 674 | 0% |  | 45,408 | 0% | +12.44% |
| [Load/inheritance](#loadinheritance) | 21,939 | 0% | -80.78% | 7 | 0% | -93.46% | 1,360 | 0% | -52.65% |
| [Render/inheritance](#renderinheritance) | 137,750 | 0% | -81.58% | 35 | 0% | -94.37% | 7,384 | 0% | +22.74% |
| [Load/int_arith](#loadint_arith) | 65,009 | 0% |  | 20 | 0% |  | 3,888 | 0% | +0.83% |
| [Render/int_arith](#renderint_arith) | 828,243 | 0% |  | 12 | 0% |  | 3,272 | 0% | -0.49% |
| [Module/keep](#modulekeep) | 7,128 | 0% |  | 7 | 0% |  |  |  |  |
| [Module/keep_written](#modulekeep_written) | 29,087 | +0.02% |  | 11 | 0% |  |  |  |  |
| [Load/large_static](#loadlarge_static) | 147,742 | 0% | -83.32% | 14 | 0% | -96.82% | 43,136 | 0% | -19.20% |
| [Render/large_static](#renderlarge_static) | 20,740 | 0% | -72.40% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [RenderSink/large_static](#rendersinklarge_static) | 12,727 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/large_static](#rendertolarge_static) | 20,303 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/macros](#loadmacros) | 54,954 | -0.03% | -75.89% | 27 | 0% | -85.48% | 4,408 | 0% | -29.09% |
| [Render/macros](#rendermacros) | 659,503 | 0% | -74.27% | 308 | 0% | -88.71% | 20,560 | 0% | +39.67% |
| [Load/many_tags](#loadmany_tags) | 8,714,972 | 0% | -80.27% | 951 | 0% | -97.22% | 438,176 | 0% | -58.48% |
| [Render/many_tags](#rendermany_tags) | 729,970 | 0% | -64.96% | 2 | 0% | -90.48% | 5,648 | 0% | -10.52% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 45,097 | 0% | -81.02% | 19 | 0% | -89.67% | 3,376 | 0% | -44.33% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,203,070 | 0% | -57.89% | 33 | 0% | -99.19% | 381,536 | 0% | +9.95% |
| [RenderSink/mitsuhiko_table](#rendersinkmitsuhiko_table) | 5,130,335 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table](#rendertomitsuhiko_table) | 5,202,623 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 52,784 | 0% |  | 35 | 0% |  | 5,704 | 0% | -30.98% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,549,116 | 0% |  | 33 | 0% |  | 1,414,928 | 0% | +2.46% |
| [RenderSink/mitsuhiko_table_wide](#rendersinkmitsuhiko_table_wide) | 5,372,418 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table_wide](#rendertomitsuhiko_table_wide) | 5,548,683 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/plain_text](#loadplain_text) | 3,752 | 0% | -61.22% | 4 | 0% | -77.78% | 688 | 0% | -38.57% |
| [Render/plain_text](#renderplain_text) | 1,048 | 0% | -84.57% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/plain_text](#rendersinkplain_text) | 749 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/plain_text](#rendertoplain_text) | 849 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/prompt_budget](#loadprompt_budget) | 34,462 | -0.02% |  | 18 | 0% |  | 2,240 | 0% | +0.72% |
| [Render/prompt_budget](#renderprompt_budget) | 1,319,640 | 0% |  | 76 | 0% |  | 36,416 | 0% | 0% |
| [Load/strings](#loadstrings) | 68,987 | +0.07% | -76.36% | 30 | 0% | -86.61% | 4,360 | 0% | -51.77% |
| [Render/strings](#renderstrings) | 583,484 | +0.02% | -72.18% | 344 | 0% | -83.73% | 9,088 | 0% | -49.31% |
| [Load/substitute](#loadsubstitute) | 7,531 | 0% | -74.78% | 6 | 0% | -83.78% | 848 | 0% | -44.21% |
| [Render/substitute](#rendersubstitute) | 2,282 | 0% | -74.11% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/substitute](#rendersinksubstitute) | 1,986 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/substitute](#rendertosubstitute) | 2,087 | 0% |  | 0 |  |  | 0 |  |  |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 739 | -1.60% | 754 | +0.27% | 0 |  | 7,682 | +3.43% |
| Render/chat_llama | 1,192 | +1.10% | 510 | -1.35% | 0 |  | 4,141 | +1.79% |
| Load/chat_mistral | 1,102 | -0.54% | 1,083 | +0.19% | 0 |  | 8,913 | +2.08% |
| Render/chat_mistral | 1,918 | +0.89% | 588 | +3.16% | 0 |  | 10,624 | +3.64% |
| Load/chat_qwen | 581 | +1.75% | 631 | +1.12% | 0 |  | 4,852 | +8.13% |
| Render/chat_qwen | 1,069 | -0.19% | 387 | -3.49% | 0 |  | 3,541 | +1.08% |
| Load/config_file | 231 | -0.86% | 182 | -6.19% | 0 |  | 2,859 | -0.69% |
| Render/config_file | 6,322 | +0.70% | 1,593 | +2.25% | 0 |  | 42,314 | -2.44% |
| Load/dict_ops | 54 | -10.00% | 29 | -3.33% | 0 |  | 1,360 | +3.26% |
| Render/dict_ops | 679 | -0.29% | 425 | +1.43% | 0 |  | 912 | +1.11% |
| Load/expressions | 42 | +7.69% | 33 | +26.92% | 0 |  | 1,322 | +4.01% |
| Render/expressions | 3 | -25.00% | 1 | 0% | 0 |  | 348 | +14.10% |
| Load/filters | 100 | -4.76% | 42 | -22.22% | 0 |  | 1,724 | +3.73% |
| Render/filters | 8 | -69.23% | 3 | -57.14% | 0 |  | 777 | +1.30% |
| Load/for_filter_if | 38 | +5.56% | 16 | -20.00% | 0 |  | 1,134 | +4.32% |
| Render/for_filter_if | 1,737 | -0.34% | 131 | -0.76% | 0 |  | 215 | +18.78% |
| Load/for_loop_vars | 52 | -8.77% | 19 | -17.39% | 0 |  | 1,179 | -3.83% |
| Render/for_loop_vars | 11 | +10.00% | 1 | 0% | 0 |  | 179 | +16.23% |
| Load/for_range | 3 | 0% | 4 | 0% | 0 |  | 765 | -0.13% |
| Render/for_range | 1 | 0% | 1 |  | 0 |  | 162 | +14.89% |
| Load/hf_deepseek_v31 | 1,016 | +2.32% | 990 | +0.92% | 0 |  | 6,528 | -2.19% |
| Render/hf_deepseek_v31 | 351 | -2.23% | 72 | -7.69% | 0 |  | 1,469 | +1.38% |
| Load/hf_gpt_oss | 4,676 | +0.30% | 3,392 | +0.06% | 0 |  | 23,645 | +1.44% |
| Render/hf_gpt_oss | 1,232 | +0.49% | 392 | 0% | 0 |  | 4,625 | -0.09% |
| Load/hf_mistral_small | 1,885 | -1.21% | 1,632 | -0.61% | 0 |  | 11,382 | +1.72% |
| Render/hf_mistral_small | 969 | +1.79% | 302 | +1.00% | 0 |  | 3,802 | +1.88% |
| Load/hf_qwen3 | 1,534 | +1.25% | 1,344 | +0.37% | 0 |  | 9,483 | +5.39% |
| Render/hf_qwen3 | 689 | -0.58% | 227 | +1.34% | 0 |  | 2,753 | +3.11% |
| Load/html_autoescape | 513 | +1.58% | 536 | +0.56% | 0 |  | 4,530 | +7.68% |
| Render/html_autoescape | 4,087 | +2.77% | 1,032 | -4.80% | 0 |  | 34,982 | -7.55% |
| Load/inheritance | 8 | +60.00% | 4 | +33.33% | 0 |  | 723 | +2.84% |
| Render/inheritance | 611 | +1.33% | 106 | -1.85% | 0 |  | 985 | +0.51% |
| Load/int_arith | 82 | -3.53% | 52 | -1.89% | 0 |  | 2,191 | +9.50% |
| Render/int_arith | 15 | -6.25% | 6 | +20.00% | 0 |  | 776 | +1.84% |
| Module/keep | 5 | +400.00% | 3 | +50.00% | 0 |  | 403 | -3.59% |
| Module/keep_written | 1,137 | 0% | 559 | 0% | 0 |  | 739 | 0% |
| Load/large_static | 1,924 | -0.16% | 904 | 0% | 0 |  | 321 | +4.22% |
| Render/large_static | 708 | 0% | 568 | 0% | 0 |  | 30 | +500.00% |
| RenderSink/large_static | 1 | 0% | 0 |  | 0 |  | 5 | +66.67% |
| RenderTo/large_static | 684 | 0% | 572 | 0% | 0 |  | 7 | +75.00% |
| Load/macros | 94 | -7.84% | 70 | -6.67% | 0 |  | 1,770 | +1.09% |
| Render/macros | 750 | -0.13% | 233 | 0% | 0 |  | 609 | -3.64% |
| Load/many_tags | 27,874 | -0.13% | 22,561 | -0.10% | 0 |  | 243,685 | +0.43% |
| Render/many_tags | 6,252 | +0.06% | 869 | +0.12% | 0 |  | 145 | +2.11% |
| Load/mitsuhiko_table | 46 | -16.36% | 59 | -1.67% | 0 |  | 850 | +1.55% |
| Render/mitsuhiko_table | 10,057 | -0.16% | 6,278 | -0.08% | 0 |  | 376 | -7.16% |
| RenderSink/mitsuhiko_table | 9,981 | -0.13% | 1,214 | -0.33% | 0 |  | 310 | -5.78% |
| RenderTo/mitsuhiko_table | 10,054 | -0.10% | 6,293 | -0.03% | 0 |  | 354 | -8.05% |
| Load/mitsuhiko_table_wide | 73 | +19.67% | 78 | -3.70% | 0 |  | 1,083 | +6.59% |
| Render/mitsuhiko_table_wide | 10,222 | -0.12% | 22,662 | -0.02% | 0 |  | 388 | -11.01% |
| RenderSink/mitsuhiko_table_wide | 10,058 | -0.14% | 1,287 | -0.23% | 0 |  | 326 | -8.17% |
| RenderTo/mitsuhiko_table_wide | 10,239 | -0.10% | 22,670 | -0.02% | 0 |  | 365 | -13.10% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 7 | +16.67% |
| Render/plain_text | 1 | 0% | 0 |  | 0 |  | 6 | 0% |
| RenderSink/plain_text | 1 |  | 0 |  | 0 |  | 5 | +25.00% |
| RenderTo/plain_text | 1 | 0% | 0 |  | 0 |  | 6 | +20.00% |
| Load/prompt_budget | 84 | -7.69% | 45 | -4.26% | 0 |  | 1,397 | -0.57% |
| Render/prompt_budget | 4,816 | +0.19% | 544 | -0.37% | 0 |  | 927 | -11.04% |
| Load/strings | 149 | -6.29% | 107 | 0% | 0 |  | 2,770 | -0.93% |
| Render/strings | 424 | +0.47% | 483 | -0.21% | 0 |  | 1,601 | -2.02% |
| Load/substitute | 6 | +200.00% | 3 | 0% | 0 |  | 205 | +0.49% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 6 | +50.00% |
| RenderSink/substitute | 0 |  | 0 |  | 0 |  | 5 | +66.67% |
| RenderTo/substitute | 0 |  | 0 |  | 0 |  | 5 | +25.00% |

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
