# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (257 so far, latest [6274c3b](https://github.com/jinja2cpp/Jinja2Cpp/commit/6274c3b8eec5a39868e7c4827d7aed218566039e) on 2026-11-21). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 336,162 | 0% |  | 54 | 0% |  | 18,520 | 0% | -52.61% |
| [Render/chat_llama](#renderchat_llama) | 290,729 | 0% |  | 224 | 0% |  | 11,592 | 0% | -19.28% |
| [Load/chat_mistral](#loadchat_mistral) | 384,949 | 0% |  | 51 | 0% |  | 20,976 | 0% | -53.02% |
| [Render/chat_mistral](#renderchat_mistral) | 491,285 | +0.02% |  | 392 | 0% |  | 9,784 | 0% | -25.92% |
| [Load/chat_qwen](#loadchat_qwen) | 261,837 | 0% |  | 43 | 0% |  | 14,472 | 0% | -47.04% |
| [Render/chat_qwen](#renderchat_qwen) | 273,001 | 0% |  | 211 | 0% |  | 8,280 | 0% | -2.63% |
| [Load/config_file](#loadconfig_file) | 93,890 | +0.01% |  | 24 | 0% |  | 5,520 | 0% | -47.73% |
| [Render/config_file](#renderconfig_file) | 1,277,614 | +0.01% |  | 908 | 0% |  | 25,928 | 0% | +8.90% |
| [Load/dict_ops](#loaddict_ops) | 31,823 | +0.02% | -78.47% | 8 | 0% | -93.89% | 2,216 | 0% | -47.93% |
| [Render/dict_ops](#renderdict_ops) | 291,082 | 0% | -57.87% | 112 | 0% | -75.11% | 24,408 | 0% | -29.65% |
| [Load/expressions](#loadexpressions) | 43,787 | +0.01% | -78.76% | 15 | 0% | -89.73% | 2,752 | 0% | -55.15% |
| [Render/expressions](#renderexpressions) | 414,399 | 0% | -54.27% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 66,921 | 0% | -75.62% | 26 | 0% | -88.60% | 3,856 | 0% | -66.22% |
| [Render/filters](#renderfilters) | 26,519 | 0% | -70.09% | 23 | 0% | -68.92% | 752 | 0% | -84.69% |
| [Load/for_filter_if](#loadfor_filter_if) | 28,513 | +0.01% | -78.94% | 6 | 0% | -95.12% | 1,928 | 0% | -44.08% |
| [Render/for_filter_if](#renderfor_filter_if) | 267,157 | 0% | -69.13% | 4 | 0% | -98.23% | 16,560 | 0% | +354.95% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 38,307 | +0.01% | -78.80% | 9 | 0% | -94.27% | 2,392 | 0% | -47.08% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 170,796 | 0% | -84.08% | 2 | 0% | -99.62% | 1,568 | 0% | -43.35% |
| [Load/for_range](#loadfor_range) | 12,493 | +0.02% | -77.96% | 6 | 0% | -89.29% | 1,128 | 0% | -43.60% |
| [Render/for_range](#renderfor_range) | 37,181 | 0% | -59.68% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/hf_deepseek_v31](#loadhf_deepseek_v31) | 383,750 | 0% |  | 54 | 0% |  | 22,032 | 0% | +0.99% |
| [Render/hf_deepseek_v31](#renderhf_deepseek_v31) | 90,628 | 0% |  | 42 | 0% |  | 2,456 | 0% | -0.97% |
| [Load/hf_gpt_oss](#loadhf_gpt_oss) | 1,526,761 | 0% |  | 282 | 0% |  | 75,000 | 0% | +1.05% |
| [Render/hf_gpt_oss](#renderhf_gpt_oss) | 169,034 | +0.01% |  | 153 | 0% |  | 5,968 | 0% | +20.91% |
| [Load/hf_mistral_small](#loadhf_mistral_small) | 591,581 | 0% |  | 87 | 0% |  | 33,136 | 0% | +0.61% |
| [Render/hf_mistral_small](#renderhf_mistral_small) | 139,515 | 0% |  | 97 | 0% |  | 6,096 | 0% | -0.65% |
| [Load/hf_qwen3](#loadhf_qwen3) | 476,595 | 0% |  | 81 | 0% |  | 25,632 | 0% | +0.60% |
| [Render/hf_qwen3](#renderhf_qwen3) | 117,344 | 0% |  | 99 | 0% |  | 3,224 | 0% | -0.74% |
| [Load/html_autoescape](#loadhtml_autoescape) | 188,577 | 0% |  | 42 | 0% |  | 12,728 | 0% | -48.43% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,197,326 | +0.01% |  | 674 | 0% |  | 46,224 | 0% | +14.46% |
| [Load/inheritance](#loadinheritance) | 21,300 | +0.01% | -81.34% | 6 | 0% | -94.39% | 1,368 | 0% | -52.37% |
| [Render/inheritance](#renderinheritance) | 136,653 | 0% | -81.73% | 35 | 0% | -94.37% | 8,136 | 0% | +35.24% |
| [Load/int_arith](#loadint_arith) | 64,633 | 0% |  | 19 | 0% |  | 3,896 | 0% | +1.04% |
| [Render/int_arith](#renderint_arith) | 825,515 | 0% |  | 12 | 0% |  | 3,272 | 0% | -0.49% |
| [Module/keep](#modulekeep) | 6,949 | 0% |  | 6 | 0% |  |  |  |  |
| [Module/keep_written](#modulekeep_written) | 29,013 | 0% |  | 10 | 0% |  |  |  |  |
| [Load/large_static](#loadlarge_static) | 146,958 | 0% | -83.41% | 13 | 0% | -97.05% | 43,128 | 0% | -19.21% |
| [Render/large_static](#renderlarge_static) | 20,342 | 0% | -72.93% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [RenderSink/large_static](#rendersinklarge_static) | 12,631 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/large_static](#rendertolarge_static) | 19,905 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/macros](#loadmacros) | 54,001 | +0.01% | -76.31% | 25 | 0% | -86.56% | 4,472 | 0% | -28.06% |
| [Render/macros](#rendermacros) | 644,502 | 0% | -74.85% | 308 | 0% | -88.71% | 20,608 | 0% | +40.00% |
| [Load/many_tags](#loadmany_tags) | 8,751,945 | 0% | -80.18% | 950 | 0% | -97.23% | 438,120 | 0% | -58.48% |
| [Render/many_tags](#rendermany_tags) | 732,455 | 0% | -64.84% | 2 | 0% | -90.48% | 5,648 | 0% | -10.52% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 44,642 | +0.02% | -81.21% | 18 | 0% | -90.22% | 3,416 | 0% | -43.67% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,103,544 | 0% | -58.70% | 33 | 0% | -99.19% | 381,536 | 0% | +9.95% |
| [RenderSink/mitsuhiko_table](#rendersinkmitsuhiko_table) | 5,033,331 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table](#rendertomitsuhiko_table) | 5,103,021 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 52,381 | +0.02% |  | 34 | 0% |  | 5,744 | 0% | -30.49% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,449,173 | 0% |  | 33 | 0% |  | 1,414,928 | 0% | +2.46% |
| [RenderSink/mitsuhiko_table_wide](#rendersinkmitsuhiko_table_wide) | 5,275,100 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table_wide](#rendertomitsuhiko_table_wide) | 5,448,590 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Nested/new_template](#nestednew_template) | 451,931 | -0.01% |  | 323 | 0% |  |  |  |  |
| [Load/plain_text](#loadplain_text) | 2,932 | 0% | -69.70% | 3 | 0% | -83.33% | 680 | 0% | -39.29% |
| [Render/plain_text](#renderplain_text) | 972 | 0% | -85.69% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/plain_text](#rendersinkplain_text) | 677 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/plain_text](#rendertoplain_text) | 773 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/prompt_budget](#loadprompt_budget) | 34,166 | +0.01% |  | 17 | 0% |  | 2,248 | 0% | +1.08% |
| [Render/prompt_budget](#renderprompt_budget) | 1,322,091 | 0% |  | 76 | 0% |  | 36,416 | 0% | 0% |
| [Nested/shared_globals](#nestedshared_globals) | 160,094 | +0.05% |  | 160 | 0% |  |  |  |  |
| [Load/strings](#loadstrings) | 68,643 | +0.01% | -76.48% | 29 | 0% | -87.05% | 4,384 | 0% | -51.50% |
| [Render/strings](#renderstrings) | 580,970 | 0% | -72.30% | 344 | 0% | -83.73% | 9,088 | 0% | -49.31% |
| [Load/substitute](#loadsubstitute) | 6,654 | 0% | -77.71% | 5 | 0% | -86.49% | 840 | 0% | -44.74% |
| [Render/substitute](#rendersubstitute) | 1,653 | 0% | -81.24% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/substitute](#rendersinksubstitute) | 1,362 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/substitute](#rendertosubstitute) | 1,458 | 0% |  | 0 |  |  | 0 |  |  |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 723 | -0.69% | 754 | -0.13% | 0 |  | 7,866 | -0.41% |
| Render/chat_llama | 1,295 | -0.84% | 548 | -0.18% | 0 |  | 4,242 | -0.35% |
| Load/chat_mistral | 1,093 | -0.27% | 1,095 | 0% | 0 |  | 8,772 | -1.96% |
| Render/chat_mistral | 2,107 | +0.86% | 672 | -1.18% | 0 |  | 10,393 | -4.22% |
| Load/chat_qwen | 537 | -1.47% | 636 | 0% | 0 |  | 4,696 | -2.11% |
| Render/chat_qwen | 1,117 | -0.53% | 431 | 0% | 0 |  | 3,442 | -3.61% |
| Load/config_file | 203 | 0% | 191 | -1.04% | 0 |  | 2,721 | +0.55% |
| Render/config_file | 7,354 | +0.66% | 1,823 | +1.62% | 0 |  | 44,502 | +0.39% |
| Load/dict_ops | 33 | 0% | 25 | +4.17% | 0 |  | 1,357 | -0.51% |
| Render/dict_ops | 704 | 0% | 419 | 0% | 0 |  | 839 | -0.71% |
| Load/expressions | 16 | +6.67% | 15 | 0% | 0 |  | 1,315 | +0.08% |
| Render/expressions | 6 | -14.29% | 1 | -50.00% | 0 |  | 302 | +8.24% |
| Load/filters | 101 | +4.12% | 37 | +5.71% | 0 |  | 1,533 | -11.49% |
| Render/filters | 13 | -23.53% | 4 | -20.00% | 0 |  | 775 | +0.52% |
| Load/for_filter_if | 24 | 0% | 15 | 0% | 0 |  | 1,029 | -0.68% |
| Render/for_filter_if | 1,718 | -0.06% | 87 | 0% | 0 |  | 163 | -1.81% |
| Load/for_loop_vars | 23 | 0% | 18 | 0% | 0 |  | 1,073 | -2.98% |
| Render/for_loop_vars | 18 | -14.29% | 2 | 0% | 0 |  | 115 | 0% |
| Load/for_range | 2 | 0% | 3 | 0% | 0 |  | 768 | +3.64% |
| Render/for_range | 9 | 0% | 1 | 0% | 0 |  | 141 | +8.46% |
| Load/hf_deepseek_v31 | 1,008 | -0.69% | 1,006 | -0.10% | 0 |  | 6,761 | -1.21% |
| Render/hf_deepseek_v31 | 370 | -1.07% | 82 | 0% | 0 |  | 1,394 | -11.38% |
| Load/hf_gpt_oss | 4,669 | -0.13% | 3,386 | -0.03% | 0 |  | 22,959 | -4.21% |
| Render/hf_gpt_oss | 1,319 | +0.84% | 405 | -0.49% | 0 |  | 4,847 | -1.38% |
| Load/hf_mistral_small | 1,886 | -0.47% | 1,640 | -0.36% | 0 |  | 11,449 | -2.95% |
| Render/hf_mistral_small | 986 | -0.50% | 337 | 0% | 0 |  | 3,756 | -0.13% |
| Load/hf_qwen3 | 1,528 | -1.10% | 1,336 | -0.07% | 0 |  | 9,026 | -2.63% |
| Render/hf_qwen3 | 691 | -0.72% | 241 | 0% | 0 |  | 2,651 | -3.84% |
| Load/html_autoescape | 529 | -0.56% | 546 | +0.18% | 0 |  | 3,872 | -3.97% |
| Render/html_autoescape | 4,488 | +0.31% | 1,396 | +2.42% | 0 |  | 37,454 | +4.79% |
| Load/inheritance | 2 | 0% | 2 | 0% | 0 |  | 694 | +0.43% |
| Render/inheritance | 611 | 0% | 114 | 0% | 0 |  | 1,041 | -0.76% |
| Load/int_arith | 68 | 0% | 55 | +1.85% | 0 |  | 2,043 | -0.54% |
| Render/int_arith | 18 | -10.00% | 11 | -8.33% | 0 |  | 768 | -42.90% |
| Module/keep | 1 | 0% | 3 | 0% | 0 |  | 417 | -11.46% |
| Module/keep_written | 1,142 | 0% | 564 | 0% | 0 |  | 751 | +1.08% |
| Load/large_static | 1,916 | -0.05% | 910 | 0% | 0 |  | 249 | -1.58% |
| Render/large_static | 700 | 0% | 568 | 0% | 0 |  | 4 | 0% |
| RenderSink/large_static | 1 | 0% | 0 |  | 0 |  | 2 | -33.33% |
| RenderTo/large_static | 680 | 0% | 569 | 0% | 0 |  | 3 | 0% |
| Load/macros | 76 | -2.56% | 69 | -2.82% | 0 |  | 1,723 | +1.95% |
| Render/macros | 808 | -0.12% | 257 | +0.78% | 0 |  | 631 | -38.86% |
| Load/many_tags | 28,062 | -0.38% | 22,609 | -0.13% | 0 |  | 233,111 | -1.63% |
| Render/many_tags | 6,094 | +0.23% | 793 | -0.25% | 0 |  | 104 | +14.29% |
| Load/mitsuhiko_table | 31 | +3.33% | 44 | +2.33% | 0 |  | 880 | +0.46% |
| Render/mitsuhiko_table | 10,094 | -0.03% | 6,247 | -0.02% | 0 |  | 283 | -8.71% |
| RenderSink/mitsuhiko_table | 10,048 | -0.01% | 1,188 | -0.08% | 0 |  | 226 | -12.06% |
| RenderTo/mitsuhiko_table | 10,104 | 0% | 6,248 | +0.02% | 0 |  | 250 | -12.59% |
| Load/mitsuhiko_table_wide | 79 | -1.25% | 95 | +2.15% | 0 |  | 1,049 | +3.25% |
| Render/mitsuhiko_table_wide | 10,262 | -0.06% | 22,714 | 0% | 0 |  | 294 | -0.68% |
| RenderSink/mitsuhiko_table_wide | 10,093 | +0.01% | 1,269 | -0.24% | 0 |  | 190 | -25.20% |
| RenderTo/mitsuhiko_table_wide | 10,265 | +0.09% | 22,653 | -0.01% | 0 |  | 259 | -8.80% |
| Nested/new_template | 2,288 | -2.51% | 1,698 | +1.31% | 0 |  | 24,516 | +0.81% |
| Load/plain_text | 0 |  | 1 | 0% | 0 |  | 3 | 0% |
| Render/plain_text | 1 | 0% | 0 |  | 0 |  | 7 | 0% |
| RenderSink/plain_text | 1 | 0% | 0 |  | 0 |  | 5 | 0% |
| RenderTo/plain_text | 2 | 0% | 0 |  | 0 |  | 7 | 0% |
| Load/prompt_budget | 73 | -2.67% | 62 | 0% | 0 |  | 1,401 | -1.06% |
| Render/prompt_budget | 4,789 | -0.02% | 554 | 0% | 0 |  | 1,130 | +13.11% |
| Nested/shared_globals | 256 | -2.66% | 114 | -2.56% | 0 |  | 9,366 | +0.20% |
| Load/strings | 122 | +0.83% | 116 | -0.85% | 0 |  | 2,790 | -2.45% |
| Render/strings | 422 | +1.69% | 488 | 0% | 0 |  | 1,574 | -2.36% |
| Load/substitute | 1 | 0% | 2 | 0% | 0 |  | 90 | -23.08% |
| Render/substitute | 1 | 0% | 0 |  | 0 |  | 3 | 0% |
| RenderSink/substitute | 1 | 0% | 0 |  | 0 |  | 3 | 0% |
| RenderTo/substitute | 1 | 0% | 0 |  | 0 |  | 3 | 0% |

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

### Nested/new_template

![Nested/new_template](charts/Nested_new_template.svg)

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

### Nested/shared_globals

![Nested/shared_globals](charts/Nested_shared_globals.svg)

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
