# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (199 so far, latest [4de982c](https://github.com/jinja2cpp/Jinja2Cpp/commit/4de982c2d221c652a4a4749ed19b18fbfa396363) on 2026-11-10). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 342,578 | 0% |  | 55 | 0% |  | 18,480 | 0% | -52.71% |
| [Render/chat_llama](#renderchat_llama) | 296,242 | 0% |  | 225 | 0% |  | 11,616 | 0% | -19.11% |
| [Load/chat_mistral](#loadchat_mistral) | 392,421 | 0% |  | 52 | 0% |  | 20,888 | 0% | -53.22% |
| [Render/chat_mistral](#renderchat_mistral) | 500,501 | +0.01% |  | 402 | 0% |  | 9,984 | 0% | -24.41% |
| [Load/chat_qwen](#loadchat_qwen) | 266,022 | 0% |  | 44 | 0% |  | 14,416 | 0% | -47.25% |
| [Render/chat_qwen](#renderchat_qwen) | 277,473 | 0% |  | 212 | 0% |  | 8,304 | 0% | -2.35% |
| [Load/config_file](#loadconfig_file) | 96,929 | 0% |  | 25 | 0% |  | 5,464 | 0% | -48.26% |
| [Render/config_file](#renderconfig_file) | 1,276,206 | +0.01% |  | 908 | 0% |  | 24,440 | 0% | +2.65% |
| [Load/dict_ops](#loaddict_ops) | 33,130 | 0% | -77.58% | 9 | 0% | -93.13% | 2,176 | 0% | -48.87% |
| [Render/dict_ops](#renderdict_ops) | 293,079 | 0% | -57.58% | 112 | 0% | -75.11% | 24,408 | 0% | -29.65% |
| [Load/expressions](#loadexpressions) | 44,973 | 0% | -78.18% | 16 | 0% | -89.04% | 2,728 | 0% | -55.54% |
| [Render/expressions](#renderexpressions) | 426,900 | 0% | -52.89% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 69,062 | 0% | -74.84% | 27 | 0% | -88.16% | 3,848 | 0% | -66.29% |
| [Render/filters](#renderfilters) | 30,470 | 0% | -65.64% | 26 | 0% | -64.86% | 1,080 | 0% | -78.01% |
| [Load/for_filter_if](#loadfor_filter_if) | 29,878 | 0% | -77.93% | 7 | 0% | -94.31% | 1,904 | 0% | -44.78% |
| [Render/for_filter_if](#renderfor_filter_if) | 274,386 | 0% | -68.30% | 4 | 0% | -98.23% | 16,560 | 0% | +354.95% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 40,318 | 0% | -77.68% | 10 | 0% | -93.63% | 2,368 | 0% | -47.61% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 180,553 | 0% | -83.17% | 2 | 0% | -99.62% | 1,568 | 0% | -43.35% |
| [Load/for_range](#loadfor_range) | 13,563 | 0% | -76.07% | 7 | 0% | -87.50% | 1,088 | 0% | -45.60% |
| [Render/for_range](#renderfor_range) | 38,351 | 0% | -58.41% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/hf_deepseek_v31](#loadhf_deepseek_v31) | 385,440 | 0% |  | 55 | 0% |  | 21,976 | 0% | +0.73% |
| [Render/hf_deepseek_v31](#renderhf_deepseek_v31) | 91,263 | +0.07% |  | 43 | 0% |  | 2,480 | 0% | 0% |
| [Load/hf_gpt_oss](#loadhf_gpt_oss) | 1,539,498 | 0% |  | 287 | 0% |  | 74,672 | 0% | +0.60% |
| [Render/hf_gpt_oss](#renderhf_gpt_oss) | 171,270 | 0% |  | 155 | 0% |  | 5,080 | 0% | +2.92% |
| [Load/hf_mistral_small](#loadhf_mistral_small) | 597,385 | 0% |  | 88 | 0% |  | 33,048 | 0% | +0.34% |
| [Render/hf_mistral_small](#renderhf_mistral_small) | 144,951 | +0.02% |  | 105 | 0% |  | 6,136 | 0% | 0% |
| [Load/hf_qwen3](#loadhf_qwen3) | 484,626 | 0% |  | 82 | 0% |  | 25,560 | 0% | +0.31% |
| [Render/hf_qwen3](#renderhf_qwen3) | 119,985 | 0% |  | 100 | 0% |  | 3,248 | 0% | 0% |
| [Load/html_autoescape](#loadhtml_autoescape) | 194,319 | 0% |  | 43 | 0% |  | 12,640 | 0% | -48.78% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,207,225 | 0% |  | 674 | 0% |  | 45,408 | 0% | +12.44% |
| [Load/inheritance](#loadinheritance) | 22,615 | 0% | -80.18% | 7 | 0% | -93.46% | 1,344 | 0% | -53.20% |
| [Render/inheritance](#renderinheritance) | 137,429 | 0% | -81.62% | 35 | 0% | -94.37% | 7,384 | 0% | +22.74% |
| [Load/int_arith](#loadint_arith) | 66,206 | 0% |  | 20 | 0% |  | 3,872 | 0% | +0.41% |
| [Render/int_arith](#renderint_arith) | 826,144 | +0.07% |  | 12 | 0% |  | 3,272 | 0% | -0.49% |
| [Module/keep](#modulekeep) | 7,045 | -0.31% |  | 7 | 0% |  |  |  |  |
| [Module/keep_written](#modulekeep_written) | 28,905 | -20.85% |  | 11 | 0% |  |  |  |  |
| [Load/large_static](#loadlarge_static) | 153,584 | 0% | -82.67% | 14 | 0% | -96.82% | 43,120 | 0% | -19.23% |
| [Render/large_static](#renderlarge_static) | 20,664 | 0% | -72.50% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [RenderSink/large_static](#rendersinklarge_static) | 12,702 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/large_static](#rendertolarge_static) | 20,275 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/macros](#loadmacros) | 56,363 | 0% | -75.27% | 27 | 0% | -85.48% | 4,392 | 0% | -29.34% |
| [Render/macros](#rendermacros) | 657,501 | 0% | -74.35% | 308 | 0% | -88.71% | 20,560 | 0% | +39.67% |
| [Load/many_tags](#loadmany_tags) | 8,997,878 | 0% | -79.63% | 951 | 0% | -97.22% | 438,128 | 0% | -58.48% |
| [Render/many_tags](#rendermany_tags) | 727,198 | 0% | -65.09% | 2 | 0% | -90.48% | 5,648 | 0% | -10.52% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 46,463 | 0% | -80.44% | 19 | 0% | -89.67% | 3,360 | 0% | -44.59% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,199,644 | 0% | -57.92% | 33 | 0% | -99.19% | 381,536 | 0% | +9.95% |
| [RenderSink/mitsuhiko_table](#rendersinkmitsuhiko_table) | 5,126,839 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table](#rendertomitsuhiko_table) | 5,199,150 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 54,242 | 0% |  | 35 | 0% |  | 5,688 | 0% | -31.17% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,545,812 | 0% |  | 33 | 0% |  | 1,414,928 | 0% | +2.46% |
| [RenderSink/mitsuhiko_table_wide](#rendersinkmitsuhiko_table_wide) | 5,369,103 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table_wide](#rendertomitsuhiko_table_wide) | 5,545,364 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/plain_text](#loadplain_text) | 3,737 | 0% | -61.38% | 4 | 0% | -77.78% | 672 | 0% | -40.00% |
| [Render/plain_text](#renderplain_text) | 1,022 | 0% | -84.96% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/plain_text](#rendersinkplain_text) | 724 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/plain_text](#rendertoplain_text) | 823 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/prompt_budget](#loadprompt_budget) | 35,140 | 0% |  | 18 | 0% |  | 2,224 | 0% | 0% |
| [Render/prompt_budget](#renderprompt_budget) | 1,315,570 | 0% |  | 76 | 0% |  | 36,416 | 0% | 0% |
| [Load/strings](#loadstrings) | 70,612 | 0% | -75.81% | 30 | 0% | -86.61% | 4,344 | 0% | -51.95% |
| [Render/strings](#renderstrings) | 580,473 | +0.09% | -72.33% | 344 | 0% | -83.73% | 9,088 | 0% | -49.31% |
| [Load/substitute](#loadsubstitute) | 7,712 | 0% | -74.17% | 6 | 0% | -83.78% | 832 | 0% | -45.26% |
| [Render/substitute](#rendersubstitute) | 2,255 | 0% | -74.41% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/substitute](#rendersinksubstitute) | 1,961 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/substitute](#rendertosubstitute) | 2,060 | 0% |  | 0 |  |  | 0 |  |  |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 725 | -0.14% | 732 | 0% | 0 |  | 7,603 | +6.93% |
| Render/chat_llama | 1,204 | +0.75% | 510 | +0.59% | 0 |  | 4,020 | +3.08% |
| Load/chat_mistral | 1,052 | -0.09% | 1,017 | 0% | 0 |  | 8,437 | +8.91% |
| Render/chat_mistral | 1,932 | +0.05% | 578 | +0.35% | 0 |  | 9,936 | +3.25% |
| Load/chat_qwen | 514 | 0% | 572 | -0.17% | 0 |  | 4,541 | +11.22% |
| Render/chat_qwen | 1,088 | +0.93% | 401 | +1.01% | 0 |  | 3,488 | +6.41% |
| Load/config_file | 229 | 0% | 183 | 0% | 0 |  | 2,774 | +7.27% |
| Render/config_file | 6,045 | -0.44% | 1,512 | -1.56% | 0 |  | 41,263 | -3.67% |
| Load/dict_ops | 37 | 0% | 18 | 0% | 0 |  | 1,390 | +4.67% |
| Render/dict_ops | 673 | 0% | 426 | 0% | 0 |  | 890 | +1.02% |
| Load/expressions | 36 | 0% | 23 | 0% | 0 |  | 1,355 | +5.61% |
| Render/expressions | 16 | 0% | 5 | 0% | 0 |  | 310 | +6.16% |
| Load/filters | 86 | 0% | 53 | 0% | 0 |  | 1,558 | -10.97% |
| Render/filters | 17 | 0% | 6 | 0% | 0 |  | 784 | +1.03% |
| Load/for_filter_if | 28 | 0% | 18 | 0% | 0 |  | 1,109 | +6.63% |
| Render/for_filter_if | 1,727 | -0.06% | 133 | -0.75% | 0 |  | 171 | -7.07% |
| Load/for_loop_vars | 50 | 0% | 23 | 0% | 0 |  | 1,148 | +8.20% |
| Render/for_loop_vars | 17 | 0% | 4 | 0% | 0 |  | 116 | -19.44% |
| Load/for_range | 4 | 0% | 3 | 0% | 0 |  | 774 | +3.61% |
| Render/for_range | 8 | 0% | 0 |  | 0 |  | 138 | +2.22% |
| Load/hf_deepseek_v31 | 989 | 0% | 953 | -0.10% | 0 |  | 5,946 | +8.68% |
| Render/hf_deepseek_v31 | 356 | +1.14% | 75 | 0% | 0 |  | 1,393 | +2.65% |
| Load/hf_gpt_oss | 4,640 | 0% | 3,268 | -0.03% | 0 |  | 22,873 | +15.67% |
| Render/hf_gpt_oss | 1,189 | +0.51% | 372 | 0% | 0 |  | 4,499 | +0.20% |
| Load/hf_mistral_small | 1,814 | -0.06% | 1,574 | 0% | 0 |  | 11,081 | +9.55% |
| Render/hf_mistral_small | 1,006 | +0.60% | 317 | +0.32% | 0 |  | 3,764 | +0.56% |
| Load/hf_qwen3 | 1,457 | 0% | 1,258 | -0.08% | 0 |  | 8,962 | +10.89% |
| Render/hf_qwen3 | 670 | +0.90% | 215 | +0.47% | 0 |  | 2,573 | +1.10% |
| Load/html_autoescape | 471 | 0% | 501 | +0.20% | 0 |  | 4,069 | +10.33% |
| Render/html_autoescape | 4,941 | -0.06% | 1,565 | -0.19% | 0 |  | 36,671 | +1.06% |
| Load/inheritance | 3 | 0% | 3 | 0% | 0 |  | 723 | +7.43% |
| Render/inheritance | 585 | -0.17% | 92 | +2.22% | 0 |  | 942 | +1.40% |
| Load/int_arith | 75 | 0% | 31 | 0% | 0 |  | 2,161 | +9.64% |
| Render/int_arith | 20 | 0% | 6 | 0% | 0 |  | 745 | -2.10% |
| Module/keep | 1 | -50.00% | 2 | 0% | 0 |  | 432 | -6.90% |
| Module/keep_written | 1,127 | -61.64% | 561 | 0% | 0 |  | 709 | -3.54% |
| Load/large_static | 1,931 | 0% | 910 | 0% | 0 |  | 276 | +3.37% |
| Render/large_static | 712 | 0% | 573 | 0% | 0 |  | 5 | 0% |
| RenderSink/large_static | 0 |  | 0 |  | 0 |  | 2 | 0% |
| RenderTo/large_static | 689 | 0% | 568 | 0% | 0 |  | 5 | +66.67% |
| Load/macros | 81 | 0% | 59 | 0% | 0 |  | 1,769 | +6.31% |
| Render/macros | 746 | -0.13% | 235 | -0.42% | 0 |  | 1,838 | -1.61% |
| Load/many_tags | 28,337 | 0% | 22,483 | 0% | 0 |  | 221,522 | -2.88% |
| Render/many_tags | 6,217 | 0% | 866 | 0% | 0 |  | 112 | -17.65% |
| Load/mitsuhiko_table | 46 | 0% | 43 | 0% | 0 |  | 963 | +15.75% |
| Render/mitsuhiko_table | 10,036 | 0% | 6,268 | -0.03% | 0 |  | 371 | -2.88% |
| RenderSink/mitsuhiko_table | 9,955 | -0.01% | 1,178 | -0.17% | 0 |  | 264 | -13.73% |
| RenderTo/mitsuhiko_table | 10,038 | +0.02% | 6,278 | -0.02% | 0 |  | 353 | +1.15% |
| Load/mitsuhiko_table_wide | 73 | 0% | 78 | 0% | 0 |  | 998 | +4.83% |
| Render/mitsuhiko_table_wide | 10,218 | 0% | 22,686 | 0% | 0 |  | 377 | +2.72% |
| RenderSink/mitsuhiko_table_wide | 10,058 | +0.05% | 1,295 | -0.15% | 0 |  | 291 | -7.91% |
| RenderTo/mitsuhiko_table_wide | 10,211 | -0.03% | 22,691 | 0% | 0 |  | 338 | -1.46% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 13 | -18.75% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 7 | +16.67% |
| RenderSink/plain_text | 0 |  | 0 |  | 0 |  | 4 | -20.00% |
| RenderTo/plain_text | 0 |  | 0 |  | 0 |  | 6 | +20.00% |
| Load/prompt_budget | 78 | 0% | 34 | 0% | 0 |  | 1,438 | +4.81% |
| Render/prompt_budget | 4,822 | 0% | 521 | 0% | 0 |  | 850 | -0.82% |
| Load/strings | 149 | 0% | 95 | 0% | 0 |  | 2,724 | +1.19% |
| Render/strings | 428 | 0% | 483 | 0% | 0 |  | 1,589 | +0.76% |
| Load/substitute | 1 | 0% | 3 | 0% | 0 |  | 173 | +73.00% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 5 | +66.67% |
| RenderSink/substitute | 0 |  | 0 |  | 0 |  | 3 | 0% |
| RenderTo/substitute | 0 |  | 0 |  | 0 |  | 5 | +66.67% |

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
