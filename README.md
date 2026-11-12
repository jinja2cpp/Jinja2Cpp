# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (209 so far, latest [d9aecf2](https://github.com/jinja2cpp/Jinja2Cpp/commit/d9aecf240953fefe785921b6e73a6afaec7a613b) on 2026-11-12). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 336,221 | 0% |  | 55 | 0% |  | 18,464 | 0% | -52.75% |
| [Render/chat_llama](#renderchat_llama) | 294,228 | 0% |  | 225 | 0% |  | 11,616 | 0% | -19.11% |
| [Load/chat_mistral](#loadchat_mistral) | 386,346 | 0% |  | 52 | 0% |  | 20,888 | 0% | -53.22% |
| [Render/chat_mistral](#renderchat_mistral) | 499,753 | +0.03% |  | 402 | 0% |  | 10,000 | 0% | -24.29% |
| [Load/chat_qwen](#loadchat_qwen) | 262,260 | 0% |  | 44 | 0% |  | 14,416 | 0% | -47.25% |
| [Render/chat_qwen](#renderchat_qwen) | 276,401 | 0% |  | 212 | 0% |  | 8,304 | 0% | -2.35% |
| [Load/config_file](#loadconfig_file) | 95,107 | 0% |  | 25 | 0% |  | 5,464 | 0% | -48.26% |
| [Render/config_file](#renderconfig_file) | 1,276,620 | -0.01% |  | 908 | 0% |  | 24,440 | 0% | +2.65% |
| [Load/dict_ops](#loaddict_ops) | 32,374 | 0% | -78.09% | 9 | 0% | -93.13% | 2,176 | 0% | -48.87% |
| [Render/dict_ops](#renderdict_ops) | 293,373 | +0.01% | -57.53% | 112 | 0% | -75.11% | 24,408 | 0% | -29.65% |
| [Load/expressions](#loadexpressions) | 44,205 | 0% | -78.56% | 16 | 0% | -89.04% | 2,728 | 0% | -55.54% |
| [Render/expressions](#renderexpressions) | 426,921 | +0.01% | -52.89% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 67,957 | 0% | -75.24% | 27 | 0% | -88.16% | 3,848 | 0% | -66.29% |
| [Render/filters](#renderfilters) | 27,415 | +0.04% | -69.08% | 23 | 0% | -68.92% | 752 | 0% | -84.69% |
| [Load/for_filter_if](#loadfor_filter_if) | 29,274 | 0% | -78.38% | 7 | 0% | -94.31% | 1,904 | 0% | -44.78% |
| [Render/for_filter_if](#renderfor_filter_if) | 274,354 | 0% | -68.30% | 4 | 0% | -98.23% | 16,560 | 0% | +354.95% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 39,161 | 0% | -78.32% | 10 | 0% | -93.63% | 2,368 | 0% | -47.61% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 180,548 | 0% | -83.17% | 2 | 0% | -99.62% | 1,568 | 0% | -43.35% |
| [Load/for_range](#loadfor_range) | 13,250 | 0% | -76.62% | 7 | 0% | -87.50% | 1,088 | 0% | -45.60% |
| [Render/for_range](#renderfor_range) | 38,372 | +0.07% | -58.39% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/hf_deepseek_v31](#loadhf_deepseek_v31) | 383,729 | 0% |  | 55 | 0% |  | 21,976 | 0% | +0.73% |
| [Render/hf_deepseek_v31](#renderhf_deepseek_v31) | 91,503 | -0.01% |  | 43 | 0% |  | 2,480 | 0% | 0% |
| [Load/hf_gpt_oss](#loadhf_gpt_oss) | 1,529,904 | 0% |  | 287 | 0% |  | 74,704 | 0% | +0.65% |
| [Render/hf_gpt_oss](#renderhf_gpt_oss) | 171,943 | -0.07% |  | 155 | 0% |  | 5,096 | 0% | +3.24% |
| [Load/hf_mistral_small](#loadhf_mistral_small) | 592,628 | 0% |  | 88 | 0% |  | 33,048 | 0% | +0.34% |
| [Render/hf_mistral_small](#renderhf_mistral_small) | 141,616 | +0.01% |  | 99 | 0% |  | 6,120 | 0% | -0.26% |
| [Load/hf_qwen3](#loadhf_qwen3) | 478,070 | 0% |  | 82 | 0% |  | 25,560 | 0% | +0.31% |
| [Render/hf_qwen3](#renderhf_qwen3) | 120,269 | 0% |  | 100 | 0% |  | 3,248 | 0% | 0% |
| [Load/html_autoescape](#loadhtml_autoescape) | 190,670 | 0% |  | 43 | 0% |  | 12,672 | 0% | -48.65% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,208,512 | 0% |  | 674 | 0% |  | 45,408 | 0% | +12.44% |
| [Load/inheritance](#loadinheritance) | 21,971 | 0% | -80.75% | 7 | 0% | -93.46% | 1,344 | 0% | -53.20% |
| [Render/inheritance](#renderinheritance) | 137,483 | +0.01% | -81.62% | 35 | 0% | -94.37% | 7,384 | 0% | +22.74% |
| [Load/int_arith](#loadint_arith) | 65,190 | 0% |  | 20 | 0% |  | 3,872 | 0% | +0.41% |
| [Render/int_arith](#renderint_arith) | 826,982 | -0.02% |  | 12 | 0% |  | 3,272 | 0% | -0.49% |
| [Module/keep](#modulekeep) | 7,107 | 0% |  | 7 | 0% |  |  |  |  |
| [Module/keep_written](#modulekeep_written) | 28,989 | 0% |  | 11 | 0% |  |  |  |  |
| [Load/large_static](#loadlarge_static) | 149,411 | 0% | -83.14% | 14 | 0% | -96.82% | 43,120 | 0% | -19.23% |
| [Render/large_static](#renderlarge_static) | 20,722 | +0.05% | -72.42% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [RenderSink/large_static](#rendersinklarge_static) | 12,712 | +0.08% |  | 0 |  |  | 0 |  |  |
| [RenderTo/large_static](#rendertolarge_static) | 20,285 | +0.05% |  | 0 |  |  | 0 |  |  |
| [Load/macros](#loadmacros) | 55,622 | 0% | -75.60% | 27 | 0% | -85.48% | 4,392 | 0% | -29.34% |
| [Render/macros](#rendermacros) | 660,056 | 0% | -74.25% | 308 | 0% | -88.71% | 20,560 | 0% | +39.67% |
| [Load/many_tags](#loadmany_tags) | 8,793,891 | 0% | -80.09% | 951 | 0% | -97.22% | 438,112 | 0% | -58.48% |
| [Render/many_tags](#rendermany_tags) | 728,413 | 0% | -65.03% | 2 | 0% | -90.48% | 5,648 | 0% | -10.52% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 45,391 | 0% | -80.89% | 19 | 0% | -89.67% | 3,360 | 0% | -44.59% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,202,688 | 0% | -57.89% | 33 | 0% | -99.19% | 381,536 | 0% | +9.95% |
| [RenderSink/mitsuhiko_table](#rendersinkmitsuhiko_table) | 5,129,885 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table](#rendertomitsuhiko_table) | 5,202,194 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 53,132 | -0.06% |  | 35 | 0% |  | 5,688 | 0% | -31.17% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,548,845 | 0% |  | 33 | 0% |  | 1,414,928 | 0% | +2.46% |
| [RenderSink/mitsuhiko_table_wide](#rendersinkmitsuhiko_table_wide) | 5,372,150 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table_wide](#rendertomitsuhiko_table_wide) | 5,548,413 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/plain_text](#loadplain_text) | 3,713 | 0% | -61.63% | 4 | 0% | -77.78% | 672 | 0% | -40.00% |
| [Render/plain_text](#renderplain_text) | 1,032 | +0.98% | -84.81% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/plain_text](#rendersinkplain_text) | 734 | +1.38% |  | 0 |  |  | 0 |  |  |
| [RenderTo/plain_text](#rendertoplain_text) | 833 | +1.22% |  | 0 |  |  | 0 |  |  |
| [Load/prompt_budget](#loadprompt_budget) | 34,625 | 0% |  | 18 | 0% |  | 2,224 | 0% | 0% |
| [Render/prompt_budget](#renderprompt_budget) | 1,315,828 | +0.01% |  | 76 | 0% |  | 36,416 | 0% | 0% |
| [Load/strings](#loadstrings) | 69,440 | 0% | -76.21% | 30 | 0% | -86.61% | 4,344 | 0% | -51.95% |
| [Render/strings](#renderstrings) | 582,615 | -0.02% | -72.22% | 344 | 0% | -83.73% | 9,088 | 0% | -49.31% |
| [Load/substitute](#loadsubstitute) | 7,512 | 0% | -74.84% | 6 | 0% | -83.78% | 832 | 0% | -45.26% |
| [Render/substitute](#rendersubstitute) | 2,265 | +0.44% | -74.30% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/substitute](#rendersinksubstitute) | 1,971 | +0.51% |  | 0 |  |  | 0 |  |  |
| [RenderTo/substitute](#rendertosubstitute) | 2,070 | +0.49% |  | 0 |  |  | 0 |  |  |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 738 | -1.34% | 741 | +0.41% | 0 |  | 7,957 | -0.31% |
| Render/chat_llama | 1,151 | -1.03% | 493 | 0% | 0 |  | 3,816 | +1.27% |
| Load/chat_mistral | 1,089 | +0.74% | 1,086 | +0.65% | 0 |  | 9,194 | -1.25% |
| Render/chat_mistral | 1,940 | +0.94% | 549 | +2.04% | 0 |  | 9,374 | -7.35% |
| Load/chat_qwen | 615 | -0.65% | 630 | +0.16% | 0 |  | 5,137 | +0.77% |
| Render/chat_qwen | 1,104 | -0.36% | 372 | -1.06% | 0 |  | 3,231 | -1.37% |
| Load/config_file | 226 | -0.44% | 209 | -0.48% | 0 |  | 2,826 | -2.72% |
| Render/config_file | 6,030 | -2.63% | 1,411 | -7.60% | 0 |  | 41,659 | -3.13% |
| Load/dict_ops | 58 | -1.69% | 31 | +3.33% | 0 |  | 1,419 | +1.36% |
| Render/dict_ops | 676 | -2.03% | 433 | 0% | 0 |  | 856 | -3.49% |
| Load/expressions | 20 | -16.67% | 16 | -15.79% | 0 |  | 1,369 | -0.73% |
| Render/expressions | 13 | -23.53% | 5 | -28.57% | 0 |  | 301 | -9.34% |
| Load/filters | 94 | +13.25% | 49 | +6.52% | 0 |  | 1,754 | -7.83% |
| Render/filters | 20 | +5.26% | 6 | -25.00% | 0 |  | 776 | +0.26% |
| Load/for_filter_if | 31 | 0% | 13 | -13.33% | 0 |  | 1,135 | +3.09% |
| Render/for_filter_if | 1,729 | 0% | 133 | -0.75% | 0 |  | 178 | -17.97% |
| Load/for_loop_vars | 39 | +14.71% | 24 | +9.09% | 0 |  | 1,176 | +3.52% |
| Render/for_loop_vars | 10 | -16.67% | 0 | -100.00% | 0 |  | 127 | -37.13% |
| Load/for_range | 6 | +100.00% | 5 | +66.67% | 0 |  | 775 | +0.78% |
| Render/for_range | 5 | 0% | 0 | -100.00% | 0 |  | 123 | -17.45% |
| Load/hf_deepseek_v31 | 997 | +1.94% | 983 | -0.20% | 0 |  | 7,062 | -1.26% |
| Render/hf_deepseek_v31 | 372 | +3.33% | 78 | -3.70% | 0 |  | 1,338 | -8.48% |
| Load/hf_gpt_oss | 4,701 | +0.28% | 3,366 | +0.03% | 0 |  | 24,642 | -5.60% |
| Render/hf_gpt_oss | 1,208 | 0% | 384 | +4.07% | 0 |  | 4,401 | -7.01% |
| Load/hf_mistral_small | 1,906 | +0.85% | 1,620 | +0.37% | 0 |  | 11,925 | -2.19% |
| Render/hf_mistral_small | 947 | +0.74% | 277 | -3.15% | 0 |  | 3,480 | -5.33% |
| Load/hf_qwen3 | 1,536 | +1.32% | 1,325 | -0.53% | 0 |  | 9,635 | -0.81% |
| Render/hf_qwen3 | 659 | +1.85% | 200 | -2.91% | 0 |  | 2,542 | -4.15% |
| Load/html_autoescape | 504 | +3.92% | 544 | +0.93% | 0 |  | 4,351 | -0.87% |
| Render/html_autoescape | 4,150 | -22.83% | 1,271 | -20.21% | 0 |  | 37,009 | +0.48% |
| Load/inheritance | 6 | +100.00% | 2 | 0% | 0 |  | 706 | +0.43% |
| Render/inheritance | 594 | -1.33% | 106 | +1.92% | 0 |  | 989 | +8.80% |
| Load/int_arith | 77 | -2.53% | 58 | +1.75% | 0 |  | 2,131 | -2.74% |
| Render/int_arith | 25 | +19.05% | 6 | -14.29% | 0 |  | 764 | -0.39% |
| Module/keep | 1 | 0% | 2 | 0% | 0 |  | 409 | +0.74% |
| Module/keep_written | 1,128 | 0% | 564 | 0% | 0 |  | 734 | +0.55% |
| Load/large_static | 1,929 | -0.05% | 907 | 0% | 0 |  | 314 | -7.10% |
| Render/large_static | 707 | -0.14% | 569 | +0.35% | 0 |  | 6 | -14.29% |
| RenderSink/large_static | 0 |  | 0 |  | 0 |  | 2 | -50.00% |
| RenderTo/large_static | 685 | 0% | 570 | +0.18% | 0 |  | 5 | 0% |
| Load/macros | 85 | -1.16% | 63 | -16.00% | 0 |  | 1,783 | -1.44% |
| Render/macros | 751 | -0.27% | 236 | +1.29% | 0 |  | 620 | -37.88% |
| Load/many_tags | 28,661 | +1.53% | 22,787 | +0.47% | 0 |  | 253,686 | -2.15% |
| Render/many_tags | 6,238 | -0.18% | 883 | +2.44% | 0 |  | 126 | -25.88% |
| Load/mitsuhiko_table | 33 | +6.45% | 46 | 0% | 0 |  | 914 | +0.11% |
| Render/mitsuhiko_table | 10,060 | -0.15% | 6,270 | +0.02% | 0 |  | 347 | -7.22% |
| RenderSink/mitsuhiko_table | 9,943 | -0.26% | 1,183 | -0.42% | 0 |  | 275 | -13.79% |
| RenderTo/mitsuhiko_table | 10,042 | -0.13% | 6,277 | +0.03% | 0 |  | 319 | -5.62% |
| Load/mitsuhiko_table_wide | 78 | -1.27% | 79 | -11.24% | 0 |  | 1,082 | +7.02% |
| Render/mitsuhiko_table_wide | 10,251 | -0.11% | 22,674 | -0.01% | 0 |  | 345 | -1.43% |
| RenderSink/mitsuhiko_table_wide | 10,067 | -0.10% | 1,301 | +0.31% | 0 |  | 279 | -6.69% |
| RenderTo/mitsuhiko_table_wide | 10,524 | +2.40% | 22,692 | +0.05% | 0 |  | 301 | -5.64% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 16 | -30.43% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 7 | +16.67% |
| RenderSink/plain_text | 0 |  | 0 |  | 0 |  | 5 | 0% |
| RenderTo/plain_text | 0 |  | 0 |  | 0 |  | 7 | +40.00% |
| Load/prompt_budget | 74 | -8.64% | 44 | -2.22% | 0 |  | 1,414 | -0.42% |
| Render/prompt_budget | 4,811 | +0.06% | 539 | 0% | 0 |  | 856 | -5.73% |
| Load/strings | 167 | +7.05% | 132 | +2.33% | 0 |  | 2,782 | -2.21% |
| Render/strings | 431 | -4.22% | 488 | 0% | 0 |  | 2,492 | +57.03% |
| Load/substitute | 2 | 0% | 3 | 0% | 0 |  | 188 | +4.44% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 5 | +25.00% |
| RenderSink/substitute | 0 |  | 0 |  | 0 |  | 3 | -25.00% |
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
