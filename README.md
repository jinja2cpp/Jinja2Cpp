# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (171 so far, latest [5248d1f](https://github.com/jinja2cpp/Jinja2Cpp/commit/5248d1fe55e7592cb7c5bcfcb9766e31af231ff0) on 2026-11-03). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 343,182 | -0.15% |  | 55 | 0% |  | 18,464 | +0.52% | -52.75% |
| [Render/chat_llama](#renderchat_llama) | 294,850 | +0.55% |  | 225 | 0% |  | 11,616 | 0% | -19.11% |
| [Load/chat_mistral](#loadchat_mistral) | 392,383 | -0.15% |  | 52 | 0% |  | 20,888 | +0.31% | -53.22% |
| [Render/chat_mistral](#renderchat_mistral) | 498,614 | -0.05% |  | 402 | 0% |  | 10,064 | +1.13% | -23.80% |
| [Load/chat_qwen](#loadchat_qwen) | 264,969 | -0.23% |  | 44 | 0% |  | 14,416 | 0% | -47.25% |
| [Render/chat_qwen](#renderchat_qwen) | 276,852 | -0.24% |  | 212 | 0% |  | 8,304 | 0% | -2.35% |
| [Load/config_file](#loadconfig_file) | 96,580 | -0.50% |  | 25 | 0% |  | 5,464 | 0% | -48.26% |
| [Render/config_file](#renderconfig_file) | 1,262,717 | -0.63% |  | 908 | 0% |  | 24,440 | +0.73% | +2.65% |
| [Load/dict_ops](#loaddict_ops) | 33,208 | -0.08% | -77.53% | 9 | 0% | -93.13% | 2,176 | 0% | -48.87% |
| [Render/dict_ops](#renderdict_ops) | 293,222 | -1.47% | -57.56% | 112 | 0% | -75.11% | 24,408 | 0% | -29.65% |
| [Load/expressions](#loadexpressions) | 44,894 | -0.89% | -78.22% | 16 | 0% | -89.04% | 2,728 | 0% | -55.54% |
| [Render/expressions](#renderexpressions) | 427,154 | -0.07% | -52.86% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 68,902 | -2.26% | -74.89% | 27 | 0% | -88.16% | 3,848 | +0.42% | -66.29% |
| [Render/filters](#renderfilters) | 30,238 | +0.04% | -65.90% | 26 | 0% | -64.86% | 1,080 | 0% | -78.01% |
| [Load/for_filter_if](#loadfor_filter_if) | 29,857 | +0.03% | -77.95% | 7 | 0% | -94.31% | 1,904 | 0% | -44.78% |
| [Render/for_filter_if](#renderfor_filter_if) | 275,077 | +0.23% | -68.22% | 4 | 0% | -98.23% | 16,560 | 0% | +354.95% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 40,118 | -0.11% | -77.79% | 10 | 0% | -93.63% | 2,368 | 0% | -47.61% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 192,484 | -0.11% | -82.06% | 2 | 0% | -99.62% | 1,568 | 0% | -43.35% |
| [Load/for_range](#loadfor_range) | 13,570 | -0.03% | -76.06% | 7 | 0% | -87.50% | 1,088 | 0% | -45.60% |
| [Render/for_range](#renderfor_range) | 38,806 | -0.02% | -57.92% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/hf_deepseek_v31](#loadhf_deepseek_v31) | 385,415 | +0.06% |  | 55 | 0% |  | 21,976 | +0.73% | +0.73% |
| [Render/hf_deepseek_v31](#renderhf_deepseek_v31) | 90,316 | -0.12% |  | 43 | 0% |  | 2,480 | 0% | 0% |
| [Load/hf_gpt_oss](#loadhf_gpt_oss) | 1,532,012 | -0.25% |  | 285 | 0% |  | 74,384 | +0.22% | +0.22% |
| [Render/hf_gpt_oss](#renderhf_gpt_oss) | 169,942 | -1.25% |  | 155 | 0% |  | 5,064 | +2.59% | +2.59% |
| [Load/hf_mistral_small](#loadhf_mistral_small) | 595,360 | -0.19% |  | 88 | 0% |  | 33,048 | +0.34% | +0.34% |
| [Render/hf_mistral_small](#renderhf_mistral_small) | 144,576 | -0.43% |  | 105 | 0% |  | 6,120 | -0.26% | -0.26% |
| [Load/hf_qwen3](#loadhf_qwen3) | 483,487 | -0.28% |  | 82 | 0% |  | 25,560 | +0.31% | +0.31% |
| [Render/hf_qwen3](#renderhf_qwen3) | 119,989 | +0.24% |  | 100 | 0% |  | 3,248 | 0% | 0% |
| [Load/html_autoescape](#loadhtml_autoescape) | 192,814 | -0.28% |  | 43 | 0% |  | 12,640 | 0% | -48.78% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,210,539 | +0.24% |  | 674 | 0% |  | 45,408 | +0.25% | +12.44% |
| [Load/inheritance](#loadinheritance) | 22,739 | -0.12% | -80.07% | 7 | 0% | -93.46% | 1,328 | 0% | -53.76% |
| [Render/inheritance](#renderinheritance) | 136,868 | +0.30% | -81.70% | 35 | 0% | -94.37% | 7,384 | +0.44% | +22.74% |
| [Load/int_arith](#loadint_arith) | 65,794 | -0.79% |  | 20 | 0% |  | 3,872 | 0% | +0.41% |
| [Render/int_arith](#renderint_arith) | 824,976 | -0.12% |  | 12 | 0% |  | 3,272 | 0% | -0.49% |
| [Module/keep](#modulekeep) | 6,951 | +0.36% |  | 7 | 0% |  |  |  |  |
| [Module/keep_written](#modulekeep_written) | 36,313 | -0.02% |  | 11 | 0% |  |  |  |  |
| [Load/large_static](#loadlarge_static) | 154,108 | -0.65% | -82.61% | 14 | 0% | -96.82% | 43,120 | 0% | -19.23% |
| [Render/large_static](#renderlarge_static) | 20,695 | -0.86% | -72.46% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [RenderSink/large_static](#rendersinklarge_static) | 12,681 | +0.04% |  | 0 |  |  | 0 |  |  |
| [RenderTo/large_static](#rendertolarge_static) | 20,258 | +0.02% |  | 0 |  |  | 0 |  |  |
| [Load/macros](#loadmacros) | 55,672 | +0.21% | -75.58% | 27 | 0% | -85.48% | 4,344 | 0% | -30.12% |
| [Render/macros](#rendermacros) | 655,766 | +0.47% | -74.41% | 308 | 0% | -88.71% | 20,560 | +0.16% | +39.67% |
| [Load/many_tags](#loadmany_tags) | 8,967,073 | +0.25% | -79.70% | 951 | 0% | -97.22% | 438,176 | +0.48% | -58.48% |
| [Render/many_tags](#rendermany_tags) | 725,693 | +0.05% | -65.16% | 2 | 0% | -90.48% | 5,648 | 0% | -10.52% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 46,282 | -0.05% | -80.52% | 19 | 0% | -89.67% | 3,360 | 0% | -44.59% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,225,795 | +0.01% | -57.71% | 33 | 0% | -99.19% | 381,536 | 0% | +9.95% |
| [RenderSink/mitsuhiko_table](#rendersinkmitsuhiko_table) | 5,152,964 | +0.01% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table](#rendertomitsuhiko_table) | 5,225,278 | +0.01% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 54,520 | -0.46% |  | 35 | 0% |  | 5,688 | 0% | -31.17% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,571,740 | +0.01% |  | 33 | 0% |  | 1,414,928 | 0% | +2.46% |
| [RenderSink/mitsuhiko_table_wide](#rendersinkmitsuhiko_table_wide) | 5,395,027 | +0.01% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table_wide](#rendertomitsuhiko_table_wide) | 5,571,311 | +0.01% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/plain_text](#loadplain_text) | 3,692 | 0% | -61.84% | 4 | 0% | -77.78% | 672 | 0% | -40.00% |
| [Render/plain_text](#renderplain_text) | 1,012 | +0.50% | -85.10% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/plain_text](#rendersinkplain_text) | 712 | +0.71% |  | 0 |  |  | 0 |  |  |
| [RenderTo/plain_text](#rendertoplain_text) | 813 | +0.62% |  | 0 |  |  | 0 |  |  |
| [Load/strings](#loadstrings) | 70,052 | -2.38% | -76.00% | 30 | 0% | -86.61% | 4,344 | +0.37% | -51.95% |
| [Render/strings](#renderstrings) | 583,086 | -0.08% | -72.20% | 344 | 0% | -83.73% | 9,088 | 0% | -49.31% |
| [Load/substitute](#loadsubstitute) | 7,709 | 0% | -74.18% | 6 | 0% | -83.78% | 832 | 0% | -45.26% |
| [Render/substitute](#rendersubstitute) | 2,228 | +0.22% | -74.72% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/substitute](#rendersinksubstitute) | 1,931 | +0.26% |  | 0 |  |  | 0 |  |  |
| [RenderTo/substitute](#rendertosubstitute) | 2,033 | +0.25% |  | 0 |  |  | 0 |  |  |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 722 | -2.17% | 740 | -1.20% | 0 |  | 7,323 | +2.16% |
| Render/chat_llama | 1,182 | -1.42% | 510 | +3.45% | 0 |  | 3,661 | -5.89% |
| Load/chat_mistral | 1,092 | +1.58% | 1,054 | +1.84% | 0 |  | 8,430 | +2.03% |
| Render/chat_mistral | 1,939 | +0.31% | 554 | -0.18% | 0 |  | 9,468 | -2.31% |
| Load/chat_qwen | 492 | +5.81% | 583 | -0.85% | 0 |  | 4,420 | -0.63% |
| Render/chat_qwen | 1,054 | -4.18% | 395 | -5.73% | 0 |  | 3,118 | -2.71% |
| Load/config_file | 164 | -12.30% | 146 | -11.52% | 0 |  | 2,699 | +3.05% |
| Render/config_file | 6,045 | -2.63% | 1,628 | +6.68% | 0 |  | 39,476 | -3.26% |
| Load/dict_ops | 28 | -46.15% | 12 | -70.00% | 0 |  | 1,300 | +1.56% |
| Render/dict_ops | 648 | -13.94% | 432 | +5.88% | 0 |  | 864 | +3.10% |
| Load/expressions | 35 | -5.41% | 24 | +9.09% | 0 |  | 1,247 | -1.66% |
| Render/expressions | 2 | -81.82% | 0 | -100.00% | 0 |  | 274 | -16.97% |
| Load/filters | 79 | -41.48% | 52 | -36.59% | 0 |  | 1,446 | -17.56% |
| Render/filters | 19 | -5.00% | 2 | -33.33% | 0 |  | 782 | 0% |
| Load/for_filter_if | 15 | -28.57% | 7 | -30.00% | 0 |  | 1,089 | +1.59% |
| Render/for_filter_if | 1,727 | -4.06% | 90 | -8.16% | 0 |  | 183 | +12.27% |
| Load/for_loop_vars | 46 | +9.52% | 22 | -29.03% | 0 |  | 1,116 | +5.18% |
| Render/for_loop_vars | 5 | -37.50% | 1 |  | 0 |  | 143 | +57.14% |
| Load/for_range | 9 | -35.71% | 4 | -50.00% | 0 |  | 694 | -2.53% |
| Render/for_range | 1 | 0% | 0 | -100.00% | 0 |  | 130 | +18.18% |
| Load/hf_deepseek_v31 | 971 | +1.25% | 963 | +2.88% | 0 |  | 6,025 | +2.01% |
| Render/hf_deepseek_v31 | 362 | -6.46% | 77 | -7.23% | 0 |  | 1,293 | -8.43% |
| Load/hf_gpt_oss | 4,521 | -2.25% | 3,276 | +0.12% | 0 |  | 21,368 | +3.41% |
| Render/hf_gpt_oss | 1,220 | -4.61% | 379 | -2.32% | 0 |  | 4,422 | -1.12% |
| Load/hf_mistral_small | 1,754 | -1.96% | 1,568 | -1.07% | 0 |  | 10,777 | -0.23% |
| Render/hf_mistral_small | 949 | -3.46% | 269 | -1.82% | 0 |  | 3,699 | -2.35% |
| Load/hf_qwen3 | 1,423 | -2.00% | 1,249 | -1.19% | 0 |  | 8,641 | +1.02% |
| Render/hf_qwen3 | 671 | -3.03% | 187 | -5.08% | 0 |  | 2,509 | +0.20% |
| Load/html_autoescape | 432 | -5.05% | 513 | +0.20% | 0 |  | 4,040 | +2.75% |
| Render/html_autoescape | 3,884 | +15.32% | 1,464 | +33.58% | 0 |  | 36,075 | +2.94% |
| Load/inheritance | 7 | +133.33% | 3 | 0% | 0 |  | 632 | -3.95% |
| Render/inheritance | 582 | +0.34% | 99 | +2.06% | 0 |  | 883 | +1.61% |
| Load/int_arith | 71 | -23.66% | 49 | -22.22% | 0 |  | 2,018 | +2.33% |
| Render/int_arith | 6 | -33.33% | 2 | -33.33% | 0 |  | 715 | -4.28% |
| Module/keep | 2 | 0% | 2 | +100.00% | 0 |  | 428 | +22.99% |
| Module/keep_written | 2,927 | -0.20% | 557 | -1.07% | 0 |  | 703 | -3.83% |
| Load/large_static | 1,929 | -1.08% | 913 | -0.22% | 0 |  | 260 | -1.52% |
| Render/large_static | 709 | +0.42% | 568 | -0.35% | 0 |  | 5 | +25.00% |
| RenderSink/large_static | 0 | -100.00% | 0 |  | 0 |  | 2 | -33.33% |
| RenderTo/large_static | 686 | +0.29% | 572 | +0.53% | 0 |  | 5 | +25.00% |
| Load/macros | 74 | -3.90% | 55 | -23.61% | 0 |  | 1,734 | +4.02% |
| Render/macros | 754 | -2.20% | 227 | -13.03% | 0 |  | 689 | -56.72% |
| Load/many_tags | 28,350 | +0.50% | 22,609 | +1.38% | 0 |  | 234,504 | +5.94% |
| Render/many_tags | 6,254 | +0.89% | 844 | -2.99% | 0 |  | 103 | -36.02% |
| Load/mitsuhiko_table | 23 | -52.08% | 18 | -58.14% | 0 |  | 810 | +0.12% |
| Render/mitsuhiko_table | 10,093 | -0.09% | 6,260 | -0.59% | 0 |  | 353 | -0.84% |
| RenderSink/mitsuhiko_table | 9,953 | -0.58% | 1,203 | -0.33% | 0 |  | 294 | +1.03% |
| RenderTo/mitsuhiko_table | 10,054 | -0.38% | 6,257 | -0.67% | 0 |  | 320 | -1.84% |
| Load/mitsuhiko_table_wide | 49 | -31.94% | 52 | -35.80% | 0 |  | 985 | +0.20% |
| Render/mitsuhiko_table_wide | 10,285 | +0.04% | 22,670 | +0.21% | 0 |  | 370 | +8.19% |
| RenderSink/mitsuhiko_table_wide | 10,070 | -0.64% | 1,299 | +0.85% | 0 |  | 294 | +7.30% |
| RenderTo/mitsuhiko_table_wide | 10,292 | 0% | 22,680 | -0.07% | 0 |  | 341 | +3.02% |
| Load/plain_text | 0 | -100.00% | 2 | 0% | 0 |  | 6 | -14.29% |
| Render/plain_text | 0 | -100.00% | 0 |  | 0 |  | 6 | +20.00% |
| RenderSink/plain_text | 0 | -100.00% | 0 |  | 0 |  | 5 | +25.00% |
| RenderTo/plain_text | 0 | -100.00% | 0 |  | 0 |  | 6 | +50.00% |
| Load/strings | 125 | -30.94% | 74 | -35.09% | 0 |  | 2,652 | -1.34% |
| Render/strings | 414 | -6.12% | 459 | -2.34% | 0 |  | 1,551 | -23.07% |
| Load/substitute | 2 | -66.67% | 3 | -40.00% | 0 |  | 123 | -15.17% |
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
