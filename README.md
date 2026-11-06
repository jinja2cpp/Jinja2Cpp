# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (186 so far, latest [3660cfd](https://github.com/jinja2cpp/Jinja2Cpp/commit/3660cfd2d0a2bd52bbf9f449e770823b5d31f983) on 2026-11-06). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 345,260 | +0.19% |  | 55 | 0% |  | 18,480 | 0% | -52.71% |
| [Render/chat_llama](#renderchat_llama) | 295,515 | -0.20% |  | 225 | 0% |  | 11,616 | 0% | -19.11% |
| [Load/chat_mistral](#loadchat_mistral) | 394,153 | +0.22% |  | 52 | 0% |  | 20,888 | 0% | -53.22% |
| [Render/chat_mistral](#renderchat_mistral) | 499,862 | 0% |  | 402 | 0% |  | 9,984 | 0% | -24.41% |
| [Load/chat_qwen](#loadchat_qwen) | 266,323 | +0.20% |  | 44 | 0% |  | 14,416 | 0% | -47.25% |
| [Render/chat_qwen](#renderchat_qwen) | 276,752 | -0.03% |  | 212 | 0% |  | 8,304 | 0% | -2.35% |
| [Load/config_file](#loadconfig_file) | 97,096 | +0.12% |  | 25 | 0% |  | 5,464 | 0% | -48.26% |
| [Render/config_file](#renderconfig_file) | 1,269,560 | +0.05% |  | 908 | 0% |  | 24,440 | 0% | +2.65% |
| [Load/dict_ops](#loaddict_ops) | 33,350 | +0.10% | -77.43% | 9 | 0% | -93.13% | 2,176 | 0% | -48.87% |
| [Render/dict_ops](#renderdict_ops) | 292,652 | -0.02% | -57.64% | 112 | 0% | -75.11% | 24,408 | 0% | -29.65% |
| [Load/expressions](#loadexpressions) | 45,014 | +0.08% | -78.16% | 16 | 0% | -89.04% | 2,728 | 0% | -55.54% |
| [Render/expressions](#renderexpressions) | 426,224 | 0% | -52.96% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 69,182 | +0.17% | -74.79% | 27 | 0% | -88.16% | 3,848 | 0% | -66.29% |
| [Render/filters](#renderfilters) | 30,419 | +0.11% | -65.69% | 26 | 0% | -64.86% | 1,080 | 0% | -78.01% |
| [Load/for_filter_if](#loadfor_filter_if) | 29,991 | +0.03% | -77.85% | 7 | 0% | -94.31% | 1,904 | 0% | -44.78% |
| [Render/for_filter_if](#renderfor_filter_if) | 273,854 | -0.29% | -68.36% | 4 | 0% | -98.23% | 16,560 | 0% | +354.95% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 40,360 | +0.12% | -77.66% | 10 | 0% | -93.63% | 2,368 | 0% | -47.61% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 180,540 | 0% | -83.17% | 2 | 0% | -99.62% | 1,568 | 0% | -43.35% |
| [Load/for_range](#loadfor_range) | 13,631 | +0.05% | -75.95% | 7 | 0% | -87.50% | 1,088 | 0% | -45.60% |
| [Render/for_range](#renderfor_range) | 38,376 | +0.01% | -58.38% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/hf_deepseek_v31](#loadhf_deepseek_v31) | 387,631 | +0.22% |  | 55 | 0% |  | 21,976 | 0% | +0.73% |
| [Render/hf_deepseek_v31](#renderhf_deepseek_v31) | 90,609 | -0.01% |  | 43 | 0% |  | 2,480 | 0% | 0% |
| [Load/hf_gpt_oss](#loadhf_gpt_oss) | 1,549,586 | +0.17% |  | 287 | 0% |  | 74,672 | +0.04% | +0.60% |
| [Render/hf_gpt_oss](#renderhf_gpt_oss) | 170,451 | -0.11% |  | 155 | 0% |  | 5,064 | -0.63% | +2.59% |
| [Load/hf_mistral_small](#loadhf_mistral_small) | 598,850 | +0.11% |  | 88 | 0% |  | 33,048 | -0.05% | +0.34% |
| [Render/hf_mistral_small](#renderhf_mistral_small) | 144,909 | +0.25% |  | 105 | 0% |  | 6,120 | 0% | -0.26% |
| [Load/hf_qwen3](#loadhf_qwen3) | 485,661 | +0.05% |  | 82 | 0% |  | 25,560 | 0% | +0.31% |
| [Render/hf_qwen3](#renderhf_qwen3) | 119,795 | +0.08% |  | 100 | 0% |  | 3,248 | 0% | 0% |
| [Load/html_autoescape](#loadhtml_autoescape) | 194,038 | +0.13% |  | 43 | 0% |  | 12,640 | 0% | -48.78% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,212,617 | +0.23% |  | 674 | 0% |  | 45,408 | 0% | +12.44% |
| [Load/inheritance](#loadinheritance) | 23,040 | +0.18% | -79.81% | 7 | 0% | -93.46% | 1,344 | 0% | -53.20% |
| [Render/inheritance](#renderinheritance) | 137,588 | -0.37% | -81.60% | 35 | 0% | -94.37% | 7,384 | 0% | +22.74% |
| [Load/int_arith](#loadint_arith) | 65,981 | +0.03% |  | 20 | 0% |  | 3,872 | 0% | +0.41% |
| [Render/int_arith](#renderint_arith) | 827,390 | +0.22% |  | 12 | 0% |  | 3,272 | 0% | -0.49% |
| [Module/keep](#modulekeep) | 7,000 | 0% |  | 7 | 0% |  |  |  |  |
| [Module/keep_written](#modulekeep_written) | 36,388 | +0.02% |  | 11 | 0% |  |  |  |  |
| [Load/large_static](#loadlarge_static) | 154,672 | +0.07% | -82.54% | 14 | 0% | -96.82% | 43,120 | 0% | -19.23% |
| [Render/large_static](#renderlarge_static) | 20,709 | +0.01% | -72.44% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [RenderSink/large_static](#rendersinklarge_static) | 12,699 | +0.02% |  | 0 |  |  | 0 |  |  |
| [RenderTo/large_static](#rendertolarge_static) | 20,272 | +0.01% |  | 0 |  |  | 0 |  |  |
| [Load/macros](#loadmacros) | 56,328 | +0.15% | -75.29% | 27 | 0% | -85.48% | 4,392 | 0% | -29.34% |
| [Render/macros](#rendermacros) | 656,431 | +0.02% | -74.39% | 308 | 0% | -88.71% | 20,560 | 0% | +39.67% |
| [Load/many_tags](#loadmany_tags) | 9,018,112 | +0.11% | -79.58% | 951 | 0% | -97.22% | 438,160 | 0% | -58.48% |
| [Render/many_tags](#rendermany_tags) | 727,212 | +0.21% | -65.09% | 2 | 0% | -90.48% | 5,648 | 0% | -10.52% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 46,561 | +0.22% | -80.40% | 19 | 0% | -89.67% | 3,360 | 0% | -44.59% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,195,668 | 0% | -57.95% | 33 | 0% | -99.19% | 381,536 | 0% | +9.95% |
| [RenderSink/mitsuhiko_table](#rendersinkmitsuhiko_table) | 5,122,853 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table](#rendertomitsuhiko_table) | 5,195,168 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 54,806 | +0.17% |  | 35 | 0% |  | 5,688 | 0% | -31.17% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,541,824 | 0% |  | 33 | 0% |  | 1,414,928 | 0% | +2.46% |
| [RenderSink/mitsuhiko_table_wide](#rendersinkmitsuhiko_table_wide) | 5,365,116 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table_wide](#rendertomitsuhiko_table_wide) | 5,541,379 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/plain_text](#loadplain_text) | 3,710 | +0.08% | -61.66% | 4 | 0% | -77.78% | 672 | 0% | -40.00% |
| [Render/plain_text](#renderplain_text) | 1,019 | +0.20% | -85.00% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/plain_text](#rendersinkplain_text) | 721 | +0.28% |  | 0 |  |  | 0 |  |  |
| [RenderTo/plain_text](#rendertoplain_text) | 820 | +0.24% |  | 0 |  |  | 0 |  |  |
| [Load/strings](#loadstrings) | 70,369 | +0.20% | -75.89% | 30 | 0% | -86.61% | 4,344 | 0% | -51.95% |
| [Render/strings](#renderstrings) | 580,126 | +0.26% | -72.34% | 344 | 0% | -83.73% | 9,088 | 0% | -49.31% |
| [Load/substitute](#loadsubstitute) | 7,743 | +0.09% | -74.07% | 6 | 0% | -83.78% | 832 | 0% | -45.26% |
| [Render/substitute](#rendersubstitute) | 2,252 | +0.09% | -74.45% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/substitute](#rendersinksubstitute) | 1,958 | +0.10% |  | 0 |  |  | 0 |  |  |
| [RenderTo/substitute](#rendertosubstitute) | 2,057 | +0.10% |  | 0 |  |  | 0 |  |  |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 727 | 0% | 727 | -1.22% | 0 |  | 7,862 | +6.33% |
| Render/chat_llama | 1,188 | -1.25% | 512 | -2.10% | 0 |  | 4,264 | -0.21% |
| Load/chat_mistral | 1,056 | +2.72% | 1,038 | +0.68% | 0 |  | 8,963 | +11.48% |
| Render/chat_mistral | 1,921 | +0.73% | 575 | +0.70% | 0 |  | 9,910 | -5.84% |
| Load/chat_qwen | 510 | -3.23% | 588 | +0.51% | 0 |  | 4,906 | +14.33% |
| Render/chat_qwen | 1,132 | +2.72% | 416 | +1.46% | 0 |  | 3,473 | -3.85% |
| Load/config_file | 186 | -0.53% | 167 | -1.18% | 0 |  | 2,817 | +7.44% |
| Render/config_file | 6,475 | +5.61% | 1,583 | +4.63% | 0 |  | 42,645 | +9.78% |
| Load/dict_ops | 44 | +22.22% | 31 | -11.43% | 0 |  | 1,385 | +4.45% |
| Render/dict_ops | 630 | -1.10% | 425 | +0.71% | 0 |  | 893 | -0.56% |
| Load/expressions | 24 | +41.18% | 16 | -15.79% | 0 |  | 1,357 | +1.34% |
| Render/expressions | 4 | 0% | 3 | +50.00% | 0 |  | 306 | +2.00% |
| Load/filters | 96 | +6.67% | 57 | -5.00% | 0 |  | 1,778 | +4.34% |
| Render/filters | 12 | -7.69% | 2 | -33.33% | 0 |  | 808 | +2.54% |
| Load/for_filter_if | 23 | +15.00% | 18 | +5.88% | 0 |  | 1,156 | +11.26% |
| Render/for_filter_if | 1,734 | -3.56% | 96 | +1.05% | 0 |  | 182 | +15.92% |
| Load/for_loop_vars | 19 | -9.52% | 15 | 0% | 0 |  | 1,188 | +11.03% |
| Render/for_loop_vars | 8 | +33.33% | 3 | +50.00% | 0 |  | 174 | +33.85% |
| Load/for_range | 4 | 0% | 5 | +25.00% | 0 |  | 748 | +0.40% |
| Render/for_range | 0 | -100.00% | 0 |  | 0 |  | 168 | +1.82% |
| Load/hf_deepseek_v31 | 979 | +0.82% | 976 | +1.24% | 0 |  | 6,701 | +12.55% |
| Render/hf_deepseek_v31 | 351 | +0.57% | 81 | +3.85% | 0 |  | 1,451 | -0.82% |
| Load/hf_gpt_oss | 4,608 | +0.59% | 3,289 | -0.45% | 0 |  | 22,814 | +5.24% |
| Render/hf_gpt_oss | 1,183 | -1.99% | 367 | -2.13% | 0 |  | 4,533 | -1.00% |
| Load/hf_mistral_small | 1,771 | -0.11% | 1,587 | +1.54% | 0 |  | 11,878 | +11.78% |
| Render/hf_mistral_small | 978 | +3.71% | 321 | -1.23% | 0 |  | 3,836 | -0.26% |
| Load/hf_qwen3 | 1,467 | +4.94% | 1,230 | -0.65% | 0 |  | 9,497 | +12.79% |
| Render/hf_qwen3 | 663 | +4.74% | 212 | +3.92% | 0 |  | 2,598 | -1.14% |
| Load/html_autoescape | 449 | +1.35% | 521 | +0.97% | 0 |  | 4,265 | +14.40% |
| Render/html_autoescape | 3,591 | -6.99% | 1,218 | -3.56% | 0 |  | 36,452 | +4.18% |
| Load/inheritance | 2 | 0% | 3 | 0% | 0 |  | 685 | +1.63% |
| Render/inheritance | 591 | +2.07% | 101 | +3.06% | 0 |  | 940 | -26.62% |
| Load/int_arith | 86 | +13.16% | 59 | +9.26% | 0 |  | 2,180 | +8.95% |
| Render/int_arith | 26 | -21.21% | 20 | +33.33% | 0 |  | 772 | +3.49% |
| Module/keep | 3 | +50.00% | 2 | +100.00% | 0 |  | 410 | +10.51% |
| Module/keep_written | 2,931 | +0.07% | 562 | 0% | 0 |  | 729 | +2.10% |
| Load/large_static | 1,937 | +0.10% | 911 | -0.22% | 0 |  | 273 | -2.85% |
| Render/large_static | 712 | +0.28% | 568 | +0.18% | 0 |  | 4 | 0% |
| RenderSink/large_static | 0 |  | 0 |  | 0 |  | 1 | -66.67% |
| RenderTo/large_static | 689 | +0.29% | 572 | +0.18% | 0 |  | 4 | +33.33% |
| Load/macros | 91 | +15.19% | 71 | -2.74% | 0 |  | 1,715 | +0.35% |
| Render/macros | 761 | +0.40% | 233 | +0.43% | 0 |  | 2,031 | -41.44% |
| Load/many_tags | 28,244 | +0.99% | 22,525 | +0.04% | 0 |  | 235,333 | +11.09% |
| Render/many_tags | 6,233 | -0.24% | 845 | -1.52% | 0 |  | 153 | -9.47% |
| Load/mitsuhiko_table | 22 | -15.38% | 20 | -4.76% | 0 |  | 858 | +0.59% |
| Render/mitsuhiko_table | 10,052 | +0.01% | 6,278 | -0.22% | 0 |  | 396 | +10.00% |
| RenderSink/mitsuhiko_table | 9,937 | -0.13% | 1,184 | -1.17% | 0 |  | 348 | +15.23% |
| RenderTo/mitsuhiko_table | 10,023 | -0.26% | 6,292 | -0.13% | 0 |  | 379 | +15.90% |
| Load/mitsuhiko_table_wide | 72 | +9.09% | 83 | +7.79% | 0 |  | 1,084 | +16.31% |
| Render/mitsuhiko_table_wide | 10,261 | -1.69% | 22,678 | +0.18% | 0 |  | 391 | +8.31% |
| RenderSink/mitsuhiko_table_wide | 10,072 | +0.14% | 1,310 | +0.61% | 0 |  | 324 | +15.71% |
| RenderTo/mitsuhiko_table_wide | 10,255 | -1.66% | 22,679 | -0.03% | 0 |  | 354 | +7.60% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 6 | -79.31% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 5 | 0% |
| RenderSink/plain_text | 0 |  | 0 |  | 0 |  | 4 | 0% |
| RenderTo/plain_text | 0 |  | 0 |  | 0 |  | 4 | 0% |
| Load/strings | 131 | +12.93% | 97 | +5.43% | 0 |  | 2,772 | +1.17% |
| Render/strings | 413 | +3.51% | 468 | -0.85% | 0 |  | 1,615 | +2.87% |
| Load/substitute | 2 | 0% | 3 | -25.00% | 0 |  | 160 | +2.56% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 4 | +33.33% |
| RenderSink/substitute | 0 |  | 0 |  | 0 |  | 1 | -66.67% |
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
