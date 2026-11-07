# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (190 so far, latest [2eb45cc](https://github.com/jinja2cpp/Jinja2Cpp/commit/2eb45cc2d270e1672757f9454400d27246ba81cd) on 2026-11-07). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 345,273 | 0% |  | 55 | 0% |  | 18,480 | 0% | -52.71% |
| [Render/chat_llama](#renderchat_llama) | 295,053 | +0.09% |  | 225 | 0% |  | 11,632 | 0% | -19.00% |
| [Load/chat_mistral](#loadchat_mistral) | 393,925 | +0.03% |  | 52 | 0% |  | 20,888 | 0% | -53.22% |
| [Render/chat_mistral](#renderchat_mistral) | 500,213 | +0.20% |  | 402 | 0% |  | 9,968 | 0% | -24.53% |
| [Load/chat_qwen](#loadchat_qwen) | 266,620 | +0.03% |  | 44 | 0% |  | 14,416 | 0% | -47.25% |
| [Render/chat_qwen](#renderchat_qwen) | 277,203 | +0.09% |  | 212 | 0% |  | 8,304 | 0% | -2.35% |
| [Load/config_file](#loadconfig_file) | 97,219 | +0.04% |  | 25 | 0% |  | 5,464 | 0% | -48.26% |
| [Render/config_file](#renderconfig_file) | 1,274,076 | +0.43% |  | 908 | 0% |  | 24,440 | 0% | +2.65% |
| [Load/dict_ops](#loaddict_ops) | 33,332 | +0.05% | -77.44% | 9 | 0% | -93.13% | 2,176 | 0% | -48.87% |
| [Render/dict_ops](#renderdict_ops) | 293,031 | -0.04% | -57.58% | 112 | 0% | -75.11% | 24,408 | 0% | -29.65% |
| [Load/expressions](#loadexpressions) | 45,045 | +0.04% | -78.15% | 16 | 0% | -89.04% | 2,728 | 0% | -55.54% |
| [Render/expressions](#renderexpressions) | 426,200 | 0% | -52.97% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 69,205 | +0.02% | -74.78% | 27 | 0% | -88.16% | 3,848 | 0% | -66.29% |
| [Render/filters](#renderfilters) | 30,525 | +0.33% | -65.57% | 26 | 0% | -64.86% | 1,080 | 0% | -78.01% |
| [Load/for_filter_if](#loadfor_filter_if) | 30,027 | +0.08% | -77.82% | 7 | 0% | -94.31% | 1,904 | 0% | -44.78% |
| [Render/for_filter_if](#renderfor_filter_if) | 274,078 | 0% | -68.33% | 4 | 0% | -98.23% | 16,560 | 0% | +354.95% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 40,391 | +0.24% | -77.64% | 10 | 0% | -93.63% | 2,368 | 0% | -47.61% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 180,543 | 0% | -83.17% | 2 | 0% | -99.62% | 1,568 | 0% | -43.35% |
| [Load/for_range](#loadfor_range) | 13,657 | +0.12% | -75.91% | 7 | 0% | -87.50% | 1,088 | 0% | -45.60% |
| [Render/for_range](#renderfor_range) | 38,354 | +0.01% | -58.41% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/hf_deepseek_v31](#loadhf_deepseek_v31) | 388,233 | +0.01% |  | 55 | 0% |  | 21,976 | 0% | +0.73% |
| [Render/hf_deepseek_v31](#renderhf_deepseek_v31) | 90,951 | +0.49% |  | 43 | 0% |  | 2,480 | 0% | 0% |
| [Load/hf_gpt_oss](#loadhf_gpt_oss) | 1,549,668 | +0.02% |  | 287 | 0% |  | 74,640 | 0% | +0.56% |
| [Render/hf_gpt_oss](#renderhf_gpt_oss) | 170,929 | +0.17% |  | 155 | 0% |  | 5,096 | 0% | +3.24% |
| [Load/hf_mistral_small](#loadhf_mistral_small) | 599,090 | +0.02% |  | 88 | 0% |  | 33,048 | 0% | +0.34% |
| [Render/hf_mistral_small](#renderhf_mistral_small) | 145,109 | +0.28% |  | 105 | 0% |  | 6,120 | 0% | -0.26% |
| [Load/hf_qwen3](#loadhf_qwen3) | 486,003 | +0.04% |  | 82 | 0% |  | 25,560 | 0% | +0.31% |
| [Render/hf_qwen3](#renderhf_qwen3) | 119,996 | +0.10% |  | 100 | 0% |  | 3,248 | 0% | 0% |
| [Load/html_autoescape](#loadhtml_autoescape) | 193,981 | +0.08% |  | 43 | 0% |  | 12,640 | 0% | -48.78% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,209,427 | +0.04% |  | 674 | 0% |  | 45,408 | 0% | +12.44% |
| [Load/inheritance](#loadinheritance) | 23,065 | +0.04% | -79.79% | 7 | 0% | -93.46% | 1,344 | 0% | -53.20% |
| [Render/inheritance](#renderinheritance) | 137,589 | -0.01% | -81.60% | 35 | 0% | -94.37% | 7,384 | 0% | +22.74% |
| [Load/int_arith](#loadint_arith) | 66,006 | +0.05% |  | 20 | 0% |  | 3,872 | 0% | +0.41% |
| [Render/int_arith](#renderint_arith) | 825,803 | +0.18% |  | 12 | 0% |  | 3,272 | 0% | -0.49% |
| [Module/keep](#modulekeep) | 6,996 | 0% |  | 7 | 0% |  |  |  |  |
| [Module/keep_written](#modulekeep_written) | 36,407 | +0.01% |  | 11 | 0% |  |  |  |  |
| [Load/large_static](#loadlarge_static) | 154,719 | +0.01% | -82.54% | 14 | 0% | -96.82% | 43,120 | 0% | -19.23% |
| [Render/large_static](#renderlarge_static) | 20,711 | +0.01% | -72.44% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [RenderSink/large_static](#rendersinklarge_static) | 12,701 | +0.02% |  | 0 |  |  | 0 |  |  |
| [RenderTo/large_static](#rendertolarge_static) | 20,274 | +0.01% |  | 0 |  |  | 0 |  |  |
| [Load/macros](#loadmacros) | 56,397 | +0.04% | -75.26% | 27 | 0% | -85.48% | 4,392 | 0% | -29.34% |
| [Render/macros](#rendermacros) | 656,539 | 0% | -74.38% | 308 | 0% | -88.71% | 20,560 | 0% | +39.67% |
| [Load/many_tags](#loadmany_tags) | 9,019,065 | +0.04% | -79.58% | 951 | 0% | -97.22% | 438,160 | 0% | -58.48% |
| [Render/many_tags](#rendermany_tags) | 728,684 | 0% | -65.02% | 2 | 0% | -90.48% | 5,648 | 0% | -10.52% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 46,587 | +0.02% | -80.39% | 19 | 0% | -89.67% | 3,360 | 0% | -44.59% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,196,675 | 0% | -57.94% | 33 | 0% | -99.19% | 381,536 | 0% | +9.95% |
| [RenderSink/mitsuhiko_table](#rendersinkmitsuhiko_table) | 5,123,868 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table](#rendertomitsuhiko_table) | 5,196,179 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 54,843 | +0.02% |  | 35 | 0% |  | 5,688 | 0% | -31.17% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,542,840 | 0% |  | 33 | 0% |  | 1,414,928 | 0% | +2.46% |
| [RenderSink/mitsuhiko_table_wide](#rendersinkmitsuhiko_table_wide) | 5,366,129 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table_wide](#rendertomitsuhiko_table_wide) | 5,542,392 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/plain_text](#loadplain_text) | 3,730 | +0.27% | -61.45% | 4 | 0% | -77.78% | 672 | 0% | -40.00% |
| [Render/plain_text](#renderplain_text) | 1,021 | +0.20% | -84.97% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/plain_text](#rendersinkplain_text) | 723 | +0.28% |  | 0 |  |  | 0 |  |  |
| [RenderTo/plain_text](#rendertoplain_text) | 822 | +0.24% |  | 0 |  |  | 0 |  |  |
| [Load/prompt_budget](#loadprompt_budget) | 35,111 | -0.05% |  | 18 | 0% |  | 2,224 | 0% | 0% |
| [Render/prompt_budget](#renderprompt_budget) | 1,316,447 | +0.15% |  | 76 | 0% |  | 36,416 | 0% | 0% |
| [Load/strings](#loadstrings) | 70,485 | +0.03% | -75.85% | 30 | 0% | -86.61% | 4,344 | 0% | -51.95% |
| [Render/strings](#renderstrings) | 580,254 | +0.22% | -72.34% | 344 | 0% | -83.73% | 9,088 | 0% | -49.31% |
| [Load/substitute](#loadsubstitute) | 7,763 | +0.13% | -74.00% | 6 | 0% | -83.78% | 832 | 0% | -45.26% |
| [Render/substitute](#rendersubstitute) | 2,254 | +0.09% | -74.43% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/substitute](#rendersinksubstitute) | 1,960 | +0.10% |  | 0 |  |  | 0 |  |  |
| [RenderTo/substitute](#rendertosubstitute) | 2,059 | +0.10% |  | 0 |  |  | 0 |  |  |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 731 | -0.41% | 752 | 0% | 0 |  | 7,270 | +1.89% |
| Render/chat_llama | 1,163 | +0.61% | 530 | +0.38% | 0 |  | 4,338 | +3.29% |
| Load/chat_mistral | 1,049 | +0.48% | 1,047 | +0.19% | 0 |  | 8,066 | +0.56% |
| Render/chat_mistral | 2,017 | +0.40% | 574 | -0.17% | 0 |  | 10,666 | +14.05% |
| Load/chat_qwen | 483 | -2.03% | 596 | -0.17% | 0 |  | 4,225 | +3.35% |
| Render/chat_qwen | 1,070 | -0.83% | 401 | -0.25% | 0 |  | 3,556 | +5.99% |
| Load/config_file | 197 | +1.55% | 168 | -1.75% | 0 |  | 2,616 | +3.81% |
| Render/config_file | 6,603 | +3.30% | 1,571 | +1.42% | 0 |  | 40,731 | +0.14% |
| Load/dict_ops | 32 | +14.29% | 21 | 0% | 0 |  | 1,300 | -0.69% |
| Render/dict_ops | 664 | +0.45% | 421 | 0% | 0 |  | 882 | +1.61% |
| Load/expressions | 12 | +33.33% | 7 | +40.00% | 0 |  | 1,312 | +2.66% |
| Render/expressions | 15 | +25.00% | 7 | +16.67% | 0 |  | 297 | -6.90% |
| Load/filters | 70 | -6.67% | 48 | +2.13% | 0 |  | 1,580 | -5.11% |
| Render/filters | 23 | -4.17% | 5 | 0% | 0 |  | 775 | -1.52% |
| Load/for_filter_if | 22 | 0% | 16 | 0% | 0 |  | 1,037 | -1.52% |
| Render/for_filter_if | 1,804 | -0.11% | 97 | 0% | 0 |  | 168 | -4.00% |
| Load/for_loop_vars | 30 | -3.23% | 16 | 0% | 0 |  | 1,068 | +3.19% |
| Render/for_loop_vars | 9 | +12.50% | 3 | 0% | 0 |  | 174 | +14.47% |
| Load/for_range | 3 | -25.00% | 3 | 0% | 0 |  | 749 | -0.53% |
| Render/for_range | 1 | 0% | 0 |  | 0 |  | 125 | 0% |
| Load/hf_deepseek_v31 | 980 | -0.41% | 973 | -0.10% | 0 |  | 5,869 | -1.56% |
| Render/hf_deepseek_v31 | 347 | -2.25% | 83 | +2.47% | 0 |  | 1,453 | +6.45% |
| Load/hf_gpt_oss | 4,603 | +0.02% | 3,291 | -0.09% | 0 |  | 20,386 | +0.11% |
| Render/hf_gpt_oss | 1,209 | +0.17% | 378 | +0.80% | 0 |  | 4,483 | +0.67% |
| Load/hf_mistral_small | 1,821 | +0.11% | 1,588 | +0.32% | 0 |  | 10,425 | +1.58% |
| Render/hf_mistral_small | 956 | -0.62% | 319 | +1.59% | 0 |  | 3,938 | +3.88% |
| Load/hf_qwen3 | 1,405 | -0.71% | 1,243 | +0.08% | 0 |  | 8,345 | +2.30% |
| Render/hf_qwen3 | 661 | -0.75% | 221 | +1.84% | 0 |  | 2,615 | +2.23% |
| Load/html_autoescape | 441 | -1.78% | 520 | +0.39% | 0 |  | 3,609 | +0.06% |
| Render/html_autoescape | 3,882 | -8.23% | 1,194 | -2.93% | 0 |  | 35,234 | -3.24% |
| Load/inheritance | 2 | 0% | 3 | 0% | 0 |  | 674 | -4.94% |
| Render/inheritance | 590 | +0.68% | 97 | +1.04% | 0 |  | 905 | -1.31% |
| Load/int_arith | 63 | -8.70% | 40 | +5.26% | 0 |  | 2,006 | +1.52% |
| Render/int_arith | 30 | +11.11% | 17 | +13.33% | 0 |  | 741 | -2.88% |
| Module/keep | 2 | 0% | 2 | 0% | 0 |  | 421 | +9.07% |
| Module/keep_written | 2,931 | 0% | 561 | 0% | 0 |  | 715 | -0.56% |
| Load/large_static | 1,927 | 0% | 913 | 0% | 0 |  | 305 | +7.02% |
| Render/large_static | 708 | -0.28% | 569 | +0.18% | 0 |  | 4 | -20.00% |
| RenderSink/large_static | 0 |  | 0 |  | 0 |  | 2 | -33.33% |
| RenderTo/large_static | 686 | -0.29% | 571 | +0.18% | 0 |  | 3 | -25.00% |
| Load/macros | 70 | +1.45% | 52 | +4.00% | 0 |  | 1,693 | -0.35% |
| Render/macros | 761 | +0.53% | 214 | +0.47% | 0 |  | 2,851 | +111.81% |
| Load/many_tags | 27,697 | -0.03% | 22,360 | -0.06% | 0 |  | 220,888 | +0.49% |
| Render/many_tags | 6,224 | +0.08% | 863 | +0.12% | 0 |  | 137 | +21.24% |
| Load/mitsuhiko_table | 27 | 0% | 29 | 0% | 0 |  | 818 | -3.31% |
| Render/mitsuhiko_table | 10,019 | 0% | 6,277 | -0.05% | 0 |  | 379 | +6.76% |
| RenderSink/mitsuhiko_table | 9,954 | 0% | 1,182 | -0.08% | 0 |  | 309 | +10.36% |
| RenderTo/mitsuhiko_table | 10,024 | -0.02% | 6,285 | +0.08% | 0 |  | 354 | +10.97% |
| Load/mitsuhiko_table_wide | 69 | -2.82% | 65 | -8.45% | 0 |  | 984 | +3.69% |
| Render/mitsuhiko_table_wide | 10,238 | 0% | 22,672 | -0.02% | 0 |  | 349 | +2.95% |
| RenderSink/mitsuhiko_table_wide | 10,057 | -0.05% | 1,292 | +0.23% | 0 |  | 298 | +5.67% |
| RenderTo/mitsuhiko_table_wide | 10,229 | +0.02% | 22,676 | +0.01% | 0 |  | 331 | +4.75% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 32 | +10.34% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 6 | -25.00% |
| RenderSink/plain_text | 0 |  | 0 |  | 0 |  | 4 | 0% |
| RenderTo/plain_text | 0 |  | 0 |  | 0 |  | 5 | -16.67% |
| Load/prompt_budget | 62 | +1.64% | 38 | -2.56% | 0 |  | 1,371 | -0.65% |
| Render/prompt_budget | 4,787 | +0.02% | 526 | 0% | 0 |  | 1,217 | +22.19% |
| Load/strings | 113 | +4.63% | 77 | +1.32% | 0 |  | 2,702 | -1.17% |
| Render/strings | 412 | +0.73% | 474 | -0.63% | 0 |  | 1,576 | +3.41% |
| Load/substitute | 1 | 0% | 2 | 0% | 0 |  | 204 | +10.87% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 2 | -50.00% |
| RenderSink/substitute | 0 |  | 0 |  | 0 |  | 2 | -33.33% |
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
