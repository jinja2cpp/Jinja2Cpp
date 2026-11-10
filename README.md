# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (194 so far, latest [eeb221c](https://github.com/jinja2cpp/Jinja2Cpp/commit/eeb221c1d1b863e42e1a799df0ddb44092416e2c) on 2026-11-10). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 345,352 | 0% |  | 55 | 0% |  | 18,480 | 0% | -52.71% |
| [Render/chat_llama](#renderchat_llama) | 296,198 | +0.18% |  | 225 | 0% |  | 11,616 | 0% | -19.11% |
| [Load/chat_mistral](#loadchat_mistral) | 394,443 | 0% |  | 52 | 0% |  | 20,888 | 0% | -53.22% |
| [Render/chat_mistral](#renderchat_mistral) | 500,271 | +0.12% |  | 402 | 0% |  | 9,984 | 0% | -24.41% |
| [Load/chat_qwen](#loadchat_qwen) | 266,700 | 0% |  | 44 | 0% |  | 14,416 | 0% | -47.25% |
| [Render/chat_qwen](#renderchat_qwen) | 277,400 | +0.18% |  | 212 | 0% |  | 8,304 | 0% | -2.35% |
| [Load/config_file](#loadconfig_file) | 97,289 | 0% |  | 25 | 0% |  | 5,464 | 0% | -48.26% |
| [Render/config_file](#renderconfig_file) | 1,274,348 | +0.24% |  | 908 | 0% |  | 24,440 | 0% | +2.65% |
| [Load/dict_ops](#loaddict_ops) | 33,360 | 0% | -77.43% | 9 | 0% | -93.13% | 2,176 | 0% | -48.87% |
| [Render/dict_ops](#renderdict_ops) | 293,021 | +0.02% | -57.59% | 112 | 0% | -75.11% | 24,408 | 0% | -29.65% |
| [Load/expressions](#loadexpressions) | 45,052 | 0% | -78.15% | 16 | 0% | -89.04% | 2,728 | 0% | -55.54% |
| [Render/expressions](#renderexpressions) | 426,191 | 0% | -52.97% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 69,049 | 0% | -74.84% | 27 | 0% | -88.16% | 3,848 | 0% | -66.29% |
| [Render/filters](#renderfilters) | 30,464 | -0.29% | -65.64% | 26 | 0% | -64.86% | 1,080 | 0% | -78.01% |
| [Load/for_filter_if](#loadfor_filter_if) | 30,053 | 0% | -77.80% | 7 | 0% | -94.31% | 1,904 | 0% | -44.78% |
| [Render/for_filter_if](#renderfor_filter_if) | 274,378 | 0% | -68.30% | 4 | 0% | -98.23% | 16,560 | 0% | +354.95% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 40,409 | 0% | -77.63% | 10 | 0% | -93.63% | 2,368 | 0% | -47.61% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 180,545 | 0% | -83.17% | 2 | 0% | -99.62% | 1,568 | 0% | -43.35% |
| [Load/for_range](#loadfor_range) | 13,672 | 0% | -75.88% | 7 | 0% | -87.50% | 1,088 | 0% | -45.60% |
| [Render/for_range](#renderfor_range) | 38,345 | 0% | -58.42% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/hf_deepseek_v31](#loadhf_deepseek_v31) | 387,378 | 0% |  | 55 | 0% |  | 21,976 | 0% | +0.73% |
| [Render/hf_deepseek_v31](#renderhf_deepseek_v31) | 90,907 | +0.09% |  | 43 | 0% |  | 2,480 | 0% | 0% |
| [Load/hf_gpt_oss](#loadhf_gpt_oss) | 1,549,745 | 0% |  | 287 | 0% |  | 74,656 | 0% | +0.58% |
| [Render/hf_gpt_oss](#renderhf_gpt_oss) | 171,300 | +0.31% |  | 155 | 0% |  | 5,080 | -0.31% | +2.92% |
| [Load/hf_mistral_small](#loadhf_mistral_small) | 599,405 | 0% |  | 88 | 0% |  | 33,048 | 0% | +0.34% |
| [Render/hf_mistral_small](#renderhf_mistral_small) | 144,761 | -0.02% |  | 105 | 0% |  | 6,120 | 0% | -0.26% |
| [Load/hf_qwen3](#loadhf_qwen3) | 486,419 | 0% |  | 82 | 0% |  | 25,560 | 0% | +0.31% |
| [Render/hf_qwen3](#renderhf_qwen3) | 119,943 | +0.14% |  | 100 | 0% |  | 3,248 | 0% | 0% |
| [Load/html_autoescape](#loadhtml_autoescape) | 194,032 | 0% |  | 43 | 0% |  | 12,640 | 0% | -48.78% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,206,961 | -0.14% |  | 674 | 0% |  | 45,408 | 0% | +12.44% |
| [Load/inheritance](#loadinheritance) | 23,102 | 0% | -79.76% | 7 | 0% | -93.46% | 1,344 | 0% | -53.20% |
| [Render/inheritance](#renderinheritance) | 137,484 | -0.01% | -81.62% | 35 | 0% | -94.37% | 7,384 | 0% | +22.74% |
| [Load/int_arith](#loadint_arith) | 66,015 | 0% |  | 20 | 0% |  | 3,872 | 0% | +0.41% |
| [Render/int_arith](#renderint_arith) | 825,738 | 0% |  | 12 | 0% |  | 3,272 | 0% | -0.49% |
| [Module/keep](#modulekeep) | 7,070 | +0.61% |  | 7 | 0% |  |  |  |  |
| [Module/keep_written](#modulekeep_written) | 36,529 | +0.30% |  | 11 | 0% |  |  |  |  |
| [Load/large_static](#loadlarge_static) | 154,784 | 0% | -82.53% | 14 | 0% | -96.82% | 43,120 | 0% | -19.23% |
| [Render/large_static](#renderlarge_static) | 20,711 | 0% | -72.44% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [RenderSink/large_static](#rendersinklarge_static) | 12,701 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/large_static](#rendertolarge_static) | 20,274 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/macros](#loadmacros) | 56,413 | 0% | -75.25% | 27 | 0% | -85.48% | 4,392 | 0% | -29.34% |
| [Render/macros](#rendermacros) | 656,309 | 0% | -74.39% | 308 | 0% | -88.71% | 20,560 | 0% | +39.67% |
| [Load/many_tags](#loadmany_tags) | 9,019,652 | 0% | -79.58% | 951 | 0% | -97.22% | 438,160 | 0% | -58.48% |
| [Render/many_tags](#rendermany_tags) | 727,197 | 0% | -65.09% | 2 | 0% | -90.48% | 5,648 | 0% | -10.52% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 46,611 | 0% | -80.38% | 19 | 0% | -89.67% | 3,360 | 0% | -44.59% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,194,649 | 0% | -57.96% | 33 | 0% | -99.19% | 381,536 | 0% | +9.95% |
| [RenderSink/mitsuhiko_table](#rendersinkmitsuhiko_table) | 5,121,828 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table](#rendertomitsuhiko_table) | 5,194,155 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 54,836 | 0% |  | 35 | 0% |  | 5,688 | 0% | -31.17% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,540,801 | 0% |  | 33 | 0% |  | 1,414,928 | 0% | +2.46% |
| [RenderSink/mitsuhiko_table_wide](#rendersinkmitsuhiko_table_wide) | 5,364,092 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table_wide](#rendertomitsuhiko_table_wide) | 5,540,353 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/plain_text](#loadplain_text) | 3,739 | 0% | -61.36% | 4 | 0% | -77.78% | 672 | 0% | -40.00% |
| [Render/plain_text](#renderplain_text) | 1,021 | 0% | -84.97% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/plain_text](#rendersinkplain_text) | 723 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/plain_text](#rendertoplain_text) | 822 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/prompt_budget](#loadprompt_budget) | 35,161 | 0% |  | 18 | 0% |  | 2,224 | 0% | 0% |
| [Render/prompt_budget](#renderprompt_budget) | 1,315,564 | -0.06% |  | 76 | 0% |  | 36,416 | 0% | 0% |
| [Load/strings](#loadstrings) | 70,459 | 0% | -75.86% | 30 | 0% | -86.61% | 4,344 | 0% | -51.95% |
| [Render/strings](#renderstrings) | 580,034 | -0.09% | -72.35% | 344 | 0% | -83.73% | 9,088 | 0% | -49.31% |
| [Load/substitute](#loadsubstitute) | 7,774 | 0% | -73.96% | 6 | 0% | -83.78% | 832 | 0% | -45.26% |
| [Render/substitute](#rendersubstitute) | 2,254 | 0% | -74.43% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/substitute](#rendersinksubstitute) | 1,960 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/substitute](#rendertosubstitute) | 2,059 | 0% |  | 0 |  |  | 0 |  |  |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 720 | -1.77% | 735 | -0.41% | 0 |  | 7,389 | 0% |
| Render/chat_llama | 1,182 | -3.19% | 506 | -2.32% | 0 |  | 3,862 | +0.49% |
| Load/chat_mistral | 1,044 | -0.76% | 1,035 | 0% | 0 |  | 8,311 | +3.67% |
| Render/chat_mistral | 1,943 | -0.31% | 572 | -1.04% | 0 |  | 9,961 | +2.84% |
| Load/chat_qwen | 520 | -4.06% | 582 | -0.85% | 0 |  | 4,293 | 0% |
| Render/chat_qwen | 1,082 | -3.22% | 389 | -3.95% | 0 |  | 3,449 | +4.90% |
| Load/config_file | 226 | +5.12% | 189 | +1.07% | 0 |  | 2,639 | -1.31% |
| Render/config_file | 6,315 | +0.02% | 1,498 | -1.38% | 0 |  | 41,978 | -2.73% |
| Load/dict_ops | 30 | -33.33% | 19 | -24.00% | 0 |  | 1,316 | -0.75% |
| Render/dict_ops | 675 | -1.17% | 426 | 0% | 0 |  | 878 | +0.11% |
| Load/expressions | 20 | -42.86% | 12 | -36.84% | 0 |  | 1,265 | -3.14% |
| Render/expressions | 17 | +41.67% | 6 | +20.00% | 0 |  | 270 | -17.68% |
| Load/filters | 84 | -5.62% | 45 | -4.26% | 0 |  | 1,424 | -13.22% |
| Render/filters | 17 | -10.53% | 8 | +14.29% | 0 |  | 791 | +5.33% |
| Load/for_filter_if | 32 | -21.95% | 22 | -4.35% | 0 |  | 1,046 | +2.55% |
| Render/for_filter_if | 1,735 | -0.17% | 135 | +0.75% | 0 |  | 132 | -26.26% |
| Load/for_loop_vars | 31 | -45.61% | 18 | -14.29% | 0 |  | 1,065 | +0.09% |
| Render/for_loop_vars | 6 | -50.00% | 1 | -80.00% | 0 |  | 132 | -2.22% |
| Load/for_range | 3 | 0% | 4 | 0% | 0 |  | 756 | +0.53% |
| Render/for_range | 1 | 0% | 0 |  | 0 |  | 98 | -28.47% |
| Load/hf_deepseek_v31 | 1,004 | +0.20% | 976 | +1.04% | 0 |  | 6,029 | +7.11% |
| Render/hf_deepseek_v31 | 362 | +1.69% | 74 | -3.90% | 0 |  | 1,376 | -5.17% |
| Load/hf_gpt_oss | 4,592 | -0.54% | 3,285 | -0.48% | 0 |  | 20,850 | -0.49% |
| Render/hf_gpt_oss | 1,214 | -0.41% | 365 | -0.27% | 0 |  | 4,543 | +4.58% |
| Load/hf_mistral_small | 1,794 | +0.56% | 1,588 | -0.38% | 0 |  | 10,657 | +2.51% |
| Render/hf_mistral_small | 981 | +0.20% | 316 | -1.25% | 0 |  | 3,737 | -1.14% |
| Load/hf_qwen3 | 1,454 | -2.09% | 1,281 | -0.16% | 0 |  | 8,392 | -0.53% |
| Render/hf_qwen3 | 668 | +0.15% | 207 | -2.36% | 0 |  | 2,541 | +0.43% |
| Load/html_autoescape | 478 | -2.25% | 503 | -1.37% | 0 |  | 3,564 | -7.33% |
| Render/html_autoescape | 5,086 | +0.73% | 1,607 | +15.45% | 0 |  | 35,218 | -2.48% |
| Load/inheritance | 4 | 0% | 3 | 0% | 0 |  | 697 | -5.56% |
| Render/inheritance | 587 | -0.84% | 95 | -4.04% | 0 |  | 941 | +4.44% |
| Load/int_arith | 71 | -8.97% | 39 | 0% | 0 |  | 1,969 | -2.19% |
| Render/int_arith | 22 | +57.14% | 8 | +100.00% | 0 |  | 708 | -3.41% |
| Module/keep | 2 | +100.00% | 2 | 0% | 0 |  | 404 | -7.34% |
| Module/keep_written | 2,933 | 0% | 561 | +0.18% | 0 |  | 721 | -3.99% |
| Load/large_static | 1,934 | -0.21% | 905 | -0.11% | 0 |  | 277 | -8.88% |
| Render/large_static | 711 | 0% | 568 | 0% | 0 |  | 7 | -56.25% |
| RenderSink/large_static | 0 |  | 0 |  | 0 |  | 5 | +25.00% |
| RenderTo/large_static | 688 | 0% | 569 | 0% | 0 |  | 6 | -14.29% |
| Load/macros | 76 | -5.00% | 61 | +8.93% | 0 |  | 1,685 | -1.81% |
| Render/macros | 752 | -0.66% | 214 | -0.47% | 0 |  | 2,239 | +220.77% |
| Load/many_tags | 27,927 | -1.51% | 22,378 | -0.19% | 0 |  | 224,255 | -2.75% |
| Render/many_tags | 6,224 | +0.03% | 869 | +0.35% | 0 |  | 147 | +56.38% |
| Load/mitsuhiko_table | 39 | -18.75% | 41 | -8.89% | 0 |  | 889 | +1.83% |
| Render/mitsuhiko_table | 10,051 | +0.06% | 6,270 | -0.06% | 0 |  | 335 | -2.05% |
| RenderSink/mitsuhiko_table | 9,978 | +0.27% | 1,182 | -0.17% | 0 |  | 266 | -5.67% |
| RenderTo/mitsuhiko_table | 10,053 | +0.12% | 6,284 | -0.13% | 0 |  | 308 | -5.52% |
| Load/mitsuhiko_table_wide | 73 | -10.98% | 89 | -9.18% | 0 |  | 981 | +3.26% |
| Render/mitsuhiko_table_wide | 10,249 | +0.25% | 22,667 | -0.02% | 0 |  | 337 | -10.13% |
| RenderSink/mitsuhiko_table_wide | 10,079 | +0.27% | 1,288 | -0.54% | 0 |  | 291 | +1.39% |
| RenderTo/mitsuhiko_table_wide | 10,250 | +0.22% | 22,675 | -0.01% | 0 |  | 313 | -11.33% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 20 | +17.65% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 7 | -12.50% |
| RenderSink/plain_text | 0 |  | 0 |  | 0 |  | 5 | -16.67% |
| RenderTo/plain_text | 0 |  | 0 |  | 0 |  | 6 | -25.00% |
| Load/prompt_budget | 78 | +4.00% | 39 | -4.88% | 0 |  | 1,389 | -0.22% |
| Render/prompt_budget | 4,821 | -0.04% | 526 | +0.19% | 0 |  | 853 | -0.23% |
| Load/strings | 135 | -2.88% | 94 | -4.08% | 0 |  | 2,713 | -0.59% |
| Render/strings | 425 | -1.16% | 479 | -1.03% | 0 |  | 2,072 | -6.07% |
| Load/substitute | 1 | 0% | 3 | 0% | 0 |  | 186 | +9.41% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 5 | -61.54% |
| RenderSink/substitute | 0 |  | 0 |  | 0 |  | 5 | +25.00% |
| RenderTo/substitute | 0 |  | 0 |  | 0 |  | 5 | 0% |

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
