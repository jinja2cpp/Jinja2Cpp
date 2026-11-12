# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (213 so far, latest [8d8abfa](https://github.com/jinja2cpp/Jinja2Cpp/commit/8d8abfaa378a1219e4435619a2f924bdc95600cb) on 2026-11-12). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 336,146 | -0.02% |  | 55 | 0% |  | 18,464 | 0% | -52.75% |
| [Render/chat_llama](#renderchat_llama) | 295,701 | +0.40% |  | 225 | 0% |  | 11,616 | 0% | -19.11% |
| [Load/chat_mistral](#loadchat_mistral) | 386,335 | 0% |  | 52 | 0% |  | 20,888 | 0% | -53.22% |
| [Render/chat_mistral](#renderchat_mistral) | 501,560 | +0.46% |  | 402 | 0% |  | 10,000 | 0% | -24.29% |
| [Load/chat_qwen](#loadchat_qwen) | 262,171 | -0.04% |  | 44 | 0% |  | 14,416 | 0% | -47.25% |
| [Render/chat_qwen](#renderchat_qwen) | 277,933 | +0.42% |  | 212 | 0% |  | 8,304 | 0% | -2.35% |
| [Load/config_file](#loadconfig_file) | 95,074 | 0% |  | 25 | 0% |  | 5,464 | 0% | -48.26% |
| [Render/config_file](#renderconfig_file) | 1,275,264 | +0.77% |  | 908 | 0% |  | 24,440 | 0% | +2.65% |
| [Load/dict_ops](#loaddict_ops) | 32,377 | +0.01% | -78.09% | 9 | 0% | -93.13% | 2,176 | 0% | -48.87% |
| [Render/dict_ops](#renderdict_ops) | 295,022 | +0.43% | -57.30% | 112 | 0% | -75.11% | 24,408 | 0% | -29.65% |
| [Load/expressions](#loadexpressions) | 44,207 | 0% | -78.56% | 16 | 0% | -89.04% | 2,728 | 0% | -55.54% |
| [Render/expressions](#renderexpressions) | 426,926 | 0% | -52.89% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 67,969 | +0.11% | -75.23% | 27 | 0% | -88.16% | 3,848 | 0% | -66.29% |
| [Render/filters](#renderfilters) | 27,629 | +0.35% | -68.84% | 23 | 0% | -68.92% | 752 | 0% | -84.69% |
| [Load/for_filter_if](#loadfor_filter_if) | 29,276 | +0.01% | -78.38% | 7 | 0% | -94.31% | 1,904 | 0% | -44.78% |
| [Render/for_filter_if](#renderfor_filter_if) | 274,390 | +0.01% | -68.30% | 4 | 0% | -98.23% | 16,560 | 0% | +354.95% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 39,185 | +0.03% | -78.31% | 10 | 0% | -93.63% | 2,368 | 0% | -47.61% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 180,568 | +0.01% | -83.17% | 2 | 0% | -99.62% | 1,568 | 0% | -43.35% |
| [Load/for_range](#loadfor_range) | 13,252 | +0.02% | -76.62% | 7 | 0% | -87.50% | 1,088 | 0% | -45.60% |
| [Render/for_range](#renderfor_range) | 38,377 | +0.05% | -58.38% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/hf_deepseek_v31](#loadhf_deepseek_v31) | 383,719 | 0% |  | 55 | 0% |  | 21,976 | 0% | +0.73% |
| [Render/hf_deepseek_v31](#renderhf_deepseek_v31) | 92,506 | +0.62% |  | 43 | 0% |  | 2,480 | 0% | 0% |
| [Load/hf_gpt_oss](#loadhf_gpt_oss) | 1,527,508 | 0% |  | 287 | 0% |  | 74,704 | 0% | +0.65% |
| [Render/hf_gpt_oss](#renderhf_gpt_oss) | 172,834 | +0.91% |  | 155 | 0% |  | 5,096 | 0% | +3.24% |
| [Load/hf_mistral_small](#loadhf_mistral_small) | 592,583 | 0% |  | 88 | 0% |  | 33,048 | 0% | +0.34% |
| [Render/hf_mistral_small](#renderhf_mistral_small) | 142,483 | +0.71% |  | 99 | 0% |  | 6,120 | 0% | -0.26% |
| [Load/hf_qwen3](#loadhf_qwen3) | 477,855 | -0.05% |  | 82 | 0% |  | 25,560 | 0% | +0.31% |
| [Render/hf_qwen3](#renderhf_qwen3) | 120,814 | +0.89% |  | 100 | 0% |  | 3,248 | 0% | 0% |
| [Load/html_autoescape](#loadhtml_autoescape) | 190,736 | +0.03% |  | 43 | 0% |  | 12,672 | 0% | -48.65% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,208,602 | +0.02% |  | 674 | 0% |  | 45,408 | 0% | +12.44% |
| [Load/inheritance](#loadinheritance) | 21,973 | +0.01% | -80.75% | 7 | 0% | -93.46% | 1,344 | 0% | -53.20% |
| [Render/inheritance](#renderinheritance) | 137,587 | +0.09% | -81.60% | 35 | 0% | -94.37% | 7,384 | 0% | +22.74% |
| [Load/int_arith](#loadint_arith) | 65,216 | +0.02% |  | 20 | 0% |  | 3,872 | 0% | +0.41% |
| [Render/int_arith](#renderint_arith) | 827,272 | +0.01% |  | 12 | 0% |  | 3,272 | 0% | -0.49% |
| [Module/keep](#modulekeep) | 7,109 | 0% |  | 7 | 0% |  |  |  |  |
| [Module/keep_written](#modulekeep_written) | 29,027 | +0.10% |  | 11 | 0% |  |  |  |  |
| [Load/large_static](#loadlarge_static) | 149,413 | 0% | -83.14% | 14 | 0% | -96.82% | 43,120 | 0% | -19.23% |
| [Render/large_static](#renderlarge_static) | 20,726 | +0.02% | -72.42% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [RenderSink/large_static](#rendersinklarge_static) | 12,716 | +0.03% |  | 0 |  |  | 0 |  |  |
| [RenderTo/large_static](#rendertolarge_static) | 20,289 | +0.02% |  | 0 |  |  | 0 |  |  |
| [Load/macros](#loadmacros) | 55,656 | +0.06% | -75.58% | 27 | 0% | -85.48% | 4,392 | 0% | -29.34% |
| [Render/macros](#rendermacros) | 659,613 | +0.01% | -74.26% | 308 | 0% | -88.71% | 20,560 | 0% | +39.67% |
| [Load/many_tags](#loadmany_tags) | 8,791,793 | +0.03% | -80.09% | 951 | 0% | -97.22% | 438,112 | 0% | -58.48% |
| [Render/many_tags](#rendermany_tags) | 729,904 | 0% | -64.96% | 2 | 0% | -90.48% | 5,648 | 0% | -10.52% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 45,393 | 0% | -80.89% | 19 | 0% | -89.67% | 3,360 | 0% | -44.59% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,202,846 | 0% | -57.89% | 33 | 0% | -99.19% | 381,536 | 0% | +9.95% |
| [RenderSink/mitsuhiko_table](#rendersinkmitsuhiko_table) | 5,130,043 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table](#rendertomitsuhiko_table) | 5,202,352 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 53,134 | 0% |  | 35 | 0% |  | 5,688 | 0% | -31.17% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,549,003 | 0% |  | 33 | 0% |  | 1,414,928 | 0% | +2.46% |
| [RenderSink/mitsuhiko_table_wide](#rendersinkmitsuhiko_table_wide) | 5,372,308 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table_wide](#rendertomitsuhiko_table_wide) | 5,548,571 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/plain_text](#loadplain_text) | 3,717 | +0.11% | -61.58% | 4 | 0% | -77.78% | 672 | 0% | -40.00% |
| [Render/plain_text](#renderplain_text) | 1,036 | +0.39% | -84.75% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/plain_text](#rendersinkplain_text) | 738 | +0.55% |  | 0 |  |  | 0 |  |  |
| [RenderTo/plain_text](#rendertoplain_text) | 837 | +0.48% |  | 0 |  |  | 0 |  |  |
| [Load/prompt_budget](#loadprompt_budget) | 34,583 | -0.03% |  | 18 | 0% |  | 2,224 | 0% | 0% |
| [Render/prompt_budget](#renderprompt_budget) | 1,319,453 | -0.01% |  | 76 | 0% |  | 36,416 | 0% | 0% |
| [Load/strings](#loadstrings) | 69,436 | +0.06% | -76.21% | 30 | 0% | -86.61% | 4,344 | 0% | -51.95% |
| [Render/strings](#renderstrings) | 582,809 | +0.01% | -72.21% | 344 | 0% | -83.73% | 9,088 | 0% | -49.31% |
| [Load/substitute](#loadsubstitute) | 7,514 | +0.03% | -74.83% | 6 | 0% | -83.78% | 832 | 0% | -45.26% |
| [Render/substitute](#rendersubstitute) | 2,269 | +0.18% | -74.26% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/substitute](#rendersinksubstitute) | 1,975 | +0.20% |  | 0 |  |  | 0 |  |  |
| [RenderTo/substitute](#rendertosubstitute) | 2,074 | +0.19% |  | 0 |  |  | 0 |  |  |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 738 | -0.14% | 742 | +0.13% | 0 |  | 7,839 | -2.72% |
| Render/chat_llama | 1,147 | 0% | 528 | +0.19% | 0 |  | 4,176 | +3.49% |
| Load/chat_mistral | 1,066 | -0.74% | 1,069 | -0.28% | 0 |  | 8,840 | -3.29% |
| Render/chat_mistral | 1,989 | +1.22% | 588 | +1.38% | 0 |  | 10,325 | +6.05% |
| Load/chat_qwen | 612 | -0.81% | 624 | -0.32% | 0 |  | 4,619 | -5.70% |
| Render/chat_qwen | 1,133 | +2.07% | 400 | +1.01% | 0 |  | 3,430 | +4.61% |
| Load/config_file | 225 | +0.45% | 209 | +0.97% | 0 |  | 2,695 | -3.82% |
| Render/config_file | 6,507 | +2.25% | 1,528 | +4.16% | 0 |  | 43,947 | +6.29% |
| Load/dict_ops | 58 | -3.33% | 30 | 0% | 0 |  | 1,372 | -3.92% |
| Render/dict_ops | 687 | +1.03% | 436 | +0.23% | 0 |  | 897 | +1.01% |
| Load/expressions | 24 | -4.00% | 19 | +5.56% | 0 |  | 1,356 | -7.19% |
| Render/expressions | 15 | 0% | 5 | 0% | 0 |  | 320 | +5.26% |
| Load/filters | 103 | +9.57% | 53 | +8.16% | 0 |  | 1,692 | -9.52% |
| Render/filters | 17 | -19.05% | 4 | -20.00% | 0 |  | 800 | +2.83% |
| Load/for_filter_if | 35 | +6.06% | 15 | +7.14% | 0 |  | 1,092 | +0.92% |
| Render/for_filter_if | 1,724 | -0.06% | 138 | +1.47% | 0 |  | 158 | -14.59% |
| Load/for_loop_vars | 50 | +13.64% | 29 | +11.54% | 0 |  | 1,123 | -0.27% |
| Render/for_loop_vars | 10 | 0% | 0 |  | 0 |  | 138 | -8.61% |
| Load/for_range | 6 | 0% | 6 | +20.00% | 0 |  | 783 | -3.57% |
| Render/for_range | 9 | 0% | 0 |  | 0 |  | 144 | +11.63% |
| Load/hf_deepseek_v31 | 989 | +0.10% | 978 | -0.51% | 0 |  | 6,440 | -6.01% |
| Render/hf_deepseek_v31 | 357 | +0.56% | 76 | +4.11% | 0 |  | 1,457 | +3.33% |
| Load/hf_gpt_oss | 4,693 | +0.02% | 3,355 | -0.12% | 0 |  | 22,336 | -5.89% |
| Render/hf_gpt_oss | 1,228 | +0.57% | 386 | 0% | 0 |  | 4,562 | +0.20% |
| Load/hf_mistral_small | 1,916 | +0.79% | 1,612 | -0.31% | 0 |  | 11,394 | -2.82% |
| Render/hf_mistral_small | 959 | 0% | 299 | +0.34% | 0 |  | 3,685 | +1.29% |
| Load/hf_qwen3 | 1,512 | -0.85% | 1,317 | -0.45% | 0 |  | 8,923 | -4.11% |
| Render/hf_qwen3 | 675 | +0.45% | 215 | -2.27% | 0 |  | 2,648 | +3.72% |
| Load/html_autoescape | 513 | +1.99% | 537 | -0.56% | 0 |  | 3,893 | -5.74% |
| Render/html_autoescape | 4,468 | -6.92% | 1,281 | -5.04% | 0 |  | 36,503 | +2.66% |
| Load/inheritance | 6 | 0% | 3 | +50.00% | 0 |  | 694 | -3.21% |
| Render/inheritance | 605 | +0.67% | 105 | -1.87% | 0 |  | 1,523 | +61.51% |
| Load/int_arith | 83 | -1.19% | 64 | +1.59% | 0 |  | 2,084 | -5.53% |
| Render/int_arith | 21 | 0% | 6 | 0% | 0 |  | 768 | +3.09% |
| Module/keep | 1 | 0% | 2 | 0% | 0 |  | 424 | +3.41% |
| Module/keep_written | 1,131 | +0.35% | 560 | -0.53% | 0 |  | 738 | +2.50% |
| Load/large_static | 1,926 | +0.05% | 909 | +0.22% | 0 |  | 307 | -5.25% |
| Render/large_static | 708 | +0.14% | 569 | 0% | 0 |  | 4 | -33.33% |
| RenderSink/large_static | 0 |  | 0 |  | 0 |  | 2 | -50.00% |
| RenderTo/large_static | 685 | 0% | 570 | 0% | 0 |  | 4 | -20.00% |
| Load/macros | 104 | +6.12% | 73 | +8.96% | 0 |  | 1,697 | -4.34% |
| Render/macros | 752 | +0.40% | 235 | -0.84% | 0 |  | 913 | +14.13% |
| Load/many_tags | 28,506 | -0.26% | 22,764 | +0.04% | 0 |  | 243,027 | -3.36% |
| Render/many_tags | 6,249 | +0.08% | 883 | +0.11% | 0 |  | 167 | +24.63% |
| Load/mitsuhiko_table | 36 | +5.88% | 46 | +6.98% | 0 |  | 896 | -2.82% |
| Render/mitsuhiko_table | 10,066 | +0.07% | 6,270 | +0.06% | 0 |  | 366 | -0.54% |
| RenderSink/mitsuhiko_table | 9,969 | +0.09% | 1,188 | +0.42% | 0 |  | 291 | +1.04% |
| RenderTo/mitsuhiko_table | 10,063 | +0.12% | 6,272 | -0.03% | 0 |  | 336 | 0% |
| Load/mitsuhiko_table_wide | 81 | +1.25% | 83 | 0% | 0 |  | 1,052 | -0.19% |
| Render/mitsuhiko_table_wide | 10,267 | +0.10% | 22,674 | +0.03% | 0 |  | 368 | -2.90% |
| RenderSink/mitsuhiko_table_wide | 10,078 | +0.12% | 1,301 | +0.15% | 0 |  | 277 | -5.78% |
| RenderTo/mitsuhiko_table_wide | 10,532 | +0.10% | 22,689 | +0.01% | 0 |  | 330 | -0.90% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 29 | +93.33% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 6 | 0% |
| RenderSink/plain_text | 0 |  | 0 |  | 0 |  | 4 | -20.00% |
| RenderTo/plain_text | 0 |  | 0 |  | 0 |  | 5 | -28.57% |
| Load/prompt_budget | 75 | 0% | 41 | -10.87% | 0 |  | 1,425 | -2.73% |
| Render/prompt_budget | 4,809 | +0.06% | 549 | +0.55% | 0 |  | 1,130 | -4.32% |
| Load/strings | 158 | -0.63% | 124 | -1.59% | 0 |  | 2,783 | -1.76% |
| Render/strings | 437 | +1.63% | 488 | 0% | 0 |  | 1,573 | -0.13% |
| Load/substitute | 2 | 0% | 3 | 0% | 0 |  | 131 | -25.57% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 4 | -33.33% |
| RenderSink/substitute | 0 |  | 0 |  | 0 |  | 3 | -25.00% |
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
