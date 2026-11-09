# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (192 so far, latest [c299890](https://github.com/jinja2cpp/Jinja2Cpp/commit/c2998905c987bf27c15a62916b70c630b0ed10e4) on 2026-11-09). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 345,107 | -0.04% |  | 55 | 0% |  | 18,480 | 0% | -52.71% |
| [Render/chat_llama](#renderchat_llama) | 294,676 | -0.13% |  | 225 | 0% |  | 11,616 | -0.14% | -19.11% |
| [Load/chat_mistral](#loadchat_mistral) | 393,830 | -0.03% |  | 52 | 0% |  | 20,888 | 0% | -53.22% |
| [Render/chat_mistral](#renderchat_mistral) | 499,976 | -0.05% |  | 402 | 0% |  | 9,984 | +0.16% | -24.41% |
| [Load/chat_qwen](#loadchat_qwen) | 266,450 | -0.08% |  | 44 | 0% |  | 14,416 | 0% | -47.25% |
| [Render/chat_qwen](#renderchat_qwen) | 276,951 | -0.09% |  | 212 | 0% |  | 8,304 | 0% | -2.35% |
| [Load/config_file](#loadconfig_file) | 97,135 | -0.10% |  | 25 | 0% |  | 5,464 | 0% | -48.26% |
| [Render/config_file](#renderconfig_file) | 1,272,012 | -0.16% |  | 908 | 0% |  | 24,440 | 0% | +2.65% |
| [Load/dict_ops](#loaddict_ops) | 33,334 | 0% | -77.44% | 9 | 0% | -93.13% | 2,176 | 0% | -48.87% |
| [Render/dict_ops](#renderdict_ops) | 293,035 | 0% | -57.58% | 112 | 0% | -75.11% | 24,408 | 0% | -29.65% |
| [Load/expressions](#loadexpressions) | 45,038 | 0% | -78.15% | 16 | 0% | -89.04% | 2,728 | 0% | -55.54% |
| [Render/expressions](#renderexpressions) | 426,191 | 0% | -52.97% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 69,028 | -0.11% | -74.85% | 27 | 0% | -88.16% | 3,848 | 0% | -66.29% |
| [Render/filters](#renderfilters) | 30,561 | +0.12% | -65.53% | 26 | 0% | -64.86% | 1,080 | 0% | -78.01% |
| [Load/for_filter_if](#loadfor_filter_if) | 30,043 | +0.03% | -77.81% | 7 | 0% | -94.31% | 1,904 | 0% | -44.78% |
| [Render/for_filter_if](#renderfor_filter_if) | 273,473 | -0.22% | -68.40% | 4 | 0% | -98.23% | 16,560 | 0% | +354.95% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 40,456 | +0.14% | -77.61% | 10 | 0% | -93.63% | 2,368 | 0% | -47.61% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 180,545 | 0% | -83.17% | 2 | 0% | -99.62% | 1,568 | 0% | -43.35% |
| [Load/for_range](#loadfor_range) | 13,657 | 0% | -75.91% | 7 | 0% | -87.50% | 1,088 | 0% | -45.60% |
| [Render/for_range](#renderfor_range) | 38,345 | -0.02% | -58.42% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/hf_deepseek_v31](#loadhf_deepseek_v31) | 387,714 | -0.14% |  | 55 | 0% |  | 21,976 | 0% | +0.73% |
| [Render/hf_deepseek_v31](#renderhf_deepseek_v31) | 90,824 | -0.14% |  | 43 | 0% |  | 2,480 | 0% | 0% |
| [Load/hf_gpt_oss](#loadhf_gpt_oss) | 1,548,809 | -0.07% |  | 287 | 0% |  | 74,656 | +0.02% | +0.58% |
| [Render/hf_gpt_oss](#renderhf_gpt_oss) | 170,787 | -0.08% |  | 155 | 0% |  | 5,080 | -0.31% | +2.92% |
| [Load/hf_mistral_small](#loadhf_mistral_small) | 598,926 | -0.03% |  | 88 | 0% |  | 33,048 | 0% | +0.34% |
| [Render/hf_mistral_small](#renderhf_mistral_small) | 145,134 | +0.02% |  | 105 | 0% |  | 6,136 | +0.26% | 0% |
| [Load/hf_qwen3](#loadhf_qwen3) | 486,086 | -0.01% |  | 82 | 0% |  | 25,560 | 0% | +0.31% |
| [Render/hf_qwen3](#renderhf_qwen3) | 119,861 | -0.11% |  | 100 | 0% |  | 3,248 | 0% | 0% |
| [Load/html_autoescape](#loadhtml_autoescape) | 194,151 | +0.06% |  | 43 | 0% |  | 12,640 | 0% | -48.78% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,208,351 | -0.09% |  | 674 | 0% |  | 45,408 | 0% | +12.44% |
| [Load/inheritance](#loadinheritance) | 23,065 | 0% | -79.79% | 7 | 0% | -93.46% | 1,344 | 0% | -53.20% |
| [Render/inheritance](#renderinheritance) | 137,395 | -0.14% | -81.63% | 35 | 0% | -94.37% | 7,384 | 0% | +22.74% |
| [Load/int_arith](#loadint_arith) | 65,984 | -0.03% |  | 20 | 0% |  | 3,872 | 0% | +0.41% |
| [Render/int_arith](#renderint_arith) | 825,752 | -0.01% |  | 12 | 0% |  | 3,272 | 0% | -0.49% |
| [Module/keep](#modulekeep) | 7,027 | +0.44% |  | 7 | 0% |  |  |  |  |
| [Module/keep_written](#modulekeep_written) | 36,420 | +0.04% |  | 11 | 0% |  |  |  |  |
| [Load/large_static](#loadlarge_static) | 154,711 | -0.01% | -82.54% | 14 | 0% | -96.82% | 43,120 | 0% | -19.23% |
| [Render/large_static](#renderlarge_static) | 20,663 | -0.23% | -72.50% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [RenderSink/large_static](#rendersinklarge_static) | 12,701 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/large_static](#rendertolarge_static) | 20,274 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/macros](#loadmacros) | 56,355 | -0.09% | -75.28% | 27 | 0% | -85.48% | 4,392 | 0% | -29.34% |
| [Render/macros](#rendermacros) | 656,200 | -0.05% | -74.40% | 308 | 0% | -88.71% | 20,560 | 0% | +39.67% |
| [Load/many_tags](#loadmany_tags) | 9,011,753 | +0.05% | -79.60% | 951 | 0% | -97.22% | 438,128 | -0.01% | -58.48% |
| [Render/many_tags](#rendermany_tags) | 727,212 | -0.20% | -65.09% | 2 | 0% | -90.48% | 5,648 | 0% | -10.52% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 46,592 | +0.01% | -80.39% | 19 | 0% | -89.67% | 3,360 | 0% | -44.59% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,194,655 | -0.04% | -57.96% | 33 | 0% | -99.19% | 381,536 | 0% | +9.95% |
| [RenderSink/mitsuhiko_table](#rendersinkmitsuhiko_table) | 5,121,848 | -0.04% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table](#rendertomitsuhiko_table) | 5,194,161 | -0.04% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 54,825 | -0.03% |  | 35 | 0% |  | 5,688 | 0% | -31.17% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,540,856 | -0.04% |  | 33 | 0% |  | 1,414,928 | 0% | +2.46% |
| [RenderSink/mitsuhiko_table_wide](#rendersinkmitsuhiko_table_wide) | 5,364,145 | -0.04% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table_wide](#rendertomitsuhiko_table_wide) | 5,540,408 | -0.04% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/plain_text](#loadplain_text) | 3,730 | 0% | -61.45% | 4 | 0% | -77.78% | 672 | 0% | -40.00% |
| [Render/plain_text](#renderplain_text) | 1,021 | 0% | -84.97% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/plain_text](#rendersinkplain_text) | 723 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/plain_text](#rendertoplain_text) | 822 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/prompt_budget](#loadprompt_budget) | 35,116 | -0.02% |  | 18 | 0% |  | 2,224 | 0% | 0% |
| [Render/prompt_budget](#renderprompt_budget) | 1,316,571 | +0.01% |  | 76 | 0% |  | 36,416 | 0% | 0% |
| [Load/strings](#loadstrings) | 70,385 | -0.11% | -75.88% | 30 | 0% | -86.61% | 4,344 | 0% | -51.95% |
| [Render/strings](#renderstrings) | 580,595 | +0.06% | -72.32% | 344 | 0% | -83.73% | 9,088 | 0% | -49.31% |
| [Load/substitute](#loadsubstitute) | 7,763 | 0% | -74.00% | 6 | 0% | -83.78% | 832 | 0% | -45.26% |
| [Render/substitute](#rendersubstitute) | 2,254 | 0% | -74.43% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/substitute](#rendersinksubstitute) | 1,960 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/substitute](#rendertosubstitute) | 2,059 | 0% |  | 0 |  |  | 0 |  |  |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 721 | -1.10% | 734 | -2.26% | 0 |  | 7,437 | -0.11% |
| Render/chat_llama | 1,205 | +5.33% | 525 | -2.05% | 0 |  | 3,756 | -8.70% |
| Load/chat_mistral | 1,036 | -0.77% | 1,049 | 0% | 0 |  | 8,436 | +1.57% |
| Render/chat_mistral | 2,006 | +5.63% | 580 | +5.65% | 0 |  | 9,635 | -2.97% |
| Load/chat_qwen | 513 | +5.77% | 580 | -1.86% | 0 |  | 4,435 | +2.95% |
| Render/chat_qwen | 1,102 | +4.06% | 409 | +4.60% | 0 |  | 3,283 | -3.18% |
| Load/config_file | 209 | +3.98% | 170 | +0.59% | 0 |  | 2,676 | +1.29% |
| Render/config_file | 5,928 | -8.15% | 1,465 | -6.63% | 0 |  | 41,974 | +3.48% |
| Load/dict_ops | 34 | -5.56% | 21 | -4.55% | 0 |  | 1,333 | +0.60% |
| Render/dict_ops | 691 | +1.62% | 417 | -0.48% | 0 |  | 877 | -0.79% |
| Load/expressions | 26 | +73.33% | 21 | +162.50% | 0 |  | 1,327 | -1.04% |
| Render/expressions | 13 | 0% | 7 | +16.67% | 0 |  | 303 | -0.33% |
| Load/filters | 89 | +20.27% | 42 | -8.70% | 0 |  | 1,604 | -14.64% |
| Render/filters | 26 | -7.14% | 6 | +50.00% | 0 |  | 754 | -3.58% |
| Load/for_filter_if | 38 | +58.33% | 17 | -15.00% | 0 |  | 1,073 | +2.29% |
| Render/for_filter_if | 1,799 | +0.17% | 95 | -3.06% | 0 |  | 177 | +24.65% |
| Load/for_loop_vars | 52 | +57.58% | 29 | +81.25% | 0 |  | 1,079 | +1.22% |
| Render/for_loop_vars | 12 | +200.00% | 4 | +300.00% | 0 |  | 122 | -4.69% |
| Load/for_range | 3 | 0% | 4 | +33.33% | 0 |  | 741 | -3.77% |
| Render/for_range | 1 | 0% | 0 |  | 0 |  | 104 | -14.05% |
| Load/hf_deepseek_v31 | 956 | -2.65% | 961 | -1.33% | 0 |  | 6,016 | -0.20% |
| Render/hf_deepseek_v31 | 351 | +1.74% | 76 | -6.17% | 0 |  | 1,430 | +3.03% |
| Load/hf_gpt_oss | 4,622 | +0.85% | 3,277 | -0.27% | 0 |  | 22,087 | +6.70% |
| Render/hf_gpt_oss | 1,216 | +1.50% | 367 | -3.93% | 0 |  | 4,476 | +1.06% |
| Load/hf_mistral_small | 1,804 | -0.99% | 1,599 | +0.50% | 0 |  | 11,119 | +0.94% |
| Render/hf_mistral_small | 987 | +3.13% | 319 | -0.93% | 0 |  | 3,850 | +0.71% |
| Load/hf_qwen3 | 1,459 | +4.36% | 1,244 | +0.08% | 0 |  | 8,563 | -0.42% |
| Render/hf_qwen3 | 681 | +4.93% | 224 | +0.90% | 0 |  | 2,524 | -0.67% |
| Load/html_autoescape | 455 | +2.02% | 503 | -3.08% | 0 |  | 3,881 | +1.92% |
| Render/html_autoescape | 4,138 | +12.23% | 1,349 | +10.66% | 0 |  | 35,794 | -2.17% |
| Load/inheritance | 3 | +50.00% | 4 | +33.33% | 0 |  | 684 | +1.94% |
| Render/inheritance | 586 | -0.68% | 95 | 0% | 0 |  | 874 | -3.96% |
| Load/int_arith | 81 | +35.00% | 44 | +12.82% | 0 |  | 2,047 | -4.75% |
| Render/int_arith | 15 | -37.50% | 4 | -60.00% | 0 |  | 732 | -2.14% |
| Module/keep | 1 | -50.00% | 3 | +50.00% | 0 |  | 385 | -11.09% |
| Module/keep_written | 2,932 | +0.03% | 556 | -0.89% | 0 |  | 742 | +4.36% |
| Load/large_static | 1,936 | +0.41% | 906 | -0.77% | 0 |  | 258 | -17.04% |
| Render/large_static | 709 | +0.14% | 573 | +0.70% | 0 |  | 6 | +100.00% |
| RenderSink/large_static | 0 |  | 0 |  | 0 |  | 2 | 0% |
| RenderTo/large_static | 686 | 0% | 568 | -0.53% | 0 |  | 5 | +66.67% |
| Load/macros | 83 | +15.28% | 59 | +9.26% | 0 |  | 1,730 | +2.31% |
| Render/macros | 763 | 0% | 218 | +0.93% | 0 |  | 1,265 | -71.88% |
| Load/many_tags | 27,751 | +0.18% | 22,468 | +0.42% | 0 |  | 219,098 | -4.20% |
| Render/many_tags | 6,217 | -0.03% | 871 | +0.81% | 0 |  | 119 | +4.39% |
| Load/mitsuhiko_table | 43 | +48.28% | 38 | +26.67% | 0 |  | 868 | +4.83% |
| Render/mitsuhiko_table | 10,024 | +0.05% | 6,278 | +0.05% | 0 |  | 365 | +9.28% |
| RenderSink/mitsuhiko_table | 9,949 | -0.02% | 1,184 | 0% | 0 |  | 269 | +0.37% |
| RenderTo/mitsuhiko_table | 10,021 | +0.02% | 6,292 | +0.14% | 0 |  | 336 | +13.51% |
| Load/mitsuhiko_table_wide | 79 | +12.86% | 76 | +16.92% | 0 |  | 966 | -5.39% |
| Render/mitsuhiko_table_wide | 10,223 | -0.19% | 22,663 | -0.01% | 0 |  | 354 | +12.38% |
| RenderSink/mitsuhiko_table_wide | 10,077 | +0.04% | 1,294 | -0.23% | 0 |  | 271 | +7.54% |
| RenderTo/mitsuhiko_table_wide | 10,232 | 0% | 22,676 | -0.02% | 0 |  | 315 | +5.70% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 27 | -10.00% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 7 | +16.67% |
| RenderSink/plain_text | 0 |  | 0 |  | 0 |  | 4 | 0% |
| RenderTo/plain_text | 0 |  | 0 |  | 0 |  | 6 | 0% |
| Load/prompt_budget | 75 | +22.95% | 39 | +14.71% | 0 |  | 1,350 | -2.03% |
| Render/prompt_budget | 4,800 | +0.29% | 526 | -0.19% | 0 |  | 907 | -3.61% |
| Load/strings | 134 | +10.74% | 92 | +13.58% | 0 |  | 2,772 | -0.36% |
| Render/strings | 419 | +1.70% | 481 | +1.05% | 0 |  | 1,586 | +1.67% |
| Load/substitute | 1 | 0% | 3 | +50.00% | 0 |  | 124 | -31.11% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 4 | +33.33% |
| RenderSink/substitute | 0 |  | 0 |  | 0 |  | 2 | 0% |
| RenderTo/substitute | 0 |  | 0 |  | 0 |  | 4 | +33.33% |

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
