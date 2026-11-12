# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (210 so far, latest [bdd93a1](https://github.com/jinja2cpp/Jinja2Cpp/commit/bdd93a1e7179940cb7d2714180c619075ca110a7) on 2026-11-12). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 336,214 | 0% |  | 55 | 0% |  | 18,464 | 0% | -52.75% |
| [Render/chat_llama](#renderchat_llama) | 294,511 | +0.10% |  | 225 | 0% |  | 11,616 | 0% | -19.11% |
| [Load/chat_mistral](#loadchat_mistral) | 386,339 | 0% |  | 52 | 0% |  | 20,888 | 0% | -53.22% |
| [Render/chat_mistral](#renderchat_mistral) | 499,243 | -0.10% |  | 402 | 0% |  | 10,000 | 0% | -24.29% |
| [Load/chat_qwen](#loadchat_qwen) | 262,260 | 0% |  | 44 | 0% |  | 14,416 | 0% | -47.25% |
| [Render/chat_qwen](#renderchat_qwen) | 276,782 | +0.14% |  | 212 | 0% |  | 8,304 | 0% | -2.35% |
| [Load/config_file](#loadconfig_file) | 95,093 | -0.01% |  | 25 | 0% |  | 5,464 | 0% | -48.26% |
| [Render/config_file](#renderconfig_file) | 1,265,574 | -0.87% |  | 908 | 0% |  | 24,440 | 0% | +2.65% |
| [Load/dict_ops](#loaddict_ops) | 32,382 | +0.02% | -78.09% | 9 | 0% | -93.13% | 2,176 | 0% | -48.87% |
| [Render/dict_ops](#renderdict_ops) | 293,745 | +0.13% | -57.48% | 112 | 0% | -75.11% | 24,408 | 0% | -29.65% |
| [Load/expressions](#loadexpressions) | 44,205 | 0% | -78.56% | 16 | 0% | -89.04% | 2,728 | 0% | -55.54% |
| [Render/expressions](#renderexpressions) | 426,906 | 0% | -52.89% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 67,907 | -0.07% | -75.26% | 27 | 0% | -88.16% | 3,848 | 0% | -66.29% |
| [Render/filters](#renderfilters) | 27,532 | +0.43% | -68.95% | 23 | 0% | -68.92% | 752 | 0% | -84.69% |
| [Load/for_filter_if](#loadfor_filter_if) | 29,274 | 0% | -78.38% | 7 | 0% | -94.31% | 1,904 | 0% | -44.78% |
| [Render/for_filter_if](#renderfor_filter_if) | 274,354 | 0% | -68.30% | 4 | 0% | -98.23% | 16,560 | 0% | +354.95% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 39,161 | 0% | -78.32% | 10 | 0% | -93.63% | 2,368 | 0% | -47.61% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 180,548 | 0% | -83.17% | 2 | 0% | -99.62% | 1,568 | 0% | -43.35% |
| [Load/for_range](#loadfor_range) | 13,250 | 0% | -76.62% | 7 | 0% | -87.50% | 1,088 | 0% | -45.60% |
| [Render/for_range](#renderfor_range) | 38,357 | -0.04% | -58.40% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/hf_deepseek_v31](#loadhf_deepseek_v31) | 383,729 | 0% |  | 55 | 0% |  | 21,976 | 0% | +0.73% |
| [Render/hf_deepseek_v31](#renderhf_deepseek_v31) | 91,935 | +0.47% |  | 43 | 0% |  | 2,480 | 0% | 0% |
| [Load/hf_gpt_oss](#loadhf_gpt_oss) | 1,527,435 | -0.16% |  | 287 | 0% |  | 74,704 | 0% | +0.65% |
| [Render/hf_gpt_oss](#renderhf_gpt_oss) | 171,272 | -0.39% |  | 155 | 0% |  | 5,096 | 0% | +3.24% |
| [Load/hf_mistral_small](#loadhf_mistral_small) | 592,600 | 0% |  | 88 | 0% |  | 33,048 | 0% | +0.34% |
| [Render/hf_mistral_small](#renderhf_mistral_small) | 141,476 | -0.10% |  | 99 | 0% |  | 6,120 | 0% | -0.26% |
| [Load/hf_qwen3](#loadhf_qwen3) | 478,063 | 0% |  | 82 | 0% |  | 25,560 | 0% | +0.31% |
| [Render/hf_qwen3](#renderhf_qwen3) | 119,763 | -0.42% |  | 100 | 0% |  | 3,248 | 0% | 0% |
| [Load/html_autoescape](#loadhtml_autoescape) | 190,691 | +0.01% |  | 43 | 0% |  | 12,672 | 0% | -48.65% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,208,396 | -0.01% |  | 674 | 0% |  | 45,408 | 0% | +12.44% |
| [Load/inheritance](#loadinheritance) | 21,971 | 0% | -80.75% | 7 | 0% | -93.46% | 1,344 | 0% | -53.20% |
| [Render/inheritance](#renderinheritance) | 137,464 | -0.01% | -81.62% | 35 | 0% | -94.37% | 7,384 | 0% | +22.74% |
| [Load/int_arith](#loadint_arith) | 65,190 | 0% |  | 20 | 0% |  | 3,872 | 0% | +0.41% |
| [Render/int_arith](#renderint_arith) | 827,172 | +0.02% |  | 12 | 0% |  | 3,272 | 0% | -0.49% |
| [Module/keep](#modulekeep) | 7,109 | +0.03% |  | 7 | 0% |  |  |  |  |
| [Module/keep_written](#modulekeep_written) | 29,001 | +0.04% |  | 11 | 0% |  |  |  |  |
| [Load/large_static](#loadlarge_static) | 149,411 | 0% | -83.14% | 14 | 0% | -96.82% | 43,120 | 0% | -19.23% |
| [Render/large_static](#renderlarge_static) | 20,722 | 0% | -72.42% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [RenderSink/large_static](#rendersinklarge_static) | 12,712 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/large_static](#rendertolarge_static) | 20,285 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/macros](#loadmacros) | 55,622 | 0% | -75.60% | 27 | 0% | -85.48% | 4,392 | 0% | -29.34% |
| [Render/macros](#rendermacros) | 659,561 | -0.07% | -74.27% | 308 | 0% | -88.71% | 20,560 | 0% | +39.67% |
| [Load/many_tags](#loadmany_tags) | 8,793,891 | 0% | -80.09% | 951 | 0% | -97.22% | 438,112 | 0% | -58.48% |
| [Render/many_tags](#rendermany_tags) | 729,884 | +0.20% | -64.96% | 2 | 0% | -90.48% | 5,648 | 0% | -10.52% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 45,391 | 0% | -80.89% | 19 | 0% | -89.67% | 3,360 | 0% | -44.59% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,202,634 | 0% | -57.89% | 33 | 0% | -99.19% | 381,536 | 0% | +9.95% |
| [RenderSink/mitsuhiko_table](#rendersinkmitsuhiko_table) | 5,129,831 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table](#rendertomitsuhiko_table) | 5,202,140 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 53,132 | 0% |  | 35 | 0% |  | 5,688 | 0% | -31.17% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,548,791 | 0% |  | 33 | 0% |  | 1,414,928 | 0% | +2.46% |
| [RenderSink/mitsuhiko_table_wide](#rendersinkmitsuhiko_table_wide) | 5,372,096 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table_wide](#rendertomitsuhiko_table_wide) | 5,548,359 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/plain_text](#loadplain_text) | 3,713 | 0% | -61.63% | 4 | 0% | -77.78% | 672 | 0% | -40.00% |
| [Render/plain_text](#renderplain_text) | 1,032 | 0% | -84.81% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/plain_text](#rendersinkplain_text) | 734 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/plain_text](#rendertoplain_text) | 833 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/prompt_budget](#loadprompt_budget) | 34,595 | -0.09% |  | 18 | 0% |  | 2,224 | 0% | 0% |
| [Render/prompt_budget](#renderprompt_budget) | 1,319,521 | +0.28% |  | 76 | 0% |  | 36,416 | 0% | 0% |
| [Load/strings](#loadstrings) | 69,399 | -0.06% | -76.22% | 30 | 0% | -86.61% | 4,344 | 0% | -51.95% |
| [Render/strings](#renderstrings) | 582,765 | +0.03% | -72.22% | 344 | 0% | -83.73% | 9,088 | 0% | -49.31% |
| [Load/substitute](#loadsubstitute) | 7,512 | 0% | -74.84% | 6 | 0% | -83.78% | 832 | 0% | -45.26% |
| [Render/substitute](#rendersubstitute) | 2,265 | 0% | -74.30% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/substitute](#rendersinksubstitute) | 1,971 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/substitute](#rendertosubstitute) | 2,070 | 0% |  | 0 |  |  | 0 |  |  |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 733 | -0.68% | 741 | 0% | 0 |  | 8,062 | +1.32% |
| Render/chat_llama | 1,166 | +1.30% | 527 | +6.90% | 0 |  | 3,964 | +3.88% |
| Load/chat_mistral | 1,098 | +0.83% | 1,086 | 0% | 0 |  | 9,142 | -0.57% |
| Render/chat_mistral | 1,958 | +0.93% | 574 | +4.55% | 0 |  | 9,936 | +6.00% |
| Load/chat_qwen | 615 | 0% | 633 | +0.48% | 0 |  | 4,987 | -2.92% |
| Render/chat_qwen | 1,107 | +0.27% | 392 | +5.38% | 0 |  | 3,290 | +1.83% |
| Load/config_file | 222 | -1.77% | 208 | -0.48% | 0 |  | 2,838 | +0.42% |
| Render/config_file | 6,052 | +0.36% | 1,395 | -1.13% | 0 |  | 39,968 | -4.06% |
| Load/dict_ops | 61 | +5.17% | 29 | -6.45% | 0 |  | 1,429 | +0.70% |
| Render/dict_ops | 672 | -0.59% | 431 | -0.46% | 0 |  | 896 | +4.67% |
| Load/expressions | 22 | +10.00% | 17 | +6.25% | 0 |  | 1,460 | +6.65% |
| Render/expressions | 14 | +7.69% | 5 | 0% | 0 |  | 287 | -4.65% |
| Load/filters | 98 | +4.26% | 48 | -2.04% | 0 |  | 1,872 | +6.73% |
| Render/filters | 16 | -20.00% | 5 | -16.67% | 0 |  | 780 | +0.52% |
| Load/for_filter_if | 29 | -6.45% | 12 | -7.69% | 0 |  | 1,119 | -1.41% |
| Render/for_filter_if | 1,731 | +0.12% | 133 | 0% | 0 |  | 146 | -17.98% |
| Load/for_loop_vars | 40 | +2.56% | 25 | +4.17% | 0 |  | 1,160 | -1.36% |
| Render/for_loop_vars | 8 | -20.00% | 0 |  | 0 |  | 170 | +33.86% |
| Load/for_range | 6 | 0% | 5 | 0% | 0 |  | 811 | +4.65% |
| Render/for_range | 6 | +20.00% | 0 |  | 0 |  | 118 | -4.07% |
| Load/hf_deepseek_v31 | 988 | -0.90% | 983 | 0% | 0 |  | 6,805 | -3.64% |
| Render/hf_deepseek_v31 | 367 | -1.34% | 77 | -1.28% | 0 |  | 1,438 | +7.47% |
| Load/hf_gpt_oss | 4,696 | -0.11% | 3,362 | -0.12% | 0 |  | 24,376 | -1.08% |
| Render/hf_gpt_oss | 1,203 | -0.41% | 385 | +0.26% | 0 |  | 4,588 | +4.25% |
| Load/hf_mistral_small | 1,898 | -0.42% | 1,622 | +0.12% | 0 |  | 11,842 | -0.70% |
| Render/hf_mistral_small | 959 | +1.27% | 293 | +5.78% | 0 |  | 3,585 | +3.02% |
| Load/hf_qwen3 | 1,528 | -0.52% | 1,327 | +0.15% | 0 |  | 9,395 | -2.49% |
| Render/hf_qwen3 | 656 | -0.46% | 221 | +10.50% | 0 |  | 2,579 | +1.46% |
| Load/html_autoescape | 496 | -1.59% | 544 | 0% | 0 |  | 4,127 | -5.15% |
| Render/html_autoescape | 4,123 | -0.65% | 1,221 | -3.93% | 0 |  | 36,029 | -2.65% |
| Load/inheritance | 6 | 0% | 2 | 0% | 0 |  | 711 | +0.71% |
| Render/inheritance | 593 | -0.17% | 106 | 0% | 0 |  | 938 | -5.16% |
| Load/int_arith | 68 | -11.69% | 44 | -24.14% | 0 |  | 2,150 | +0.89% |
| Render/int_arith | 20 | -20.00% | 6 | 0% | 0 |  | 752 | -1.57% |
| Module/keep | 1 | 0% | 2 | 0% | 0 |  | 421 | +2.93% |
| Module/keep_written | 1,129 | +0.09% | 563 | -0.18% | 0 |  | 730 | -0.54% |
| Load/large_static | 1,927 | -0.10% | 907 | 0% | 0 |  | 288 | -8.28% |
| Render/large_static | 707 | 0% | 569 | 0% | 0 |  | 6 | 0% |
| RenderSink/large_static | 0 |  | 0 |  | 0 |  | 3 | +50.00% |
| RenderTo/large_static | 685 | 0% | 570 | 0% | 0 |  | 6 | +20.00% |
| Load/macros | 79 | -7.06% | 59 | -6.35% | 0 |  | 1,775 | -0.45% |
| Render/macros | 750 | -0.13% | 239 | +1.27% | 0 |  | 2,012 | +224.52% |
| Load/many_tags | 28,616 | -0.16% | 22,778 | -0.04% | 0 |  | 246,798 | -2.72% |
| Render/many_tags | 6,237 | -0.02% | 882 | -0.11% | 0 |  | 139 | +10.32% |
| Load/mitsuhiko_table | 31 | -6.06% | 41 | -10.87% | 0 |  | 939 | +2.74% |
| Render/mitsuhiko_table | 10,056 | -0.04% | 6,269 | -0.02% | 0 |  | 342 | -1.44% |
| RenderSink/mitsuhiko_table | 9,942 | -0.01% | 1,184 | +0.08% | 0 |  | 291 | +5.82% |
| RenderTo/mitsuhiko_table | 10,032 | -0.10% | 6,274 | -0.05% | 0 |  | 304 | -4.70% |
| Load/mitsuhiko_table_wide | 76 | -2.56% | 77 | -2.53% | 0 |  | 1,078 | -0.37% |
| Render/mitsuhiko_table_wide | 10,225 | -0.25% | 22,669 | -0.02% | 0 |  | 345 | 0% |
| RenderSink/mitsuhiko_table_wide | 10,058 | -0.09% | 1,302 | +0.08% | 0 |  | 304 | +8.96% |
| RenderTo/mitsuhiko_table_wide | 10,238 | -2.72% | 22,682 | -0.04% | 0 |  | 319 | +5.98% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 7 | -56.25% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 7 | 0% |
| RenderSink/plain_text | 0 |  | 0 |  | 0 |  | 5 | 0% |
| RenderTo/plain_text | 0 |  | 0 |  | 0 |  | 7 | 0% |
| Load/prompt_budget | 71 | -4.05% | 42 | -4.55% | 0 |  | 1,447 | +2.33% |
| Render/prompt_budget | 4,805 | -0.12% | 544 | +0.93% | 0 |  | 1,100 | +28.50% |
| Load/strings | 160 | -4.19% | 125 | -5.30% | 0 |  | 2,852 | +2.52% |
| Render/strings | 434 | +0.70% | 488 | 0% | 0 |  | 2,433 | -2.37% |
| Load/substitute | 2 | 0% | 3 | 0% | 0 |  | 166 | -11.70% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 5 | 0% |
| RenderSink/substitute | 0 |  | 0 |  | 0 |  | 4 | +33.33% |
| RenderTo/substitute | 0 |  | 0 |  | 0 |  | 6 | +50.00% |

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
