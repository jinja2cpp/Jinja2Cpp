# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (178 so far, latest [4e129e9](https://github.com/jinja2cpp/Jinja2Cpp/commit/4e129e92c77885bec9a2c0edfe2a816446371d48) on 2026-11-06). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 344,895 | 0% |  | 55 | 0% |  | 18,464 | 0% | -52.75% |
| [Render/chat_llama](#renderchat_llama) | 295,167 | 0% |  | 225 | 0% |  | 11,616 | 0% | -19.11% |
| [Load/chat_mistral](#loadchat_mistral) | 393,400 | 0% |  | 52 | 0% |  | 20,888 | 0% | -53.22% |
| [Render/chat_mistral](#renderchat_mistral) | 499,377 | 0% |  | 402 | 0% |  | 10,064 | 0% | -23.80% |
| [Load/chat_qwen](#loadchat_qwen) | 265,887 | 0% |  | 44 | 0% |  | 14,416 | 0% | -47.25% |
| [Render/chat_qwen](#renderchat_qwen) | 276,843 | 0% |  | 212 | 0% |  | 8,304 | 0% | -2.35% |
| [Load/config_file](#loadconfig_file) | 97,003 | 0% |  | 25 | 0% |  | 5,464 | 0% | -48.26% |
| [Render/config_file](#renderconfig_file) | 1,266,068 | 0% |  | 908 | 0% |  | 24,440 | 0% | +2.65% |
| [Load/dict_ops](#loaddict_ops) | 33,316 | 0% | -77.46% | 9 | 0% | -93.13% | 2,176 | 0% | -48.87% |
| [Render/dict_ops](#renderdict_ops) | 292,547 | 0% | -57.65% | 112 | 0% | -75.11% | 24,408 | 0% | -29.65% |
| [Load/expressions](#loadexpressions) | 44,974 | 0% | -78.18% | 16 | 0% | -89.04% | 2,728 | 0% | -55.54% |
| [Render/expressions](#renderexpressions) | 426,194 | 0% | -52.97% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 69,089 | 0% | -74.83% | 27 | 0% | -88.16% | 3,848 | 0% | -66.29% |
| [Render/filters](#renderfilters) | 30,368 | 0% | -65.75% | 26 | 0% | -64.86% | 1,080 | 0% | -78.01% |
| [Load/for_filter_if](#loadfor_filter_if) | 30,000 | 0% | -77.84% | 7 | 0% | -94.31% | 1,904 | 0% | -44.78% |
| [Render/for_filter_if](#renderfor_filter_if) | 274,669 | 0% | -68.26% | 4 | 0% | -98.23% | 16,560 | 0% | +354.95% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 40,302 | 0% | -77.69% | 10 | 0% | -93.63% | 2,368 | 0% | -47.61% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 194,105 | 0% | -81.91% | 2 | 0% | -99.62% | 1,568 | 0% | -43.35% |
| [Load/for_range](#loadfor_range) | 13,631 | 0% | -75.95% | 7 | 0% | -87.50% | 1,088 | 0% | -45.60% |
| [Render/for_range](#renderfor_range) | 38,346 | 0% | -58.41% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/hf_deepseek_v31](#loadhf_deepseek_v31) | 386,738 | 0% |  | 55 | 0% |  | 21,976 | 0% | +0.73% |
| [Render/hf_deepseek_v31](#renderhf_deepseek_v31) | 90,302 | 0% |  | 43 | 0% |  | 2,480 | 0% | 0% |
| [Load/hf_gpt_oss](#loadhf_gpt_oss) | 1,546,617 | 0% |  | 287 | 0% |  | 74,672 | 0% | +0.60% |
| [Render/hf_gpt_oss](#renderhf_gpt_oss) | 170,388 | 0% |  | 155 | 0% |  | 5,096 | 0% | +3.24% |
| [Load/hf_mistral_small](#loadhf_mistral_small) | 596,950 | 0% |  | 88 | 0% |  | 33,048 | 0% | +0.34% |
| [Render/hf_mistral_small](#renderhf_mistral_small) | 144,752 | 0% |  | 105 | 0% |  | 6,120 | 0% | -0.26% |
| [Load/hf_qwen3](#loadhf_qwen3) | 485,238 | 0% |  | 82 | 0% |  | 25,560 | 0% | +0.31% |
| [Render/hf_qwen3](#renderhf_qwen3) | 120,106 | 0% |  | 100 | 0% |  | 3,248 | 0% | 0% |
| [Load/html_autoescape](#loadhtml_autoescape) | 193,756 | 0% |  | 43 | 0% |  | 12,640 | 0% | -48.78% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,210,826 | 0% |  | 674 | 0% |  | 45,408 | 0% | +12.44% |
| [Load/inheritance](#loadinheritance) | 23,006 | +0.56% | -79.84% | 7 | 0% | -93.46% | 1,344 | +1.20% | -53.20% |
| [Render/inheritance](#renderinheritance) | 136,985 | -0.02% | -81.68% | 35 | 0% | -94.37% | 7,384 | 0% | +22.74% |
| [Load/int_arith](#loadint_arith) | 65,995 | 0% |  | 20 | 0% |  | 3,872 | 0% | +0.41% |
| [Render/int_arith](#renderint_arith) | 824,272 | 0% |  | 12 | 0% |  | 3,272 | 0% | -0.49% |
| [Module/keep](#modulekeep) | 6,960 | 0% |  | 7 | 0% |  |  |  |  |
| [Module/keep_written](#modulekeep_written) | 36,344 | 0% |  | 11 | 0% |  |  |  |  |
| [Load/large_static](#loadlarge_static) | 154,576 | 0% | -82.55% | 14 | 0% | -96.82% | 43,120 | 0% | -19.23% |
| [Render/large_static](#renderlarge_static) | 20,700 | 0% | -72.45% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [RenderSink/large_static](#rendersinklarge_static) | 12,686 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/large_static](#rendertolarge_static) | 20,263 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/macros](#loadmacros) | 56,253 | 0% | -75.32% | 27 | 0% | -85.48% | 4,392 | 0% | -29.34% |
| [Render/macros](#rendermacros) | 656,289 | 0% | -74.39% | 308 | 0% | -88.71% | 20,560 | 0% | +39.67% |
| [Load/many_tags](#loadmany_tags) | 9,012,602 | 0% | -79.59% | 951 | 0% | -97.22% | 438,144 | 0% | -58.48% |
| [Render/many_tags](#rendermany_tags) | 725,698 | 0% | -65.16% | 2 | 0% | -90.48% | 5,648 | 0% | -10.52% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 46,462 | 0% | -80.44% | 19 | 0% | -89.67% | 3,360 | 0% | -44.59% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,180,778 | 0% | -58.07% | 33 | 0% | -99.19% | 381,536 | 0% | +9.95% |
| [RenderSink/mitsuhiko_table](#rendersinkmitsuhiko_table) | 5,108,016 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table](#rendertomitsuhiko_table) | 5,180,276 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 54,700 | 0% |  | 35 | 0% |  | 5,688 | 0% | -31.17% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,526,780 | 0% |  | 33 | 0% |  | 1,414,928 | 0% | +2.46% |
| [RenderSink/mitsuhiko_table_wide](#rendersinkmitsuhiko_table_wide) | 5,350,087 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table_wide](#rendertomitsuhiko_table_wide) | 5,526,352 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/plain_text](#loadplain_text) | 3,714 | 0% | -61.62% | 4 | 0% | -77.78% | 672 | 0% | -40.00% |
| [Render/plain_text](#renderplain_text) | 1,017 | 0% | -85.03% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/plain_text](#rendersinkplain_text) | 717 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/plain_text](#rendertoplain_text) | 818 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/strings](#loadstrings) | 70,226 | 0% | -75.94% | 30 | 0% | -86.61% | 4,344 | 0% | -51.95% |
| [Render/strings](#renderstrings) | 582,109 | 0% | -72.25% | 344 | 0% | -83.73% | 9,088 | 0% | -49.31% |
| [Load/substitute](#loadsubstitute) | 7,743 | 0% | -74.07% | 6 | 0% | -83.78% | 832 | 0% | -45.26% |
| [Render/substitute](#rendersubstitute) | 2,233 | 0% | -74.66% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/substitute](#rendersinksubstitute) | 1,936 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/substitute](#rendertosubstitute) | 2,038 | 0% |  | 0 |  |  | 0 |  |  |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 735 | +1.10% | 731 | -0.41% | 0 |  | 7,624 | +3.42% |
| Render/chat_llama | 1,227 | +3.54% | 527 | +1.74% | 0 |  | 4,001 | -0.20% |
| Load/chat_mistral | 1,086 | -0.46% | 1,053 | -0.28% | 0 |  | 8,666 | +3.82% |
| Render/chat_mistral | 1,842 | +0.55% | 530 | 0% | 0 |  | 9,527 | -0.30% |
| Load/chat_qwen | 571 | +0.18% | 597 | -0.17% | 0 |  | 4,514 | +3.84% |
| Render/chat_qwen | 1,113 | +1.18% | 389 | -0.51% | 0 |  | 3,247 | -3.28% |
| Load/config_file | 193 | +2.66% | 168 | +4.35% | 0 |  | 2,626 | +1.51% |
| Render/config_file | 6,434 | +2.42% | 1,573 | -0.76% | 0 |  | 42,436 | +1.76% |
| Load/dict_ops | 26 | -13.33% | 17 | -5.56% | 0 |  | 1,328 | +3.19% |
| Render/dict_ops | 674 | +0.90% | 426 | -0.47% | 0 |  | 885 | 0% |
| Load/expressions | 31 | -13.89% | 26 | +8.33% | 0 |  | 1,286 | -1.30% |
| Render/expressions | 5 | +150.00% | 2 | +100.00% | 0 |  | 312 | +2.63% |
| Load/filters | 89 | -4.30% | 56 | +3.70% | 0 |  | 1,660 | +7.44% |
| Render/filters | 16 | -5.88% | 5 | 0% | 0 |  | 776 | +0.91% |
| Load/for_filter_if | 18 | -14.29% | 11 | 0% | 0 |  | 1,085 | +1.78% |
| Render/for_filter_if | 1,728 | -0.17% | 90 | -2.17% | 0 |  | 217 | +9.05% |
| Load/for_loop_vars | 50 | +16.28% | 27 | +3.85% | 0 |  | 1,082 | +1.69% |
| Render/for_loop_vars | 7 | -22.22% | 1 |  | 0 |  | 173 | +8.13% |
| Load/for_range | 6 | -62.50% | 5 | -37.50% | 0 |  | 741 | +0.41% |
| Render/for_range | 1 | 0% | 0 |  | 0 |  | 119 | +19.00% |
| Load/hf_deepseek_v31 | 998 | +1.22% | 973 | +0.41% | 0 |  | 6,339 | +3.94% |
| Render/hf_deepseek_v31 | 359 | +2.28% | 74 | -3.90% | 0 |  | 1,372 | -3.45% |
| Load/hf_gpt_oss | 4,608 | -0.15% | 3,287 | -0.30% | 0 |  | 22,431 | +6.46% |
| Render/hf_gpt_oss | 1,225 | +1.49% | 376 | +0.53% | 0 |  | 4,467 | -0.38% |
| Load/hf_mistral_small | 1,810 | +1.46% | 1,599 | +0.38% | 0 |  | 10,919 | +2.26% |
| Render/hf_mistral_small | 967 | +0.52% | 317 | +0.32% | 0 |  | 3,763 | +0.11% |
| Load/hf_qwen3 | 1,434 | -0.49% | 1,240 | -0.64% | 0 |  | 8,769 | +1.74% |
| Render/hf_qwen3 | 673 | +1.51% | 218 | +2.35% | 0 |  | 2,569 | -1.23% |
| Load/html_autoescape | 437 | +0.92% | 511 | -0.58% | 0 |  | 3,930 | +4.13% |
| Render/html_autoescape | 4,094 | +18.56% | 1,263 | +0.48% | 0 |  | 35,517 | -1.91% |
| Load/inheritance | 3 | 0% | 4 | 0% | 0 |  | 698 | +9.06% |
| Render/inheritance | 583 | -0.68% | 100 | +4.17% | 0 |  | 913 | -1.19% |
| Load/int_arith | 89 | +4.71% | 60 | 0% | 0 |  | 2,060 | -0.87% |
| Render/int_arith | 6 | -14.29% | 5 | +25.00% | 0 |  | 768 | +1.19% |
| Module/keep | 2 | 0% | 2 | 0% | 0 |  | 381 | -8.41% |
| Module/keep_written | 2,930 | +0.03% | 557 | 0% | 0 |  | 733 | -0.81% |
| Load/large_static | 1,935 | +0.05% | 910 | 0% | 0 |  | 296 | +1.37% |
| Render/large_static | 710 | 0% | 568 | 0% | 0 |  | 5 | -58.33% |
| RenderSink/large_static | 0 |  | 0 |  | 0 |  | 3 | -25.00% |
| RenderTo/large_static | 687 | 0% | 572 | 0% | 0 |  | 3 | -40.00% |
| Load/macros | 81 | +3.85% | 64 | +1.59% | 0 |  | 1,713 | +4.96% |
| Render/macros | 766 | -0.26% | 215 | -0.46% | 0 |  | 610 | -59.71% |
| Load/many_tags | 28,487 | +1.13% | 22,626 | +0.17% | 0 |  | 226,008 | +4.55% |
| Render/many_tags | 6,226 | -0.02% | 844 | 0% | 0 |  | 159 | +0.63% |
| Load/mitsuhiko_table | 27 | +12.50% | 25 | +4.17% | 0 |  | 842 | +7.40% |
| Render/mitsuhiko_table | 10,098 | +0.12% | 6,255 | -0.02% | 0 |  | 383 | +2.13% |
| RenderSink/mitsuhiko_table | 9,959 | -0.11% | 1,200 | -0.08% | 0 |  | 320 | +8.84% |
| RenderTo/mitsuhiko_table | 10,071 | +0.05% | 6,260 | 0% | 0 |  | 353 | -2.22% |
| Load/mitsuhiko_table_wide | 65 | +18.18% | 61 | +3.39% | 0 |  | 963 | +1.58% |
| Render/mitsuhiko_table_wide | 10,465 | -0.24% | 22,772 | -0.05% | 0 |  | 387 | +17.27% |
| RenderSink/mitsuhiko_table_wide | 10,116 | -0.15% | 1,302 | -0.08% | 0 |  | 345 | +23.21% |
| RenderTo/mitsuhiko_table_wide | 10,467 | -0.25% | 22,673 | -0.02% | 0 |  | 368 | +16.46% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 23 | +64.29% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 6 | -14.29% |
| RenderSink/plain_text | 0 |  | 0 |  | 0 |  | 5 | 0% |
| RenderTo/plain_text | 0 |  | 0 |  | 0 |  | 5 | -16.67% |
| Load/strings | 126 | +2.44% | 80 | -6.98% | 0 |  | 2,697 | +1.81% |
| Render/strings | 418 | +2.96% | 478 | 0% | 0 |  | 1,574 | -0.38% |
| Load/substitute | 2 | 0% | 3 | -25.00% | 0 |  | 161 | +21.97% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 3 | -25.00% |
| RenderSink/substitute | 0 |  | 0 |  | 0 |  | 2 | -50.00% |
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
