# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (167 so far, latest [28cdf08](https://github.com/jinja2cpp/Jinja2Cpp/commit/28cdf0817ba16c9c1d503cdb9e0e2a2510134287) on 2026-11-01). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 344,094 | +0.05% |  | 55 | 0% |  | 18,240 | 0% | -53.33% |
| [Render/chat_llama](#renderchat_llama) | 311,467 | +0.15% |  | 251 | 0% |  | 11,696 | -0.14% | -18.55% |
| [Load/chat_mistral](#loadchat_mistral) | 392,001 | +0.07% |  | 52 | 0% |  | 20,680 | 0% | -53.68% |
| [Render/chat_mistral](#renderchat_mistral) | 517,168 | +0.17% |  | 424 | 0% |  | 10,064 | 0% | -23.80% |
| [Load/chat_qwen](#loadchat_qwen) | 266,719 | +0.09% |  | 44 | 0% |  | 14,400 | 0% | -47.31% |
| [Render/chat_qwen](#renderchat_qwen) | 296,486 | +0.15% |  | 238 | 0% |  | 8,304 | 0% | -2.35% |
| [Load/config_file](#loadconfig_file) | 98,565 | +0.06% |  | 25 | 0% |  | 5,432 | 0% | -48.56% |
| [Render/config_file](#renderconfig_file) | 1,269,240 | +0.13% |  | 908 | 0% |  | 24,264 | +1.13% | +1.92% |
| [Load/dict_ops](#loaddict_ops) | 33,094 | +0.09% | -77.61% | 9 | 0% | -93.13% | 2,176 | 0% | -48.87% |
| [Render/dict_ops](#renderdict_ops) | 299,186 | +0.60% | -56.69% | 112 | 0% | -75.11% | 24,408 | 0% | -29.65% |
| [Load/expressions](#loadexpressions) | 45,255 | +0.01% | -78.05% | 16 | 0% | -89.04% | 2,728 | 0% | -55.54% |
| [Render/expressions](#renderexpressions) | 425,823 | 0% | -53.01% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 71,688 | 0% | -73.88% | 27 | 0% | -88.16% | 3,720 | 0% | -67.41% |
| [Render/filters](#renderfilters) | 30,421 | -0.03% | -65.69% | 26 | 0% | -64.86% | 1,080 | 0% | -78.01% |
| [Load/for_filter_if](#loadfor_filter_if) | 29,862 | +0.06% | -77.94% | 7 | 0% | -94.31% | 1,904 | 0% | -44.78% |
| [Render/for_filter_if](#renderfor_filter_if) | 274,477 | +0.01% | -68.29% | 4 | 0% | -98.23% | 16,560 | 0% | +354.95% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 40,210 | +0.10% | -77.74% | 10 | 0% | -93.63% | 2,368 | 0% | -47.61% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 192,678 | -0.01% | -82.04% | 2 | 0% | -99.62% | 1,568 | 0% | -43.35% |
| [Load/for_range](#loadfor_range) | 13,567 | +0.03% | -76.07% | 7 | 0% | -87.50% | 1,088 | 0% | -45.60% |
| [Render/for_range](#renderfor_range) | 38,796 | +0.01% | -57.93% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/html_autoescape](#loadhtml_autoescape) | 193,976 | +0.09% |  | 43 | 0% |  | 12,592 | 0% | -48.98% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,223,820 | +0.11% |  | 674 | 0% |  | 45,296 | +0.28% | +12.16% |
| [Load/inheritance](#loadinheritance) | 22,740 | +0.02% | -80.07% | 7 | 0% | -93.46% | 1,328 | 0% | -53.76% |
| [Render/inheritance](#renderinheritance) | 136,639 | -0.04% | -81.73% | 35 | 0% | -94.37% | 7,352 | +0.44% | +22.21% |
| [Load/int_arith](#loadint_arith) | 65,827 | +0.08% |  | 20 | 0% |  | 3,840 | 0% | -0.41% |
| [Render/int_arith](#renderint_arith) | 825,468 | -0.20% |  | 12 | 0% |  | 3,272 | 0% | -0.49% |
| [Module/keep](#modulekeep) | 6,893 | +0.15% |  | 7 | 0% |  |  |  |  |
| [Module/keep_written](#modulekeep_written) | 36,255 | -0.06% |  | 11 | 0% |  |  |  |  |
| [Load/large_static](#loadlarge_static) | 154,888 | 0% | -82.52% | 14 | 0% | -96.82% | 43,120 | 0% | -19.23% |
| [Render/large_static](#renderlarge_static) | 20,874 | -0.07% | -72.22% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [RenderSink/large_static](#rendersinklarge_static) | 12,676 | -0.11% |  | 0 |  |  | 0 |  |  |
| [RenderTo/large_static](#rendertolarge_static) | 20,253 | -0.07% |  | 0 |  |  | 0 |  |  |
| [Load/macros](#loadmacros) | 55,774 | +0.07% | -75.53% | 27 | 0% | -85.48% | 4,344 | 0% | -30.12% |
| [Render/macros](#rendermacros) | 652,277 | +0.32% | -74.55% | 308 | 0% | -88.71% | 20,528 | +0.16% | +39.46% |
| [Load/many_tags](#loadmany_tags) | 9,059,871 | +0.08% | -79.49% | 951 | 0% | -97.22% | 431,120 | 0% | -59.14% |
| [Render/many_tags](#rendermany_tags) | 725,492 | +0.08% | -65.17% | 2 | 0% | -90.48% | 5,648 | 0% | -10.52% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 46,451 | +0.03% | -80.45% | 19 | 0% | -89.67% | 3,360 | 0% | -44.59% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,222,409 | -0.04% | -57.73% | 33 | 0% | -99.19% | 381,536 | 0% | +9.95% |
| [RenderSink/mitsuhiko_table](#rendersinkmitsuhiko_table) | 5,149,546 | -0.04% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table](#rendertomitsuhiko_table) | 5,221,815 | -0.04% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 55,027 | +0.03% |  | 35 | 0% |  | 5,688 | 0% | -31.17% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,568,181 | -0.04% |  | 33 | 0% |  | 1,414,928 | 0% | +2.46% |
| [RenderSink/mitsuhiko_table_wide](#rendersinkmitsuhiko_table_wide) | 5,391,484 | -0.04% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table_wide](#rendertomitsuhiko_table_wide) | 5,567,746 | -0.04% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/plain_text](#loadplain_text) | 3,692 | 0% | -61.84% | 4 | 0% | -77.78% | 672 | 0% | -40.00% |
| [Render/plain_text](#renderplain_text) | 1,007 | -1.37% | -85.18% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/plain_text](#rendersinkplain_text) | 707 | -1.94% |  | 0 |  |  | 0 |  |  |
| [RenderTo/plain_text](#rendertoplain_text) | 808 | -1.70% |  | 0 |  |  | 0 |  |  |
| [Load/strings](#loadstrings) | 71,826 | +0.07% | -75.39% | 30 | 0% | -86.61% | 4,248 | 0% | -53.01% |
| [Render/strings](#renderstrings) | 583,188 | +0.05% | -72.20% | 344 | 0% | -83.73% | 9,088 | 0% | -49.31% |
| [Load/substitute](#loadsubstitute) | 7,709 | 0% | -74.18% | 6 | 0% | -83.78% | 832 | 0% | -45.26% |
| [Render/substitute](#rendersubstitute) | 2,223 | -0.63% | -74.78% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/substitute](#rendersinksubstitute) | 1,926 | -0.72% |  | 0 |  |  | 0 |  |  |
| [RenderTo/substitute](#rendertosubstitute) | 2,028 | -0.69% |  | 0 |  |  | 0 |  |  |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 720 | -2.96% | 729 | -0.27% | 0 |  | 7,269 | -3.03% |
| Render/chat_llama | 1,160 | +2.93% | 466 | +1.30% | 0 |  | 3,811 | -2.21% |
| Load/chat_mistral | 1,091 | +1.58% | 1,015 | +1.81% | 0 |  | 8,120 | -0.84% |
| Render/chat_mistral | 1,941 | +2.59% | 550 | +2.42% | 0 |  | 9,284 | +1.01% |
| Load/chat_qwen | 496 | -1.00% | 611 | +1.66% | 0 |  | 4,407 | -0.59% |
| Render/chat_qwen | 1,208 | +5.50% | 399 | -0.25% | 0 |  | 3,286 | +2.37% |
| Load/config_file | 195 | +5.41% | 169 | +1.81% | 0 |  | 2,942 | -3.60% |
| Render/config_file | 6,035 | -2.38% | 1,487 | -0.34% | 0 |  | 39,679 | -2.20% |
| Load/dict_ops | 37 | +19.35% | 19 | +35.71% | 0 |  | 1,279 | -2.22% |
| Render/dict_ops | 759 | +1.34% | 410 | -2.61% | 0 |  | 838 | -4.23% |
| Load/expressions | 52 | +18.18% | 32 | +23.08% | 0 |  | 1,318 | -2.95% |
| Render/expressions | 9 | +125.00% | 3 | +50.00% | 0 |  | 312 | 0% |
| Load/filters | 137 | +7.87% | 65 | +1.56% | 0 |  | 2,069 | -14.18% |
| Render/filters | 12 | -29.41% | 2 | -33.33% | 0 |  | 730 | -2.54% |
| Load/for_filter_if | 18 | -25.00% | 10 | +11.11% | 0 |  | 1,021 | -4.31% |
| Render/for_filter_if | 1,743 | +0.06% | 95 | 0% | 0 |  | 148 | -8.64% |
| Load/for_loop_vars | 33 | +37.50% | 14 | +7.69% | 0 |  | 1,088 | -0.55% |
| Render/for_loop_vars | 21 | +61.54% | 3 | +200.00% | 0 |  | 105 | -37.87% |
| Load/for_range | 8 | +60.00% | 6 | +20.00% | 0 |  | 743 | +0.68% |
| Render/for_range | 1 | 0% | 1 | 0% | 0 |  | 119 | -6.30% |
| Load/html_autoescape | 453 | +1.57% | 513 | -1.16% | 0 |  | 3,996 | -2.49% |
| Render/html_autoescape | 4,004 | -16.79% | 1,060 | -35.05% | 0 |  | 34,627 | -4.00% |
| Load/inheritance | 14 | +55.56% | 3 | 0% | 0 |  | 655 | +1.71% |
| Render/inheritance | 606 | -1.30% | 105 | 0% | 0 |  | 876 | -1.13% |
| Load/int_arith | 95 | -11.21% | 84 | -4.55% | 0 |  | 1,955 | -3.88% |
| Render/int_arith | 16 | 0% | 3 | -25.00% | 0 |  | 699 | -5.54% |
| Module/keep | 1 | -50.00% | 1 | 0% | 0 |  | 337 | -11.78% |
| Module/keep_written | 2,930 | -0.07% | 559 | +0.36% | 0 |  | 662 | -8.44% |
| Load/large_static | 1,948 | -0.41% | 912 | 0% | 0 |  | 244 | -11.27% |
| Render/large_static | 709 | 0% | 570 | 0% | 0 |  | 5 | +66.67% |
| RenderSink/large_static | 1 |  | 0 |  | 0 |  | 2 | 0% |
| RenderTo/large_static | 687 | 0% | 570 | 0% | 0 |  | 3 | 0% |
| Load/macros | 69 | -13.75% | 67 | -11.84% | 0 |  | 1,652 | 0% |
| Render/macros | 764 | -1.04% | 260 | +1.17% | 0 |  | 1,491 | +146.04% |
| Load/many_tags | 29,315 | +0.78% | 22,384 | +0.28% | 0 |  | 257,031 | -3.71% |
| Render/many_tags | 6,132 | +0.21% | 861 | -0.69% | 0 |  | 163 | +1.24% |
| Load/mitsuhiko_table | 29 | -14.71% | 51 | 0% | 0 |  | 790 | -7.17% |
| Render/mitsuhiko_table | 10,087 | +0.05% | 6,377 | +0.03% | 0 |  | 337 | -7.92% |
| RenderSink/mitsuhiko_table | 9,962 | +0.03% | 1,231 | +0.41% | 0 |  | 274 | -1.79% |
| RenderTo/mitsuhiko_table | 10,039 | +0.12% | 6,307 | -0.05% | 0 |  | 304 | -10.32% |
| Load/mitsuhiko_table_wide | 71 | -10.13% | 83 | -10.75% | 0 |  | 996 | +3.75% |
| Render/mitsuhiko_table_wide | 10,225 | -0.06% | 22,691 | -0.03% | 0 |  | 325 | -7.41% |
| RenderSink/mitsuhiko_table_wide | 10,045 | 0% | 1,298 | -0.61% | 0 |  | 273 | +1.87% |
| RenderTo/mitsuhiko_table_wide | 10,238 | -0.06% | 22,688 | +0.01% | 0 |  | 309 | -3.74% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 15 | -51.61% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 5 | -28.57% |
| RenderSink/plain_text | 0 |  | 0 |  | 0 |  | 5 | 0% |
| RenderTo/plain_text | 0 |  | 0 |  | 0 |  | 4 | -33.33% |
| Load/strings | 154 | +1.32% | 90 | +7.14% | 0 |  | 2,786 | -2.45% |
| Render/strings | 442 | -1.78% | 467 | -1.06% | 0 |  | 1,509 | +0.73% |
| Load/substitute | 2 | 0% | 3 | 0% | 0 |  | 165 | -1.20% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 3 | +50.00% |
| RenderSink/substitute | 0 |  | 0 |  | 0 |  | 2 | 0% |
| RenderTo/substitute | 0 |  | 0 |  | 0 |  | 2 | 0% |

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
