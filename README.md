# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (181 so far, latest [8bccb62](https://github.com/jinja2cpp/Jinja2Cpp/commit/8bccb6237acfc0f0fe17a573e266a8dcf3e40fe8) on 2026-11-06). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 344,877 | -0.01% |  | 55 | 0% |  | 18,464 | 0% | -52.75% |
| [Render/chat_llama](#renderchat_llama) | 295,205 | +0.08% |  | 225 | 0% |  | 11,632 | +0.14% | -19.00% |
| [Load/chat_mistral](#loadchat_mistral) | 393,410 | 0% |  | 52 | 0% |  | 20,888 | 0% | -53.22% |
| [Render/chat_mistral](#renderchat_mistral) | 499,515 | +0.01% |  | 402 | 0% |  | 10,112 | +0.64% | -23.44% |
| [Load/chat_qwen](#loadchat_qwen) | 265,830 | -0.02% |  | 44 | 0% |  | 14,416 | 0% | -47.25% |
| [Render/chat_qwen](#renderchat_qwen) | 276,769 | -0.05% |  | 212 | 0% |  | 8,304 | 0% | -2.35% |
| [Load/config_file](#loadconfig_file) | 97,028 | +0.03% |  | 25 | 0% |  | 5,464 | 0% | -48.26% |
| [Render/config_file](#renderconfig_file) | 1,268,966 | +0.11% |  | 908 | 0% |  | 24,440 | 0% | +2.65% |
| [Load/dict_ops](#loaddict_ops) | 33,306 | -0.03% | -77.46% | 9 | 0% | -93.13% | 2,176 | 0% | -48.87% |
| [Render/dict_ops](#renderdict_ops) | 292,561 | +0.08% | -57.65% | 112 | 0% | -75.11% | 24,408 | 0% | -29.65% |
| [Load/expressions](#loadexpressions) | 44,971 | -0.01% | -78.19% | 16 | 0% | -89.04% | 2,728 | 0% | -55.54% |
| [Render/expressions](#renderexpressions) | 426,222 | 0% | -52.96% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 69,040 | -0.07% | -74.84% | 27 | 0% | -88.16% | 3,848 | 0% | -66.29% |
| [Render/filters](#renderfilters) | 30,386 | +0.06% | -65.73% | 26 | 0% | -64.86% | 1,080 | 0% | -78.01% |
| [Load/for_filter_if](#loadfor_filter_if) | 30,018 | +0.06% | -77.83% | 7 | 0% | -94.31% | 1,904 | 0% | -44.78% |
| [Render/for_filter_if](#renderfor_filter_if) | 274,682 | 0% | -68.26% | 4 | 0% | -98.23% | 16,560 | 0% | +354.95% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 40,264 | -0.09% | -77.71% | 10 | 0% | -93.63% | 2,368 | 0% | -47.61% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 180,540 | -7.12% | -83.17% | 2 | 0% | -99.62% | 1,568 | 0% | -43.35% |
| [Load/for_range](#loadfor_range) | 13,621 | -0.07% | -75.97% | 7 | 0% | -87.50% | 1,088 | 0% | -45.60% |
| [Render/for_range](#renderfor_range) | 38,374 | +0.03% | -58.38% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/hf_deepseek_v31](#loadhf_deepseek_v31) | 386,727 | 0% |  | 55 | 0% |  | 21,976 | 0% | +0.73% |
| [Render/hf_deepseek_v31](#renderhf_deepseek_v31) | 90,653 | +0.36% |  | 43 | 0% |  | 2,480 | 0% | 0% |
| [Load/hf_gpt_oss](#loadhf_gpt_oss) | 1,546,339 | -0.02% |  | 287 | 0% |  | 74,672 | 0% | +0.60% |
| [Render/hf_gpt_oss](#renderhf_gpt_oss) | 170,760 | +0.09% |  | 155 | 0% |  | 5,080 | 0% | +2.92% |
| [Load/hf_mistral_small](#loadhf_mistral_small) | 596,906 | -0.01% |  | 88 | 0% |  | 33,048 | 0% | +0.34% |
| [Render/hf_mistral_small](#renderhf_mistral_small) | 144,562 | -0.16% |  | 105 | 0% |  | 6,120 | 0% | -0.26% |
| [Load/hf_qwen3](#loadhf_qwen3) | 485,324 | +0.02% |  | 82 | 0% |  | 25,560 | 0% | +0.31% |
| [Render/hf_qwen3](#renderhf_qwen3) | 119,657 | -0.42% |  | 100 | 0% |  | 3,248 | 0% | 0% |
| [Load/html_autoescape](#loadhtml_autoescape) | 193,918 | +0.08% |  | 43 | 0% |  | 12,640 | 0% | -48.78% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,209,251 | -0.13% |  | 674 | 0% |  | 45,408 | 0% | +12.44% |
| [Load/inheritance](#loadinheritance) | 22,996 | -0.04% | -79.85% | 7 | 0% | -93.46% | 1,344 | 0% | -53.20% |
| [Render/inheritance](#renderinheritance) | 137,363 | +0.19% | -81.63% | 35 | 0% | -94.37% | 7,384 | 0% | +22.74% |
| [Load/int_arith](#loadint_arith) | 65,949 | -0.07% |  | 20 | 0% |  | 3,872 | 0% | +0.41% |
| [Render/int_arith](#renderint_arith) | 824,154 | +0.01% |  | 12 | 0% |  | 3,272 | 0% | -0.49% |
| [Module/keep](#modulekeep) | 6,960 | 0% |  | 7 | 0% |  |  |  |  |
| [Module/keep_written](#modulekeep_written) | 36,345 | 0% |  | 11 | 0% |  |  |  |  |
| [Load/large_static](#loadlarge_static) | 154,566 | -0.01% | -82.55% | 14 | 0% | -96.82% | 43,120 | 0% | -19.23% |
| [Render/large_static](#renderlarge_static) | 20,709 | +0.04% | -72.44% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [RenderSink/large_static](#rendersinklarge_static) | 12,695 | +0.07% |  | 0 |  |  | 0 |  |  |
| [RenderTo/large_static](#rendertolarge_static) | 20,272 | +0.04% |  | 0 |  |  | 0 |  |  |
| [Load/macros](#loadmacros) | 56,243 | -0.02% | -75.33% | 27 | 0% | -85.48% | 4,392 | 0% | -29.34% |
| [Render/macros](#rendermacros) | 657,111 | +0.12% | -74.36% | 308 | 0% | -88.71% | 20,560 | 0% | +39.67% |
| [Load/many_tags](#loadmany_tags) | 9,005,692 | -0.08% | -79.61% | 951 | 0% | -97.22% | 438,144 | 0% | -58.48% |
| [Render/many_tags](#rendermany_tags) | 725,711 | 0% | -65.16% | 2 | 0% | -90.48% | 5,648 | 0% | -10.52% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 46,452 | -0.02% | -80.45% | 19 | 0% | -89.67% | 3,360 | 0% | -44.59% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,195,667 | 0% | -57.95% | 33 | 0% | -99.19% | 381,536 | 0% | +9.95% |
| [RenderSink/mitsuhiko_table](#rendersinkmitsuhiko_table) | 5,122,847 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table](#rendertomitsuhiko_table) | 5,195,169 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 54,700 | 0% |  | 35 | 0% |  | 5,688 | 0% | -31.17% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,541,804 | 0% |  | 33 | 0% |  | 1,414,928 | 0% | +2.46% |
| [RenderSink/mitsuhiko_table_wide](#rendersinkmitsuhiko_table_wide) | 5,365,110 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table_wide](#rendertomitsuhiko_table_wide) | 5,541,376 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/plain_text](#loadplain_text) | 3,704 | -0.27% | -61.72% | 4 | 0% | -77.78% | 672 | 0% | -40.00% |
| [Render/plain_text](#renderplain_text) | 1,017 | 0% | -85.03% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/plain_text](#rendersinkplain_text) | 717 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/plain_text](#rendertoplain_text) | 818 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/strings](#loadstrings) | 70,209 | -0.02% | -75.94% | 30 | 0% | -86.61% | 4,344 | 0% | -51.95% |
| [Render/strings](#renderstrings) | 578,647 | -0.58% | -72.41% | 344 | 0% | -83.73% | 9,088 | 0% | -49.31% |
| [Load/substitute](#loadsubstitute) | 7,733 | -0.13% | -74.10% | 6 | 0% | -83.78% | 832 | 0% | -45.26% |
| [Render/substitute](#rendersubstitute) | 2,251 | +0.81% | -74.46% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/substitute](#rendersinksubstitute) | 1,954 | +0.93% |  | 0 |  |  | 0 |  |  |
| [RenderTo/substitute](#rendertosubstitute) | 2,056 | +0.88% |  | 0 |  |  | 0 |  |  |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 731 | -2.01% | 737 | +0.27% | 0 |  | 7,156 | -2.64% |
| Render/chat_llama | 1,153 | -6.41% | 521 | -1.14% | 0 |  | 3,789 | -2.45% |
| Load/chat_mistral | 1,105 | +0.91% | 1,061 | +0.28% | 0 |  | 8,080 | -2.20% |
| Render/chat_mistral | 1,853 | +1.42% | 541 | +2.46% | 0 |  | 9,930 | +1.23% |
| Load/chat_qwen | 575 | +0.70% | 596 | 0% | 0 |  | 4,285 | -1.61% |
| Render/chat_qwen | 1,117 | -0.89% | 410 | +5.67% | 0 |  | 3,276 | +1.83% |
| Load/config_file | 199 | +3.65% | 165 | +1.23% | 0 |  | 2,634 | -2.26% |
| Render/config_file | 6,028 | -5.00% | 1,649 | +6.18% | 0 |  | 40,292 | -0.48% |
| Load/dict_ops | 31 | 0% | 21 | +10.53% | 0 |  | 1,322 | +0.30% |
| Render/dict_ops | 670 | +1.67% | 429 | +2.14% | 0 |  | 846 | -2.08% |
| Load/expressions | 25 | -10.71% | 21 | -12.50% | 0 |  | 1,301 | +4.08% |
| Render/expressions | 2 | -60.00% | 1 | -50.00% | 0 |  | 285 | +0.71% |
| Load/filters | 90 | +3.45% | 58 | +3.57% | 0 |  | 1,669 | +5.43% |
| Render/filters | 15 | -6.25% | 4 | -20.00% | 0 |  | 742 | -3.64% |
| Load/for_filter_if | 18 | +5.88% | 12 | -7.69% | 0 |  | 1,051 | +0.10% |
| Render/for_filter_if | 1,732 | +0.29% | 95 | +4.40% | 0 |  | 160 | -22.71% |
| Load/for_loop_vars | 33 | -13.16% | 18 | -21.74% | 0 |  | 1,047 | -3.59% |
| Render/for_loop_vars | 10 | +42.86% | 3 | +200.00% | 0 |  | 137 | -18.93% |
| Load/for_range | 10 | +66.67% | 5 | 0% | 0 |  | 763 | +4.81% |
| Render/for_range | 1 | 0% | 0 |  | 0 |  | 134 | +27.62% |
| Load/hf_deepseek_v31 | 1,007 | +0.50% | 970 | -0.51% | 0 |  | 5,799 | -6.06% |
| Render/hf_deepseek_v31 | 354 | -0.28% | 76 | 0% | 0 |  | 1,409 | +4.68% |
| Load/hf_gpt_oss | 4,699 | +1.80% | 3,293 | +0.15% | 0 |  | 20,854 | -4.53% |
| Render/hf_gpt_oss | 1,205 | -1.39% | 368 | -1.60% | 0 |  | 4,453 | -0.40% |
| Load/hf_mistral_small | 1,836 | +0.05% | 1,596 | -0.44% | 0 |  | 10,334 | -1.50% |
| Render/hf_mistral_small | 945 | -0.94% | 330 | +5.43% | 0 |  | 3,699 | +0.22% |
| Load/hf_qwen3 | 1,462 | +1.32% | 1,242 | -0.32% | 0 |  | 8,293 | -2.99% |
| Render/hf_qwen3 | 668 | -1.18% | 210 | -2.78% | 0 |  | 2,522 | -1.06% |
| Load/html_autoescape | 443 | +1.37% | 513 | +0.20% | 0 |  | 3,873 | +1.44% |
| Render/html_autoescape | 4,023 | -9.49% | 1,378 | -1.29% | 0 |  | 36,322 | -0.08% |
| Load/inheritance | 5 | +66.67% | 5 | +25.00% | 0 |  | 682 | +1.49% |
| Render/inheritance | 586 | -0.51% | 102 | +4.08% | 0 |  | 1,338 | +48.17% |
| Load/int_arith | 93 | +4.49% | 65 | +8.33% | 0 |  | 2,051 | +1.64% |
| Render/int_arith | 13 | +116.67% | 12 | +140.00% | 0 |  | 721 | -1.23% |
| Module/keep | 2 | 0% | 2 | 0% | 0 |  | 388 | +11.49% |
| Module/keep_written | 2,927 | -0.03% | 557 | 0% | 0 |  | 681 | -4.22% |
| Load/large_static | 1,938 | +0.16% | 911 | +0.11% | 0 |  | 255 | -5.90% |
| Render/large_static | 710 | 0% | 568 | 0% | 0 |  | 4 | 0% |
| RenderSink/large_static | 0 |  | 0 |  | 0 |  | 2 | 0% |
| RenderTo/large_static | 687 | 0% | 572 | 0% | 0 |  | 4 | +33.33% |
| Load/macros | 87 | +12.99% | 69 | +7.81% | 0 |  | 1,680 | -2.10% |
| Render/macros | 755 | -1.69% | 214 | 0% | 0 |  | 620 | -13.89% |
| Load/many_tags | 29,416 | +3.28% | 22,639 | +0.09% | 0 |  | 234,861 | +5.16% |
| Render/many_tags | 6,233 | +0.11% | 845 | +0.24% | 0 |  | 135 | +4.65% |
| Load/mitsuhiko_table | 24 | -11.11% | 25 | 0% | 0 |  | 873 | +4.05% |
| Render/mitsuhiko_table | 10,100 | -0.01% | 6,291 | -0.02% | 0 |  | 350 | -0.57% |
| RenderSink/mitsuhiko_table | 9,986 | +0.28% | 1,218 | +1.58% | 0 |  | 274 | -10.46% |
| RenderTo/mitsuhiko_table | 10,076 | -0.09% | 6,300 | 0% | 0 |  | 305 | -7.85% |
| Load/mitsuhiko_table_wide | 51 | -12.07% | 61 | 0% | 0 |  | 930 | -7.28% |
| Render/mitsuhiko_table_wide | 10,281 | -2.40% | 22,683 | -0.40% | 0 |  | 340 | -4.23% |
| RenderSink/mitsuhiko_table_wide | 10,080 | -0.36% | 1,305 | +0.08% | 0 |  | 260 | -13.62% |
| RenderTo/mitsuhiko_table_wide | 10,267 | -2.54% | 22,675 | +0.01% | 0 |  | 307 | -4.66% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 7 | -50.00% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 6 | 0% |
| RenderSink/plain_text | 0 |  | 0 |  | 0 |  | 4 | -20.00% |
| RenderTo/plain_text | 0 |  | 0 |  | 0 |  | 6 | 0% |
| Load/strings | 116 | 0% | 82 | 0% | 0 |  | 2,715 | -0.73% |
| Render/strings | 414 | +0.24% | 474 | 0% | 0 |  | 1,546 | -1.97% |
| Load/substitute | 2 | 0% | 3 | 0% | 0 |  | 127 | -18.06% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 3 | -25.00% |
| RenderSink/substitute | 0 |  | 0 |  | 0 |  | 2 | 0% |
| RenderTo/substitute | 0 |  | 0 |  | 0 |  | 3 | 0% |

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
