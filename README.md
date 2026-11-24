# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (267 so far, latest [6217b8c](https://github.com/jinja2cpp/Jinja2Cpp/commit/6217b8cd276b70d501ef93b55f05c235670750af) on 2026-11-24). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/big_int_radix](#loadbig_int_radix) | 43,266 | -0.04% |  | 14 | 0% |  | 2,688 | 0% | 0% |
| [Render/big_int_radix](#renderbig_int_radix) | 308,312,379 | 0% |  | 163 | 0% |  | 1,540,760 | 0% | -39.13% |
| [Load/chat_llama](#loadchat_llama) | 337,434 | 0% |  | 54 | 0% |  | 18,536 | 0% | -52.57% |
| [Render/chat_llama](#renderchat_llama) | 291,240 | -0.44% |  | 224 | 0% |  | 11,608 | 0% | -19.16% |
| [Load/chat_mistral](#loadchat_mistral) | 385,885 | 0% |  | 51 | 0% |  | 20,976 | 0% | -53.02% |
| [Render/chat_mistral](#renderchat_mistral) | 491,436 | -0.30% |  | 392 | 0% |  | 9,752 | 0% | -26.17% |
| [Load/chat_qwen](#loadchat_qwen) | 262,663 | 0% |  | 43 | 0% |  | 14,472 | 0% | -47.04% |
| [Render/chat_qwen](#renderchat_qwen) | 272,957 | -0.49% |  | 211 | 0% |  | 8,280 | 0% | -2.63% |
| [Load/config_file](#loadconfig_file) | 94,403 | 0% |  | 24 | 0% |  | 5,520 | 0% | -47.73% |
| [Render/config_file](#renderconfig_file) | 1,245,277 | -2.59% |  | 882 | -2.86% |  | 25,960 | 0% | +9.04% |
| [Load/dict_ops](#loaddict_ops) | 31,968 | 0% | -78.37% | 8 | 0% | -93.89% | 2,216 | 0% | -47.93% |
| [Render/dict_ops](#renderdict_ops) | 288,997 | -0.58% | -58.17% | 112 | 0% | -75.11% | 24,408 | 0% | -29.65% |
| [Load/expressions](#loadexpressions) | 44,175 | 0% | -78.57% | 15 | 0% | -89.73% | 2,752 | 0% | -55.15% |
| [Render/expressions](#renderexpressions) | 414,401 | 0% | -54.27% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 67,517 | 0% | -75.40% | 26 | 0% | -88.60% | 3,856 | 0% | -66.22% |
| [Render/filters](#renderfilters) | 24,781 | -6.56% | -72.05% | 22 | -4.35% | -70.27% | 752 | 0% | -84.69% |
| [Load/for_filter_if](#loadfor_filter_if) | 28,571 | 0% | -78.90% | 6 | 0% | -95.12% | 1,928 | 0% | -44.08% |
| [Render/for_filter_if](#renderfor_filter_if) | 266,154 | 0% | -69.25% | 4 | 0% | -98.23% | 16,560 | 0% | +354.95% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 38,233 | 0% | -78.84% | 9 | 0% | -94.27% | 2,392 | 0% | -47.08% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 170,801 | 0% | -84.08% | 2 | 0% | -99.62% | 1,568 | 0% | -43.35% |
| [Load/for_range](#loadfor_range) | 12,526 | 0% | -77.90% | 6 | 0% | -89.29% | 1,128 | 0% | -43.60% |
| [Render/for_range](#renderfor_range) | 37,183 | 0% | -59.68% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/hf_deepseek_v31](#loadhf_deepseek_v31) | 384,912 | 0% |  | 54 | 0% |  | 22,032 | 0% | +0.99% |
| [Render/hf_deepseek_v31](#renderhf_deepseek_v31) | 90,625 | +0.03% |  | 42 | 0% |  | 2,456 | 0% | -0.97% |
| [Load/hf_gpt_oss](#loadhf_gpt_oss) | 1,528,022 | 0% |  | 282 | 0% |  | 75,000 | 0% | +1.05% |
| [Render/hf_gpt_oss](#renderhf_gpt_oss) | 169,266 | +0.02% |  | 153 | 0% |  | 5,984 | 0% | +21.23% |
| [Load/hf_mistral_small](#loadhf_mistral_small) | 592,679 | 0% |  | 87 | 0% |  | 33,136 | 0% | +0.61% |
| [Render/hf_mistral_small](#renderhf_mistral_small) | 140,222 | -0.10% |  | 97 | 0% |  | 6,096 | 0% | -0.65% |
| [Load/hf_qwen3](#loadhf_qwen3) | 478,885 | 0% |  | 81 | 0% |  | 25,648 | 0% | +0.66% |
| [Render/hf_qwen3](#renderhf_qwen3) | 117,185 | -0.29% |  | 99 | 0% |  | 3,224 | 0% | -0.74% |
| [Load/html_autoescape](#loadhtml_autoescape) | 189,392 | 0% |  | 42 | 0% |  | 12,728 | 0% | -48.43% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,197,368 | 0% |  | 674 | 0% |  | 46,256 | 0% | +14.54% |
| [Load/inheritance](#loadinheritance) | 21,312 | 0% | -81.32% | 6 | 0% | -94.39% | 1,368 | 0% | -52.37% |
| [Render/inheritance](#renderinheritance) | 136,800 | 0% | -81.71% | 35 | 0% | -94.37% | 8,168 | 0% | +35.77% |
| [Load/int_arith](#loadint_arith) | 65,188 | 0% |  | 19 | 0% |  | 3,896 | 0% | +1.04% |
| [Render/int_arith](#renderint_arith) | 824,190 | -0.02% |  | 12 | 0% |  | 3,272 | 0% | -0.49% |
| [Module/keep](#modulekeep) | 7,163 | +1.14% |  | 6 | 0% |  |  |  |  |
| [Module/keep_written](#modulekeep_written) | 29,158 | +0.26% |  | 10 | 0% |  |  |  |  |
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
| [Nested/new_template](#nestednew_template) | 452,189 | -0.14% |  | 323 | 0% |  |  |  |  |
| [Load/plain_text](#loadplain_text) | 2,932 | 0% | -69.70% | 3 | 0% | -83.33% | 680 | 0% | -39.29% |
| [Render/plain_text](#renderplain_text) | 972 | 0% | -85.69% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/plain_text](#rendersinkplain_text) | 677 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/plain_text](#rendertoplain_text) | 773 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/prompt_budget](#loadprompt_budget) | 34,335 | 0% |  | 17 | 0% |  | 2,248 | 0% | +1.08% |
| [Render/prompt_budget](#renderprompt_budget) | 1,324,946 | +0.24% |  | 76 | 0% |  | 36,640 | +0.62% | +0.62% |
| [Nested/shared_globals](#nestedshared_globals) | 160,033 | -0.45% |  | 160 | 0% |  |  |  |  |
| [Load/strings](#loadstrings) | 69,091 | 0% | -76.33% | 29 | 0% | -87.05% | 4,384 | 0% | -51.50% |
| [Render/strings](#renderstrings) | 580,755 | -0.05% | -72.31% | 344 | 0% | -83.73% | 9,088 | 0% | -49.31% |
| [Load/substitute](#loadsubstitute) | 6,662 | 0% | -77.69% | 5 | 0% | -86.49% | 840 | 0% | -44.74% |
| [Render/substitute](#rendersubstitute) | 1,653 | 0% | -81.24% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/substitute](#rendersinksubstitute) | 1,362 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/substitute](#rendertosubstitute) | 1,458 | 0% |  | 0 |  |  | 0 |  |  |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/big_int_radix | 111 | +5.71% | 58 | 0% | 0 |  | 1,942 | -1.37% |
| Render/big_int_radix | 774,905 | 0% | 515,295 | 0% | 0 |  | 4,273 | +33.70% |
| Load/chat_llama | 793 | +0.63% | 776 | 0% | 0 |  | 7,701 | -3.64% |
| Render/chat_llama | 1,235 | -1.28% | 569 | -1.04% | 0 |  | 4,382 | -1.15% |
| Load/chat_mistral | 1,085 | +0.65% | 1,093 | -0.09% | 0 |  | 8,652 | -2.52% |
| Render/chat_mistral | 1,982 | -1.69% | 666 | +1.52% | 0 |  | 10,259 | -5.43% |
| Load/chat_qwen | 537 | +0.37% | 647 | -0.15% | 0 |  | 4,704 | -6.74% |
| Render/chat_qwen | 1,089 | -0.82% | 429 | +0.70% | 0 |  | 3,627 | +2.20% |
| Load/config_file | 207 | +5.61% | 185 | -2.63% | 0 |  | 2,772 | -1.28% |
| Render/config_file | 7,271 | -2.27% | 1,895 | -1.66% | 0 |  | 45,292 | +0.80% |
| Load/dict_ops | 37 | +12.12% | 21 | +16.67% | 0 |  | 1,373 | 0% |
| Render/dict_ops | 693 | -0.14% | 421 | +0.48% | 0 |  | 765 | -6.93% |
| Load/expressions | 33 | -2.94% | 27 | 0% | 0 |  | 1,331 | +2.38% |
| Render/expressions | 3 | 0% | 1 |  | 0 |  | 302 | +8.63% |
| Load/filters | 97 | -1.02% | 52 | -1.89% | 0 |  | 1,675 | -11.42% |
| Render/filters | 23 | +4.55% | 7 | 0% | 0 |  | 708 | -7.57% |
| Load/for_filter_if | 19 | -38.71% | 5 | -37.50% | 0 |  | 1,050 | -3.14% |
| Render/for_filter_if | 1,721 | -0.12% | 90 | +3.45% | 0 |  | 142 | -17.92% |
| Load/for_loop_vars | 52 | +6.12% | 17 | 0% | 0 |  | 1,092 | -2.41% |
| Render/for_loop_vars | 4 | +33.33% | 0 |  | 0 |  | 103 | -41.14% |
| Load/for_range | 5 | -54.55% | 4 | -20.00% | 0 |  | 762 | +2.28% |
| Render/for_range | 2 | 0% | 1 | 0% | 0 |  | 137 | -26.34% |
| Load/hf_deepseek_v31 | 957 | +0.63% | 972 | +1.04% | 0 |  | 6,393 | -1.24% |
| Render/hf_deepseek_v31 | 389 | +0.78% | 91 | 0% | 0 |  | 1,471 | -4.17% |
| Load/hf_gpt_oss | 4,704 | +0.19% | 3,402 | +0.09% | 0 |  | 22,216 | -8.34% |
| Render/hf_gpt_oss | 1,310 | -0.46% | 416 | +1.22% | 0 |  | 4,933 | -2.03% |
| Load/hf_mistral_small | 1,874 | +1.08% | 1,647 | +0.12% | 0 |  | 11,311 | -2.72% |
| Render/hf_mistral_small | 1,020 | -1.16% | 325 | +2.52% | 0 |  | 3,713 | -2.70% |
| Load/hf_qwen3 | 1,502 | +1.56% | 1,300 | +0.31% | 0 |  | 8,931 | -7.64% |
| Render/hf_qwen3 | 709 | -2.88% | 236 | +1.29% | 0 |  | 2,709 | -2.90% |
| Load/html_autoescape | 487 | +1.04% | 536 | -0.74% | 0 |  | 3,861 | -9.62% |
| Render/html_autoescape | 4,858 | +13.27% | 1,634 | -1.63% | 0 |  | 37,001 | -2.40% |
| Load/inheritance | 5 | -58.33% | 3 | -50.00% | 0 |  | 710 | +7.25% |
| Render/inheritance | 597 | 0% | 119 | 0% | 0 |  | 1,059 | +2.42% |
| Load/int_arith | 80 | -4.76% | 77 | -4.94% | 0 |  | 2,068 | -3.95% |
| Render/int_arith | 36 | +9.09% | 20 | +5.26% | 0 |  | 781 | -4.41% |
| Module/keep | 2 | 0% | 2 | 0% | 0 |  | 482 | +5.93% |
| Module/keep_written | 1,145 | +0.35% | 579 | +1.40% | 0 |  | 782 | +1.43% |
| Load/large_static | 1,916 | -0.16% | 911 | +0.11% | 0 |  | 295 | +16.14% |
| Render/large_static | 700 | 0% | 570 | 0% | 0 |  | 3 | -57.14% |
| RenderSink/large_static | 2 | 0% | 0 |  | 0 |  | 3 | 0% |
| RenderTo/large_static | 679 | 0% | 581 | 0% | 0 |  | 3 | -40.00% |
| Load/macros | 85 | +4.94% | 70 | -2.78% | 0 |  | 1,729 | -1.87% |
| Render/macros | 759 | -0.26% | 269 | 0% | 0 |  | 1,537 | +54.01% |
| Load/many_tags | 27,904 | -0.40% | 22,598 | +0.13% | 0 |  | 241,923 | -3.85% |
| Render/many_tags | 6,094 | -0.11% | 778 | 0% | 0 |  | 106 | -97.25% |
| Load/mitsuhiko_table | 45 | -4.26% | 52 | +8.33% | 0 |  | 857 | -4.67% |
| Render/mitsuhiko_table | 10,038 | -0.11% | 6,335 | +0.02% | 0 |  | 269 | -12.66% |
| RenderSink/mitsuhiko_table | 9,957 | -0.10% | 1,204 | 0% | 0 |  | 242 | -6.92% |
| RenderTo/mitsuhiko_table | 10,043 | -0.14% | 6,287 | +0.03% | 0 |  | 241 | -14.23% |
| Load/mitsuhiko_table_wide | 91 | +7.06% | 93 | +8.14% | 0 |  | 1,005 | -1.08% |
| Render/mitsuhiko_table_wide | 10,328 | +0.08% | 22,599 | -0.01% | 0 |  | 289 | -17.90% |
| RenderSink/mitsuhiko_table_wide | 10,033 | +0.06% | 1,269 | 0% | 0 |  | 243 | -11.31% |
| RenderTo/mitsuhiko_table_wide | 10,190 | +0.08% | 22,673 | +0.01% | 0 |  | 261 | -22.09% |
| Nested/new_template | 2,186 | -2.80% | 1,636 | -0.79% | 0 |  | 24,267 | -0.37% |
| Load/plain_text | 0 |  | 1 | 0% | 0 |  | 3 | -70.00% |
| Render/plain_text | 1 | 0% | 0 |  | 0 |  | 5 | -44.44% |
| RenderSink/plain_text | 1 | 0% | 0 |  | 0 |  | 4 | -33.33% |
| RenderTo/plain_text | 2 | 0% | 0 |  | 0 |  | 5 | -28.57% |
| Load/prompt_budget | 74 | +1.37% | 51 | +4.08% | 0 |  | 1,415 | -0.21% |
| Render/prompt_budget | 4,787 | -0.21% | 546 | -0.55% | 0 |  | 821 | -30.36% |
| Nested/shared_globals | 226 | +61.43% | 108 | -1.82% | 0 |  | 9,180 | +0.17% |
| Load/strings | 147 | +4.26% | 78 | +2.63% | 0 |  | 2,837 | +0.14% |
| Render/strings | 431 | +1.41% | 478 | -0.62% | 0 |  | 2,234 | +43.30% |
| Load/substitute | 1 | -85.71% | 2 | 0% | 0 |  | 112 | -13.18% |
| Render/substitute | 1 | 0% | 0 |  | 0 |  | 3 | -50.00% |
| RenderSink/substitute | 1 | 0% | 0 |  | 0 |  | 3 | 0% |
| RenderTo/substitute | 1 | 0% | 0 |  | 0 |  | 3 | -40.00% |

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
