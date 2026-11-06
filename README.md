# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (187 so far, latest [93392d4](https://github.com/jinja2cpp/Jinja2Cpp/commit/93392d4a6f8110a38fc00fe79858266de5650867) on 2026-11-06). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 345,267 | 0% |  | 55 | 0% |  | 18,480 | 0% | -52.71% |
| [Render/chat_llama](#renderchat_llama) | 294,648 | -0.29% |  | 225 | 0% |  | 11,632 | +0.14% | -19.00% |
| [Load/chat_mistral](#loadchat_mistral) | 393,805 | -0.09% |  | 52 | 0% |  | 20,888 | 0% | -53.22% |
| [Render/chat_mistral](#renderchat_mistral) | 498,881 | -0.20% |  | 402 | 0% |  | 9,968 | -0.16% | -24.53% |
| [Load/chat_qwen](#loadchat_qwen) | 266,480 | +0.06% |  | 44 | 0% |  | 14,416 | 0% | -47.25% |
| [Render/chat_qwen](#renderchat_qwen) | 276,806 | +0.02% |  | 212 | 0% |  | 8,304 | 0% | -2.35% |
| [Load/config_file](#loadconfig_file) | 97,039 | -0.06% |  | 25 | 0% |  | 5,464 | 0% | -48.26% |
| [Render/config_file](#renderconfig_file) | 1,267,730 | -0.14% |  | 908 | 0% |  | 24,440 | 0% | +2.65% |
| [Load/dict_ops](#loaddict_ops) | 33,313 | -0.11% | -77.46% | 9 | 0% | -93.13% | 2,176 | 0% | -48.87% |
| [Render/dict_ops](#renderdict_ops) | 293,079 | +0.15% | -57.58% | 112 | 0% | -75.11% | 24,408 | 0% | -29.65% |
| [Load/expressions](#loadexpressions) | 45,015 | 0% | -78.16% | 16 | 0% | -89.04% | 2,728 | 0% | -55.54% |
| [Render/expressions](#renderexpressions) | 426,224 | 0% | -52.96% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 68,966 | -0.31% | -74.87% | 27 | 0% | -88.16% | 3,848 | 0% | -66.29% |
| [Render/filters](#renderfilters) | 30,408 | -0.04% | -65.71% | 26 | 0% | -64.86% | 1,080 | 0% | -78.01% |
| [Load/for_filter_if](#loadfor_filter_if) | 30,017 | +0.09% | -77.83% | 7 | 0% | -94.31% | 1,904 | 0% | -44.78% |
| [Render/for_filter_if](#renderfor_filter_if) | 273,874 | +0.01% | -68.36% | 4 | 0% | -98.23% | 16,560 | 0% | +354.95% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 40,311 | -0.12% | -77.69% | 10 | 0% | -93.63% | 2,368 | 0% | -47.61% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 180,540 | 0% | -83.17% | 2 | 0% | -99.62% | 1,568 | 0% | -43.35% |
| [Load/for_range](#loadfor_range) | 13,634 | +0.02% | -75.95% | 7 | 0% | -87.50% | 1,088 | 0% | -45.60% |
| [Render/for_range](#renderfor_range) | 38,378 | +0.01% | -58.38% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/hf_deepseek_v31](#loadhf_deepseek_v31) | 388,203 | +0.15% |  | 55 | 0% |  | 21,976 | 0% | +0.73% |
| [Render/hf_deepseek_v31](#renderhf_deepseek_v31) | 90,550 | -0.07% |  | 43 | 0% |  | 2,480 | 0% | 0% |
| [Load/hf_gpt_oss](#loadhf_gpt_oss) | 1,549,322 | -0.02% |  | 287 | 0% |  | 74,640 | -0.04% | +0.56% |
| [Render/hf_gpt_oss](#renderhf_gpt_oss) | 170,506 | +0.03% |  | 155 | 0% |  | 5,096 | +0.63% | +3.24% |
| [Load/hf_mistral_small](#loadhf_mistral_small) | 598,924 | +0.01% |  | 88 | 0% |  | 33,048 | 0% | +0.34% |
| [Render/hf_mistral_small](#renderhf_mistral_small) | 144,631 | -0.19% |  | 105 | 0% |  | 6,120 | 0% | -0.26% |
| [Load/hf_qwen3](#loadhf_qwen3) | 485,761 | +0.02% |  | 82 | 0% |  | 25,560 | 0% | +0.31% |
| [Render/hf_qwen3](#renderhf_qwen3) | 119,792 | 0% |  | 100 | 0% |  | 3,248 | 0% | 0% |
| [Load/html_autoescape](#loadhtml_autoescape) | 193,808 | -0.12% |  | 43 | 0% |  | 12,640 | 0% | -48.78% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,208,748 | -0.32% |  | 674 | 0% |  | 45,408 | 0% | +12.44% |
| [Load/inheritance](#loadinheritance) | 23,039 | 0% | -79.81% | 7 | 0% | -93.46% | 1,344 | 0% | -53.20% |
| [Render/inheritance](#renderinheritance) | 137,563 | -0.02% | -81.61% | 35 | 0% | -94.37% | 7,384 | 0% | +22.74% |
| [Load/int_arith](#loadint_arith) | 65,931 | -0.08% |  | 20 | 0% |  | 3,872 | 0% | +0.41% |
| [Render/int_arith](#renderint_arith) | 824,156 | -0.39% |  | 12 | 0% |  | 3,272 | 0% | -0.49% |
| [Module/keep](#modulekeep) | 6,992 | -0.11% |  | 7 | 0% |  |  |  |  |
| [Module/keep_written](#modulekeep_written) | 36,387 | 0% |  | 11 | 0% |  |  |  |  |
| [Load/large_static](#loadlarge_static) | 154,694 | +0.01% | -82.54% | 14 | 0% | -96.82% | 43,120 | 0% | -19.23% |
| [Render/large_static](#renderlarge_static) | 20,709 | 0% | -72.44% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [RenderSink/large_static](#rendersinklarge_static) | 12,699 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/large_static](#rendertolarge_static) | 20,272 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/macros](#loadmacros) | 56,324 | -0.01% | -75.29% | 27 | 0% | -85.48% | 4,392 | 0% | -29.34% |
| [Render/macros](#rendermacros) | 656,429 | 0% | -74.39% | 308 | 0% | -88.71% | 20,560 | 0% | +39.67% |
| [Load/many_tags](#loadmany_tags) | 9,007,376 | -0.12% | -79.61% | 951 | 0% | -97.22% | 438,160 | 0% | -58.48% |
| [Render/many_tags](#rendermany_tags) | 727,210 | 0% | -65.09% | 2 | 0% | -90.48% | 5,648 | 0% | -10.52% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 46,562 | 0% | -80.40% | 19 | 0% | -89.67% | 3,360 | 0% | -44.59% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,195,660 | 0% | -57.95% | 33 | 0% | -99.19% | 381,536 | 0% | +9.95% |
| [RenderSink/mitsuhiko_table](#rendersinkmitsuhiko_table) | 5,122,853 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table](#rendertomitsuhiko_table) | 5,195,164 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 54,820 | +0.03% |  | 35 | 0% |  | 5,688 | 0% | -31.17% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,541,825 | 0% |  | 33 | 0% |  | 1,414,928 | 0% | +2.46% |
| [RenderSink/mitsuhiko_table_wide](#rendersinkmitsuhiko_table_wide) | 5,365,114 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table_wide](#rendertomitsuhiko_table_wide) | 5,541,377 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/plain_text](#loadplain_text) | 3,713 | +0.08% | -61.63% | 4 | 0% | -77.78% | 672 | 0% | -40.00% |
| [Render/plain_text](#renderplain_text) | 1,019 | 0% | -85.00% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/plain_text](#rendersinkplain_text) | 721 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/plain_text](#rendertoplain_text) | 820 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/prompt_budget](#loadprompt_budget) | 35,142 |  |  | 18 |  |  | 2,224 |  | 0% |
| [Render/prompt_budget](#renderprompt_budget) | 1,314,285 |  |  | 76 |  |  | 36,416 |  | 0% |
| [Load/strings](#loadstrings) | 70,329 | -0.06% | -75.90% | 30 | 0% | -86.61% | 4,344 | 0% | -51.95% |
| [Render/strings](#renderstrings) | 578,677 | -0.25% | -72.41% | 344 | 0% | -83.73% | 9,088 | 0% | -49.31% |
| [Load/substitute](#loadsubstitute) | 7,746 | +0.04% | -74.06% | 6 | 0% | -83.78% | 832 | 0% | -45.26% |
| [Render/substitute](#rendersubstitute) | 2,252 | 0% | -74.45% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/substitute](#rendersinksubstitute) | 1,958 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/substitute](#rendertosubstitute) | 2,057 | 0% |  | 0 |  |  | 0 |  |  |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 721 | -0.83% | 748 | +2.89% | 0 |  | 7,497 | -4.64% |
| Render/chat_llama | 1,186 | -0.17% | 531 | +3.71% | 0 |  | 4,161 | -2.42% |
| Load/chat_mistral | 1,018 | -3.60% | 1,048 | +0.96% | 0 |  | 8,143 | -9.15% |
| Render/chat_mistral | 1,990 | +3.59% | 572 | -0.52% | 0 |  | 9,854 | -0.57% |
| Load/chat_qwen | 499 | -2.16% | 587 | -0.17% | 0 |  | 4,331 | -11.72% |
| Render/chat_qwen | 1,092 | -3.53% | 405 | -2.64% | 0 |  | 3,459 | -0.40% |
| Load/config_file | 198 | +6.45% | 170 | +1.80% | 0 |  | 2,669 | -5.25% |
| Render/config_file | 6,238 | -3.66% | 1,521 | -3.92% | 0 |  | 39,775 | -6.73% |
| Load/dict_ops | 33 | -25.00% | 23 | -25.81% | 0 |  | 1,348 | -2.67% |
| Render/dict_ops | 669 | +6.19% | 420 | -1.18% | 0 |  | 878 | -1.68% |
| Load/expressions | 14 | -41.67% | 7 | -56.25% | 0 |  | 1,282 | -5.53% |
| Render/expressions | 21 | +425.00% | 6 | +100.00% | 0 |  | 256 | -16.34% |
| Load/filters | 77 | -19.79% | 53 | -7.02% | 0 |  | 1,499 | -15.69% |
| Render/filters | 27 | +125.00% | 5 | +150.00% | 0 |  | 787 | -2.60% |
| Load/for_filter_if | 21 | -8.70% | 15 | -16.67% | 0 |  | 1,023 | -11.51% |
| Render/for_filter_if | 1,808 | +4.27% | 98 | +2.08% | 0 |  | 145 | -20.33% |
| Load/for_loop_vars | 28 | +47.37% | 13 | -13.33% | 0 |  | 1,097 | -7.66% |
| Render/for_loop_vars | 7 | -12.50% | 1 | -66.67% | 0 |  | 146 | -16.09% |
| Load/for_range | 3 | -25.00% | 4 | -20.00% | 0 |  | 747 | -0.13% |
| Render/for_range | 7 |  | 0 |  | 0 |  | 114 | -32.14% |
| Load/hf_deepseek_v31 | 973 | -0.61% | 966 | -1.02% | 0 |  | 5,983 | -10.71% |
| Render/hf_deepseek_v31 | 355 | +1.14% | 78 | -3.70% | 0 |  | 1,357 | -6.48% |
| Load/hf_gpt_oss | 4,629 | +0.46% | 3,293 | +0.12% | 0 |  | 21,495 | -5.78% |
| Render/hf_gpt_oss | 1,218 | +2.96% | 374 | +1.91% | 0 |  | 4,471 | -1.37% |
| Load/hf_mistral_small | 1,803 | +1.81% | 1,581 | -0.38% | 0 |  | 10,445 | -12.06% |
| Render/hf_mistral_small | 957 | -2.15% | 318 | -0.93% | 0 |  | 3,747 | -2.32% |
| Load/hf_qwen3 | 1,401 | -4.50% | 1,240 | +0.81% | 0 |  | 8,538 | -10.10% |
| Render/hf_qwen3 | 671 | +1.21% | 217 | +2.36% | 0 |  | 2,489 | -4.20% |
| Load/html_autoescape | 456 | +1.56% | 513 | -1.54% | 0 |  | 3,717 | -12.85% |
| Render/html_autoescape | 4,084 | +13.73% | 1,360 | +11.66% | 0 |  | 36,338 | -0.31% |
| Load/inheritance | 3 | +50.00% | 4 | +33.33% | 0 |  | 688 | +0.44% |
| Render/inheritance | 594 | +0.51% | 96 | -4.95% | 0 |  | 925 | -1.60% |
| Load/int_arith | 83 | -3.49% | 61 | +3.39% | 0 |  | 2,037 | -6.56% |
| Render/int_arith | 32 | +23.08% | 9 | -55.00% | 0 |  | 704 | -8.81% |
| Module/keep | 2 | -33.33% | 2 | 0% | 0 |  | 348 | -15.12% |
| Module/keep_written | 2,929 | -0.07% | 561 | -0.18% | 0 |  | 716 | -1.78% |
| Load/large_static | 1,927 | -0.52% | 908 | -0.33% | 0 |  | 235 | -13.92% |
| Render/large_static | 710 | -0.28% | 568 | 0% | 0 |  | 4 | 0% |
| RenderSink/large_static | 0 |  | 0 |  | 0 |  | 3 | +200.00% |
| RenderTo/large_static | 688 | -0.15% | 570 | -0.35% | 0 |  | 4 | 0% |
| Load/macros | 73 | -19.78% | 58 | -18.31% | 0 |  | 1,710 | -0.29% |
| Render/macros | 761 | 0% | 218 | -6.44% | 0 |  | 1,414 | -30.38% |
| Load/many_tags | 27,809 | -1.54% | 22,398 | -0.56% | 0 |  | 217,990 | -7.37% |
| Render/many_tags | 6,222 | -0.18% | 867 | +2.60% | 0 |  | 135 | -11.76% |
| Load/mitsuhiko_table | 35 | +59.09% | 35 | +75.00% | 0 |  | 893 | +4.08% |
| Render/mitsuhiko_table | 10,024 | -0.28% | 6,280 | +0.03% | 0 |  | 337 | -14.90% |
| RenderSink/mitsuhiko_table | 9,940 | +0.03% | 1,181 | -0.25% | 0 |  | 296 | -14.94% |
| RenderTo/mitsuhiko_table | 10,010 | -0.13% | 6,276 | -0.25% | 0 |  | 310 | -18.21% |
| Load/mitsuhiko_table_wide | 78 | +8.33% | 70 | -15.66% | 0 |  | 962 | -11.25% |
| Render/mitsuhiko_table_wide | 10,242 | -0.19% | 22,666 | -0.05% | 0 |  | 348 | -11.00% |
| RenderSink/mitsuhiko_table_wide | 10,057 | -0.15% | 1,295 | -1.15% | 0 |  | 252 | -22.22% |
| RenderTo/mitsuhiko_table_wide | 10,221 | -0.33% | 22,678 | 0% | 0 |  | 312 | -11.86% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 14 | +133.33% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 4 | -20.00% |
| RenderSink/plain_text | 0 |  | 0 |  | 0 |  | 4 | 0% |
| RenderTo/plain_text | 0 |  | 0 |  | 0 |  | 4 | 0% |
| Load/prompt_budget | 69 |  | 45 |  | 0 |  | 1,373 |  |
| Render/prompt_budget | 4,789 |  | 528 |  | 0 |  | 860 |  |
| Load/strings | 122 | -6.87% | 87 | -10.31% | 0 |  | 2,742 | -1.08% |
| Render/strings | 414 | +0.24% | 475 | +1.50% | 0 |  | 1,540 | -4.64% |
| Load/substitute | 1 | -50.00% | 4 | +33.33% | 0 |  | 160 | 0% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 4 | 0% |
| RenderSink/substitute | 0 |  | 0 |  | 0 |  | 3 | +200.00% |
| RenderTo/substitute | 0 |  | 0 |  | 0 |  | 4 | 0% |

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
