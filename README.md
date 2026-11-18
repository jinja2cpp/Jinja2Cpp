# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (248 so far, latest [0102d88](https://github.com/jinja2cpp/Jinja2Cpp/commit/0102d88ecddbd38278290d582190fafa80dc1d09) on 2026-11-18). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 334,981 | +0.01% |  | 54 | 0% |  | 18,520 | 0% | -52.61% |
| [Render/chat_llama](#renderchat_llama) | 291,295 | 0% |  | 224 | 0% |  | 11,592 | 0% | -19.28% |
| [Load/chat_mistral](#loadchat_mistral) | 384,158 | 0% |  | 51 | 0% |  | 20,976 | 0% | -53.02% |
| [Render/chat_mistral](#renderchat_mistral) | 491,191 | -0.01% |  | 392 | 0% |  | 9,816 | 0% | -25.68% |
| [Load/chat_qwen](#loadchat_qwen) | 261,049 | 0% |  | 43 | 0% |  | 14,472 | 0% | -47.04% |
| [Render/chat_qwen](#renderchat_qwen) | 271,661 | 0% |  | 211 | 0% |  | 8,280 | 0% | -2.63% |
| [Load/config_file](#loadconfig_file) | 94,069 | -0.01% |  | 24 | 0% |  | 5,520 | 0% | -47.73% |
| [Render/config_file](#renderconfig_file) | 1,272,205 | 0% |  | 908 | 0% |  | 25,720 | 0% | +8.03% |
| [Load/dict_ops](#loaddict_ops) | 31,818 | 0% | -78.47% | 8 | 0% | -93.89% | 2,216 | 0% | -47.93% |
| [Render/dict_ops](#renderdict_ops) | 290,349 | 0% | -57.97% | 112 | 0% | -75.11% | 24,408 | 0% | -29.65% |
| [Load/expressions](#loadexpressions) | 43,695 | 0% | -78.80% | 15 | 0% | -89.73% | 2,752 | 0% | -55.15% |
| [Render/expressions](#renderexpressions) | 414,361 | 0% | -54.27% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 67,290 | -0.02% | -75.48% | 26 | 0% | -88.60% | 3,856 | 0% | -66.22% |
| [Render/filters](#renderfilters) | 26,455 | -0.13% | -70.16% | 23 | 0% | -68.92% | 752 | 0% | -84.69% |
| [Load/for_filter_if](#loadfor_filter_if) | 28,550 | 0% | -78.91% | 6 | 0% | -95.12% | 1,928 | 0% | -44.08% |
| [Render/for_filter_if](#renderfor_filter_if) | 267,127 | 0% | -69.14% | 4 | 0% | -98.23% | 16,560 | 0% | +354.95% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 38,356 | 0% | -78.77% | 9 | 0% | -94.27% | 2,392 | 0% | -47.08% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 170,756 | 0% | -84.08% | 2 | 0% | -99.62% | 1,568 | 0% | -43.35% |
| [Load/for_range](#loadfor_range) | 12,490 | 0% | -77.97% | 6 | 0% | -89.29% | 1,128 | 0% | -43.60% |
| [Render/for_range](#renderfor_range) | 37,143 | 0% | -59.72% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/hf_deepseek_v31](#loadhf_deepseek_v31) | 383,271 | 0% |  | 54 | 0% |  | 22,032 | 0% | +0.99% |
| [Render/hf_deepseek_v31](#renderhf_deepseek_v31) | 90,115 | -0.04% |  | 42 | 0% |  | 2,456 | 0% | -0.97% |
| [Load/hf_gpt_oss](#loadhf_gpt_oss) | 1,522,431 | 0% |  | 282 | 0% |  | 75,000 | 0% | +1.05% |
| [Render/hf_gpt_oss](#renderhf_gpt_oss) | 169,208 | -0.01% |  | 153 | 0% |  | 5,760 | 0% | +16.69% |
| [Load/hf_mistral_small](#loadhf_mistral_small) | 589,805 | 0% |  | 87 | 0% |  | 33,136 | 0% | +0.61% |
| [Render/hf_mistral_small](#renderhf_mistral_small) | 139,372 | -0.02% |  | 97 | 0% |  | 6,096 | 0% | -0.65% |
| [Load/hf_qwen3](#loadhf_qwen3) | 476,021 | 0% |  | 81 | 0% |  | 25,632 | 0% | +0.60% |
| [Render/hf_qwen3](#renderhf_qwen3) | 116,792 | -0.07% |  | 99 | 0% |  | 3,224 | 0% | -0.74% |
| [Load/html_autoescape](#loadhtml_autoescape) | 188,681 | -0.01% |  | 42 | 0% |  | 12,728 | 0% | -48.43% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,196,689 | -0.02% |  | 674 | 0% |  | 46,080 | 0% | +14.10% |
| [Load/inheritance](#loadinheritance) | 21,304 | 0% | -81.33% | 6 | 0% | -94.39% | 1,368 | 0% | -52.37% |
| [Render/inheritance](#renderinheritance) | 135,123 | 0% | -81.93% | 35 | 0% | -94.37% | 8,072 | 0% | +34.18% |
| [Load/int_arith](#loadint_arith) | 64,597 | 0% |  | 19 | 0% |  | 3,896 | 0% | +1.04% |
| [Render/int_arith](#renderint_arith) | 825,260 | -0.05% |  | 12 | 0% |  | 3,272 | 0% | -0.49% |
| [Module/keep](#modulekeep) | 6,940 | 0% |  | 6 | 0% |  |  |  |  |
| [Module/keep_written](#modulekeep_written) | 29,005 | 0% |  | 10 | 0% |  |  |  |  |
| [Load/large_static](#loadlarge_static) | 146,667 | 0% | -83.45% | 13 | 0% | -97.05% | 43,128 | 0% | -19.21% |
| [Render/large_static](#renderlarge_static) | 20,254 | 0% | -73.04% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [RenderSink/large_static](#rendersinklarge_static) | 12,621 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/large_static](#rendertolarge_static) | 19,895 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/macros](#loadmacros) | 53,998 | 0% | -76.31% | 25 | 0% | -86.56% | 4,472 | 0% | -28.06% |
| [Render/macros](#rendermacros) | 644,378 | 0% | -74.86% | 308 | 0% | -88.71% | 20,576 | 0% | +39.78% |
| [Load/many_tags](#loadmany_tags) | 8,768,226 | -0.02% | -80.15% | 950 | 0% | -97.23% | 438,120 | 0% | -58.48% |
| [Render/many_tags](#rendermany_tags) | 732,445 | 0% | -64.84% | 2 | 0% | -90.48% | 5,648 | 0% | -10.52% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 44,601 | 0% | -81.23% | 18 | 0% | -90.22% | 3,416 | 0% | -43.67% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,075,414 | 0% | -58.92% | 33 | 0% | -99.19% | 381,536 | 0% | +9.95% |
| [RenderSink/mitsuhiko_table](#rendersinkmitsuhiko_table) | 5,005,247 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table](#rendertomitsuhiko_table) | 5,074,944 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 52,340 | 0% |  | 34 | 0% |  | 5,744 | 0% | -30.49% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,421,243 | 0% |  | 33 | 0% |  | 1,414,928 | 0% | +2.46% |
| [RenderSink/mitsuhiko_table_wide](#rendersinkmitsuhiko_table_wide) | 5,247,186 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table_wide](#rendertomitsuhiko_table_wide) | 5,420,676 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Nested/new_template](#nestednew_template) | 450,370 | -0.02% |  | 323 | 0% |  |  |  |  |
| [Load/plain_text](#loadplain_text) | 2,941 | 0% | -69.60% | 3 | 0% | -83.33% | 680 | 0% | -39.29% |
| [Render/plain_text](#renderplain_text) | 962 | 0% | -85.84% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/plain_text](#rendersinkplain_text) | 667 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/plain_text](#rendertoplain_text) | 763 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/prompt_budget](#loadprompt_budget) | 34,163 | -0.05% |  | 17 | 0% |  | 2,248 | 0% | +1.08% |
| [Render/prompt_budget](#renderprompt_budget) | 1,321,760 | +0.01% |  | 76 | 0% |  | 36,416 | 0% | 0% |
| [Nested/shared_globals](#nestedshared_globals) | 158,582 | -0.07% |  | 160 | 0% |  |  |  |  |
| [Load/strings](#loadstrings) | 68,812 | -0.02% | -76.42% | 29 | 0% | -87.05% | 4,384 | 0% | -51.50% |
| [Render/strings](#renderstrings) | 581,167 | -0.17% | -72.29% | 344 | 0% | -83.73% | 9,088 | 0% | -49.31% |
| [Load/substitute](#loadsubstitute) | 6,651 | 0% | -77.72% | 5 | 0% | -86.49% | 840 | 0% | -44.74% |
| [Render/substitute](#rendersubstitute) | 1,643 | 0% | -81.36% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/substitute](#rendersinksubstitute) | 1,352 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/substitute](#rendertosubstitute) | 1,448 | 0% |  | 0 |  |  | 0 |  |  |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 748 | +0.67% | 772 | +0.52% | 0 |  | 8,153 | -0.68% |
| Render/chat_llama | 1,209 | +1.68% | 532 | +0.95% | 0 |  | 4,251 | +2.88% |
| Load/chat_mistral | 1,100 | +1.01% | 1,110 | +0.73% | 0 |  | 9,459 | -0.79% |
| Render/chat_mistral | 2,080 | +0.68% | 654 | +0.15% | 0 |  | 11,042 | +3.26% |
| Load/chat_qwen | 551 | +2.04% | 651 | +1.72% | 0 |  | 4,910 | -2.66% |
| Render/chat_qwen | 1,095 | +3.20% | 436 | +2.35% | 0 |  | 3,616 | +0.86% |
| Load/config_file | 205 | +0.49% | 194 | 0% | 0 |  | 2,919 | +3.25% |
| Render/config_file | 7,250 | -1.80% | 1,862 | -3.32% | 0 |  | 42,627 | -1.09% |
| Load/dict_ops | 32 | -3.03% | 24 | +9.09% | 0 |  | 1,446 | +1.12% |
| Render/dict_ops | 665 | +0.45% | 416 | 0% | 0 |  | 835 | +1.33% |
| Load/expressions | 22 | +4.76% | 19 | +18.75% | 0 |  | 1,430 | +6.00% |
| Render/expressions | 3 | -50.00% | 1 | -50.00% | 0 |  | 316 | +13.26% |
| Load/filters | 82 | +9.33% | 35 | 0% | 0 |  | 1,820 | +8.59% |
| Render/filters | 10 | -9.09% | 1 | -50.00% | 0 |  | 764 | +2.41% |
| Load/for_filter_if | 24 | +4.35% | 18 | +5.88% | 0 |  | 1,148 | +4.08% |
| Render/for_filter_if | 1,714 | -0.06% | 95 | -1.04% | 0 |  | 151 | +15.27% |
| Load/for_loop_vars | 30 | -14.29% | 24 | -4.00% | 0 |  | 1,228 | +9.84% |
| Render/for_loop_vars | 17 | +30.77% | 6 | +500.00% | 0 |  | 105 | +5.00% |
| Load/for_range | 1 | 0% | 3 | 0% | 0 |  | 796 | +2.71% |
| Render/for_range | 2 | 0% | 0 |  | 0 |  | 141 | +34.29% |
| Load/hf_deepseek_v31 | 972 | +0.93% | 978 | -0.71% | 0 |  | 7,377 | -1.15% |
| Render/hf_deepseek_v31 | 379 | -1.56% | 92 | -3.16% | 0 |  | 1,528 | -2.49% |
| Load/hf_gpt_oss | 4,622 | +0.30% | 3,403 | +0.06% | 0 |  | 25,023 | +2.88% |
| Render/hf_gpt_oss | 1,340 | +0.98% | 416 | 0% | 0 |  | 4,972 | +0.46% |
| Load/hf_mistral_small | 1,851 | -0.32% | 1,639 | +0.43% | 0 |  | 12,324 | +1.28% |
| Render/hf_mistral_small | 981 | +0.62% | 297 | -0.34% | 0 |  | 3,788 | -3.27% |
| Load/hf_qwen3 | 1,590 | +0.32% | 1,350 | 0% | 0 |  | 9,618 | -0.22% |
| Render/hf_qwen3 | 708 | +0.57% | 215 | 0% | 0 |  | 2,739 | -0.40% |
| Load/html_autoescape | 532 | +0.95% | 539 | +0.56% | 0 |  | 4,466 | +13.01% |
| Render/html_autoescape | 3,995 | -4.04% | 1,426 | -8.59% | 0 |  | 36,595 | +1.23% |
| Load/inheritance | 2 | -60.00% | 3 | -25.00% | 0 |  | 710 | -4.70% |
| Render/inheritance | 600 | +1.01% | 115 | +0.88% | 0 |  | 1,001 | -1.18% |
| Load/int_arith | 88 | +14.29% | 60 | +7.14% | 0 |  | 2,182 | +5.26% |
| Render/int_arith | 7 | +75.00% | 4 | +300.00% | 0 |  | 745 | -2.74% |
| Module/keep | 2 | 0% | 2 | 0% | 0 |  | 452 | +11.88% |
| Module/keep_written | 1,146 | +0.09% | 564 | +0.18% | 0 |  | 733 | +1.38% |
| Load/large_static | 1,915 | +0.10% | 909 | 0% | 0 |  | 333 | +19.35% |
| Render/large_static | 700 | 0% | 581 | 0% | 0 |  | 3 | -25.00% |
| RenderSink/large_static | 1 | 0% | 0 |  | 0 |  | 2 | 0% |
| RenderTo/large_static | 680 | 0% | 569 | 0% | 0 |  | 3 | -25.00% |
| Load/macros | 67 | 0% | 66 | +3.12% | 0 |  | 1,805 | +4.21% |
| Render/macros | 762 | -0.78% | 251 | -0.40% | 0 |  | 2,983 | +417.88% |
| Load/many_tags | 27,781 | +0.03% | 22,586 | -0.02% | 0 |  | 255,378 | -0.43% |
| Render/many_tags | 6,104 | -0.33% | 783 | +0.38% | 0 |  | 123 | +51.85% |
| Load/mitsuhiko_table | 29 | 0% | 43 | +13.16% | 0 |  | 953 | -3.15% |
| Render/mitsuhiko_table | 10,025 | +0.11% | 6,260 | +0.03% | 0 |  | 334 | +6.71% |
| RenderSink/mitsuhiko_table | 9,974 | 0% | 1,207 | +0.08% | 0 |  | 300 | +16.73% |
| RenderTo/mitsuhiko_table | 10,023 | +0.05% | 6,256 | +0.02% | 0 |  | 315 | +6.78% |
| Load/mitsuhiko_table_wide | 72 | -12.20% | 92 | -5.15% | 0 |  | 886 | -12.97% |
| Render/mitsuhiko_table_wide | 10,241 | +0.03% | 22,668 | -0.01% | 0 |  | 337 | +5.31% |
| RenderSink/mitsuhiko_table_wide | 10,056 | +0.03% | 1,297 | +0.08% | 0 |  | 266 | +1.14% |
| RenderTo/mitsuhiko_table_wide | 10,238 | 0% | 22,688 | +0.01% | 0 |  | 310 | +7.27% |
| Nested/new_template | 1,975 | +3.24% | 1,424 | +0.71% | 0 |  | 24,874 | +0.25% |
| Load/plain_text | 0 |  | 1 | 0% | 0 |  | 3 | -66.67% |
| Render/plain_text | 1 | 0% | 0 |  | 0 |  | 6 | -14.29% |
| RenderSink/plain_text | 1 | 0% | 0 |  | 0 |  | 3 | -25.00% |
| RenderTo/plain_text | 2 | +100.00% | 0 |  | 0 |  | 5 | -28.57% |
| Load/prompt_budget | 76 | -1.30% | 63 | +1.61% | 0 |  | 1,456 | +1.75% |
| Render/prompt_budget | 4,817 | +0.10% | 541 | +0.37% | 0 |  | 1,105 | +21.03% |
| Nested/shared_globals | 244 | +27.75% | 108 | -17.56% | 0 |  | 9,260 | +3.23% |
| Load/strings | 130 | +2.36% | 122 | +6.09% | 0 |  | 2,918 | +1.85% |
| Render/strings | 424 | +3.16% | 479 | +0.21% | 0 |  | 2,521 | +60.06% |
| Load/substitute | 1 | 0% | 2 | 0% | 0 |  | 152 | +21.60% |
| Render/substitute | 1 | 0% | 0 |  | 0 |  | 3 | -25.00% |
| RenderSink/substitute | 1 | 0% | 0 |  | 0 |  | 1 | -50.00% |
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
