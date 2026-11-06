# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (188 so far, latest [821a9fc](https://github.com/jinja2cpp/Jinja2Cpp/commit/821a9fc3252e9606727da9081bd54d9dac03e7ad) on 2026-11-06). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 345,284 | 0% |  | 55 | 0% |  | 18,480 | 0% | -52.71% |
| [Render/chat_llama](#renderchat_llama) | 294,787 | +0.05% |  | 225 | 0% |  | 11,632 | 0% | -19.00% |
| [Load/chat_mistral](#loadchat_mistral) | 393,812 | 0% |  | 52 | 0% |  | 20,888 | 0% | -53.22% |
| [Render/chat_mistral](#renderchat_mistral) | 499,204 | +0.06% |  | 402 | 0% |  | 9,968 | 0% | -24.53% |
| [Load/chat_qwen](#loadchat_qwen) | 266,492 | 0% |  | 44 | 0% |  | 14,416 | 0% | -47.25% |
| [Render/chat_qwen](#renderchat_qwen) | 276,949 | +0.05% |  | 212 | 0% |  | 8,304 | 0% | -2.35% |
| [Load/config_file](#loadconfig_file) | 97,039 | 0% |  | 25 | 0% |  | 5,464 | 0% | -48.26% |
| [Render/config_file](#renderconfig_file) | 1,268,683 | +0.08% |  | 908 | 0% |  | 24,440 | 0% | +2.65% |
| [Load/dict_ops](#loaddict_ops) | 33,313 | 0% | -77.46% | 9 | 0% | -93.13% | 2,176 | 0% | -48.87% |
| [Render/dict_ops](#renderdict_ops) | 293,156 | +0.03% | -57.57% | 112 | 0% | -75.11% | 24,408 | 0% | -29.65% |
| [Load/expressions](#loadexpressions) | 45,015 | 0% | -78.16% | 16 | 0% | -89.04% | 2,728 | 0% | -55.54% |
| [Render/expressions](#renderexpressions) | 426,198 | -0.01% | -52.97% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 68,966 | 0% | -74.87% | 27 | 0% | -88.16% | 3,848 | 0% | -66.29% |
| [Render/filters](#renderfilters) | 30,426 | +0.06% | -65.69% | 26 | 0% | -64.86% | 1,080 | 0% | -78.01% |
| [Load/for_filter_if](#loadfor_filter_if) | 30,017 | 0% | -77.83% | 7 | 0% | -94.31% | 1,904 | 0% | -44.78% |
| [Render/for_filter_if](#renderfor_filter_if) | 274,076 | +0.07% | -68.33% | 4 | 0% | -98.23% | 16,560 | 0% | +354.95% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 40,311 | 0% | -77.69% | 10 | 0% | -93.63% | 2,368 | 0% | -47.61% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 180,541 | 0% | -83.17% | 2 | 0% | -99.62% | 1,568 | 0% | -43.35% |
| [Load/for_range](#loadfor_range) | 13,634 | 0% | -75.95% | 7 | 0% | -87.50% | 1,088 | 0% | -45.60% |
| [Render/for_range](#renderfor_range) | 38,352 | -0.07% | -58.41% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/hf_deepseek_v31](#loadhf_deepseek_v31) | 388,220 | 0% |  | 55 | 0% |  | 21,976 | 0% | +0.73% |
| [Render/hf_deepseek_v31](#renderhf_deepseek_v31) | 90,509 | -0.05% |  | 43 | 0% |  | 2,480 | 0% | 0% |
| [Load/hf_gpt_oss](#loadhf_gpt_oss) | 1,549,425 | +0.01% |  | 287 | 0% |  | 74,640 | 0% | +0.56% |
| [Render/hf_gpt_oss](#renderhf_gpt_oss) | 170,638 | +0.08% |  | 155 | 0% |  | 5,096 | 0% | +3.24% |
| [Load/hf_mistral_small](#loadhf_mistral_small) | 598,937 | 0% |  | 88 | 0% |  | 33,048 | 0% | +0.34% |
| [Render/hf_mistral_small](#renderhf_mistral_small) | 144,711 | +0.06% |  | 105 | 0% |  | 6,120 | 0% | -0.26% |
| [Load/hf_qwen3](#loadhf_qwen3) | 485,773 | 0% |  | 82 | 0% |  | 25,560 | 0% | +0.31% |
| [Render/hf_qwen3](#renderhf_qwen3) | 119,879 | +0.07% |  | 100 | 0% |  | 3,248 | 0% | 0% |
| [Load/html_autoescape](#loadhtml_autoescape) | 193,829 | +0.01% |  | 43 | 0% |  | 12,640 | 0% | -48.78% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,208,953 | +0.02% |  | 674 | 0% |  | 45,408 | 0% | +12.44% |
| [Load/inheritance](#loadinheritance) | 23,039 | 0% | -79.81% | 7 | 0% | -93.46% | 1,344 | 0% | -53.20% |
| [Render/inheritance](#renderinheritance) | 137,603 | +0.03% | -81.60% | 35 | 0% | -94.37% | 7,384 | 0% | +22.74% |
| [Load/int_arith](#loadint_arith) | 65,931 | 0% |  | 20 | 0% |  | 3,872 | 0% | +0.41% |
| [Render/int_arith](#renderint_arith) | 824,292 | +0.02% |  | 12 | 0% |  | 3,272 | 0% | -0.49% |
| [Module/keep](#modulekeep) | 6,996 | +0.06% |  | 7 | 0% |  |  |  |  |
| [Module/keep_written](#modulekeep_written) | 36,405 | +0.05% |  | 11 | 0% |  |  |  |  |
| [Load/large_static](#loadlarge_static) | 154,694 | 0% | -82.54% | 14 | 0% | -96.82% | 43,120 | 0% | -19.23% |
| [Render/large_static](#renderlarge_static) | 20,709 | 0% | -72.44% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [RenderSink/large_static](#rendersinklarge_static) | 12,699 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/large_static](#rendertolarge_static) | 20,272 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/macros](#loadmacros) | 56,326 | 0% | -75.29% | 27 | 0% | -85.48% | 4,392 | 0% | -29.34% |
| [Render/macros](#rendermacros) | 656,537 | +0.02% | -74.38% | 308 | 0% | -88.71% | 20,560 | 0% | +39.67% |
| [Load/many_tags](#loadmany_tags) | 9,007,376 | 0% | -79.61% | 951 | 0% | -97.22% | 438,160 | 0% | -58.48% |
| [Render/many_tags](#rendermany_tags) | 728,682 | +0.20% | -65.02% | 2 | 0% | -90.48% | 5,648 | 0% | -10.52% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 46,562 | 0% | -80.40% | 19 | 0% | -89.67% | 3,360 | 0% | -44.59% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,196,673 | +0.02% | -57.94% | 33 | 0% | -99.19% | 381,536 | 0% | +9.95% |
| [RenderSink/mitsuhiko_table](#rendersinkmitsuhiko_table) | 5,123,866 | +0.02% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table](#rendertomitsuhiko_table) | 5,196,177 | +0.02% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 54,826 | +0.01% |  | 35 | 0% |  | 5,688 | 0% | -31.17% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,542,838 | +0.02% |  | 33 | 0% |  | 1,414,928 | 0% | +2.46% |
| [RenderSink/mitsuhiko_table_wide](#rendersinkmitsuhiko_table_wide) | 5,366,127 | +0.02% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table_wide](#rendertomitsuhiko_table_wide) | 5,542,390 | +0.02% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/plain_text](#loadplain_text) | 3,713 | 0% | -61.63% | 4 | 0% | -77.78% | 672 | 0% | -40.00% |
| [Render/plain_text](#renderplain_text) | 1,019 | 0% | -85.00% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/plain_text](#rendersinkplain_text) | 721 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/plain_text](#rendertoplain_text) | 820 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/prompt_budget](#loadprompt_budget) | 35,142 | 0% |  | 18 | 0% |  | 2,224 | 0% | 0% |
| [Render/prompt_budget](#renderprompt_budget) | 1,314,517 | +0.02% |  | 76 | 0% |  | 36,416 | 0% | 0% |
| [Load/strings](#loadstrings) | 70,329 | 0% | -75.90% | 30 | 0% | -86.61% | 4,344 | 0% | -51.95% |
| [Render/strings](#renderstrings) | 578,990 | +0.05% | -72.40% | 344 | 0% | -83.73% | 9,088 | 0% | -49.31% |
| [Load/substitute](#loadsubstitute) | 7,746 | 0% | -74.06% | 6 | 0% | -83.78% | 832 | 0% | -45.26% |
| [Render/substitute](#rendersubstitute) | 2,252 | 0% | -74.45% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/substitute](#rendersinksubstitute) | 1,958 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/substitute](#rendertosubstitute) | 2,057 | 0% |  | 0 |  |  | 0 |  |  |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 738 | +2.36% | 747 | -0.13% | 0 |  | 7,276 | -2.95% |
| Render/chat_llama | 1,156 | -2.53% | 528 | -0.56% | 0 |  | 4,321 | +3.85% |
| Load/chat_mistral | 1,024 | +0.59% | 1,043 | -0.48% | 0 |  | 8,121 | -0.27% |
| Render/chat_mistral | 2,009 | +0.95% | 575 | +0.52% | 0 |  | 10,038 | +1.87% |
| Load/chat_qwen | 496 | -0.60% | 593 | +1.02% | 0 |  | 4,313 | -0.42% |
| Render/chat_qwen | 1,079 | -1.19% | 402 | -0.74% | 0 |  | 3,390 | -1.99% |
| Load/config_file | 201 | +1.52% | 172 | +1.18% | 0 |  | 2,607 | -2.32% |
| Render/config_file | 6,392 | +2.47% | 1,549 | +1.84% | 0 |  | 41,299 | +3.83% |
| Load/dict_ops | 29 | -12.12% | 20 | -13.04% | 0 |  | 1,319 | -2.15% |
| Render/dict_ops | 661 | -1.20% | 421 | +0.24% | 0 |  | 869 | -1.03% |
| Load/expressions | 9 | -35.71% | 6 | -14.29% | 0 |  | 1,283 | +0.08% |
| Render/expressions | 12 | -42.86% | 6 | 0% | 0 |  | 282 | +10.16% |
| Load/filters | 75 | -2.60% | 51 | -3.77% | 0 |  | 1,416 | -5.54% |
| Render/filters | 24 | -11.11% | 5 | 0% | 0 |  | 774 | -1.65% |
| Load/for_filter_if | 28 | +33.33% | 17 | +13.33% | 0 |  | 1,053 | +2.93% |
| Render/for_filter_if | 1,806 | -0.11% | 97 | -1.02% | 0 |  | 206 | +42.07% |
| Load/for_loop_vars | 26 | -7.14% | 13 | 0% | 0 |  | 1,104 | +0.64% |
| Render/for_loop_vars | 8 | +14.29% | 3 | +200.00% | 0 |  | 201 | +37.67% |
| Load/for_range | 3 | 0% | 4 | 0% | 0 |  | 750 | +0.40% |
| Render/for_range | 1 | -85.71% | 0 |  | 0 |  | 150 | +31.58% |
| Load/hf_deepseek_v31 | 975 | +0.21% | 967 | +0.10% | 0 |  | 6,116 | +2.22% |
| Render/hf_deepseek_v31 | 355 | 0% | 81 | +3.85% | 0 |  | 1,430 | +5.38% |
| Load/hf_gpt_oss | 4,619 | -0.22% | 3,293 | 0% | 0 |  | 21,236 | -1.20% |
| Render/hf_gpt_oss | 1,207 | -0.90% | 375 | +0.27% | 0 |  | 4,508 | +0.83% |
| Load/hf_mistral_small | 1,807 | +0.22% | 1,581 | 0% | 0 |  | 10,410 | -0.34% |
| Render/hf_mistral_small | 962 | +0.52% | 314 | -1.26% | 0 |  | 3,849 | +2.72% |
| Load/hf_qwen3 | 1,411 | +0.71% | 1,236 | -0.32% | 0 |  | 8,433 | -1.23% |
| Render/hf_qwen3 | 666 | -0.75% | 217 | 0% | 0 |  | 2,543 | +2.17% |
| Load/html_autoescape | 462 | +1.32% | 514 | +0.19% | 0 |  | 3,645 | -1.94% |
| Render/html_autoescape | 4,230 | +3.57% | 1,230 | -9.56% | 0 |  | 35,424 | -2.52% |
| Load/inheritance | 2 | -33.33% | 4 | 0% | 0 |  | 667 | -3.05% |
| Render/inheritance | 586 | -1.35% | 96 | 0% | 0 |  | 935 | +1.08% |
| Load/int_arith | 69 | -16.87% | 38 | -37.70% | 0 |  | 1,975 | -3.04% |
| Render/int_arith | 27 | -15.62% | 15 | +66.67% | 0 |  | 753 | +6.96% |
| Module/keep | 2 | 0% | 2 | 0% | 0 |  | 412 | +18.39% |
| Module/keep_written | 2,931 | +0.07% | 561 | 0% | 0 |  | 722 | +0.84% |
| Load/large_static | 1,927 | 0% | 908 | 0% | 0 |  | 262 | +11.49% |
| Render/large_static | 710 | 0% | 568 | 0% | 0 |  | 6 | +50.00% |
| RenderSink/large_static | 0 |  | 0 |  | 0 |  | 3 | 0% |
| RenderTo/large_static | 688 | 0% | 570 | 0% | 0 |  | 3 | -25.00% |
| Load/macros | 70 | -4.11% | 54 | -6.90% | 0 |  | 1,717 | +0.41% |
| Render/macros | 757 | -0.53% | 213 | -2.29% | 0 |  | 602 | -57.43% |
| Load/many_tags | 27,810 | 0% | 22,386 | -0.05% | 0 |  | 221,583 | +1.65% |
| Render/many_tags | 6,219 | -0.05% | 862 | -0.58% | 0 |  | 166 | +22.96% |
| Load/mitsuhiko_table | 27 | -22.86% | 31 | -11.43% | 0 |  | 825 | -7.61% |
| Render/mitsuhiko_table | 10,019 | -0.05% | 6,280 | 0% | 0 |  | 396 | +17.51% |
| RenderSink/mitsuhiko_table | 9,954 | +0.14% | 1,183 | +0.17% | 0 |  | 322 | +8.78% |
| RenderTo/mitsuhiko_table | 10,026 | +0.16% | 6,280 | +0.06% | 0 |  | 374 | +20.65% |
| Load/mitsuhiko_table_wide | 79 | +1.28% | 68 | -2.86% | 0 |  | 959 | -0.31% |
| Render/mitsuhiko_table_wide | 10,238 | -0.04% | 22,676 | +0.04% | 0 |  | 380 | +9.20% |
| RenderSink/mitsuhiko_table_wide | 10,062 | +0.05% | 1,289 | -0.46% | 0 |  | 319 | +26.59% |
| RenderTo/mitsuhiko_table_wide | 10,227 | +0.06% | 22,673 | -0.02% | 0 |  | 356 | +14.10% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 29 | +107.14% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 8 | +100.00% |
| RenderSink/plain_text | 0 |  | 0 |  | 0 |  | 6 | +50.00% |
| RenderTo/plain_text | 0 |  | 0 |  | 0 |  | 7 | +75.00% |
| Load/prompt_budget | 65 | -5.80% | 39 | -13.33% | 0 |  | 1,354 | -1.38% |
| Render/prompt_budget | 4,786 | -0.06% | 526 | -0.38% | 0 |  | 920 | +6.98% |
| Load/strings | 110 | -9.84% | 78 | -10.34% | 0 |  | 2,673 | -2.52% |
| Render/strings | 409 | -1.21% | 477 | +0.42% | 0 |  | 1,563 | +1.49% |
| Load/substitute | 1 | 0% | 4 | 0% | 0 |  | 147 | -8.13% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 4 | 0% |
| RenderSink/substitute | 0 |  | 0 |  | 0 |  | 3 | 0% |
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
