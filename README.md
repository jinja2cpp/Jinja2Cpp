# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (185 so far, latest [297aa0c](https://github.com/jinja2cpp/Jinja2Cpp/commit/297aa0cfd1e36b018aa10262846964550ca99809) on 2026-11-06). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 344,592 | 0% |  | 55 | 0% |  | 18,480 | 0% | -52.71% |
| [Render/chat_llama](#renderchat_llama) | 296,105 | 0% |  | 225 | 0% |  | 11,616 | 0% | -19.11% |
| [Load/chat_mistral](#loadchat_mistral) | 393,293 | 0% |  | 52 | 0% |  | 20,888 | 0% | -53.22% |
| [Render/chat_mistral](#renderchat_mistral) | 499,846 | 0% |  | 402 | 0% |  | 9,984 | 0% | -24.41% |
| [Load/chat_qwen](#loadchat_qwen) | 265,795 | 0% |  | 44 | 0% |  | 14,416 | 0% | -47.25% |
| [Render/chat_qwen](#renderchat_qwen) | 276,838 | 0% |  | 212 | 0% |  | 8,304 | 0% | -2.35% |
| [Load/config_file](#loadconfig_file) | 96,980 | 0% |  | 25 | 0% |  | 5,464 | 0% | -48.26% |
| [Render/config_file](#renderconfig_file) | 1,268,879 | 0% |  | 908 | 0% |  | 24,440 | 0% | +2.65% |
| [Load/dict_ops](#loaddict_ops) | 33,318 | 0% | -77.45% | 9 | 0% | -93.13% | 2,176 | 0% | -48.87% |
| [Render/dict_ops](#renderdict_ops) | 292,706 | 0% | -57.63% | 112 | 0% | -75.11% | 24,408 | 0% | -29.65% |
| [Load/expressions](#loadexpressions) | 44,977 | 0% | -78.18% | 16 | 0% | -89.04% | 2,728 | 0% | -55.54% |
| [Render/expressions](#renderexpressions) | 426,222 | 0% | -52.96% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 69,062 | 0% | -74.84% | 27 | 0% | -88.16% | 3,848 | 0% | -66.29% |
| [Render/filters](#renderfilters) | 30,385 | 0% | -65.73% | 26 | 0% | -64.86% | 1,080 | 0% | -78.01% |
| [Load/for_filter_if](#loadfor_filter_if) | 29,983 | 0% | -77.85% | 7 | 0% | -94.31% | 1,904 | 0% | -44.78% |
| [Render/for_filter_if](#renderfor_filter_if) | 274,664 | 0% | -68.26% | 4 | 0% | -98.23% | 16,560 | 0% | +354.95% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 40,311 | 0% | -77.69% | 10 | 0% | -93.63% | 2,368 | 0% | -47.61% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 180,538 | 0% | -83.17% | 2 | 0% | -99.62% | 1,568 | 0% | -43.35% |
| [Load/for_range](#loadfor_range) | 13,624 | 0% | -75.96% | 7 | 0% | -87.50% | 1,088 | 0% | -45.60% |
| [Render/for_range](#renderfor_range) | 38,374 | 0% | -58.38% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/hf_deepseek_v31](#loadhf_deepseek_v31) | 386,796 | 0% |  | 55 | 0% |  | 21,976 | 0% | +0.73% |
| [Render/hf_deepseek_v31](#renderhf_deepseek_v31) | 90,619 | 0% |  | 43 | 0% |  | 2,480 | 0% | 0% |
| [Load/hf_gpt_oss](#loadhf_gpt_oss) | 1,546,907 | 0% |  | 287 | 0% |  | 74,640 | 0% | +0.56% |
| [Render/hf_gpt_oss](#renderhf_gpt_oss) | 170,639 | 0% |  | 155 | 0% |  | 5,096 | 0% | +3.24% |
| [Load/hf_mistral_small](#loadhf_mistral_small) | 598,174 | 0% |  | 88 | 0% |  | 33,064 | 0% | +0.39% |
| [Render/hf_mistral_small](#renderhf_mistral_small) | 144,550 | 0% |  | 105 | 0% |  | 6,120 | 0% | -0.26% |
| [Load/hf_qwen3](#loadhf_qwen3) | 485,395 | 0% |  | 82 | 0% |  | 25,560 | 0% | +0.31% |
| [Render/hf_qwen3](#renderhf_qwen3) | 119,696 | 0% |  | 100 | 0% |  | 3,248 | 0% | 0% |
| [Load/html_autoescape](#loadhtml_autoescape) | 193,787 | 0% |  | 43 | 0% |  | 12,640 | 0% | -48.78% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,209,855 | 0% |  | 674 | 0% |  | 45,408 | 0% | +12.44% |
| [Load/inheritance](#loadinheritance) | 22,999 | 0% | -79.85% | 7 | 0% | -93.46% | 1,344 | 0% | -53.20% |
| [Render/inheritance](#renderinheritance) | 138,105 | 0% | -81.53% | 35 | 0% | -94.37% | 7,384 | 0% | +22.74% |
| [Load/int_arith](#loadint_arith) | 65,958 | 0% |  | 20 | 0% |  | 3,872 | 0% | +0.41% |
| [Render/int_arith](#renderint_arith) | 825,593 | 0% |  | 12 | 0% |  | 3,272 | 0% | -0.49% |
| [Module/keep](#modulekeep) | 7,000 | +0.32% |  | 7 | 0% |  |  |  |  |
| [Module/keep_written](#modulekeep_written) | 36,381 | +0.06% |  | 11 | 0% |  |  |  |  |
| [Load/large_static](#loadlarge_static) | 154,569 | 0% | -82.55% | 14 | 0% | -96.82% | 43,120 | 0% | -19.23% |
| [Render/large_static](#renderlarge_static) | 20,707 | 0% | -72.44% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [RenderSink/large_static](#rendersinklarge_static) | 12,697 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/large_static](#rendertolarge_static) | 20,270 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/macros](#loadmacros) | 56,246 | 0% | -75.33% | 27 | 0% | -85.48% | 4,392 | 0% | -29.34% |
| [Render/macros](#rendermacros) | 656,313 | 0% | -74.39% | 308 | 0% | -88.71% | 20,560 | 0% | +39.67% |
| [Load/many_tags](#loadmany_tags) | 9,007,813 | 0% | -79.60% | 951 | 0% | -97.22% | 438,144 | 0% | -58.48% |
| [Render/many_tags](#rendermany_tags) | 725,708 | 0% | -65.16% | 2 | 0% | -90.48% | 5,648 | 0% | -10.52% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 46,460 | 0% | -80.44% | 19 | 0% | -89.67% | 3,360 | 0% | -44.59% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,195,666 | 0% | -57.95% | 33 | 0% | -99.19% | 381,536 | 0% | +9.95% |
| [RenderSink/mitsuhiko_table](#rendersinkmitsuhiko_table) | 5,122,851 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table](#rendertomitsuhiko_table) | 5,195,168 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 54,713 | 0% |  | 35 | 0% |  | 5,688 | 0% | -31.17% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,541,826 | 0% |  | 33 | 0% |  | 1,414,928 | 0% | +2.46% |
| [RenderSink/mitsuhiko_table_wide](#rendersinkmitsuhiko_table_wide) | 5,365,137 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table_wide](#rendertomitsuhiko_table_wide) | 5,541,398 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/plain_text](#loadplain_text) | 3,707 | 0% | -61.69% | 4 | 0% | -77.78% | 672 | 0% | -40.00% |
| [Render/plain_text](#renderplain_text) | 1,017 | 0% | -85.03% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/plain_text](#rendersinkplain_text) | 719 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/plain_text](#rendertoplain_text) | 818 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/strings](#loadstrings) | 70,231 | 0% | -75.94% | 30 | 0% | -86.61% | 4,344 | 0% | -51.95% |
| [Render/strings](#renderstrings) | 578,649 | 0% | -72.41% | 344 | 0% | -83.73% | 9,088 | 0% | -49.31% |
| [Load/substitute](#loadsubstitute) | 7,736 | 0% | -74.09% | 6 | 0% | -83.78% | 832 | 0% | -45.26% |
| [Render/substitute](#rendersubstitute) | 2,250 | 0% | -74.47% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/substitute](#rendersinksubstitute) | 1,956 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/substitute](#rendertosubstitute) | 2,055 | 0% |  | 0 |  |  | 0 |  |  |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 727 | +1.25% | 736 | +0.27% | 0 |  | 7,394 | +0.49% |
| Render/chat_llama | 1,203 | +0.84% | 523 | +0.77% | 0 |  | 4,273 | +7.99% |
| Load/chat_mistral | 1,028 | +1.28% | 1,031 | +0.10% | 0 |  | 8,040 | -2.75% |
| Render/chat_mistral | 1,907 | +0.26% | 571 | -1.38% | 0 |  | 10,525 | +6.24% |
| Load/chat_qwen | 527 | +0.96% | 585 | +0.17% | 0 |  | 4,291 | -2.12% |
| Render/chat_qwen | 1,102 | -0.90% | 410 | +0.99% | 0 |  | 3,612 | +7.89% |
| Load/config_file | 187 | +3.31% | 169 | +1.81% | 0 |  | 2,622 | -1.32% |
| Render/config_file | 6,131 | -1.90% | 1,513 | -0.79% | 0 |  | 38,847 | -5.31% |
| Load/dict_ops | 36 | -2.70% | 35 | +2.94% | 0 |  | 1,326 | +0.61% |
| Render/dict_ops | 637 | 0% | 422 | +0.48% | 0 |  | 898 | +1.93% |
| Load/expressions | 17 | -19.05% | 19 | -9.52% | 0 |  | 1,339 | -0.45% |
| Render/expressions | 4 | +100.00% | 2 |  | 0 |  | 300 | +3.45% |
| Load/filters | 90 | -3.23% | 60 | +1.69% | 0 |  | 1,704 | +14.82% |
| Render/filters | 13 | -7.14% | 3 | -25.00% | 0 |  | 788 | -0.25% |
| Load/for_filter_if | 20 | -4.76% | 17 | +13.33% | 0 |  | 1,039 | -5.03% |
| Render/for_filter_if | 1,798 | -0.17% | 95 | 0% | 0 |  | 157 | -1.88% |
| Load/for_loop_vars | 21 | +16.67% | 15 | +50.00% | 0 |  | 1,070 | -1.29% |
| Render/for_loop_vars | 6 | 0% | 2 | 0% | 0 |  | 130 | -26.14% |
| Load/for_range | 4 | 0% | 4 | 0% | 0 |  | 745 | -1.32% |
| Render/for_range | 1 | 0% | 0 |  | 0 |  | 165 | +46.02% |
| Load/hf_deepseek_v31 | 971 | +0.41% | 964 | -0.10% | 0 |  | 5,954 | +1.10% |
| Render/hf_deepseek_v31 | 349 | -3.32% | 78 | -7.14% | 0 |  | 1,463 | +8.77% |
| Load/hf_gpt_oss | 4,581 | +0.33% | 3,304 | +0.06% | 0 |  | 21,678 | -0.71% |
| Render/hf_gpt_oss | 1,207 | -0.74% | 375 | -0.79% | 0 |  | 4,579 | +0.99% |
| Load/hf_mistral_small | 1,773 | +0.34% | 1,563 | 0% | 0 |  | 10,626 | -1.26% |
| Render/hf_mistral_small | 943 | -1.36% | 325 | -1.22% | 0 |  | 3,846 | +3.28% |
| Load/hf_qwen3 | 1,398 | +0.29% | 1,238 | +0.08% | 0 |  | 8,420 | -2.61% |
| Render/hf_qwen3 | 633 | -1.86% | 204 | -0.49% | 0 |  | 2,628 | +2.26% |
| Load/html_autoescape | 443 | +1.84% | 516 | +0.19% | 0 |  | 3,728 | -3.32% |
| Render/html_autoescape | 3,861 | -3.93% | 1,263 | -0.86% | 0 |  | 34,989 | -0.52% |
| Load/inheritance | 2 | 0% | 3 | -25.00% | 0 |  | 674 | -1.61% |
| Render/inheritance | 579 | +1.05% | 98 | -1.01% | 0 |  | 1,281 | +32.47% |
| Load/int_arith | 76 | +4.11% | 54 | -1.82% | 0 |  | 2,001 | -4.12% |
| Render/int_arith | 33 | -2.94% | 15 | 0% | 0 |  | 746 | -1.32% |
| Module/keep | 2 | 0% | 1 | 0% | 0 |  | 371 | +0.82% |
| Module/keep_written | 2,929 | +0.14% | 562 | 0% | 0 |  | 714 | +0.14% |
| Load/large_static | 1,935 | 0% | 913 | 0% | 0 |  | 281 | +9.34% |
| Render/large_static | 710 | 0% | 567 | 0% | 0 |  | 4 | -33.33% |
| RenderSink/large_static | 0 |  | 0 |  | 0 |  | 3 | +50.00% |
| RenderTo/large_static | 687 | 0% | 571 | 0% | 0 |  | 3 | -50.00% |
| Load/macros | 79 | +5.33% | 73 | +2.82% | 0 |  | 1,709 | +0.12% |
| Render/macros | 758 | -0.79% | 232 | -2.11% | 0 |  | 3,468 | +70.17% |
| Load/many_tags | 27,967 | +0.13% | 22,516 | 0% | 0 |  | 211,848 | -4.54% |
| Render/many_tags | 6,248 | -0.05% | 858 | +1.30% | 0 |  | 169 | +4.97% |
| Load/mitsuhiko_table | 26 | +4.00% | 21 | +10.53% | 0 |  | 853 | -4.91% |
| Render/mitsuhiko_table | 10,051 | +0.05% | 6,292 | -0.06% | 0 |  | 360 | +1.98% |
| RenderSink/mitsuhiko_table | 9,950 | +0.04% | 1,198 | 0% | 0 |  | 302 | +9.03% |
| RenderTo/mitsuhiko_table | 10,049 | +0.08% | 6,300 | -0.06% | 0 |  | 327 | -1.21% |
| Load/mitsuhiko_table_wide | 66 | +1.54% | 77 | +2.67% | 0 |  | 932 | -2.61% |
| Render/mitsuhiko_table_wide | 10,437 | -0.01% | 22,637 | 0% | 0 |  | 361 | -2.96% |
| RenderSink/mitsuhiko_table_wide | 10,058 | -0.08% | 1,302 | -0.69% | 0 |  | 280 | -1.75% |
| RenderTo/mitsuhiko_table_wide | 10,428 | -0.10% | 22,685 | -0.03% | 0 |  | 329 | -3.52% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 29 | +93.33% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 5 | -16.67% |
| RenderSink/plain_text | 0 |  | 0 |  | 0 |  | 4 | -20.00% |
| RenderTo/plain_text | 0 |  | 0 |  | 0 |  | 4 | -20.00% |
| Load/strings | 116 | -2.52% | 92 | +2.22% | 0 |  | 2,740 | +2.20% |
| Render/strings | 399 | +0.50% | 472 | -0.84% | 0 |  | 1,570 | +0.13% |
| Load/substitute | 2 | 0% | 4 | +33.33% | 0 |  | 156 | -4.29% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 3 | -40.00% |
| RenderSink/substitute | 0 |  | 0 |  | 0 |  | 3 | 0% |
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
