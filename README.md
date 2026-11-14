# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (223 so far, latest [1f489e5](https://github.com/jinja2cpp/Jinja2Cpp/commit/1f489e5cd48b42ffc1f44e00acd241be00e26e80) on 2026-11-14). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 335,407 | 0% |  | 55 | 0% |  | 18,480 | 0% | -52.71% |
| [Render/chat_llama](#renderchat_llama) | 295,755 | +0.09% |  | 225 | 0% |  | 11,616 | 0% | -19.11% |
| [Load/chat_mistral](#loadchat_mistral) | 385,087 | 0% |  | 52 | 0% |  | 20,904 | 0% | -53.18% |
| [Render/chat_mistral](#renderchat_mistral) | 502,022 | 0% |  | 402 | 0% |  | 9,984 | -0.48% | -24.41% |
| [Load/chat_qwen](#loadchat_qwen) | 261,172 | 0% |  | 44 | 0% |  | 14,432 | 0% | -47.19% |
| [Render/chat_qwen](#renderchat_qwen) | 278,779 | -0.02% |  | 212 | 0% |  | 8,304 | 0% | -2.35% |
| [Load/config_file](#loadconfig_file) | 94,253 | 0% |  | 25 | 0% |  | 5,480 | 0% | -48.11% |
| [Render/config_file](#renderconfig_file) | 1,277,853 | 0% |  | 908 | 0% |  | 24,552 | +0.46% | +3.12% |
| [Load/dict_ops](#loaddict_ops) | 32,235 | 0% | -78.19% | 9 | 0% | -93.13% | 2,192 | 0% | -48.50% |
| [Render/dict_ops](#renderdict_ops) | 296,383 | +0.25% | -57.10% | 112 | 0% | -75.11% | 24,408 | 0% | -29.65% |
| [Load/expressions](#loadexpressions) | 44,235 | 0% | -78.54% | 16 | 0% | -89.04% | 2,744 | 0% | -55.28% |
| [Render/expressions](#renderexpressions) | 427,017 | 0% | -52.88% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 67,515 | 0% | -75.40% | 27 | 0% | -88.16% | 3,864 | 0% | -66.15% |
| [Render/filters](#renderfilters) | 27,827 | 0% | -68.62% | 23 | 0% | -68.92% | 752 | 0% | -84.69% |
| [Load/for_filter_if](#loadfor_filter_if) | 29,057 | 0% | -78.54% | 7 | 0% | -94.31% | 1,920 | 0% | -44.32% |
| [Render/for_filter_if](#renderfor_filter_if) | 275,016 | 0% | -68.22% | 4 | 0% | -98.23% | 16,560 | 0% | +354.95% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 38,841 | 0% | -78.50% | 10 | 0% | -93.63% | 2,384 | 0% | -47.26% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 180,600 | 0% | -83.16% | 2 | 0% | -99.62% | 1,568 | 0% | -43.35% |
| [Load/for_range](#loadfor_range) | 13,259 | 0% | -76.61% | 7 | 0% | -87.50% | 1,104 | 0% | -44.80% |
| [Render/for_range](#renderfor_range) | 38,468 | +0.01% | -58.28% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/hf_deepseek_v31](#loadhf_deepseek_v31) | 382,862 | 0% |  | 55 | 0% |  | 21,992 | 0% | +0.81% |
| [Render/hf_deepseek_v31](#renderhf_deepseek_v31) | 93,250 | +0.01% |  | 43 | 0% |  | 2,480 | 0% | 0% |
| [Load/hf_gpt_oss](#loadhf_gpt_oss) | 1,522,407 | 0% |  | 287 | 0% |  | 74,688 | 0% | +0.63% |
| [Render/hf_gpt_oss](#renderhf_gpt_oss) | 173,198 | -0.25% |  | 155 | 0% |  | 5,144 | +1.26% | +4.21% |
| [Load/hf_mistral_small](#loadhf_mistral_small) | 591,652 | 0% |  | 88 | 0% |  | 33,064 | 0% | +0.39% |
| [Render/hf_mistral_small](#renderhf_mistral_small) | 142,868 | +0.01% |  | 99 | 0% |  | 6,120 | 0% | -0.26% |
| [Load/hf_qwen3](#loadhf_qwen3) | 475,801 | 0% |  | 82 | 0% |  | 25,576 | 0% | +0.38% |
| [Render/hf_qwen3](#renderhf_qwen3) | 121,449 | +0.01% |  | 100 | 0% |  | 3,248 | 0% | 0% |
| [Load/html_autoescape](#loadhtml_autoescape) | 188,619 | 0% |  | 43 | 0% |  | 12,688 | 0% | -48.59% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,211,506 | -0.01% |  | 674 | 0% |  | 45,488 | +0.18% | +12.64% |
| [Load/inheritance](#loadinheritance) | 21,939 | 0% | -80.78% | 7 | 0% | -93.46% | 1,360 | 0% | -52.65% |
| [Render/inheritance](#renderinheritance) | 138,249 | +0.22% | -81.51% | 35 | 0% | -94.37% | 7,416 | +0.43% | +23.27% |
| [Load/int_arith](#loadint_arith) | 65,009 | 0% |  | 20 | 0% |  | 3,888 | 0% | +0.83% |
| [Render/int_arith](#renderint_arith) | 831,485 | 0% |  | 12 | 0% |  | 3,272 | 0% | -0.49% |
| [Module/keep](#modulekeep) | 7,206 | +0.92% |  | 7 | 0% |  |  |  |  |
| [Module/keep_written](#modulekeep_written) | 29,151 | +0.23% |  | 11 | 0% |  |  |  |  |
| [Load/large_static](#loadlarge_static) | 147,742 | 0% | -83.32% | 14 | 0% | -96.82% | 43,136 | 0% | -19.20% |
| [Render/large_static](#renderlarge_static) | 20,743 | 0% | -72.39% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [RenderSink/large_static](#rendersinklarge_static) | 12,730 | +0.01% |  | 0 |  |  | 0 |  |  |
| [RenderTo/large_static](#rendertolarge_static) | 20,306 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/macros](#loadmacros) | 54,940 | 0% | -75.90% | 27 | 0% | -85.48% | 4,408 | 0% | -29.09% |
| [Render/macros](#rendermacros) | 662,135 | -0.06% | -74.17% | 308 | 0% | -88.71% | 20,576 | +0.08% | +39.78% |
| [Load/many_tags](#loadmany_tags) | 8,714,972 | 0% | -80.27% | 951 | 0% | -97.22% | 438,176 | 0% | -58.48% |
| [Render/many_tags](#rendermany_tags) | 729,975 | 0% | -64.96% | 2 | 0% | -90.48% | 5,648 | 0% | -10.52% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 45,097 | 0% | -81.02% | 19 | 0% | -89.67% | 3,376 | 0% | -44.33% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,216,052 | +0.08% | -57.79% | 33 | 0% | -99.19% | 381,536 | 0% | +9.95% |
| [RenderSink/mitsuhiko_table](#rendersinkmitsuhiko_table) | 5,143,318 | +0.08% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table](#rendertomitsuhiko_table) | 5,215,605 | +0.08% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 52,802 | 0% |  | 35 | 0% |  | 5,704 | 0% | -30.98% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,562,099 | +0.07% |  | 33 | 0% |  | 1,414,928 | 0% | +2.46% |
| [RenderSink/mitsuhiko_table_wide](#rendersinkmitsuhiko_table_wide) | 5,385,401 | +0.07% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table_wide](#rendertomitsuhiko_table_wide) | 5,561,650 | +0.07% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/plain_text](#loadplain_text) | 3,752 | 0% | -61.22% | 4 | 0% | -77.78% | 688 | 0% | -38.57% |
| [Render/plain_text](#renderplain_text) | 1,051 | +0.10% | -84.53% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/plain_text](#rendersinkplain_text) | 752 | +0.13% |  | 0 |  |  | 0 |  |  |
| [RenderTo/plain_text](#rendertoplain_text) | 852 | +0.12% |  | 0 |  |  | 0 |  |  |
| [Load/prompt_budget](#loadprompt_budget) | 34,462 | 0% |  | 18 | 0% |  | 2,240 | 0% | +0.72% |
| [Render/prompt_budget](#renderprompt_budget) | 1,320,457 | 0% |  | 76 | 0% |  | 36,416 | 0% | 0% |
| [Load/strings](#loadstrings) | 68,994 | 0% | -76.36% | 30 | 0% | -86.61% | 4,360 | 0% | -51.77% |
| [Render/strings](#renderstrings) | 584,162 | 0% | -72.15% | 344 | 0% | -83.73% | 9,088 | 0% | -49.31% |
| [Load/substitute](#loadsubstitute) | 7,531 | 0% | -74.78% | 6 | 0% | -83.78% | 848 | 0% | -44.21% |
| [Render/substitute](#rendersubstitute) | 2,285 | +0.04% | -74.07% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/substitute](#rendersinksubstitute) | 1,989 | +0.05% |  | 0 |  |  | 0 |  |  |
| [RenderTo/substitute](#rendertosubstitute) | 2,090 | +0.05% |  | 0 |  |  | 0 |  |  |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 755 | +1.62% | 761 | +0.40% | 0 |  | 7,826 | +2.88% |
| Render/chat_llama | 1,177 | +1.47% | 523 | +2.95% | 0 |  | 4,029 | -5.47% |
| Load/chat_mistral | 1,103 | +1.47% | 1,079 | -0.09% | 0 |  | 8,896 | +3.02% |
| Render/chat_mistral | 1,940 | +1.41% | 577 | -0.52% | 0 |  | 10,694 | +4.80% |
| Load/chat_qwen | 578 | +1.40% | 628 | -0.16% | 0 |  | 4,761 | +6.70% |
| Render/chat_qwen | 1,045 | -0.29% | 396 | -3.18% | 0 |  | 3,445 | -4.70% |
| Load/config_file | 228 | -0.44% | 196 | +2.08% | 0 |  | 2,802 | +0.90% |
| Render/config_file | 5,972 | -2.27% | 1,506 | -6.11% | 0 |  | 45,038 | +3.53% |
| Load/dict_ops | 56 | -6.67% | 30 | -11.76% | 0 |  | 1,360 | -0.80% |
| Render/dict_ops | 672 | -0.74% | 417 | -1.18% | 0 |  | 880 | -2.65% |
| Load/expressions | 34 | -10.53% | 27 | -10.00% | 0 |  | 1,343 | +0.07% |
| Render/expressions | 6 | 0% | 5 | +25.00% | 0 |  | 308 | +4.41% |
| Load/filters | 97 | +10.23% | 45 | +9.76% | 0 |  | 1,611 | +7.69% |
| Render/filters | 14 | +16.67% | 6 | +50.00% | 0 |  | 790 | 0% |
| Load/for_filter_if | 28 | -6.67% | 12 | -7.69% | 0 |  | 1,071 | -2.99% |
| Render/for_filter_if | 1,736 | -0.17% | 128 | -0.78% | 0 |  | 221 | +20.77% |
| Load/for_loop_vars | 57 | -8.06% | 22 | -8.33% | 0 |  | 1,127 | -4.25% |
| Render/for_loop_vars | 6 | -33.33% | 2 | 0% | 0 |  | 177 | +7.27% |
| Load/for_range | 3 | 0% | 4 | 0% | 0 |  | 767 | -4.13% |
| Render/for_range | 1 | 0% | 0 |  | 0 |  | 132 | +24.53% |
| Load/hf_deepseek_v31 | 995 | +0.81% | 987 | +0.41% | 0 |  | 6,557 | +3.64% |
| Render/hf_deepseek_v31 | 348 | -1.42% | 82 | +7.89% | 0 |  | 1,538 | +2.12% |
| Load/hf_gpt_oss | 4,643 | +0.09% | 3,392 | -0.24% | 0 |  | 24,610 | +12.07% |
| Render/hf_gpt_oss | 1,212 | -0.66% | 395 | +0.25% | 0 |  | 4,774 | -0.87% |
| Load/hf_mistral_small | 1,871 | +0.21% | 1,632 | -0.24% | 0 |  | 11,417 | +1.27% |
| Render/hf_mistral_small | 954 | -0.31% | 303 | -0.33% | 0 |  | 3,771 | +0.80% |
| Load/hf_qwen3 | 1,529 | +2.48% | 1,343 | +0.83% | 0 |  | 9,427 | +6.34% |
| Render/hf_qwen3 | 691 | +4.22% | 219 | -0.90% | 0 |  | 2,699 | -0.41% |
| Load/html_autoescape | 500 | +0.20% | 532 | +0.38% | 0 |  | 4,139 | +1.77% |
| Render/html_autoescape | 4,431 | +20.54% | 1,318 | +20.48% | 0 |  | 35,414 | -2.50% |
| Load/inheritance | 7 | -12.50% | 4 | 0% | 0 |  | 742 | +1.92% |
| Render/inheritance | 598 | -0.17% | 98 | -3.92% | 0 |  | 959 | -3.03% |
| Load/int_arith | 85 | -3.41% | 62 | -4.62% | 0 |  | 2,113 | +0.19% |
| Render/int_arith | 17 | +30.77% | 11 | +120.00% | 0 |  | 755 | +2.58% |
| Module/keep | 1 | 0% | 2 | 0% | 0 |  | 431 | -0.69% |
| Module/keep_written | 1,136 | -0.18% | 562 | +0.36% | 0 |  | 745 | -0.93% |
| Load/large_static | 1,925 | 0% | 904 | 0% | 0 |  | 293 | -2.01% |
| Render/large_static | 709 | 0% | 569 | 0% | 0 |  | 7 | +16.67% |
| RenderSink/large_static | 0 |  | 0 |  | 0 |  | 5 | +150.00% |
| RenderTo/large_static | 685 | 0% | 573 | 0% | 0 |  | 6 | 0% |
| Load/macros | 98 | +3.16% | 76 | +2.70% | 0 |  | 1,774 | +0.45% |
| Render/macros | 754 | -0.53% | 230 | +1.32% | 0 |  | 1,417 | -12.96% |
| Load/many_tags | 27,924 | -0.03% | 22,571 | +0.03% | 0 |  | 237,696 | +2.05% |
| Render/many_tags | 6,237 | -0.10% | 857 | +0.12% | 0 |  | 123 | -23.60% |
| Load/mitsuhiko_table | 51 | +6.25% | 62 | +6.90% | 0 |  | 866 | -8.17% |
| Render/mitsuhiko_table | 10,068 | +0.02% | 6,280 | 0% | 0 |  | 384 | -5.42% |
| RenderSink/mitsuhiko_table | 10,002 | +0.25% | 1,218 | +0.66% | 0 |  | 337 | -0.30% |
| RenderTo/mitsuhiko_table | 10,090 | +0.31% | 6,300 | +0.17% | 0 |  | 358 | -8.67% |
| Load/mitsuhiko_table_wide | 91 | +13.75% | 84 | +2.44% | 0 |  | 1,104 | -2.21% |
| Render/mitsuhiko_table_wide | 10,228 | +0.12% | 22,667 | +0.02% | 0 |  | 387 | -2.27% |
| RenderSink/mitsuhiko_table_wide | 10,065 | -0.02% | 1,292 | +0.23% | 0 |  | 308 | -4.35% |
| RenderTo/mitsuhiko_table_wide | 10,240 | +0.09% | 22,675 | +0.02% | 0 |  | 362 | +0.56% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 23 | -25.81% |
| Render/plain_text | 1 | 0% | 0 |  | 0 |  | 6 | -14.29% |
| RenderSink/plain_text | 1 | 0% | 0 |  | 0 |  | 5 | -16.67% |
| RenderTo/plain_text | 1 | 0% | 0 |  | 0 |  | 5 | -16.67% |
| Load/prompt_budget | 83 | -3.49% | 47 | +4.44% | 0 |  | 1,440 | +3.97% |
| Render/prompt_budget | 4,818 | +0.12% | 544 | -0.55% | 0 |  | 1,406 | +32.14% |
| Load/strings | 143 | -1.38% | 109 | -1.80% | 0 |  | 2,803 | +1.41% |
| Render/strings | 430 | +2.87% | 486 | +0.62% | 0 |  | 2,358 | -3.99% |
| Load/substitute | 2 | 0% | 3 | 0% | 0 |  | 186 | +8.14% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 5 | -16.67% |
| RenderSink/substitute | 0 |  | 0 |  | 0 |  | 5 | +66.67% |
| RenderTo/substitute | 0 |  | 0 |  | 0 |  | 6 | 0% |

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
