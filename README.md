# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (261 so far, latest [fb833ad](https://github.com/jinja2cpp/Jinja2Cpp/commit/fb833ad98b856f4e9da19a71cde2f0c78b65b4b1) on 2026-11-22). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/big_int_radix](#loadbig_int_radix) | 42,895 | 0% |  | 14 | 0% |  | 2,688 | 0% | 0% |
| [Render/big_int_radix](#renderbig_int_radix) | 308,312,466 | -79.73% |  | 163 | -99.95% |  | 1,540,760 | -39.13% | -39.13% |
| [Load/chat_llama](#loadchat_llama) | 336,602 | 0% |  | 54 | 0% |  | 18,536 | 0% | -52.57% |
| [Render/chat_llama](#renderchat_llama) | 291,561 | -0.06% |  | 224 | 0% |  | 11,608 | 0% | -19.16% |
| [Load/chat_mistral](#loadchat_mistral) | 385,002 | 0% |  | 51 | 0% |  | 20,976 | 0% | -53.02% |
| [Render/chat_mistral](#renderchat_mistral) | 492,066 | -0.03% |  | 392 | 0% |  | 9,752 | 0% | -26.17% |
| [Load/chat_qwen](#loadchat_qwen) | 261,943 | 0% |  | 43 | 0% |  | 14,472 | 0% | -47.04% |
| [Render/chat_qwen](#renderchat_qwen) | 273,367 | -0.12% |  | 211 | 0% |  | 8,280 | 0% | -2.63% |
| [Load/config_file](#loadconfig_file) | 94,181 | 0% |  | 24 | 0% |  | 5,520 | 0% | -47.73% |
| [Render/config_file](#renderconfig_file) | 1,278,322 | -0.11% |  | 908 | 0% |  | 25,960 | 0% | +9.04% |
| [Load/dict_ops](#loaddict_ops) | 31,883 | 0% | -78.42% | 8 | 0% | -93.89% | 2,216 | 0% | -47.93% |
| [Render/dict_ops](#renderdict_ops) | 290,693 | -0.03% | -57.92% | 112 | 0% | -75.11% | 24,408 | 0% | -29.65% |
| [Load/expressions](#loadexpressions) | 43,776 | 0% | -78.76% | 15 | 0% | -89.73% | 2,752 | 0% | -55.15% |
| [Render/expressions](#renderexpressions) | 414,401 | 0% | -54.27% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 66,923 | 0% | -75.61% | 26 | 0% | -88.60% | 3,856 | 0% | -66.22% |
| [Render/filters](#renderfilters) | 26,520 | 0% | -70.09% | 23 | 0% | -68.92% | 752 | 0% | -84.69% |
| [Load/for_filter_if](#loadfor_filter_if) | 28,522 | 0% | -78.93% | 6 | 0% | -95.12% | 1,928 | 0% | -44.08% |
| [Render/for_filter_if](#renderfor_filter_if) | 266,154 | -0.36% | -69.25% | 4 | 0% | -98.23% | 16,560 | 0% | +354.95% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 38,180 | 0% | -78.87% | 9 | 0% | -94.27% | 2,392 | 0% | -47.08% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 170,801 | 0% | -84.08% | 2 | 0% | -99.62% | 1,568 | 0% | -43.35% |
| [Load/for_range](#loadfor_range) | 12,493 | 0% | -77.96% | 6 | 0% | -89.29% | 1,128 | 0% | -43.60% |
| [Render/for_range](#renderfor_range) | 37,183 | 0% | -59.68% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/hf_deepseek_v31](#loadhf_deepseek_v31) | 384,091 | 0% |  | 54 | 0% |  | 22,032 | 0% | +0.99% |
| [Render/hf_deepseek_v31](#renderhf_deepseek_v31) | 90,594 | -0.07% |  | 42 | 0% |  | 2,456 | 0% | -0.97% |
| [Load/hf_gpt_oss](#loadhf_gpt_oss) | 1,525,610 | 0% |  | 282 | 0% |  | 75,000 | 0% | +1.05% |
| [Render/hf_gpt_oss](#renderhf_gpt_oss) | 169,375 | -0.09% |  | 153 | 0% |  | 5,984 | 0% | +21.23% |
| [Load/hf_mistral_small](#loadhf_mistral_small) | 591,363 | 0% |  | 87 | 0% |  | 33,136 | 0% | +0.61% |
| [Render/hf_mistral_small](#renderhf_mistral_small) | 140,367 | -0.02% |  | 97 | 0% |  | 6,096 | 0% | -0.65% |
| [Load/hf_qwen3](#loadhf_qwen3) | 477,659 | 0% |  | 81 | 0% |  | 25,648 | 0% | +0.66% |
| [Render/hf_qwen3](#renderhf_qwen3) | 117,518 | -0.10% |  | 99 | 0% |  | 3,224 | 0% | -0.74% |
| [Load/html_autoescape](#loadhtml_autoescape) | 189,012 | 0% |  | 42 | 0% |  | 12,728 | 0% | -48.43% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,197,364 | -0.13% |  | 674 | 0% |  | 46,256 | 0% | +14.54% |
| [Load/inheritance](#loadinheritance) | 21,300 | 0% | -81.34% | 6 | 0% | -94.39% | 1,368 | 0% | -52.37% |
| [Render/inheritance](#renderinheritance) | 136,800 | -0.22% | -81.71% | 35 | 0% | -94.37% | 8,168 | 0% | +35.77% |
| [Load/int_arith](#loadint_arith) | 64,666 | 0% |  | 19 | 0% |  | 3,896 | 0% | +1.04% |
| [Render/int_arith](#renderint_arith) | 824,338 | -0.19% |  | 12 | 0% |  | 3,272 | 0% | -0.49% |
| [Module/keep](#modulekeep) | 6,989 | 0% |  | 6 | 0% |  |  |  |  |
| [Module/keep_written](#modulekeep_written) | 28,988 | 0% |  | 10 | 0% |  |  |  |  |
| [Load/large_static](#loadlarge_static) | 146,958 | 0% | -83.41% | 13 | 0% | -97.05% | 43,128 | 0% | -19.21% |
| [Render/large_static](#renderlarge_static) | 20,342 | 0% | -72.93% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [RenderSink/large_static](#rendersinklarge_static) | 12,631 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/large_static](#rendertolarge_static) | 19,845 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/macros](#loadmacros) | 54,293 | 0% | -76.18% | 25 | 0% | -86.56% | 4,472 | 0% | -28.06% |
| [Render/macros](#rendermacros) | 647,390 | -0.09% | -74.74% | 308 | 0% | -88.71% | 20,608 | 0% | +40.00% |
| [Load/many_tags](#loadmany_tags) | 8,753,681 | 0% | -80.18% | 950 | 0% | -97.23% | 438,136 | 0% | -58.48% |
| [Render/many_tags](#rendermany_tags) | 730,657 | -0.25% | -64.92% | 2 | 0% | -90.48% | 5,648 | 0% | -10.52% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 44,630 | 0% | -81.21% | 18 | 0% | -90.22% | 3,416 | 0% | -43.67% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,105,184 | 0% | -58.68% | 33 | 0% | -99.19% | 381,536 | 0% | +9.95% |
| [RenderSink/mitsuhiko_table](#rendersinkmitsuhiko_table) | 5,035,279 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table](#rendertomitsuhiko_table) | 5,104,640 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 52,115 | 0% |  | 34 | 0% |  | 5,744 | 0% | -30.49% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,451,418 | 0% |  | 33 | 0% |  | 1,414,928 | 0% | +2.46% |
| [RenderSink/mitsuhiko_table_wide](#rendersinkmitsuhiko_table_wide) | 5,277,323 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table_wide](#rendertomitsuhiko_table_wide) | 5,450,812 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Nested/new_template](#nestednew_template) | 452,566 | 0% |  | 323 | 0% |  |  |  |  |
| [Load/plain_text](#loadplain_text) | 2,932 | 0% | -69.70% | 3 | 0% | -83.33% | 680 | 0% | -39.29% |
| [Render/plain_text](#renderplain_text) | 972 | 0% | -85.69% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/plain_text](#rendersinkplain_text) | 677 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/plain_text](#rendertoplain_text) | 773 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/prompt_budget](#loadprompt_budget) | 34,256 | 0% |  | 17 | 0% |  | 2,248 | 0% | +1.08% |
| [Render/prompt_budget](#renderprompt_budget) | 1,321,775 | -0.01% |  | 76 | 0% |  | 36,416 | 0% | 0% |
| [Nested/shared_globals](#nestedshared_globals) | 160,766 | 0% |  | 160 | 0% |  |  |  |  |
| [Load/strings](#loadstrings) | 68,836 | 0% | -76.41% | 29 | 0% | -87.05% | 4,384 | 0% | -51.50% |
| [Render/strings](#renderstrings) | 581,065 | -0.08% | -72.30% | 344 | 0% | -83.73% | 9,088 | 0% | -49.31% |
| [Load/substitute](#loadsubstitute) | 6,654 | 0% | -77.71% | 5 | 0% | -86.49% | 840 | 0% | -44.74% |
| [Render/substitute](#rendersubstitute) | 1,653 | 0% | -81.24% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/substitute](#rendersinksubstitute) | 1,362 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/substitute](#rendertosubstitute) | 1,458 | 0% |  | 0 |  |  | 0 |  |  |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/big_int_radix | 91 | -3.19% | 53 | -3.64% | 0 |  | 1,991 | +2.84% |
| Render/big_int_radix | 774,954 | -37.37% | 515,277 | -40.67% | 0 |  | 4,242 | +3.34% |
| Load/chat_llama | 770 | 0% | 769 | +0.79% | 0 |  | 7,968 | +3.20% |
| Render/chat_llama | 1,207 | +1.26% | 559 | -0.36% | 0 |  | 4,328 | +0.25% |
| Load/chat_mistral | 1,119 | +0.18% | 1,109 | -0.27% | 0 |  | 8,875 | +5.05% |
| Render/chat_mistral | 2,104 | -0.14% | 675 | -0.44% | 0 |  | 10,606 | +1.91% |
| Load/chat_qwen | 550 | +2.61% | 657 | -0.15% | 0 |  | 4,799 | +4.55% |
| Render/chat_qwen | 1,125 | +1.35% | 439 | 0% | 0 |  | 3,689 | +3.07% |
| Load/config_file | 197 | -5.74% | 185 | -1.60% | 0 |  | 2,861 | +6.48% |
| Render/config_file | 6,835 | +3.09% | 1,882 | +3.52% | 0 |  | 45,811 | +5.93% |
| Load/dict_ops | 26 | -3.70% | 16 | -5.88% | 0 |  | 1,388 | +3.89% |
| Render/dict_ops | 693 | +0.58% | 423 | +0.24% | 0 |  | 833 | +1.22% |
| Load/expressions | 18 | -18.18% | 15 | -6.25% | 0 |  | 1,335 | +1.52% |
| Render/expressions | 7 | +16.67% | 2 | 0% | 0 |  | 279 | -21.63% |
| Load/filters | 90 | -1.10% | 53 | +3.92% | 0 |  | 1,739 | +6.17% |
| Render/filters | 15 | -25.00% | 4 | +33.33% | 0 |  | 751 | +0.40% |
| Load/for_filter_if | 6 | -40.00% | 4 | 0% | 0 |  | 1,096 | +4.68% |
| Render/for_filter_if | 1,716 | -0.12% | 85 | 0% | 0 |  | 157 | +25.60% |
| Load/for_loop_vars | 26 | -13.33% | 13 | -13.33% | 0 |  | 1,152 | +6.08% |
| Render/for_loop_vars | 3 | -72.73% | 0 | -100.00% | 0 |  | 104 | -8.77% |
| Load/for_range | 2 | +100.00% | 3 | 0% | 0 |  | 795 | +8.16% |
| Render/for_range | 2 | 0% | 1 | 0% | 0 |  | 134 | 0% |
| Load/hf_deepseek_v31 | 971 | -1.92% | 977 | -0.51% | 0 |  | 6,317 | -2.17% |
| Render/hf_deepseek_v31 | 378 | -1.05% | 89 | +1.14% | 0 |  | 1,463 | +1.88% |
| Load/hf_gpt_oss | 4,734 | -0.19% | 3,417 | -0.29% | 0 |  | 23,292 | +7.05% |
| Render/hf_gpt_oss | 1,309 | -0.08% | 409 | -0.97% | 0 |  | 5,101 | +3.03% |
| Load/hf_mistral_small | 1,896 | -0.52% | 1,650 | -0.36% | 0 |  | 11,576 | +6.54% |
| Render/hf_mistral_small | 1,017 | +0.20% | 324 | +0.31% | 0 |  | 3,833 | +1.91% |
| Load/hf_qwen3 | 1,523 | +0.40% | 1,316 | -0.53% | 0 |  | 9,481 | +6.28% |
| Render/hf_qwen3 | 699 | -1.41% | 230 | +0.88% | 0 |  | 2,845 | +4.14% |
| Load/html_autoescape | 479 | -2.44% | 547 | -1.44% | 0 |  | 4,183 | +8.93% |
| Render/html_autoescape | 4,781 | -0.33% | 1,639 | -1.50% | 0 |  | 36,033 | -1.39% |
| Load/inheritance | 2 | 0% | 2 | 0% | 0 |  | 735 | +6.37% |
| Render/inheritance | 603 | +0.17% | 115 | -2.54% | 0 |  | 1,076 | +2.18% |
| Load/int_arith | 64 | 0% | 62 | +5.08% | 0 |  | 2,125 | +7.59% |
| Render/int_arith | 38 | +5.56% | 24 | +4.35% | 0 |  | 767 | -0.13% |
| Module/keep | 2 | 0% | 2 | 0% | 0 |  | 465 | +6.65% |
| Module/keep_written | 1,145 | 0% | 557 | 0% | 0 |  | 759 | +4.40% |
| Load/large_static | 1,918 | +0.10% | 910 | 0% | 0 |  | 267 | -4.98% |
| Render/large_static | 700 | 0% | 570 | 0% | 0 |  | 2 | -50.00% |
| RenderSink/large_static | 2 | 0% | 0 |  | 0 |  | 2 | -50.00% |
| RenderTo/large_static | 679 | 0% | 581 | 0% | 0 |  | 2 | -50.00% |
| Load/macros | 74 | -6.33% | 62 | -4.62% | 0 |  | 1,774 | +4.29% |
| Render/macros | 760 | +0.13% | 267 | 0% | 0 |  | 605 | -45.30% |
| Load/many_tags | 28,187 | -0.21% | 22,612 | -0.29% | 0 |  | 241,734 | +4.89% |
| Render/many_tags | 6,129 | +0.02% | 784 | -0.38% | 0 |  | 123 | -94.56% |
| Load/mitsuhiko_table | 31 | -3.12% | 40 | -6.98% | 0 |  | 930 | +10.85% |
| Render/mitsuhiko_table | 10,054 | -0.11% | 6,342 | -0.02% | 0 |  | 311 | -28.83% |
| RenderSink/mitsuhiko_table | 9,983 | +0.09% | 1,198 | -0.08% | 0 |  | 257 | -6.55% |
| RenderTo/mitsuhiko_table | 10,068 | +0.11% | 6,283 | 0% | 0 |  | 279 | -32.12% |
| Load/mitsuhiko_table_wide | 69 | -8.00% | 87 | 0% | 0 |  | 979 | +4.15% |
| Render/mitsuhiko_table_wide | 10,217 | +0.17% | 22,598 | 0% | 0 |  | 315 | -2.78% |
| RenderSink/mitsuhiko_table_wide | 10,036 | +0.03% | 1,264 | +0.08% | 0 |  | 253 | +0.80% |
| RenderTo/mitsuhiko_table_wide | 10,187 | +0.07% | 22,672 | 0% | 0 |  | 290 | -3.01% |
| Nested/new_template | 2,155 | -1.78% | 1,624 | +0.93% | 0 |  | 24,737 | +2.08% |
| Load/plain_text | 0 |  | 1 | 0% | 0 |  | 12 | -42.86% |
| Render/plain_text | 1 | 0% | 0 |  | 0 |  | 3 | -40.00% |
| RenderSink/plain_text | 1 | 0% | 0 |  | 0 |  | 3 | -40.00% |
| RenderTo/plain_text | 1 | 0% | 0 |  | 0 |  | 2 | -60.00% |
| Load/prompt_budget | 67 | -8.22% | 39 | -4.88% | 0 |  | 1,457 | +3.85% |
| Render/prompt_budget | 4,799 | -0.02% | 548 | -0.18% | 0 |  | 890 | -4.91% |
| Nested/shared_globals | 218 | -12.80% | 143 | -0.69% | 0 |  | 9,289 | -0.17% |
| Load/strings | 151 | +0.67% | 86 | +1.18% | 0 |  | 2,868 | +3.13% |
| Render/strings | 435 | +0.23% | 483 | 0% | 0 |  | 2,335 | +48.82% |
| Load/substitute | 1 | -50.00% | 2 | 0% | 0 |  | 161 | -10.56% |
| Render/substitute | 1 | 0% | 0 |  | 0 |  | 2 | -50.00% |
| RenderSink/substitute | 1 | 0% | 0 |  | 0 |  | 2 | -50.00% |
| RenderTo/substitute | 2 | 0% | 0 |  | 0 |  | 2 | -50.00% |

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
