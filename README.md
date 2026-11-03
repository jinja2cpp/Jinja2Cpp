# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (174 so far, latest [8e93fe8](https://github.com/jinja2cpp/Jinja2Cpp/commit/8e93fe870e572db66528ec584fe4bc4950fec73d) on 2026-11-03). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 343,175 | -0.04% |  | 55 | 0% |  | 18,464 | 0% | -52.75% |
| [Render/chat_llama](#renderchat_llama) | 295,349 | 0% |  | 225 | 0% |  | 11,616 | 0% | -19.11% |
| [Load/chat_mistral](#loadchat_mistral) | 392,365 | -0.04% |  | 52 | 0% |  | 20,888 | 0% | -53.22% |
| [Render/chat_mistral](#renderchat_mistral) | 498,488 | 0% |  | 402 | 0% |  | 10,016 | 0% | -24.17% |
| [Load/chat_qwen](#loadchat_qwen) | 264,969 | -0.03% |  | 44 | 0% |  | 14,416 | 0% | -47.25% |
| [Render/chat_qwen](#renderchat_qwen) | 277,051 | 0% |  | 212 | 0% |  | 8,304 | 0% | -2.35% |
| [Load/config_file](#loadconfig_file) | 96,591 | -0.03% |  | 25 | 0% |  | 5,464 | 0% | -48.26% |
| [Render/config_file](#renderconfig_file) | 1,265,678 | 0% |  | 908 | 0% |  | 24,440 | 0% | +2.65% |
| [Load/dict_ops](#loaddict_ops) | 33,166 | -0.03% | -77.56% | 9 | 0% | -93.13% | 2,176 | 0% | -48.87% |
| [Render/dict_ops](#renderdict_ops) | 293,389 | 0% | -57.53% | 112 | 0% | -75.11% | 24,408 | 0% | -29.65% |
| [Load/expressions](#loadexpressions) | 44,894 | -0.01% | -78.22% | 16 | 0% | -89.04% | 2,728 | 0% | -55.54% |
| [Render/expressions](#renderexpressions) | 427,169 | 0% | -52.86% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 68,911 | +0.01% | -74.89% | 27 | 0% | -88.16% | 3,848 | 0% | -66.29% |
| [Render/filters](#renderfilters) | 30,243 | 0% | -65.89% | 26 | 0% | -64.86% | 1,080 | 0% | -78.01% |
| [Load/for_filter_if](#loadfor_filter_if) | 29,853 | -0.04% | -77.95% | 7 | 0% | -94.31% | 1,904 | 0% | -44.78% |
| [Render/for_filter_if](#renderfor_filter_if) | 275,089 | 0% | -68.22% | 4 | 0% | -98.23% | 16,560 | 0% | +354.95% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 40,082 | -0.03% | -77.81% | 10 | 0% | -93.63% | 2,368 | 0% | -47.61% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 192,299 | 0% | -82.07% | 2 | 0% | -99.62% | 1,568 | 0% | -43.35% |
| [Load/for_range](#loadfor_range) | 13,571 | -0.04% | -76.06% | 7 | 0% | -87.50% | 1,088 | 0% | -45.60% |
| [Render/for_range](#renderfor_range) | 38,821 | 0% | -57.90% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/hf_deepseek_v31](#loadhf_deepseek_v31) | 386,011 | 0% |  | 55 | 0% |  | 21,976 | 0% | +0.73% |
| [Render/hf_deepseek_v31](#renderhf_deepseek_v31) | 90,318 | 0% |  | 43 | 0% |  | 2,480 | 0% | 0% |
| [Load/hf_gpt_oss](#loadhf_gpt_oss) | 1,532,254 | -0.02% |  | 285 | 0% |  | 74,384 | 0% | +0.22% |
| [Render/hf_gpt_oss](#renderhf_gpt_oss) | 170,270 | 0% |  | 155 | 0% |  | 5,064 | 0% | +2.59% |
| [Load/hf_mistral_small](#loadhf_mistral_small) | 595,499 | -0.03% |  | 88 | 0% |  | 33,048 | 0% | +0.34% |
| [Render/hf_mistral_small](#renderhf_mistral_small) | 144,581 | 0% |  | 105 | 0% |  | 6,120 | 0% | -0.26% |
| [Load/hf_qwen3](#loadhf_qwen3) | 483,379 | -0.04% |  | 82 | 0% |  | 25,560 | 0% | +0.31% |
| [Render/hf_qwen3](#renderhf_qwen3) | 120,030 | 0% |  | 100 | 0% |  | 3,248 | 0% | 0% |
| [Load/html_autoescape](#loadhtml_autoescape) | 192,876 | 0% |  | 43 | 0% |  | 12,640 | 0% | -48.78% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,211,254 | 0% |  | 674 | 0% |  | 45,408 | 0% | +12.44% |
| [Load/inheritance](#loadinheritance) | 22,740 | -0.10% | -80.07% | 7 | 0% | -93.46% | 1,328 | 0% | -53.76% |
| [Render/inheritance](#renderinheritance) | 137,619 | 0% | -81.60% | 35 | 0% | -94.37% | 7,384 | 0% | +22.74% |
| [Load/int_arith](#loadint_arith) | 65,830 | -0.01% |  | 20 | 0% |  | 3,872 | 0% | +0.41% |
| [Render/int_arith](#renderint_arith) | 824,991 | 0% |  | 12 | 0% |  | 3,272 | 0% | -0.49% |
| [Module/keep](#modulekeep) | 6,960 | -0.07% |  | 7 | 0% |  |  |  |  |
| [Module/keep_written](#modulekeep_written) | 36,313 | -0.01% |  | 11 | 0% |  |  |  |  |
| [Load/large_static](#loadlarge_static) | 154,163 | +0.04% | -82.60% | 14 | 0% | -96.82% | 43,120 | 0% | -19.23% |
| [Render/large_static](#renderlarge_static) | 20,700 | 0% | -72.45% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [RenderSink/large_static](#rendersinklarge_static) | 12,686 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/large_static](#rendertolarge_static) | 20,263 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/macros](#loadmacros) | 55,680 | 0% | -75.57% | 27 | 0% | -85.48% | 4,344 | 0% | -30.12% |
| [Render/macros](#rendermacros) | 656,763 | 0% | -74.38% | 308 | 0% | -88.71% | 20,560 | 0% | +39.67% |
| [Load/many_tags](#loadmany_tags) | 8,967,706 | -0.03% | -79.69% | 951 | 0% | -97.22% | 438,176 | 0% | -58.48% |
| [Render/many_tags](#rendermany_tags) | 725,698 | 0% | -65.16% | 2 | 0% | -90.48% | 5,648 | 0% | -10.52% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 46,294 | -0.01% | -80.51% | 19 | 0% | -89.67% | 3,360 | 0% | -44.59% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,235,831 | 0% | -57.63% | 33 | 0% | -99.19% | 381,536 | 0% | +9.95% |
| [RenderSink/mitsuhiko_table](#rendersinkmitsuhiko_table) | 5,163,000 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table](#rendertomitsuhiko_table) | 5,235,314 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 54,532 | +0.02% |  | 35 | 0% |  | 5,688 | 0% | -31.17% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,581,776 | 0% |  | 33 | 0% |  | 1,414,928 | 0% | +2.46% |
| [RenderSink/mitsuhiko_table_wide](#rendersinkmitsuhiko_table_wide) | 5,405,063 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table_wide](#rendertomitsuhiko_table_wide) | 5,581,347 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/plain_text](#loadplain_text) | 3,693 | +0.03% | -61.83% | 4 | 0% | -77.78% | 672 | 0% | -40.00% |
| [Render/plain_text](#renderplain_text) | 1,017 | 0% | -85.03% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/plain_text](#rendersinkplain_text) | 717 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/plain_text](#rendertoplain_text) | 818 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/strings](#loadstrings) | 70,037 | -0.02% | -76.00% | 30 | 0% | -86.61% | 4,344 | 0% | -51.95% |
| [Render/strings](#renderstrings) | 582,647 | 0% | -72.22% | 344 | 0% | -83.73% | 9,088 | 0% | -49.31% |
| [Load/substitute](#loadsubstitute) | 7,710 | +0.01% | -74.18% | 6 | 0% | -83.78% | 832 | 0% | -45.26% |
| [Render/substitute](#rendersubstitute) | 2,233 | 0% | -74.66% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/substitute](#rendersinksubstitute) | 1,936 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/substitute](#rendertosubstitute) | 2,038 | 0% |  | 0 |  |  | 0 |  |  |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 742 | +2.20% | 732 | 0% | 0 |  | 7,283 | -0.49% |
| Render/chat_llama | 1,165 | -0.77% | 504 | -0.98% | 0 |  | 3,564 | -1.95% |
| Load/chat_mistral | 1,146 | +5.14% | 1,063 | +1.24% | 0 |  | 8,324 | +1.07% |
| Render/chat_mistral | 1,877 | -2.14% | 547 | +2.63% | 0 |  | 9,366 | -3.97% |
| Load/chat_qwen | 483 | -0.82% | 586 | +0.17% | 0 |  | 4,552 | +4.48% |
| Render/chat_qwen | 1,092 | -4.46% | 398 | -1.24% | 0 |  | 3,179 | -0.93% |
| Load/config_file | 186 | +5.68% | 156 | +2.63% | 0 |  | 2,637 | -0.90% |
| Render/config_file | 6,420 | -0.99% | 1,600 | -6.76% | 0 |  | 41,616 | -0.15% |
| Load/dict_ops | 36 | +5.88% | 21 | +23.53% | 0 |  | 1,299 | -0.08% |
| Render/dict_ops | 655 | -1.06% | 427 | +0.47% | 0 |  | 875 | +0.23% |
| Load/expressions | 42 | +2.44% | 30 | +25.00% | 0 |  | 1,241 | -5.05% |
| Render/expressions | 2 | 0% | 0 | -100.00% | 0 |  | 268 | -13.83% |
| Load/filters | 84 | -3.45% | 50 | -1.96% | 0 |  | 1,798 | +6.83% |
| Render/filters | 12 | -20.00% | 1 | -50.00% | 0 |  | 780 | +1.83% |
| Load/for_filter_if | 16 | 0% | 11 | +10.00% | 0 |  | 1,058 | +0.47% |
| Render/for_filter_if | 1,727 | -0.46% | 87 | -3.33% | 0 |  | 190 | +9.20% |
| Load/for_loop_vars | 45 | +9.76% | 29 | +16.00% | 0 |  | 1,086 | +1.21% |
| Render/for_loop_vars | 4 | -55.56% | 1 | 0% | 0 |  | 173 | +13.82% |
| Load/for_range | 7 | -12.50% | 5 | -28.57% | 0 |  | 740 | +4.67% |
| Render/for_range | 1 | 0% | 0 |  | 0 |  | 90 | -36.17% |
| Load/hf_deepseek_v31 | 960 | +0.95% | 964 | +0.10% | 0 |  | 6,199 | +7.57% |
| Render/hf_deepseek_v31 | 357 | -4.03% | 87 | +2.35% | 0 |  | 1,281 | -5.74% |
| Load/hf_gpt_oss | 4,556 | +0.66% | 3,246 | -0.06% | 0 |  | 21,438 | +6.44% |
| Render/hf_gpt_oss | 1,204 | -0.58% | 380 | 0% | 0 |  | 4,343 | -4.02% |
| Load/hf_mistral_small | 1,746 | +0.40% | 1,564 | -0.13% | 0 |  | 10,821 | +2.49% |
| Render/hf_mistral_small | 951 | +0.53% | 277 | +0.73% | 0 |  | 3,583 | -5.69% |
| Load/hf_qwen3 | 1,420 | +1.28% | 1,246 | +0.24% | 0 |  | 8,736 | +2.79% |
| Render/hf_qwen3 | 685 | -2.42% | 198 | +2.59% | 0 |  | 2,511 | -1.45% |
| Load/html_autoescape | 439 | +1.62% | 516 | +0.58% | 0 |  | 3,869 | +2.79% |
| Render/html_autoescape | 4,260 | -2.09% | 1,541 | +1.85% | 0 |  | 37,384 | +4.72% |
| Load/inheritance | 3 | 0% | 4 | 0% | 0 |  | 648 | +4.01% |
| Render/inheritance | 584 | 0% | 92 | +1.10% | 0 |  | 897 | +0.79% |
| Load/int_arith | 87 | +7.41% | 60 | -1.64% | 0 |  | 1,948 | -4.70% |
| Render/int_arith | 3 | 0% | 1 | 0% | 0 |  | 741 | +0.14% |
| Module/keep | 1 | 0% | 2 | 0% | 0 |  | 416 | +13.04% |
| Module/keep_written | 2,929 | +0.03% | 557 | -0.18% | 0 |  | 704 | -1.54% |
| Load/large_static | 1,929 | +0.05% | 913 | 0% | 0 |  | 263 | -9.62% |
| Render/large_static | 710 | 0% | 568 | 0% | 0 |  | 11 | -15.38% |
| RenderSink/large_static | 0 | -100.00% | 0 |  | 0 |  | 2 | 0% |
| RenderTo/large_static | 687 | 0% | 572 | 0% | 0 |  | 4 | 0% |
| Load/macros | 72 | +2.86% | 52 | +1.96% | 0 |  | 1,646 | +0.61% |
| Render/macros | 756 | -0.66% | 218 | +0.46% | 0 |  | 1,527 | -10.33% |
| Load/many_tags | 28,318 | -1.50% | 22,574 | -0.89% | 0 |  | 230,298 | +2.54% |
| Render/many_tags | 6,233 | +0.02% | 859 | +0.47% | 0 |  | 192 | +30.61% |
| Load/mitsuhiko_table | 21 | +10.53% | 21 | +16.67% | 0 |  | 852 | +4.80% |
| Render/mitsuhiko_table | 10,078 | -0.24% | 6,263 | +0.02% | 0 |  | 340 | 0% |
| RenderSink/mitsuhiko_table | 9,948 | -0.22% | 1,203 | -0.08% | 0 |  | 263 | -4.36% |
| RenderTo/mitsuhiko_table | 10,043 | -0.28% | 6,256 | -0.14% | 0 |  | 319 | -2.74% |
| Load/mitsuhiko_table_wide | 50 | +6.38% | 53 | 0% | 0 |  | 914 | +0.99% |
| Render/mitsuhiko_table_wide | 10,327 | -4.25% | 22,668 | -0.25% | 0 |  | 320 | -1.54% |
| RenderSink/mitsuhiko_table_wide | 10,107 | -0.44% | 1,294 | -1.52% | 0 |  | 265 | -1.49% |
| RenderTo/mitsuhiko_table_wide | 10,313 | -4.31% | 22,667 | -0.28% | 0 |  | 283 | -7.21% |
| Load/plain_text | 1 | 0% | 2 | 0% | 0 |  | 6 | -25.00% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 6 | 0% |
| RenderSink/plain_text | 0 |  | 0 |  | 0 |  | 3 | 0% |
| RenderTo/plain_text | 0 |  | 0 |  | 0 |  | 5 | -16.67% |
| Load/strings | 124 | -1.59% | 81 | +5.19% | 0 |  | 2,655 | -2.50% |
| Render/strings | 397 | -1.98% | 457 | 0% | 0 |  | 1,544 | -0.26% |
| Load/substitute | 2 | 0% | 3 | 0% | 0 |  | 131 | -33.50% |
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
