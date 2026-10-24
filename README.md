# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (129 so far, latest [d79e11c](https://github.com/jinja2cpp/Jinja2Cpp/commit/d79e11cb6b8fa35ffc2439e88341df00b51cf6eb) on 2026-10-24). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 345,247 | +0.01% |  | 55 | 0% |  | 18,288 | +0.09% | -53.20% |
| [Render/chat_llama](#renderchat_llama) | 305,932 | 0% |  | 251 | 0% |  | 11,136 | 0% | -22.45% |
| [Load/chat_mistral](#loadchat_mistral) | 394,126 | -0.09% |  | 51 | 0% |  | 20,744 | +0.08% | -53.54% |
| [Render/chat_mistral](#renderchat_mistral) | 508,740 | +0.09% |  | 424 | 0% |  | 9,536 | +0.17% | -27.80% |
| [Load/chat_qwen](#loadchat_qwen) | 266,542 | 0% |  | 44 | 0% |  | 14,448 | +0.11% | -47.13% |
| [Render/chat_qwen](#renderchat_qwen) | 293,374 | 0% |  | 238 | 0% |  | 7,824 | 0% | -8.00% |
| [Load/config_file](#loadconfig_file) | 98,888 | 0% |  | 25 | 0% |  | 5,480 | +0.29% | -48.11% |
| [Render/config_file](#renderconfig_file) | 1,256,800 | +0.02% |  | 908 | 0% |  | 23,320 | 0% | -2.05% |
| [Load/dict_ops](#loaddict_ops) | 33,234 | 0% | -77.51% | 9 | 0% | -93.13% | 2,224 | +0.72% | -47.74% |
| [Render/dict_ops](#renderdict_ops) | 297,698 | 0% | -56.91% | 112 | 0% | -75.11% | 24,408 | 0% | -29.65% |
| [Load/expressions](#loadexpressions) | 45,429 | 0% | -77.96% | 16 | 0% | -89.04% | 2,776 | +0.58% | -54.76% |
| [Render/expressions](#renderexpressions) | 423,478 | 0% | -53.27% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 72,008 | -0.01% | -73.76% | 27 | 0% | -88.16% | 3,784 | +0.42% | -66.85% |
| [Render/filters](#renderfilters) | 30,143 | -2.64% | -66.00% | 26 | -10.34% | -64.86% | 1,080 | -21.51% | -78.01% |
| [Load/for_filter_if](#loadfor_filter_if) | 30,015 | 0% | -77.83% | 7 | 0% | -94.31% | 1,968 | +0.82% | -42.92% |
| [Render/for_filter_if](#renderfor_filter_if) | 271,642 | 0% | -68.61% | 4 | 0% | -98.23% | 13,328 | 0% | +266.15% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 40,525 | -0.02% | -77.57% | 10 | 0% | -93.63% | 2,416 | +0.67% | -46.55% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 194,138 | 0% | -81.90% | 2 | 0% | -99.62% | 1,552 | 0% | -43.93% |
| [Load/for_range](#loadfor_range) | 13,648 | 0% | -75.92% | 7 | 0% | -87.50% | 1,152 | +1.41% | -42.40% |
| [Render/for_range](#renderfor_range) | 38,749 | 0% | -57.98% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/html_autoescape](#loadhtml_autoescape) | 194,350 | -0.03% |  | 43 | 0% |  | 12,608 | +0.13% | -48.91% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,210,007 | -0.01% |  | 674 | 0% |  | 43,680 | 0% | +8.16% |
| [Load/inheritance](#loadinheritance) | 23,162 | 0% | -79.70% | 7 | 0% | -93.46% | 1,488 | +1.09% | -48.19% |
| [Render/inheritance](#renderinheritance) | 136,426 | 0% | -81.76% | 35 | 0% | -94.37% | 6,456 | 0% | +7.31% |
| [Load/int_arith](#loadint_arith) | 66,033 | 0% |  | 20 | 0% |  | 3,872 | +0.41% | +0.41% |
| [Render/int_arith](#renderint_arith) | 819,382 | 0% |  | 12 | 0% |  | 3,288 | 0% | 0% |
| [Load/large_static](#loadlarge_static) | 155,011 | -0.03% | -82.50% | 14 | 0% | -96.82% | 43,184 | +0.04% | -19.11% |
| [Render/large_static](#renderlarge_static) | 20,881 | 0% | -72.21% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [RenderSink/large_static](#rendersinklarge_static) | 12,691 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/large_static](#rendertolarge_static) | 20,260 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/macros](#loadmacros) | 55,346 | -0.01% | -75.72% | 27 | 0% | -85.48% | 4,408 | +0.36% | -29.09% |
| [Render/macros](#rendermacros) | 645,919 | 0% | -74.80% | 308 | 0% | -88.71% | 18,848 | 0% | +28.04% |
| [Load/many_tags](#loadmany_tags) | 9,108,989 | 0% | -79.37% | 951 | 0% | -97.22% | 431,184 | 0% | -59.14% |
| [Render/many_tags](#rendermany_tags) | 720,834 | 0% | -65.39% | 2 | 0% | -90.48% | 5,632 | 0% | -10.77% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 46,353 | 0% | -80.49% | 19 | 0% | -89.67% | 3,424 | +0.47% | -43.54% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,187,129 | 0% | -58.02% | 33 | 0% | -99.19% | 373,296 | 0% | +7.58% |
| [RenderSink/mitsuhiko_table](#rendersinkmitsuhiko_table) | 5,114,410 | 0% |  | 32 | 0% |  | 28,824 | 0% | 0% |
| [RenderTo/mitsuhiko_table](#rendertomitsuhiko_table) | 5,186,679 | 0% |  | 32 | 0% |  | 28,824 | 0% | 0% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 55,689 | 0% |  | 35 | 0% |  | 5,752 | +0.28% | -30.40% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,533,186 | 0% |  | 33 | 0% |  | 1,406,688 | 0% | +1.86% |
| [RenderSink/mitsuhiko_table_wide](#rendersinkmitsuhiko_table_wide) | 5,356,489 | 0% |  | 32 | 0% |  | 28,824 | 0% | 0% |
| [RenderTo/mitsuhiko_table_wide](#rendertomitsuhiko_table_wide) | 5,532,723 | 0% |  | 32 | 0% |  | 28,824 | 0% | 0% |
| [Load/plain_text](#loadplain_text) | 3,747 | 0% | -61.27% | 4 | 0% | -77.78% | 736 | +2.22% | -34.29% |
| [Render/plain_text](#renderplain_text) | 1,013 | 0% | -85.09% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/plain_text](#rendersinkplain_text) | 719 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/plain_text](#rendertoplain_text) | 814 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/strings](#loadstrings) | 72,249 | 0% | -75.24% | 29 | 0% | -87.05% | 4,296 | +0.37% | -52.48% |
| [Render/strings](#renderstrings) | 577,479 | 0% | -72.47% | 344 | 0% | -83.73% | 9,104 | 0% | -49.22% |
| [Load/substitute](#loadsubstitute) | 7,776 | 0% | -73.96% | 6 | 0% | -83.78% | 896 | +1.82% | -41.05% |
| [Render/substitute](#rendersubstitute) | 2,230 | 0% | -74.70% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/substitute](#rendersinksubstitute) | 1,941 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/substitute](#rendertosubstitute) | 2,035 | 0% |  | 0 |  |  | 0 |  |  |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 722 | 0% | 709 | -0.98% | 0 |  | 7,485 | +2.77% |
| Render/chat_llama | 1,101 | -1.17% | 485 | -0.21% | 0 |  | 3,654 | +4.31% |
| Load/chat_mistral | 1,039 | +2.77% | 1,008 | -0.20% | 0 |  | 8,311 | +3.45% |
| Render/chat_mistral | 1,821 | -3.65% | 512 | -1.92% | 0 |  | 9,055 | -1.07% |
| Load/chat_qwen | 471 | +2.39% | 600 | +1.69% | 0 |  | 4,236 | -0.80% |
| Render/chat_qwen | 1,041 | +0.39% | 373 | -1.32% | 0 |  | 3,103 | -1.08% |
| Load/config_file | 174 | +4.82% | 170 | +2.41% | 0 |  | 2,986 | -0.13% |
| Render/config_file | 5,881 | +4.81% | 1,815 | +1.34% | 0 |  | 38,014 | -3.53% |
| Load/dict_ops | 35 | +9.38% | 15 | -11.76% | 0 |  | 1,292 | -1.90% |
| Render/dict_ops | 740 | -0.40% | 430 | +0.47% | 0 |  | 829 | +0.85% |
| Load/expressions | 31 | +10.71% | 11 | 0% | 0 |  | 1,355 | 0% |
| Render/expressions | 2 | 0% | 0 | -100.00% | 0 |  | 273 | -9.30% |
| Load/filters | 122 | +1.67% | 71 | -1.39% | 0 |  | 2,150 | +0.89% |
| Render/filters | 24 | +26.32% | 5 | -28.57% | 0 |  | 716 | -6.16% |
| Load/for_filter_if | 5 | 0% | 2 | 0% | 0 |  | 1,073 | -2.37% |
| Render/for_filter_if | 1,648 | -0.12% | 67 | +1.52% | 0 |  | 142 | -17.44% |
| Load/for_loop_vars | 18 | +63.64% | 7 | 0% | 0 |  | 1,134 | -1.31% |
| Render/for_loop_vars | 7 | +16.67% | 3 | 0% | 0 |  | 105 | -29.53% |
| Load/for_range | 3 | -50.00% | 3 | -40.00% | 0 |  | 745 | -2.36% |
| Render/for_range | 0 | -100.00% | 1 | 0% | 0 |  | 76 | -19.15% |
| Load/html_autoescape | 466 | +0.43% | 515 | +1.18% | 0 |  | 4,372 | -0.34% |
| Render/html_autoescape | 4,584 | -1.74% | 1,409 | -0.28% | 0 |  | 33,798 | +4.61% |
| Load/inheritance | 3 | 0% | 3 | +50.00% | 0 |  | 613 | -6.27% |
| Render/inheritance | 580 | -0.85% | 121 | +0.83% | 0 |  | 837 | -1.76% |
| Load/int_arith | 70 | +2.94% | 43 | -8.51% | 0 |  | 2,087 | -1.14% |
| Render/int_arith | 21 | +61.54% | 8 | +60.00% | 0 |  | 682 | +1.49% |
| Load/large_static | 1,943 | +0.05% | 908 | -0.22% | 0 |  | 406 | +45.00% |
| Render/large_static | 707 | 0% | 568 | 0% | 0 |  | 5 | -58.33% |
| RenderSink/large_static | 1 | 0% | 0 |  | 0 |  | 3 | 0% |
| RenderTo/large_static | 686 | 0% | 568 | 0% | 0 |  | 4 | 0% |
| Load/macros | 59 | -23.38% | 46 | -8.00% | 0 |  | 1,665 | -0.66% |
| Render/macros | 721 | 0% | 209 | 0% | 0 |  | 926 | -30.90% |
| Load/many_tags | 27,749 | +0.02% | 22,224 | +0.09% | 0 |  | 273,410 | +7.39% |
| Render/many_tags | 6,131 | -0.03% | 843 | -0.94% | 0 |  | 159 | -13.59% |
| Load/mitsuhiko_table | 13 | +30.00% | 15 | +7.14% | 0 |  | 853 | -1.73% |
| Render/mitsuhiko_table | 9,028 | -0.01% | 6,603 | -0.02% | 0 |  | 300 | -7.12% |
| RenderSink/mitsuhiko_table | 8,993 | -0.01% | 1,494 | +0.13% | 0 |  | 258 | -0.39% |
| RenderTo/mitsuhiko_table | 9,032 | -0.01% | 6,613 | 0% | 0 |  | 279 | -3.46% |
| Load/mitsuhiko_table_wide | 93 | +16.25% | 88 | +15.79% | 0 |  | 962 | -2.83% |
| Render/mitsuhiko_table_wide | 9,182 | 0% | 23,043 | +0.01% | 0 |  | 296 | -5.73% |
| RenderSink/mitsuhiko_table_wide | 9,076 | 0% | 1,534 | 0% | 0 |  | 238 | -10.19% |
| RenderTo/mitsuhiko_table_wide | 9,173 | -0.02% | 22,997 | -0.01% | 0 |  | 267 | -7.29% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 15 | +150.00% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 7 | +40.00% |
| RenderSink/plain_text | 0 |  | 0 |  | 0 |  | 4 | 0% |
| RenderTo/plain_text | 0 |  | 0 |  | 0 |  | 5 | 0% |
| Load/strings | 180 | +1.69% | 116 | -4.13% | 0 |  | 2,852 | +1.10% |
| Render/strings | 410 | -1.91% | 457 | -0.65% | 0 |  | 1,438 | -3.30% |
| Load/substitute | 2 | 0% | 3 | 0% | 0 |  | 174 | +6.10% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 4 | -33.33% |
| RenderSink/substitute | 0 |  | 0 |  | 0 |  | 3 | -25.00% |
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
