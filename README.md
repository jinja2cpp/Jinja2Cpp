# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (173 so far, latest [a47fafd](https://github.com/jinja2cpp/Jinja2Cpp/commit/a47fafd11b210c2c08847184a425bf7232d4d346) on 2026-11-03). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 343,316 | +0.04% |  | 55 | 0% |  | 18,464 | 0% | -52.75% |
| [Render/chat_llama](#renderchat_llama) | 295,349 | +0.05% |  | 225 | 0% |  | 11,616 | 0% | -19.11% |
| [Load/chat_mistral](#loadchat_mistral) | 392,536 | +0.04% |  | 52 | 0% |  | 20,888 | 0% | -53.22% |
| [Render/chat_mistral](#renderchat_mistral) | 498,488 | +0.01% |  | 402 | 0% |  | 10,016 | 0% | -24.17% |
| [Load/chat_qwen](#loadchat_qwen) | 265,059 | +0.03% |  | 44 | 0% |  | 14,416 | 0% | -47.25% |
| [Render/chat_qwen](#renderchat_qwen) | 277,051 | +0.02% |  | 212 | 0% |  | 8,304 | 0% | -2.35% |
| [Load/config_file](#loadconfig_file) | 96,619 | +0.04% |  | 25 | 0% |  | 5,464 | 0% | -48.26% |
| [Render/config_file](#renderconfig_file) | 1,265,678 | +0.21% |  | 908 | 0% |  | 24,440 | 0% | +2.65% |
| [Load/dict_ops](#loaddict_ops) | 33,175 | +0.04% | -77.55% | 9 | 0% | -93.13% | 2,176 | 0% | -48.87% |
| [Render/dict_ops](#renderdict_ops) | 293,389 | +0.01% | -57.53% | 112 | 0% | -75.11% | 24,408 | 0% | -29.65% |
| [Load/expressions](#loadexpressions) | 44,900 | +0.01% | -78.22% | 16 | 0% | -89.04% | 2,728 | 0% | -55.54% |
| [Render/expressions](#renderexpressions) | 427,169 | 0% | -52.86% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 68,902 | 0% | -74.89% | 27 | 0% | -88.16% | 3,848 | 0% | -66.29% |
| [Render/filters](#renderfilters) | 30,243 | +0.02% | -65.89% | 26 | 0% | -64.86% | 1,080 | 0% | -78.01% |
| [Load/for_filter_if](#loadfor_filter_if) | 29,865 | +0.05% | -77.94% | 7 | 0% | -94.31% | 1,904 | 0% | -44.78% |
| [Render/for_filter_if](#renderfor_filter_if) | 275,089 | +0.01% | -68.22% | 4 | 0% | -98.23% | 16,560 | 0% | +354.95% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 40,096 | +0.04% | -77.81% | 10 | 0% | -93.63% | 2,368 | 0% | -47.61% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 192,299 | -0.10% | -82.07% | 2 | 0% | -99.62% | 1,568 | 0% | -43.35% |
| [Load/for_range](#loadfor_range) | 13,576 | +0.04% | -76.05% | 7 | 0% | -87.50% | 1,088 | 0% | -45.60% |
| [Render/for_range](#renderfor_range) | 38,821 | +0.04% | -57.90% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/hf_deepseek_v31](#loadhf_deepseek_v31) | 386,014 | 0% |  | 55 | 0% |  | 21,976 | 0% | +0.73% |
| [Render/hf_deepseek_v31](#renderhf_deepseek_v31) | 90,318 | +0.06% |  | 43 | 0% |  | 2,480 | 0% | 0% |
| [Load/hf_gpt_oss](#loadhf_gpt_oss) | 1,532,559 | +0.02% |  | 285 | 0% |  | 74,384 | 0% | +0.22% |
| [Render/hf_gpt_oss](#renderhf_gpt_oss) | 170,270 | +0.08% |  | 155 | 0% |  | 5,064 | 0% | +2.59% |
| [Load/hf_mistral_small](#loadhf_mistral_small) | 595,700 | +0.03% |  | 88 | 0% |  | 33,048 | 0% | +0.34% |
| [Render/hf_mistral_small](#renderhf_mistral_small) | 144,581 | +0.01% |  | 105 | 0% |  | 6,120 | 0% | -0.26% |
| [Load/hf_qwen3](#loadhf_qwen3) | 483,556 | +0.04% |  | 82 | 0% |  | 25,560 | 0% | +0.31% |
| [Render/hf_qwen3](#renderhf_qwen3) | 120,030 | +0.04% |  | 100 | 0% |  | 3,248 | 0% | 0% |
| [Load/html_autoescape](#loadhtml_autoescape) | 192,875 | +0.02% |  | 43 | 0% |  | 12,640 | 0% | -48.78% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,211,254 | +0.07% |  | 674 | 0% |  | 45,408 | 0% | +12.44% |
| [Load/inheritance](#loadinheritance) | 22,763 | +0.11% | -80.05% | 7 | 0% | -93.46% | 1,328 | 0% | -53.76% |
| [Render/inheritance](#renderinheritance) | 137,619 | +0.35% | -81.60% | 35 | 0% | -94.37% | 7,384 | 0% | +22.74% |
| [Load/int_arith](#loadint_arith) | 65,838 | +0.02% |  | 20 | 0% |  | 3,872 | 0% | +0.41% |
| [Render/int_arith](#renderint_arith) | 824,991 | 0% |  | 12 | 0% |  | 3,272 | 0% | -0.49% |
| [Module/keep](#modulekeep) | 6,965 | +0.20% |  | 7 | 0% |  |  |  |  |
| [Module/keep_written](#modulekeep_written) | 36,318 | +0.04% |  | 11 | 0% |  |  |  |  |
| [Load/large_static](#loadlarge_static) | 154,108 | 0% | -82.61% | 14 | 0% | -96.82% | 43,120 | 0% | -19.23% |
| [Render/large_static](#renderlarge_static) | 20,700 | +0.02% | -72.45% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [RenderSink/large_static](#rendersinklarge_static) | 12,686 | +0.04% |  | 0 |  |  | 0 |  |  |
| [RenderTo/large_static](#rendertolarge_static) | 20,263 | +0.02% |  | 0 |  |  | 0 |  |  |
| [Load/macros](#loadmacros) | 55,681 | +0.02% | -75.57% | 27 | 0% | -85.48% | 4,344 | 0% | -30.12% |
| [Render/macros](#rendermacros) | 656,763 | +0.28% | -74.38% | 308 | 0% | -88.71% | 20,560 | 0% | +39.67% |
| [Load/many_tags](#loadmany_tags) | 8,970,707 | +0.04% | -79.69% | 951 | 0% | -97.22% | 438,176 | 0% | -58.48% |
| [Render/many_tags](#rendermany_tags) | 725,698 | 0% | -65.16% | 2 | 0% | -90.48% | 5,648 | 0% | -10.52% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 46,300 | +0.04% | -80.51% | 19 | 0% | -89.67% | 3,360 | 0% | -44.59% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,235,831 | +0.19% | -57.63% | 33 | 0% | -99.19% | 381,536 | 0% | +9.95% |
| [RenderSink/mitsuhiko_table](#rendersinkmitsuhiko_table) | 5,163,000 | +0.19% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table](#rendertomitsuhiko_table) | 5,235,314 | +0.19% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 54,520 | 0% |  | 35 | 0% |  | 5,688 | 0% | -31.17% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,581,776 | +0.18% |  | 33 | 0% |  | 1,414,928 | 0% | +2.46% |
| [RenderSink/mitsuhiko_table_wide](#rendersinkmitsuhiko_table_wide) | 5,405,063 | +0.19% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table_wide](#rendertomitsuhiko_table_wide) | 5,581,347 | +0.18% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/plain_text](#loadplain_text) | 3,692 | 0% | -61.84% | 4 | 0% | -77.78% | 672 | 0% | -40.00% |
| [Render/plain_text](#renderplain_text) | 1,017 | +0.49% | -85.03% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/plain_text](#rendersinkplain_text) | 717 | +0.70% |  | 0 |  |  | 0 |  |  |
| [RenderTo/plain_text](#rendertoplain_text) | 818 | +0.62% |  | 0 |  |  | 0 |  |  |
| [Load/strings](#loadstrings) | 70,051 | +0.03% | -76.00% | 30 | 0% | -86.61% | 4,344 | 0% | -51.95% |
| [Render/strings](#renderstrings) | 582,647 | -0.01% | -72.22% | 344 | 0% | -83.73% | 9,088 | 0% | -49.31% |
| [Load/substitute](#loadsubstitute) | 7,709 | 0% | -74.18% | 6 | 0% | -83.78% | 832 | 0% | -45.26% |
| [Render/substitute](#rendersubstitute) | 2,233 | +0.22% | -74.66% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/substitute](#rendersinksubstitute) | 1,936 | +0.26% |  | 0 |  |  | 0 |  |  |
| [RenderTo/substitute](#rendertosubstitute) | 2,038 | +0.25% |  | 0 |  |  | 0 |  |  |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 726 | 0% | 732 | 0% | 0 |  | 7,319 | +0.14% |
| Render/chat_llama | 1,174 | +0.26% | 509 | +0.39% | 0 |  | 3,635 | -4.79% |
| Load/chat_mistral | 1,090 | 0% | 1,050 | 0% | 0 |  | 8,236 | -2.50% |
| Render/chat_mistral | 1,918 | +0.74% | 533 | -2.20% | 0 |  | 9,753 | +1.37% |
| Load/chat_qwen | 487 | 0% | 585 | 0% | 0 |  | 4,357 | -1.49% |
| Render/chat_qwen | 1,143 | +0.97% | 403 | -1.23% | 0 |  | 3,209 | +2.72% |
| Load/config_file | 176 | 0% | 152 | 0% | 0 |  | 2,661 | -3.52% |
| Render/config_file | 6,484 | +0.86% | 1,716 | +3.44% | 0 |  | 41,679 | +3.58% |
| Load/dict_ops | 34 | 0% | 17 | 0% | 0 |  | 1,300 | -3.42% |
| Render/dict_ops | 662 | +0.15% | 425 | 0% | 0 |  | 873 | +2.59% |
| Load/expressions | 41 | 0% | 24 | 0% | 0 |  | 1,307 | +0.62% |
| Render/expressions | 2 | 0% | 1 |  | 0 |  | 311 | +9.51% |
| Load/filters | 87 | 0% | 51 | 0% | 0 |  | 1,683 | +7.95% |
| Render/filters | 15 | 0% | 2 | 0% | 0 |  | 766 | -1.79% |
| Load/for_filter_if | 16 | 0% | 10 | 0% | 0 |  | 1,053 | -5.05% |
| Render/for_filter_if | 1,735 | -0.06% | 90 | +1.12% | 0 |  | 174 | -13.43% |
| Load/for_loop_vars | 41 | 0% | 25 | 0% | 0 |  | 1,073 | -6.94% |
| Render/for_loop_vars | 9 | 0% | 1 | 0% | 0 |  | 152 | -5.00% |
| Load/for_range | 8 | 0% | 7 | 0% | 0 |  | 707 | -4.46% |
| Render/for_range | 1 | 0% | 0 |  | 0 |  | 141 | +35.58% |
| Load/hf_deepseek_v31 | 951 | 0% | 963 | 0% | 0 |  | 5,763 | -4.84% |
| Render/hf_deepseek_v31 | 372 | +0.81% | 85 | +3.66% | 0 |  | 1,359 | +5.27% |
| Load/hf_gpt_oss | 4,526 | +0.07% | 3,248 | 0% | 0 |  | 20,141 | -5.26% |
| Render/hf_gpt_oss | 1,211 | +1.00% | 380 | -0.52% | 0 |  | 4,525 | +2.14% |
| Load/hf_mistral_small | 1,739 | 0% | 1,566 | 0% | 0 |  | 10,558 | -4.10% |
| Render/hf_mistral_small | 946 | +0.53% | 275 | 0% | 0 |  | 3,799 | +3.15% |
| Load/hf_qwen3 | 1,402 | 0% | 1,243 | 0% | 0 |  | 8,499 | -1.60% |
| Render/hf_qwen3 | 702 | +0.86% | 193 | +1.58% | 0 |  | 2,548 | +1.43% |
| Load/html_autoescape | 432 | 0% | 513 | 0% | 0 |  | 3,764 | -7.34% |
| Render/html_autoescape | 4,351 | +2.47% | 1,513 | +1.48% | 0 |  | 35,700 | +3.16% |
| Load/inheritance | 3 | 0% | 4 | 0% | 0 |  | 623 | -7.70% |
| Render/inheritance | 584 | +0.34% | 91 | -1.09% | 0 |  | 890 | +2.77% |
| Load/int_arith | 81 | 0% | 61 | 0% | 0 |  | 2,044 | -2.43% |
| Render/int_arith | 3 | 0% | 1 | 0% | 0 |  | 740 | -2.76% |
| Module/keep | 1 | 0% | 2 | 0% | 0 |  | 368 | -7.77% |
| Module/keep_written | 2,928 | 0% | 558 | +0.18% | 0 |  | 715 | +1.85% |
| Load/large_static | 1,928 | 0% | 913 | 0% | 0 |  | 291 | +2.83% |
| Render/large_static | 710 | +0.14% | 568 | 0% | 0 |  | 13 | +225.00% |
| RenderSink/large_static | 1 |  | 0 |  | 0 |  | 2 | 0% |
| RenderTo/large_static | 687 | +0.15% | 572 | 0% | 0 |  | 4 | 0% |
| Load/macros | 70 | 0% | 51 | 0% | 0 |  | 1,636 | -3.31% |
| Render/macros | 761 | +0.13% | 217 | +0.46% | 0 |  | 1,703 | +138.52% |
| Load/many_tags | 28,750 | 0% | 22,777 | 0% | 0 |  | 224,601 | -6.03% |
| Render/many_tags | 6,232 | 0% | 855 | 0% | 0 |  | 147 | +19.51% |
| Load/mitsuhiko_table | 19 | 0% | 18 | 0% | 0 |  | 813 | -3.33% |
| Render/mitsuhiko_table | 10,102 | -0.02% | 6,262 | -0.10% | 0 |  | 340 | -2.86% |
| RenderSink/mitsuhiko_table | 9,970 | 0% | 1,204 | +0.08% | 0 |  | 275 | -0.36% |
| RenderTo/mitsuhiko_table | 10,071 | -0.01% | 6,265 | -0.05% | 0 |  | 328 | -1.20% |
| Load/mitsuhiko_table_wide | 47 | 0% | 53 | 0% | 0 |  | 905 | -4.94% |
| Render/mitsuhiko_table_wide | 10,785 | 0% | 22,724 | -0.02% | 0 |  | 325 | -10.71% |
| RenderSink/mitsuhiko_table_wide | 10,152 | 0% | 1,314 | -0.15% | 0 |  | 269 | -7.24% |
| RenderTo/mitsuhiko_table_wide | 10,778 | -0.02% | 22,731 | -0.02% | 0 |  | 305 | -6.73% |
| Load/plain_text | 1 | 0% | 2 | 0% | 0 |  | 8 | -42.86% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 6 | +50.00% |
| RenderSink/plain_text | 0 |  | 0 |  | 0 |  | 3 | 0% |
| RenderTo/plain_text | 0 |  | 0 |  | 0 |  | 6 | +50.00% |
| Load/strings | 126 | 0% | 77 | 0% | 0 |  | 2,723 | +0.48% |
| Render/strings | 405 | 0% | 457 | -0.22% | 0 |  | 1,548 | +1.78% |
| Load/substitute | 2 | 0% | 3 | 0% | 0 |  | 197 | +48.12% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 3 | -25.00% |
| RenderSink/substitute | 0 |  | 0 |  | 0 |  | 2 | -33.33% |
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
