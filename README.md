# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (221 so far, latest [ff1357f](https://github.com/jinja2cpp/Jinja2Cpp/commit/ff1357f04221c16660ad234522a8b540fa0a8f9d) on 2026-11-14). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 335,407 | -0.01% |  | 55 | 0% |  | 18,480 | 0% | -52.71% |
| [Render/chat_llama](#renderchat_llama) | 295,392 | +0.01% |  | 225 | 0% |  | 11,616 | 0% | -19.11% |
| [Load/chat_mistral](#loadchat_mistral) | 385,087 | 0% |  | 52 | 0% |  | 20,904 | 0% | -53.18% |
| [Render/chat_mistral](#renderchat_mistral) | 501,954 | 0% |  | 402 | 0% |  | 10,032 | 0% | -24.05% |
| [Load/chat_qwen](#loadchat_qwen) | 261,172 | +0.01% |  | 44 | 0% |  | 14,432 | 0% | -47.19% |
| [Render/chat_qwen](#renderchat_qwen) | 278,755 | -0.01% |  | 212 | 0% |  | 8,304 | 0% | -2.35% |
| [Load/config_file](#loadconfig_file) | 94,253 | 0% |  | 25 | 0% |  | 5,480 | 0% | -48.11% |
| [Render/config_file](#renderconfig_file) | 1,275,588 | +0.02% |  | 908 | 0% |  | 24,440 | 0% | +2.65% |
| [Load/dict_ops](#loaddict_ops) | 32,235 | 0% | -78.19% | 9 | 0% | -93.13% | 2,192 | 0% | -48.50% |
| [Render/dict_ops](#renderdict_ops) | 295,643 | -0.13% | -57.21% | 112 | 0% | -75.11% | 24,408 | 0% | -29.65% |
| [Load/expressions](#loadexpressions) | 44,235 | 0% | -78.54% | 16 | 0% | -89.04% | 2,744 | 0% | -55.28% |
| [Render/expressions](#renderexpressions) | 427,002 | 0% | -52.88% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 67,515 | 0% | -75.40% | 27 | 0% | -88.16% | 3,864 | 0% | -66.15% |
| [Render/filters](#renderfilters) | 27,796 | +0.01% | -68.65% | 23 | 0% | -68.92% | 752 | 0% | -84.69% |
| [Load/for_filter_if](#loadfor_filter_if) | 29,057 | 0% | -78.54% | 7 | 0% | -94.31% | 1,920 | 0% | -44.32% |
| [Render/for_filter_if](#renderfor_filter_if) | 275,006 | 0% | -68.23% | 4 | 0% | -98.23% | 16,560 | 0% | +354.95% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 38,841 | 0% | -78.50% | 10 | 0% | -93.63% | 2,384 | 0% | -47.26% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 180,590 | 0% | -83.17% | 2 | 0% | -99.62% | 1,568 | 0% | -43.35% |
| [Load/for_range](#loadfor_range) | 13,259 | 0% | -76.61% | 7 | 0% | -87.50% | 1,104 | 0% | -44.80% |
| [Render/for_range](#renderfor_range) | 38,453 | +0.01% | -58.30% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/hf_deepseek_v31](#loadhf_deepseek_v31) | 382,862 | -0.01% |  | 55 | 0% |  | 21,992 | 0% | +0.81% |
| [Render/hf_deepseek_v31](#renderhf_deepseek_v31) | 93,212 | +0.03% |  | 43 | 0% |  | 2,480 | 0% | 0% |
| [Load/hf_gpt_oss](#loadhf_gpt_oss) | 1,522,407 | +0.01% |  | 287 | 0% |  | 74,688 | 0% | +0.63% |
| [Render/hf_gpt_oss](#renderhf_gpt_oss) | 173,554 | +0.02% |  | 155 | 0% |  | 5,080 | 0% | +2.92% |
| [Load/hf_mistral_small](#loadhf_mistral_small) | 591,652 | 0% |  | 88 | 0% |  | 33,064 | 0% | +0.39% |
| [Render/hf_mistral_small](#renderhf_mistral_small) | 142,781 | +0.02% |  | 99 | 0% |  | 6,120 | 0% | -0.26% |
| [Load/hf_qwen3](#loadhf_qwen3) | 475,801 | +0.01% |  | 82 | 0% |  | 25,576 | 0% | +0.38% |
| [Render/hf_qwen3](#renderhf_qwen3) | 121,409 | +0.02% |  | 100 | 0% |  | 3,248 | 0% | 0% |
| [Load/html_autoescape](#loadhtml_autoescape) | 188,619 | 0% |  | 43 | 0% |  | 12,688 | 0% | -48.59% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,210,899 | -0.04% |  | 674 | 0% |  | 45,408 | 0% | +12.44% |
| [Load/inheritance](#loadinheritance) | 21,939 | 0% | -80.78% | 7 | 0% | -93.46% | 1,360 | 0% | -52.65% |
| [Render/inheritance](#renderinheritance) | 137,926 | +0.02% | -81.56% | 35 | 0% | -94.37% | 7,384 | 0% | +22.74% |
| [Load/int_arith](#loadint_arith) | 65,009 | 0% |  | 20 | 0% |  | 3,888 | 0% | +0.83% |
| [Render/int_arith](#renderint_arith) | 829,442 | 0% |  | 12 | 0% |  | 3,272 | 0% | -0.49% |
| [Module/keep](#modulekeep) | 7,135 | +0.03% |  | 7 | 0% |  |  |  |  |
| [Module/keep_written](#modulekeep_written) | 29,080 | +0.01% |  | 11 | 0% |  |  |  |  |
| [Load/large_static](#loadlarge_static) | 147,742 | 0% | -83.32% | 14 | 0% | -96.82% | 43,136 | 0% | -19.20% |
| [Render/large_static](#renderlarge_static) | 20,742 | +0.01% | -72.39% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [RenderSink/large_static](#rendersinklarge_static) | 12,729 | +0.02% |  | 0 |  |  | 0 |  |  |
| [RenderTo/large_static](#rendertolarge_static) | 20,305 | +0.01% |  | 0 |  |  | 0 |  |  |
| [Load/macros](#loadmacros) | 54,940 | 0% | -75.90% | 27 | 0% | -85.48% | 4,408 | 0% | -29.09% |
| [Render/macros](#rendermacros) | 659,714 | 0% | -74.26% | 308 | 0% | -88.71% | 20,560 | 0% | +39.67% |
| [Load/many_tags](#loadmany_tags) | 8,714,972 | 0% | -80.27% | 951 | 0% | -97.22% | 438,176 | 0% | -58.48% |
| [Render/many_tags](#rendermany_tags) | 729,974 | 0% | -64.96% | 2 | 0% | -90.48% | 5,648 | 0% | -10.52% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 45,097 | 0% | -81.02% | 19 | 0% | -89.67% | 3,376 | 0% | -44.33% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,207,034 | +0.04% | -57.86% | 33 | 0% | -99.19% | 381,536 | 0% | +9.95% |
| [RenderSink/mitsuhiko_table](#rendersinkmitsuhiko_table) | 5,134,299 | +0.04% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table](#rendertomitsuhiko_table) | 5,206,587 | +0.04% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 52,802 | 0% |  | 35 | 0% |  | 5,704 | 0% | -30.98% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,553,080 | +0.04% |  | 33 | 0% |  | 1,414,928 | 0% | +2.46% |
| [RenderSink/mitsuhiko_table_wide](#rendersinkmitsuhiko_table_wide) | 5,376,382 | +0.04% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table_wide](#rendertomitsuhiko_table_wide) | 5,552,631 | +0.04% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/plain_text](#loadplain_text) | 3,752 | 0% | -61.22% | 4 | 0% | -77.78% | 688 | 0% | -38.57% |
| [Render/plain_text](#renderplain_text) | 1,050 | +0.19% | -84.54% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/plain_text](#rendersinkplain_text) | 751 | +0.27% |  | 0 |  |  | 0 |  |  |
| [RenderTo/plain_text](#rendertoplain_text) | 851 | +0.24% |  | 0 |  |  | 0 |  |  |
| [Load/prompt_budget](#loadprompt_budget) | 34,462 | 0% |  | 18 | 0% |  | 2,240 | 0% | +0.72% |
| [Render/prompt_budget](#renderprompt_budget) | 1,320,447 | +0.01% |  | 76 | 0% |  | 36,416 | 0% | 0% |
| [Load/strings](#loadstrings) | 68,994 | 0% | -76.36% | 30 | 0% | -86.61% | 4,360 | 0% | -51.77% |
| [Render/strings](#renderstrings) | 584,319 | 0% | -72.14% | 344 | 0% | -83.73% | 9,088 | 0% | -49.31% |
| [Load/substitute](#loadsubstitute) | 7,531 | 0% | -74.78% | 6 | 0% | -83.78% | 848 | 0% | -44.21% |
| [Render/substitute](#rendersubstitute) | 2,284 | +0.09% | -74.09% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/substitute](#rendersinksubstitute) | 1,988 | +0.10% |  | 0 |  |  | 0 |  |  |
| [RenderTo/substitute](#rendertosubstitute) | 2,089 | +0.10% |  | 0 |  |  | 0 |  |  |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 747 | +1.49% | 756 | +0.53% | 0 |  | 7,874 | +0.59% |
| Render/chat_llama | 1,190 | -0.67% | 510 | -0.39% | 0 |  | 4,114 | -3.06% |
| Load/chat_mistral | 1,127 | +1.53% | 1,077 | -0.74% | 0 |  | 9,158 | +2.99% |
| Render/chat_mistral | 1,943 | +0.15% | 577 | -1.70% | 0 |  | 10,370 | +0.52% |
| Load/chat_qwen | 571 | -0.35% | 625 | -0.32% | 0 |  | 4,807 | +3.60% |
| Render/chat_qwen | 1,042 | -3.34% | 398 | +0.51% | 0 |  | 3,532 | -2.35% |
| Load/config_file | 229 | -3.78% | 197 | +7.07% | 0 |  | 2,898 | +6.31% |
| Render/config_file | 6,301 | -2.16% | 1,633 | +2.45% | 0 |  | 42,975 | +0.12% |
| Load/dict_ops | 54 | -8.47% | 32 | +6.67% | 0 |  | 1,406 | +0.86% |
| Render/dict_ops | 680 | -0.58% | 423 | +0.24% | 0 |  | 894 | 0% |
| Load/expressions | 37 | -21.28% | 29 | -17.14% | 0 |  | 1,378 | +1.85% |
| Render/expressions | 5 | -54.55% | 4 | 0% | 0 |  | 334 | +4.05% |
| Load/filters | 107 | +7.00% | 42 | +10.53% | 0 |  | 1,617 | +3.26% |
| Render/filters | 10 | -23.08% | 4 | 0% | 0 |  | 789 | -0.25% |
| Load/for_filter_if | 25 | -16.67% | 12 | -20.00% | 0 |  | 1,157 | +7.53% |
| Render/for_filter_if | 1,741 | -0.06% | 131 | 0% | 0 |  | 197 | -1.01% |
| Load/for_loop_vars | 62 | +12.73% | 24 | +20.00% | 0 |  | 1,241 | +13.54% |
| Render/for_loop_vars | 18 | +5.88% | 3 | +50.00% | 0 |  | 153 | -19.47% |
| Load/for_range | 3 | 0% | 4 | 0% | 0 |  | 812 | +7.27% |
| Render/for_range | 1 | 0% | 0 |  | 0 |  | 145 | -0.68% |
| Load/hf_deepseek_v31 | 1,011 | -0.10% | 986 | +0.10% | 0 |  | 6,671 | +2.17% |
| Render/hf_deepseek_v31 | 368 | +2.22% | 76 | +8.57% | 0 |  | 1,489 | +5.45% |
| Load/hf_gpt_oss | 4,708 | +0.13% | 3,407 | +0.24% | 0 |  | 23,339 | +4.22% |
| Render/hf_gpt_oss | 1,245 | +0.16% | 392 | 0% | 0 |  | 4,736 | +1.46% |
| Load/hf_mistral_small | 1,887 | -0.37% | 1,628 | -0.43% | 0 |  | 11,754 | +2.50% |
| Render/hf_mistral_small | 968 | +1.79% | 298 | -1.65% | 0 |  | 3,700 | +2.01% |
| Load/hf_qwen3 | 1,535 | +0.79% | 1,344 | +0.07% | 0 |  | 9,402 | +5.11% |
| Render/hf_qwen3 | 674 | -3.85% | 224 | -0.44% | 0 |  | 2,715 | +2.57% |
| Load/html_autoescape | 519 | +1.76% | 538 | +0.94% | 0 |  | 4,289 | +10.46% |
| Render/html_autoescape | 5,030 | +28.09% | 1,205 | +13.47% | 0 |  | 35,694 | -3.25% |
| Load/inheritance | 8 | +100.00% | 4 | +33.33% | 0 |  | 730 | +4.43% |
| Render/inheritance | 606 | +0.50% | 104 | -2.80% | 0 |  | 998 | +3.21% |
| Load/int_arith | 88 | +7.32% | 58 | +7.41% | 0 |  | 2,123 | +5.36% |
| Render/int_arith | 10 | -37.50% | 4 | -20.00% | 0 |  | 770 | +2.67% |
| Module/keep | 1 | 0% | 2 | 0% | 0 |  | 447 | +3.23% |
| Module/keep_written | 1,138 | +0.26% | 559 | 0% | 0 |  | 755 | -0.26% |
| Load/large_static | 1,928 | +0.26% | 904 | 0% | 0 |  | 321 | -6.96% |
| Render/large_static | 708 | 0% | 569 | +0.18% | 0 |  | 8 | +33.33% |
| RenderSink/large_static | 0 | -100.00% | 0 |  | 0 |  | 5 | +25.00% |
| RenderTo/large_static | 684 | 0% | 573 | +0.17% | 0 |  | 7 | +40.00% |
| Load/macros | 97 | -1.02% | 76 | +5.56% | 0 |  | 1,749 | +2.22% |
| Render/macros | 751 | -0.53% | 232 | -0.43% | 0 |  | 630 | +0.96% |
| Load/many_tags | 28,437 | +1.59% | 22,599 | +0.13% | 0 |  | 245,213 | +5.37% |
| Render/many_tags | 6,244 | -0.14% | 860 | -0.81% | 0 |  | 178 | +11.95% |
| Load/mitsuhiko_table | 48 | 0% | 65 | +6.56% | 0 |  | 933 | +5.78% |
| Render/mitsuhiko_table | 10,059 | +0.06% | 6,287 | +0.13% | 0 |  | 405 | +10.35% |
| RenderSink/mitsuhiko_table | 9,979 | +0.02% | 1,207 | -0.58% | 0 |  | 355 | +15.26% |
| RenderTo/mitsuhiko_table | 10,056 | +0.05% | 6,294 | +0.05% | 0 |  | 402 | +21.08% |
| Load/mitsuhiko_table_wide | 74 | +5.71% | 90 | +20.00% | 0 |  | 1,069 | +0.85% |
| Render/mitsuhiko_table_wide | 10,205 | -0.14% | 22,665 | +0.01% | 0 |  | 435 | +13.58% |
| RenderSink/mitsuhiko_table_wide | 10,059 | -0.02% | 1,283 | +0.39% | 0 |  | 352 | +13.18% |
| RenderTo/mitsuhiko_table_wide | 10,230 | -0.05% | 22,674 | +0.02% | 0 |  | 417 | +15.19% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 6 | -85.71% |
| Render/plain_text | 1 | 0% | 0 |  | 0 |  | 6 | -14.29% |
| RenderSink/plain_text | 1 | 0% | 0 |  | 0 |  | 7 | +40.00% |
| RenderTo/plain_text | 1 | 0% | 0 |  | 0 |  | 6 | 0% |
| Load/prompt_budget | 91 | +5.81% | 49 | +8.89% | 0 |  | 1,432 | +2.80% |
| Render/prompt_budget | 4,821 | +0.12% | 547 | +0.55% | 0 |  | 924 | +2.90% |
| Load/strings | 152 | +3.40% | 112 | +4.67% | 0 |  | 2,794 | +0.43% |
| Render/strings | 423 | +1.44% | 484 | -0.21% | 0 |  | 2,394 | +51.14% |
| Load/substitute | 2 | 0% | 3 | 0% | 0 |  | 205 | -5.53% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 8 | +60.00% |
| RenderSink/substitute | 0 |  | 0 |  | 0 |  | 5 | 0% |
| RenderTo/substitute | 0 |  | 0 |  | 0 |  | 7 | +40.00% |

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
