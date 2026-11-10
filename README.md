# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (195 so far, latest [8b7b4f4](https://github.com/jinja2cpp/Jinja2Cpp/commit/8b7b4f4d6eb7aa99fb6899e3f7c06bd1f5fc2ece) on 2026-11-10). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 345,346 | 0% |  | 55 | 0% |  | 18,480 | 0% | -52.71% |
| [Render/chat_llama](#renderchat_llama) | 296,198 | 0% |  | 225 | 0% |  | 11,616 | 0% | -19.11% |
| [Load/chat_mistral](#loadchat_mistral) | 394,437 | 0% |  | 52 | 0% |  | 20,888 | 0% | -53.22% |
| [Render/chat_mistral](#renderchat_mistral) | 500,271 | 0% |  | 402 | 0% |  | 9,984 | 0% | -24.41% |
| [Load/chat_qwen](#loadchat_qwen) | 266,694 | 0% |  | 44 | 0% |  | 14,416 | 0% | -47.25% |
| [Render/chat_qwen](#renderchat_qwen) | 277,400 | 0% |  | 212 | 0% |  | 8,304 | 0% | -2.35% |
| [Load/config_file](#loadconfig_file) | 97,283 | -0.01% |  | 25 | 0% |  | 5,464 | 0% | -48.26% |
| [Render/config_file](#renderconfig_file) | 1,274,260 | -0.01% |  | 908 | 0% |  | 24,440 | 0% | +2.65% |
| [Load/dict_ops](#loaddict_ops) | 33,354 | -0.02% | -77.43% | 9 | 0% | -93.13% | 2,176 | 0% | -48.87% |
| [Render/dict_ops](#renderdict_ops) | 293,077 | +0.02% | -57.58% | 112 | 0% | -75.11% | 24,408 | 0% | -29.65% |
| [Load/expressions](#loadexpressions) | 45,046 | -0.01% | -78.15% | 16 | 0% | -89.04% | 2,728 | 0% | -55.54% |
| [Render/expressions](#renderexpressions) | 426,191 | 0% | -52.97% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 69,043 | -0.01% | -74.84% | 27 | 0% | -88.16% | 3,848 | 0% | -66.29% |
| [Render/filters](#renderfilters) | 30,464 | 0% | -65.64% | 26 | 0% | -64.86% | 1,080 | 0% | -78.01% |
| [Load/for_filter_if](#loadfor_filter_if) | 30,047 | -0.02% | -77.81% | 7 | 0% | -94.31% | 1,904 | 0% | -44.78% |
| [Render/for_filter_if](#renderfor_filter_if) | 274,378 | 0% | -68.30% | 4 | 0% | -98.23% | 16,560 | 0% | +354.95% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 40,413 | +0.01% | -77.63% | 10 | 0% | -93.63% | 2,368 | 0% | -47.61% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 180,545 | 0% | -83.17% | 2 | 0% | -99.62% | 1,568 | 0% | -43.35% |
| [Load/for_range](#loadfor_range) | 13,666 | -0.04% | -75.89% | 7 | 0% | -87.50% | 1,088 | 0% | -45.60% |
| [Render/for_range](#renderfor_range) | 38,345 | 0% | -58.42% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/hf_deepseek_v31](#loadhf_deepseek_v31) | 387,525 | +0.04% |  | 55 | 0% |  | 21,976 | 0% | +0.73% |
| [Render/hf_deepseek_v31](#renderhf_deepseek_v31) | 90,899 | -0.01% |  | 43 | 0% |  | 2,480 | 0% | 0% |
| [Load/hf_gpt_oss](#loadhf_gpt_oss) | 1,549,679 | 0% |  | 287 | 0% |  | 74,672 | +0.02% | +0.60% |
| [Render/hf_gpt_oss](#renderhf_gpt_oss) | 171,158 | -0.08% |  | 155 | 0% |  | 5,080 | 0% | +2.92% |
| [Load/hf_mistral_small](#loadhf_mistral_small) | 599,334 | -0.01% |  | 88 | 0% |  | 33,048 | 0% | +0.34% |
| [Render/hf_mistral_small](#renderhf_mistral_small) | 144,919 | +0.11% |  | 105 | 0% |  | 6,136 | +0.26% | 0% |
| [Load/hf_qwen3](#loadhf_qwen3) | 486,327 | -0.02% |  | 82 | 0% |  | 25,560 | 0% | +0.31% |
| [Render/hf_qwen3](#renderhf_qwen3) | 119,934 | -0.01% |  | 100 | 0% |  | 3,248 | 0% | 0% |
| [Load/html_autoescape](#loadhtml_autoescape) | 194,026 | 0% |  | 43 | 0% |  | 12,640 | 0% | -48.78% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,206,825 | -0.01% |  | 674 | 0% |  | 45,408 | 0% | +12.44% |
| [Load/inheritance](#loadinheritance) | 23,096 | -0.03% | -79.76% | 7 | 0% | -93.46% | 1,344 | 0% | -53.20% |
| [Render/inheritance](#renderinheritance) | 137,421 | -0.05% | -81.62% | 35 | 0% | -94.37% | 7,384 | 0% | +22.74% |
| [Load/int_arith](#loadint_arith) | 66,009 | -0.01% |  | 20 | 0% |  | 3,872 | 0% | +0.41% |
| [Render/int_arith](#renderint_arith) | 825,738 | 0% |  | 12 | 0% |  | 3,272 | 0% | -0.49% |
| [Module/keep](#modulekeep) | 7,070 | 0% |  | 7 | 0% |  |  |  |  |
| [Module/keep_written](#modulekeep_written) | 36,529 | 0% |  | 11 | 0% |  |  |  |  |
| [Load/large_static](#loadlarge_static) | 154,778 | 0% | -82.53% | 14 | 0% | -96.82% | 43,120 | 0% | -19.23% |
| [Render/large_static](#renderlarge_static) | 20,663 | -0.23% | -72.50% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [RenderSink/large_static](#rendersinklarge_static) | 12,701 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/large_static](#rendertolarge_static) | 20,274 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/macros](#loadmacros) | 56,407 | -0.01% | -75.25% | 27 | 0% | -85.48% | 4,392 | 0% | -29.34% |
| [Render/macros](#rendermacros) | 656,293 | 0% | -74.39% | 308 | 0% | -88.71% | 20,560 | 0% | +39.67% |
| [Load/many_tags](#loadmany_tags) | 9,019,220 | 0% | -79.58% | 951 | 0% | -97.22% | 438,128 | -0.01% | -58.48% |
| [Render/many_tags](#rendermany_tags) | 727,197 | 0% | -65.09% | 2 | 0% | -90.48% | 5,648 | 0% | -10.52% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 46,621 | +0.02% | -80.38% | 19 | 0% | -89.67% | 3,360 | 0% | -44.59% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,194,633 | 0% | -57.96% | 33 | 0% | -99.19% | 381,536 | 0% | +9.95% |
| [RenderSink/mitsuhiko_table](#rendersinkmitsuhiko_table) | 5,121,828 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table](#rendertomitsuhiko_table) | 5,194,139 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 54,830 | -0.01% |  | 35 | 0% |  | 5,688 | 0% | -31.17% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,540,801 | 0% |  | 33 | 0% |  | 1,414,928 | 0% | +2.46% |
| [RenderSink/mitsuhiko_table_wide](#rendersinkmitsuhiko_table_wide) | 5,364,092 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table_wide](#rendertomitsuhiko_table_wide) | 5,540,353 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/plain_text](#loadplain_text) | 3,733 | -0.16% | -61.42% | 4 | 0% | -77.78% | 672 | 0% | -40.00% |
| [Render/plain_text](#renderplain_text) | 1,021 | 0% | -84.97% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/plain_text](#rendersinkplain_text) | 723 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/plain_text](#rendertoplain_text) | 822 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/prompt_budget](#loadprompt_budget) | 35,155 | -0.02% |  | 18 | 0% |  | 2,224 | 0% | 0% |
| [Render/prompt_budget](#renderprompt_budget) | 1,315,564 | 0% |  | 76 | 0% |  | 36,416 | 0% | 0% |
| [Load/strings](#loadstrings) | 70,455 | -0.01% | -75.86% | 30 | 0% | -86.61% | 4,344 | 0% | -51.95% |
| [Render/strings](#renderstrings) | 580,030 | 0% | -72.35% | 344 | 0% | -83.73% | 9,088 | 0% | -49.31% |
| [Load/substitute](#loadsubstitute) | 7,768 | -0.08% | -73.98% | 6 | 0% | -83.78% | 832 | 0% | -45.26% |
| [Render/substitute](#rendersubstitute) | 2,254 | 0% | -74.43% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/substitute](#rendersinksubstitute) | 1,960 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/substitute](#rendertosubstitute) | 2,059 | 0% |  | 0 |  |  | 0 |  |  |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 723 | +0.42% | 743 | +1.09% | 0 |  | 7,875 | +6.58% |
| Render/chat_llama | 1,199 | +1.44% | 518 | +2.37% | 0 |  | 3,951 | +2.30% |
| Load/chat_mistral | 1,030 | -1.34% | 1,030 | -0.48% | 0 |  | 8,922 | +7.35% |
| Render/chat_mistral | 1,906 | -1.90% | 580 | +1.40% | 0 |  | 9,809 | -1.53% |
| Load/chat_qwen | 525 | +0.96% | 585 | +0.52% | 0 |  | 4,879 | +13.65% |
| Render/chat_qwen | 1,090 | +0.74% | 395 | +1.54% | 0 |  | 3,429 | -0.58% |
| Load/config_file | 221 | -2.21% | 181 | -4.23% | 0 |  | 2,833 | +7.35% |
| Render/config_file | 6,235 | -1.27% | 1,547 | +3.27% | 0 |  | 43,282 | +3.11% |
| Load/dict_ops | 30 | 0% | 20 | +5.26% | 0 |  | 1,351 | +2.66% |
| Render/dict_ops | 675 | 0% | 428 | +0.47% | 0 |  | 891 | +1.48% |
| Load/expressions | 20 | 0% | 17 | +41.67% | 0 |  | 1,370 | +8.30% |
| Render/expressions | 16 | -5.88% | 5 | -16.67% | 0 |  | 310 | +14.81% |
| Load/filters | 77 | -8.33% | 50 | +11.11% | 0 |  | 1,970 | +38.34% |
| Render/filters | 10 | -41.18% | 5 | -37.50% | 0 |  | 773 | -2.28% |
| Load/for_filter_if | 32 | 0% | 17 | -22.73% | 0 |  | 1,128 | +7.84% |
| Render/for_filter_if | 1,730 | -0.29% | 132 | -2.22% | 0 |  | 174 | +31.82% |
| Load/for_loop_vars | 44 | +41.94% | 23 | +27.78% | 0 |  | 1,176 | +10.42% |
| Render/for_loop_vars | 10 | +66.67% | 4 | +300.00% | 0 |  | 141 | +6.82% |
| Load/for_range | 4 | +33.33% | 4 | 0% | 0 |  | 769 | +1.72% |
| Render/for_range | 8 | +700.00% | 0 |  | 0 |  | 143 | +45.92% |
| Load/hf_deepseek_v31 | 997 | -0.70% | 973 | -0.31% | 0 |  | 6,103 | +1.23% |
| Render/hf_deepseek_v31 | 353 | -2.49% | 74 | 0% | 0 |  | 1,353 | -1.67% |
| Load/hf_gpt_oss | 4,627 | +0.76% | 3,270 | -0.46% | 0 |  | 23,126 | +10.92% |
| Render/hf_gpt_oss | 1,190 | -1.98% | 374 | +2.47% | 0 |  | 4,403 | -3.08% |
| Load/hf_mistral_small | 1,792 | -0.11% | 1,595 | +0.44% | 0 |  | 11,590 | +8.75% |
| Render/hf_mistral_small | 994 | +1.33% | 316 | 0% | 0 |  | 3,763 | +0.70% |
| Load/hf_qwen3 | 1,453 | -0.07% | 1,262 | -1.48% | 0 |  | 9,035 | +7.66% |
| Render/hf_qwen3 | 670 | +0.30% | 215 | +3.86% | 0 |  | 2,533 | -0.31% |
| Load/html_autoescape | 476 | -0.42% | 507 | +0.80% | 0 |  | 4,554 | +27.78% |
| Render/html_autoescape | 4,780 | -6.02% | 1,473 | -8.34% | 0 |  | 35,682 | +1.32% |
| Load/inheritance | 4 | 0% | 3 | 0% | 0 |  | 700 | +0.43% |
| Render/inheritance | 583 | -0.68% | 90 | -5.26% | 0 |  | 968 | +2.87% |
| Load/int_arith | 66 | -7.04% | 38 | -2.56% | 0 |  | 2,190 | +11.22% |
| Render/int_arith | 17 | -22.73% | 5 | -37.50% | 0 |  | 712 | +0.56% |
| Module/keep | 2 | 0% | 2 | 0% | 0 |  | 413 | +2.23% |
| Module/keep_written | 2,939 | +0.20% | 559 | -0.36% | 0 |  | 718 | -0.42% |
| Load/large_static | 1,932 | -0.10% | 906 | +0.11% | 0 |  | 278 | +0.36% |
| Render/large_static | 712 | +0.14% | 573 | +0.88% | 0 |  | 23 | +228.57% |
| RenderSink/large_static | 0 |  | 0 |  | 0 |  | 4 | -20.00% |
| RenderTo/large_static | 689 | +0.15% | 568 | -0.18% | 0 |  | 12 | +100.00% |
| Load/macros | 85 | +11.84% | 57 | -6.56% | 0 |  | 1,769 | +4.99% |
| Render/macros | 750 | -0.27% | 237 | +10.75% | 0 |  | 1,896 | -15.32% |
| Load/many_tags | 28,765 | +3.00% | 22,510 | +0.59% | 0 |  | 247,593 | +10.41% |
| Render/many_tags | 6,218 | -0.10% | 868 | -0.12% | 0 |  | 148 | +0.68% |
| Load/mitsuhiko_table | 38 | -2.56% | 39 | -4.88% | 0 |  | 886 | -0.34% |
| Render/mitsuhiko_table | 10,035 | -0.16% | 6,274 | +0.06% | 0 |  | 366 | +9.25% |
| RenderSink/mitsuhiko_table | 9,962 | -0.16% | 1,184 | +0.17% | 0 |  | 304 | +14.29% |
| RenderTo/mitsuhiko_table | 10,046 | -0.07% | 6,282 | -0.03% | 0 |  | 337 | +9.42% |
| Load/mitsuhiko_table_wide | 73 | 0% | 85 | -4.49% | 0 |  | 1,088 | +10.91% |
| Render/mitsuhiko_table_wide | 10,223 | -0.25% | 22,686 | +0.08% | 0 |  | 387 | +14.84% |
| RenderSink/mitsuhiko_table_wide | 10,054 | -0.25% | 1,298 | +0.78% | 0 |  | 308 | +5.84% |
| RenderTo/mitsuhiko_table_wide | 10,217 | -0.32% | 22,689 | +0.06% | 0 |  | 358 | +14.38% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 16 | -20.00% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 5 | -28.57% |
| RenderSink/plain_text | 0 |  | 0 |  | 0 |  | 4 | -20.00% |
| RenderTo/plain_text | 0 |  | 0 |  | 0 |  | 4 | -33.33% |
| Load/prompt_budget | 70 | -10.26% | 36 | -7.69% | 0 |  | 1,435 | +3.31% |
| Render/prompt_budget | 4,818 | -0.06% | 521 | -0.95% | 0 |  | 853 | 0% |
| Load/strings | 140 | +3.70% | 91 | -3.19% | 0 |  | 2,816 | +3.80% |
| Render/strings | 424 | -0.24% | 489 | +2.09% | 0 |  | 2,511 | +21.19% |
| Load/substitute | 1 | 0% | 3 | 0% | 0 |  | 150 | -19.35% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 22 | +340.00% |
| RenderSink/substitute | 0 |  | 0 |  | 0 |  | 4 | -20.00% |
| RenderTo/substitute | 0 |  | 0 |  | 0 |  | 13 | +160.00% |

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
