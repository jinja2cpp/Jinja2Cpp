# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (238 so far, latest [53eb4bd](https://github.com/jinja2cpp/Jinja2Cpp/commit/53eb4bd02186224b3ee20498a52bf765aeafd34e) on 2026-11-17). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 334,494 | 0% |  | 54 | 0% |  | 18,472 | 0% | -52.73% |
| [Render/chat_llama](#renderchat_llama) | 289,806 | 0% |  | 224 | 0% |  | 11,592 | 0% | -19.28% |
| [Load/chat_mistral](#loadchat_mistral) | 384,137 | 0% |  | 51 | 0% |  | 20,896 | 0% | -53.20% |
| [Render/chat_mistral](#renderchat_mistral) | 491,447 | 0% |  | 401 | 0% |  | 9,944 | 0% | -24.71% |
| [Load/chat_qwen](#loadchat_qwen) | 260,603 | 0% |  | 43 | 0% |  | 14,424 | 0% | -47.22% |
| [Render/chat_qwen](#renderchat_qwen) | 271,584 | 0% |  | 211 | 0% |  | 8,280 | 0% | -2.63% |
| [Load/config_file](#loadconfig_file) | 93,871 | 0% |  | 24 | 0% |  | 5,472 | 0% | -48.18% |
| [Render/config_file](#renderconfig_file) | 1,262,797 | 0% |  | 908 | 0% |  | 25,576 | 0% | +7.43% |
| [Load/dict_ops](#loaddict_ops) | 31,645 | 0% | -78.59% | 8 | 0% | -93.89% | 2,184 | 0% | -48.68% |
| [Render/dict_ops](#renderdict_ops) | 290,441 | 0% | -57.96% | 112 | 0% | -75.11% | 24,408 | 0% | -29.65% |
| [Load/expressions](#loadexpressions) | 43,590 | 0% | -78.85% | 15 | 0% | -89.73% | 2,736 | 0% | -55.41% |
| [Render/expressions](#renderexpressions) | 415,811 | 0% | -54.11% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 66,674 | 0% | -75.71% | 26 | 0% | -88.60% | 3,856 | 0% | -66.22% |
| [Render/filters](#renderfilters) | 26,383 | 0% | -70.25% | 23 | 0% | -68.92% | 752 | 0% | -84.69% |
| [Load/for_filter_if](#loadfor_filter_if) | 28,486 | 0% | -78.96% | 6 | 0% | -95.12% | 1,912 | 0% | -44.55% |
| [Render/for_filter_if](#renderfor_filter_if) | 267,395 | 0% | -69.10% | 4 | 0% | -98.23% | 16,560 | 0% | +354.95% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 38,323 | 0% | -78.79% | 9 | 0% | -94.27% | 2,376 | 0% | -47.43% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 170,641 | 0% | -84.09% | 2 | 0% | -99.62% | 1,568 | 0% | -43.35% |
| [Load/for_range](#loadfor_range) | 12,407 | 0% | -78.11% | 6 | 0% | -89.29% | 1,096 | 0% | -45.20% |
| [Render/for_range](#renderfor_range) | 37,046 | 0% | -59.82% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/hf_deepseek_v31](#loadhf_deepseek_v31) | 383,025 | 0% |  | 54 | 0% |  | 21,984 | 0% | +0.77% |
| [Render/hf_deepseek_v31](#renderhf_deepseek_v31) | 89,958 | 0% |  | 42 | 0% |  | 2,456 | 0% | -0.97% |
| [Load/hf_gpt_oss](#loadhf_gpt_oss) | 1,520,419 | 0% |  | 282 | 0% |  | 74,744 | 0% | +0.70% |
| [Render/hf_gpt_oss](#renderhf_gpt_oss) | 167,094 | 0% |  | 153 | 0% |  | 5,664 | 0% | +14.75% |
| [Load/hf_mistral_small](#loadhf_mistral_small) | 589,374 | 0% |  | 87 | 0% |  | 33,056 | 0% | +0.36% |
| [Render/hf_mistral_small](#renderhf_mistral_small) | 139,112 | 0% |  | 97 | 0% |  | 6,112 | 0% | -0.39% |
| [Load/hf_qwen3](#loadhf_qwen3) | 475,548 | 0% |  | 81 | 0% |  | 25,568 | 0% | +0.35% |
| [Render/hf_qwen3](#renderhf_qwen3) | 116,579 | 0% |  | 99 | 0% |  | 3,224 | 0% | -0.74% |
| [Load/html_autoescape](#loadhtml_autoescape) | 188,768 | 0% |  | 42 | 0% |  | 12,680 | 0% | -48.62% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,194,780 | 0% |  | 674 | 0% |  | 45,968 | 0% | +13.83% |
| [Load/inheritance](#loadinheritance) | 21,175 | 0% | -81.44% | 6 | 0% | -94.39% | 1,352 | 0% | -52.92% |
| [Render/inheritance](#renderinheritance) | 134,056 | 0% | -82.07% | 35 | 0% | -94.37% | 7,960 | 0% | +32.31% |
| [Load/int_arith](#loadint_arith) | 64,478 | 0% |  | 19 | 0% |  | 3,880 | 0% | +0.62% |
| [Render/int_arith](#renderint_arith) | 827,268 | 0% |  | 12 | 0% |  | 3,272 | 0% | -0.49% |
| [Module/keep](#modulekeep) | 6,880 | 0% |  | 6 | 0% |  |  |  |  |
| [Module/keep_written](#modulekeep_written) | 29,019 | 0% |  | 10 | 0% |  |  |  |  |
| [Load/large_static](#loadlarge_static) | 146,911 | 0% | -83.42% | 13 | 0% | -97.05% | 43,128 | 0% | -19.21% |
| [Render/large_static](#renderlarge_static) | 20,322 | 0% | -72.95% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [RenderSink/large_static](#rendersinklarge_static) | 12,310 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/large_static](#rendertolarge_static) | 19,885 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/macros](#loadmacros) | 54,266 | 0% | -76.19% | 25 | 0% | -86.56% | 4,424 | 0% | -28.83% |
| [Render/macros](#rendermacros) | 640,833 | 0% | -75.00% | 308 | 0% | -88.71% | 20,576 | 0% | +39.78% |
| [Load/many_tags](#loadmany_tags) | 8,746,541 | 0% | -80.20% | 950 | 0% | -97.23% | 438,120 | 0% | -58.48% |
| [Render/many_tags](#rendermany_tags) | 730,181 | 0% | -64.95% | 2 | 0% | -90.48% | 5,648 | 0% | -10.52% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 44,443 | 0% | -81.29% | 18 | 0% | -90.22% | 3,368 | 0% | -44.46% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,068,134 | 0% | -58.98% | 33 | 0% | -99.19% | 381,536 | 0% | +9.95% |
| [RenderSink/mitsuhiko_table](#rendersinkmitsuhiko_table) | 4,995,275 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table](#rendertomitsuhiko_table) | 5,067,640 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 52,183 | 0% |  | 34 | 0% |  | 5,696 | 0% | -31.07% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,414,086 | 0% |  | 33 | 0% |  | 1,414,928 | 0% | +2.46% |
| [RenderSink/mitsuhiko_table_wide](#rendersinkmitsuhiko_table_wide) | 5,237,306 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table_wide](#rendertomitsuhiko_table_wide) | 5,413,553 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/plain_text](#loadplain_text) | 2,919 | 0% | -69.83% | 3 | 0% | -83.33% | 680 | 0% | -39.29% |
| [Render/plain_text](#renderplain_text) | 951 | 0% | -86.00% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/plain_text](#rendersinkplain_text) | 651 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/plain_text](#rendertoplain_text) | 752 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/prompt_budget](#loadprompt_budget) | 33,775 | 0% |  | 17 | 0% |  | 2,232 | 0% | +0.36% |
| [Render/prompt_budget](#renderprompt_budget) | 1,319,383 | 0% |  | 76 | 0% |  | 36,416 | 0% | 0% |
| [Load/strings](#loadstrings) | 68,545 | 0% | -76.51% | 29 | 0% | -87.05% | 4,352 | 0% | -51.86% |
| [Render/strings](#renderstrings) | 580,440 | 0% | -72.33% | 344 | 0% | -83.73% | 9,088 | 0% | -49.31% |
| [Load/substitute](#loadsubstitute) | 6,639 | 0% | -77.76% | 5 | 0% | -86.49% | 840 | 0% | -44.74% |
| [Render/substitute](#rendersubstitute) | 1,631 | 0% | -81.49% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/substitute](#rendersinksubstitute) | 1,335 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/substitute](#rendertosubstitute) | 1,436 | 0% |  | 0 |  |  | 0 |  |  |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 761 | +0.26% | 772 | -0.26% | 0 |  | 7,808 | -0.96% |
| Render/chat_llama | 1,153 | -0.60% | 519 | -1.14% | 0 |  | 4,058 | -2.36% |
| Load/chat_mistral | 1,097 | +0.18% | 1,092 | -0.09% | 0 |  | 8,942 | -3.66% |
| Render/chat_mistral | 1,976 | -0.30% | 614 | -1.60% | 0 |  | 9,789 | -4.73% |
| Load/chat_qwen | 546 | +0.55% | 646 | +0.31% | 0 |  | 4,825 | -0.86% |
| Render/chat_qwen | 1,038 | +0.68% | 419 | +1.21% | 0 |  | 3,540 | -1.03% |
| Load/config_file | 207 | -0.96% | 191 | 0% | 0 |  | 2,732 | -1.69% |
| Render/config_file | 5,986 | -0.03% | 1,479 | +0.96% | 0 |  | 40,381 | -6.25% |
| Load/dict_ops | 31 | 0% | 19 | 0% | 0 |  | 1,334 | -3.19% |
| Render/dict_ops | 671 | +1.21% | 420 | +0.24% | 0 |  | 813 | +1.75% |
| Load/expressions | 27 | 0% | 21 | 0% | 0 |  | 1,287 | +0.94% |
| Render/expressions | 9 | 0% | 4 | 0% | 0 |  | 267 | -20.30% |
| Load/filters | 83 | 0% | 40 | 0% | 0 |  | 1,451 | -4.54% |
| Render/filters | 6 | 0% | 0 |  | 0 |  | 730 | -3.31% |
| Load/for_filter_if | 21 | 0% | 11 | 0% | 0 |  | 1,051 | -3.58% |
| Render/for_filter_if | 1,700 | +0.12% | 88 | +3.53% | 0 |  | 111 | -28.85% |
| Load/for_loop_vars | 35 | +6.06% | 18 | +12.50% | 0 |  | 1,095 | -0.36% |
| Render/for_loop_vars | 12 | 0% | 2 | 0% | 0 |  | 89 | -43.31% |
| Load/for_range | 1 | 0% | 4 | 0% | 0 |  | 714 | -1.24% |
| Render/for_range | 1 | 0% | 0 |  | 0 |  | 126 | 0% |
| Load/hf_deepseek_v31 | 1,022 | +0.79% | 996 | 0% | 0 |  | 6,954 | -2.93% |
| Render/hf_deepseek_v31 | 356 | 0% | 77 | 0% | 0 |  | 1,426 | -5.69% |
| Load/hf_gpt_oss | 4,633 | +0.19% | 3,372 | -0.06% | 0 |  | 23,520 | -4.01% |
| Render/hf_gpt_oss | 1,251 | -0.08% | 390 | -0.51% | 0 |  | 4,634 | -0.34% |
| Load/hf_mistral_small | 1,824 | +0.16% | 1,628 | -0.06% | 0 |  | 11,909 | -1.35% |
| Render/hf_mistral_small | 938 | -0.21% | 283 | -2.75% | 0 |  | 3,586 | -5.38% |
| Load/hf_qwen3 | 1,539 | +0.26% | 1,335 | -0.07% | 0 |  | 9,372 | -2.62% |
| Render/hf_qwen3 | 680 | -0.58% | 222 | -1.77% | 0 |  | 2,614 | -1.40% |
| Load/html_autoescape | 500 | -0.20% | 534 | 0% | 0 |  | 3,992 | -3.29% |
| Render/html_autoescape | 3,948 | +0.33% | 1,300 | +0.93% | 0 |  | 37,273 | +2.94% |
| Load/inheritance | 2 | 0% | 3 | 0% | 0 |  | 691 | -1.00% |
| Render/inheritance | 596 | 0% | 108 | -0.92% | 0 |  | 890 | +1.83% |
| Load/int_arith | 74 | 0% | 51 | 0% | 0 |  | 1,962 | -3.11% |
| Render/int_arith | 9 | 0% | 3 | 0% | 0 |  | 713 | -75.86% |
| Module/keep | 2 | 0% | 2 | 0% | 0 |  | 424 | +3.41% |
| Module/keep_written | 1,135 | +0.27% | 563 | 0% | 0 |  | 736 | -0.14% |
| Load/large_static | 1,913 | +0.05% | 907 | -0.11% | 0 |  | 236 | +8.26% |
| Render/large_static | 702 | +0.14% | 569 | +0.18% | 0 |  | 20 | +566.67% |
| RenderSink/large_static | 1 | 0% | 0 |  | 0 |  | 2 | -33.33% |
| RenderTo/large_static | 678 | 0% | 572 | +0.18% | 0 |  | 6 | +100.00% |
| Load/macros | 71 | -1.39% | 58 | 0% | 0 |  | 1,694 | -4.24% |
| Render/macros | 748 | +0.27% | 244 | 0% | 0 |  | 572 | -60.42% |
| Load/many_tags | 27,769 | +0.01% | 22,612 | -0.02% | 0 |  | 235,291 | +0.01% |
| Render/many_tags | 6,090 | +0.02% | 783 | 0% | 0 |  | 126 | +21.15% |
| Load/mitsuhiko_table | 39 | -2.50% | 43 | -8.51% | 0 |  | 881 | -2.33% |
| Render/mitsuhiko_table | 10,019 | +0.07% | 6,302 | +0.05% | 0 |  | 310 | -3.43% |
| RenderSink/mitsuhiko_table | 9,963 | +0.06% | 1,211 | +0.33% | 0 |  | 215 | -15.02% |
| RenderTo/mitsuhiko_table | 10,034 | +0.08% | 6,313 | +0.03% | 0 |  | 284 | -5.65% |
| Load/mitsuhiko_table_wide | 96 | -2.04% | 93 | +1.09% | 0 |  | 951 | -6.58% |
| Render/mitsuhiko_table_wide | 10,215 | 0% | 22,693 | +0.02% | 0 |  | 299 | -7.72% |
| RenderSink/mitsuhiko_table_wide | 10,030 | +0.06% | 1,309 | +0.69% | 0 |  | 250 | +0.40% |
| RenderTo/mitsuhiko_table_wide | 10,214 | +0.01% | 22,716 | -0.02% | 0 |  | 281 | -7.57% |
| Load/plain_text | 0 |  | 1 | 0% | 0 |  | 2 | -88.89% |
| Render/plain_text | 1 | 0% | 0 |  | 0 |  | 8 | +14.29% |
| RenderSink/plain_text | 1 | 0% | 0 |  | 0 |  | 5 | 0% |
| RenderTo/plain_text | 1 | 0% | 0 |  | 0 |  | 8 | +14.29% |
| Load/prompt_budget | 67 | -1.47% | 47 | 0% | 0 |  | 1,355 | -1.09% |
| Render/prompt_budget | 4,811 | +0.04% | 548 | +0.92% | 0 |  | 871 | -0.80% |
| Load/strings | 132 | +0.76% | 106 | 0% | 0 |  | 2,699 | -2.53% |
| Render/strings | 399 | -0.50% | 465 | +0.65% | 0 |  | 1,529 | -43.93% |
| Load/substitute | 1 | 0% | 2 | 0% | 0 |  | 121 | +4.31% |
| Render/substitute | 1 | 0% | 0 |  | 0 |  | 5 | +66.67% |
| RenderSink/substitute | 0 |  | 0 |  | 0 |  | 9 | +200.00% |
| RenderTo/substitute | 1 | 0% | 0 |  | 0 |  | 4 | +33.33% |

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
