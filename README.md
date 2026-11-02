# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (172 so far, latest [856dab2](https://github.com/jinja2cpp/Jinja2Cpp/commit/856dab252a3316d8bf18d7a8dd4a2f997176543f) on 2026-11-03). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 343,175 | 0% |  | 55 | 0% |  | 18,464 | 0% | -52.75% |
| [Render/chat_llama](#renderchat_llama) | 295,206 | +0.12% |  | 225 | 0% |  | 11,616 | 0% | -19.11% |
| [Load/chat_mistral](#loadchat_mistral) | 392,365 | 0% |  | 52 | 0% |  | 20,888 | 0% | -53.22% |
| [Render/chat_mistral](#renderchat_mistral) | 498,440 | -0.03% |  | 402 | 0% |  | 10,016 | -0.48% | -24.17% |
| [Load/chat_qwen](#loadchat_qwen) | 264,969 | 0% |  | 44 | 0% |  | 14,416 | 0% | -47.25% |
| [Render/chat_qwen](#renderchat_qwen) | 276,997 | +0.05% |  | 212 | 0% |  | 8,304 | 0% | -2.35% |
| [Load/config_file](#loadconfig_file) | 96,580 | 0% |  | 25 | 0% |  | 5,464 | 0% | -48.26% |
| [Render/config_file](#renderconfig_file) | 1,263,062 | +0.03% |  | 908 | 0% |  | 24,440 | 0% | +2.65% |
| [Load/dict_ops](#loaddict_ops) | 33,163 | -0.14% | -77.56% | 9 | 0% | -93.13% | 2,176 | 0% | -48.87% |
| [Render/dict_ops](#renderdict_ops) | 293,355 | +0.05% | -57.54% | 112 | 0% | -75.11% | 24,408 | 0% | -29.65% |
| [Load/expressions](#loadexpressions) | 44,894 | 0% | -78.22% | 16 | 0% | -89.04% | 2,728 | 0% | -55.54% |
| [Render/expressions](#renderexpressions) | 427,154 | 0% | -52.86% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 68,902 | 0% | -74.89% | 27 | 0% | -88.16% | 3,848 | 0% | -66.29% |
| [Render/filters](#renderfilters) | 30,238 | 0% | -65.90% | 26 | 0% | -64.86% | 1,080 | 0% | -78.01% |
| [Load/for_filter_if](#loadfor_filter_if) | 29,850 | -0.02% | -77.95% | 7 | 0% | -94.31% | 1,904 | 0% | -44.78% |
| [Render/for_filter_if](#renderfor_filter_if) | 275,074 | 0% | -68.22% | 4 | 0% | -98.23% | 16,560 | 0% | +354.95% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 40,078 | -0.10% | -77.82% | 10 | 0% | -93.63% | 2,368 | 0% | -47.61% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 192,484 | 0% | -82.06% | 2 | 0% | -99.62% | 1,568 | 0% | -43.35% |
| [Load/for_range](#loadfor_range) | 13,570 | 0% | -76.06% | 7 | 0% | -87.50% | 1,088 | 0% | -45.60% |
| [Render/for_range](#renderfor_range) | 38,806 | 0% | -57.92% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/hf_deepseek_v31](#loadhf_deepseek_v31) | 386,011 | +0.15% |  | 55 | 0% |  | 21,976 | 0% | +0.73% |
| [Render/hf_deepseek_v31](#renderhf_deepseek_v31) | 90,267 | -0.05% |  | 43 | 0% |  | 2,480 | 0% | 0% |
| [Load/hf_gpt_oss](#loadhf_gpt_oss) | 1,532,250 | +0.02% |  | 285 | 0% |  | 74,384 | 0% | +0.22% |
| [Render/hf_gpt_oss](#renderhf_gpt_oss) | 170,128 | +0.11% |  | 155 | 0% |  | 5,064 | 0% | +2.59% |
| [Load/hf_mistral_small](#loadhf_mistral_small) | 595,499 | +0.02% |  | 88 | 0% |  | 33,048 | 0% | +0.34% |
| [Render/hf_mistral_small](#renderhf_mistral_small) | 144,572 | 0% |  | 105 | 0% |  | 6,120 | 0% | -0.26% |
| [Load/hf_qwen3](#loadhf_qwen3) | 483,379 | -0.02% |  | 82 | 0% |  | 25,560 | 0% | +0.31% |
| [Render/hf_qwen3](#renderhf_qwen3) | 119,985 | 0% |  | 100 | 0% |  | 3,248 | 0% | 0% |
| [Load/html_autoescape](#loadhtml_autoescape) | 192,845 | +0.02% |  | 43 | 0% |  | 12,640 | 0% | -48.78% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,210,430 | -0.01% |  | 674 | 0% |  | 45,408 | 0% | +12.44% |
| [Load/inheritance](#loadinheritance) | 22,739 | 0% | -80.07% | 7 | 0% | -93.46% | 1,328 | 0% | -53.76% |
| [Render/inheritance](#renderinheritance) | 137,139 | +0.20% | -81.66% | 35 | 0% | -94.37% | 7,384 | 0% | +22.74% |
| [Load/int_arith](#loadint_arith) | 65,826 | +0.05% |  | 20 | 0% |  | 3,872 | 0% | +0.41% |
| [Render/int_arith](#renderint_arith) | 824,976 | 0% |  | 12 | 0% |  | 3,272 | 0% | -0.49% |
| [Module/keep](#modulekeep) | 6,951 | 0% |  | 7 | 0% |  |  |  |  |
| [Module/keep_written](#modulekeep_written) | 36,304 | -0.02% |  | 11 | 0% |  |  |  |  |
| [Load/large_static](#loadlarge_static) | 154,108 | 0% | -82.61% | 14 | 0% | -96.82% | 43,120 | 0% | -19.23% |
| [Render/large_static](#renderlarge_static) | 20,695 | 0% | -72.46% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [RenderSink/large_static](#rendersinklarge_static) | 12,681 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/large_static](#rendertolarge_static) | 20,258 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/macros](#loadmacros) | 55,672 | 0% | -75.58% | 27 | 0% | -85.48% | 4,344 | 0% | -30.12% |
| [Render/macros](#rendermacros) | 654,934 | -0.13% | -74.45% | 308 | 0% | -88.71% | 20,560 | 0% | +39.67% |
| [Load/many_tags](#loadmany_tags) | 8,967,107 | 0% | -79.70% | 951 | 0% | -97.22% | 438,176 | 0% | -58.48% |
| [Render/many_tags](#rendermany_tags) | 725,693 | 0% | -65.16% | 2 | 0% | -90.48% | 5,648 | 0% | -10.52% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 46,282 | 0% | -80.52% | 19 | 0% | -89.67% | 3,360 | 0% | -44.59% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,225,806 | 0% | -57.71% | 33 | 0% | -99.19% | 381,536 | 0% | +9.95% |
| [RenderSink/mitsuhiko_table](#rendersinkmitsuhiko_table) | 5,152,975 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table](#rendertomitsuhiko_table) | 5,225,289 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 54,520 | 0% |  | 35 | 0% |  | 5,688 | 0% | -31.17% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,571,751 | 0% |  | 33 | 0% |  | 1,414,928 | 0% | +2.46% |
| [RenderSink/mitsuhiko_table_wide](#rendersinkmitsuhiko_table_wide) | 5,395,038 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table_wide](#rendertomitsuhiko_table_wide) | 5,571,322 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/plain_text](#loadplain_text) | 3,692 | 0% | -61.84% | 4 | 0% | -77.78% | 672 | 0% | -40.00% |
| [Render/plain_text](#renderplain_text) | 1,012 | 0% | -85.10% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/plain_text](#rendersinkplain_text) | 712 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/plain_text](#rendertoplain_text) | 813 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/strings](#loadstrings) | 70,033 | -0.03% | -76.00% | 30 | 0% | -86.61% | 4,344 | 0% | -51.95% |
| [Render/strings](#renderstrings) | 582,730 | -0.06% | -72.22% | 344 | 0% | -83.73% | 9,088 | 0% | -49.31% |
| [Load/substitute](#loadsubstitute) | 7,709 | 0% | -74.18% | 6 | 0% | -83.78% | 832 | 0% | -45.26% |
| [Render/substitute](#rendersubstitute) | 2,228 | 0% | -74.72% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/substitute](#rendersinksubstitute) | 1,931 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/substitute](#rendertosubstitute) | 2,033 | 0% |  | 0 |  |  | 0 |  |  |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 726 | +0.55% | 732 | -1.08% | 0 |  | 7,309 | -0.19% |
| Render/chat_llama | 1,171 | -0.93% | 507 | -0.59% | 0 |  | 3,818 | +4.29% |
| Load/chat_mistral | 1,090 | -0.18% | 1,050 | -0.38% | 0 |  | 8,447 | +0.20% |
| Render/chat_mistral | 1,904 | -1.81% | 545 | -1.62% | 0 |  | 9,621 | +1.62% |
| Load/chat_qwen | 487 | -1.02% | 585 | +0.34% | 0 |  | 4,423 | +0.07% |
| Render/chat_qwen | 1,132 | +7.40% | 408 | +3.29% | 0 |  | 3,124 | +0.19% |
| Load/config_file | 176 | +7.32% | 152 | +4.11% | 0 |  | 2,758 | +2.19% |
| Render/config_file | 6,429 | +6.35% | 1,659 | +1.90% | 0 |  | 40,240 | +1.94% |
| Load/dict_ops | 34 | +21.43% | 17 | +41.67% | 0 |  | 1,346 | +3.54% |
| Render/dict_ops | 661 | +2.01% | 425 | -1.62% | 0 |  | 851 | -1.50% |
| Load/expressions | 41 | +17.14% | 24 | 0% | 0 |  | 1,299 | +4.17% |
| Render/expressions | 2 | 0% | 0 |  | 0 |  | 284 | +3.65% |
| Load/filters | 87 | +10.13% | 51 | -1.92% | 0 |  | 1,559 | +7.81% |
| Render/filters | 15 | -21.05% | 2 | 0% | 0 |  | 780 | -0.26% |
| Load/for_filter_if | 16 | +6.67% | 10 | +42.86% | 0 |  | 1,109 | +1.84% |
| Render/for_filter_if | 1,736 | +0.52% | 89 | -1.11% | 0 |  | 201 | +9.84% |
| Load/for_loop_vars | 41 | -10.87% | 25 | +13.64% | 0 |  | 1,153 | +3.32% |
| Render/for_loop_vars | 9 | +80.00% | 1 | 0% | 0 |  | 160 | +11.89% |
| Load/for_range | 8 | -11.11% | 7 | +75.00% | 0 |  | 740 | +6.63% |
| Render/for_range | 1 | 0% | 0 |  | 0 |  | 104 | -20.00% |
| Load/hf_deepseek_v31 | 951 | -2.06% | 963 | 0% | 0 |  | 6,056 | +0.51% |
| Render/hf_deepseek_v31 | 369 | +1.93% | 82 | +6.49% | 0 |  | 1,291 | -0.15% |
| Load/hf_gpt_oss | 4,523 | +0.04% | 3,248 | -0.85% | 0 |  | 21,259 | -0.51% |
| Render/hf_gpt_oss | 1,199 | -1.72% | 382 | +0.79% | 0 |  | 4,430 | +0.18% |
| Load/hf_mistral_small | 1,739 | -0.86% | 1,566 | -0.13% | 0 |  | 11,009 | +2.15% |
| Render/hf_mistral_small | 941 | -0.84% | 275 | +2.23% | 0 |  | 3,683 | -0.43% |
| Load/hf_qwen3 | 1,402 | -1.48% | 1,243 | -0.48% | 0 |  | 8,637 | -0.05% |
| Render/hf_qwen3 | 696 | +3.73% | 190 | +1.60% | 0 |  | 2,512 | +0.12% |
| Load/html_autoescape | 432 | 0% | 513 | 0% | 0 |  | 4,062 | +0.54% |
| Render/html_autoescape | 4,246 | +9.32% | 1,491 | +1.84% | 0 |  | 34,606 | -4.07% |
| Load/inheritance | 3 | -57.14% | 4 | +33.33% | 0 |  | 675 | +6.80% |
| Render/inheritance | 582 | 0% | 92 | -7.07% | 0 |  | 866 | -1.93% |
| Load/int_arith | 81 | +14.08% | 61 | +24.49% | 0 |  | 2,095 | +3.82% |
| Render/int_arith | 3 | -50.00% | 1 | -50.00% | 0 |  | 761 | +6.43% |
| Module/keep | 1 | -50.00% | 2 | 0% | 0 |  | 399 | -6.78% |
| Module/keep_written | 2,928 | +0.03% | 557 | 0% | 0 |  | 702 | -0.14% |
| Load/large_static | 1,928 | -0.05% | 913 | 0% | 0 |  | 283 | +8.85% |
| Render/large_static | 709 | 0% | 568 | 0% | 0 |  | 4 | -20.00% |
| RenderSink/large_static | 0 |  | 0 |  | 0 |  | 2 | 0% |
| RenderTo/large_static | 686 | 0% | 572 | 0% | 0 |  | 4 | -20.00% |
| Load/macros | 70 | -5.41% | 51 | -7.27% | 0 |  | 1,692 | -2.42% |
| Render/macros | 760 | +0.80% | 216 | -4.85% | 0 |  | 714 | +3.63% |
| Load/many_tags | 28,750 | +1.41% | 22,777 | +0.74% | 0 |  | 239,010 | +1.92% |
| Render/many_tags | 6,232 | -0.35% | 855 | +1.30% | 0 |  | 123 | +19.42% |
| Load/mitsuhiko_table | 19 | -17.39% | 18 | 0% | 0 |  | 841 | +3.83% |
| Render/mitsuhiko_table | 10,104 | +0.11% | 6,268 | +0.13% | 0 |  | 350 | -0.85% |
| RenderSink/mitsuhiko_table | 9,970 | +0.17% | 1,203 | 0% | 0 |  | 276 | -6.12% |
| RenderTo/mitsuhiko_table | 10,072 | +0.18% | 6,268 | +0.18% | 0 |  | 332 | +3.75% |
| Load/mitsuhiko_table_wide | 47 | -4.08% | 53 | +1.92% | 0 |  | 952 | -3.35% |
| Render/mitsuhiko_table_wide | 10,785 | +4.86% | 22,729 | +0.26% | 0 |  | 364 | -1.62% |
| RenderSink/mitsuhiko_table_wide | 10,152 | +0.81% | 1,316 | +1.31% | 0 |  | 290 | -1.36% |
| RenderTo/mitsuhiko_table_wide | 10,780 | +4.74% | 22,736 | +0.25% | 0 |  | 327 | -4.11% |
| Load/plain_text | 1 |  | 2 | 0% | 0 |  | 14 | +133.33% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 4 | -33.33% |
| RenderSink/plain_text | 0 |  | 0 |  | 0 |  | 3 | -40.00% |
| RenderTo/plain_text | 0 |  | 0 |  | 0 |  | 4 | -33.33% |
| Load/strings | 126 | +0.80% | 77 | +4.05% | 0 |  | 2,710 | +2.19% |
| Render/strings | 405 | -2.17% | 458 | -0.22% | 0 |  | 1,521 | -1.93% |
| Load/substitute | 2 | 0% | 3 | 0% | 0 |  | 133 | +8.13% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 4 | +33.33% |
| RenderSink/substitute | 0 |  | 0 |  | 0 |  | 3 | +50.00% |
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
