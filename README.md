# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (215 so far, latest [5174feb](https://github.com/jinja2cpp/Jinja2Cpp/commit/5174feb995a4dfdea9f2e7ed149014ae53c3ad11) on 2026-11-13). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 335,039 | 0% |  | 55 | 0% |  | 18,464 | 0% | -52.75% |
| [Render/chat_llama](#renderchat_llama) | 295,715 | 0% |  | 225 | 0% |  | 11,616 | 0% | -19.11% |
| [Load/chat_mistral](#loadchat_mistral) | 384,675 | 0% |  | 52 | 0% |  | 20,888 | 0% | -53.22% |
| [Render/chat_mistral](#renderchat_mistral) | 501,956 | +0.08% |  | 402 | 0% |  | 10,000 | 0% | -24.29% |
| [Load/chat_qwen](#loadchat_qwen) | 260,887 | 0% |  | 44 | 0% |  | 14,416 | 0% | -47.25% |
| [Render/chat_qwen](#renderchat_qwen) | 277,933 | 0% |  | 212 | 0% |  | 8,304 | 0% | -2.35% |
| [Load/config_file](#loadconfig_file) | 94,237 | 0% |  | 25 | 0% |  | 5,464 | 0% | -48.26% |
| [Render/config_file](#renderconfig_file) | 1,276,401 | -0.01% |  | 908 | 0% |  | 24,440 | 0% | +2.65% |
| [Load/dict_ops](#loaddict_ops) | 32,187 | 0% | -78.22% | 9 | 0% | -93.13% | 2,176 | 0% | -48.87% |
| [Render/dict_ops](#renderdict_ops) | 295,715 | 0% | -57.20% | 112 | 0% | -75.11% | 24,408 | 0% | -29.65% |
| [Load/expressions](#loadexpressions) | 44,111 | 0% | -78.60% | 16 | 0% | -89.04% | 2,728 | 0% | -55.54% |
| [Render/expressions](#renderexpressions) | 426,926 | 0% | -52.89% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 67,449 | 0% | -75.42% | 27 | 0% | -88.16% | 3,848 | 0% | -66.29% |
| [Render/filters](#renderfilters) | 27,651 | 0% | -68.82% | 23 | 0% | -68.92% | 752 | 0% | -84.69% |
| [Load/for_filter_if](#loadfor_filter_if) | 29,032 | 0% | -78.56% | 7 | 0% | -94.31% | 1,904 | 0% | -44.78% |
| [Render/for_filter_if](#renderfor_filter_if) | 274,390 | 0% | -68.30% | 4 | 0% | -98.23% | 16,560 | 0% | +354.95% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 38,831 | 0% | -78.51% | 10 | 0% | -93.63% | 2,368 | 0% | -47.61% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 180,568 | 0% | -83.17% | 2 | 0% | -99.62% | 1,568 | 0% | -43.35% |
| [Load/for_range](#loadfor_range) | 13,209 | 0% | -76.70% | 7 | 0% | -87.50% | 1,088 | 0% | -45.60% |
| [Render/for_range](#renderfor_range) | 38,377 | 0% | -58.38% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/hf_deepseek_v31](#loadhf_deepseek_v31) | 382,655 | 0% |  | 55 | 0% |  | 21,976 | 0% | +0.73% |
| [Render/hf_deepseek_v31](#renderhf_deepseek_v31) | 92,584 | +0.07% |  | 43 | 0% |  | 2,480 | 0% | 0% |
| [Load/hf_gpt_oss](#loadhf_gpt_oss) | 1,521,692 | 0% |  | 287 | 0% |  | 74,704 | 0% | +0.65% |
| [Render/hf_gpt_oss](#renderhf_gpt_oss) | 172,923 | +0.03% |  | 155 | 0% |  | 5,096 | 0% | +3.24% |
| [Load/hf_mistral_small](#loadhf_mistral_small) | 590,861 | 0% |  | 88 | 0% |  | 33,048 | 0% | +0.34% |
| [Render/hf_mistral_small](#renderhf_mistral_small) | 142,509 | +0.01% |  | 99 | 0% |  | 6,120 | 0% | -0.26% |
| [Load/hf_qwen3](#loadhf_qwen3) | 475,019 | 0% |  | 82 | 0% |  | 25,560 | 0% | +0.31% |
| [Render/hf_qwen3](#renderhf_qwen3) | 120,840 | +0.01% |  | 100 | 0% |  | 3,248 | 0% | 0% |
| [Load/html_autoescape](#loadhtml_autoescape) | 188,629 | 0% |  | 43 | 0% |  | 12,672 | 0% | -48.65% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,208,553 | -0.01% |  | 674 | 0% |  | 45,408 | 0% | +12.44% |
| [Load/inheritance](#loadinheritance) | 21,887 | 0% | -80.82% | 7 | 0% | -93.46% | 1,344 | 0% | -53.20% |
| [Render/inheritance](#renderinheritance) | 137,631 | 0% | -81.60% | 35 | 0% | -94.37% | 7,384 | 0% | +22.74% |
| [Load/int_arith](#loadint_arith) | 64,887 | 0% |  | 20 | 0% |  | 3,872 | 0% | +0.41% |
| [Render/int_arith](#renderint_arith) | 828,086 | +0.10% |  | 12 | 0% |  | 3,272 | 0% | -0.49% |
| [Module/keep](#modulekeep) | 7,115 | -0.11% |  | 7 | 0% |  |  |  |  |
| [Module/keep_written](#modulekeep_written) | 29,069 | +0.10% |  | 11 | 0% |  |  |  |  |
| [Load/large_static](#loadlarge_static) | 147,598 | 0% | -83.34% | 14 | 0% | -96.82% | 43,120 | 0% | -19.23% |
| [Render/large_static](#renderlarge_static) | 20,726 | 0% | -72.42% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [RenderSink/large_static](#rendersinklarge_static) | 12,716 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/large_static](#rendertolarge_static) | 20,289 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/macros](#loadmacros) | 55,208 | 0% | -75.78% | 27 | 0% | -85.48% | 4,392 | 0% | -29.34% |
| [Render/macros](#rendermacros) | 659,727 | +0.02% | -74.26% | 308 | 0% | -88.71% | 20,560 | 0% | +39.67% |
| [Load/many_tags](#loadmany_tags) | 8,714,344 | 0% | -80.27% | 951 | 0% | -97.22% | 438,112 | 0% | -58.48% |
| [Render/many_tags](#rendermany_tags) | 729,918 | 0% | -64.96% | 2 | 0% | -90.48% | 5,648 | 0% | -10.52% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 45,013 | 0% | -81.05% | 19 | 0% | -89.67% | 3,360 | 0% | -44.59% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,202,900 | 0% | -57.89% | 33 | 0% | -99.19% | 381,536 | 0% | +9.95% |
| [RenderSink/mitsuhiko_table](#rendersinkmitsuhiko_table) | 5,130,097 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table](#rendertomitsuhiko_table) | 5,202,406 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 52,730 | 0% |  | 35 | 0% |  | 5,688 | 0% | -31.17% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,549,057 | 0% |  | 33 | 0% |  | 1,414,928 | 0% | +2.46% |
| [RenderSink/mitsuhiko_table_wide](#rendersinkmitsuhiko_table_wide) | 5,372,362 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table_wide](#rendertomitsuhiko_table_wide) | 5,548,625 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/plain_text](#loadplain_text) | 3,708 | 0% | -61.68% | 4 | 0% | -77.78% | 672 | 0% | -40.00% |
| [Render/plain_text](#renderplain_text) | 1,036 | 0% | -84.75% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/plain_text](#rendersinkplain_text) | 738 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/plain_text](#rendertoplain_text) | 837 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/prompt_budget](#loadprompt_budget) | 34,421 | 0% |  | 18 | 0% |  | 2,224 | 0% | 0% |
| [Render/prompt_budget](#renderprompt_budget) | 1,319,453 | 0% |  | 76 | 0% |  | 36,416 | 0% | 0% |
| [Load/strings](#loadstrings) | 69,018 | 0% | -76.35% | 30 | 0% | -86.61% | 4,344 | 0% | -51.95% |
| [Render/strings](#renderstrings) | 583,266 | +0.07% | -72.19% | 344 | 0% | -83.73% | 9,088 | 0% | -49.31% |
| [Load/substitute](#loadsubstitute) | 7,483 | 0% | -74.94% | 6 | 0% | -83.78% | 832 | 0% | -45.26% |
| [Render/substitute](#rendersubstitute) | 2,269 | 0% | -74.26% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/substitute](#rendersinksubstitute) | 1,975 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/substitute](#rendertosubstitute) | 2,074 | 0% |  | 0 |  |  | 0 |  |  |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 754 | +4.00% | 751 | +0.13% | 0 |  | 7,617 | -0.04% |
| Render/chat_llama | 1,151 | -1.20% | 521 | -0.95% | 0 |  | 4,444 | +10.60% |
| Load/chat_mistral | 1,109 | +0.73% | 1,103 | +0.46% | 0 |  | 8,674 | -1.89% |
| Render/chat_mistral | 1,879 | -5.20% | 587 | +1.03% | 0 |  | 10,780 | +3.49% |
| Load/chat_qwen | 619 | +1.98% | 631 | -0.47% | 0 |  | 4,547 | -2.47% |
| Render/chat_qwen | 1,127 | -0.79% | 394 | -1.75% | 0 |  | 3,570 | +5.34% |
| Load/config_file | 223 | -2.19% | 203 | -1.93% | 0 |  | 2,793 | -0.75% |
| Render/config_file | 5,992 | -1.79% | 1,465 | +0.21% | 0 |  | 43,447 | -2.31% |
| Load/dict_ops | 54 | -20.59% | 34 | 0% | 0 |  | 1,411 | +2.99% |
| Render/dict_ops | 687 | +1.18% | 432 | +0.23% | 0 |  | 911 | +1.56% |
| Load/expressions | 28 | -3.45% | 25 | +13.64% | 0 |  | 1,371 | +1.63% |
| Render/expressions | 12 | +50.00% | 6 | +20.00% | 0 |  | 330 | -0.90% |
| Load/filters | 112 | -1.75% | 50 | -1.96% | 0 |  | 1,694 | -0.35% |
| Render/filters | 17 | -19.05% | 5 | 0% | 0 |  | 803 | -0.37% |
| Load/for_filter_if | 33 | -13.16% | 19 | -5.00% | 0 |  | 1,120 | -1.75% |
| Render/for_filter_if | 1,730 | -0.29% | 140 | -1.41% | 0 |  | 151 | -21.76% |
| Load/for_loop_vars | 44 | -10.20% | 22 | -12.00% | 0 |  | 1,159 | -2.36% |
| Render/for_loop_vars | 9 | -25.00% | 3 | +200.00% | 0 |  | 150 | -4.46% |
| Load/for_range | 4 | -20.00% | 3 | -40.00% | 0 |  | 807 | +2.54% |
| Render/for_range | 1 | 0% | 0 |  | 0 |  | 168 | +55.56% |
| Load/hf_deepseek_v31 | 992 | -0.10% | 984 | -0.10% | 0 |  | 6,563 | +0.11% |
| Render/hf_deepseek_v31 | 348 | -6.45% | 76 | -1.30% | 0 |  | 1,482 | +2.92% |
| Load/hf_gpt_oss | 4,676 | -0.49% | 3,379 | -0.44% | 0 |  | 21,660 | -3.56% |
| Render/hf_gpt_oss | 1,223 | -0.73% | 383 | -1.54% | 0 |  | 4,717 | +1.05% |
| Load/hf_mistral_small | 1,936 | +0.05% | 1,641 | -0.61% | 0 |  | 11,062 | -4.18% |
| Render/hf_mistral_small | 959 | +0.31% | 297 | +2.06% | 0 |  | 3,683 | -2.77% |
| Load/hf_qwen3 | 1,530 | 0% | 1,346 | -1.10% | 0 |  | 8,783 | -4.63% |
| Render/hf_qwen3 | 658 | -2.81% | 217 | +0.93% | 0 |  | 2,739 | +2.62% |
| Load/html_autoescape | 515 | +1.38% | 541 | -1.10% | 0 |  | 4,099 | +1.33% |
| Render/html_autoescape | 4,247 | -4.45% | 1,283 | +7.91% | 0 |  | 36,350 | +0.68% |
| Load/inheritance | 6 | -33.33% | 5 | 0% | 0 |  | 724 | +3.87% |
| Render/inheritance | 599 | 0% | 101 | -2.88% | 0 |  | 1,001 | +2.88% |
| Load/int_arith | 75 | +4.17% | 52 | +1.96% | 0 |  | 2,168 | -1.28% |
| Render/int_arith | 17 | -32.00% | 11 | +37.50% | 0 |  | 760 | -4.52% |
| Module/keep | 2 | 0% | 2 | 0% | 0 |  | 433 | +6.39% |
| Module/keep_written | 1,130 | -0.18% | 562 | 0% | 0 |  | 745 | +2.05% |
| Load/large_static | 1,931 | -0.05% | 905 | 0% | 0 |  | 330 | +12.63% |
| Render/large_static | 708 | 0% | 569 | 0% | 0 |  | 5 | -44.44% |
| RenderSink/large_static | 0 |  | 0 |  | 0 |  | 3 | 0% |
| RenderTo/large_static | 685 | 0% | 570 | 0% | 0 |  | 5 | +25.00% |
| Load/macros | 97 | +8.99% | 77 | +6.94% | 0 |  | 1,751 | +1.16% |
| Render/macros | 753 | +0.27% | 239 | +1.27% | 0 |  | 656 | -48.95% |
| Load/many_tags | 28,981 | +2.58% | 22,749 | +0.65% | 0 |  | 233,394 | -2.39% |
| Render/many_tags | 6,254 | +0.08% | 881 | +0.11% | 0 |  | 140 | -10.26% |
| Load/mitsuhiko_table | 41 | -8.89% | 56 | -3.45% | 0 |  | 957 | +7.05% |
| Render/mitsuhiko_table | 10,065 | +0.17% | 6,276 | +0.05% | 0 |  | 417 | +21.93% |
| RenderSink/mitsuhiko_table | 9,990 | +0.14% | 1,186 | -0.17% | 0 |  | 335 | +12.04% |
| RenderTo/mitsuhiko_table | 10,063 | +0.12% | 6,273 | -0.05% | 0 |  | 382 | +15.41% |
| Load/mitsuhiko_table_wide | 71 | +1.43% | 89 | +2.30% | 0 |  | 1,054 | +7.99% |
| Render/mitsuhiko_table_wide | 10,269 | +0.03% | 22,678 | -0.01% | 0 |  | 399 | +23.15% |
| RenderSink/mitsuhiko_table_wide | 10,077 | -0.11% | 1,292 | -0.77% | 0 |  | 333 | +22.88% |
| RenderTo/mitsuhiko_table_wide | 10,283 | -0.01% | 22,682 | 0% | 0 |  | 372 | +22.37% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 16 | +166.67% |
| Render/plain_text | 1 | 0% | 0 |  | 0 |  | 6 | -14.29% |
| RenderSink/plain_text | 0 |  | 0 |  | 0 |  | 5 | 0% |
| RenderTo/plain_text | 0 |  | 0 |  | 0 |  | 5 | 0% |
| Load/prompt_budget | 82 | -4.65% | 45 | -8.16% | 0 |  | 1,426 | +2.15% |
| Render/prompt_budget | 4,803 | -0.02% | 549 | +0.73% | 0 |  | 1,119 | +2.19% |
| Load/strings | 152 | -5.59% | 123 | +4.24% | 0 |  | 2,812 | -0.32% |
| Render/strings | 430 | -2.71% | 481 | -1.64% | 0 |  | 1,617 | +0.62% |
| Load/substitute | 2 | -60.00% | 3 | -25.00% | 0 |  | 172 | +17.81% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 5 | -61.54% |
| RenderSink/substitute | 0 |  | 0 |  | 0 |  | 5 | +25.00% |
| RenderTo/substitute | 0 |  | 0 |  | 0 |  | 5 | +25.00% |

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
