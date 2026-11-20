# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (254 so far, latest [5f95ec8](https://github.com/jinja2cpp/Jinja2Cpp/commit/5f95ec8cd1227783f72a9313e515796984816180) on 2026-11-20). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 336,280 | 0% |  | 54 | 0% |  | 18,520 | 0% | -52.61% |
| [Render/chat_llama](#renderchat_llama) | 290,729 | 0% |  | 224 | 0% |  | 11,592 | 0% | -19.28% |
| [Load/chat_mistral](#loadchat_mistral) | 385,143 | 0% |  | 51 | 0% |  | 20,976 | 0% | -53.02% |
| [Render/chat_mistral](#renderchat_mistral) | 491,129 | +0.02% |  | 392 | 0% |  | 9,784 | 0% | -25.92% |
| [Load/chat_qwen](#loadchat_qwen) | 262,024 | 0% |  | 43 | 0% |  | 14,472 | 0% | -47.04% |
| [Render/chat_qwen](#renderchat_qwen) | 273,001 | 0% |  | 211 | 0% |  | 8,280 | 0% | -2.63% |
| [Load/config_file](#loadconfig_file) | 94,044 | 0% |  | 24 | 0% |  | 5,520 | 0% | -47.73% |
| [Render/config_file](#renderconfig_file) | 1,277,507 | +0.02% |  | 908 | 0% |  | 25,928 | 0% | +8.90% |
| [Load/dict_ops](#loaddict_ops) | 31,824 | 0% | -78.46% | 8 | 0% | -93.89% | 2,216 | 0% | -47.93% |
| [Render/dict_ops](#renderdict_ops) | 291,082 | 0% | -57.87% | 112 | 0% | -75.11% | 24,408 | 0% | -29.65% |
| [Load/expressions](#loadexpressions) | 43,770 | 0% | -78.77% | 15 | 0% | -89.73% | 2,752 | 0% | -55.15% |
| [Render/expressions](#renderexpressions) | 414,399 | 0% | -54.27% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 66,926 | 0% | -75.61% | 26 | 0% | -88.60% | 3,856 | 0% | -66.22% |
| [Render/filters](#renderfilters) | 26,519 | 0% | -70.09% | 23 | 0% | -68.92% | 752 | 0% | -84.69% |
| [Load/for_filter_if](#loadfor_filter_if) | 28,556 | 0% | -78.91% | 6 | 0% | -95.12% | 1,928 | 0% | -44.08% |
| [Render/for_filter_if](#renderfor_filter_if) | 267,157 | 0% | -69.13% | 4 | 0% | -98.23% | 16,560 | 0% | +354.95% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 38,361 | 0% | -78.77% | 9 | 0% | -94.27% | 2,392 | 0% | -47.08% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 170,796 | 0% | -84.08% | 2 | 0% | -99.62% | 1,568 | 0% | -43.35% |
| [Load/for_range](#loadfor_range) | 12,487 | 0% | -77.97% | 6 | 0% | -89.29% | 1,128 | 0% | -43.60% |
| [Render/for_range](#renderfor_range) | 37,181 | 0% | -59.68% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/hf_deepseek_v31](#loadhf_deepseek_v31) | 383,807 | 0% |  | 54 | 0% |  | 22,032 | 0% | +0.99% |
| [Render/hf_deepseek_v31](#renderhf_deepseek_v31) | 90,628 | +0.29% |  | 42 | 0% |  | 2,456 | 0% | -0.97% |
| [Load/hf_gpt_oss](#loadhf_gpt_oss) | 1,527,562 | 0% |  | 282 | 0% |  | 75,000 | 0% | +1.05% |
| [Render/hf_gpt_oss](#renderhf_gpt_oss) | 169,008 | +0.06% |  | 153 | 0% |  | 5,968 | +0.54% | +20.91% |
| [Load/hf_mistral_small](#loadhf_mistral_small) | 591,680 | 0% |  | 87 | 0% |  | 33,136 | 0% | +0.61% |
| [Render/hf_mistral_small](#renderhf_mistral_small) | 139,501 | +0.04% |  | 97 | 0% |  | 6,096 | 0% | -0.65% |
| [Load/hf_qwen3](#loadhf_qwen3) | 476,980 | 0% |  | 81 | 0% |  | 25,632 | 0% | +0.60% |
| [Render/hf_qwen3](#renderhf_qwen3) | 117,344 | +0.10% |  | 99 | 0% |  | 3,224 | 0% | -0.74% |
| [Load/html_autoescape](#loadhtml_autoescape) | 188,917 | 0% |  | 42 | 0% |  | 12,728 | 0% | -48.43% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,197,263 | 0% |  | 674 | 0% |  | 46,224 | 0% | +14.46% |
| [Load/inheritance](#loadinheritance) | 21,295 | 0% | -81.34% | 6 | 0% | -94.39% | 1,368 | 0% | -52.37% |
| [Render/inheritance](#renderinheritance) | 136,650 | 0% | -81.73% | 35 | 0% | -94.37% | 8,136 | 0% | +35.24% |
| [Load/int_arith](#loadint_arith) | 64,671 | 0% |  | 19 | 0% |  | 3,896 | 0% | +1.04% |
| [Render/int_arith](#renderint_arith) | 825,515 | -0.06% |  | 12 | 0% |  | 3,272 | 0% | -0.49% |
| [Module/keep](#modulekeep) | 6,942 | 0% |  | 6 | 0% |  |  |  |  |
| [Module/keep_written](#modulekeep_written) | 29,006 | +0.01% |  | 10 | 0% |  |  |  |  |
| [Load/large_static](#loadlarge_static) | 146,858 | 0% | -83.42% | 13 | 0% | -97.05% | 43,128 | 0% | -19.21% |
| [Render/large_static](#renderlarge_static) | 20,342 | 0% | -72.93% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [RenderSink/large_static](#rendersinklarge_static) | 12,631 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/large_static](#rendertolarge_static) | 19,905 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/macros](#loadmacros) | 54,022 | 0% | -76.30% | 25 | 0% | -86.56% | 4,472 | 0% | -28.06% |
| [Render/macros](#rendermacros) | 644,502 | 0% | -74.85% | 308 | 0% | -88.71% | 20,608 | 0% | +40.00% |
| [Load/many_tags](#loadmany_tags) | 8,762,455 | 0% | -80.16% | 950 | 0% | -97.23% | 438,120 | 0% | -58.48% |
| [Render/many_tags](#rendermany_tags) | 732,455 | 0% | -64.84% | 2 | 0% | -90.48% | 5,648 | 0% | -10.52% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 44,619 | 0% | -81.22% | 18 | 0% | -90.22% | 3,416 | 0% | -43.67% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,103,526 | 0% | -58.70% | 33 | 0% | -99.19% | 381,536 | 0% | +9.95% |
| [RenderSink/mitsuhiko_table](#rendersinkmitsuhiko_table) | 5,033,313 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table](#rendertomitsuhiko_table) | 5,103,003 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 52,358 | 0% |  | 34 | 0% |  | 5,744 | 0% | -30.49% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,449,155 | 0% |  | 33 | 0% |  | 1,414,928 | 0% | +2.46% |
| [RenderSink/mitsuhiko_table_wide](#rendersinkmitsuhiko_table_wide) | 5,275,082 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table_wide](#rendertomitsuhiko_table_wide) | 5,448,572 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Nested/new_template](#nestednew_template) | 451,904 | 0% |  | 323 | 0% |  |  |  |  |
| [Load/plain_text](#loadplain_text) | 2,932 | 0% | -69.70% | 3 | 0% | -83.33% | 680 | 0% | -39.29% |
| [Render/plain_text](#renderplain_text) | 972 | 0% | -85.69% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/plain_text](#rendersinkplain_text) | 677 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/plain_text](#rendertoplain_text) | 773 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/prompt_budget](#loadprompt_budget) | 34,190 | 0% |  | 17 | 0% |  | 2,248 | 0% | +1.08% |
| [Render/prompt_budget](#renderprompt_budget) | 1,322,091 | 0% |  | 76 | 0% |  | 36,416 | 0% | 0% |
| [Nested/shared_globals](#nestedshared_globals) | 160,014 | 0% |  | 160 | 0% |  |  |  |  |
| [Load/strings](#loadstrings) | 68,672 | 0% | -76.47% | 29 | 0% | -87.05% | 4,384 | 0% | -51.50% |
| [Render/strings](#renderstrings) | 580,986 | -0.04% | -72.30% | 344 | 0% | -83.73% | 9,088 | 0% | -49.31% |
| [Load/substitute](#loadsubstitute) | 6,650 | 0% | -77.73% | 5 | 0% | -86.49% | 840 | 0% | -44.74% |
| [Render/substitute](#rendersubstitute) | 1,653 | 0% | -81.24% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/substitute](#rendersinksubstitute) | 1,362 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/substitute](#rendertosubstitute) | 1,458 | 0% |  | 0 |  |  | 0 |  |  |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 743 | -0.13% | 759 | 0% | 0 |  | 7,868 | +1.59% |
| Render/chat_llama | 1,182 | -0.17% | 547 | 0% | 0 |  | 4,329 | -6.64% |
| Load/chat_mistral | 1,106 | -0.09% | 1,089 | 0% | 0 |  | 8,957 | +2.07% |
| Render/chat_mistral | 2,118 | -0.61% | 685 | +0.59% | 0 |  | 10,600 | -3.59% |
| Load/chat_qwen | 545 | 0% | 647 | +0.15% | 0 |  | 4,733 | +1.28% |
| Render/chat_qwen | 1,135 | 0% | 437 | 0% | 0 |  | 3,585 | +3.61% |
| Load/config_file | 192 | -0.52% | 191 | 0% | 0 |  | 2,782 | +1.68% |
| Render/config_file | 7,149 | 0% | 1,803 | 0% | 0 |  | 45,037 | +4.39% |
| Load/dict_ops | 35 | 0% | 27 | 0% | 0 |  | 1,380 | +1.10% |
| Render/dict_ops | 700 | 0% | 421 | 0% | 0 |  | 850 | +4.94% |
| Load/expressions | 16 | 0% | 14 | 0% | 0 |  | 1,385 | +1.54% |
| Render/expressions | 2 | 0% | 0 |  | 0 |  | 342 | +15.54% |
| Load/filters | 91 | 0% | 38 | 0% | 0 |  | 1,881 | +13.66% |
| Render/filters | 12 | 0% | 5 | 0% | 0 |  | 771 | +3.35% |
| Load/for_filter_if | 25 | 0% | 15 | 0% | 0 |  | 1,066 | +0.66% |
| Render/for_filter_if | 1,722 | 0% | 88 | 0% | 0 |  | 150 | +22.95% |
| Load/for_loop_vars | 27 | 0% | 23 | 0% | 0 |  | 1,108 | +1.28% |
| Render/for_loop_vars | 9 | 0% | 1 | 0% | 0 |  | 90 | -7.22% |
| Load/for_range | 1 | 0% | 3 | 0% | 0 |  | 775 | +3.89% |
| Render/for_range | 8 | 0% | 1 | 0% | 0 |  | 145 | +10.69% |
| Load/hf_deepseek_v31 | 1,003 | -0.10% | 997 | 0% | 0 |  | 6,600 | +1.40% |
| Render/hf_deepseek_v31 | 383 | +0.26% | 83 | 0% | 0 |  | 1,493 | -8.18% |
| Load/hf_gpt_oss | 4,700 | -0.04% | 3,384 | 0% | 0 |  | 22,354 | -3.74% |
| Render/hf_gpt_oss | 1,344 | +0.67% | 408 | +0.99% | 0 |  | 4,918 | +0.33% |
| Load/hf_mistral_small | 1,905 | -0.16% | 1,648 | 0% | 0 |  | 11,402 | +2.78% |
| Render/hf_mistral_small | 974 | +0.10% | 336 | 0% | 0 |  | 3,861 | +0.68% |
| Load/hf_qwen3 | 1,538 | 0% | 1,343 | 0% | 0 |  | 9,290 | +1.76% |
| Render/hf_qwen3 | 703 | +0.29% | 240 | +0.42% | 0 |  | 2,754 | +0.58% |
| Load/html_autoescape | 539 | +0.19% | 547 | 0% | 0 |  | 4,076 | +3.24% |
| Render/html_autoescape | 4,168 | +0.31% | 1,271 | +0.16% | 0 |  | 37,359 | -2.31% |
| Load/inheritance | 1 | 0% | 3 | 0% | 0 |  | 694 | +3.43% |
| Render/inheritance | 610 | 0% | 120 | 0% | 0 |  | 1,034 | +0.49% |
| Load/int_arith | 68 | 0% | 59 | 0% | 0 |  | 2,122 | +1.68% |
| Render/int_arith | 15 | 0% | 13 | 0% | 0 |  | 799 | +6.25% |
| Module/keep | 2 | 0% | 2 | 0% | 0 |  | 434 | +12.14% |
| Module/keep_written | 1,142 | 0% | 566 | 0% | 0 |  | 742 | +1.92% |
| Load/large_static | 1,916 | 0% | 911 | 0% | 0 |  | 251 | -7.38% |
| Render/large_static | 700 | 0% | 568 | 0% | 0 |  | 11 | +83.33% |
| RenderSink/large_static | 1 | 0% | 0 |  | 0 |  | 2 | 0% |
| RenderTo/large_static | 680 | 0% | 569 | 0% | 0 |  | 3 | -25.00% |
| Load/macros | 64 | 0% | 64 | 0% | 0 |  | 1,733 | +2.24% |
| Render/macros | 791 | 0% | 236 | 0% | 0 |  | 1,718 | +6.58% |
| Load/many_tags | 28,486 | 0% | 22,691 | 0% | 0 |  | 251,399 | +6.22% |
| Render/many_tags | 6,101 | 0% | 790 | 0% | 0 |  | 87 | -24.35% |
| Load/mitsuhiko_table | 32 | 0% | 42 | 0% | 0 |  | 892 | +2.88% |
| Render/mitsuhiko_table | 10,051 | 0% | 6,248 | 0% | 0 |  | 321 | +10.69% |
| RenderSink/mitsuhiko_table | 10,019 | 0% | 1,188 | 0% | 0 |  | 290 | +16.47% |
| RenderTo/mitsuhiko_table | 10,080 | 0% | 6,252 | 0% | 0 |  | 298 | +11.61% |
| Load/mitsuhiko_table_wide | 75 | 0% | 92 | 0% | 0 |  | 1,018 | -0.39% |
| Render/mitsuhiko_table_wide | 10,219 | 0% | 22,709 | 0% | 0 |  | 346 | +14.95% |
| RenderSink/mitsuhiko_table_wide | 10,046 | 0% | 1,264 | 0% | 0 |  | 264 | +3.94% |
| RenderTo/mitsuhiko_table_wide | 10,212 | 0% | 22,653 | 0% | 0 |  | 309 | +5.82% |
| Nested/new_template | 2,211 | +0.64% | 1,613 | +0.81% | 0 |  | 24,525 | +1.18% |
| Load/plain_text | 0 |  | 1 | 0% | 0 |  | 2 | -89.47% |
| Render/plain_text | 1 | 0% | 0 |  | 0 |  | 6 | -14.29% |
| RenderSink/plain_text | 1 | 0% | 0 |  | 0 |  | 5 | -16.67% |
| RenderTo/plain_text | 2 | 0% | 0 |  | 0 |  | 6 | -14.29% |
| Load/prompt_budget | 75 | 0% | 64 | 0% | 0 |  | 1,431 | -0.07% |
| Render/prompt_budget | 4,797 | 0% | 554 | 0% | 0 |  | 900 | -11.33% |
| Nested/shared_globals | 192 | 0% | 115 | 0% | 0 |  | 9,532 | +2.49% |
| Load/strings | 125 | +0.81% | 110 | +0.92% | 0 |  | 2,813 | -0.21% |
| Render/strings | 421 | +0.48% | 491 | 0% | 0 |  | 1,598 | +2.57% |
| Load/substitute | 1 | 0% | 2 | 0% | 0 |  | 102 | -20.31% |
| Render/substitute | 1 | 0% | 0 |  | 0 |  | 3 | -25.00% |
| RenderSink/substitute | 1 | 0% | 0 |  | 0 |  | 2 | 0% |
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
