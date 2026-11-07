# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (191 so far, latest [b104640](https://github.com/jinja2cpp/Jinja2Cpp/commit/b104640acaf07626e6a3c5b771fdb7d8998a8750) on 2026-11-07). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 345,246 | -0.01% |  | 55 | 0% |  | 18,480 | 0% | -52.71% |
| [Render/chat_llama](#renderchat_llama) | 295,053 | 0% |  | 225 | 0% |  | 11,632 | 0% | -19.00% |
| [Load/chat_mistral](#loadchat_mistral) | 393,933 | 0% |  | 52 | 0% |  | 20,888 | 0% | -53.22% |
| [Render/chat_mistral](#renderchat_mistral) | 500,213 | 0% |  | 402 | 0% |  | 9,968 | 0% | -24.53% |
| [Load/chat_qwen](#loadchat_qwen) | 266,654 | +0.01% |  | 44 | 0% |  | 14,416 | 0% | -47.25% |
| [Render/chat_qwen](#renderchat_qwen) | 277,203 | 0% |  | 212 | 0% |  | 8,304 | 0% | -2.35% |
| [Load/config_file](#loadconfig_file) | 97,229 | +0.01% |  | 25 | 0% |  | 5,464 | 0% | -48.26% |
| [Render/config_file](#renderconfig_file) | 1,274,076 | 0% |  | 908 | 0% |  | 24,440 | 0% | +2.65% |
| [Load/dict_ops](#loaddict_ops) | 33,334 | +0.01% | -77.44% | 9 | 0% | -93.13% | 2,176 | 0% | -48.87% |
| [Render/dict_ops](#renderdict_ops) | 293,031 | 0% | -57.58% | 112 | 0% | -75.11% | 24,408 | 0% | -29.65% |
| [Load/expressions](#loadexpressions) | 45,038 | -0.02% | -78.15% | 16 | 0% | -89.04% | 2,728 | 0% | -55.54% |
| [Render/expressions](#renderexpressions) | 426,200 | 0% | -52.97% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 69,101 | -0.15% | -74.82% | 27 | 0% | -88.16% | 3,848 | 0% | -66.29% |
| [Render/filters](#renderfilters) | 30,525 | 0% | -65.57% | 26 | 0% | -64.86% | 1,080 | 0% | -78.01% |
| [Load/for_filter_if](#loadfor_filter_if) | 30,035 | +0.03% | -77.82% | 7 | 0% | -94.31% | 1,904 | 0% | -44.78% |
| [Render/for_filter_if](#renderfor_filter_if) | 274,078 | 0% | -68.33% | 4 | 0% | -98.23% | 16,560 | 0% | +354.95% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 40,401 | +0.02% | -77.64% | 10 | 0% | -93.63% | 2,368 | 0% | -47.61% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 180,547 | 0% | -83.17% | 2 | 0% | -99.62% | 1,568 | 0% | -43.35% |
| [Load/for_range](#loadfor_range) | 13,657 | 0% | -75.91% | 7 | 0% | -87.50% | 1,088 | 0% | -45.60% |
| [Render/for_range](#renderfor_range) | 38,354 | 0% | -58.41% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/hf_deepseek_v31](#loadhf_deepseek_v31) | 388,255 | +0.01% |  | 55 | 0% |  | 21,976 | 0% | +0.73% |
| [Render/hf_deepseek_v31](#renderhf_deepseek_v31) | 90,951 | 0% |  | 43 | 0% |  | 2,480 | 0% | 0% |
| [Load/hf_gpt_oss](#loadhf_gpt_oss) | 1,549,842 | +0.01% |  | 287 | 0% |  | 74,640 | 0% | +0.56% |
| [Render/hf_gpt_oss](#renderhf_gpt_oss) | 170,929 | 0% |  | 155 | 0% |  | 5,096 | 0% | +3.24% |
| [Load/hf_mistral_small](#loadhf_mistral_small) | 599,123 | +0.01% |  | 88 | 0% |  | 33,048 | 0% | +0.34% |
| [Render/hf_mistral_small](#renderhf_mistral_small) | 145,109 | 0% |  | 105 | 0% |  | 6,120 | 0% | -0.26% |
| [Load/hf_qwen3](#loadhf_qwen3) | 486,117 | +0.02% |  | 82 | 0% |  | 25,560 | 0% | +0.31% |
| [Render/hf_qwen3](#renderhf_qwen3) | 119,996 | 0% |  | 100 | 0% |  | 3,248 | 0% | 0% |
| [Load/html_autoescape](#loadhtml_autoescape) | 194,037 | +0.03% |  | 43 | 0% |  | 12,640 | 0% | -48.78% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,209,427 | 0% |  | 674 | 0% |  | 45,408 | 0% | +12.44% |
| [Load/inheritance](#loadinheritance) | 23,065 | 0% | -79.79% | 7 | 0% | -93.46% | 1,344 | 0% | -53.20% |
| [Render/inheritance](#renderinheritance) | 137,589 | 0% | -81.60% | 35 | 0% | -94.37% | 7,384 | 0% | +22.74% |
| [Load/int_arith](#loadint_arith) | 66,002 | -0.01% |  | 20 | 0% |  | 3,872 | 0% | +0.41% |
| [Render/int_arith](#renderint_arith) | 825,803 | 0% |  | 12 | 0% |  | 3,272 | 0% | -0.49% |
| [Module/keep](#modulekeep) | 6,996 | 0% |  | 7 | 0% |  |  |  |  |
| [Module/keep_written](#modulekeep_written) | 36,407 | 0% |  | 11 | 0% |  |  |  |  |
| [Load/large_static](#loadlarge_static) | 154,719 | 0% | -82.54% | 14 | 0% | -96.82% | 43,120 | 0% | -19.23% |
| [Render/large_static](#renderlarge_static) | 20,711 | 0% | -72.44% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [RenderSink/large_static](#rendersinklarge_static) | 12,701 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/large_static](#rendertolarge_static) | 20,274 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/macros](#loadmacros) | 56,403 | +0.01% | -75.26% | 27 | 0% | -85.48% | 4,392 | 0% | -29.34% |
| [Render/macros](#rendermacros) | 656,539 | 0% | -74.38% | 308 | 0% | -88.71% | 20,560 | 0% | +39.67% |
| [Load/many_tags](#loadmany_tags) | 9,007,065 | -0.13% | -79.61% | 951 | 0% | -97.22% | 438,160 | 0% | -58.48% |
| [Render/many_tags](#rendermany_tags) | 728,684 | 0% | -65.02% | 2 | 0% | -90.48% | 5,648 | 0% | -10.52% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 46,587 | 0% | -80.39% | 19 | 0% | -89.67% | 3,360 | 0% | -44.59% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,196,675 | 0% | -57.94% | 33 | 0% | -99.19% | 381,536 | 0% | +9.95% |
| [RenderSink/mitsuhiko_table](#rendersinkmitsuhiko_table) | 5,123,868 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table](#rendertomitsuhiko_table) | 5,196,179 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 54,843 | 0% |  | 35 | 0% |  | 5,688 | 0% | -31.17% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,542,840 | 0% |  | 33 | 0% |  | 1,414,928 | 0% | +2.46% |
| [RenderSink/mitsuhiko_table_wide](#rendersinkmitsuhiko_table_wide) | 5,366,129 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table_wide](#rendertomitsuhiko_table_wide) | 5,542,392 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/plain_text](#loadplain_text) | 3,730 | 0% | -61.45% | 4 | 0% | -77.78% | 672 | 0% | -40.00% |
| [Render/plain_text](#renderplain_text) | 1,021 | 0% | -84.97% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/plain_text](#rendersinkplain_text) | 723 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/plain_text](#rendertoplain_text) | 822 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/prompt_budget](#loadprompt_budget) | 35,123 | +0.03% |  | 18 | 0% |  | 2,224 | 0% | 0% |
| [Render/prompt_budget](#renderprompt_budget) | 1,316,447 | 0% |  | 76 | 0% |  | 36,416 | 0% | 0% |
| [Load/strings](#loadstrings) | 70,462 | -0.03% | -75.86% | 30 | 0% | -86.61% | 4,344 | 0% | -51.95% |
| [Render/strings](#renderstrings) | 580,254 | 0% | -72.34% | 344 | 0% | -83.73% | 9,088 | 0% | -49.31% |
| [Load/substitute](#loadsubstitute) | 7,763 | 0% | -74.00% | 6 | 0% | -83.78% | 832 | 0% | -45.26% |
| [Render/substitute](#rendersubstitute) | 2,254 | 0% | -74.43% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/substitute](#rendersinksubstitute) | 1,960 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/substitute](#rendertosubstitute) | 2,059 | 0% |  | 0 |  |  | 0 |  |  |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 729 | -0.27% | 751 | -0.13% | 0 |  | 7,445 | +2.41% |
| Render/chat_llama | 1,144 | -1.63% | 536 | +1.13% | 0 |  | 4,114 | -5.16% |
| Load/chat_mistral | 1,044 | -0.48% | 1,049 | +0.19% | 0 |  | 8,306 | +2.98% |
| Render/chat_mistral | 1,899 | -5.85% | 549 | -4.36% | 0 |  | 9,930 | -6.90% |
| Load/chat_qwen | 485 | +0.41% | 591 | -0.84% | 0 |  | 4,308 | +1.96% |
| Render/chat_qwen | 1,059 | -1.03% | 391 | -2.49% | 0 |  | 3,391 | -4.64% |
| Load/config_file | 201 | +2.03% | 169 | +0.60% | 0 |  | 2,642 | +0.99% |
| Render/config_file | 6,454 | -2.26% | 1,569 | -0.13% | 0 |  | 40,564 | -0.41% |
| Load/dict_ops | 36 | +12.50% | 22 | +4.76% | 0 |  | 1,325 | +1.92% |
| Render/dict_ops | 680 | +2.41% | 419 | -0.48% | 0 |  | 884 | +0.23% |
| Load/expressions | 15 | +25.00% | 8 | +14.29% | 0 |  | 1,341 | +2.21% |
| Render/expressions | 13 | -13.33% | 6 | -14.29% | 0 |  | 304 | +2.36% |
| Load/filters | 74 | +5.71% | 46 | -4.17% | 0 |  | 1,879 | +18.92% |
| Render/filters | 28 | +21.74% | 4 | -20.00% | 0 |  | 782 | +0.90% |
| Load/for_filter_if | 24 | +9.09% | 20 | +25.00% | 0 |  | 1,049 | +1.16% |
| Render/for_filter_if | 1,796 | -0.44% | 98 | +1.03% | 0 |  | 142 | -15.48% |
| Load/for_loop_vars | 33 | +10.00% | 16 | 0% | 0 |  | 1,066 | -0.19% |
| Render/for_loop_vars | 4 | -55.56% | 1 | -66.67% | 0 |  | 128 | -26.44% |
| Load/for_range | 3 | 0% | 3 | 0% | 0 |  | 770 | +2.80% |
| Render/for_range | 1 | 0% | 0 |  | 0 |  | 121 | -3.20% |
| Load/hf_deepseek_v31 | 982 | +0.20% | 974 | +0.10% | 0 |  | 6,028 | +2.71% |
| Render/hf_deepseek_v31 | 345 | -0.58% | 81 | -2.41% | 0 |  | 1,388 | -4.47% |
| Load/hf_gpt_oss | 4,583 | -0.43% | 3,286 | -0.15% | 0 |  | 20,701 | +1.55% |
| Render/hf_gpt_oss | 1,198 | -0.91% | 382 | +1.06% | 0 |  | 4,429 | -1.20% |
| Load/hf_mistral_small | 1,822 | +0.05% | 1,591 | +0.19% | 0 |  | 11,015 | +5.66% |
| Render/hf_mistral_small | 957 | +0.10% | 322 | +0.94% | 0 |  | 3,823 | -2.92% |
| Load/hf_qwen3 | 1,398 | -0.50% | 1,243 | 0% | 0 |  | 8,599 | +3.04% |
| Render/hf_qwen3 | 649 | -1.82% | 222 | +0.45% | 0 |  | 2,541 | -2.83% |
| Load/html_autoescape | 446 | +1.13% | 519 | -0.19% | 0 |  | 3,808 | +5.51% |
| Render/html_autoescape | 3,687 | -5.02% | 1,219 | +2.09% | 0 |  | 36,588 | +3.84% |
| Load/inheritance | 2 | 0% | 3 | 0% | 0 |  | 671 | -0.45% |
| Render/inheritance | 590 | 0% | 95 | -2.06% | 0 |  | 910 | +0.55% |
| Load/int_arith | 60 | -4.76% | 39 | -2.50% | 0 |  | 2,149 | +7.13% |
| Render/int_arith | 24 | -20.00% | 10 | -41.18% | 0 |  | 748 | +0.94% |
| Module/keep | 2 | 0% | 2 | 0% | 0 |  | 433 | +2.85% |
| Module/keep_written | 2,931 | 0% | 561 | 0% | 0 |  | 711 | -0.56% |
| Load/large_static | 1,928 | +0.05% | 913 | 0% | 0 |  | 311 | +1.97% |
| Render/large_static | 708 | 0% | 569 | 0% | 0 |  | 3 | -25.00% |
| RenderSink/large_static | 0 |  | 0 |  | 0 |  | 2 | 0% |
| RenderTo/large_static | 686 | 0% | 571 | 0% | 0 |  | 3 | 0% |
| Load/macros | 72 | +2.86% | 54 | +3.85% | 0 |  | 1,691 | -0.12% |
| Render/macros | 763 | +0.26% | 216 | +0.93% | 0 |  | 4,498 | +57.77% |
| Load/many_tags | 27,702 | +0.02% | 22,373 | +0.06% | 0 |  | 228,693 | +3.53% |
| Render/many_tags | 6,219 | -0.08% | 864 | +0.12% | 0 |  | 114 | -16.79% |
| Load/mitsuhiko_table | 29 | +7.41% | 30 | +3.45% | 0 |  | 828 | +1.22% |
| Render/mitsuhiko_table | 10,019 | 0% | 6,275 | -0.03% | 0 |  | 334 | -11.87% |
| RenderSink/mitsuhiko_table | 9,951 | -0.03% | 1,184 | +0.17% | 0 |  | 268 | -13.27% |
| RenderTo/mitsuhiko_table | 10,019 | -0.05% | 6,283 | -0.03% | 0 |  | 296 | -16.38% |
| Load/mitsuhiko_table_wide | 70 | +1.45% | 65 | 0% | 0 |  | 1,021 | +3.76% |
| Render/mitsuhiko_table_wide | 10,242 | +0.04% | 22,666 | -0.03% | 0 |  | 315 | -9.74% |
| RenderSink/mitsuhiko_table_wide | 10,073 | +0.16% | 1,297 | +0.39% | 0 |  | 252 | -15.44% |
| RenderTo/mitsuhiko_table_wide | 10,232 | +0.03% | 22,681 | +0.02% | 0 |  | 298 | -9.97% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 30 | -6.25% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 6 | 0% |
| RenderSink/plain_text | 0 |  | 0 |  | 0 |  | 4 | 0% |
| RenderTo/plain_text | 0 |  | 0 |  | 0 |  | 6 | +20.00% |
| Load/prompt_budget | 61 | -1.61% | 34 | -10.53% | 0 |  | 1,378 | +0.51% |
| Render/prompt_budget | 4,786 | -0.02% | 527 | +0.19% | 0 |  | 941 | -22.68% |
| Load/strings | 121 | +7.08% | 81 | +5.19% | 0 |  | 2,782 | +2.96% |
| Render/strings | 412 | 0% | 476 | +0.42% | 0 |  | 1,560 | -1.02% |
| Load/substitute | 1 | 0% | 2 | 0% | 0 |  | 180 | -11.76% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 3 | +50.00% |
| RenderSink/substitute | 0 |  | 0 |  | 0 |  | 2 | 0% |
| RenderTo/substitute | 0 |  | 0 |  | 0 |  | 3 | +50.00% |

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
