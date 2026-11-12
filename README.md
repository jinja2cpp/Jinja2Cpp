# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (206 so far, latest [8f06315](https://github.com/jinja2cpp/Jinja2Cpp/commit/8f06315c1955dc8cd73e3f90f0a603000189cab0) on 2026-11-12). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 335,938 | -1.82% |  | 55 | 0% |  | 18,464 | 0% | -52.75% |
| [Render/chat_llama](#renderchat_llama) | 296,087 | 0% |  | 225 | 0% |  | 11,616 | 0% | -19.11% |
| [Load/chat_mistral](#loadchat_mistral) | 386,310 | -1.64% |  | 52 | 0% |  | 20,888 | 0% | -53.22% |
| [Render/chat_mistral](#renderchat_mistral) | 500,909 | 0% |  | 402 | 0% |  | 9,920 | 0% | -24.89% |
| [Load/chat_qwen](#loadchat_qwen) | 262,175 | -1.38% |  | 44 | 0% |  | 14,416 | 0% | -47.25% |
| [Render/chat_qwen](#renderchat_qwen) | 277,817 | 0% |  | 212 | 0% |  | 8,304 | 0% | -2.35% |
| [Load/config_file](#loadconfig_file) | 95,068 | -1.45% |  | 25 | 0% |  | 5,464 | 0% | -48.26% |
| [Render/config_file](#renderconfig_file) | 1,273,231 | 0% |  | 908 | 0% |  | 24,440 | 0% | +2.65% |
| [Load/dict_ops](#loaddict_ops) | 32,371 | -1.87% | -78.09% | 9 | 0% | -93.13% | 2,176 | 0% | -48.87% |
| [Render/dict_ops](#renderdict_ops) | 293,639 | 0% | -57.50% | 112 | 0% | -75.11% | 24,408 | 0% | -29.65% |
| [Load/expressions](#loadexpressions) | 44,205 | -1.60% | -78.56% | 16 | 0% | -89.04% | 2,728 | 0% | -55.54% |
| [Render/expressions](#renderexpressions) | 426,906 | 0% | -52.89% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 67,902 | -1.71% | -75.26% | 27 | 0% | -88.16% | 3,848 | 0% | -66.29% |
| [Render/filters](#renderfilters) | 27,336 | 0% | -69.17% | 23 | 0% | -68.92% | 752 | 0% | -84.69% |
| [Load/for_filter_if](#loadfor_filter_if) | 29,288 | -1.78% | -78.37% | 7 | 0% | -94.31% | 1,904 | 0% | -44.78% |
| [Render/for_filter_if](#renderfor_filter_if) | 274,386 | 0% | -68.30% | 4 | 0% | -98.23% | 16,560 | 0% | +354.95% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 39,176 | -1.84% | -78.31% | 10 | 0% | -93.63% | 2,368 | 0% | -47.61% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 180,540 | 0% | -83.17% | 2 | 0% | -99.62% | 1,568 | 0% | -43.35% |
| [Load/for_range](#loadfor_range) | 13,247 | -2.04% | -76.63% | 7 | 0% | -87.50% | 1,088 | 0% | -45.60% |
| [Render/for_range](#renderfor_range) | 38,357 | 0% | -58.40% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/hf_deepseek_v31](#loadhf_deepseek_v31) | 383,758 | -0.74% |  | 55 | 0% |  | 21,976 | 0% | +0.73% |
| [Render/hf_deepseek_v31](#renderhf_deepseek_v31) | 91,370 | 0% |  | 43 | 0% |  | 2,480 | 0% | 0% |
| [Load/hf_gpt_oss](#loadhf_gpt_oss) | 1,528,668 | -0.76% |  | 287 | 0% |  | 74,672 | 0% | +0.60% |
| [Render/hf_gpt_oss](#renderhf_gpt_oss) | 171,670 | 0% |  | 155 | 0% |  | 5,064 | 0% | +2.59% |
| [Load/hf_mistral_small](#loadhf_mistral_small) | 592,359 | -1.05% |  | 88 | 0% |  | 33,048 | 0% | +0.34% |
| [Render/hf_mistral_small](#renderhf_mistral_small) | 141,550 | 0% |  | 99 | 0% |  | 6,120 | 0% | -0.26% |
| [Load/hf_qwen3](#loadhf_qwen3) | 477,990 | -1.30% |  | 82 | 0% |  | 25,560 | 0% | +0.31% |
| [Render/hf_qwen3](#renderhf_qwen3) | 120,205 | 0% |  | 100 | 0% |  | 3,248 | 0% | 0% |
| [Load/html_autoescape](#loadhtml_autoescape) | 190,724 | -1.27% |  | 43 | 0% |  | 12,672 | 0% | -48.65% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,208,113 | 0% |  | 674 | 0% |  | 45,408 | 0% | +12.44% |
| [Load/inheritance](#loadinheritance) | 21,964 | -3.28% | -80.75% | 7 | 0% | -93.46% | 1,344 | 0% | -53.20% |
| [Render/inheritance](#renderinheritance) | 137,432 | 0% | -81.62% | 35 | 0% | -94.37% | 7,384 | 0% | +22.74% |
| [Load/int_arith](#loadint_arith) | 65,199 | -1.45% |  | 20 | 0% |  | 3,872 | 0% | +0.41% |
| [Render/int_arith](#renderint_arith) | 826,343 | 0% |  | 12 | 0% |  | 3,272 | 0% | -0.49% |
| [Module/keep](#modulekeep) | 7,054 | 0% |  | 7 | 0% |  |  |  |  |
| [Module/keep_written](#modulekeep_written) | 28,934 | 0% |  | 11 | 0% |  |  |  |  |
| [Load/large_static](#loadlarge_static) | 149,408 | -2.16% | -83.14% | 14 | 0% | -96.82% | 43,120 | 0% | -19.23% |
| [Render/large_static](#renderlarge_static) | 20,664 | 0% | -72.50% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [RenderSink/large_static](#rendersinklarge_static) | 12,702 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/large_static](#rendertolarge_static) | 20,275 | +0.01% |  | 0 |  |  | 0 |  |  |
| [Load/macros](#loadmacros) | 55,619 | -1.42% | -75.60% | 27 | 0% | -85.48% | 4,392 | 0% | -29.34% |
| [Render/macros](#rendermacros) | 660,069 | 0% | -74.25% | 308 | 0% | -88.71% | 20,560 | 0% | +39.67% |
| [Load/many_tags](#loadmany_tags) | 8,791,117 | -2.13% | -80.09% | 951 | 0% | -97.22% | 438,160 | 0% | -58.48% |
| [Render/many_tags](#rendermany_tags) | 729,874 | 0% | -64.96% | 2 | 0% | -90.48% | 5,648 | 0% | -10.52% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 45,404 | -2.03% | -80.89% | 19 | 0% | -89.67% | 3,360 | 0% | -44.59% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,202,672 | 0% | -57.89% | 33 | 0% | -99.19% | 381,536 | 0% | +9.95% |
| [RenderSink/mitsuhiko_table](#rendersinkmitsuhiko_table) | 5,129,867 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table](#rendertomitsuhiko_table) | 5,202,178 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 53,131 | -1.71% |  | 35 | 0% |  | 5,688 | 0% | -31.17% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,548,839 | 0% |  | 33 | 0% |  | 1,414,928 | 0% | +2.46% |
| [RenderSink/mitsuhiko_table_wide](#rendersinkmitsuhiko_table_wide) | 5,372,131 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table_wide](#rendertomitsuhiko_table_wide) | 5,548,393 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/plain_text](#loadplain_text) | 3,710 | +0.11% | -61.66% | 4 | 0% | -77.78% | 672 | 0% | -40.00% |
| [Render/plain_text](#renderplain_text) | 1,022 | 0% | -84.96% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/plain_text](#rendersinkplain_text) | 724 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/plain_text](#rendertoplain_text) | 823 | +0.37% |  | 0 |  |  | 0 |  |  |
| [Load/prompt_budget](#loadprompt_budget) | 34,647 | -1.16% |  | 18 | 0% |  | 2,224 | 0% | 0% |
| [Render/prompt_budget](#renderprompt_budget) | 1,315,548 | 0% |  | 76 | 0% |  | 36,416 | 0% | 0% |
| [Load/strings](#loadstrings) | 69,335 | -1.62% | -76.24% | 30 | 0% | -86.61% | 4,344 | 0% | -51.95% |
| [Render/strings](#renderstrings) | 581,816 | 0% | -72.26% | 344 | 0% | -83.73% | 9,088 | 0% | -49.31% |
| [Load/substitute](#loadsubstitute) | 7,509 | -1.75% | -74.85% | 6 | 0% | -83.78% | 832 | 0% | -45.26% |
| [Render/substitute](#rendersubstitute) | 2,255 | 0% | -74.41% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/substitute](#rendersinksubstitute) | 1,961 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/substitute](#rendertosubstitute) | 2,060 | +0.15% |  | 0 |  |  | 0 |  |  |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 754 | -0.13% | 761 | -0.13% | 0 |  | 7,531 | -4.54% |
| Render/chat_llama | 1,155 | -0.43% | 506 | -0.20% | 0 |  | 3,964 | -2.05% |
| Load/chat_mistral | 1,101 | +0.46% | 1,088 | -0.27% | 0 |  | 8,562 | -5.27% |
| Render/chat_mistral | 1,945 | 0% | 601 | -0.50% | 0 |  | 9,620 | -3.34% |
| Load/chat_qwen | 617 | -0.80% | 631 | +1.12% | 0 |  | 4,625 | -5.38% |
| Render/chat_qwen | 1,098 | +0.09% | 400 | -0.25% | 0 |  | 3,409 | +3.33% |
| Load/config_file | 235 | +0.86% | 211 | -0.94% | 0 |  | 2,710 | -5.05% |
| Render/config_file | 6,400 | +1.07% | 1,704 | +0.83% | 0 |  | 42,235 | +4.90% |
| Load/dict_ops | 57 | +11.76% | 27 | +12.50% | 0 |  | 1,341 | -6.94% |
| Render/dict_ops | 658 | -0.45% | 426 | 0% | 0 |  | 879 | +1.15% |
| Load/expressions | 26 | -3.70% | 23 | 0% | 0 |  | 1,345 | -6.86% |
| Render/expressions | 16 | 0% | 7 | 0% | 0 |  | 305 | 0% |
| Load/filters | 97 | -11.01% | 54 | -6.90% | 0 |  | 1,650 | -6.25% |
| Render/filters | 27 | +8.00% | 8 | 0% | 0 |  | 741 | -5.48% |
| Load/for_filter_if | 27 | +3.85% | 14 | 0% | 0 |  | 1,089 | -4.14% |
| Render/for_filter_if | 1,737 | 0% | 130 | -1.52% | 0 |  | 203 | +5.73% |
| Load/for_loop_vars | 45 | -2.17% | 30 | -3.23% | 0 |  | 1,106 | -5.47% |
| Render/for_loop_vars | 6 | 0% | 1 | 0% | 0 |  | 185 | +34.06% |
| Load/for_range | 3 | -25.00% | 4 | 0% | 0 |  | 785 | +2.21% |
| Render/for_range | 6 | 0% | 0 |  | 0 |  | 126 | -14.29% |
| Load/hf_deepseek_v31 | 979 | +0.41% | 980 | +0.20% | 0 |  | 6,484 | -3.71% |
| Render/hf_deepseek_v31 | 333 | -0.89% | 72 | 0% | 0 |  | 1,345 | -1.97% |
| Load/hf_gpt_oss | 4,628 | -0.26% | 3,352 | -0.24% | 0 |  | 22,665 | -6.08% |
| Render/hf_gpt_oss | 1,183 | -0.42% | 367 | 0% | 0 |  | 4,592 | +1.39% |
| Load/hf_mistral_small | 1,876 | -0.53% | 1,619 | -1.34% | 0 |  | 11,252 | -4.67% |
| Render/hf_mistral_small | 943 | -0.63% | 292 | -0.34% | 0 |  | 3,516 | -3.01% |
| Load/hf_qwen3 | 1,514 | -0.59% | 1,327 | -0.75% | 0 |  | 8,929 | -4.31% |
| Render/hf_qwen3 | 653 | +0.46% | 212 | +0.47% | 0 |  | 2,605 | +0.31% |
| Load/html_autoescape | 485 | -0.61% | 537 | -0.37% | 0 |  | 3,940 | -8.80% |
| Render/html_autoescape | 3,959 | -4.12% | 1,472 | -1.67% | 0 |  | 34,947 | -4.88% |
| Load/inheritance | 3 | 0% | 3 | +50.00% | 0 |  | 713 | +3.18% |
| Render/inheritance | 600 | 0% | 98 | +1.03% | 0 |  | 969 | +1.79% |
| Load/int_arith | 86 | -7.53% | 62 | -7.46% | 0 |  | 1,994 | -12.31% |
| Render/int_arith | 12 | 0% | 4 | 0% | 0 |  | 764 | +3.80% |
| Module/keep | 1 | 0% | 2 | 0% | 0 |  | 448 | +9.27% |
| Module/keep_written | 1,128 | +0.18% | 562 | 0% | 0 |  | 759 | +2.99% |
| Load/large_static | 1,933 | -0.10% | 908 | -0.55% | 0 |  | 317 | -6.21% |
| Render/large_static | 714 | 0% | 571 | 0% | 0 |  | 7 | 0% |
| RenderSink/large_static | 0 |  | 0 |  | 0 |  | 4 | +33.33% |
| RenderTo/large_static | 691 | +0.14% | 567 | 0% | 0 |  | 7 | +40.00% |
| Load/macros | 81 | -4.71% | 72 | -4.00% | 0 |  | 1,722 | -3.48% |
| Render/macros | 765 | 0% | 210 | -0.94% | 0 |  | 1,086 | +8.60% |
| Load/many_tags | 28,037 | -1.70% | 22,670 | -0.69% | 0 |  | 230,383 | -9.34% |
| Render/many_tags | 6,243 | +0.03% | 870 | 0% | 0 |  | 200 | +16.28% |
| Load/mitsuhiko_table | 40 | 0% | 52 | 0% | 0 |  | 889 | -3.47% |
| Render/mitsuhiko_table | 10,050 | 0% | 6,276 | 0% | 0 |  | 360 | -4.76% |
| RenderSink/mitsuhiko_table | 9,990 | -0.01% | 1,186 | 0% | 0 |  | 297 | -1.66% |
| RenderTo/mitsuhiko_table | 10,078 | -0.02% | 6,281 | -0.05% | 0 |  | 332 | -5.41% |
| Load/mitsuhiko_table_wide | 74 | +1.37% | 95 | -1.04% | 0 |  | 982 | -8.82% |
| Render/mitsuhiko_table_wide | 10,238 | -0.01% | 22,698 | 0% | 0 |  | 367 | +3.09% |
| RenderSink/mitsuhiko_table_wide | 10,073 | -0.01% | 1,310 | -0.15% | 0 |  | 281 | +1.81% |
| RenderTo/mitsuhiko_table_wide | 10,246 | +0.02% | 22,699 | -0.01% | 0 |  | 333 | +1.22% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 6 | -62.50% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 6 | 0% |
| RenderSink/plain_text | 0 |  | 0 |  | 0 |  | 5 | 0% |
| RenderTo/plain_text | 0 |  | 0 |  | 0 |  | 5 | -16.67% |
| Load/prompt_budget | 83 | -2.35% | 54 | +1.89% | 0 |  | 1,392 | -2.32% |
| Render/prompt_budget | 4,814 | +0.04% | 525 | 0% | 0 |  | 930 | +7.76% |
| Load/strings | 138 | +0.73% | 122 | -3.17% | 0 |  | 2,707 | -7.20% |
| Render/strings | 435 | -0.46% | 484 | 0% | 0 |  | 1,566 | -3.21% |
| Load/substitute | 2 | 0% | 3 | 0% | 0 |  | 167 | -22.69% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 5 | -16.67% |
| RenderSink/substitute | 0 |  | 0 |  | 0 |  | 4 | 0% |
| RenderTo/substitute | 0 |  | 0 |  | 0 |  | 5 | +25.00% |

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
