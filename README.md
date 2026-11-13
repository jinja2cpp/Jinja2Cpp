# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (216 so far, latest [0bfd55d](https://github.com/jinja2cpp/Jinja2Cpp/commit/0bfd55db2552f03d0d40f39cef2aa4f2dab4e612) on 2026-11-13). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 335,028 | 0% |  | 55 | 0% |  | 18,464 | 0% | -52.75% |
| [Render/chat_llama](#renderchat_llama) | 295,019 | -0.24% |  | 225 | 0% |  | 11,616 | 0% | -19.11% |
| [Load/chat_mistral](#loadchat_mistral) | 384,820 | +0.04% |  | 52 | 0% |  | 20,888 | 0% | -53.22% |
| [Render/chat_mistral](#renderchat_mistral) | 501,385 | -0.11% |  | 402 | 0% |  | 10,000 | 0% | -24.29% |
| [Load/chat_qwen](#loadchat_qwen) | 260,997 | +0.04% |  | 44 | 0% |  | 14,416 | 0% | -47.25% |
| [Render/chat_qwen](#renderchat_qwen) | 278,819 | +0.32% |  | 212 | 0% |  | 8,304 | 0% | -2.35% |
| [Load/config_file](#loadconfig_file) | 94,276 | +0.04% |  | 25 | 0% |  | 5,464 | 0% | -48.26% |
| [Render/config_file](#renderconfig_file) | 1,275,406 | -0.08% |  | 908 | 0% |  | 24,440 | 0% | +2.65% |
| [Load/dict_ops](#loaddict_ops) | 32,192 | +0.02% | -78.22% | 9 | 0% | -93.13% | 2,176 | 0% | -48.87% |
| [Render/dict_ops](#renderdict_ops) | 295,519 | -0.07% | -57.22% | 112 | 0% | -75.11% | 24,408 | 0% | -29.65% |
| [Load/expressions](#loadexpressions) | 44,111 | 0% | -78.60% | 16 | 0% | -89.04% | 2,728 | 0% | -55.54% |
| [Render/expressions](#renderexpressions) | 426,984 | +0.01% | -52.88% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 67,394 | -0.08% | -75.44% | 27 | 0% | -88.16% | 3,848 | 0% | -66.29% |
| [Render/filters](#renderfilters) | 27,711 | +0.22% | -68.75% | 23 | 0% | -68.92% | 752 | 0% | -84.69% |
| [Load/for_filter_if](#loadfor_filter_if) | 29,037 | +0.02% | -78.55% | 7 | 0% | -94.31% | 1,904 | 0% | -44.78% |
| [Render/for_filter_if](#renderfor_filter_if) | 274,383 | 0% | -68.30% | 4 | 0% | -98.23% | 16,560 | 0% | +354.95% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 38,850 | +0.05% | -78.50% | 10 | 0% | -93.63% | 2,368 | 0% | -47.61% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 180,570 | 0% | -83.17% | 2 | 0% | -99.62% | 1,568 | 0% | -43.35% |
| [Load/for_range](#loadfor_range) | 13,209 | 0% | -76.70% | 7 | 0% | -87.50% | 1,088 | 0% | -45.60% |
| [Render/for_range](#renderfor_range) | 38,435 | +0.15% | -58.32% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/hf_deepseek_v31](#loadhf_deepseek_v31) | 382,752 | +0.03% |  | 55 | 0% |  | 21,976 | 0% | +0.73% |
| [Render/hf_deepseek_v31](#renderhf_deepseek_v31) | 92,949 | +0.39% |  | 43 | 0% |  | 2,480 | 0% | 0% |
| [Load/hf_gpt_oss](#loadhf_gpt_oss) | 1,522,027 | +0.02% |  | 287 | 0% |  | 74,704 | 0% | +0.65% |
| [Render/hf_gpt_oss](#renderhf_gpt_oss) | 173,381 | +0.26% |  | 155 | 0% |  | 5,096 | 0% | +3.24% |
| [Load/hf_mistral_small](#loadhf_mistral_small) | 591,012 | +0.03% |  | 88 | 0% |  | 33,048 | 0% | +0.34% |
| [Render/hf_mistral_small](#renderhf_mistral_small) | 142,648 | +0.10% |  | 99 | 0% |  | 6,120 | 0% | -0.26% |
| [Load/hf_qwen3](#loadhf_qwen3) | 475,275 | +0.05% |  | 82 | 0% |  | 25,560 | 0% | +0.31% |
| [Render/hf_qwen3](#renderhf_qwen3) | 121,152 | +0.26% |  | 100 | 0% |  | 3,248 | 0% | 0% |
| [Load/html_autoescape](#loadhtml_autoescape) | 188,728 | +0.05% |  | 43 | 0% |  | 12,672 | 0% | -48.65% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,209,575 | +0.08% |  | 674 | 0% |  | 45,408 | 0% | +12.44% |
| [Load/inheritance](#loadinheritance) | 21,887 | 0% | -80.82% | 7 | 0% | -93.46% | 1,344 | 0% | -53.20% |
| [Render/inheritance](#renderinheritance) | 137,625 | 0% | -81.60% | 35 | 0% | -94.37% | 7,384 | 0% | +22.74% |
| [Load/int_arith](#loadint_arith) | 64,887 | 0% |  | 20 | 0% |  | 3,872 | 0% | +0.41% |
| [Render/int_arith](#renderint_arith) | 828,231 | +0.02% |  | 12 | 0% |  | 3,272 | 0% | -0.49% |
| [Module/keep](#modulekeep) | 7,120 | +0.07% |  | 7 | 0% |  |  |  |  |
| [Module/keep_written](#modulekeep_written) | 29,078 | +0.03% |  | 11 | 0% |  |  |  |  |
| [Load/large_static](#loadlarge_static) | 147,598 | 0% | -83.34% | 14 | 0% | -96.82% | 43,120 | 0% | -19.23% |
| [Render/large_static](#renderlarge_static) | 20,726 | 0% | -72.42% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [RenderSink/large_static](#rendersinklarge_static) | 12,716 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/large_static](#rendertolarge_static) | 20,289 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/macros](#loadmacros) | 55,217 | +0.02% | -75.78% | 27 | 0% | -85.48% | 4,392 | 0% | -29.34% |
| [Render/macros](#rendermacros) | 658,825 | -0.14% | -74.30% | 308 | 0% | -88.71% | 20,560 | 0% | +39.67% |
| [Load/many_tags](#loadmany_tags) | 8,717,044 | +0.03% | -80.26% | 951 | 0% | -97.22% | 438,112 | 0% | -58.48% |
| [Render/many_tags](#rendermany_tags) | 729,957 | +0.01% | -64.96% | 2 | 0% | -90.48% | 5,648 | 0% | -10.52% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 45,013 | 0% | -81.05% | 19 | 0% | -89.67% | 3,360 | 0% | -44.59% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,203,060 | 0% | -57.89% | 33 | 0% | -99.19% | 381,536 | 0% | +9.95% |
| [RenderSink/mitsuhiko_table](#rendersinkmitsuhiko_table) | 5,130,327 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table](#rendertomitsuhiko_table) | 5,202,612 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 52,730 | 0% |  | 35 | 0% |  | 5,688 | 0% | -31.17% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,549,076 | 0% |  | 33 | 0% |  | 1,414,928 | 0% | +2.46% |
| [RenderSink/mitsuhiko_table_wide](#rendersinkmitsuhiko_table_wide) | 5,372,377 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table_wide](#rendertomitsuhiko_table_wide) | 5,548,644 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/plain_text](#loadplain_text) | 3,708 | 0% | -61.68% | 4 | 0% | -77.78% | 672 | 0% | -40.00% |
| [Render/plain_text](#renderplain_text) | 1,036 | 0% | -84.75% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/plain_text](#rendersinkplain_text) | 738 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/plain_text](#rendertoplain_text) | 837 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/prompt_budget](#loadprompt_budget) | 34,427 | +0.02% |  | 18 | 0% |  | 2,224 | 0% | 0% |
| [Render/prompt_budget](#renderprompt_budget) | 1,319,766 | +0.02% |  | 76 | 0% |  | 36,416 | 0% | 0% |
| [Load/strings](#loadstrings) | 69,033 | +0.02% | -76.35% | 30 | 0% | -86.61% | 4,344 | 0% | -51.95% |
| [Render/strings](#renderstrings) | 583,372 | +0.02% | -72.19% | 344 | 0% | -83.73% | 9,088 | 0% | -49.31% |
| [Load/substitute](#loadsubstitute) | 7,483 | 0% | -74.94% | 6 | 0% | -83.78% | 832 | 0% | -45.26% |
| [Render/substitute](#rendersubstitute) | 2,269 | 0% | -74.26% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/substitute](#rendersinksubstitute) | 1,975 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/substitute](#rendertosubstitute) | 2,074 | 0% |  | 0 |  |  | 0 |  |  |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 751 | -0.40% | 758 | +0.93% | 0 |  | 7,541 | -1.00% |
| Render/chat_llama | 1,157 | +0.52% | 534 | +2.50% | 0 |  | 4,310 | -3.02% |
| Load/chat_mistral | 1,107 | -0.18% | 1,093 | -0.91% | 0 |  | 8,764 | +1.04% |
| Render/chat_mistral | 1,874 | -0.27% | 586 | -0.17% | 0 |  | 10,547 | -2.16% |
| Load/chat_qwen | 649 | +4.85% | 633 | +0.32% | 0 |  | 4,516 | -0.68% |
| Render/chat_qwen | 1,114 | -1.15% | 426 | +8.12% | 0 |  | 3,703 | +3.73% |
| Load/config_file | 233 | +4.48% | 198 | -2.46% | 0 |  | 2,763 | -1.07% |
| Render/config_file | 6,398 | +6.78% | 1,440 | -1.71% | 0 |  | 44,669 | +2.81% |
| Load/dict_ops | 57 | +5.56% | 33 | -2.94% | 0 |  | 1,372 | -2.76% |
| Render/dict_ops | 668 | -2.77% | 425 | -1.62% | 0 |  | 869 | -4.61% |
| Load/expressions | 28 | 0% | 20 | -20.00% | 0 |  | 1,314 | -4.16% |
| Render/expressions | 10 | -16.67% | 1 | -83.33% | 0 |  | 338 | +2.42% |
| Load/filters | 97 | -13.39% | 51 | +2.00% | 0 |  | 1,840 | +8.62% |
| Render/filters | 20 | +17.65% | 7 | +40.00% | 0 |  | 791 | -1.49% |
| Load/for_filter_if | 43 | +30.30% | 24 | +26.32% | 0 |  | 1,099 | -1.88% |
| Render/for_filter_if | 1,739 | +0.52% | 132 | -5.71% | 0 |  | 165 | +9.27% |
| Load/for_loop_vars | 49 | +11.36% | 26 | +18.18% | 0 |  | 1,200 | +3.54% |
| Render/for_loop_vars | 13 | +44.44% | 3 | 0% | 0 |  | 139 | -7.33% |
| Load/for_range | 4 | 0% | 6 | +100.00% | 0 |  | 785 | -2.73% |
| Render/for_range | 2 | +100.00% | 0 |  | 0 |  | 145 | -13.69% |
| Load/hf_deepseek_v31 | 996 | +0.40% | 998 | +1.42% | 0 |  | 6,583 | +0.30% |
| Render/hf_deepseek_v31 | 368 | +5.75% | 76 | 0% | 0 |  | 1,525 | +2.90% |
| Load/hf_gpt_oss | 4,663 | -0.28% | 3,374 | -0.15% | 0 |  | 21,852 | +0.89% |
| Render/hf_gpt_oss | 1,202 | -1.72% | 382 | -0.26% | 0 |  | 4,695 | -0.47% |
| Load/hf_mistral_small | 1,920 | -0.83% | 1,643 | +0.12% | 0 |  | 11,345 | +2.56% |
| Render/hf_mistral_small | 968 | +0.94% | 297 | 0% | 0 |  | 3,809 | +3.42% |
| Load/hf_qwen3 | 1,543 | +0.85% | 1,335 | -0.82% | 0 |  | 8,831 | +0.55% |
| Render/hf_qwen3 | 671 | +1.98% | 219 | +0.92% | 0 |  | 2,861 | +4.45% |
| Load/html_autoescape | 511 | -0.78% | 544 | +0.55% | 0 |  | 4,042 | -1.39% |
| Render/html_autoescape | 4,399 | +3.58% | 1,308 | +1.95% | 0 |  | 36,773 | +1.16% |
| Load/inheritance | 3 | -50.00% | 3 | -40.00% | 0 |  | 706 | -2.49% |
| Render/inheritance | 609 | +1.67% | 107 | +5.94% | 0 |  | 1,000 | -0.10% |
| Load/int_arith | 93 | +24.00% | 55 | +5.77% | 0 |  | 2,071 | -4.47% |
| Render/int_arith | 23 | +35.29% | 5 | -54.55% | 0 |  | 781 | +2.76% |
| Module/keep | 1 | -50.00% | 2 | 0% | 0 |  | 436 | +0.69% |
| Module/keep_written | 1,134 | +0.35% | 561 | -0.18% | 0 |  | 726 | -2.55% |
| Load/large_static | 1,935 | +0.21% | 905 | 0% | 0 |  | 306 | -7.27% |
| Render/large_static | 709 | +0.14% | 568 | -0.18% | 0 |  | 6 | +20.00% |
| RenderSink/large_static | 1 |  | 0 |  | 0 |  | 5 | +66.67% |
| RenderTo/large_static | 685 | 0% | 572 | +0.35% | 0 |  | 6 | +20.00% |
| Load/macros | 101 | +4.12% | 76 | -1.30% | 0 |  | 1,715 | -2.06% |
| Render/macros | 747 | -0.80% | 229 | -4.18% | 0 |  | 659 | +0.46% |
| Load/many_tags | 28,409 | -1.97% | 22,571 | -0.78% | 0 |  | 233,120 | -0.12% |
| Render/many_tags | 6,249 | -0.08% | 862 | -2.16% | 0 |  | 151 | +7.86% |
| Load/mitsuhiko_table | 53 | +29.27% | 60 | +7.14% | 0 |  | 892 | -6.79% |
| Render/mitsuhiko_table | 10,086 | +0.21% | 6,287 | +0.18% | 0 |  | 364 | -12.71% |
| RenderSink/mitsuhiko_table | 9,990 | 0% | 1,205 | +1.60% | 0 |  | 315 | -5.97% |
| RenderTo/mitsuhiko_table | 10,079 | +0.16% | 6,292 | +0.30% | 0 |  | 341 | -10.73% |
| Load/mitsuhiko_table_wide | 89 | +25.35% | 99 | +11.24% | 0 |  | 932 | -11.57% |
| Render/mitsuhiko_table_wide | 10,202 | -0.65% | 22,648 | -0.13% | 0 |  | 390 | -2.26% |
| RenderSink/mitsuhiko_table_wide | 10,060 | -0.17% | 1,262 | -2.32% | 0 |  | 317 | -4.80% |
| RenderTo/mitsuhiko_table_wide | 10,213 | -0.68% | 22,653 | -0.13% | 0 |  | 366 | -1.61% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 17 | +6.25% |
| Render/plain_text | 1 | 0% | 0 |  | 0 |  | 6 | 0% |
| RenderSink/plain_text | 0 |  | 0 |  | 0 |  | 5 | 0% |
| RenderTo/plain_text | 1 |  | 0 |  | 0 |  | 5 | 0% |
| Load/prompt_budget | 82 | 0% | 45 | 0% | 0 |  | 1,403 | -1.61% |
| Render/prompt_budget | 4,814 | +0.23% | 547 | -0.36% | 0 |  | 1,395 | +24.66% |
| Load/strings | 162 | +6.58% | 116 | -5.69% | 0 |  | 2,838 | +0.92% |
| Render/strings | 440 | +2.33% | 488 | +1.46% | 0 |  | 1,606 | -0.68% |
| Load/substitute | 2 | 0% | 3 | 0% | 0 |  | 183 | +6.40% |
| Render/substitute | 1 |  | 0 |  | 0 |  | 5 | 0% |
| RenderSink/substitute | 1 |  | 0 |  | 0 |  | 6 | +20.00% |
| RenderTo/substitute | 1 |  | 0 |  | 0 |  | 5 | 0% |

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
