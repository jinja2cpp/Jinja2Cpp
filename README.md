# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (200 so far, latest [1cf228b](https://github.com/jinja2cpp/Jinja2Cpp/commit/1cf228b859bf670c00508195fb8c7f14320f778a) on 2026-11-11). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 342,578 | 0% |  | 55 | 0% |  | 18,480 | 0% | -52.71% |
| [Render/chat_llama](#renderchat_llama) | 296,260 | +0.01% |  | 225 | 0% |  | 11,616 | 0% | -19.11% |
| [Load/chat_mistral](#loadchat_mistral) | 392,421 | 0% |  | 52 | 0% |  | 20,888 | 0% | -53.22% |
| [Render/chat_mistral](#renderchat_mistral) | 500,537 | +0.01% |  | 402 | 0% |  | 9,984 | 0% | -24.41% |
| [Load/chat_qwen](#loadchat_qwen) | 266,022 | 0% |  | 44 | 0% |  | 14,416 | 0% | -47.25% |
| [Render/chat_qwen](#renderchat_qwen) | 277,491 | +0.01% |  | 212 | 0% |  | 8,304 | 0% | -2.35% |
| [Load/config_file](#loadconfig_file) | 96,929 | 0% |  | 25 | 0% |  | 5,464 | 0% | -48.26% |
| [Render/config_file](#renderconfig_file) | 1,276,491 | +0.02% |  | 908 | 0% |  | 24,440 | 0% | +2.65% |
| [Load/dict_ops](#loaddict_ops) | 33,130 | 0% | -77.58% | 9 | 0% | -93.13% | 2,176 | 0% | -48.87% |
| [Render/dict_ops](#renderdict_ops) | 293,085 | 0% | -57.58% | 112 | 0% | -75.11% | 24,408 | 0% | -29.65% |
| [Load/expressions](#loadexpressions) | 44,973 | 0% | -78.18% | 16 | 0% | -89.04% | 2,728 | 0% | -55.54% |
| [Render/expressions](#renderexpressions) | 426,903 | 0% | -52.89% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 69,062 | 0% | -74.84% | 27 | 0% | -88.16% | 3,848 | 0% | -66.29% |
| [Render/filters](#renderfilters) | 30,470 | 0% | -65.64% | 26 | 0% | -64.86% | 1,080 | 0% | -78.01% |
| [Load/for_filter_if](#loadfor_filter_if) | 29,878 | 0% | -77.93% | 7 | 0% | -94.31% | 1,904 | 0% | -44.78% |
| [Render/for_filter_if](#renderfor_filter_if) | 274,386 | 0% | -68.30% | 4 | 0% | -98.23% | 16,560 | 0% | +354.95% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 40,318 | 0% | -77.68% | 10 | 0% | -93.63% | 2,368 | 0% | -47.61% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 180,558 | 0% | -83.17% | 2 | 0% | -99.62% | 1,568 | 0% | -43.35% |
| [Load/for_range](#loadfor_range) | 13,563 | 0% | -76.07% | 7 | 0% | -87.50% | 1,088 | 0% | -45.60% |
| [Render/for_range](#renderfor_range) | 38,354 | +0.01% | -58.41% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/hf_deepseek_v31](#loadhf_deepseek_v31) | 385,440 | 0% |  | 55 | 0% |  | 21,976 | 0% | +0.73% |
| [Render/hf_deepseek_v31](#renderhf_deepseek_v31) | 91,288 | +0.03% |  | 43 | 0% |  | 2,480 | 0% | 0% |
| [Load/hf_gpt_oss](#loadhf_gpt_oss) | 1,539,498 | 0% |  | 287 | 0% |  | 74,672 | 0% | +0.60% |
| [Render/hf_gpt_oss](#renderhf_gpt_oss) | 171,285 | +0.01% |  | 155 | 0% |  | 5,080 | 0% | +2.92% |
| [Load/hf_mistral_small](#loadhf_mistral_small) | 597,385 | 0% |  | 88 | 0% |  | 33,048 | 0% | +0.34% |
| [Render/hf_mistral_small](#renderhf_mistral_small) | 144,963 | +0.01% |  | 105 | 0% |  | 6,136 | 0% | 0% |
| [Load/hf_qwen3](#loadhf_qwen3) | 484,626 | 0% |  | 82 | 0% |  | 25,560 | 0% | +0.31% |
| [Render/hf_qwen3](#renderhf_qwen3) | 119,999 | +0.01% |  | 100 | 0% |  | 3,248 | 0% | 0% |
| [Load/html_autoescape](#loadhtml_autoescape) | 194,319 | 0% |  | 43 | 0% |  | 12,640 | 0% | -48.78% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,207,234 | 0% |  | 674 | 0% |  | 45,408 | 0% | +12.44% |
| [Load/inheritance](#loadinheritance) | 22,615 | 0% | -80.18% | 7 | 0% | -93.46% | 1,344 | 0% | -53.20% |
| [Render/inheritance](#renderinheritance) | 137,432 | 0% | -81.62% | 35 | 0% | -94.37% | 7,384 | 0% | +22.74% |
| [Load/int_arith](#loadint_arith) | 66,206 | 0% |  | 20 | 0% |  | 3,872 | 0% | +0.41% |
| [Render/int_arith](#renderint_arith) | 826,347 | +0.02% |  | 12 | 0% |  | 3,272 | 0% | -0.49% |
| [Module/keep](#modulekeep) | 7,045 | 0% |  | 7 | 0% |  |  |  |  |
| [Module/keep_written](#modulekeep_written) | 28,905 | 0% |  | 11 | 0% |  |  |  |  |
| [Load/large_static](#loadlarge_static) | 153,584 | 0% | -82.67% | 14 | 0% | -96.82% | 43,120 | 0% | -19.23% |
| [Render/large_static](#renderlarge_static) | 20,664 | 0% | -72.50% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [RenderSink/large_static](#rendersinklarge_static) | 12,702 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/large_static](#rendertolarge_static) | 20,275 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/macros](#loadmacros) | 56,363 | 0% | -75.27% | 27 | 0% | -85.48% | 4,392 | 0% | -29.34% |
| [Render/macros](#rendermacros) | 657,504 | 0% | -74.35% | 308 | 0% | -88.71% | 20,560 | 0% | +39.67% |
| [Load/many_tags](#loadmany_tags) | 8,997,878 | 0% | -79.63% | 951 | 0% | -97.22% | 438,128 | 0% | -58.48% |
| [Render/many_tags](#rendermany_tags) | 727,198 | 0% | -65.09% | 2 | 0% | -90.48% | 5,648 | 0% | -10.52% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 46,463 | 0% | -80.44% | 19 | 0% | -89.67% | 3,360 | 0% | -44.59% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,202,650 | +0.06% | -57.89% | 33 | 0% | -99.19% | 381,536 | 0% | +9.95% |
| [RenderSink/mitsuhiko_table](#rendersinkmitsuhiko_table) | 5,129,845 | +0.06% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table](#rendertomitsuhiko_table) | 5,202,156 | +0.06% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 54,242 | 0% |  | 35 | 0% |  | 5,688 | 0% | -31.17% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,548,818 | +0.05% |  | 33 | 0% |  | 1,414,928 | 0% | +2.46% |
| [RenderSink/mitsuhiko_table_wide](#rendersinkmitsuhiko_table_wide) | 5,372,109 | +0.06% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table_wide](#rendertomitsuhiko_table_wide) | 5,548,370 | +0.05% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/plain_text](#loadplain_text) | 3,737 | 0% | -61.38% | 4 | 0% | -77.78% | 672 | 0% | -40.00% |
| [Render/plain_text](#renderplain_text) | 1,022 | 0% | -84.96% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/plain_text](#rendersinkplain_text) | 724 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/plain_text](#rendertoplain_text) | 823 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/prompt_budget](#loadprompt_budget) | 35,140 | 0% |  | 18 | 0% |  | 2,224 | 0% | 0% |
| [Render/prompt_budget](#renderprompt_budget) | 1,315,573 | 0% |  | 76 | 0% |  | 36,416 | 0% | 0% |
| [Load/strings](#loadstrings) | 70,612 | 0% | -75.81% | 30 | 0% | -86.61% | 4,344 | 0% | -51.95% |
| [Render/strings](#renderstrings) | 580,587 | +0.02% | -72.32% | 344 | 0% | -83.73% | 9,088 | 0% | -49.31% |
| [Load/substitute](#loadsubstitute) | 7,712 | 0% | -74.17% | 6 | 0% | -83.78% | 832 | 0% | -45.26% |
| [Render/substitute](#rendersubstitute) | 2,255 | 0% | -74.41% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/substitute](#rendersinksubstitute) | 1,961 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/substitute](#rendertosubstitute) | 2,060 | 0% |  | 0 |  |  | 0 |  |  |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 732 | +0.97% | 726 | -0.82% | 0 |  | 7,236 | -4.83% |
| Render/chat_llama | 1,174 | -2.49% | 512 | +0.39% | 0 |  | 4,271 | +6.24% |
| Load/chat_mistral | 1,055 | +0.29% | 1,018 | +0.10% | 0 |  | 8,067 | -4.39% |
| Render/chat_mistral | 1,930 | -0.10% | 576 | -0.35% | 0 |  | 10,709 | +7.78% |
| Load/chat_qwen | 520 | +1.17% | 574 | +0.35% | 0 |  | 4,254 | -6.32% |
| Render/chat_qwen | 1,075 | -1.19% | 394 | -1.75% | 0 |  | 3,540 | +1.49% |
| Load/config_file | 232 | +1.31% | 186 | +1.64% | 0 |  | 2,623 | -5.44% |
| Render/config_file | 6,300 | +4.22% | 1,552 | +2.65% | 0 |  | 40,494 | -1.86% |
| Load/dict_ops | 37 | 0% | 16 | -11.11% | 0 |  | 1,310 | -5.76% |
| Render/dict_ops | 675 | +0.30% | 426 | 0% | 0 |  | 884 | -0.67% |
| Load/expressions | 41 | +13.89% | 24 | +4.35% | 0 |  | 1,286 | -5.09% |
| Render/expressions | 13 | -18.75% | 6 | +20.00% | 0 |  | 336 | +8.39% |
| Load/filters | 88 | +2.33% | 52 | -1.89% | 0 |  | 1,434 | -7.96% |
| Render/filters | 14 | -17.65% | 6 | 0% | 0 |  | 783 | -0.13% |
| Load/for_filter_if | 30 | +7.14% | 19 | +5.56% | 0 |  | 1,072 | -3.34% |
| Render/for_filter_if | 1,730 | +0.17% | 133 | 0% | 0 |  | 210 | +22.81% |
| Load/for_loop_vars | 47 | -6.00% | 24 | +4.35% | 0 |  | 1,075 | -6.36% |
| Render/for_loop_vars | 14 | -17.65% | 2 | -50.00% | 0 |  | 181 | +56.03% |
| Load/for_range | 3 | -25.00% | 3 | 0% | 0 |  | 754 | -2.58% |
| Render/for_range | 1 | -87.50% | 0 |  | 0 |  | 167 | +21.01% |
| Load/hf_deepseek_v31 | 1,004 | +1.52% | 957 | +0.42% | 0 |  | 5,687 | -4.36% |
| Render/hf_deepseek_v31 | 362 | +1.69% | 69 | -8.00% | 0 |  | 1,692 | +21.46% |
| Load/hf_gpt_oss | 4,664 | +0.52% | 3,272 | +0.12% | 0 |  | 21,254 | -7.08% |
| Render/hf_gpt_oss | 1,204 | +1.26% | 368 | -1.08% | 0 |  | 4,510 | +0.24% |
| Load/hf_mistral_small | 1,825 | +0.61% | 1,570 | -0.25% | 0 |  | 10,287 | -7.17% |
| Render/hf_mistral_small | 1,008 | +0.20% | 314 | -0.95% | 0 |  | 3,931 | +4.44% |
| Load/hf_qwen3 | 1,464 | +0.48% | 1,257 | -0.08% | 0 |  | 8,413 | -6.13% |
| Render/hf_qwen3 | 683 | +1.94% | 209 | -2.79% | 0 |  | 2,539 | -1.32% |
| Load/html_autoescape | 480 | +1.91% | 505 | +0.80% | 0 |  | 3,705 | -8.95% |
| Render/html_autoescape | 4,870 | -1.44% | 1,568 | +0.19% | 0 |  | 35,483 | -3.24% |
| Load/inheritance | 3 | 0% | 2 | -33.33% | 0 |  | 697 | -3.60% |
| Render/inheritance | 586 | +0.17% | 93 | +1.09% | 0 |  | 935 | -0.74% |
| Load/int_arith | 74 | -1.33% | 35 | +12.90% | 0 |  | 1,938 | -10.32% |
| Render/int_arith | 21 | +5.00% | 5 | -16.67% | 0 |  | 722 | -3.09% |
| Module/keep | 1 | 0% | 2 | 0% | 0 |  | 423 | -2.08% |
| Module/keep_written | 1,128 | +0.09% | 561 | 0% | 0 |  | 722 | +1.83% |
| Load/large_static | 1,934 | +0.16% | 910 | 0% | 0 |  | 293 | +6.16% |
| Render/large_static | 712 | 0% | 573 | 0% | 0 |  | 8 | +60.00% |
| RenderSink/large_static | 0 |  | 0 |  | 0 |  | 3 | +50.00% |
| RenderTo/large_static | 689 | 0% | 568 | 0% | 0 |  | 5 | 0% |
| Load/macros | 83 | +2.47% | 58 | -1.69% | 0 |  | 1,700 | -3.90% |
| Render/macros | 749 | +0.40% | 236 | +0.43% | 0 |  | 627 | -65.89% |
| Load/many_tags | 28,501 | +0.58% | 22,492 | +0.04% | 0 |  | 210,102 | -5.16% |
| Render/many_tags | 6,227 | +0.16% | 865 | -0.12% | 0 |  | 108 | -3.57% |
| Load/mitsuhiko_table | 46 | 0% | 44 | +2.33% | 0 |  | 917 | -4.78% |
| Render/mitsuhiko_table | 10,044 | +0.08% | 6,270 | +0.03% | 0 |  | 378 | +1.89% |
| RenderSink/mitsuhiko_table | 9,964 | +0.09% | 1,179 | +0.08% | 0 |  | 299 | +13.26% |
| RenderTo/mitsuhiko_table | 10,049 | +0.11% | 6,280 | +0.03% | 0 |  | 336 | -4.82% |
| Load/mitsuhiko_table_wide | 81 | +10.96% | 82 | +5.13% | 0 |  | 915 | -8.32% |
| Render/mitsuhiko_table_wide | 10,226 | +0.08% | 22,684 | -0.01% | 0 |  | 346 | -8.22% |
| RenderSink/mitsuhiko_table_wide | 10,055 | -0.03% | 1,292 | -0.23% | 0 |  | 293 | +0.69% |
| RenderTo/mitsuhiko_table_wide | 10,233 | +0.22% | 22,688 | -0.01% | 0 |  | 331 | -2.07% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 6 | -53.85% |
| Render/plain_text | 1 |  | 0 |  | 0 |  | 7 | 0% |
| RenderSink/plain_text | 0 |  | 0 |  | 0 |  | 5 | +25.00% |
| RenderTo/plain_text | 1 |  | 0 |  | 0 |  | 7 | +16.67% |
| Load/prompt_budget | 76 | -2.56% | 32 | -5.88% | 0 |  | 1,333 | -7.30% |
| Render/prompt_budget | 4,826 | +0.08% | 521 | 0% | 0 |  | 864 | +1.65% |
| Load/strings | 150 | +0.67% | 93 | -2.11% | 0 |  | 2,652 | -2.64% |
| Render/strings | 431 | +0.70% | 484 | +0.21% | 0 |  | 1,568 | -1.32% |
| Load/substitute | 2 | +100.00% | 3 | 0% | 0 |  | 162 | -6.36% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 5 | 0% |
| RenderSink/substitute | 0 |  | 0 |  | 0 |  | 4 | +33.33% |
| RenderTo/substitute | 0 |  | 0 |  | 0 |  | 5 | 0% |

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
