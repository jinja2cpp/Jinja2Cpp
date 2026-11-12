# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (208 so far, latest [d09926f](https://github.com/jinja2cpp/Jinja2Cpp/commit/d09926ff158de5ccf03f47a7445f1f19bcdb7136) on 2026-11-12). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 336,221 | +0.03% |  | 55 | 0% |  | 18,464 | 0% | -52.75% |
| [Render/chat_llama](#renderchat_llama) | 294,234 | 0% |  | 225 | 0% |  | 11,616 | 0% | -19.11% |
| [Load/chat_mistral](#loadchat_mistral) | 386,346 | +0.01% |  | 52 | 0% |  | 20,888 | 0% | -53.22% |
| [Render/chat_mistral](#renderchat_mistral) | 499,613 | 0% |  | 402 | 0% |  | 10,000 | 0% | -24.29% |
| [Load/chat_qwen](#loadchat_qwen) | 262,260 | +0.02% |  | 44 | 0% |  | 14,416 | 0% | -47.25% |
| [Render/chat_qwen](#renderchat_qwen) | 276,401 | 0% |  | 212 | 0% |  | 8,304 | 0% | -2.35% |
| [Load/config_file](#loadconfig_file) | 95,107 | +0.04% |  | 25 | 0% |  | 5,464 | 0% | -48.26% |
| [Render/config_file](#renderconfig_file) | 1,276,712 | -0.05% |  | 908 | 0% |  | 24,440 | 0% | +2.65% |
| [Load/dict_ops](#loaddict_ops) | 32,374 | +0.02% | -78.09% | 9 | 0% | -93.13% | 2,176 | 0% | -48.87% |
| [Render/dict_ops](#renderdict_ops) | 293,341 | -0.01% | -57.54% | 112 | 0% | -75.11% | 24,408 | 0% | -29.65% |
| [Load/expressions](#loadexpressions) | 44,205 | 0% | -78.56% | 16 | 0% | -89.04% | 2,728 | 0% | -55.54% |
| [Render/expressions](#renderexpressions) | 426,896 | 0% | -52.89% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 67,957 | +0.07% | -75.24% | 27 | 0% | -88.16% | 3,848 | 0% | -66.29% |
| [Render/filters](#renderfilters) | 27,405 | 0% | -69.09% | 23 | 0% | -68.92% | 752 | 0% | -84.69% |
| [Load/for_filter_if](#loadfor_filter_if) | 29,274 | 0% | -78.38% | 7 | 0% | -94.31% | 1,904 | 0% | -44.78% |
| [Render/for_filter_if](#renderfor_filter_if) | 274,346 | 0% | -68.30% | 4 | 0% | -98.23% | 16,560 | 0% | +354.95% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 39,161 | -0.04% | -78.32% | 10 | 0% | -93.63% | 2,368 | 0% | -47.61% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 180,540 | 0% | -83.17% | 2 | 0% | -99.62% | 1,568 | 0% | -43.35% |
| [Load/for_range](#loadfor_range) | 13,250 | 0% | -76.62% | 7 | 0% | -87.50% | 1,088 | 0% | -45.60% |
| [Render/for_range](#renderfor_range) | 38,347 | -0.04% | -58.41% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/hf_deepseek_v31](#loadhf_deepseek_v31) | 383,729 | 0% |  | 55 | 0% |  | 21,976 | 0% | +0.73% |
| [Render/hf_deepseek_v31](#renderhf_deepseek_v31) | 91,515 | 0% |  | 43 | 0% |  | 2,480 | 0% | 0% |
| [Load/hf_gpt_oss](#loadhf_gpt_oss) | 1,529,904 | +0.01% |  | 287 | 0% |  | 74,704 | 0% | +0.65% |
| [Render/hf_gpt_oss](#renderhf_gpt_oss) | 172,063 | 0% |  | 155 | 0% |  | 5,096 | 0% | +3.24% |
| [Load/hf_mistral_small](#loadhf_mistral_small) | 592,628 | 0% |  | 88 | 0% |  | 33,048 | 0% | +0.34% |
| [Render/hf_mistral_small](#renderhf_mistral_small) | 141,603 | 0% |  | 99 | 0% |  | 6,120 | 0% | -0.26% |
| [Load/hf_qwen3](#loadhf_qwen3) | 478,070 | +0.02% |  | 82 | 0% |  | 25,560 | 0% | +0.31% |
| [Render/hf_qwen3](#renderhf_qwen3) | 120,265 | 0% |  | 100 | 0% |  | 3,248 | 0% | 0% |
| [Load/html_autoescape](#loadhtml_autoescape) | 190,670 | +0.01% |  | 43 | 0% |  | 12,672 | 0% | -48.65% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,208,533 | 0% |  | 674 | 0% |  | 45,408 | 0% | +12.44% |
| [Load/inheritance](#loadinheritance) | 21,971 | 0% | -80.75% | 7 | 0% | -93.46% | 1,344 | 0% | -53.20% |
| [Render/inheritance](#renderinheritance) | 137,475 | 0% | -81.62% | 35 | 0% | -94.37% | 7,384 | 0% | +22.74% |
| [Load/int_arith](#loadint_arith) | 65,190 | -0.02% |  | 20 | 0% |  | 3,872 | 0% | +0.41% |
| [Render/int_arith](#renderint_arith) | 827,157 | 0% |  | 12 | 0% |  | 3,272 | 0% | -0.49% |
| [Module/keep](#modulekeep) | 7,107 | 0% |  | 7 | 0% |  |  |  |  |
| [Module/keep_written](#modulekeep_written) | 28,989 | 0% |  | 11 | 0% |  |  |  |  |
| [Load/large_static](#loadlarge_static) | 149,411 | 0% | -83.14% | 14 | 0% | -96.82% | 43,120 | 0% | -19.23% |
| [Render/large_static](#renderlarge_static) | 20,712 | 0% | -72.43% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [RenderSink/large_static](#rendersinklarge_static) | 12,702 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/large_static](#rendertolarge_static) | 20,275 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/macros](#loadmacros) | 55,622 | 0% | -75.60% | 27 | 0% | -85.48% | 4,392 | 0% | -29.34% |
| [Render/macros](#rendermacros) | 660,048 | 0% | -74.25% | 308 | 0% | -88.71% | 20,560 | 0% | +39.67% |
| [Load/many_tags](#loadmany_tags) | 8,793,891 | +0.08% | -80.09% | 951 | 0% | -97.22% | 438,112 | 0% | -58.48% |
| [Render/many_tags](#rendermany_tags) | 728,403 | 0% | -65.03% | 2 | 0% | -90.48% | 5,648 | 0% | -10.52% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 45,391 | 0% | -80.89% | 19 | 0% | -89.67% | 3,360 | 0% | -44.59% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,202,678 | 0% | -57.89% | 33 | 0% | -99.19% | 381,536 | 0% | +9.95% |
| [RenderSink/mitsuhiko_table](#rendersinkmitsuhiko_table) | 5,129,875 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table](#rendertomitsuhiko_table) | 5,202,184 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 53,164 | 0% |  | 35 | 0% |  | 5,688 | 0% | -31.17% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,548,834 | 0% |  | 33 | 0% |  | 1,414,928 | 0% | +2.46% |
| [RenderSink/mitsuhiko_table_wide](#rendersinkmitsuhiko_table_wide) | 5,372,140 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table_wide](#rendertomitsuhiko_table_wide) | 5,548,402 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/plain_text](#loadplain_text) | 3,713 | 0% | -61.63% | 4 | 0% | -77.78% | 672 | 0% | -40.00% |
| [Render/plain_text](#renderplain_text) | 1,022 | 0% | -84.96% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/plain_text](#rendersinkplain_text) | 724 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/plain_text](#rendertoplain_text) | 823 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/prompt_budget](#loadprompt_budget) | 34,625 | 0% |  | 18 | 0% |  | 2,224 | 0% | 0% |
| [Render/prompt_budget](#renderprompt_budget) | 1,315,666 | 0% |  | 76 | 0% |  | 36,416 | 0% | 0% |
| [Load/strings](#loadstrings) | 69,440 | +0.02% | -76.21% | 30 | 0% | -86.61% | 4,344 | 0% | -51.95% |
| [Render/strings](#renderstrings) | 582,714 | 0% | -72.22% | 344 | 0% | -83.73% | 9,088 | 0% | -49.31% |
| [Load/substitute](#loadsubstitute) | 7,512 | 0% | -74.84% | 6 | 0% | -83.78% | 832 | 0% | -45.26% |
| [Render/substitute](#rendersubstitute) | 2,255 | 0% | -74.41% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/substitute](#rendersinksubstitute) | 1,961 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/substitute](#rendertosubstitute) | 2,060 | 0% |  | 0 |  |  | 0 |  |  |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 748 | -1.45% | 738 | -1.34% | 0 |  | 7,982 | +5.65% |
| Render/chat_llama | 1,163 | +1.75% | 493 | +0.20% | 0 |  | 3,768 | -4.20% |
| Load/chat_mistral | 1,081 | -1.46% | 1,079 | 0% | 0 |  | 9,310 | +9.52% |
| Render/chat_mistral | 1,922 | +1.32% | 538 | +0.75% | 0 |  | 10,118 | +1.47% |
| Load/chat_qwen | 619 | -1.59% | 629 | +0.48% | 0 |  | 5,098 | +17.11% |
| Render/chat_qwen | 1,108 | +2.97% | 376 | +2.73% | 0 |  | 3,276 | -3.48% |
| Load/config_file | 227 | +0.89% | 210 | -4.11% | 0 |  | 2,905 | +6.25% |
| Render/config_file | 6,193 | -4.96% | 1,527 | -0.07% | 0 |  | 43,003 | +0.16% |
| Load/dict_ops | 59 | +11.32% | 30 | +7.14% | 0 |  | 1,400 | +1.38% |
| Render/dict_ops | 690 | -0.58% | 433 | -0.23% | 0 |  | 887 | +1.84% |
| Load/expressions | 24 | 0% | 19 | 0% | 0 |  | 1,379 | +4.79% |
| Render/expressions | 17 | +6.25% | 7 | 0% | 0 |  | 332 | -0.90% |
| Load/filters | 83 | -5.68% | 46 | -6.12% | 0 |  | 1,903 | +15.54% |
| Render/filters | 19 | -24.00% | 8 | -20.00% | 0 |  | 774 | -1.28% |
| Load/for_filter_if | 31 | 0% | 15 | 0% | 0 |  | 1,101 | +3.09% |
| Render/for_filter_if | 1,729 | -0.17% | 134 | 0% | 0 |  | 217 | +18.58% |
| Load/for_loop_vars | 34 | -8.11% | 22 | -4.35% | 0 |  | 1,136 | +1.07% |
| Render/for_loop_vars | 12 | 0% | 2 | 0% | 0 |  | 202 | +37.41% |
| Load/for_range | 3 | 0% | 3 | 0% | 0 |  | 769 | +0.13% |
| Render/for_range | 5 | -28.57% | 2 | 0% | 0 |  | 149 | +19.20% |
| Load/hf_deepseek_v31 | 978 | +0.51% | 985 | -0.20% | 0 |  | 7,152 | +8.45% |
| Render/hf_deepseek_v31 | 360 | +2.27% | 81 | 0% | 0 |  | 1,462 | +1.74% |
| Load/hf_gpt_oss | 4,688 | +0.36% | 3,365 | +0.15% | 0 |  | 26,105 | +17.01% |
| Render/hf_gpt_oss | 1,208 | -1.79% | 369 | +0.82% | 0 |  | 4,733 | +4.44% |
| Load/hf_mistral_small | 1,890 | -0.79% | 1,614 | -0.37% | 0 |  | 12,192 | +10.69% |
| Render/hf_mistral_small | 940 | -0.32% | 286 | +1.78% | 0 |  | 3,676 | +0.96% |
| Load/hf_qwen3 | 1,516 | -0.98% | 1,332 | +0.15% | 0 |  | 9,714 | +14.54% |
| Render/hf_qwen3 | 647 | +0.94% | 206 | -1.44% | 0 |  | 2,652 | +1.77% |
| Load/html_autoescape | 485 | -0.61% | 539 | -0.19% | 0 |  | 4,389 | +16.67% |
| Render/html_autoescape | 5,378 | -5.08% | 1,593 | +2.84% | 0 |  | 36,832 | +4.31% |
| Load/inheritance | 3 | 0% | 2 | 0% | 0 |  | 703 | -2.77% |
| Render/inheritance | 602 | -0.33% | 104 | -4.59% | 0 |  | 909 | -4.72% |
| Load/int_arith | 79 | 0% | 57 | 0% | 0 |  | 2,191 | +8.52% |
| Render/int_arith | 21 | 0% | 7 | 0% | 0 |  | 767 | +3.37% |
| Module/keep | 1 | 0% | 2 | 0% | 0 |  | 406 | -5.80% |
| Module/keep_written | 1,128 | -0.09% | 564 | 0% | 0 |  | 730 | -1.35% |
| Load/large_static | 1,930 | 0% | 907 | 0% | 0 |  | 338 | +9.39% |
| Render/large_static | 708 | 0% | 567 | 0% | 0 |  | 7 | +16.67% |
| RenderSink/large_static | 0 |  | 0 |  | 0 |  | 4 | +33.33% |
| RenderTo/large_static | 685 | 0% | 569 | 0% | 0 |  | 5 | 0% |
| Load/macros | 86 | +2.38% | 75 | +5.63% | 0 |  | 1,809 | +4.93% |
| Render/macros | 753 | -1.18% | 233 | -0.43% | 0 |  | 998 | +68.01% |
| Load/many_tags | 28,229 | +0.34% | 22,680 | +0.15% | 0 |  | 259,258 | +13.03% |
| Render/many_tags | 6,249 | -0.10% | 862 | -0.58% | 0 |  | 170 | -10.05% |
| Load/mitsuhiko_table | 31 | -11.43% | 46 | -6.12% | 0 |  | 913 | +1.22% |
| Render/mitsuhiko_table | 10,075 | +0.08% | 6,269 | -0.14% | 0 |  | 374 | +3.60% |
| RenderSink/mitsuhiko_table | 9,969 | -0.02% | 1,188 | 0% | 0 |  | 319 | +8.50% |
| RenderTo/mitsuhiko_table | 10,055 | -0.07% | 6,275 | 0% | 0 |  | 338 | -1.17% |
| Load/mitsuhiko_table_wide | 79 | +3.95% | 89 | +9.88% | 0 |  | 1,011 | -1.27% |
| Render/mitsuhiko_table_wide | 10,262 | +0.11% | 22,676 | -0.02% | 0 |  | 350 | -4.63% |
| RenderSink/mitsuhiko_table_wide | 10,077 | +0.10% | 1,297 | -0.38% | 0 |  | 299 | +3.10% |
| RenderTo/mitsuhiko_table_wide | 10,277 | +0.23% | 22,681 | -0.02% | 0 |  | 319 | -7.80% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 23 | +53.33% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 6 | -25.00% |
| RenderSink/plain_text | 0 |  | 0 |  | 0 |  | 5 | -28.57% |
| RenderTo/plain_text | 0 |  | 0 |  | 0 |  | 5 | -28.57% |
| Load/prompt_budget | 81 | -2.41% | 45 | -6.25% | 0 |  | 1,420 | +2.23% |
| Render/prompt_budget | 4,808 | +0.04% | 539 | +0.75% | 0 |  | 908 | +2.71% |
| Load/strings | 156 | -3.11% | 129 | +0.78% | 0 |  | 2,845 | +5.49% |
| Render/strings | 450 | +0.67% | 488 | +0.21% | 0 |  | 1,587 | -1.43% |
| Load/substitute | 2 | 0% | 3 | 0% | 0 |  | 180 | +15.38% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 4 | -20.00% |
| RenderSink/substitute | 0 |  | 0 |  | 0 |  | 4 | 0% |
| RenderTo/substitute | 0 |  | 0 |  | 0 |  | 4 | -20.00% |

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
