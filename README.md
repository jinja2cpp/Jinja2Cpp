# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (197 so far, latest [5b6bed9](https://github.com/jinja2cpp/Jinja2Cpp/commit/5b6bed98ff0aa6456fa1493a899f19614f487522) on 2026-11-10). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 342,571 | -0.82% |  | 55 | 0% |  | 18,480 | 0% | -52.71% |
| [Render/chat_llama](#renderchat_llama) | 296,245 | +0.01% |  | 225 | 0% |  | 11,616 | 0% | -19.11% |
| [Load/chat_mistral](#loadchat_mistral) | 392,435 | -0.50% |  | 52 | 0% |  | 20,888 | 0% | -53.22% |
| [Render/chat_mistral](#renderchat_mistral) | 500,443 | +0.03% |  | 402 | 0% |  | 9,984 | 0% | -24.41% |
| [Load/chat_qwen](#loadchat_qwen) | 266,021 | -0.26% |  | 44 | 0% |  | 14,416 | 0% | -47.25% |
| [Render/chat_qwen](#renderchat_qwen) | 277,461 | +0.01% |  | 212 | 0% |  | 8,304 | 0% | -2.35% |
| [Load/config_file](#loadconfig_file) | 96,906 | -0.38% |  | 25 | 0% |  | 5,464 | 0% | -48.26% |
| [Render/config_file](#renderconfig_file) | 1,276,105 | +0.06% |  | 908 | 0% |  | 24,440 | 0% | +2.65% |
| [Load/dict_ops](#loaddict_ops) | 33,137 | -0.69% | -77.58% | 9 | 0% | -93.13% | 2,176 | 0% | -48.87% |
| [Render/dict_ops](#renderdict_ops) | 293,079 | 0% | -57.58% | 112 | 0% | -75.11% | 24,408 | 0% | -29.65% |
| [Load/expressions](#loadexpressions) | 44,973 | -0.16% | -78.18% | 16 | 0% | -89.04% | 2,728 | 0% | -55.54% |
| [Render/expressions](#renderexpressions) | 426,900 | +0.16% | -52.89% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 68,995 | -0.04% | -74.86% | 27 | 0% | -88.16% | 3,848 | 0% | -66.29% |
| [Render/filters](#renderfilters) | 30,470 | +0.02% | -65.64% | 26 | 0% | -64.86% | 1,080 | 0% | -78.01% |
| [Load/for_filter_if](#loadfor_filter_if) | 29,878 | -0.56% | -77.93% | 7 | 0% | -94.31% | 1,904 | 0% | -44.78% |
| [Render/for_filter_if](#renderfor_filter_if) | 274,384 | 0% | -68.30% | 4 | 0% | -98.23% | 16,560 | 0% | +354.95% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 40,318 | -0.25% | -77.68% | 10 | 0% | -93.63% | 2,368 | 0% | -47.61% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 180,551 | 0% | -83.17% | 2 | 0% | -99.62% | 1,568 | 0% | -43.35% |
| [Load/for_range](#loadfor_range) | 13,563 | -0.75% | -76.07% | 7 | 0% | -87.50% | 1,088 | 0% | -45.60% |
| [Render/for_range](#renderfor_range) | 38,351 | 0% | -58.41% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/hf_deepseek_v31](#loadhf_deepseek_v31) | 385,446 | -0.54% |  | 55 | 0% |  | 21,976 | 0% | +0.73% |
| [Render/hf_deepseek_v31](#renderhf_deepseek_v31) | 90,921 | +0.02% |  | 43 | 0% |  | 2,480 | 0% | 0% |
| [Load/hf_gpt_oss](#loadhf_gpt_oss) | 1,539,486 | -0.66% |  | 287 | 0% |  | 74,672 | 0% | +0.60% |
| [Render/hf_gpt_oss](#renderhf_gpt_oss) | 171,270 | +0.04% |  | 155 | 0% |  | 5,080 | 0% | +2.92% |
| [Load/hf_mistral_small](#loadhf_mistral_small) | 597,385 | -0.32% |  | 88 | 0% |  | 33,048 | 0% | +0.34% |
| [Render/hf_mistral_small](#renderhf_mistral_small) | 144,928 | 0% |  | 105 | 0% |  | 6,136 | 0% | 0% |
| [Load/hf_qwen3](#loadhf_qwen3) | 484,616 | -0.36% |  | 82 | 0% |  | 25,560 | 0% | +0.31% |
| [Render/hf_qwen3](#renderhf_qwen3) | 119,984 | +0.03% |  | 100 | 0% |  | 3,248 | 0% | 0% |
| [Load/html_autoescape](#loadhtml_autoescape) | 194,319 | +0.12% |  | 43 | 0% |  | 12,640 | 0% | -48.78% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,207,219 | 0% |  | 674 | 0% |  | 45,408 | 0% | +12.44% |
| [Load/inheritance](#loadinheritance) | 22,615 | -2.08% | -80.18% | 7 | 0% | -93.46% | 1,344 | 0% | -53.20% |
| [Render/inheritance](#renderinheritance) | 137,427 | 0% | -81.62% | 35 | 0% | -94.37% | 7,384 | 0% | +22.74% |
| [Load/int_arith](#loadint_arith) | 66,206 | +0.28% |  | 20 | 0% |  | 3,872 | 0% | +0.41% |
| [Render/int_arith](#renderint_arith) | 825,544 | 0% |  | 12 | 0% |  | 3,272 | 0% | -0.49% |
| [Module/keep](#modulekeep) | 7,067 | -0.04% |  | 7 | 0% |  |  |  |  |
| [Module/keep_written](#modulekeep_written) | 36,526 | -0.01% |  | 11 | 0% |  |  |  |  |
| [Load/large_static](#loadlarge_static) | 153,584 | -0.77% | -82.67% | 14 | 0% | -96.82% | 43,120 | 0% | -19.23% |
| [Render/large_static](#renderlarge_static) | 20,664 | 0% | -72.50% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [RenderSink/large_static](#rendersinklarge_static) | 12,702 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/large_static](#rendertolarge_static) | 20,275 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/macros](#loadmacros) | 56,363 | -0.09% | -75.27% | 27 | 0% | -85.48% | 4,392 | 0% | -29.34% |
| [Render/macros](#rendermacros) | 657,499 | 0% | -74.35% | 308 | 0% | -88.71% | 20,560 | 0% | +39.67% |
| [Load/many_tags](#loadmany_tags) | 8,993,078 | -0.29% | -79.64% | 951 | 0% | -97.22% | 438,128 | 0% | -58.48% |
| [Render/many_tags](#rendermany_tags) | 727,198 | 0% | -65.09% | 2 | 0% | -90.48% | 5,648 | 0% | -10.52% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 46,463 | -0.34% | -80.44% | 19 | 0% | -89.67% | 3,360 | 0% | -44.59% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,199,655 | 0% | -57.92% | 33 | 0% | -99.19% | 381,536 | 0% | +9.95% |
| [RenderSink/mitsuhiko_table](#rendersinkmitsuhiko_table) | 5,126,850 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table](#rendertomitsuhiko_table) | 5,199,161 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 54,242 | -1.07% |  | 35 | 0% |  | 5,688 | 0% | -31.17% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,545,823 | 0% |  | 33 | 0% |  | 1,414,928 | 0% | +2.46% |
| [RenderSink/mitsuhiko_table_wide](#rendersinkmitsuhiko_table_wide) | 5,369,114 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table_wide](#rendertomitsuhiko_table_wide) | 5,545,375 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/plain_text](#loadplain_text) | 3,737 | +0.11% | -61.38% | 4 | 0% | -77.78% | 672 | 0% | -40.00% |
| [Render/plain_text](#renderplain_text) | 1,022 | 0% | -84.96% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/plain_text](#rendersinkplain_text) | 724 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/plain_text](#rendertoplain_text) | 823 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/prompt_budget](#loadprompt_budget) | 35,140 | -0.04% |  | 18 | 0% |  | 2,224 | 0% | 0% |
| [Render/prompt_budget](#renderprompt_budget) | 1,315,570 | 0% |  | 76 | 0% |  | 36,416 | 0% | 0% |
| [Load/strings](#loadstrings) | 70,570 | +0.21% | -75.82% | 30 | 0% | -86.61% | 4,344 | 0% | -51.95% |
| [Render/strings](#renderstrings) | 579,933 | 0% | -72.35% | 344 | 0% | -83.73% | 9,088 | 0% | -49.31% |
| [Load/substitute](#loadsubstitute) | 7,712 | -0.72% | -74.17% | 6 | 0% | -83.78% | 832 | 0% | -45.26% |
| [Render/substitute](#rendersubstitute) | 2,255 | 0% | -74.41% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/substitute](#rendersinksubstitute) | 1,961 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/substitute](#rendertosubstitute) | 2,060 | 0% |  | 0 |  |  | 0 |  |  |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 728 | +3.12% | 732 | -0.81% | 0 |  | 7,058 | -7.53% |
| Render/chat_llama | 1,195 | +0.50% | 507 | -0.39% | 0 |  | 3,979 | +4.85% |
| Load/chat_mistral | 1,048 | +2.64% | 1,017 | -1.17% | 0 |  | 7,850 | -8.31% |
| Render/chat_mistral | 1,931 | -0.57% | 577 | 0% | 0 |  | 9,680 | +1.51% |
| Load/chat_qwen | 515 | +0.78% | 573 | -0.52% | 0 |  | 4,113 | -9.76% |
| Render/chat_qwen | 1,078 | -1.82% | 397 | +0.25% | 0 |  | 3,386 | -1.71% |
| Load/config_file | 232 | +8.41% | 182 | +0.55% | 0 |  | 2,583 | -6.99% |
| Render/config_file | 6,054 | +0.25% | 1,525 | -0.72% | 0 |  | 41,717 | -4.06% |
| Load/dict_ops | 37 | +12.12% | 18 | -18.18% | 0 |  | 1,317 | -3.87% |
| Render/dict_ops | 673 | +1.82% | 426 | -0.23% | 0 |  | 871 | -2.79% |
| Load/expressions | 36 | +80.00% | 23 | +64.29% | 0 |  | 1,271 | -5.15% |
| Render/expressions | 16 | 0% | 5 | -28.57% | 0 |  | 295 | -9.79% |
| Load/filters | 87 | 0% | 54 | +10.20% | 0 |  | 1,671 | -0.18% |
| Render/filters | 17 | +13.33% | 6 | -14.29% | 0 |  | 792 | +1.41% |
| Load/for_filter_if | 28 | +3.70% | 18 | +5.88% | 0 |  | 1,004 | -6.08% |
| Render/for_filter_if | 1,728 | -0.17% | 134 | 0% | 0 |  | 173 | -9.42% |
| Load/for_loop_vars | 50 | +42.86% | 23 | +43.75% | 0 |  | 1,061 | -8.38% |
| Render/for_loop_vars | 17 | +325.00% | 4 |  | 0 |  | 166 | -2.35% |
| Load/for_range | 4 | 0% | 3 | -25.00% | 0 |  | 737 | -5.15% |
| Render/for_range | 8 | +700.00% | 0 |  | 0 |  | 117 | -4.10% |
| Load/hf_deepseek_v31 | 990 | +0.71% | 954 | -0.93% | 0 |  | 5,447 | -13.32% |
| Render/hf_deepseek_v31 | 350 | -4.11% | 75 | +4.17% | 0 |  | 1,312 | -2.09% |
| Load/hf_gpt_oss | 4,631 | +1.09% | 3,269 | +0.03% | 0 |  | 19,609 | -14.29% |
| Render/hf_gpt_oss | 1,183 | -1.58% | 372 | +0.54% | 0 |  | 4,507 | +0.54% |
| Load/hf_mistral_small | 1,811 | +1.86% | 1,575 | -0.88% | 0 |  | 10,067 | -9.20% |
| Render/hf_mistral_small | 1,000 | +0.40% | 316 | +0.96% | 0 |  | 3,711 | -1.20% |
| Load/hf_qwen3 | 1,457 | +1.89% | 1,258 | -0.32% | 0 |  | 8,246 | -7.59% |
| Render/hf_qwen3 | 664 | -2.50% | 214 | +1.90% | 0 |  | 2,543 | -2.04% |
| Load/html_autoescape | 472 | +0.85% | 500 | -1.77% | 0 |  | 3,637 | -10.22% |
| Render/html_autoescape | 4,944 | +7.38% | 1,568 | +2.42% | 0 |  | 35,798 | +0.43% |
| Load/inheritance | 3 | -25.00% | 3 | 0% | 0 |  | 693 | -2.12% |
| Render/inheritance | 586 | +1.38% | 90 | -1.10% | 0 |  | 926 | +1.76% |
| Load/int_arith | 75 | +11.94% | 31 | -20.51% | 0 |  | 1,946 | -4.70% |
| Render/int_arith | 20 | -13.04% | 6 | -14.29% | 0 |  | 718 | -5.28% |
| Module/keep | 2 | +100.00% | 2 | 0% | 0 |  | 474 | +9.98% |
| Module/keep_written | 2,937 | +0.14% | 561 | +0.36% | 0 |  | 726 | -0.27% |
| Load/large_static | 1,931 | 0% | 910 | +0.44% | 0 |  | 279 | -13.62% |
| Render/large_static | 712 | 0% | 573 | 0% | 0 |  | 5 | +25.00% |
| RenderSink/large_static | 0 |  | 0 |  | 0 |  | 2 | -50.00% |
| RenderTo/large_static | 689 | 0% | 568 | 0% | 0 |  | 4 | 0% |
| Load/macros | 78 | +14.71% | 59 | +9.26% | 0 |  | 1,649 | -3.85% |
| Render/macros | 747 | -0.80% | 236 | +0.85% | 0 |  | 3,253 | +69.16% |
| Load/many_tags | 28,333 | +1.23% | 22,481 | +0.12% | 0 |  | 218,863 | -9.02% |
| Render/many_tags | 6,217 | -0.02% | 866 | -0.12% | 0 |  | 130 | +9.24% |
| Load/mitsuhiko_table | 46 | +27.78% | 43 | +10.26% | 0 |  | 883 | -2.86% |
| Render/mitsuhiko_table | 10,036 | -0.07% | 6,270 | -0.02% | 0 |  | 375 | +8.38% |
| RenderSink/mitsuhiko_table | 9,956 | -0.09% | 1,180 | +0.25% | 0 |  | 308 | +9.61% |
| RenderTo/mitsuhiko_table | 10,036 | -0.14% | 6,279 | -0.13% | 0 |  | 348 | +7.74% |
| Load/mitsuhiko_table_wide | 73 | +1.39% | 78 | -14.29% | 0 |  | 1,031 | +2.38% |
| Render/mitsuhiko_table_wide | 10,218 | -0.23% | 22,686 | -0.01% | 0 |  | 338 | -2.59% |
| RenderSink/mitsuhiko_table_wide | 10,053 | -0.12% | 1,297 | -0.38% | 0 |  | 300 | +3.81% |
| RenderTo/mitsuhiko_table_wide | 10,214 | -0.33% | 22,690 | 0% | 0 |  | 317 | -6.49% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 16 | +14.29% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 6 | +20.00% |
| RenderSink/plain_text | 0 |  | 0 |  | 0 |  | 5 | +66.67% |
| RenderTo/plain_text | 0 |  | 0 |  | 0 |  | 6 | +20.00% |
| Load/prompt_budget | 71 | -4.05% | 33 | -10.81% | 0 |  | 1,371 | -1.08% |
| Render/prompt_budget | 4,822 | +0.12% | 521 | -0.76% | 0 |  | 1,050 | -6.33% |
| Load/strings | 149 | +15.50% | 95 | +3.26% | 0 |  | 2,692 | -3.37% |
| Render/strings | 428 | 0% | 483 | -0.41% | 0 |  | 1,572 | -0.51% |
| Load/substitute | 1 | -50.00% | 3 | 0% | 0 |  | 102 | -41.04% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 4 | -20.00% |
| RenderSink/substitute | 0 |  | 0 |  | 0 |  | 3 | -25.00% |
| RenderTo/substitute | 0 |  | 0 |  | 0 |  | 3 | -25.00% |

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
