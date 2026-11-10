# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (196 so far, latest [786cfdf](https://github.com/jinja2cpp/Jinja2Cpp/commit/786cfdf559a6ef5622f0054cd9e93ad2bce65f87) on 2026-11-10). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 345,391 | +0.01% |  | 55 | 0% |  | 18,480 | 0% | -52.71% |
| [Render/chat_llama](#renderchat_llama) | 296,229 | +0.01% |  | 225 | 0% |  | 11,616 | 0% | -19.11% |
| [Load/chat_mistral](#loadchat_mistral) | 394,415 | -0.01% |  | 52 | 0% |  | 20,888 | 0% | -53.22% |
| [Render/chat_mistral](#renderchat_mistral) | 500,298 | +0.01% |  | 402 | 0% |  | 9,984 | 0% | -24.41% |
| [Load/chat_qwen](#loadchat_qwen) | 266,724 | +0.01% |  | 44 | 0% |  | 14,416 | 0% | -47.25% |
| [Render/chat_qwen](#renderchat_qwen) | 277,431 | +0.01% |  | 212 | 0% |  | 8,304 | 0% | -2.35% |
| [Load/config_file](#loadconfig_file) | 97,274 | -0.01% |  | 25 | 0% |  | 5,464 | 0% | -48.26% |
| [Render/config_file](#renderconfig_file) | 1,275,324 | +0.08% |  | 908 | 0% |  | 24,440 | 0% | +2.65% |
| [Load/dict_ops](#loaddict_ops) | 33,368 | +0.04% | -77.42% | 9 | 0% | -93.13% | 2,176 | 0% | -48.87% |
| [Render/dict_ops](#renderdict_ops) | 293,079 | 0% | -57.58% | 112 | 0% | -75.11% | 24,408 | 0% | -29.65% |
| [Load/expressions](#loadexpressions) | 45,046 | 0% | -78.15% | 16 | 0% | -89.04% | 2,728 | 0% | -55.54% |
| [Render/expressions](#renderexpressions) | 426,197 | 0% | -52.97% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 69,024 | -0.03% | -74.85% | 27 | 0% | -88.16% | 3,848 | 0% | -66.29% |
| [Render/filters](#renderfilters) | 30,465 | 0% | -65.64% | 26 | 0% | -64.86% | 1,080 | 0% | -78.01% |
| [Load/for_filter_if](#loadfor_filter_if) | 30,047 | 0% | -77.81% | 7 | 0% | -94.31% | 1,904 | 0% | -44.78% |
| [Render/for_filter_if](#renderfor_filter_if) | 274,384 | 0% | -68.30% | 4 | 0% | -98.23% | 16,560 | 0% | +354.95% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 40,420 | +0.02% | -77.63% | 10 | 0% | -93.63% | 2,368 | 0% | -47.61% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 180,551 | 0% | -83.17% | 2 | 0% | -99.62% | 1,568 | 0% | -43.35% |
| [Load/for_range](#loadfor_range) | 13,666 | 0% | -75.89% | 7 | 0% | -87.50% | 1,088 | 0% | -45.60% |
| [Render/for_range](#renderfor_range) | 38,351 | +0.02% | -58.41% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/hf_deepseek_v31](#loadhf_deepseek_v31) | 387,524 | 0% |  | 55 | 0% |  | 21,976 | 0% | +0.73% |
| [Render/hf_deepseek_v31](#renderhf_deepseek_v31) | 90,901 | 0% |  | 43 | 0% |  | 2,480 | 0% | 0% |
| [Load/hf_gpt_oss](#loadhf_gpt_oss) | 1,549,723 | 0% |  | 287 | 0% |  | 74,672 | 0% | +0.60% |
| [Render/hf_gpt_oss](#renderhf_gpt_oss) | 171,200 | +0.02% |  | 155 | 0% |  | 5,080 | 0% | +2.92% |
| [Load/hf_mistral_small](#loadhf_mistral_small) | 599,298 | -0.01% |  | 88 | 0% |  | 33,048 | 0% | +0.34% |
| [Render/hf_mistral_small](#renderhf_mistral_small) | 144,923 | 0% |  | 105 | 0% |  | 6,136 | 0% | 0% |
| [Load/hf_qwen3](#loadhf_qwen3) | 486,388 | +0.01% |  | 82 | 0% |  | 25,560 | 0% | +0.31% |
| [Render/hf_qwen3](#renderhf_qwen3) | 119,953 | +0.02% |  | 100 | 0% |  | 3,248 | 0% | 0% |
| [Load/html_autoescape](#loadhtml_autoescape) | 194,086 | +0.03% |  | 43 | 0% |  | 12,640 | 0% | -48.78% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,207,219 | +0.03% |  | 674 | 0% |  | 45,408 | 0% | +12.44% |
| [Load/inheritance](#loadinheritance) | 23,096 | 0% | -79.76% | 7 | 0% | -93.46% | 1,344 | 0% | -53.20% |
| [Render/inheritance](#renderinheritance) | 137,427 | 0% | -81.62% | 35 | 0% | -94.37% | 7,384 | 0% | +22.74% |
| [Load/int_arith](#loadint_arith) | 66,021 | +0.02% |  | 20 | 0% |  | 3,872 | 0% | +0.41% |
| [Render/int_arith](#renderint_arith) | 825,544 | -0.02% |  | 12 | 0% |  | 3,272 | 0% | -0.49% |
| [Module/keep](#modulekeep) | 7,070 | 0% |  | 7 | 0% |  |  |  |  |
| [Module/keep_written](#modulekeep_written) | 36,529 | 0% |  | 11 | 0% |  |  |  |  |
| [Load/large_static](#loadlarge_static) | 154,778 | 0% | -82.53% | 14 | 0% | -96.82% | 43,120 | 0% | -19.23% |
| [Render/large_static](#renderlarge_static) | 20,664 | 0% | -72.50% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [RenderSink/large_static](#rendersinklarge_static) | 12,702 | +0.01% |  | 0 |  |  | 0 |  |  |
| [RenderTo/large_static](#rendertolarge_static) | 20,275 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/macros](#loadmacros) | 56,414 | +0.01% | -75.25% | 27 | 0% | -85.48% | 4,392 | 0% | -29.34% |
| [Render/macros](#rendermacros) | 657,499 | +0.18% | -74.35% | 308 | 0% | -88.71% | 20,560 | 0% | +39.67% |
| [Load/many_tags](#loadmany_tags) | 9,019,220 | 0% | -79.58% | 951 | 0% | -97.22% | 438,128 | 0% | -58.48% |
| [Render/many_tags](#rendermany_tags) | 727,198 | 0% | -65.09% | 2 | 0% | -90.48% | 5,648 | 0% | -10.52% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 46,621 | 0% | -80.38% | 19 | 0% | -89.67% | 3,360 | 0% | -44.59% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,199,644 | +0.10% | -57.92% | 33 | 0% | -99.19% | 381,536 | 0% | +9.95% |
| [RenderSink/mitsuhiko_table](#rendersinkmitsuhiko_table) | 5,126,839 | +0.10% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table](#rendertomitsuhiko_table) | 5,199,150 | +0.10% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 54,830 | 0% |  | 35 | 0% |  | 5,688 | 0% | -31.17% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,545,812 | +0.09% |  | 33 | 0% |  | 1,414,928 | 0% | +2.46% |
| [RenderSink/mitsuhiko_table_wide](#rendersinkmitsuhiko_table_wide) | 5,369,103 | +0.09% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table_wide](#rendertomitsuhiko_table_wide) | 5,545,364 | +0.09% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/plain_text](#loadplain_text) | 3,733 | 0% | -61.42% | 4 | 0% | -77.78% | 672 | 0% | -40.00% |
| [Render/plain_text](#renderplain_text) | 1,022 | +0.10% | -84.96% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/plain_text](#rendersinkplain_text) | 724 | +0.14% |  | 0 |  |  | 0 |  |  |
| [RenderTo/plain_text](#rendertoplain_text) | 823 | +0.12% |  | 0 |  |  | 0 |  |  |
| [Load/prompt_budget](#loadprompt_budget) | 35,155 | 0% |  | 18 | 0% |  | 2,224 | 0% | 0% |
| [Render/prompt_budget](#renderprompt_budget) | 1,315,570 | 0% |  | 76 | 0% |  | 36,416 | 0% | 0% |
| [Load/strings](#loadstrings) | 70,419 | -0.05% | -75.87% | 30 | 0% | -86.61% | 4,344 | 0% | -51.95% |
| [Render/strings](#renderstrings) | 579,933 | -0.02% | -72.35% | 344 | 0% | -83.73% | 9,088 | 0% | -49.31% |
| [Load/substitute](#loadsubstitute) | 7,768 | 0% | -73.98% | 6 | 0% | -83.78% | 832 | 0% | -45.26% |
| [Render/substitute](#rendersubstitute) | 2,255 | +0.04% | -74.41% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/substitute](#rendersinksubstitute) | 1,961 | +0.05% |  | 0 |  |  | 0 |  |  |
| [RenderTo/substitute](#rendertosubstitute) | 2,060 | +0.05% |  | 0 |  |  | 0 |  |  |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 706 | -2.35% | 738 | -0.67% | 0 |  | 7,633 | -3.07% |
| Render/chat_llama | 1,189 | -0.83% | 509 | -1.74% | 0 |  | 3,795 | -3.95% |
| Load/chat_mistral | 1,021 | -0.87% | 1,029 | -0.10% | 0 |  | 8,561 | -4.05% |
| Render/chat_mistral | 1,942 | +1.89% | 577 | -0.52% | 0 |  | 9,536 | -2.78% |
| Load/chat_qwen | 511 | -2.67% | 576 | -1.54% | 0 |  | 4,558 | -6.58% |
| Render/chat_qwen | 1,098 | +0.73% | 396 | +0.25% | 0 |  | 3,445 | +0.47% |
| Load/config_file | 214 | -3.17% | 181 | 0% | 0 |  | 2,777 | -1.98% |
| Render/config_file | 6,039 | -3.14% | 1,536 | -0.71% | 0 |  | 43,483 | +0.46% |
| Load/dict_ops | 33 | +10.00% | 22 | +10.00% | 0 |  | 1,370 | +1.41% |
| Render/dict_ops | 661 | -2.07% | 427 | -0.23% | 0 |  | 896 | +0.56% |
| Load/expressions | 20 | 0% | 14 | -17.65% | 0 |  | 1,340 | -2.19% |
| Render/expressions | 16 | 0% | 7 | +40.00% | 0 |  | 327 | +5.48% |
| Load/filters | 87 | +12.99% | 49 | -2.00% | 0 |  | 1,674 | -15.03% |
| Render/filters | 15 | +50.00% | 7 | +40.00% | 0 |  | 781 | +1.03% |
| Load/for_filter_if | 27 | -15.62% | 17 | 0% | 0 |  | 1,069 | -5.23% |
| Render/for_filter_if | 1,731 | +0.06% | 134 | +1.52% | 0 |  | 191 | +9.77% |
| Load/for_loop_vars | 35 | -20.45% | 16 | -30.43% | 0 |  | 1,158 | -1.53% |
| Render/for_loop_vars | 4 | -60.00% | 0 | -100.00% | 0 |  | 170 | +20.57% |
| Load/for_range | 4 | 0% | 4 | 0% | 0 |  | 777 | +1.04% |
| Render/for_range | 1 | -87.50% | 0 |  | 0 |  | 122 | -14.69% |
| Load/hf_deepseek_v31 | 983 | -1.40% | 963 | -1.03% | 0 |  | 6,284 | +2.97% |
| Render/hf_deepseek_v31 | 365 | +3.40% | 72 | -2.70% | 0 |  | 1,340 | -0.96% |
| Load/hf_gpt_oss | 4,581 | -0.99% | 3,268 | -0.06% | 0 |  | 22,879 | -1.07% |
| Render/hf_gpt_oss | 1,202 | +1.01% | 370 | -1.07% | 0 |  | 4,483 | +1.82% |
| Load/hf_mistral_small | 1,778 | -0.78% | 1,589 | -0.38% | 0 |  | 11,087 | -4.34% |
| Render/hf_mistral_small | 996 | +0.20% | 313 | -0.95% | 0 |  | 3,756 | -0.19% |
| Load/hf_qwen3 | 1,430 | -1.58% | 1,262 | 0% | 0 |  | 8,923 | -1.24% |
| Render/hf_qwen3 | 681 | +1.64% | 210 | -2.33% | 0 |  | 2,596 | +2.49% |
| Load/html_autoescape | 468 | -1.68% | 509 | +0.39% | 0 |  | 4,051 | -11.05% |
| Render/html_autoescape | 4,604 | -3.68% | 1,531 | +3.94% | 0 |  | 35,643 | -0.11% |
| Load/inheritance | 4 | 0% | 3 | 0% | 0 |  | 708 | +1.14% |
| Render/inheritance | 578 | -0.86% | 91 | +1.11% | 0 |  | 910 | -5.99% |
| Load/int_arith | 67 | +1.52% | 39 | +2.63% | 0 |  | 2,042 | -6.76% |
| Render/int_arith | 23 | +35.29% | 7 | +40.00% | 0 |  | 758 | +6.46% |
| Module/keep | 1 | -50.00% | 2 | 0% | 0 |  | 431 | +4.36% |
| Module/keep_written | 2,933 | -0.20% | 559 | 0% | 0 |  | 728 | +1.39% |
| Load/large_static | 1,931 | -0.05% | 906 | 0% | 0 |  | 323 | +16.19% |
| Render/large_static | 712 | 0% | 573 | 0% | 0 |  | 4 | -82.61% |
| RenderSink/large_static | 0 |  | 0 |  | 0 |  | 4 | 0% |
| RenderTo/large_static | 689 | 0% | 568 | 0% | 0 |  | 4 | -66.67% |
| Load/macros | 68 | -20.00% | 54 | -5.26% | 0 |  | 1,715 | -3.05% |
| Render/macros | 753 | +0.40% | 234 | -1.27% | 0 |  | 1,923 | +1.42% |
| Load/many_tags | 27,988 | -2.70% | 22,455 | -0.24% | 0 |  | 240,558 | -2.84% |
| Render/many_tags | 6,218 | 0% | 867 | -0.12% | 0 |  | 119 | -19.59% |
| Load/mitsuhiko_table | 36 | -5.26% | 39 | 0% | 0 |  | 909 | +2.60% |
| Render/mitsuhiko_table | 10,043 | +0.08% | 6,271 | -0.05% | 0 |  | 346 | -5.46% |
| RenderSink/mitsuhiko_table | 9,965 | +0.03% | 1,177 | -0.59% | 0 |  | 281 | -7.57% |
| RenderTo/mitsuhiko_table | 10,050 | +0.04% | 6,287 | +0.08% | 0 |  | 323 | -4.15% |
| Load/mitsuhiko_table_wide | 72 | -1.37% | 91 | +7.06% | 0 |  | 1,007 | -7.44% |
| Render/mitsuhiko_table_wide | 10,242 | +0.19% | 22,688 | +0.01% | 0 |  | 347 | -10.34% |
| RenderSink/mitsuhiko_table_wide | 10,065 | +0.11% | 1,302 | +0.31% | 0 |  | 289 | -6.17% |
| RenderTo/mitsuhiko_table_wide | 10,248 | +0.30% | 22,691 | +0.01% | 0 |  | 339 | -5.31% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 14 | -12.50% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 5 | 0% |
| RenderSink/plain_text | 0 |  | 0 |  | 0 |  | 3 | -25.00% |
| RenderTo/plain_text | 0 |  | 0 |  | 0 |  | 5 | +25.00% |
| Load/prompt_budget | 74 | +5.71% | 37 | +2.78% | 0 |  | 1,386 | -3.41% |
| Render/prompt_budget | 4,816 | -0.04% | 525 | +0.77% | 0 |  | 1,121 | +31.42% |
| Load/strings | 129 | -7.86% | 92 | +1.10% | 0 |  | 2,786 | -1.07% |
| Render/strings | 428 | +0.94% | 485 | -0.82% | 0 |  | 1,580 | -37.08% |
| Load/substitute | 2 | +100.00% | 3 | 0% | 0 |  | 173 | +15.33% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 5 | -77.27% |
| RenderSink/substitute | 0 |  | 0 |  | 0 |  | 4 | 0% |
| RenderTo/substitute | 0 |  | 0 |  | 0 |  | 4 | -69.23% |

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
