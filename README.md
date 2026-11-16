# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (234 so far, latest [6838d57](https://github.com/jinja2cpp/Jinja2Cpp/commit/6838d57d5c08ed3929f88adaeaab769bb394eeab) on 2026-11-16). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 335,524 | 0% |  | 54 | 0% |  | 18,472 | 0% | -52.73% |
| [Render/chat_llama](#renderchat_llama) | 288,957 | 0% |  | 224 | 0% |  | 11,592 | 0% | -19.28% |
| [Load/chat_mistral](#loadchat_mistral) | 385,600 | 0% |  | 51 | 0% |  | 20,896 | 0% | -53.20% |
| [Render/chat_mistral](#renderchat_mistral) | 491,311 | 0% |  | 401 | 0% |  | 9,992 | 0% | -24.35% |
| [Load/chat_qwen](#loadchat_qwen) | 261,340 | 0% |  | 43 | 0% |  | 14,424 | 0% | -47.22% |
| [Render/chat_qwen](#renderchat_qwen) | 271,745 | 0% |  | 211 | 0% |  | 8,280 | 0% | -2.63% |
| [Load/config_file](#loadconfig_file) | 94,414 | 0% |  | 24 | 0% |  | 5,472 | 0% | -48.18% |
| [Render/config_file](#renderconfig_file) | 1,269,268 | 0% |  | 908 | 0% |  | 25,576 | 0% | +7.43% |
| [Load/dict_ops](#loaddict_ops) | 32,040 | 0% | -78.32% | 8 | 0% | -93.89% | 2,184 | 0% | -48.68% |
| [Render/dict_ops](#renderdict_ops) | 290,581 | 0% | -57.94% | 112 | 0% | -75.11% | 24,408 | 0% | -29.65% |
| [Load/expressions](#loadexpressions) | 44,017 | 0% | -78.65% | 15 | 0% | -89.73% | 2,736 | 0% | -55.41% |
| [Render/expressions](#renderexpressions) | 415,881 | 0% | -54.10% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 67,165 | 0% | -75.53% | 26 | 0% | -88.60% | 3,856 | 0% | -66.22% |
| [Render/filters](#renderfilters) | 26,383 | 0% | -70.25% | 23 | 0% | -68.92% | 752 | 0% | -84.69% |
| [Load/for_filter_if](#loadfor_filter_if) | 28,875 | 0% | -78.67% | 6 | 0% | -95.12% | 1,912 | 0% | -44.55% |
| [Render/for_filter_if](#renderfor_filter_if) | 267,465 | 0% | -69.10% | 4 | 0% | -98.23% | 16,560 | 0% | +354.95% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 38,728 | 0% | -78.56% | 9 | 0% | -94.27% | 2,376 | 0% | -47.43% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 170,711 | 0% | -84.09% | 2 | 0% | -99.62% | 1,568 | 0% | -43.35% |
| [Load/for_range](#loadfor_range) | 12,931 | 0% | -77.19% | 6 | 0% | -89.29% | 1,096 | 0% | -45.20% |
| [Render/for_range](#renderfor_range) | 37,116 | 0% | -59.75% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/hf_deepseek_v31](#loadhf_deepseek_v31) | 384,412 | 0% |  | 54 | 0% |  | 21,984 | 0% | +0.77% |
| [Render/hf_deepseek_v31](#renderhf_deepseek_v31) | 90,167 | 0% |  | 42 | 0% |  | 2,456 | 0% | -0.97% |
| [Load/hf_gpt_oss](#loadhf_gpt_oss) | 1,523,483 | 0% |  | 282 | 0% |  | 74,776 | 0% | +0.74% |
| [Render/hf_gpt_oss](#renderhf_gpt_oss) | 167,413 | 0% |  | 154 | 0% |  | 5,664 | 0% | +14.75% |
| [Load/hf_mistral_small](#loadhf_mistral_small) | 592,078 | 0% |  | 87 | 0% |  | 33,072 | 0% | +0.41% |
| [Render/hf_mistral_small](#renderhf_mistral_small) | 139,154 | 0% |  | 98 | 0% |  | 6,096 | 0% | -0.65% |
| [Load/hf_qwen3](#loadhf_qwen3) | 476,840 | 0% |  | 81 | 0% |  | 25,568 | 0% | +0.35% |
| [Render/hf_qwen3](#renderhf_qwen3) | 116,675 | 0% |  | 99 | 0% |  | 3,224 | 0% | -0.74% |
| [Load/html_autoescape](#loadhtml_autoescape) | 189,482 | 0% |  | 42 | 0% |  | 12,680 | 0% | -48.62% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,195,481 | 0% |  | 674 | 0% |  | 45,968 | 0% | +13.83% |
| [Load/inheritance](#loadinheritance) | 21,544 | 0% | -81.12% | 6 | 0% | -94.39% | 1,352 | 0% | -52.92% |
| [Render/inheritance](#renderinheritance) | 134,126 | 0% | -82.07% | 35 | 0% | -94.37% | 7,960 | 0% | +32.31% |
| [Load/int_arith](#loadint_arith) | 64,949 | 0% |  | 19 | 0% |  | 3,880 | 0% | +0.62% |
| [Render/int_arith](#renderint_arith) | 827,334 | 0% |  | 12 | 0% |  | 3,272 | 0% | -0.49% |
| [Module/keep](#modulekeep) | 7,127 | 0% |  | 7 | 0% |  |  |  |  |
| [Module/keep_written](#modulekeep_written) | 29,266 | 0% |  | 11 | 0% |  |  |  |  |
| [Load/large_static](#loadlarge_static) | 147,594 | 0% | -83.34% | 13 | 0% | -97.05% | 43,128 | 0% | -19.21% |
| [Render/large_static](#renderlarge_static) | 20,322 | 0% | -72.95% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [RenderSink/large_static](#rendersinklarge_static) | 12,310 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/large_static](#rendertolarge_static) | 19,885 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/macros](#loadmacros) | 54,689 | 0% | -76.01% | 25 | 0% | -86.56% | 4,424 | 0% | -28.83% |
| [Render/macros](#rendermacros) | 640,899 | 0% | -74.99% | 308 | 0% | -88.71% | 20,576 | 0% | +39.78% |
| [Load/many_tags](#loadmany_tags) | 8,764,284 | 0% | -80.16% | 950 | 0% | -97.23% | 438,120 | 0% | -58.48% |
| [Render/many_tags](#rendermany_tags) | 730,177 | 0% | -64.95% | 2 | 0% | -90.48% | 5,648 | 0% | -10.52% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 44,868 | 0% | -81.11% | 18 | 0% | -90.22% | 3,368 | 0% | -44.46% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,138,274 | 0% | -58.42% | 33 | 0% | -99.19% | 381,536 | 0% | +9.95% |
| [RenderSink/mitsuhiko_table](#rendersinkmitsuhiko_table) | 5,065,415 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table](#rendertomitsuhiko_table) | 5,137,780 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 52,588 | 0% |  | 34 | 0% |  | 5,696 | 0% | -31.07% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,484,685 | 0% |  | 33 | 0% |  | 1,414,928 | 0% | +2.46% |
| [RenderSink/mitsuhiko_table_wide](#rendersinkmitsuhiko_table_wide) | 5,307,857 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table_wide](#rendertomitsuhiko_table_wide) | 5,484,104 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/plain_text](#loadplain_text) | 3,338 | 0% | -65.50% | 3 | 0% | -83.33% | 680 | 0% | -39.29% |
| [Render/plain_text](#renderplain_text) | 951 | 0% | -86.00% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/plain_text](#rendersinkplain_text) | 651 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/plain_text](#rendertoplain_text) | 752 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/prompt_budget](#loadprompt_budget) | 34,212 | 0% |  | 17 | 0% |  | 2,232 | 0% | +0.36% |
| [Render/prompt_budget](#renderprompt_budget) | 1,319,323 | 0% |  | 76 | 0% |  | 36,416 | 0% | 0% |
| [Load/strings](#loadstrings) | 68,998 | 0% | -76.36% | 29 | 0% | -87.05% | 4,352 | 0% | -51.86% |
| [Render/strings](#renderstrings) | 580,576 | 0% | -72.32% | 344 | 0% | -83.73% | 9,088 | 0% | -49.31% |
| [Load/substitute](#loadsubstitute) | 7,118 | 0% | -76.16% | 5 | 0% | -86.49% | 840 | 0% | -44.74% |
| [Render/substitute](#rendersubstitute) | 1,631 | 0% | -81.49% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/substitute](#rendersinksubstitute) | 1,335 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/substitute](#rendertosubstitute) | 1,436 | 0% |  | 0 |  |  | 0 |  |  |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 735 | 0% | 748 | 0% | 0 |  | 8,043 | 0% |
| Render/chat_llama | 1,162 | 0% | 522 | 0% | 0 |  | 4,093 | 0% |
| Load/chat_mistral | 1,083 | 0% | 1,073 | 0% | 0 |  | 9,458 | 0% |
| Render/chat_mistral | 1,916 | 0% | 584 | 0% | 0 |  | 10,255 | 0% |
| Load/chat_qwen | 565 | 0% | 627 | 0% | 0 |  | 5,061 | 0% |
| Render/chat_qwen | 1,086 | 0% | 424 | 0% | 0 |  | 3,450 | 0% |
| Load/config_file | 222 | 0% | 189 | 0% | 0 |  | 2,806 | 0% |
| Render/config_file | 5,922 | 0% | 1,607 | 0% | 0 |  | 44,405 | 0% |
| Load/dict_ops | 46 | 0% | 26 | 0% | 0 |  | 1,384 | 0% |
| Render/dict_ops | 669 | 0% | 419 | 0% | 0 |  | 827 | 0% |
| Load/expressions | 46 | 0% | 32 | 0% | 0 |  | 1,309 | 0% |
| Render/expressions | 9 | 0% | 4 | 0% | 0 |  | 294 | 0% |
| Load/filters | 101 | 0% | 47 | 0% | 0 |  | 1,607 | 0% |
| Render/filters | 9 | 0% | 0 |  | 0 |  | 740 | 0% |
| Load/for_filter_if | 34 | 0% | 18 | 0% | 0 |  | 1,157 | 0% |
| Render/for_filter_if | 1,701 | 0% | 90 | 0% | 0 |  | 157 | 0% |
| Load/for_loop_vars | 48 | 0% | 26 | 0% | 0 |  | 1,162 | 0% |
| Render/for_loop_vars | 11 | 0% | 2 | 0% | 0 |  | 125 | 0% |
| Load/for_range | 3 | 0% | 5 | 0% | 0 |  | 753 | 0% |
| Render/for_range | 1 | 0% | 0 |  | 0 |  | 106 | 0% |
| Load/hf_deepseek_v31 | 1,061 | 0% | 1,003 | 0% | 0 |  | 7,419 | 0% |
| Render/hf_deepseek_v31 | 330 | 0% | 79 | 0% | 0 |  | 1,440 | 0% |
| Load/hf_gpt_oss | 4,631 | 0% | 3,347 | 0% | 0 |  | 25,012 | 0% |
| Render/hf_gpt_oss | 1,250 | 0% | 403 | 0% | 0 |  | 4,579 | 0% |
| Load/hf_mistral_small | 1,912 | 0% | 1,657 | 0% | 0 |  | 12,371 | 0% |
| Render/hf_mistral_small | 978 | 0% | 288 | 0% | 0 |  | 3,623 | 0% |
| Load/hf_qwen3 | 1,524 | 0% | 1,347 | 0% | 0 |  | 10,010 | 0% |
| Render/hf_qwen3 | 676 | 0% | 219 | 0% | 0 |  | 2,635 | 0% |
| Load/html_autoescape | 478 | 0% | 534 | 0% | 0 |  | 4,254 | 0% |
| Render/html_autoescape | 5,456 | 0% | 1,772 | 0% | 0 |  | 35,853 | 0% |
| Load/inheritance | 8 | 0% | 4 | 0% | 0 |  | 715 | 0% |
| Render/inheritance | 596 | 0% | 109 | 0% | 0 |  | 941 | 0% |
| Load/int_arith | 90 | 0% | 64 | 0% | 0 |  | 2,060 | 0% |
| Render/int_arith | 8 | 0% | 3 | 0% | 0 |  | 724 | 0% |
| Module/keep | 2 | 0% | 2 | 0% | 0 |  | 403 | 0% |
| Module/keep_written | 1,133 | 0% | 563 | 0% | 0 |  | 758 | 0% |
| Load/large_static | 1,927 | 0% | 907 | 0% | 0 |  | 273 | 0% |
| Render/large_static | 702 | 0% | 569 | 0% | 0 |  | 4 | 0% |
| RenderSink/large_static | 1 | 0% | 0 |  | 0 |  | 2 | 0% |
| RenderTo/large_static | 678 | 0% | 572 | 0% | 0 |  | 4 | 0% |
| Load/macros | 85 | 0% | 70 | 0% | 0 |  | 1,760 | 0% |
| Render/macros | 752 | 0% | 247 | 0% | 0 |  | 950 | 0% |
| Load/many_tags | 27,811 | 0% | 22,621 | 0% | 0 |  | 250,183 | 0% |
| Render/many_tags | 6,096 | 0% | 755 | 0% | 0 |  | 79 | 0% |
| Load/mitsuhiko_table | 49 | 0% | 48 | 0% | 0 |  | 906 | 0% |
| Render/mitsuhiko_table | 10,026 | 0% | 6,306 | 0% | 0 |  | 325 | 0% |
| RenderSink/mitsuhiko_table | 9,967 | 0% | 1,209 | 0% | 0 |  | 266 | 0% |
| RenderTo/mitsuhiko_table | 10,040 | 0% | 6,308 | 0% | 0 |  | 313 | 0% |
| Load/mitsuhiko_table_wide | 98 | 0% | 94 | 0% | 0 |  | 959 | 0% |
| Render/mitsuhiko_table_wide | 10,254 | 0% | 22,740 | 0% | 0 |  | 323 | 0% |
| RenderSink/mitsuhiko_table_wide | 10,062 | 0% | 1,325 | 0% | 0 |  | 257 | 0% |
| RenderTo/mitsuhiko_table_wide | 10,254 | 0% | 22,733 | 0% | 0 |  | 295 | 0% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 36 | 0% |
| Render/plain_text | 1 | 0% | 0 |  | 0 |  | 5 | 0% |
| RenderSink/plain_text | 1 | 0% | 0 |  | 0 |  | 3 | 0% |
| RenderTo/plain_text | 1 | 0% | 0 |  | 0 |  | 4 | 0% |
| Load/prompt_budget | 81 | 0% | 49 | 0% | 0 |  | 1,393 | 0% |
| Render/prompt_budget | 4,790 | 0% | 543 | 0% | 0 |  | 1,005 | 0% |
| Load/strings | 159 | 0% | 128 | 0% | 0 |  | 2,791 | 0% |
| Render/strings | 400 | 0% | 463 | 0% | 0 |  | 1,579 | 0% |
| Load/substitute | 2 | 0% | 3 | 0% | 0 |  | 123 | 0% |
| Render/substitute | 1 | 0% | 0 |  | 0 |  | 4 | 0% |
| RenderSink/substitute | 0 |  | 0 |  | 0 |  | 3 | 0% |
| RenderTo/substitute | 1 | 0% | 0 |  | 0 |  | 3 | 0% |

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
