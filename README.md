# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (189 so far, latest [8cdbac3](https://github.com/jinja2cpp/Jinja2Cpp/commit/8cdbac3fe05b52c8ee79daee05300be7d93946b0) on 2026-11-06). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 345,272 | 0% |  | 55 | 0% |  | 18,480 | 0% | -52.71% |
| [Render/chat_llama](#renderchat_llama) | 294,787 | 0% |  | 225 | 0% |  | 11,632 | 0% | -19.00% |
| [Load/chat_mistral](#loadchat_mistral) | 393,803 | 0% |  | 52 | 0% |  | 20,888 | 0% | -53.22% |
| [Render/chat_mistral](#renderchat_mistral) | 499,204 | 0% |  | 402 | 0% |  | 9,968 | 0% | -24.53% |
| [Load/chat_qwen](#loadchat_qwen) | 266,540 | +0.02% |  | 44 | 0% |  | 14,416 | 0% | -47.25% |
| [Render/chat_qwen](#renderchat_qwen) | 276,949 | 0% |  | 212 | 0% |  | 8,304 | 0% | -2.35% |
| [Load/config_file](#loadconfig_file) | 97,178 | +0.14% |  | 25 | 0% |  | 5,464 | 0% | -48.26% |
| [Render/config_file](#renderconfig_file) | 1,268,683 | 0% |  | 908 | 0% |  | 24,440 | 0% | +2.65% |
| [Load/dict_ops](#loaddict_ops) | 33,314 | 0% | -77.46% | 9 | 0% | -93.13% | 2,176 | 0% | -48.87% |
| [Render/dict_ops](#renderdict_ops) | 293,156 | 0% | -57.57% | 112 | 0% | -75.11% | 24,408 | 0% | -29.65% |
| [Load/expressions](#loadexpressions) | 45,029 | +0.03% | -78.16% | 16 | 0% | -89.04% | 2,728 | 0% | -55.54% |
| [Render/expressions](#renderexpressions) | 426,198 | 0% | -52.97% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 69,188 | +0.32% | -74.79% | 27 | 0% | -88.16% | 3,848 | 0% | -66.29% |
| [Render/filters](#renderfilters) | 30,426 | 0% | -65.69% | 26 | 0% | -64.86% | 1,080 | 0% | -78.01% |
| [Load/for_filter_if](#loadfor_filter_if) | 30,002 | -0.05% | -77.84% | 7 | 0% | -94.31% | 1,904 | 0% | -44.78% |
| [Render/for_filter_if](#renderfor_filter_if) | 274,076 | 0% | -68.33% | 4 | 0% | -98.23% | 16,560 | 0% | +354.95% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 40,295 | -0.04% | -77.70% | 10 | 0% | -93.63% | 2,368 | 0% | -47.61% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 180,541 | 0% | -83.17% | 2 | 0% | -99.62% | 1,568 | 0% | -43.35% |
| [Load/for_range](#loadfor_range) | 13,641 | +0.05% | -75.93% | 7 | 0% | -87.50% | 1,088 | 0% | -45.60% |
| [Render/for_range](#renderfor_range) | 38,352 | 0% | -58.41% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/hf_deepseek_v31](#loadhf_deepseek_v31) | 388,198 | -0.01% |  | 55 | 0% |  | 21,976 | 0% | +0.73% |
| [Render/hf_deepseek_v31](#renderhf_deepseek_v31) | 90,509 | 0% |  | 43 | 0% |  | 2,480 | 0% | 0% |
| [Load/hf_gpt_oss](#loadhf_gpt_oss) | 1,549,329 | -0.01% |  | 287 | 0% |  | 74,640 | 0% | +0.56% |
| [Render/hf_gpt_oss](#renderhf_gpt_oss) | 170,638 | 0% |  | 155 | 0% |  | 5,096 | 0% | +3.24% |
| [Load/hf_mistral_small](#loadhf_mistral_small) | 598,941 | 0% |  | 88 | 0% |  | 33,048 | 0% | +0.34% |
| [Render/hf_mistral_small](#renderhf_mistral_small) | 144,711 | 0% |  | 105 | 0% |  | 6,120 | 0% | -0.26% |
| [Load/hf_qwen3](#loadhf_qwen3) | 485,830 | +0.01% |  | 82 | 0% |  | 25,560 | 0% | +0.31% |
| [Render/hf_qwen3](#renderhf_qwen3) | 119,879 | 0% |  | 100 | 0% |  | 3,248 | 0% | 0% |
| [Load/html_autoescape](#loadhtml_autoescape) | 193,825 | 0% |  | 43 | 0% |  | 12,640 | 0% | -48.78% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,208,953 | 0% |  | 674 | 0% |  | 45,408 | 0% | +12.44% |
| [Load/inheritance](#loadinheritance) | 23,055 | +0.07% | -79.80% | 7 | 0% | -93.46% | 1,344 | 0% | -53.20% |
| [Render/inheritance](#renderinheritance) | 137,603 | 0% | -81.60% | 35 | 0% | -94.37% | 7,384 | 0% | +22.74% |
| [Load/int_arith](#loadint_arith) | 65,970 | +0.06% |  | 20 | 0% |  | 3,872 | 0% | +0.41% |
| [Render/int_arith](#renderint_arith) | 824,292 | 0% |  | 12 | 0% |  | 3,272 | 0% | -0.49% |
| [Module/keep](#modulekeep) | 6,996 | 0% |  | 7 | 0% |  |  |  |  |
| [Module/keep_written](#modulekeep_written) | 36,405 | 0% |  | 11 | 0% |  |  |  |  |
| [Load/large_static](#loadlarge_static) | 154,709 | +0.01% | -82.54% | 14 | 0% | -96.82% | 43,120 | 0% | -19.23% |
| [Render/large_static](#renderlarge_static) | 20,709 | 0% | -72.44% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [RenderSink/large_static](#rendersinklarge_static) | 12,699 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/large_static](#rendertolarge_static) | 20,272 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/macros](#loadmacros) | 56,375 | +0.09% | -75.27% | 27 | 0% | -85.48% | 4,392 | 0% | -29.34% |
| [Render/macros](#rendermacros) | 656,537 | 0% | -74.38% | 308 | 0% | -88.71% | 20,560 | 0% | +39.67% |
| [Load/many_tags](#loadmany_tags) | 9,015,155 | +0.09% | -79.59% | 951 | 0% | -97.22% | 438,160 | 0% | -58.48% |
| [Render/many_tags](#rendermany_tags) | 728,682 | 0% | -65.02% | 2 | 0% | -90.48% | 5,648 | 0% | -10.52% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 46,577 | +0.03% | -80.39% | 19 | 0% | -89.67% | 3,360 | 0% | -44.59% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,196,673 | 0% | -57.94% | 33 | 0% | -99.19% | 381,536 | 0% | +9.95% |
| [RenderSink/mitsuhiko_table](#rendersinkmitsuhiko_table) | 5,123,866 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table](#rendertomitsuhiko_table) | 5,196,177 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 54,833 | +0.01% |  | 35 | 0% |  | 5,688 | 0% | -31.17% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,542,838 | 0% |  | 33 | 0% |  | 1,414,928 | 0% | +2.46% |
| [RenderSink/mitsuhiko_table_wide](#rendersinkmitsuhiko_table_wide) | 5,366,127 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table_wide](#rendertomitsuhiko_table_wide) | 5,542,390 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/plain_text](#loadplain_text) | 3,720 | +0.19% | -61.55% | 4 | 0% | -77.78% | 672 | 0% | -40.00% |
| [Render/plain_text](#renderplain_text) | 1,019 | 0% | -85.00% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/plain_text](#rendersinkplain_text) | 721 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/plain_text](#rendertoplain_text) | 820 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/prompt_budget](#loadprompt_budget) | 35,128 | -0.04% |  | 18 | 0% |  | 2,224 | 0% | 0% |
| [Render/prompt_budget](#renderprompt_budget) | 1,314,517 | 0% |  | 76 | 0% |  | 36,416 | 0% | 0% |
| [Load/strings](#loadstrings) | 70,466 | +0.19% | -75.86% | 30 | 0% | -86.61% | 4,344 | 0% | -51.95% |
| [Render/strings](#renderstrings) | 578,990 | 0% | -72.40% | 344 | 0% | -83.73% | 9,088 | 0% | -49.31% |
| [Load/substitute](#loadsubstitute) | 7,753 | +0.09% | -74.03% | 6 | 0% | -83.78% | 832 | 0% | -45.26% |
| [Render/substitute](#rendersubstitute) | 2,252 | 0% | -74.45% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/substitute](#rendersinksubstitute) | 1,958 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/substitute](#rendertosubstitute) | 2,057 | 0% |  | 0 |  |  | 0 |  |  |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 734 | -0.54% | 752 | +0.67% | 0 |  | 7,135 | -1.94% |
| Render/chat_llama | 1,156 | 0% | 528 | 0% | 0 |  | 4,200 | -2.80% |
| Load/chat_mistral | 1,044 | +1.95% | 1,045 | +0.19% | 0 |  | 8,021 | -1.23% |
| Render/chat_mistral | 2,009 | 0% | 575 | 0% | 0 |  | 9,352 | -6.83% |
| Load/chat_qwen | 493 | -0.60% | 597 | +0.67% | 0 |  | 4,088 | -5.22% |
| Render/chat_qwen | 1,079 | 0% | 402 | 0% | 0 |  | 3,355 | -1.03% |
| Load/config_file | 194 | -3.48% | 171 | -0.58% | 0 |  | 2,520 | -3.34% |
| Render/config_file | 6,392 | 0% | 1,549 | 0% | 0 |  | 40,675 | -1.51% |
| Load/dict_ops | 28 | -3.45% | 21 | +5.00% | 0 |  | 1,309 | -0.76% |
| Render/dict_ops | 661 | 0% | 421 | 0% | 0 |  | 868 | -0.12% |
| Load/expressions | 9 | 0% | 5 | -16.67% | 0 |  | 1,278 | -0.39% |
| Render/expressions | 12 | 0% | 6 | 0% | 0 |  | 319 | +13.12% |
| Load/filters | 75 | 0% | 47 | -7.84% | 0 |  | 1,665 | +17.58% |
| Render/filters | 24 | 0% | 5 | 0% | 0 |  | 787 | +1.68% |
| Load/for_filter_if | 22 | -21.43% | 16 | -5.88% | 0 |  | 1,053 | 0% |
| Render/for_filter_if | 1,806 | 0% | 97 | 0% | 0 |  | 175 | -15.05% |
| Load/for_loop_vars | 31 | +19.23% | 16 | +23.08% | 0 |  | 1,035 | -6.25% |
| Render/for_loop_vars | 8 | 0% | 3 | 0% | 0 |  | 152 | -24.38% |
| Load/for_range | 4 | +33.33% | 3 | -25.00% | 0 |  | 753 | +0.40% |
| Render/for_range | 1 | 0% | 0 |  | 0 |  | 125 | -16.67% |
| Load/hf_deepseek_v31 | 984 | +0.92% | 974 | +0.72% | 0 |  | 5,962 | -2.52% |
| Render/hf_deepseek_v31 | 355 | 0% | 81 | 0% | 0 |  | 1,365 | -4.55% |
| Load/hf_gpt_oss | 4,602 | -0.37% | 3,294 | +0.03% | 0 |  | 20,364 | -4.11% |
| Render/hf_gpt_oss | 1,207 | 0% | 375 | 0% | 0 |  | 4,453 | -1.22% |
| Load/hf_mistral_small | 1,819 | +0.66% | 1,583 | +0.13% | 0 |  | 10,263 | -1.41% |
| Render/hf_mistral_small | 962 | 0% | 314 | 0% | 0 |  | 3,791 | -1.51% |
| Load/hf_qwen3 | 1,415 | +0.28% | 1,242 | +0.49% | 0 |  | 8,157 | -3.27% |
| Render/hf_qwen3 | 666 | 0% | 217 | 0% | 0 |  | 2,558 | +0.59% |
| Load/html_autoescape | 449 | -2.81% | 518 | +0.78% | 0 |  | 3,607 | -1.04% |
| Render/html_autoescape | 4,230 | 0% | 1,230 | 0% | 0 |  | 36,412 | +2.79% |
| Load/inheritance | 2 | 0% | 3 | -25.00% | 0 |  | 709 | +6.30% |
| Render/inheritance | 586 | 0% | 96 | 0% | 0 |  | 917 | -1.93% |
| Load/int_arith | 69 | 0% | 38 | 0% | 0 |  | 1,976 | +0.05% |
| Render/int_arith | 27 | 0% | 15 | 0% | 0 |  | 763 | +1.33% |
| Module/keep | 2 | 0% | 2 | 0% | 0 |  | 386 | -6.31% |
| Module/keep_written | 2,931 | 0% | 561 | 0% | 0 |  | 719 | -0.42% |
| Load/large_static | 1,927 | 0% | 913 | +0.55% | 0 |  | 285 | +8.78% |
| Render/large_static | 710 | 0% | 568 | 0% | 0 |  | 5 | -16.67% |
| RenderSink/large_static | 0 |  | 0 |  | 0 |  | 3 | 0% |
| RenderTo/large_static | 688 | 0% | 570 | 0% | 0 |  | 4 | +33.33% |
| Load/macros | 69 | -1.43% | 50 | -7.41% | 0 |  | 1,699 | -1.05% |
| Render/macros | 757 | 0% | 213 | 0% | 0 |  | 1,346 | +123.59% |
| Load/many_tags | 27,706 | -0.37% | 22,373 | -0.06% | 0 |  | 219,821 | -0.80% |
| Render/many_tags | 6,219 | 0% | 862 | 0% | 0 |  | 113 | -31.93% |
| Load/mitsuhiko_table | 27 | 0% | 29 | -6.45% | 0 |  | 846 | +2.55% |
| Render/mitsuhiko_table | 10,019 | 0% | 6,280 | 0% | 0 |  | 355 | -10.35% |
| RenderSink/mitsuhiko_table | 9,954 | 0% | 1,183 | 0% | 0 |  | 280 | -13.04% |
| RenderTo/mitsuhiko_table | 10,026 | 0% | 6,280 | 0% | 0 |  | 319 | -14.71% |
| Load/mitsuhiko_table_wide | 71 | -10.13% | 71 | +4.41% | 0 |  | 949 | -1.04% |
| Render/mitsuhiko_table_wide | 10,238 | 0% | 22,676 | 0% | 0 |  | 339 | -10.79% |
| RenderSink/mitsuhiko_table_wide | 10,062 | 0% | 1,289 | 0% | 0 |  | 282 | -11.60% |
| RenderTo/mitsuhiko_table_wide | 10,227 | 0% | 22,673 | 0% | 0 |  | 316 | -11.24% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 29 | 0% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 8 | 0% |
| RenderSink/plain_text | 0 |  | 0 |  | 0 |  | 4 | -33.33% |
| RenderTo/plain_text | 0 |  | 0 |  | 0 |  | 6 | -14.29% |
| Load/prompt_budget | 61 | -6.15% | 39 | 0% | 0 |  | 1,380 | +1.92% |
| Render/prompt_budget | 4,786 | 0% | 526 | 0% | 0 |  | 996 | +8.26% |
| Load/strings | 108 | -1.82% | 76 | -2.56% | 0 |  | 2,734 | +2.28% |
| Render/strings | 409 | 0% | 477 | 0% | 0 |  | 1,524 | -2.50% |
| Load/substitute | 1 | 0% | 2 | -50.00% | 0 |  | 184 | +25.17% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 4 | 0% |
| RenderSink/substitute | 0 |  | 0 |  | 0 |  | 3 | 0% |
| RenderTo/substitute | 0 |  | 0 |  | 0 |  | 4 | +33.33% |

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
