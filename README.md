# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (235 so far, latest [27cc505](https://github.com/jinja2cpp/Jinja2Cpp/commit/27cc505d294cccc3270b219b640c10695c3f2b6d) on 2026-11-16). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 335,524 | 0% |  | 54 | 0% |  | 18,472 | 0% | -52.73% |
| [Render/chat_llama](#renderchat_llama) | 288,853 | -0.04% |  | 224 | 0% |  | 11,592 | 0% | -19.28% |
| [Load/chat_mistral](#loadchat_mistral) | 385,600 | 0% |  | 51 | 0% |  | 20,896 | 0% | -53.20% |
| [Render/chat_mistral](#renderchat_mistral) | 491,161 | -0.03% |  | 401 | 0% |  | 9,992 | 0% | -24.35% |
| [Load/chat_qwen](#loadchat_qwen) | 261,340 | 0% |  | 43 | 0% |  | 14,424 | 0% | -47.22% |
| [Render/chat_qwen](#renderchat_qwen) | 271,637 | -0.04% |  | 211 | 0% |  | 8,280 | 0% | -2.63% |
| [Load/config_file](#loadconfig_file) | 94,414 | 0% |  | 24 | 0% |  | 5,472 | 0% | -48.18% |
| [Render/config_file](#renderconfig_file) | 1,262,622 | -0.52% |  | 908 | 0% |  | 25,576 | 0% | +7.43% |
| [Load/dict_ops](#loaddict_ops) | 32,040 | 0% | -78.32% | 8 | 0% | -93.89% | 2,184 | 0% | -48.68% |
| [Render/dict_ops](#renderdict_ops) | 290,441 | -0.05% | -57.96% | 112 | 0% | -75.11% | 24,408 | 0% | -29.65% |
| [Load/expressions](#loadexpressions) | 44,017 | 0% | -78.65% | 15 | 0% | -89.73% | 2,736 | 0% | -55.41% |
| [Render/expressions](#renderexpressions) | 415,811 | -0.02% | -54.11% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 67,165 | 0% | -75.53% | 26 | 0% | -88.60% | 3,856 | 0% | -66.22% |
| [Render/filters](#renderfilters) | 26,383 | 0% | -70.25% | 23 | 0% | -68.92% | 752 | 0% | -84.69% |
| [Load/for_filter_if](#loadfor_filter_if) | 28,875 | 0% | -78.67% | 6 | 0% | -95.12% | 1,912 | 0% | -44.55% |
| [Render/for_filter_if](#renderfor_filter_if) | 267,395 | -0.03% | -69.10% | 4 | 0% | -98.23% | 16,560 | 0% | +354.95% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 38,728 | 0% | -78.56% | 9 | 0% | -94.27% | 2,376 | 0% | -47.43% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 170,641 | -0.04% | -84.09% | 2 | 0% | -99.62% | 1,568 | 0% | -43.35% |
| [Load/for_range](#loadfor_range) | 12,931 | 0% | -77.19% | 6 | 0% | -89.29% | 1,096 | 0% | -45.20% |
| [Render/for_range](#renderfor_range) | 37,046 | -0.19% | -59.82% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/hf_deepseek_v31](#loadhf_deepseek_v31) | 384,412 | 0% |  | 54 | 0% |  | 21,984 | 0% | +0.77% |
| [Render/hf_deepseek_v31](#renderhf_deepseek_v31) | 89,961 | -0.23% |  | 42 | 0% |  | 2,456 | 0% | -0.97% |
| [Load/hf_gpt_oss](#loadhf_gpt_oss) | 1,523,483 | 0% |  | 282 | 0% |  | 74,776 | 0% | +0.74% |
| [Render/hf_gpt_oss](#renderhf_gpt_oss) | 167,293 | -0.07% |  | 154 | 0% |  | 5,664 | 0% | +14.75% |
| [Load/hf_mistral_small](#loadhf_mistral_small) | 592,078 | 0% |  | 87 | 0% |  | 33,072 | 0% | +0.41% |
| [Render/hf_mistral_small](#renderhf_mistral_small) | 139,104 | -0.04% |  | 98 | 0% |  | 6,096 | 0% | -0.65% |
| [Load/hf_qwen3](#loadhf_qwen3) | 476,840 | 0% |  | 81 | 0% |  | 25,568 | 0% | +0.35% |
| [Render/hf_qwen3](#renderhf_qwen3) | 116,711 | +0.03% |  | 99 | 0% |  | 3,224 | 0% | -0.74% |
| [Load/html_autoescape](#loadhtml_autoescape) | 189,482 | 0% |  | 42 | 0% |  | 12,680 | 0% | -48.62% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,195,275 | -0.02% |  | 674 | 0% |  | 45,968 | 0% | +13.83% |
| [Load/inheritance](#loadinheritance) | 21,544 | 0% | -81.12% | 6 | 0% | -94.39% | 1,352 | 0% | -52.92% |
| [Render/inheritance](#renderinheritance) | 134,056 | -0.05% | -82.07% | 35 | 0% | -94.37% | 7,960 | 0% | +32.31% |
| [Load/int_arith](#loadint_arith) | 64,949 | 0% |  | 19 | 0% |  | 3,880 | 0% | +0.62% |
| [Render/int_arith](#renderint_arith) | 827,268 | -0.01% |  | 12 | 0% |  | 3,272 | 0% | -0.49% |
| [Module/keep](#modulekeep) | 7,127 | 0% |  | 7 | 0% |  |  |  |  |
| [Module/keep_written](#modulekeep_written) | 29,266 | 0% |  | 11 | 0% |  |  |  |  |
| [Load/large_static](#loadlarge_static) | 147,594 | 0% | -83.34% | 13 | 0% | -97.05% | 43,128 | 0% | -19.21% |
| [Render/large_static](#renderlarge_static) | 20,322 | 0% | -72.95% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [RenderSink/large_static](#rendersinklarge_static) | 12,310 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/large_static](#rendertolarge_static) | 19,885 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/macros](#loadmacros) | 54,689 | 0% | -76.01% | 25 | 0% | -86.56% | 4,424 | 0% | -28.83% |
| [Render/macros](#rendermacros) | 640,833 | -0.01% | -75.00% | 308 | 0% | -88.71% | 20,576 | 0% | +39.78% |
| [Load/many_tags](#loadmany_tags) | 8,764,284 | 0% | -80.16% | 950 | 0% | -97.23% | 438,120 | 0% | -58.48% |
| [Render/many_tags](#rendermany_tags) | 730,181 | 0% | -64.95% | 2 | 0% | -90.48% | 5,648 | 0% | -10.52% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 44,868 | 0% | -81.11% | 18 | 0% | -90.22% | 3,368 | 0% | -44.46% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,068,134 | -1.37% | -58.98% | 33 | 0% | -99.19% | 381,536 | 0% | +9.95% |
| [RenderSink/mitsuhiko_table](#rendersinkmitsuhiko_table) | 4,995,275 | -1.38% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table](#rendertomitsuhiko_table) | 5,067,640 | -1.37% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 52,588 | 0% |  | 34 | 0% |  | 5,696 | 0% | -31.07% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,414,545 | -1.28% |  | 33 | 0% |  | 1,414,928 | 0% | +2.46% |
| [RenderSink/mitsuhiko_table_wide](#rendersinkmitsuhiko_table_wide) | 5,237,717 | -1.32% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table_wide](#rendertomitsuhiko_table_wide) | 5,413,964 | -1.28% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/plain_text](#loadplain_text) | 3,338 | 0% | -65.50% | 3 | 0% | -83.33% | 680 | 0% | -39.29% |
| [Render/plain_text](#renderplain_text) | 951 | 0% | -86.00% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/plain_text](#rendersinkplain_text) | 651 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/plain_text](#rendertoplain_text) | 752 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/prompt_budget](#loadprompt_budget) | 34,212 | 0% |  | 17 | 0% |  | 2,232 | 0% | +0.36% |
| [Render/prompt_budget](#renderprompt_budget) | 1,319,253 | -0.01% |  | 76 | 0% |  | 36,416 | 0% | 0% |
| [Load/strings](#loadstrings) | 68,998 | 0% | -76.36% | 29 | 0% | -87.05% | 4,352 | 0% | -51.86% |
| [Render/strings](#renderstrings) | 580,440 | -0.02% | -72.33% | 344 | 0% | -83.73% | 9,088 | 0% | -49.31% |
| [Load/substitute](#loadsubstitute) | 7,118 | 0% | -76.16% | 5 | 0% | -86.49% | 840 | 0% | -44.74% |
| [Render/substitute](#rendersubstitute) | 1,631 | 0% | -81.49% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/substitute](#rendersinksubstitute) | 1,335 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/substitute](#rendertosubstitute) | 1,436 | 0% |  | 0 |  |  | 0 |  |  |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 735 | 0% | 748 | 0% | 0 |  | 8,222 | +2.23% |
| Render/chat_llama | 1,160 | -0.17% | 522 | 0% | 0 |  | 4,072 | -0.51% |
| Load/chat_mistral | 1,083 | 0% | 1,073 | 0% | 0 |  | 9,696 | +2.52% |
| Render/chat_mistral | 1,913 | -0.16% | 584 | 0% | 0 |  | 10,278 | +0.22% |
| Load/chat_qwen | 565 | 0% | 627 | 0% | 0 |  | 5,193 | +2.61% |
| Render/chat_qwen | 1,085 | -0.09% | 424 | 0% | 0 |  | 3,498 | +1.39% |
| Load/config_file | 222 | 0% | 189 | 0% | 0 |  | 2,927 | +4.31% |
| Render/config_file | 5,889 | -0.56% | 1,599 | -0.50% | 0 |  | 42,846 | -3.51% |
| Load/dict_ops | 46 | 0% | 26 | 0% | 0 |  | 1,429 | +3.25% |
| Render/dict_ops | 668 | -0.15% | 419 | 0% | 0 |  | 810 | -2.06% |
| Load/expressions | 46 | 0% | 32 | 0% | 0 |  | 1,343 | +2.60% |
| Render/expressions | 10 | +11.11% | 4 | 0% | 0 |  | 287 | -2.38% |
| Load/filters | 101 | 0% | 47 | 0% | 0 |  | 1,682 | +4.67% |
| Render/filters | 9 | 0% | 0 |  | 0 |  | 745 | +0.68% |
| Load/for_filter_if | 34 | 0% | 18 | 0% | 0 |  | 1,182 | +2.16% |
| Render/for_filter_if | 1,700 | -0.06% | 90 | 0% | 0 |  | 99 | -36.94% |
| Load/for_loop_vars | 48 | 0% | 26 | 0% | 0 |  | 1,216 | +4.65% |
| Render/for_loop_vars | 11 | 0% | 2 | 0% | 0 |  | 114 | -8.80% |
| Load/for_range | 3 | 0% | 5 | 0% | 0 |  | 794 | +5.44% |
| Render/for_range | 1 | 0% | 0 |  | 0 |  | 136 | +28.30% |
| Load/hf_deepseek_v31 | 1,061 | 0% | 1,003 | 0% | 0 |  | 7,733 | +4.23% |
| Render/hf_deepseek_v31 | 330 | 0% | 78 | -1.27% | 0 |  | 1,459 | +1.32% |
| Load/hf_gpt_oss | 4,631 | 0% | 3,347 | 0% | 0 |  | 25,787 | +3.10% |
| Render/hf_gpt_oss | 1,248 | -0.16% | 403 | 0% | 0 |  | 4,669 | +1.97% |
| Load/hf_mistral_small | 1,912 | 0% | 1,657 | 0% | 0 |  | 12,741 | +2.99% |
| Render/hf_mistral_small | 975 | -0.31% | 288 | 0% | 0 |  | 3,713 | +2.48% |
| Load/hf_qwen3 | 1,524 | 0% | 1,347 | 0% | 0 |  | 10,341 | +3.31% |
| Render/hf_qwen3 | 676 | 0% | 219 | 0% | 0 |  | 2,625 | -0.38% |
| Load/html_autoescape | 478 | 0% | 534 | 0% | 0 |  | 4,456 | +4.75% |
| Render/html_autoescape | 5,456 | 0% | 1,772 | 0% | 0 |  | 36,042 | +0.53% |
| Load/inheritance | 8 | 0% | 4 | 0% | 0 |  | 712 | -0.42% |
| Render/inheritance | 595 | -0.17% | 109 | 0% | 0 |  | 933 | -0.85% |
| Load/int_arith | 90 | 0% | 64 | 0% | 0 |  | 2,186 | +6.12% |
| Render/int_arith | 8 | 0% | 3 | 0% | 0 |  | 696 | -3.87% |
| Module/keep | 2 | 0% | 2 | 0% | 0 |  | 443 | +9.93% |
| Module/keep_written | 1,133 | 0% | 563 | 0% | 0 |  | 751 | -0.92% |
| Load/large_static | 1,927 | 0% | 907 | 0% | 0 |  | 330 | +20.88% |
| Render/large_static | 702 | 0% | 569 | 0% | 0 |  | 13 | +225.00% |
| RenderSink/large_static | 1 | 0% | 0 |  | 0 |  | 2 | 0% |
| RenderTo/large_static | 678 | 0% | 572 | 0% | 0 |  | 5 | +25.00% |
| Load/macros | 85 | 0% | 70 | 0% | 0 |  | 1,821 | +3.47% |
| Render/macros | 749 | -0.40% | 246 | -0.40% | 0 |  | 2,222 | +133.89% |
| Load/many_tags | 27,811 | 0% | 22,621 | 0% | 0 |  | 257,245 | +2.82% |
| Render/many_tags | 6,096 | 0% | 755 | 0% | 0 |  | 53 | -32.91% |
| Load/mitsuhiko_table | 49 | 0% | 48 | 0% | 0 |  | 962 | +6.18% |
| Render/mitsuhiko_table | 10,023 | -0.03% | 6,306 | 0% | 0 |  | 314 | -3.38% |
| RenderSink/mitsuhiko_table | 9,965 | -0.02% | 1,209 | 0% | 0 |  | 251 | -5.64% |
| RenderTo/mitsuhiko_table | 10,036 | -0.04% | 6,308 | 0% | 0 |  | 281 | -10.22% |
| Load/mitsuhiko_table_wide | 98 | 0% | 94 | 0% | 0 |  | 972 | +1.36% |
| Render/mitsuhiko_table_wide | 10,246 | -0.08% | 22,740 | 0% | 0 |  | 318 | -1.55% |
| RenderSink/mitsuhiko_table_wide | 10,056 | -0.06% | 1,320 | -0.38% | 0 |  | 260 | +1.17% |
| RenderTo/mitsuhiko_table_wide | 10,246 | -0.08% | 22,733 | 0% | 0 |  | 294 | -0.34% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 25 | -30.56% |
| Render/plain_text | 1 | 0% | 0 |  | 0 |  | 5 | 0% |
| RenderSink/plain_text | 1 | 0% | 0 |  | 0 |  | 4 | +33.33% |
| RenderTo/plain_text | 1 | 0% | 0 |  | 0 |  | 5 | +25.00% |
| Load/prompt_budget | 81 | 0% | 49 | 0% | 0 |  | 1,450 | +4.09% |
| Render/prompt_budget | 4,787 | -0.06% | 543 | 0% | 0 |  | 896 | -10.85% |
| Load/strings | 159 | 0% | 128 | 0% | 0 |  | 2,900 | +3.91% |
| Render/strings | 401 | +0.25% | 463 | 0% | 0 |  | 1,537 | -2.66% |
| Load/substitute | 2 | 0% | 3 | 0% | 0 |  | 159 | +29.27% |
| Render/substitute | 1 | 0% | 0 |  | 0 |  | 5 | +25.00% |
| RenderSink/substitute | 0 |  | 0 |  | 0 |  | 3 | 0% |
| RenderTo/substitute | 1 | 0% | 0 |  | 0 |  | 5 | +66.67% |

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
