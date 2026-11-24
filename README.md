# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (266 so far, latest [486673d](https://github.com/jinja2cpp/Jinja2Cpp/commit/486673db5d2fe971b57321cf7dedb519ab3ebaaf) on 2026-11-24). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/big_int_radix](#loadbig_int_radix) | 43,283 | 0% |  | 14 | 0% |  | 2,688 | 0% | 0% |
| [Render/big_int_radix](#renderbig_int_radix) | 308,312,466 | 0% |  | 163 | 0% |  | 1,540,760 | 0% | -39.13% |
| [Load/chat_llama](#loadchat_llama) | 337,418 | 0% |  | 54 | 0% |  | 18,536 | 0% | -52.57% |
| [Render/chat_llama](#renderchat_llama) | 292,519 | 0% |  | 224 | 0% |  | 11,608 | 0% | -19.16% |
| [Load/chat_mistral](#loadchat_mistral) | 385,885 | 0% |  | 51 | 0% |  | 20,976 | 0% | -53.02% |
| [Render/chat_mistral](#renderchat_mistral) | 492,902 | 0% |  | 392 | 0% |  | 9,752 | 0% | -26.17% |
| [Load/chat_qwen](#loadchat_qwen) | 262,663 | 0% |  | 43 | 0% |  | 14,472 | 0% | -47.04% |
| [Render/chat_qwen](#renderchat_qwen) | 274,289 | 0% |  | 211 | 0% |  | 8,280 | 0% | -2.63% |
| [Load/config_file](#loadconfig_file) | 94,403 | 0% |  | 24 | 0% |  | 5,520 | 0% | -47.73% |
| [Render/config_file](#renderconfig_file) | 1,278,446 | 0% |  | 908 | 0% |  | 25,960 | 0% | +9.04% |
| [Load/dict_ops](#loaddict_ops) | 31,968 | 0% | -78.37% | 8 | 0% | -93.89% | 2,216 | 0% | -47.93% |
| [Render/dict_ops](#renderdict_ops) | 290,693 | 0% | -57.92% | 112 | 0% | -75.11% | 24,408 | 0% | -29.65% |
| [Load/expressions](#loadexpressions) | 44,175 | 0% | -78.57% | 15 | 0% | -89.73% | 2,752 | 0% | -55.15% |
| [Render/expressions](#renderexpressions) | 414,401 | 0% | -54.27% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 67,517 | 0% | -75.40% | 26 | 0% | -88.60% | 3,856 | 0% | -66.22% |
| [Render/filters](#renderfilters) | 26,520 | 0% | -70.09% | 23 | 0% | -68.92% | 752 | 0% | -84.69% |
| [Load/for_filter_if](#loadfor_filter_if) | 28,571 | 0% | -78.90% | 6 | 0% | -95.12% | 1,928 | 0% | -44.08% |
| [Render/for_filter_if](#renderfor_filter_if) | 266,156 | 0% | -69.25% | 4 | 0% | -98.23% | 16,560 | 0% | +354.95% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 38,233 | 0% | -78.84% | 9 | 0% | -94.27% | 2,392 | 0% | -47.08% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 170,803 | 0% | -84.08% | 2 | 0% | -99.62% | 1,568 | 0% | -43.35% |
| [Load/for_range](#loadfor_range) | 12,526 | 0% | -77.90% | 6 | 0% | -89.29% | 1,128 | 0% | -43.60% |
| [Render/for_range](#renderfor_range) | 37,183 | 0% | -59.68% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/hf_deepseek_v31](#loadhf_deepseek_v31) | 384,912 | 0% |  | 54 | 0% |  | 22,032 | 0% | +0.99% |
| [Render/hf_deepseek_v31](#renderhf_deepseek_v31) | 90,602 | +0.01% |  | 42 | 0% |  | 2,456 | 0% | -0.97% |
| [Load/hf_gpt_oss](#loadhf_gpt_oss) | 1,528,022 | -0.02% |  | 282 | 0% |  | 75,000 | -0.02% | +1.05% |
| [Render/hf_gpt_oss](#renderhf_gpt_oss) | 169,231 | 0% |  | 153 | 0% |  | 5,984 | 0% | +21.23% |
| [Load/hf_mistral_small](#loadhf_mistral_small) | 592,679 | 0% |  | 87 | 0% |  | 33,136 | 0% | +0.61% |
| [Render/hf_mistral_small](#renderhf_mistral_small) | 140,369 | 0% |  | 97 | 0% |  | 6,096 | 0% | -0.65% |
| [Load/hf_qwen3](#loadhf_qwen3) | 478,885 | 0% |  | 81 | 0% |  | 25,648 | 0% | +0.66% |
| [Render/hf_qwen3](#renderhf_qwen3) | 117,522 | 0% |  | 99 | 0% |  | 3,224 | 0% | -0.74% |
| [Load/html_autoescape](#loadhtml_autoescape) | 189,392 | 0% |  | 42 | 0% |  | 12,728 | 0% | -48.43% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,197,370 | 0% |  | 674 | 0% |  | 46,256 | 0% | +14.54% |
| [Load/inheritance](#loadinheritance) | 21,312 | 0% | -81.32% | 6 | 0% | -94.39% | 1,368 | 0% | -52.37% |
| [Render/inheritance](#renderinheritance) | 136,802 | 0% | -81.71% | 35 | 0% | -94.37% | 8,168 | 0% | +35.77% |
| [Load/int_arith](#loadint_arith) | 65,188 | 0% |  | 19 | 0% |  | 3,896 | 0% | +1.04% |
| [Render/int_arith](#renderint_arith) | 824,338 | 0% |  | 12 | 0% |  | 3,272 | 0% | -0.49% |
| [Module/keep](#modulekeep) | 7,082 | +1.33% |  | 6 | 0% |  |  |  |  |
| [Module/keep_written](#modulekeep_written) | 29,081 | +0.32% |  | 10 | 0% |  |  |  |  |
| [Load/large_static](#loadlarge_static) | 147,158 | 0% | -83.39% | 13 | 0% | -97.05% | 43,128 | 0% | -19.21% |
| [Render/large_static](#renderlarge_static) | 20,342 | 0% | -72.93% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [RenderSink/large_static](#rendersinklarge_static) | 12,631 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/large_static](#rendertolarge_static) | 19,845 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/macros](#loadmacros) | 54,385 | 0% | -76.14% | 25 | 0% | -86.56% | 4,472 | 0% | -28.06% |
| [Render/macros](#rendermacros) | 647,392 | 0% | -74.74% | 308 | 0% | -88.71% | 20,608 | 0% | +40.00% |
| [Load/many_tags](#loadmany_tags) | 8,785,186 | 0% | -80.11% | 950 | 0% | -97.23% | 438,136 | 0% | -58.48% |
| [Render/many_tags](#rendermany_tags) | 730,657 | 0% | -64.92% | 2 | 0% | -90.48% | 5,648 | 0% | -10.52% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 44,693 | 0% | -81.19% | 18 | 0% | -90.22% | 3,416 | 0% | -43.67% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,105,184 | 0% | -58.68% | 33 | 0% | -99.19% | 381,536 | 0% | +9.95% |
| [RenderSink/mitsuhiko_table](#rendersinkmitsuhiko_table) | 5,035,279 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table](#rendertomitsuhiko_table) | 5,104,640 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 52,178 | 0% |  | 34 | 0% |  | 5,744 | 0% | -30.49% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,451,418 | 0% |  | 33 | 0% |  | 1,414,928 | 0% | +2.46% |
| [RenderSink/mitsuhiko_table_wide](#rendersinkmitsuhiko_table_wide) | 5,277,323 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table_wide](#rendertomitsuhiko_table_wide) | 5,450,812 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Nested/new_template](#nestednew_template) | 452,836 | 0% |  | 323 | 0% |  |  |  |  |
| [Load/plain_text](#loadplain_text) | 2,932 | 0% | -69.70% | 3 | 0% | -83.33% | 680 | 0% | -39.29% |
| [Render/plain_text](#renderplain_text) | 972 | 0% | -85.69% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/plain_text](#rendersinkplain_text) | 677 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/plain_text](#rendertoplain_text) | 773 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/prompt_budget](#loadprompt_budget) | 34,335 | 0% |  | 17 | 0% |  | 2,248 | 0% | +1.08% |
| [Render/prompt_budget](#renderprompt_budget) | 1,321,775 | 0% |  | 76 | 0% |  | 36,416 | 0% | 0% |
| [Nested/shared_globals](#nestedshared_globals) | 160,756 | -0.01% |  | 160 | 0% |  |  |  |  |
| [Load/strings](#loadstrings) | 69,091 | 0% | -76.33% | 29 | 0% | -87.05% | 4,384 | 0% | -51.50% |
| [Render/strings](#renderstrings) | 581,069 | 0% | -72.30% | 344 | 0% | -83.73% | 9,088 | 0% | -49.31% |
| [Load/substitute](#loadsubstitute) | 6,662 | 0% | -77.69% | 5 | 0% | -86.49% | 840 | 0% | -44.74% |
| [Render/substitute](#rendersubstitute) | 1,653 | 0% | -81.24% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/substitute](#rendersinksubstitute) | 1,362 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/substitute](#rendertosubstitute) | 1,458 | 0% |  | 0 |  |  | 0 |  |  |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/big_int_radix | 105 | +20.69% | 58 | +7.41% | 0 |  | 1,969 | -3.62% |
| Render/big_int_radix | 774,934 | +0.01% | 515,277 | 0% | 0 |  | 3,196 | -22.01% |
| Load/chat_llama | 788 | +2.87% | 776 | +1.57% | 0 |  | 7,992 | +0.49% |
| Render/chat_llama | 1,251 | +4.77% | 575 | +4.17% | 0 |  | 4,433 | +1.03% |
| Load/chat_mistral | 1,078 | -3.06% | 1,094 | -1.44% | 0 |  | 8,876 | -0.18% |
| Render/chat_mistral | 2,016 | -2.89% | 656 | -1.65% | 0 |  | 10,848 | +3.60% |
| Load/chat_qwen | 535 | -3.08% | 648 | -0.77% | 0 |  | 5,044 | +5.24% |
| Render/chat_qwen | 1,098 | -1.26% | 426 | -1.84% | 0 |  | 3,549 | -2.12% |
| Load/config_file | 196 | -5.31% | 190 | +2.15% | 0 |  | 2,808 | -1.20% |
| Render/config_file | 7,440 | +10.24% | 1,927 | +2.66% | 0 |  | 44,933 | -1.91% |
| Load/dict_ops | 33 | +17.86% | 18 | -5.26% | 0 |  | 1,373 | -2.35% |
| Render/dict_ops | 694 | +0.14% | 419 | -0.48% | 0 |  | 822 | +0.98% |
| Load/expressions | 34 | +30.77% | 27 | +35.00% | 0 |  | 1,300 | -5.39% |
| Render/expressions | 3 | -50.00% | 0 | -100.00% | 0 |  | 278 | -13.12% |
| Load/filters | 98 | +10.11% | 53 | +12.77% | 0 |  | 1,891 | -4.83% |
| Render/filters | 22 | +46.67% | 7 | +40.00% | 0 |  | 766 | +2.13% |
| Load/for_filter_if | 31 | +138.46% | 8 | +33.33% | 0 |  | 1,084 | +0.46% |
| Render/for_filter_if | 1,723 | +0.35% | 87 | 0% | 0 |  | 173 | +13.07% |
| Load/for_loop_vars | 49 | +53.12% | 17 | +13.33% | 0 |  | 1,119 | -0.97% |
| Render/for_loop_vars | 3 | -57.14% | 0 |  | 0 |  | 175 | +10.76% |
| Load/for_range | 11 | +450.00% | 5 | +66.67% | 0 |  | 745 | -1.19% |
| Render/for_range | 2 | 0% | 1 | 0% | 0 |  | 186 | +16.98% |
| Load/hf_deepseek_v31 | 951 | -2.86% | 962 | -1.54% | 0 |  | 6,473 | -3.04% |
| Render/hf_deepseek_v31 | 386 | +0.26% | 91 | +2.25% | 0 |  | 1,535 | -0.32% |
| Load/hf_gpt_oss | 4,695 | -1.16% | 3,399 | -0.26% | 0 |  | 24,238 | +3.17% |
| Render/hf_gpt_oss | 1,316 | +0.69% | 411 | +1.23% | 0 |  | 5,035 | +1.84% |
| Load/hf_mistral_small | 1,854 | -2.42% | 1,645 | -0.54% | 0 |  | 11,627 | +0.28% |
| Render/hf_mistral_small | 1,032 | +1.98% | 317 | -0.94% | 0 |  | 3,816 | -0.68% |
| Load/hf_qwen3 | 1,479 | -3.02% | 1,296 | -0.92% | 0 |  | 9,670 | +4.21% |
| Render/hf_qwen3 | 730 | +4.14% | 233 | +5.43% | 0 |  | 2,790 | +1.23% |
| Load/html_autoescape | 482 | -0.62% | 540 | -1.82% | 0 |  | 4,272 | -2.24% |
| Render/html_autoescape | 4,289 | -3.44% | 1,661 | -2.58% | 0 |  | 37,911 | +3.25% |
| Load/inheritance | 12 | +140.00% | 6 | +50.00% | 0 |  | 662 | -2.07% |
| Render/inheritance | 597 | -0.67% | 119 | +0.85% | 0 |  | 1,034 | -0.10% |
| Load/int_arith | 84 | +6.33% | 81 | +17.39% | 0 |  | 2,153 | -0.37% |
| Render/int_arith | 33 | -2.94% | 19 | -17.39% | 0 |  | 817 | +5.56% |
| Module/keep | 2 | 0% | 2 | 0% | 0 |  | 455 | +7.57% |
| Module/keep_written | 1,141 | -0.52% | 571 | +2.33% | 0 |  | 771 | +2.12% |
| Load/large_static | 1,919 | 0% | 910 | 0% | 0 |  | 254 | -11.19% |
| Render/large_static | 700 | 0% | 570 | 0% | 0 |  | 7 | +40.00% |
| RenderSink/large_static | 2 | 0% | 0 |  | 0 |  | 3 | 0% |
| RenderTo/large_static | 679 | 0% | 581 | 0% | 0 |  | 5 | +25.00% |
| Load/macros | 81 | -1.22% | 72 | +9.09% | 0 |  | 1,762 | +1.26% |
| Render/macros | 761 | +0.13% | 269 | +0.37% | 0 |  | 998 | +64.69% |
| Load/many_tags | 28,017 | +0.63% | 22,568 | 0% | 0 |  | 251,607 | +1.99% |
| Render/many_tags | 6,101 | -0.03% | 778 | -0.89% | 0 |  | 3,848 | +2815.15% |
| Load/mitsuhiko_table | 47 | +27.03% | 48 | +6.67% | 0 |  | 899 | +7.79% |
| Render/mitsuhiko_table | 10,049 | +0.02% | 6,334 | -0.11% | 0 |  | 308 | -9.68% |
| RenderSink/mitsuhiko_table | 9,967 | 0% | 1,204 | +0.50% | 0 |  | 260 | -7.80% |
| RenderTo/mitsuhiko_table | 10,057 | +0.04% | 6,285 | +0.02% | 0 |  | 281 | -6.02% |
| Load/mitsuhiko_table_wide | 85 | +13.33% | 86 | +4.88% | 0 |  | 1,016 | -1.93% |
| Render/mitsuhiko_table_wide | 10,320 | +1.18% | 22,602 | +0.01% | 0 |  | 352 | +11.75% |
| RenderSink/mitsuhiko_table_wide | 10,027 | -0.07% | 1,269 | +0.55% | 0 |  | 274 | -5.52% |
| RenderTo/mitsuhiko_table_wide | 10,182 | +0.02% | 22,670 | -0.01% | 0 |  | 335 | +9.84% |
| Nested/new_template | 2,249 | -1.58% | 1,649 | -0.66% | 0 |  | 24,356 | +0.45% |
| Load/plain_text | 0 |  | 1 | 0% | 0 |  | 10 | -50.00% |
| Render/plain_text | 1 | 0% | 0 |  | 0 |  | 9 | +80.00% |
| RenderSink/plain_text | 1 | 0% | 0 |  | 0 |  | 6 | 0% |
| RenderTo/plain_text | 2 | +100.00% | 0 |  | 0 |  | 7 | +75.00% |
| Load/prompt_budget | 73 | +1.39% | 49 | 0% | 0 |  | 1,418 | -2.00% |
| Render/prompt_budget | 4,797 | +0.04% | 549 | +0.55% | 0 |  | 1,179 | +15.93% |
| Nested/shared_globals | 140 | -43.55% | 110 | -23.08% | 0 |  | 9,164 | -0.07% |
| Load/strings | 141 | -14.02% | 76 | -9.52% | 0 |  | 2,833 | -0.56% |
| Render/strings | 425 | -1.16% | 481 | -0.41% | 0 |  | 1,559 | -2.32% |
| Load/substitute | 7 | +600.00% | 2 | 0% | 0 |  | 129 | +15.18% |
| Render/substitute | 1 | 0% | 0 |  | 0 |  | 6 | +50.00% |
| RenderSink/substitute | 1 | 0% | 0 |  | 0 |  | 3 | 0% |
| RenderTo/substitute | 1 | -50.00% | 0 |  | 0 |  | 5 | +25.00% |

## Charts

### Load/big_int_radix

![Load/big_int_radix](charts/Load_big_int_radix.svg)

### Render/big_int_radix

![Render/big_int_radix](charts/Render_big_int_radix.svg)

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

### Nested/new_template

![Nested/new_template](charts/Nested_new_template.svg)

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

### Nested/shared_globals

![Nested/shared_globals](charts/Nested_shared_globals.svg)

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
