# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (256 so far, latest [a5eb623](https://github.com/jinja2cpp/Jinja2Cpp/commit/a5eb623e1263e72889807f2e8c27844b003ef6f5) on 2026-11-20). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 336,150 | -0.04% |  | 54 | 0% |  | 18,520 | 0% | -52.61% |
| [Render/chat_llama](#renderchat_llama) | 290,729 | 0% |  | 224 | 0% |  | 11,592 | 0% | -19.28% |
| [Load/chat_mistral](#loadchat_mistral) | 384,934 | -0.05% |  | 51 | 0% |  | 20,976 | 0% | -53.02% |
| [Render/chat_mistral](#renderchat_mistral) | 491,177 | +0.01% |  | 392 | 0% |  | 9,784 | 0% | -25.92% |
| [Load/chat_qwen](#loadchat_qwen) | 261,828 | -0.07% |  | 43 | 0% |  | 14,472 | 0% | -47.04% |
| [Render/chat_qwen](#renderchat_qwen) | 273,001 | 0% |  | 211 | 0% |  | 8,280 | 0% | -2.63% |
| [Load/config_file](#loadconfig_file) | 93,881 | -0.17% |  | 24 | 0% |  | 5,520 | 0% | -47.73% |
| [Render/config_file](#renderconfig_file) | 1,277,507 | 0% |  | 908 | 0% |  | 25,928 | 0% | +8.90% |
| [Load/dict_ops](#loaddict_ops) | 31,817 | -0.02% | -78.47% | 8 | 0% | -93.89% | 2,216 | 0% | -47.93% |
| [Render/dict_ops](#renderdict_ops) | 291,082 | 0% | -57.87% | 112 | 0% | -75.11% | 24,408 | 0% | -29.65% |
| [Load/expressions](#loadexpressions) | 43,784 | +0.02% | -78.76% | 15 | 0% | -89.73% | 2,752 | 0% | -55.15% |
| [Render/expressions](#renderexpressions) | 414,399 | 0% | -54.27% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 66,921 | -0.01% | -75.62% | 26 | 0% | -88.60% | 3,856 | 0% | -66.22% |
| [Render/filters](#renderfilters) | 26,519 | 0% | -70.09% | 23 | 0% | -68.92% | 752 | 0% | -84.69% |
| [Load/for_filter_if](#loadfor_filter_if) | 28,510 | -0.16% | -78.94% | 6 | 0% | -95.12% | 1,928 | 0% | -44.08% |
| [Render/for_filter_if](#renderfor_filter_if) | 267,157 | 0% | -69.13% | 4 | 0% | -98.23% | 16,560 | 0% | +354.95% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 38,304 | -0.15% | -78.80% | 9 | 0% | -94.27% | 2,392 | 0% | -47.08% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 170,796 | 0% | -84.08% | 2 | 0% | -99.62% | 1,568 | 0% | -43.35% |
| [Load/for_range](#loadfor_range) | 12,490 | +0.02% | -77.97% | 6 | 0% | -89.29% | 1,128 | 0% | -43.60% |
| [Render/for_range](#renderfor_range) | 37,181 | 0% | -59.68% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/hf_deepseek_v31](#loadhf_deepseek_v31) | 383,741 | -0.02% |  | 54 | 0% |  | 22,032 | 0% | +0.99% |
| [Render/hf_deepseek_v31](#renderhf_deepseek_v31) | 90,628 | 0% |  | 42 | 0% |  | 2,456 | 0% | -0.97% |
| [Load/hf_gpt_oss](#loadhf_gpt_oss) | 1,526,737 | -0.05% |  | 282 | 0% |  | 75,000 | 0% | +1.05% |
| [Render/hf_gpt_oss](#renderhf_gpt_oss) | 169,011 | 0% |  | 153 | 0% |  | 5,968 | 0% | +20.91% |
| [Load/hf_mistral_small](#loadhf_mistral_small) | 591,566 | -0.02% |  | 87 | 0% |  | 33,136 | 0% | +0.61% |
| [Render/hf_mistral_small](#renderhf_mistral_small) | 139,515 | +0.01% |  | 97 | 0% |  | 6,096 | 0% | -0.65% |
| [Load/hf_qwen3](#loadhf_qwen3) | 476,583 | -0.08% |  | 81 | 0% |  | 25,632 | 0% | +0.60% |
| [Render/hf_qwen3](#renderhf_qwen3) | 117,344 | 0% |  | 99 | 0% |  | 3,224 | 0% | -0.74% |
| [Load/html_autoescape](#loadhtml_autoescape) | 188,568 | -0.18% |  | 42 | 0% |  | 12,728 | 0% | -48.43% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,197,263 | 0% |  | 674 | 0% |  | 46,224 | 0% | +14.46% |
| [Load/inheritance](#loadinheritance) | 21,297 | +0.01% | -81.34% | 6 | 0% | -94.39% | 1,368 | 0% | -52.37% |
| [Render/inheritance](#renderinheritance) | 136,650 | 0% | -81.73% | 35 | 0% | -94.37% | 8,136 | 0% | +35.24% |
| [Load/int_arith](#loadint_arith) | 64,630 | -0.06% |  | 19 | 0% |  | 3,896 | 0% | +1.04% |
| [Render/int_arith](#renderint_arith) | 825,515 | 0% |  | 12 | 0% |  | 3,272 | 0% | -0.49% |
| [Module/keep](#modulekeep) | 6,949 | +0.10% |  | 6 | 0% |  |  |  |  |
| [Module/keep_written](#modulekeep_written) | 29,013 | +0.02% |  | 10 | 0% |  |  |  |  |
| [Load/large_static](#loadlarge_static) | 146,958 | +0.07% | -83.41% | 13 | 0% | -97.05% | 43,128 | 0% | -19.21% |
| [Render/large_static](#renderlarge_static) | 20,342 | 0% | -72.93% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [RenderSink/large_static](#rendersinklarge_static) | 12,631 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/large_static](#rendertolarge_static) | 19,905 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/macros](#loadmacros) | 53,998 | -0.04% | -76.31% | 25 | 0% | -86.56% | 4,472 | 0% | -28.06% |
| [Render/macros](#rendermacros) | 644,502 | 0% | -74.85% | 308 | 0% | -88.71% | 20,608 | 0% | +40.00% |
| [Load/many_tags](#loadmany_tags) | 8,751,945 | -0.12% | -80.18% | 950 | 0% | -97.23% | 438,120 | 0% | -58.48% |
| [Render/many_tags](#rendermany_tags) | 732,455 | 0% | -64.84% | 2 | 0% | -90.48% | 5,648 | 0% | -10.52% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 44,633 | +0.03% | -81.21% | 18 | 0% | -90.22% | 3,416 | 0% | -43.67% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,103,526 | 0% | -58.70% | 33 | 0% | -99.19% | 381,536 | 0% | +9.95% |
| [RenderSink/mitsuhiko_table](#rendersinkmitsuhiko_table) | 5,033,313 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table](#rendertomitsuhiko_table) | 5,103,003 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 52,372 | +0.03% |  | 34 | 0% |  | 5,744 | 0% | -30.49% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,449,155 | 0% |  | 33 | 0% |  | 1,414,928 | 0% | +2.46% |
| [RenderSink/mitsuhiko_table_wide](#rendersinkmitsuhiko_table_wide) | 5,275,082 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table_wide](#rendertomitsuhiko_table_wide) | 5,448,572 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Nested/new_template](#nestednew_template) | 451,974 | +0.02% |  | 323 | 0% |  |  |  |  |
| [Load/plain_text](#loadplain_text) | 2,932 | 0% | -69.70% | 3 | 0% | -83.33% | 680 | 0% | -39.29% |
| [Render/plain_text](#renderplain_text) | 972 | 0% | -85.69% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/plain_text](#rendersinkplain_text) | 677 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/plain_text](#rendertoplain_text) | 773 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/prompt_budget](#loadprompt_budget) | 34,163 | -0.08% |  | 17 | 0% |  | 2,248 | 0% | +1.08% |
| [Render/prompt_budget](#renderprompt_budget) | 1,322,091 | 0% |  | 76 | 0% |  | 36,416 | 0% | 0% |
| [Nested/shared_globals](#nestedshared_globals) | 160,014 | 0% |  | 160 | 0% |  |  |  |  |
| [Load/strings](#loadstrings) | 68,637 | -0.05% | -76.48% | 29 | 0% | -87.05% | 4,384 | 0% | -51.50% |
| [Render/strings](#renderstrings) | 580,970 | 0% | -72.30% | 344 | 0% | -83.73% | 9,088 | 0% | -49.31% |
| [Load/substitute](#loadsubstitute) | 6,654 | +0.06% | -77.71% | 5 | 0% | -86.49% | 840 | 0% | -44.74% |
| [Render/substitute](#rendersubstitute) | 1,653 | 0% | -81.24% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/substitute](#rendersinksubstitute) | 1,362 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/substitute](#rendertosubstitute) | 1,458 | 0% |  | 0 |  |  | 0 |  |  |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 728 | -0.82% | 755 | -0.79% | 0 |  | 7,898 | +0.95% |
| Render/chat_llama | 1,306 | +8.92% | 549 | +1.86% | 0 |  | 4,257 | +1.84% |
| Load/chat_mistral | 1,096 | 0% | 1,095 | +0.83% | 0 |  | 8,947 | +2.85% |
| Render/chat_mistral | 2,089 | +0.24% | 680 | -0.58% | 0 |  | 10,851 | +7.77% |
| Load/chat_qwen | 545 | +0.37% | 636 | -0.78% | 0 |  | 4,797 | +2.28% |
| Render/chat_qwen | 1,123 | -0.35% | 431 | -0.23% | 0 |  | 3,571 | +0.73% |
| Load/config_file | 203 | +6.28% | 193 | +0.52% | 0 |  | 2,706 | -3.25% |
| Render/config_file | 7,306 | +0.95% | 1,794 | 0% | 0 |  | 44,330 | +0.13% |
| Load/dict_ops | 33 | -8.33% | 24 | -4.00% | 0 |  | 1,364 | -0.94% |
| Render/dict_ops | 704 | +0.43% | 419 | -0.48% | 0 |  | 845 | +3.81% |
| Load/expressions | 15 | -6.25% | 15 | +7.14% | 0 |  | 1,314 | -1.72% |
| Render/expressions | 7 | +133.33% | 2 | +100.00% | 0 |  | 279 | -4.12% |
| Load/filters | 97 | +3.19% | 35 | -2.78% | 0 |  | 1,732 | -2.64% |
| Render/filters | 17 | +41.67% | 5 | 0% | 0 |  | 771 | +0.65% |
| Load/for_filter_if | 24 | +20.00% | 15 | -6.25% | 0 |  | 1,036 | +1.07% |
| Render/for_filter_if | 1,719 | -0.29% | 87 | -3.33% | 0 |  | 166 | +21.17% |
| Load/for_loop_vars | 23 | 0% | 18 | 0% | 0 |  | 1,106 | -1.07% |
| Render/for_loop_vars | 21 | +40.00% | 2 | -50.00% | 0 |  | 115 | +0.88% |
| Load/for_range | 2 | +100.00% | 3 | 0% | 0 |  | 741 | -2.37% |
| Render/for_range | 9 | +12.50% | 1 | 0% | 0 |  | 130 | 0% |
| Load/hf_deepseek_v31 | 1,015 | +1.20% | 1,007 | +0.90% | 0 |  | 6,844 | +5.05% |
| Render/hf_deepseek_v31 | 374 | -3.36% | 82 | 0% | 0 |  | 1,573 | +4.87% |
| Load/hf_gpt_oss | 4,675 | -0.17% | 3,387 | +0.06% | 0 |  | 23,969 | +6.58% |
| Render/hf_gpt_oss | 1,308 | -2.39% | 407 | +0.25% | 0 |  | 4,915 | +0.49% |
| Load/hf_mistral_small | 1,895 | +0.05% | 1,646 | +0.30% | 0 |  | 11,797 | +3.50% |
| Render/hf_mistral_small | 991 | +0.92% | 337 | -0.30% | 0 |  | 3,761 | -0.32% |
| Load/hf_qwen3 | 1,545 | +1.38% | 1,337 | +0.07% | 0 |  | 9,270 | +2.06% |
| Render/hf_qwen3 | 696 | -1.56% | 241 | +2.55% | 0 |  | 2,757 | +1.92% |
| Load/html_autoescape | 532 | -0.56% | 545 | -0.37% | 0 |  | 4,032 | +1.51% |
| Render/html_autoescape | 4,474 | +9.68% | 1,363 | +13.58% | 0 |  | 35,743 | -4.80% |
| Load/inheritance | 2 | +100.00% | 2 | -33.33% | 0 |  | 691 | +1.32% |
| Render/inheritance | 611 | +0.66% | 114 | -1.72% | 0 |  | 1,049 | -0.76% |
| Load/int_arith | 68 | +3.03% | 54 | +10.20% | 0 |  | 2,054 | +0.69% |
| Render/int_arith | 20 | +17.65% | 12 | +9.09% | 0 |  | 1,345 | +79.57% |
| Module/keep | 1 | -50.00% | 3 | +50.00% | 0 |  | 471 | +17.16% |
| Module/keep_written | 1,142 | 0% | 564 | -0.35% | 0 |  | 743 | +1.50% |
| Load/large_static | 1,917 | +0.05% | 910 | 0% | 0 |  | 253 | +4.98% |
| Render/large_static | 700 | 0% | 568 | 0% | 0 |  | 4 | +33.33% |
| RenderSink/large_static | 1 | 0% | 0 |  | 0 |  | 3 | 0% |
| RenderTo/large_static | 680 | 0% | 569 | 0% | 0 |  | 3 | +50.00% |
| Load/macros | 78 | +14.71% | 71 | +14.52% | 0 |  | 1,690 | -0.94% |
| Render/macros | 809 | -0.74% | 255 | +1.19% | 0 |  | 1,032 | +25.39% |
| Load/many_tags | 28,169 | +0.33% | 22,638 | -0.08% | 0 |  | 236,962 | -1.59% |
| Render/many_tags | 6,080 | -0.16% | 795 | +0.51% | 0 |  | 91 | -8.08% |
| Load/mitsuhiko_table | 30 | -6.25% | 43 | -4.44% | 0 |  | 876 | -0.34% |
| Render/mitsuhiko_table | 10,097 | +0.36% | 6,248 | -0.02% | 0 |  | 310 | +5.80% |
| RenderSink/mitsuhiko_table | 10,049 | +0.21% | 1,189 | +0.34% | 0 |  | 257 | -2.28% |
| RenderTo/mitsuhiko_table | 10,104 | +0.13% | 6,247 | -0.02% | 0 |  | 286 | +3.62% |
| Load/mitsuhiko_table_wide | 80 | +6.67% | 93 | +4.49% | 0 |  | 1,016 | +5.28% |
| Render/mitsuhiko_table_wide | 10,268 | +0.45% | 22,715 | +0.03% | 0 |  | 296 | -8.36% |
| RenderSink/mitsuhiko_table_wide | 10,092 | +0.32% | 1,272 | +0.63% | 0 |  | 254 | +0.79% |
| RenderTo/mitsuhiko_table_wide | 10,256 | +0.34% | 22,655 | 0% | 0 |  | 284 | -4.38% |
| Nested/new_template | 2,347 | +2.27% | 1,676 | +1.09% | 0 |  | 24,319 | -0.05% |
| Load/plain_text | 0 |  | 1 | 0% | 0 |  | 3 | +50.00% |
| Render/plain_text | 1 | 0% | 0 |  | 0 |  | 7 | +133.33% |
| RenderSink/plain_text | 1 | 0% | 0 |  | 0 |  | 5 | +150.00% |
| RenderTo/plain_text | 2 | 0% | 0 |  | 0 |  | 7 | +133.33% |
| Load/prompt_budget | 75 | -3.85% | 62 | -6.06% | 0 |  | 1,416 | -1.53% |
| Render/prompt_budget | 4,790 | -0.13% | 554 | +0.18% | 0 |  | 999 | +6.73% |
| Nested/shared_globals | 263 | +57.49% | 117 | +20.62% | 0 |  | 9,347 | -0.48% |
| Load/strings | 121 | -13.57% | 117 | +2.63% | 0 |  | 2,860 | +1.56% |
| Render/strings | 415 | +1.22% | 488 | -0.81% | 0 |  | 1,612 | +1.70% |
| Load/substitute | 1 | 0% | 2 | 0% | 0 |  | 117 | +10.38% |
| Render/substitute | 1 | 0% | 0 |  | 0 |  | 3 | -72.73% |
| RenderSink/substitute | 1 | 0% | 0 |  | 0 |  | 3 | 0% |
| RenderTo/substitute | 1 | 0% | 0 |  | 0 |  | 3 | +50.00% |

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
