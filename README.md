# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (177 so far, latest [e7a17c0](https://github.com/jinja2cpp/Jinja2Cpp/commit/e7a17c0fc8437bbd648508042e0012f48f788178) on 2026-11-05). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 344,895 | +0.46% |  | 55 | 0% |  | 18,464 | 0% | -52.75% |
| [Render/chat_llama](#renderchat_llama) | 295,167 | +0.15% |  | 225 | 0% |  | 11,616 | -0.14% | -19.11% |
| [Load/chat_mistral](#loadchat_mistral) | 393,400 | +0.24% |  | 52 | 0% |  | 20,888 | 0% | -53.22% |
| [Render/chat_mistral](#renderchat_mistral) | 499,377 | +0.09% |  | 402 | 0% |  | 10,064 | +0.32% | -23.80% |
| [Load/chat_qwen](#loadchat_qwen) | 265,887 | +0.26% |  | 44 | 0% |  | 14,416 | 0% | -47.25% |
| [Render/chat_qwen](#renderchat_qwen) | 276,843 | -0.09% |  | 212 | 0% |  | 8,304 | 0% | -2.35% |
| [Load/config_file](#loadconfig_file) | 97,003 | +0.34% |  | 25 | 0% |  | 5,464 | 0% | -48.26% |
| [Render/config_file](#renderconfig_file) | 1,266,068 | +0.07% |  | 908 | 0% |  | 24,440 | 0% | +2.65% |
| [Load/dict_ops](#loaddict_ops) | 33,316 | +0.43% | -77.46% | 9 | 0% | -93.13% | 2,176 | 0% | -48.87% |
| [Render/dict_ops](#renderdict_ops) | 292,547 | +0.27% | -57.65% | 112 | 0% | -75.11% | 24,408 | 0% | -29.65% |
| [Load/expressions](#loadexpressions) | 44,974 | +0.18% | -78.18% | 16 | 0% | -89.04% | 2,728 | 0% | -55.54% |
| [Render/expressions](#renderexpressions) | 426,194 | 0% | -52.97% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 69,089 | +0.26% | -74.83% | 27 | 0% | -88.16% | 3,848 | 0% | -66.29% |
| [Render/filters](#renderfilters) | 30,368 | 0% | -65.75% | 26 | 0% | -64.86% | 1,080 | 0% | -78.01% |
| [Load/for_filter_if](#loadfor_filter_if) | 30,000 | +0.41% | -77.84% | 7 | 0% | -94.31% | 1,904 | 0% | -44.78% |
| [Render/for_filter_if](#renderfor_filter_if) | 274,669 | 0% | -68.26% | 4 | 0% | -98.23% | 16,560 | 0% | +354.95% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 40,302 | +0.47% | -77.69% | 10 | 0% | -93.63% | 2,368 | 0% | -47.61% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 194,105 | +0.10% | -81.91% | 2 | 0% | -99.62% | 1,568 | 0% | -43.35% |
| [Load/for_range](#loadfor_range) | 13,631 | +0.44% | -75.95% | 7 | 0% | -87.50% | 1,088 | 0% | -45.60% |
| [Render/for_range](#renderfor_range) | 38,346 | -0.01% | -58.41% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/hf_deepseek_v31](#loadhf_deepseek_v31) | 386,738 | +0.30% |  | 55 | 0% |  | 21,976 | 0% | +0.73% |
| [Render/hf_deepseek_v31](#renderhf_deepseek_v31) | 90,302 | +0.02% |  | 43 | 0% |  | 2,480 | 0% | 0% |
| [Load/hf_gpt_oss](#loadhf_gpt_oss) | 1,546,617 | +0.80% |  | 287 | +0.70% |  | 74,672 | +0.34% | +0.60% |
| [Render/hf_gpt_oss](#renderhf_gpt_oss) | 170,388 | +0.04% |  | 155 | 0% |  | 5,096 | 0% | +3.24% |
| [Load/hf_mistral_small](#loadhf_mistral_small) | 596,950 | +0.22% |  | 88 | 0% |  | 33,048 | 0% | +0.34% |
| [Render/hf_mistral_small](#renderhf_mistral_small) | 144,752 | +0.05% |  | 105 | 0% |  | 6,120 | 0% | -0.26% |
| [Load/hf_qwen3](#loadhf_qwen3) | 485,238 | +0.28% |  | 82 | 0% |  | 25,560 | 0% | +0.31% |
| [Render/hf_qwen3](#renderhf_qwen3) | 120,106 | -0.02% |  | 100 | 0% |  | 3,248 | 0% | 0% |
| [Load/html_autoescape](#loadhtml_autoescape) | 193,754 | +0.35% |  | 43 | 0% |  | 12,640 | 0% | -48.78% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,210,826 | 0% |  | 674 | 0% |  | 45,408 | 0% | +12.44% |
| [Load/inheritance](#loadinheritance) | 22,878 | +0.61% | -79.95% | 7 | 0% | -93.46% | 1,328 | 0% | -53.76% |
| [Render/inheritance](#renderinheritance) | 137,008 | -0.01% | -81.68% | 35 | 0% | -94.37% | 7,384 | 0% | +22.74% |
| [Load/int_arith](#loadint_arith) | 65,995 | +0.24% |  | 20 | 0% |  | 3,872 | 0% | +0.41% |
| [Render/int_arith](#renderint_arith) | 824,272 | +0.02% |  | 12 | 0% |  | 3,272 | 0% | -0.49% |
| [Module/keep](#modulekeep) | 6,960 | 0% |  | 7 | 0% |  |  |  |  |
| [Module/keep_written](#modulekeep_written) | 36,344 | -0.02% |  | 11 | 0% |  |  |  |  |
| [Load/large_static](#loadlarge_static) | 154,576 | +0.27% | -82.55% | 14 | 0% | -96.82% | 43,120 | 0% | -19.23% |
| [Render/large_static](#renderlarge_static) | 20,700 | 0% | -72.45% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [RenderSink/large_static](#rendersinklarge_static) | 12,686 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/large_static](#rendertolarge_static) | 20,263 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/macros](#loadmacros) | 56,253 | +0.97% | -75.32% | 27 | 0% | -85.48% | 4,392 | +1.10% | -29.34% |
| [Render/macros](#rendermacros) | 656,289 | 0% | -74.39% | 308 | 0% | -88.71% | 20,560 | 0% | +39.67% |
| [Load/many_tags](#loadmany_tags) | 9,012,602 | +0.44% | -79.59% | 951 | 0% | -97.22% | 438,144 | 0% | -58.48% |
| [Render/many_tags](#rendermany_tags) | 725,698 | 0% | -65.16% | 2 | 0% | -90.48% | 5,648 | 0% | -10.52% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 46,462 | +0.36% | -80.44% | 19 | 0% | -89.67% | 3,360 | 0% | -44.59% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,180,778 | -0.08% | -58.07% | 33 | 0% | -99.19% | 381,536 | 0% | +9.95% |
| [RenderSink/mitsuhiko_table](#rendersinkmitsuhiko_table) | 5,108,016 | -0.08% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table](#rendertomitsuhiko_table) | 5,180,276 | -0.08% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 54,700 | +0.31% |  | 35 | 0% |  | 5,688 | 0% | -31.17% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,526,780 | -0.07% |  | 33 | 0% |  | 1,414,928 | 0% | +2.46% |
| [RenderSink/mitsuhiko_table_wide](#rendersinkmitsuhiko_table_wide) | 5,350,087 | -0.07% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table_wide](#rendertomitsuhiko_table_wide) | 5,526,352 | -0.07% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/plain_text](#loadplain_text) | 3,714 | +0.57% | -61.62% | 4 | 0% | -77.78% | 672 | 0% | -40.00% |
| [Render/plain_text](#renderplain_text) | 1,017 | 0% | -85.03% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/plain_text](#rendersinkplain_text) | 717 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/plain_text](#rendertoplain_text) | 818 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/strings](#loadstrings) | 70,226 | +0.24% | -75.94% | 30 | 0% | -86.61% | 4,344 | 0% | -51.95% |
| [Render/strings](#renderstrings) | 582,109 | +0.04% | -72.25% | 344 | 0% | -83.73% | 9,088 | 0% | -49.31% |
| [Load/substitute](#loadsubstitute) | 7,743 | +0.43% | -74.07% | 6 | 0% | -83.78% | 832 | 0% | -45.26% |
| [Render/substitute](#rendersubstitute) | 2,233 | 0% | -74.66% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/substitute](#rendersinksubstitute) | 1,936 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/substitute](#rendertosubstitute) | 2,038 | 0% |  | 0 |  |  | 0 |  |  |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 727 | -3.07% | 734 | -1.21% | 0 |  | 7,372 | +1.25% |
| Render/chat_llama | 1,185 | +1.63% | 518 | 0% | 0 |  | 4,009 | +1.83% |
| Load/chat_mistral | 1,091 | -1.80% | 1,056 | -1.68% | 0 |  | 8,347 | +0.37% |
| Render/chat_mistral | 1,832 | -1.40% | 530 | -1.12% | 0 |  | 9,556 | -2.91% |
| Load/chat_qwen | 570 | +14.69% | 598 | +1.01% | 0 |  | 4,347 | -2.18% |
| Render/chat_qwen | 1,100 | +2.52% | 391 | -2.74% | 0 |  | 3,357 | -0.74% |
| Load/config_file | 188 | +3.87% | 161 | +4.55% | 0 |  | 2,587 | -2.56% |
| Render/config_file | 6,282 | +2.13% | 1,585 | -1.43% | 0 |  | 41,703 | -3.02% |
| Load/dict_ops | 30 | 0% | 18 | +38.46% | 0 |  | 1,287 | +0.31% |
| Render/dict_ops | 668 | 0% | 428 | -0.23% | 0 |  | 885 | +2.67% |
| Load/expressions | 36 | +5.88% | 24 | 0% | 0 |  | 1,303 | +0.46% |
| Render/expressions | 2 | 0% | 1 | 0% | 0 |  | 304 | +8.19% |
| Load/filters | 93 | +3.33% | 54 | +25.58% | 0 |  | 1,545 | -2.83% |
| Render/filters | 17 | 0% | 5 | 0% | 0 |  | 769 | -1.54% |
| Load/for_filter_if | 21 | 0% | 11 | +10.00% | 0 |  | 1,066 | -2.38% |
| Render/for_filter_if | 1,731 | 0% | 92 | 0% | 0 |  | 199 | +13.07% |
| Load/for_loop_vars | 43 | +16.22% | 26 | +18.18% | 0 |  | 1,064 | -3.62% |
| Render/for_loop_vars | 9 | 0% | 0 | -100.00% | 0 |  | 160 | +14.29% |
| Load/for_range | 16 | +128.57% | 8 | +60.00% | 0 |  | 738 | +0.41% |
| Render/for_range | 1 | 0% | 0 |  | 0 |  | 100 | -2.91% |
| Load/hf_deepseek_v31 | 986 | -0.50% | 969 | -0.72% | 0 |  | 6,099 | +5.03% |
| Render/hf_deepseek_v31 | 351 | -2.50% | 77 | -9.41% | 0 |  | 1,421 | +3.42% |
| Load/hf_gpt_oss | 4,615 | +0.33% | 3,297 | +1.76% | 0 |  | 21,070 | +2.13% |
| Render/hf_gpt_oss | 1,207 | -0.49% | 374 | -1.58% | 0 |  | 4,484 | -1.90% |
| Load/hf_mistral_small | 1,784 | +0.34% | 1,593 | +1.14% | 0 |  | 10,678 | +0.26% |
| Render/hf_mistral_small | 962 | +0.31% | 316 | +0.64% | 0 |  | 3,759 | -1.83% |
| Load/hf_qwen3 | 1,441 | -0.14% | 1,248 | 0% | 0 |  | 8,619 | -0.47% |
| Render/hf_qwen3 | 663 | -1.78% | 213 | +1.43% | 0 |  | 2,601 | +1.68% |
| Load/html_autoescape | 433 | -2.26% | 514 | -0.19% | 0 |  | 3,774 | -4.58% |
| Render/html_autoescape | 3,453 | -10.96% | 1,257 | -0.08% | 0 |  | 36,209 | -1.98% |
| Load/inheritance | 3 | 0% | 4 | +33.33% | 0 |  | 640 | -1.39% |
| Render/inheritance | 587 | 0% | 96 | +1.05% | 0 |  | 924 | +2.10% |
| Load/int_arith | 85 | 0% | 60 | +17.65% | 0 |  | 2,078 | -0.34% |
| Render/int_arith | 7 | 0% | 4 | 0% | 0 |  | 759 | +0.40% |
| Module/keep | 2 | 0% | 2 | 0% | 0 |  | 416 | +9.19% |
| Module/keep_written | 2,929 | +0.03% | 557 | -0.36% | 0 |  | 739 | +1.93% |
| Load/large_static | 1,934 | +0.26% | 910 | -0.33% | 0 |  | 292 | +2.46% |
| Render/large_static | 710 | 0% | 568 | 0% | 0 |  | 12 | +300.00% |
| RenderSink/large_static | 0 |  | 0 |  | 0 |  | 4 | +100.00% |
| RenderTo/large_static | 687 | 0% | 572 | 0% | 0 |  | 5 | +66.67% |
| Load/macros | 78 | +11.43% | 63 | +26.00% | 0 |  | 1,632 | -3.72% |
| Render/macros | 768 | +0.26% | 216 | +0.47% | 0 |  | 1,514 | -7.34% |
| Load/many_tags | 28,169 | -0.08% | 22,587 | 0% | 0 |  | 216,182 | -4.58% |
| Render/many_tags | 6,227 | 0% | 844 | 0% | 0 |  | 158 | -5.39% |
| Load/mitsuhiko_table | 24 | -4.00% | 24 | 0% | 0 |  | 784 | -5.77% |
| Render/mitsuhiko_table | 10,086 | +0.01% | 6,256 | -0.03% | 0 |  | 375 | +13.29% |
| RenderSink/mitsuhiko_table | 9,970 | +0.04% | 1,201 | -0.33% | 0 |  | 294 | +6.52% |
| RenderTo/mitsuhiko_table | 10,066 | +0.03% | 6,260 | +0.08% | 0 |  | 361 | +18.75% |
| Load/mitsuhiko_table_wide | 55 | +7.84% | 59 | +18.00% | 0 |  | 948 | -5.48% |
| Render/mitsuhiko_table_wide | 10,490 | -0.01% | 22,783 | +0.03% | 0 |  | 330 | -0.90% |
| RenderSink/mitsuhiko_table_wide | 10,131 | +0.12% | 1,303 | -0.15% | 0 |  | 280 | +3.32% |
| RenderTo/mitsuhiko_table_wide | 10,493 | +0.03% | 22,678 | +0.04% | 0 |  | 316 | +5.33% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 14 | +133.33% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 7 | +40.00% |
| RenderSink/plain_text | 0 |  | 0 |  | 0 |  | 5 | +25.00% |
| RenderTo/plain_text | 0 |  | 0 |  | 0 |  | 6 | +20.00% |
| Load/strings | 123 | +6.03% | 86 | +11.69% | 0 |  | 2,649 | -1.12% |
| Render/strings | 406 | 0% | 478 | -0.21% | 0 |  | 1,580 | -0.75% |
| Load/substitute | 2 | -33.33% | 4 | 0% | 0 |  | 132 | -7.04% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 4 | +33.33% |
| RenderSink/substitute | 0 |  | 0 |  | 0 |  | 4 | +100.00% |
| RenderTo/substitute | 0 |  | 0 |  | 0 |  | 4 | +100.00% |

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
