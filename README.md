# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (222 so far, latest [3fe8977](https://github.com/jinja2cpp/Jinja2Cpp/commit/3fe89772cda6a269bf4fb5748d68c0f115be819b) on 2026-11-14). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 335,407 | 0% |  | 55 | 0% |  | 18,480 | 0% | -52.71% |
| [Render/chat_llama](#renderchat_llama) | 295,477 | +0.03% |  | 225 | 0% |  | 11,616 | 0% | -19.11% |
| [Load/chat_mistral](#loadchat_mistral) | 385,087 | 0% |  | 52 | 0% |  | 20,904 | 0% | -53.18% |
| [Render/chat_mistral](#renderchat_mistral) | 502,026 | +0.01% |  | 402 | 0% |  | 10,032 | 0% | -24.05% |
| [Load/chat_qwen](#loadchat_qwen) | 261,172 | 0% |  | 44 | 0% |  | 14,432 | 0% | -47.19% |
| [Render/chat_qwen](#renderchat_qwen) | 278,828 | +0.03% |  | 212 | 0% |  | 8,304 | 0% | -2.35% |
| [Load/config_file](#loadconfig_file) | 94,253 | 0% |  | 25 | 0% |  | 5,480 | 0% | -48.11% |
| [Render/config_file](#renderconfig_file) | 1,277,836 | +0.18% |  | 908 | 0% |  | 24,440 | 0% | +2.65% |
| [Load/dict_ops](#loaddict_ops) | 32,235 | 0% | -78.19% | 9 | 0% | -93.13% | 2,192 | 0% | -48.50% |
| [Render/dict_ops](#renderdict_ops) | 295,658 | +0.01% | -57.20% | 112 | 0% | -75.11% | 24,408 | 0% | -29.65% |
| [Load/expressions](#loadexpressions) | 44,235 | 0% | -78.54% | 16 | 0% | -89.04% | 2,744 | 0% | -55.28% |
| [Render/expressions](#renderexpressions) | 427,012 | 0% | -52.88% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 67,515 | 0% | -75.40% | 27 | 0% | -88.16% | 3,864 | 0% | -66.15% |
| [Render/filters](#renderfilters) | 27,826 | +0.11% | -68.62% | 23 | 0% | -68.92% | 752 | 0% | -84.69% |
| [Load/for_filter_if](#loadfor_filter_if) | 29,057 | 0% | -78.54% | 7 | 0% | -94.31% | 1,920 | 0% | -44.32% |
| [Render/for_filter_if](#renderfor_filter_if) | 275,011 | 0% | -68.22% | 4 | 0% | -98.23% | 16,560 | 0% | +354.95% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 38,841 | 0% | -78.50% | 10 | 0% | -93.63% | 2,384 | 0% | -47.26% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 180,595 | 0% | -83.17% | 2 | 0% | -99.62% | 1,568 | 0% | -43.35% |
| [Load/for_range](#loadfor_range) | 13,259 | 0% | -76.61% | 7 | 0% | -87.50% | 1,104 | 0% | -44.80% |
| [Render/for_range](#renderfor_range) | 38,463 | +0.03% | -58.29% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/hf_deepseek_v31](#loadhf_deepseek_v31) | 382,862 | 0% |  | 55 | 0% |  | 21,992 | 0% | +0.81% |
| [Render/hf_deepseek_v31](#renderhf_deepseek_v31) | 93,240 | +0.03% |  | 43 | 0% |  | 2,480 | 0% | 0% |
| [Load/hf_gpt_oss](#loadhf_gpt_oss) | 1,522,409 | 0% |  | 287 | 0% |  | 74,688 | 0% | +0.63% |
| [Render/hf_gpt_oss](#renderhf_gpt_oss) | 173,626 | +0.04% |  | 155 | 0% |  | 5,080 | 0% | +2.92% |
| [Load/hf_mistral_small](#loadhf_mistral_small) | 591,652 | 0% |  | 88 | 0% |  | 33,064 | 0% | +0.39% |
| [Render/hf_mistral_small](#renderhf_mistral_small) | 142,855 | +0.05% |  | 99 | 0% |  | 6,120 | 0% | -0.26% |
| [Load/hf_qwen3](#loadhf_qwen3) | 475,801 | 0% |  | 82 | 0% |  | 25,576 | 0% | +0.38% |
| [Render/hf_qwen3](#renderhf_qwen3) | 121,434 | +0.02% |  | 100 | 0% |  | 3,248 | 0% | 0% |
| [Load/html_autoescape](#loadhtml_autoescape) | 188,619 | 0% |  | 43 | 0% |  | 12,688 | 0% | -48.59% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,211,670 | +0.06% |  | 674 | 0% |  | 45,408 | 0% | +12.44% |
| [Load/inheritance](#loadinheritance) | 21,939 | 0% | -80.78% | 7 | 0% | -93.46% | 1,360 | 0% | -52.65% |
| [Render/inheritance](#renderinheritance) | 137,944 | +0.01% | -81.55% | 35 | 0% | -94.37% | 7,384 | 0% | +22.74% |
| [Load/int_arith](#loadint_arith) | 65,009 | 0% |  | 20 | 0% |  | 3,888 | 0% | +0.83% |
| [Render/int_arith](#renderint_arith) | 831,480 | +0.25% |  | 12 | 0% |  | 3,272 | 0% | -0.49% |
| [Module/keep](#modulekeep) | 7,140 | +0.07% |  | 7 | 0% |  |  |  |  |
| [Module/keep_written](#modulekeep_written) | 29,085 | +0.02% |  | 11 | 0% |  |  |  |  |
| [Load/large_static](#loadlarge_static) | 147,742 | 0% | -83.32% | 14 | 0% | -96.82% | 43,136 | 0% | -19.20% |
| [Render/large_static](#renderlarge_static) | 20,742 | 0% | -72.39% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [RenderSink/large_static](#rendersinklarge_static) | 12,729 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/large_static](#rendertolarge_static) | 20,305 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/macros](#loadmacros) | 54,940 | 0% | -75.90% | 27 | 0% | -85.48% | 4,408 | 0% | -29.09% |
| [Render/macros](#rendermacros) | 662,519 | +0.43% | -74.15% | 308 | 0% | -88.71% | 20,560 | 0% | +39.67% |
| [Load/many_tags](#loadmany_tags) | 8,714,972 | 0% | -80.27% | 951 | 0% | -97.22% | 438,176 | 0% | -58.48% |
| [Render/many_tags](#rendermany_tags) | 729,974 | 0% | -64.96% | 2 | 0% | -90.48% | 5,648 | 0% | -10.52% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 45,097 | 0% | -81.02% | 19 | 0% | -89.67% | 3,376 | 0% | -44.33% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,212,044 | +0.10% | -57.82% | 33 | 0% | -99.19% | 381,536 | 0% | +9.95% |
| [RenderSink/mitsuhiko_table](#rendersinkmitsuhiko_table) | 5,139,309 | +0.10% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table](#rendertomitsuhiko_table) | 5,211,597 | +0.10% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 52,802 | 0% |  | 35 | 0% |  | 5,704 | 0% | -30.98% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,558,090 | +0.09% |  | 33 | 0% |  | 1,414,928 | 0% | +2.46% |
| [RenderSink/mitsuhiko_table_wide](#rendersinkmitsuhiko_table_wide) | 5,381,392 | +0.09% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table_wide](#rendertomitsuhiko_table_wide) | 5,557,641 | +0.09% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/plain_text](#loadplain_text) | 3,752 | 0% | -61.22% | 4 | 0% | -77.78% | 688 | 0% | -38.57% |
| [Render/plain_text](#renderplain_text) | 1,050 | 0% | -84.54% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/plain_text](#rendersinkplain_text) | 751 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/plain_text](#rendertoplain_text) | 851 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/prompt_budget](#loadprompt_budget) | 34,462 | 0% |  | 18 | 0% |  | 2,240 | 0% | +0.72% |
| [Render/prompt_budget](#renderprompt_budget) | 1,320,452 | 0% |  | 76 | 0% |  | 36,416 | 0% | 0% |
| [Load/strings](#loadstrings) | 68,994 | 0% | -76.36% | 30 | 0% | -86.61% | 4,360 | 0% | -51.77% |
| [Render/strings](#renderstrings) | 584,141 | -0.03% | -72.15% | 344 | 0% | -83.73% | 9,088 | 0% | -49.31% |
| [Load/substitute](#loadsubstitute) | 7,531 | 0% | -74.78% | 6 | 0% | -83.78% | 848 | 0% | -44.21% |
| [Render/substitute](#rendersubstitute) | 2,284 | 0% | -74.09% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/substitute](#rendersinksubstitute) | 1,988 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/substitute](#rendertosubstitute) | 2,089 | 0% |  | 0 |  |  | 0 |  |  |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 743 | -0.54% | 758 | +0.26% | 0 |  | 7,607 | -3.39% |
| Render/chat_llama | 1,160 | -2.52% | 508 | -0.39% | 0 |  | 4,262 | +3.60% |
| Load/chat_mistral | 1,087 | -3.55% | 1,080 | +0.28% | 0 |  | 8,635 | -5.71% |
| Render/chat_mistral | 1,913 | -1.54% | 580 | +0.52% | 0 |  | 10,204 | -1.60% |
| Load/chat_qwen | 570 | -0.18% | 629 | +0.64% | 0 |  | 4,462 | -7.18% |
| Render/chat_qwen | 1,048 | +0.58% | 409 | +2.76% | 0 |  | 3,615 | +2.35% |
| Load/config_file | 229 | 0% | 192 | -2.54% | 0 |  | 2,777 | -4.18% |
| Render/config_file | 6,111 | -3.02% | 1,604 | -1.78% | 0 |  | 43,503 | +1.23% |
| Load/dict_ops | 60 | +11.11% | 34 | +6.25% | 0 |  | 1,371 | -2.49% |
| Render/dict_ops | 677 | -0.44% | 422 | -0.24% | 0 |  | 904 | +1.12% |
| Load/expressions | 38 | +2.70% | 30 | +3.45% | 0 |  | 1,342 | -2.61% |
| Render/expressions | 6 | +20.00% | 4 | 0% | 0 |  | 295 | -11.68% |
| Load/filters | 88 | -17.76% | 41 | -2.38% | 0 |  | 1,496 | -7.48% |
| Render/filters | 12 | +20.00% | 4 | 0% | 0 |  | 790 | +0.13% |
| Load/for_filter_if | 30 | +20.00% | 13 | +8.33% | 0 |  | 1,104 | -4.58% |
| Render/for_filter_if | 1,739 | -0.11% | 129 | -1.53% | 0 |  | 183 | -7.11% |
| Load/for_loop_vars | 62 | 0% | 24 | 0% | 0 |  | 1,177 | -5.16% |
| Render/for_loop_vars | 9 | -50.00% | 2 | -33.33% | 0 |  | 165 | +7.84% |
| Load/for_range | 3 | 0% | 4 | 0% | 0 |  | 800 | -1.48% |
| Render/for_range | 1 | 0% | 0 |  | 0 |  | 106 | -26.90% |
| Load/hf_deepseek_v31 | 987 | -2.37% | 983 | -0.30% | 0 |  | 6,327 | -5.16% |
| Render/hf_deepseek_v31 | 353 | -4.08% | 76 | 0% | 0 |  | 1,506 | +1.14% |
| Load/hf_gpt_oss | 4,639 | -1.47% | 3,400 | -0.21% | 0 |  | 21,959 | -5.91% |
| Render/hf_gpt_oss | 1,220 | -2.01% | 394 | +0.51% | 0 |  | 4,816 | +1.69% |
| Load/hf_mistral_small | 1,867 | -1.06% | 1,636 | +0.49% | 0 |  | 11,274 | -4.08% |
| Render/hf_mistral_small | 957 | -1.14% | 304 | +2.01% | 0 |  | 3,741 | +1.11% |
| Load/hf_qwen3 | 1,492 | -2.80% | 1,332 | -0.89% | 0 |  | 8,865 | -5.71% |
| Render/hf_qwen3 | 663 | -1.63% | 221 | -1.34% | 0 |  | 2,710 | -0.18% |
| Load/html_autoescape | 499 | -3.85% | 530 | -1.49% | 0 |  | 4,067 | -5.18% |
| Render/html_autoescape | 3,676 | -26.92% | 1,094 | -9.21% | 0 |  | 36,322 | +1.76% |
| Load/inheritance | 8 | 0% | 4 | 0% | 0 |  | 728 | -0.27% |
| Render/inheritance | 599 | -1.16% | 102 | -1.92% | 0 |  | 989 | -0.90% |
| Load/int_arith | 88 | 0% | 65 | +12.07% | 0 |  | 2,109 | -0.66% |
| Render/int_arith | 13 | +30.00% | 5 | +25.00% | 0 |  | 736 | -4.42% |
| Module/keep | 1 | 0% | 2 | 0% | 0 |  | 434 | -2.91% |
| Module/keep_written | 1,138 | 0% | 560 | +0.18% | 0 |  | 752 | -0.40% |
| Load/large_static | 1,925 | -0.16% | 904 | 0% | 0 |  | 299 | -6.85% |
| Render/large_static | 709 | +0.14% | 569 | 0% | 0 |  | 6 | -25.00% |
| RenderSink/large_static | 0 |  | 0 |  | 0 |  | 2 | -60.00% |
| RenderTo/large_static | 685 | +0.15% | 573 | 0% | 0 |  | 6 | -14.29% |
| Load/macros | 95 | -2.06% | 74 | -2.63% | 0 |  | 1,766 | +0.97% |
| Render/macros | 758 | +0.93% | 227 | -2.16% | 0 |  | 1,628 | +158.41% |
| Load/many_tags | 27,931 | -1.78% | 22,565 | -0.15% | 0 |  | 232,925 | -5.01% |
| Render/many_tags | 6,243 | -0.02% | 856 | -0.47% | 0 |  | 161 | -9.55% |
| Load/mitsuhiko_table | 48 | 0% | 58 | -10.77% | 0 |  | 943 | +1.07% |
| Render/mitsuhiko_table | 10,066 | +0.07% | 6,280 | -0.11% | 0 |  | 406 | +0.25% |
| RenderSink/mitsuhiko_table | 9,977 | -0.02% | 1,210 | +0.25% | 0 |  | 338 | -4.79% |
| RenderTo/mitsuhiko_table | 10,059 | +0.03% | 6,289 | -0.08% | 0 |  | 392 | -2.49% |
| Load/mitsuhiko_table_wide | 80 | +8.11% | 82 | -8.89% | 0 |  | 1,129 | +5.61% |
| Render/mitsuhiko_table_wide | 10,216 | +0.11% | 22,662 | -0.01% | 0 |  | 396 | -8.97% |
| RenderSink/mitsuhiko_table_wide | 10,067 | +0.08% | 1,289 | +0.47% | 0 |  | 322 | -8.52% |
| RenderTo/mitsuhiko_table_wide | 10,231 | +0.01% | 22,671 | -0.01% | 0 |  | 360 | -13.67% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 31 | +416.67% |
| Render/plain_text | 1 | 0% | 0 |  | 0 |  | 7 | +16.67% |
| RenderSink/plain_text | 1 | 0% | 0 |  | 0 |  | 6 | -14.29% |
| RenderTo/plain_text | 1 | 0% | 0 |  | 0 |  | 6 | 0% |
| Load/prompt_budget | 86 | -5.49% | 45 | -8.16% | 0 |  | 1,385 | -3.28% |
| Render/prompt_budget | 4,812 | -0.19% | 547 | 0% | 0 |  | 1,064 | +15.15% |
| Load/strings | 145 | -4.61% | 111 | -0.89% | 0 |  | 2,764 | -1.07% |
| Render/strings | 418 | -1.18% | 483 | -0.21% | 0 |  | 2,456 | +2.59% |
| Load/substitute | 2 | 0% | 3 | 0% | 0 |  | 172 | -16.10% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 6 | -25.00% |
| RenderSink/substitute | 0 |  | 0 |  | 0 |  | 3 | -40.00% |
| RenderTo/substitute | 0 |  | 0 |  | 0 |  | 6 | -14.29% |

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
