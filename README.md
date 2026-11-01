# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (166 so far, latest [47fea4d](https://github.com/jinja2cpp/Jinja2Cpp/commit/47fea4d13c497b8be58aab3bdef429548e79996b) on 2026-11-01). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 343,922 | -0.12% |  | 55 | 0% |  | 18,240 | 0% | -53.33% |
| [Render/chat_llama](#renderchat_llama) | 311,002 | +0.19% |  | 251 | 0% |  | 11,712 | 0% | -18.44% |
| [Load/chat_mistral](#loadchat_mistral) | 391,734 | -0.05% |  | 52 | 0% |  | 20,680 | 0% | -53.68% |
| [Render/chat_mistral](#renderchat_mistral) | 516,266 | +0.16% |  | 424 | 0% |  | 10,064 | 0% | -23.80% |
| [Load/chat_qwen](#loadchat_qwen) | 266,470 | -0.07% |  | 44 | 0% |  | 14,400 | 0% | -47.31% |
| [Render/chat_qwen](#renderchat_qwen) | 296,039 | +0.26% |  | 238 | 0% |  | 8,304 | 0% | -2.35% |
| [Load/config_file](#loadconfig_file) | 98,506 | -0.08% |  | 25 | 0% |  | 5,432 | 0% | -48.56% |
| [Render/config_file](#renderconfig_file) | 1,267,639 | +0.01% |  | 908 | 0% |  | 23,992 | 0% | +0.77% |
| [Load/dict_ops](#loaddict_ops) | 33,065 | -0.09% | -77.62% | 9 | 0% | -93.13% | 2,176 | 0% | -48.87% |
| [Render/dict_ops](#renderdict_ops) | 297,401 | 0% | -56.95% | 112 | 0% | -75.11% | 24,408 | 0% | -29.65% |
| [Load/expressions](#loadexpressions) | 45,251 | -0.11% | -78.05% | 16 | 0% | -89.04% | 2,728 | 0% | -55.54% |
| [Render/expressions](#renderexpressions) | 425,818 | +0.27% | -53.01% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 71,688 | -0.15% | -73.88% | 27 | 0% | -88.16% | 3,720 | 0% | -67.41% |
| [Render/filters](#renderfilters) | 30,431 | 0% | -65.68% | 26 | 0% | -64.86% | 1,080 | 0% | -78.01% |
| [Load/for_filter_if](#loadfor_filter_if) | 29,843 | -0.03% | -77.96% | 7 | 0% | -94.31% | 1,904 | 0% | -44.78% |
| [Render/for_filter_if](#renderfor_filter_if) | 274,436 | +0.05% | -68.29% | 4 | 0% | -98.23% | 16,560 | 0% | +354.95% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 40,171 | -0.10% | -77.76% | 10 | 0% | -93.63% | 2,368 | 0% | -47.61% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 192,697 | 0% | -82.04% | 2 | 0% | -99.62% | 1,568 | 0% | -43.35% |
| [Load/for_range](#loadfor_range) | 13,563 | -0.07% | -76.07% | 7 | 0% | -87.50% | 1,088 | 0% | -45.60% |
| [Render/for_range](#renderfor_range) | 38,791 | 0% | -57.93% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/html_autoescape](#loadhtml_autoescape) | 193,796 | -0.08% |  | 43 | 0% |  | 12,592 | 0% | -48.98% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,222,435 | +0.05% |  | 674 | 0% |  | 45,168 | 0% | +11.85% |
| [Load/inheritance](#loadinheritance) | 22,736 | -0.04% | -80.08% | 7 | 0% | -93.46% | 1,328 | 0% | -53.76% |
| [Render/inheritance](#renderinheritance) | 136,690 | 0% | -81.72% | 35 | 0% | -94.37% | 7,320 | 0% | +21.68% |
| [Load/int_arith](#loadint_arith) | 65,774 | -0.09% |  | 20 | 0% |  | 3,840 | 0% | -0.41% |
| [Render/int_arith](#renderint_arith) | 827,159 | +0.22% |  | 12 | 0% |  | 3,272 | 0% | -0.49% |
| [Module/keep](#modulekeep) | 6,883 | 0% |  | 7 | 0% |  |  |  |  |
| [Module/keep_written](#modulekeep_written) | 36,278 | 0% |  | 11 | 0% |  |  |  |  |
| [Load/large_static](#loadlarge_static) | 154,888 | -0.10% | -82.52% | 14 | 0% | -96.82% | 43,120 | 0% | -19.23% |
| [Render/large_static](#renderlarge_static) | 20,888 | 0% | -72.20% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [RenderSink/large_static](#rendersinklarge_static) | 12,690 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/large_static](#rendertolarge_static) | 20,267 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/macros](#loadmacros) | 55,735 | +0.03% | -75.55% | 27 | 0% | -85.48% | 4,344 | 0% | -30.12% |
| [Render/macros](#rendermacros) | 650,206 | 0% | -74.63% | 308 | 0% | -88.71% | 20,496 | 0% | +39.24% |
| [Load/many_tags](#loadmany_tags) | 9,052,971 | +0.11% | -79.50% | 951 | 0% | -97.22% | 431,120 | 0% | -59.14% |
| [Render/many_tags](#rendermany_tags) | 724,906 | 0% | -65.20% | 2 | 0% | -90.48% | 5,648 | 0% | -10.52% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 46,435 | -0.12% | -80.45% | 19 | 0% | -89.67% | 3,360 | 0% | -44.59% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,224,452 | 0% | -57.72% | 33 | 0% | -99.19% | 381,536 | 0% | +9.95% |
| [RenderSink/mitsuhiko_table](#rendersinkmitsuhiko_table) | 5,151,753 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table](#rendertomitsuhiko_table) | 5,224,022 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 55,011 | -0.10% |  | 35 | 0% |  | 5,688 | 0% | -31.17% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,570,170 | 0% |  | 33 | 0% |  | 1,414,928 | 0% | +2.46% |
| [RenderSink/mitsuhiko_table_wide](#rendersinkmitsuhiko_table_wide) | 5,393,472 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table_wide](#rendertomitsuhiko_table_wide) | 5,569,734 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/plain_text](#loadplain_text) | 3,692 | 0% | -61.84% | 4 | 0% | -77.78% | 672 | 0% | -40.00% |
| [Render/plain_text](#renderplain_text) | 1,021 | 0% | -84.97% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/plain_text](#rendersinkplain_text) | 721 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/plain_text](#rendertoplain_text) | 822 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/strings](#loadstrings) | 71,775 | -0.01% | -75.41% | 30 | 0% | -86.61% | 4,248 | 0% | -53.01% |
| [Render/strings](#renderstrings) | 582,891 | +0.13% | -72.21% | 344 | 0% | -83.73% | 9,088 | 0% | -49.31% |
| [Load/substitute](#loadsubstitute) | 7,709 | -0.08% | -74.18% | 6 | 0% | -83.78% | 832 | 0% | -45.26% |
| [Render/substitute](#rendersubstitute) | 2,237 | 0% | -74.62% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/substitute](#rendersinksubstitute) | 1,940 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/substitute](#rendertosubstitute) | 2,042 | 0% |  | 0 |  |  | 0 |  |  |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 742 | +0.54% | 731 | +0.14% | 0 |  | 7,496 | 0% |
| Render/chat_llama | 1,127 | -0.09% | 460 | +0.44% | 0 |  | 3,897 | +1.19% |
| Load/chat_mistral | 1,074 | +0.19% | 997 | +1.42% | 0 |  | 8,189 | +0.02% |
| Render/chat_mistral | 1,892 | -0.84% | 537 | -1.65% | 0 |  | 9,191 | -2.28% |
| Load/chat_qwen | 501 | +1.21% | 601 | +1.01% | 0 |  | 4,433 | +2.45% |
| Render/chat_qwen | 1,145 | -1.12% | 400 | +0.25% | 0 |  | 3,210 | +1.94% |
| Load/config_file | 185 | +1.65% | 166 | +0.61% | 0 |  | 3,052 | +1.36% |
| Render/config_file | 6,182 | -0.32% | 1,492 | +0.47% | 0 |  | 40,573 | +4.38% |
| Load/dict_ops | 31 | -6.06% | 14 | -6.67% | 0 |  | 1,308 | -2.10% |
| Render/dict_ops | 749 | 0% | 421 | 0% | 0 |  | 875 | +2.10% |
| Load/expressions | 44 | +12.82% | 26 | +44.44% | 0 |  | 1,358 | +2.41% |
| Render/expressions | 4 | -20.00% | 2 | 0% | 0 |  | 312 | +4.35% |
| Load/filters | 127 | -0.78% | 64 | -3.03% | 0 |  | 2,411 | +9.74% |
| Render/filters | 17 | +6.25% | 3 | 0% | 0 |  | 749 | -2.60% |
| Load/for_filter_if | 24 | +20.00% | 9 | +12.50% | 0 |  | 1,067 | +4.30% |
| Render/for_filter_if | 1,742 | 0% | 95 | 0% | 0 |  | 162 | -31.36% |
| Load/for_loop_vars | 24 | +4.35% | 13 | +8.33% | 0 |  | 1,094 | +3.80% |
| Render/for_loop_vars | 13 | +44.44% | 1 | 0% | 0 |  | 169 | +0.60% |
| Load/for_range | 5 | 0% | 5 | 0% | 0 |  | 738 | -2.38% |
| Render/for_range | 1 | 0% | 1 | 0% | 0 |  | 127 | +12.39% |
| Load/html_autoescape | 446 | +0.68% | 519 | +0.97% | 0 |  | 4,098 | +2.91% |
| Render/html_autoescape | 4,812 | -1.53% | 1,632 | -2.91% | 0 |  | 36,071 | +6.22% |
| Load/inheritance | 9 | 0% | 3 | 0% | 0 |  | 644 | -4.87% |
| Render/inheritance | 614 | +0.33% | 105 | 0% | 0 |  | 886 | +0.91% |
| Load/int_arith | 107 | 0% | 88 | 0% | 0 |  | 2,034 | +2.83% |
| Render/int_arith | 16 | +6.67% | 4 | 0% | 0 |  | 740 | +5.56% |
| Module/keep | 2 | 0% | 1 | 0% | 0 |  | 382 | +11.37% |
| Module/keep_written | 2,932 | 0% | 557 | 0% | 0 |  | 723 | +5.09% |
| Load/large_static | 1,956 | +0.41% | 912 | +0.22% | 0 |  | 275 | -3.17% |
| Render/large_static | 709 | 0% | 570 | 0% | 0 |  | 3 | -75.00% |
| RenderSink/large_static | 0 |  | 0 |  | 0 |  | 2 | +100.00% |
| RenderTo/large_static | 687 | 0% | 570 | 0% | 0 |  | 3 | 0% |
| Load/macros | 80 | -4.76% | 76 | -1.30% | 0 |  | 1,652 | -1.90% |
| Render/macros | 772 | +0.65% | 257 | 0% | 0 |  | 606 | -37.14% |
| Load/many_tags | 29,087 | +0.40% | 22,322 | +0.51% | 0 |  | 266,938 | +8.21% |
| Render/many_tags | 6,119 | 0% | 867 | 0% | 0 |  | 161 | -21.84% |
| Load/mitsuhiko_table | 34 | 0% | 51 | -1.92% | 0 |  | 851 | -1.05% |
| Render/mitsuhiko_table | 10,082 | 0% | 6,375 | 0% | 0 |  | 366 | +0.55% |
| RenderSink/mitsuhiko_table | 9,959 | 0% | 1,226 | 0% | 0 |  | 279 | -8.82% |
| RenderTo/mitsuhiko_table | 10,027 | 0% | 6,310 | 0% | 0 |  | 339 | -1.74% |
| Load/mitsuhiko_table_wide | 79 | +5.33% | 93 | -1.06% | 0 |  | 960 | -13.04% |
| Render/mitsuhiko_table_wide | 10,231 | +0.06% | 22,697 | 0% | 0 |  | 351 | -1.40% |
| RenderSink/mitsuhiko_table_wide | 10,045 | +0.05% | 1,306 | -0.23% | 0 |  | 268 | -13.55% |
| RenderTo/mitsuhiko_table_wide | 10,244 | +0.06% | 22,686 | 0% | 0 |  | 321 | -5.59% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 31 | +106.67% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 7 | -12.50% |
| RenderSink/plain_text | 0 |  | 0 |  | 0 |  | 5 | 0% |
| RenderTo/plain_text | 0 |  | 0 |  | 0 |  | 6 | -14.29% |
| Load/strings | 152 | 0% | 84 | -1.18% | 0 |  | 2,856 | -0.90% |
| Render/strings | 450 | +0.90% | 472 | -0.21% | 0 |  | 1,498 | +0.13% |
| Load/substitute | 2 | 0% | 3 | 0% | 0 |  | 167 | -10.70% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 2 | -50.00% |
| RenderSink/substitute | 0 |  | 0 |  | 0 |  | 2 | 0% |
| RenderTo/substitute | 0 |  | 0 |  | 0 |  | 2 | -33.33% |

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
