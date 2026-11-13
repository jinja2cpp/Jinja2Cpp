# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (214 so far, latest [2f9b9c2](https://github.com/jinja2cpp/Jinja2Cpp/commit/2f9b9c26cceda0e2dacbad52bd92b8dd5c48c23e) on 2026-11-13). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 335,039 | -0.33% |  | 55 | 0% |  | 18,464 | 0% | -52.75% |
| [Render/chat_llama](#renderchat_llama) | 295,715 | 0% |  | 225 | 0% |  | 11,616 | 0% | -19.11% |
| [Load/chat_mistral](#loadchat_mistral) | 384,674 | -0.43% |  | 52 | 0% |  | 20,888 | 0% | -53.22% |
| [Render/chat_mistral](#renderchat_mistral) | 501,568 | 0% |  | 402 | 0% |  | 10,000 | 0% | -24.29% |
| [Load/chat_qwen](#loadchat_qwen) | 260,887 | -0.49% |  | 44 | 0% |  | 14,416 | 0% | -47.25% |
| [Render/chat_qwen](#renderchat_qwen) | 277,933 | 0% |  | 212 | 0% |  | 8,304 | 0% | -2.35% |
| [Load/config_file](#loadconfig_file) | 94,237 | -0.88% |  | 25 | 0% |  | 5,464 | 0% | -48.26% |
| [Render/config_file](#renderconfig_file) | 1,276,479 | +0.10% |  | 908 | 0% |  | 24,440 | 0% | +2.65% |
| [Load/dict_ops](#loaddict_ops) | 32,187 | -0.59% | -78.22% | 9 | 0% | -93.13% | 2,176 | 0% | -48.87% |
| [Render/dict_ops](#renderdict_ops) | 295,715 | +0.23% | -57.20% | 112 | 0% | -75.11% | 24,408 | 0% | -29.65% |
| [Load/expressions](#loadexpressions) | 44,111 | -0.22% | -78.60% | 16 | 0% | -89.04% | 2,728 | 0% | -55.54% |
| [Render/expressions](#renderexpressions) | 426,926 | 0% | -52.89% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 67,449 | -0.77% | -75.42% | 27 | 0% | -88.16% | 3,848 | 0% | -66.29% |
| [Render/filters](#renderfilters) | 27,651 | +0.08% | -68.82% | 23 | 0% | -68.92% | 752 | 0% | -84.69% |
| [Load/for_filter_if](#loadfor_filter_if) | 29,032 | -0.83% | -78.56% | 7 | 0% | -94.31% | 1,904 | 0% | -44.78% |
| [Render/for_filter_if](#renderfor_filter_if) | 274,390 | 0% | -68.30% | 4 | 0% | -98.23% | 16,560 | 0% | +354.95% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 38,831 | -0.90% | -78.51% | 10 | 0% | -93.63% | 2,368 | 0% | -47.61% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 180,568 | 0% | -83.17% | 2 | 0% | -99.62% | 1,568 | 0% | -43.35% |
| [Load/for_range](#loadfor_range) | 13,209 | -0.32% | -76.70% | 7 | 0% | -87.50% | 1,088 | 0% | -45.60% |
| [Render/for_range](#renderfor_range) | 38,377 | 0% | -58.38% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/hf_deepseek_v31](#loadhf_deepseek_v31) | 382,655 | -0.28% |  | 55 | 0% |  | 21,976 | 0% | +0.73% |
| [Render/hf_deepseek_v31](#renderhf_deepseek_v31) | 92,520 | +0.02% |  | 43 | 0% |  | 2,480 | 0% | 0% |
| [Load/hf_gpt_oss](#loadhf_gpt_oss) | 1,521,692 | -0.38% |  | 287 | 0% |  | 74,704 | 0% | +0.65% |
| [Render/hf_gpt_oss](#renderhf_gpt_oss) | 172,873 | +0.02% |  | 155 | 0% |  | 5,096 | 0% | +3.24% |
| [Load/hf_mistral_small](#loadhf_mistral_small) | 590,861 | -0.29% |  | 88 | 0% |  | 33,048 | 0% | +0.34% |
| [Render/hf_mistral_small](#renderhf_mistral_small) | 142,497 | +0.01% |  | 99 | 0% |  | 6,120 | 0% | -0.26% |
| [Load/hf_qwen3](#loadhf_qwen3) | 475,019 | -0.59% |  | 82 | 0% |  | 25,560 | 0% | +0.31% |
| [Render/hf_qwen3](#renderhf_qwen3) | 120,832 | +0.01% |  | 100 | 0% |  | 3,248 | 0% | 0% |
| [Load/html_autoescape](#loadhtml_autoescape) | 188,629 | -1.10% |  | 43 | 0% |  | 12,672 | 0% | -48.65% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,208,616 | 0% |  | 674 | 0% |  | 45,408 | 0% | +12.44% |
| [Load/inheritance](#loadinheritance) | 21,887 | -0.39% | -80.82% | 7 | 0% | -93.46% | 1,344 | 0% | -53.20% |
| [Render/inheritance](#renderinheritance) | 137,635 | +0.03% | -81.60% | 35 | 0% | -94.37% | 7,384 | 0% | +22.74% |
| [Load/int_arith](#loadint_arith) | 64,887 | -0.50% |  | 20 | 0% |  | 3,872 | 0% | +0.41% |
| [Render/int_arith](#renderint_arith) | 827,286 | 0% |  | 12 | 0% |  | 3,272 | 0% | -0.49% |
| [Module/keep](#modulekeep) | 7,123 | +0.20% |  | 7 | 0% |  |  |  |  |
| [Module/keep_written](#modulekeep_written) | 29,039 | +0.04% |  | 11 | 0% |  |  |  |  |
| [Load/large_static](#loadlarge_static) | 147,598 | -1.21% | -83.34% | 14 | 0% | -96.82% | 43,120 | 0% | -19.23% |
| [Render/large_static](#renderlarge_static) | 20,726 | 0% | -72.42% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [RenderSink/large_static](#rendersinklarge_static) | 12,716 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/large_static](#rendertolarge_static) | 20,289 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/macros](#loadmacros) | 55,208 | -0.80% | -75.78% | 27 | 0% | -85.48% | 4,392 | 0% | -29.34% |
| [Render/macros](#rendermacros) | 659,627 | 0% | -74.26% | 308 | 0% | -88.71% | 20,560 | 0% | +39.67% |
| [Load/many_tags](#loadmany_tags) | 8,714,344 | -0.88% | -80.27% | 951 | 0% | -97.22% | 438,112 | 0% | -58.48% |
| [Render/many_tags](#rendermany_tags) | 729,918 | 0% | -64.96% | 2 | 0% | -90.48% | 5,648 | 0% | -10.52% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 45,013 | -0.84% | -81.05% | 19 | 0% | -89.67% | 3,360 | 0% | -44.59% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,202,846 | 0% | -57.89% | 33 | 0% | -99.19% | 381,536 | 0% | +9.95% |
| [RenderSink/mitsuhiko_table](#rendersinkmitsuhiko_table) | 5,130,043 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table](#rendertomitsuhiko_table) | 5,202,352 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 52,730 | -0.76% |  | 35 | 0% |  | 5,688 | 0% | -31.17% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,549,003 | 0% |  | 33 | 0% |  | 1,414,928 | 0% | +2.46% |
| [RenderSink/mitsuhiko_table_wide](#rendersinkmitsuhiko_table_wide) | 5,372,308 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table_wide](#rendertomitsuhiko_table_wide) | 5,548,571 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/plain_text](#loadplain_text) | 3,708 | -0.24% | -61.68% | 4 | 0% | -77.78% | 672 | 0% | -40.00% |
| [Render/plain_text](#renderplain_text) | 1,036 | 0% | -84.75% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/plain_text](#rendersinkplain_text) | 738 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/plain_text](#rendertoplain_text) | 837 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/prompt_budget](#loadprompt_budget) | 34,421 | -0.47% |  | 18 | 0% |  | 2,224 | 0% | 0% |
| [Render/prompt_budget](#renderprompt_budget) | 1,319,453 | 0% |  | 76 | 0% |  | 36,416 | 0% | 0% |
| [Load/strings](#loadstrings) | 69,018 | -0.60% | -76.35% | 30 | 0% | -86.61% | 4,344 | 0% | -51.95% |
| [Render/strings](#renderstrings) | 582,834 | 0% | -72.21% | 344 | 0% | -83.73% | 9,088 | 0% | -49.31% |
| [Load/substitute](#loadsubstitute) | 7,483 | -0.41% | -74.94% | 6 | 0% | -83.78% | 832 | 0% | -45.26% |
| [Render/substitute](#rendersubstitute) | 2,269 | 0% | -74.26% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/substitute](#rendersinksubstitute) | 1,975 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/substitute](#rendertosubstitute) | 2,074 | 0% |  | 0 |  |  | 0 |  |  |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 725 | -1.76% | 750 | +1.08% | 0 |  | 7,620 | -2.79% |
| Render/chat_llama | 1,165 | +1.57% | 526 | -0.38% | 0 |  | 4,018 | -3.78% |
| Load/chat_mistral | 1,101 | +3.28% | 1,098 | +2.71% | 0 |  | 8,841 | +0.01% |
| Render/chat_mistral | 1,982 | -0.35% | 581 | -1.19% | 0 |  | 10,416 | +0.88% |
| Load/chat_qwen | 607 | -0.82% | 634 | +1.60% | 0 |  | 4,662 | +0.93% |
| Render/chat_qwen | 1,136 | +0.26% | 401 | +0.25% | 0 |  | 3,389 | -1.20% |
| Load/config_file | 228 | +1.33% | 207 | -0.96% | 0 |  | 2,814 | +4.42% |
| Render/config_file | 6,101 | -6.24% | 1,462 | -4.32% | 0 |  | 44,475 | +1.20% |
| Load/dict_ops | 68 | +17.24% | 34 | +13.33% | 0 |  | 1,370 | -0.15% |
| Render/dict_ops | 679 | -1.16% | 431 | -1.15% | 0 |  | 897 | 0% |
| Load/expressions | 29 | +20.83% | 22 | +15.79% | 0 |  | 1,349 | -0.52% |
| Render/expressions | 8 | -46.67% | 5 | 0% | 0 |  | 333 | +4.06% |
| Load/filters | 114 | +10.68% | 51 | -3.77% | 0 |  | 1,700 | +0.47% |
| Render/filters | 21 | +23.53% | 5 | +25.00% | 0 |  | 806 | +0.75% |
| Load/for_filter_if | 38 | +8.57% | 20 | +33.33% | 0 |  | 1,140 | +4.40% |
| Render/for_filter_if | 1,735 | +0.64% | 142 | +2.90% | 0 |  | 193 | +22.15% |
| Load/for_loop_vars | 49 | -2.00% | 25 | -13.79% | 0 |  | 1,187 | +5.70% |
| Render/for_loop_vars | 12 | +20.00% | 1 |  | 0 |  | 157 | +13.77% |
| Load/for_range | 5 | -16.67% | 5 | -16.67% | 0 |  | 787 | +0.51% |
| Render/for_range | 1 | -88.89% | 0 |  | 0 |  | 108 | -25.00% |
| Load/hf_deepseek_v31 | 993 | +0.40% | 985 | +0.72% | 0 |  | 6,556 | +1.80% |
| Render/hf_deepseek_v31 | 372 | +4.20% | 77 | +1.32% | 0 |  | 1,440 | -1.17% |
| Load/hf_gpt_oss | 4,699 | +0.13% | 3,394 | +1.16% | 0 |  | 22,459 | +0.55% |
| Render/hf_gpt_oss | 1,232 | +0.33% | 389 | +0.78% | 0 |  | 4,668 | +2.32% |
| Load/hf_mistral_small | 1,935 | +0.99% | 1,651 | +2.42% | 0 |  | 11,544 | +1.32% |
| Render/hf_mistral_small | 956 | -0.31% | 291 | -2.68% | 0 |  | 3,788 | +2.80% |
| Load/hf_qwen3 | 1,530 | +1.19% | 1,361 | +3.34% | 0 |  | 9,209 | +3.21% |
| Render/hf_qwen3 | 677 | +0.30% | 215 | 0% | 0 |  | 2,669 | +0.79% |
| Load/html_autoescape | 508 | -0.97% | 547 | +1.86% | 0 |  | 4,045 | +3.90% |
| Render/html_autoescape | 4,445 | -0.51% | 1,189 | -7.18% | 0 |  | 36,104 | -1.09% |
| Load/inheritance | 9 | +50.00% | 5 | +66.67% | 0 |  | 697 | +0.43% |
| Render/inheritance | 599 | -0.99% | 104 | -0.95% | 0 |  | 973 | -36.11% |
| Load/int_arith | 72 | -13.25% | 51 | -20.31% | 0 |  | 2,196 | +5.37% |
| Render/int_arith | 25 | +19.05% | 8 | +33.33% | 0 |  | 796 | +3.65% |
| Module/keep | 2 | +100.00% | 2 | 0% | 0 |  | 407 | -4.01% |
| Module/keep_written | 1,132 | +0.09% | 562 | +0.36% | 0 |  | 730 | -1.08% |
| Load/large_static | 1,932 | +0.31% | 905 | -0.44% | 0 |  | 293 | -4.56% |
| Render/large_static | 708 | 0% | 569 | 0% | 0 |  | 9 | +125.00% |
| RenderSink/large_static | 0 |  | 0 |  | 0 |  | 3 | +50.00% |
| RenderTo/large_static | 685 | 0% | 570 | 0% | 0 |  | 4 | 0% |
| Load/macros | 89 | -14.42% | 72 | -1.37% | 0 |  | 1,731 | +2.00% |
| Render/macros | 751 | -0.13% | 236 | +0.43% | 0 |  | 1,285 | +40.74% |
| Load/many_tags | 28,253 | -0.89% | 22,603 | -0.71% | 0 |  | 239,106 | -1.61% |
| Render/many_tags | 6,249 | 0% | 880 | -0.34% | 0 |  | 156 | -6.59% |
| Load/mitsuhiko_table | 45 | +25.00% | 58 | +26.09% | 0 |  | 894 | -0.22% |
| Render/mitsuhiko_table | 10,048 | -0.18% | 6,273 | +0.05% | 0 |  | 342 | -6.56% |
| RenderSink/mitsuhiko_table | 9,976 | +0.07% | 1,188 | 0% | 0 |  | 299 | +2.75% |
| RenderTo/mitsuhiko_table | 10,051 | -0.12% | 6,276 | +0.06% | 0 |  | 331 | -1.49% |
| Load/mitsuhiko_table_wide | 70 | -13.58% | 87 | +4.82% | 0 |  | 976 | -7.22% |
| Render/mitsuhiko_table_wide | 10,266 | -0.01% | 22,680 | +0.03% | 0 |  | 324 | -11.96% |
| RenderSink/mitsuhiko_table_wide | 10,088 | +0.10% | 1,302 | +0.08% | 0 |  | 271 | -2.17% |
| RenderTo/mitsuhiko_table_wide | 10,284 | -2.35% | 22,682 | -0.03% | 0 |  | 304 | -7.88% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 6 | -79.31% |
| Render/plain_text | 1 |  | 0 |  | 0 |  | 7 | +16.67% |
| RenderSink/plain_text | 0 |  | 0 |  | 0 |  | 5 | +25.00% |
| RenderTo/plain_text | 0 |  | 0 |  | 0 |  | 5 | 0% |
| Load/prompt_budget | 86 | +14.67% | 49 | +19.51% | 0 |  | 1,396 | -2.04% |
| Render/prompt_budget | 4,804 | -0.10% | 545 | -0.73% | 0 |  | 1,095 | -3.10% |
| Load/strings | 161 | +1.90% | 118 | -4.84% | 0 |  | 2,821 | +1.37% |
| Render/strings | 442 | +1.14% | 489 | +0.20% | 0 |  | 1,607 | +2.16% |
| Load/substitute | 5 | +150.00% | 4 | +33.33% | 0 |  | 146 | +11.45% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 13 | +225.00% |
| RenderSink/substitute | 0 |  | 0 |  | 0 |  | 4 | +33.33% |
| RenderTo/substitute | 0 |  | 0 |  | 0 |  | 4 | 0% |

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
