# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (202 so far, latest [7787250](https://github.com/jinja2cpp/Jinja2Cpp/commit/7787250273f6e20f29f4f5466d1506b2bad6cd6c) on 2026-11-11). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 342,431 | -0.03% |  | 55 | 0% |  | 18,480 | 0% | -52.71% |
| [Render/chat_llama](#renderchat_llama) | 296,599 | +0.03% |  | 225 | 0% |  | 11,616 | 0% | -19.11% |
| [Load/chat_mistral](#loadchat_mistral) | 392,733 | +0.10% |  | 52 | 0% |  | 20,888 | 0% | -53.22% |
| [Render/chat_mistral](#renderchat_mistral) | 500,902 | +0.01% |  | 402 | 0% |  | 9,984 | 0% | -24.41% |
| [Load/chat_qwen](#loadchat_qwen) | 265,835 | -0.07% |  | 44 | 0% |  | 14,416 | 0% | -47.25% |
| [Render/chat_qwen](#renderchat_qwen) | 277,721 | -0.01% |  | 212 | 0% |  | 8,304 | 0% | -2.35% |
| [Load/config_file](#loadconfig_file) | 96,462 | -0.38% |  | 25 | 0% |  | 5,464 | 0% | -48.26% |
| [Render/config_file](#renderconfig_file) | 1,272,660 | +0.13% |  | 908 | 0% |  | 24,440 | 0% | +2.65% |
| [Load/dict_ops](#loaddict_ops) | 32,993 | -0.37% | -77.67% | 9 | 0% | -93.13% | 2,176 | 0% | -48.87% |
| [Render/dict_ops](#renderdict_ops) | 293,634 | 0% | -57.50% | 112 | 0% | -75.11% | 24,408 | 0% | -29.65% |
| [Load/expressions](#loadexpressions) | 44,924 | -0.11% | -78.21% | 16 | 0% | -89.04% | 2,728 | 0% | -55.54% |
| [Render/expressions](#renderexpressions) | 426,891 | 0% | -52.89% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 69,102 | +0.02% | -74.82% | 27 | 0% | -88.16% | 3,848 | 0% | -66.29% |
| [Render/filters](#renderfilters) | 27,336 | -0.02% | -69.17% | 23 | 0% | -68.92% | 752 | 0% | -84.69% |
| [Load/for_filter_if](#loadfor_filter_if) | 29,773 | -0.42% | -78.01% | 7 | 0% | -94.31% | 1,904 | 0% | -44.78% |
| [Render/for_filter_if](#renderfor_filter_if) | 274,384 | 0% | -68.30% | 4 | 0% | -98.23% | 16,560 | 0% | +354.95% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 39,966 | -0.86% | -77.88% | 10 | 0% | -93.63% | 2,368 | 0% | -47.61% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 180,540 | -0.01% | -83.17% | 2 | 0% | -99.62% | 1,568 | 0% | -43.35% |
| [Load/for_range](#loadfor_range) | 13,523 | -0.29% | -76.14% | 7 | 0% | -87.50% | 1,088 | 0% | -45.60% |
| [Render/for_range](#renderfor_range) | 38,357 | +0.03% | -58.40% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/hf_deepseek_v31](#loadhf_deepseek_v31) | 386,379 | +0.25% |  | 55 | 0% |  | 21,976 | 0% | +0.73% |
| [Render/hf_deepseek_v31](#renderhf_deepseek_v31) | 91,358 | +0.04% |  | 43 | 0% |  | 2,480 | 0% | 0% |
| [Load/hf_gpt_oss](#loadhf_gpt_oss) | 1,539,194 | -0.01% |  | 287 | 0% |  | 74,672 | 0% | +0.60% |
| [Render/hf_gpt_oss](#renderhf_gpt_oss) | 171,521 | +0.08% |  | 155 | 0% |  | 5,080 | 0% | +2.92% |
| [Load/hf_mistral_small](#loadhf_mistral_small) | 598,529 | +0.21% |  | 88 | 0% |  | 33,048 | 0% | +0.34% |
| [Render/hf_mistral_small](#renderhf_mistral_small) | 141,611 | +0.02% |  | 99 | 0% |  | 6,120 | 0% | -0.26% |
| [Load/hf_qwen3](#loadhf_qwen3) | 483,799 | -0.16% |  | 82 | 0% |  | 25,560 | 0% | +0.31% |
| [Render/hf_qwen3](#renderhf_qwen3) | 120,331 | +0.08% |  | 100 | 0% |  | 3,248 | 0% | 0% |
| [Load/html_autoescape](#loadhtml_autoescape) | 193,159 | -0.65% |  | 43 | 0% |  | 12,672 | +0.25% | -48.65% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,208,627 | +0.02% |  | 674 | 0% |  | 45,408 | 0% | +12.44% |
| [Load/inheritance](#loadinheritance) | 22,709 | +0.42% | -80.10% | 7 | 0% | -93.46% | 1,344 | 0% | -53.20% |
| [Render/inheritance](#renderinheritance) | 137,504 | +0.05% | -81.61% | 35 | 0% | -94.37% | 7,384 | 0% | +22.74% |
| [Load/int_arith](#loadint_arith) | 65,977 | -0.40% |  | 20 | 0% |  | 3,872 | 0% | +0.41% |
| [Render/int_arith](#renderint_arith) | 826,343 | 0% |  | 12 | 0% |  | 3,272 | 0% | -0.49% |
| [Module/keep](#modulekeep) | 7,061 | +0.14% |  | 7 | 0% |  |  |  |  |
| [Module/keep_written](#modulekeep_written) | 28,942 | +0.06% |  | 11 | 0% |  |  |  |  |
| [Load/large_static](#loadlarge_static) | 152,704 | -0.57% | -82.76% | 14 | 0% | -96.82% | 43,120 | 0% | -19.23% |
| [Render/large_static](#renderlarge_static) | 20,664 | 0% | -72.50% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [RenderSink/large_static](#rendersinklarge_static) | 12,702 | +0.01% |  | 0 |  |  | 0 |  |  |
| [RenderTo/large_static](#rendertolarge_static) | 20,275 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/macros](#loadmacros) | 56,421 | +0.13% | -75.25% | 27 | 0% | -85.48% | 4,392 | 0% | -29.34% |
| [Render/macros](#rendermacros) | 659,107 | +0.18% | -74.28% | 308 | 0% | -88.71% | 20,560 | 0% | +39.67% |
| [Load/many_tags](#loadmany_tags) | 8,982,454 | -0.14% | -79.66% | 951 | 0% | -97.22% | 438,128 | 0% | -58.48% |
| [Render/many_tags](#rendermany_tags) | 729,869 | +0.20% | -64.96% | 2 | 0% | -90.48% | 5,648 | 0% | -10.52% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 46,346 | -0.25% | -80.49% | 19 | 0% | -89.67% | 3,360 | 0% | -44.59% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,202,650 | 0% | -57.89% | 33 | 0% | -99.19% | 381,536 | 0% | +9.95% |
| [RenderSink/mitsuhiko_table](#rendersinkmitsuhiko_table) | 5,129,845 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table](#rendertomitsuhiko_table) | 5,202,156 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 54,036 | -0.38% |  | 35 | 0% |  | 5,688 | 0% | -31.17% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,548,817 | 0% |  | 33 | 0% |  | 1,414,928 | 0% | +2.46% |
| [RenderSink/mitsuhiko_table_wide](#rendersinkmitsuhiko_table_wide) | 5,372,109 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table_wide](#rendertomitsuhiko_table_wide) | 5,548,369 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/plain_text](#loadplain_text) | 3,706 | -0.83% | -61.70% | 4 | 0% | -77.78% | 672 | 0% | -40.00% |
| [Render/plain_text](#renderplain_text) | 1,022 | +0.10% | -84.96% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/plain_text](#rendersinkplain_text) | 724 | +0.14% |  | 0 |  |  | 0 |  |  |
| [RenderTo/plain_text](#rendertoplain_text) | 823 | +0.12% |  | 0 |  |  | 0 |  |  |
| [Load/prompt_budget](#loadprompt_budget) | 35,062 | -0.22% |  | 18 | 0% |  | 2,224 | 0% | 0% |
| [Render/prompt_budget](#renderprompt_budget) | 1,315,493 | -0.01% |  | 76 | 0% |  | 36,416 | 0% | 0% |
| [Load/strings](#loadstrings) | 70,522 | -0.09% | -75.84% | 30 | 0% | -86.61% | 4,344 | 0% | -51.95% |
| [Render/strings](#renderstrings) | 581,806 | -0.02% | -72.26% | 344 | 0% | -83.73% | 9,088 | 0% | -49.31% |
| [Load/substitute](#loadsubstitute) | 7,643 | -0.89% | -74.40% | 6 | 0% | -83.78% | 832 | 0% | -45.26% |
| [Render/substitute](#rendersubstitute) | 2,255 | +0.04% | -74.41% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/substitute](#rendersinksubstitute) | 1,961 | +0.05% |  | 0 |  |  | 0 |  |  |
| [RenderTo/substitute](#rendertosubstitute) | 2,060 | +0.05% |  | 0 |  |  | 0 |  |  |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 735 | +0.68% | 765 | +5.96% | 0 |  | 7,691 | +1.04% |
| Render/chat_llama | 1,154 | -2.62% | 502 | -3.83% | 0 |  | 3,900 | -1.56% |
| Load/chat_mistral | 1,079 | +2.86% | 1,069 | +4.91% | 0 |  | 8,682 | +2.23% |
| Render/chat_mistral | 1,909 | -1.55% | 576 | -2.21% | 0 |  | 9,700 | -3.25% |
| Load/chat_qwen | 560 | +8.95% | 627 | +8.29% | 0 |  | 4,662 | +4.44% |
| Render/chat_qwen | 1,093 | +0.28% | 391 | -2.74% | 0 |  | 3,294 | -2.49% |
| Load/config_file | 228 | +0.88% | 203 | +8.56% | 0 |  | 2,818 | +3.79% |
| Render/config_file | 6,103 | -3.30% | 1,537 | -1.09% | 0 |  | 41,940 | -6.21% |
| Load/dict_ops | 42 | 0% | 21 | +23.53% | 0 |  | 1,379 | +1.85% |
| Render/dict_ops | 678 | +0.15% | 430 | +1.18% | 0 |  | 860 | -2.82% |
| Load/expressions | 36 | 0% | 26 | +23.81% | 0 |  | 1,406 | +5.56% |
| Render/expressions | 13 | -13.33% | 7 | +16.67% | 0 |  | 279 | -12.26% |
| Load/filters | 113 | +20.21% | 61 | +15.09% | 0 |  | 1,840 | +18.18% |
| Render/filters | 17 | +21.43% | 3 | -50.00% | 0 |  | 751 | -0.27% |
| Load/for_filter_if | 35 | +12.90% | 21 | +40.00% | 0 |  | 1,079 | +0.47% |
| Render/for_filter_if | 1,736 | +0.23% | 139 | +7.75% | 0 |  | 193 | +7.82% |
| Load/for_loop_vars | 52 | +20.93% | 22 | +4.76% | 0 |  | 1,124 | +0.54% |
| Render/for_loop_vars | 7 | +40.00% | 3 |  | 0 |  | 163 | +1.88% |
| Load/for_range | 3 | 0% | 4 | +33.33% | 0 |  | 761 | +0.26% |
| Render/for_range | 1 | 0% | 0 |  | 0 |  | 160 | +8.11% |
| Load/hf_deepseek_v31 | 1,010 | +0.40% | 981 | +2.83% | 0 |  | 6,087 | -0.13% |
| Render/hf_deepseek_v31 | 350 | -3.58% | 80 | +12.68% | 0 |  | 1,336 | -3.12% |
| Load/hf_gpt_oss | 4,695 | +0.86% | 3,378 | +3.24% | 0 |  | 22,859 | +1.07% |
| Render/hf_gpt_oss | 1,192 | -0.91% | 367 | -2.65% | 0 |  | 4,517 | -0.37% |
| Load/hf_mistral_small | 1,863 | +0.27% | 1,641 | +4.19% | 0 |  | 11,184 | +1.58% |
| Render/hf_mistral_small | 959 | -2.34% | 299 | +1.01% | 0 |  | 3,602 | -0.66% |
| Load/hf_qwen3 | 1,517 | +6.23% | 1,341 | +6.43% | 0 |  | 8,987 | +0.95% |
| Render/hf_qwen3 | 674 | -1.32% | 214 | -0.47% | 0 |  | 2,565 | -0.35% |
| Load/html_autoescape | 498 | +5.96% | 548 | +8.30% | 0 |  | 4,151 | +4.98% |
| Render/html_autoescape | 4,297 | -7.51% | 1,359 | -0.37% | 0 |  | 35,760 | -4.12% |
| Load/inheritance | 3 | +50.00% | 3 | 0% | 0 |  | 694 | +1.31% |
| Render/inheritance | 585 | +1.04% | 92 | 0% | 0 |  | 1,178 | +20.94% |
| Load/int_arith | 106 | +41.33% | 69 | +137.93% | 0 |  | 2,234 | +6.08% |
| Render/int_arith | 17 | +30.77% | 9 | +80.00% | 0 |  | 714 | -2.46% |
| Module/keep | 2 | +100.00% | 2 | 0% | 0 |  | 442 | -2.43% |
| Module/keep_written | 1,128 | 0% | 562 | 0% | 0 |  | 752 | +1.35% |
| Load/large_static | 1,933 | -0.21% | 913 | +0.22% | 0 |  | 313 | +17.23% |
| Render/large_static | 713 | +0.14% | 572 | -0.17% | 0 |  | 11 | +83.33% |
| RenderSink/large_static | 0 |  | 0 |  | 0 |  | 3 | +50.00% |
| RenderTo/large_static | 689 | 0% | 568 | 0% | 0 |  | 4 | -20.00% |
| Load/macros | 99 | +17.86% | 90 | +36.36% | 0 |  | 1,767 | +3.45% |
| Render/macros | 755 | +0.80% | 236 | +0.43% | 0 |  | 1,052 | -21.20% |
| Load/many_tags | 29,026 | -0.32% | 22,791 | +1.27% | 0 |  | 239,118 | +8.67% |
| Render/many_tags | 6,224 | +0.02% | 863 | -1.26% | 0 |  | 152 | -8.98% |
| Load/mitsuhiko_table | 45 | -2.17% | 55 | +34.15% | 0 |  | 915 | +2.23% |
| Render/mitsuhiko_table | 10,043 | +0.10% | 6,277 | +0.13% | 0 |  | 351 | -3.04% |
| RenderSink/mitsuhiko_table | 9,961 | +0.14% | 1,178 | +0.17% | 0 |  | 309 | +4.75% |
| RenderTo/mitsuhiko_table | 10,050 | +0.12% | 6,282 | +0.10% | 0 |  | 334 | +0.30% |
| Load/mitsuhiko_table_wide | 72 | -15.29% | 94 | +8.05% | 0 |  | 1,036 | -5.22% |
| Render/mitsuhiko_table_wide | 10,225 | -0.05% | 22,687 | 0% | 0 |  | 368 | +4.55% |
| RenderSink/mitsuhiko_table_wide | 10,049 | -0.12% | 1,308 | +0.62% | 0 |  | 320 | +4.23% |
| RenderTo/mitsuhiko_table_wide | 10,226 | -0.01% | 22,691 | -0.03% | 0 |  | 352 | +10.00% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 17 | +21.43% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 6 | 0% |
| RenderSink/plain_text | 0 |  | 0 |  | 0 |  | 5 | 0% |
| RenderTo/plain_text | 0 |  | 0 |  | 0 |  | 6 | 0% |
| Load/prompt_budget | 77 | +16.67% | 45 | +21.62% | 0 |  | 1,389 | +0.73% |
| Render/prompt_budget | 4,812 | -0.33% | 519 | -0.95% | 0 |  | 836 | -2.11% |
| Load/strings | 177 | +29.20% | 127 | +41.11% | 0 |  | 2,800 | +1.97% |
| Render/strings | 423 | -0.47% | 483 | +0.21% | 0 |  | 1,571 | 0% |
| Load/substitute | 1 | -50.00% | 3 | 0% | 0 |  | 190 | +11.76% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 2 | -60.00% |
| RenderSink/substitute | 0 |  | 0 |  | 0 |  | 3 | +50.00% |
| RenderTo/substitute | 0 |  | 0 |  | 0 |  | 2 | -50.00% |

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
