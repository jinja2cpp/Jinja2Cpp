# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (275 so far, latest [da5e229](https://github.com/jinja2cpp/Jinja2Cpp/commit/da5e229340c43dd313e5298ad53c744560f8458a) on 2026-11-28). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/big_int_radix](#loadbig_int_radix) | 43,565 | +0.02% |  | 14 | 0% |  | 2,688 | 0% | 0% |
| [Render/big_int_radix](#renderbig_int_radix) | 146,579,786 | 0% |  | 123 | 0% |  | 1,540,760 | 0% | -39.13% |
| [Load/chat_llama](#loadchat_llama) | 338,624 | +0.01% |  | 54 | 0% |  | 18,536 | 0% | -52.57% |
| [Render/chat_llama](#renderchat_llama) | 291,210 | 0% |  | 224 | 0% |  | 11,608 | 0% | -19.16% |
| [Load/chat_mistral](#loadchat_mistral) | 389,808 | +0.02% |  | 52 | 0% |  | 21,024 | 0% | -52.91% |
| [Render/chat_mistral](#renderchat_mistral) | 495,276 | 0% |  | 392 | 0% |  | 9,816 | 0% | -25.68% |
| [Load/chat_qwen](#loadchat_qwen) | 263,927 | +0.03% |  | 43 | 0% |  | 14,472 | 0% | -47.04% |
| [Render/chat_qwen](#renderchat_qwen) | 274,240 | 0% |  | 211 | 0% |  | 8,280 | 0% | -2.63% |
| [Load/config_file](#loadconfig_file) | 95,025 | +0.04% |  | 24 | 0% |  | 5,520 | 0% | -47.73% |
| [Render/config_file](#renderconfig_file) | 1,254,604 | 0% |  | 882 | 0% |  | 25,976 | 0% | +9.11% |
| [Load/dict_ops](#loaddict_ops) | 32,183 | +0.01% | -78.22% | 8 | 0% | -93.89% | 2,216 | 0% | -47.93% |
| [Render/dict_ops](#renderdict_ops) | 293,242 | 0% | -57.55% | 112 | 0% | -75.11% | 24,424 | 0% | -29.61% |
| [Load/expressions](#loadexpressions) | 44,436 | 0% | -78.44% | 15 | 0% | -89.73% | 2,752 | 0% | -55.15% |
| [Render/expressions](#renderexpressions) | 414,651 | 0% | -54.24% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 67,184 | 0% | -75.52% | 26 | 0% | -88.60% | 3,856 | 0% | -66.22% |
| [Render/filters](#renderfilters) | 25,107 | 0% | -71.68% | 22 | 0% | -70.27% | 784 | 0% | -84.04% |
| [Load/for_filter_if](#loadfor_filter_if) | 28,829 | +0.04% | -78.71% | 6 | 0% | -95.12% | 1,928 | 0% | -44.08% |
| [Render/for_filter_if](#renderfor_filter_if) | 267,172 | 0% | -69.13% | 4 | 0% | -98.23% | 16,560 | 0% | +354.95% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 38,623 | +0.04% | -78.62% | 9 | 0% | -94.27% | 2,392 | 0% | -47.08% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 170,823 | 0% | -84.08% | 2 | 0% | -99.62% | 1,568 | 0% | -43.35% |
| [Load/for_range](#loadfor_range) | 12,693 | 0% | -77.61% | 6 | 0% | -89.29% | 1,128 | 0% | -43.60% |
| [Render/for_range](#renderfor_range) | 37,233 | 0% | -59.62% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/hf_deepseek_v31](#loadhf_deepseek_v31) | 387,779 | +0.01% |  | 54 | 0% |  | 22,064 | 0% | +1.14% |
| [Render/hf_deepseek_v31](#renderhf_deepseek_v31) | 90,471 | 0% |  | 42 | 0% |  | 2,456 | 0% | -0.97% |
| [Load/hf_gpt_oss](#loadhf_gpt_oss) | 1,538,246 | +0.02% |  | 282 | 0% |  | 75,192 | 0% | +1.30% |
| [Render/hf_gpt_oss](#renderhf_gpt_oss) | 170,655 | 0% |  | 153 | 0% |  | 6,000 | 0% | +21.56% |
| [Load/hf_mistral_small](#loadhf_mistral_small) | 596,468 | +0.01% |  | 87 | 0% |  | 33,168 | 0% | +0.70% |
| [Render/hf_mistral_small](#renderhf_mistral_small) | 140,451 | 0% |  | 97 | 0% |  | 6,128 | 0% | -0.13% |
| [Load/hf_qwen3](#loadhf_qwen3) | 485,165 | +0.03% |  | 81 | 0% |  | 25,744 | 0% | +1.04% |
| [Render/hf_qwen3](#renderhf_qwen3) | 117,559 | 0% |  | 99 | 0% |  | 3,224 | 0% | -0.74% |
| [Load/html_autoescape](#loadhtml_autoescape) | 190,101 | +0.04% |  | 42 | 0% |  | 12,728 | 0% | -48.43% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,197,763 | 0% |  | 674 | 0% |  | 46,256 | 0% | +14.54% |
| [Load/inheritance](#loadinheritance) | 21,536 | 0% | -81.13% | 6 | 0% | -94.39% | 1,368 | 0% | -52.37% |
| [Render/inheritance](#renderinheritance) | 137,464 | 0% | -81.62% | 35 | 0% | -94.37% | 8,168 | 0% | +35.77% |
| [Load/int_arith](#loadint_arith) | 65,572 | +0.02% |  | 19 | 0% |  | 3,896 | 0% | +1.04% |
| [Render/int_arith](#renderint_arith) | 825,531 | 0% |  | 12 | 0% |  | 3,288 | 0% | 0% |
| [Module/keep](#modulekeep) | 7,228 | 0% |  | 6 | 0% |  |  |  |  |
| [Module/keep_written](#modulekeep_written) | 29,246 | 0% |  | 10 | 0% |  |  |  |  |
| [Load/large_static](#loadlarge_static) | 147,307 | 0% | -83.37% | 13 | 0% | -97.05% | 43,128 | 0% | -19.21% |
| [Render/large_static](#renderlarge_static) | 20,340 | 0% | -72.93% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [RenderSink/large_static](#rendersinklarge_static) | 12,629 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/large_static](#rendertolarge_static) | 19,893 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/macros](#loadmacros) | 54,767 | +0.02% | -75.97% | 25 | 0% | -86.56% | 4,488 | 0% | -27.80% |
| [Render/macros](#rendermacros) | 645,214 | 0% | -74.83% | 308 | 0% | -88.71% | 20,608 | 0% | +40.00% |
| [Load/many_tags](#loadmany_tags) | 8,797,721 | +0.03% | -80.08% | 950 | 0% | -97.23% | 438,136 | 0% | -58.48% |
| [Render/many_tags](#rendermany_tags) | 732,451 | 0% | -64.84% | 2 | 0% | -90.48% | 5,648 | 0% | -10.52% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 44,985 | 0% | -81.06% | 18 | 0% | -90.22% | 3,416 | 0% | -43.67% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,129,109 | 0% | -58.49% | 33 | 0% | -99.19% | 381,536 | 0% | +9.95% |
| [RenderSink/mitsuhiko_table](#rendersinkmitsuhiko_table) | 5,059,015 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table](#rendertomitsuhiko_table) | 5,128,568 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 52,450 | 0% |  | 34 | 0% |  | 5,744 | 0% | -30.49% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,475,321 | 0% |  | 33 | 0% |  | 1,414,928 | 0% | +2.46% |
| [RenderSink/mitsuhiko_table_wide](#rendersinkmitsuhiko_table_wide) | 5,301,047 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table_wide](#rendertomitsuhiko_table_wide) | 5,474,536 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Nested/new_template](#nestednew_template) | 455,121 | 0% |  | 323 | 0% |  |  |  |  |
| [Load/plain_text](#loadplain_text) | 2,943 | 0% | -69.58% | 3 | 0% | -83.33% | 680 | 0% | -39.29% |
| [Render/plain_text](#renderplain_text) | 970 | 0% | -85.72% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/plain_text](#rendersinkplain_text) | 675 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/plain_text](#rendertoplain_text) | 771 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/prompt_budget](#loadprompt_budget) | 34,452 | +0.02% |  | 17 | 0% |  | 2,248 | 0% | +1.08% |
| [Render/prompt_budget](#renderprompt_budget) | 1,325,567 | 0% |  | 76 | 0% |  | 36,640 | 0% | +0.62% |
| [Nested/shared_globals](#nestedshared_globals) | 160,763 | -0.01% |  | 160 | 0% |  |  |  |  |
| [Load/strings](#loadstrings) | 69,236 | +0.01% | -76.28% | 29 | 0% | -87.05% | 4,384 | 0% | -51.50% |
| [Render/strings](#renderstrings) | 587,322 | 0% | -72.00% | 344 | 0% | -83.73% | 9,120 | 0% | -49.13% |
| [Load/substitute](#loadsubstitute) | 6,675 | 0% | -77.64% | 5 | 0% | -86.49% | 840 | 0% | -44.74% |
| [Render/substitute](#rendersubstitute) | 1,651 | 0% | -81.27% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/substitute](#rendersinksubstitute) | 1,360 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/substitute](#rendertosubstitute) | 1,456 | 0% |  | 0 |  |  | 0 |  |  |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/big_int_radix | 90 | 0% | 61 | 0% | 0 |  | 2,018 | -2.28% |
| Render/big_int_radix | 364,129 | 0% | 350,974 | 0% | 0 |  | 4,429 | +19.16% |
| Load/chat_llama | 776 | 0% | 770 | 0% | 0 |  | 7,974 | -1.36% |
| Render/chat_llama | 1,203 | 0% | 535 | 0% | 0 |  | 4,372 | -2.50% |
| Load/chat_mistral | 1,121 | 0% | 1,094 | -0.09% | 0 |  | 9,102 | -2.00% |
| Render/chat_mistral | 1,994 | 0% | 637 | -0.16% | 0 |  | 11,120 | -5.03% |
| Load/chat_qwen | 529 | 0% | 650 | 0% | 0 |  | 4,894 | +0.02% |
| Render/chat_qwen | 1,122 | 0% | 437 | 0% | 0 |  | 3,710 | -1.49% |
| Load/config_file | 210 | -0.47% | 191 | 0% | 0 |  | 2,827 | -3.55% |
| Render/config_file | 6,547 | +0.08% | 1,706 | +0.29% | 0 |  | 45,500 | +3.23% |
| Load/dict_ops | 44 | 0% | 32 | 0% | 0 |  | 1,382 | -3.09% |
| Render/dict_ops | 712 | 0% | 396 | 0% | 0 |  | 761 | -1.17% |
| Load/expressions | 29 | 0% | 32 | 0% | 0 |  | 1,318 | -3.65% |
| Render/expressions | 7 | 0% | 3 | 0% | 0 |  | 312 | -2.19% |
| Load/filters | 84 | 0% | 53 | 0% | 0 |  | 1,666 | -3.70% |
| Render/filters | 21 | 0% | 7 | 0% | 0 |  | 713 | +0.42% |
| Load/for_filter_if | 12 | 0% | 4 | 0% | 0 |  | 1,099 | -0.27% |
| Render/for_filter_if | 1,704 | 0% | 91 | 0% | 0 |  | 145 | -5.23% |
| Load/for_loop_vars | 23 | 0% | 17 | 0% | 0 |  | 1,156 | +1.05% |
| Render/for_loop_vars | 7 | 0% | 0 |  | 0 |  | 115 | +11.65% |
| Load/for_range | 2 | 0% | 5 | 0% | 0 |  | 760 | -2.81% |
| Render/for_range | 4 | 0% | 2 | 0% | 0 |  | 153 | -21.94% |
| Load/hf_deepseek_v31 | 964 | 0% | 975 | 0% | 0 |  | 6,687 | -4.59% |
| Render/hf_deepseek_v31 | 391 | 0% | 88 | 0% | 0 |  | 1,505 | -4.87% |
| Load/hf_gpt_oss | 4,720 | 0% | 3,404 | 0% | 0 |  | 23,744 | -3.14% |
| Render/hf_gpt_oss | 1,301 | 0% | 423 | 0% | 0 |  | 4,938 | -2.68% |
| Load/hf_mistral_small | 1,898 | 0% | 1,645 | 0% | 0 |  | 11,809 | -1.85% |
| Render/hf_mistral_small | 1,001 | 0% | 322 | 0% | 0 |  | 3,727 | -1.30% |
| Load/hf_qwen3 | 1,502 | 0% | 1,297 | 0% | 0 |  | 9,826 | +0.22% |
| Render/hf_qwen3 | 724 | 0% | 237 | 0% | 0 |  | 2,739 | -2.73% |
| Load/html_autoescape | 457 | 0% | 539 | 0% | 0 |  | 4,038 | -7.47% |
| Render/html_autoescape | 5,146 | 0% | 1,721 | 0% | 0 |  | 35,727 | -3.35% |
| Load/inheritance | 2 | 0% | 4 | +33.33% | 0 |  | 719 | +1.13% |
| Render/inheritance | 590 | +0.17% | 115 | 0% | 0 |  | 1,045 | +1.46% |
| Load/int_arith | 74 | 0% | 65 | 0% | 0 |  | 2,164 | -4.37% |
| Render/int_arith | 28 | 0% | 14 | 0% | 0 |  | 771 | +0.52% |
| Module/keep | 5 | 0% | 3 | 0% | 0 |  | 503 | +0.80% |
| Module/keep_written | 1,152 | 0% | 575 | 0% | 0 |  | 778 | -0.51% |
| Load/large_static | 1,919 | 0% | 904 | 0% | 0 |  | 259 | -14.52% |
| Render/large_static | 700 | 0% | 571 | 0% | 0 |  | 5 | 0% |
| RenderSink/large_static | 1 | 0% | 0 |  | 0 |  | 3 | +50.00% |
| RenderTo/large_static | 679 | 0% | 573 | 0% | 0 |  | 4 | -20.00% |
| Load/macros | 64 | 0% | 57 | 0% | 0 |  | 1,772 | -3.01% |
| Render/macros | 754 | 0% | 246 | 0% | 0 |  | 705 | +10.50% |
| Load/many_tags | 28,324 | 0% | 22,639 | 0% | 0 |  | 244,133 | -6.42% |
| Render/many_tags | 6,123 | 0% | 804 | 0% | 0 |  | 101 | +119.57% |
| Load/mitsuhiko_table | 38 | 0% | 48 | 0% | 0 |  | 930 | -5.78% |
| Render/mitsuhiko_table | 10,056 | 0% | 6,341 | 0% | 0 |  | 327 | +0.62% |
| RenderSink/mitsuhiko_table | 9,955 | 0% | 1,193 | 0% | 0 |  | 254 | +4.96% |
| RenderTo/mitsuhiko_table | 10,063 | 0% | 6,296 | 0% | 0 |  | 286 | +0.70% |
| Load/mitsuhiko_table_wide | 88 | 0% | 79 | 0% | 0 |  | 1,199 | +23.61% |
| Render/mitsuhiko_table_wide | 10,147 | 0% | 22,550 | 0% | 0 |  | 309 | +5.10% |
| RenderSink/mitsuhiko_table_wide | 10,013 | 0% | 1,243 | 0% | 0 |  | 271 | +22.62% |
| RenderTo/mitsuhiko_table_wide | 10,146 | 0% | 22,656 | 0% | 0 |  | 295 | +12.60% |
| Nested/new_template | 2,360 | 0% | 1,653 | 0% | 0 |  | 24,569 | -2.40% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 19 | +90.00% |
| Render/plain_text | 1 | 0% | 0 |  | 0 |  | 6 | -14.29% |
| RenderSink/plain_text | 1 | 0% | 0 |  | 0 |  | 5 | 0% |
| RenderTo/plain_text | 1 | 0% | 0 |  | 0 |  | 6 | 0% |
| Load/prompt_budget | 63 | 0% | 48 | 0% | 0 |  | 1,401 | -3.58% |
| Render/prompt_budget | 4,780 | +0.06% | 543 | 0% | 0 |  | 1,186 | +43.58% |
| Nested/shared_globals | 253 | 0% | 123 | 0% | 0 |  | 9,260 | +0.34% |
| Load/strings | 140 | 0% | 82 | 0% | 0 |  | 2,895 | -1.23% |
| Render/strings | 429 | 0% | 480 | 0% | 0 |  | 1,617 | +2.08% |
| Load/substitute | 1 | 0% | 3 | 0% | 0 |  | 144 | -2.70% |
| Render/substitute | 1 | 0% | 0 |  | 0 |  | 5 | 0% |
| RenderSink/substitute | 1 | 0% | 0 |  | 0 |  | 3 | +50.00% |
| RenderTo/substitute | 1 | 0% | 0 |  | 0 |  | 4 | 0% |

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
