# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (227 so far, latest [7536fc2](https://github.com/jinja2cpp/Jinja2Cpp/commit/7536fc224ef1d9dbb686f02b58594ac0e7fa8ca6) on 2026-11-15). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 335,988 | 0% |  | 55 | 0% |  | 18,480 | 0% | -52.71% |
| [Render/chat_llama](#renderchat_llama) | 292,309 | +0.03% |  | 225 | 0% |  | 11,616 | 0% | -19.11% |
| [Load/chat_mistral](#loadchat_mistral) | 386,266 | +0.02% |  | 52 | 0% |  | 20,904 | 0% | -53.18% |
| [Render/chat_mistral](#renderchat_mistral) | 494,873 | 0% |  | 402 | 0% |  | 9,952 | 0% | -24.65% |
| [Load/chat_qwen](#loadchat_qwen) | 261,852 | 0% |  | 44 | 0% |  | 14,432 | 0% | -47.19% |
| [Render/chat_qwen](#renderchat_qwen) | 273,720 | -0.03% |  | 212 | 0% |  | 8,304 | 0% | -2.35% |
| [Load/config_file](#loadconfig_file) | 94,912 | 0% |  | 25 | 0% |  | 5,480 | 0% | -48.11% |
| [Render/config_file](#renderconfig_file) | 1,284,510 | +0.23% |  | 910 | 0% |  | 25,560 | 0% | +7.36% |
| [Load/dict_ops](#loaddict_ops) | 32,527 | 0% | -77.99% | 9 | 0% | -93.13% | 2,192 | 0% | -48.50% |
| [Render/dict_ops](#renderdict_ops) | 292,339 | 0% | -57.68% | 112 | 0% | -75.11% | 24,408 | 0% | -29.65% |
| [Load/expressions](#loadexpressions) | 44,486 | 0% | -78.42% | 16 | 0% | -89.04% | 2,744 | 0% | -55.28% |
| [Render/expressions](#renderexpressions) | 417,534 | 0% | -53.92% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 67,539 | 0% | -75.39% | 27 | 0% | -88.16% | 3,864 | 0% | -66.15% |
| [Render/filters](#renderfilters) | 27,008 | 0% | -69.54% | 23 | 0% | -68.92% | 752 | 0% | -84.69% |
| [Load/for_filter_if](#loadfor_filter_if) | 29,344 | 0% | -78.33% | 7 | 0% | -94.31% | 1,920 | 0% | -44.32% |
| [Render/for_filter_if](#renderfor_filter_if) | 268,882 | 0% | -68.93% | 4 | 0% | -98.23% | 16,560 | 0% | +354.95% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 39,169 | 0% | -78.32% | 10 | 0% | -93.63% | 2,384 | 0% | -47.26% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 172,283 | 0% | -83.94% | 2 | 0% | -99.62% | 1,568 | 0% | -43.35% |
| [Load/for_range](#loadfor_range) | 13,368 | 0% | -76.42% | 7 | 0% | -87.50% | 1,104 | 0% | -44.80% |
| [Render/for_range](#renderfor_range) | 37,471 | 0% | -59.36% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/hf_deepseek_v31](#loadhf_deepseek_v31) | 384,216 | 0% |  | 55 | 0% |  | 21,992 | 0% | +0.81% |
| [Render/hf_deepseek_v31](#renderhf_deepseek_v31) | 92,112 | 0% |  | 43 | 0% |  | 2,480 | 0% | 0% |
| [Load/hf_gpt_oss](#loadhf_gpt_oss) | 1,525,223 | +0.03% |  | 287 | 0% |  | 74,576 | 0% | +0.47% |
| [Render/hf_gpt_oss](#renderhf_gpt_oss) | 170,473 | +0.19% |  | 156 | 0% |  | 5,664 | 0% | +14.75% |
| [Load/hf_mistral_small](#loadhf_mistral_small) | 592,509 | 0% |  | 88 | 0% |  | 33,064 | 0% | +0.39% |
| [Render/hf_mistral_small](#renderhf_mistral_small) | 141,459 | +0.01% |  | 99 | 0% |  | 6,120 | 0% | -0.26% |
| [Load/hf_qwen3](#loadhf_qwen3) | 477,033 | 0% |  | 82 | 0% |  | 25,576 | 0% | +0.38% |
| [Render/hf_qwen3](#renderhf_qwen3) | 118,862 | -0.49% |  | 100 | 0% |  | 3,248 | 0% | 0% |
| [Load/html_autoescape](#loadhtml_autoescape) | 190,064 | 0% |  | 43 | 0% |  | 12,688 | 0% | -48.59% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,200,066 | -0.01% |  | 675 | 0% |  | 45,992 | 0% | +13.89% |
| [Load/inheritance](#loadinheritance) | 21,978 | 0% | -80.74% | 7 | 0% | -93.46% | 1,360 | 0% | -52.65% |
| [Render/inheritance](#renderinheritance) | 138,365 | 0% | -81.50% | 38 | 0% | -93.89% | 7,920 | 0% | +31.65% |
| [Load/int_arith](#loadint_arith) | 65,431 | 0% |  | 20 | 0% |  | 3,888 | 0% | +0.83% |
| [Render/int_arith](#renderint_arith) | 831,754 | 0% |  | 12 | 0% |  | 3,272 | 0% | -0.49% |
| [Module/keep](#modulekeep) | 7,192 | 0% |  | 7 | 0% |  |  |  |  |
| [Module/keep_written](#modulekeep_written) | 29,252 | 0% |  | 11 | 0% |  |  |  |  |
| [Load/large_static](#loadlarge_static) | 147,997 | 0% | -83.30% | 14 | 0% | -96.82% | 43,136 | 0% | -19.20% |
| [Render/large_static](#renderlarge_static) | 20,753 | 0% | -72.38% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [RenderSink/large_static](#rendersinklarge_static) | 12,743 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/large_static](#rendertolarge_static) | 20,316 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/macros](#loadmacros) | 55,023 | 0% | -75.86% | 27 | 0% | -85.48% | 4,392 | 0% | -29.34% |
| [Render/macros](#rendermacros) | 644,966 | 0% | -74.84% | 308 | 0% | -88.71% | 20,576 | 0% | +39.78% |
| [Load/many_tags](#loadmany_tags) | 8,765,149 | -0.01% | -80.15% | 951 | 0% | -97.22% | 438,128 | 0% | -58.48% |
| [Render/many_tags](#rendermany_tags) | 741,278 | +0.13% | -64.41% | 2 | 0% | -90.48% | 5,648 | 0% | -10.52% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 45,372 | 0% | -80.90% | 19 | 0% | -89.67% | 3,376 | 0% | -44.33% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,154,052 | 0% | -58.29% | 33 | 0% | -99.19% | 381,536 | 0% | +9.95% |
| [RenderSink/mitsuhiko_table](#rendersinkmitsuhiko_table) | 5,081,030 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table](#rendertomitsuhiko_table) | 5,153,544 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 53,060 | 0% |  | 35 | 0% |  | 5,704 | 0% | -30.98% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,499,865 | 0% |  | 33 | 0% |  | 1,414,928 | 0% | +2.46% |
| [RenderSink/mitsuhiko_table_wide](#rendersinkmitsuhiko_table_wide) | 5,323,084 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table_wide](#rendertomitsuhiko_table_wide) | 5,499,334 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/plain_text](#loadplain_text) | 3,757 | 0% | -61.17% | 4 | 0% | -77.78% | 688 | 0% | -38.57% |
| [Render/plain_text](#renderplain_text) | 1,058 | 0% | -84.43% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/plain_text](#rendersinkplain_text) | 760 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/plain_text](#rendertoplain_text) | 859 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/prompt_budget](#loadprompt_budget) | 34,647 | 0% |  | 18 | 0% |  | 2,240 | 0% | +0.72% |
| [Render/prompt_budget](#renderprompt_budget) | 1,319,827 | 0% |  | 76 | 0% |  | 36,416 | 0% | 0% |
| [Load/strings](#loadstrings) | 69,422 | 0% | -76.21% | 30 | 0% | -86.61% | 4,360 | 0% | -51.77% |
| [Render/strings](#renderstrings) | 582,925 | 0% | -72.21% | 344 | 0% | -83.73% | 9,088 | 0% | -49.31% |
| [Load/substitute](#loadsubstitute) | 7,537 | 0% | -74.76% | 6 | 0% | -83.78% | 848 | 0% | -44.21% |
| [Render/substitute](#rendersubstitute) | 2,257 | 0% | -74.39% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/substitute](#rendersinksubstitute) | 1,963 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/substitute](#rendertosubstitute) | 2,062 | 0% |  | 0 |  |  | 0 |  |  |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 764 | 0% | 761 | 0% | 0 |  | 7,823 | -4.55% |
| Render/chat_llama | 1,239 | +0.90% | 547 | +2.63% | 0 |  | 4,097 | -1.96% |
| Load/chat_mistral | 1,117 | +0.18% | 1,088 | 0% | 0 |  | 8,944 | -6.75% |
| Render/chat_mistral | 1,952 | -0.20% | 589 | -1.34% | 0 |  | 10,408 | +1.23% |
| Load/chat_qwen | 580 | 0% | 632 | 0% | 0 |  | 4,943 | -7.90% |
| Render/chat_qwen | 1,089 | +1.30% | 408 | -1.45% | 0 |  | 3,448 | -4.04% |
| Load/config_file | 224 | 0% | 184 | 0% | 0 |  | 2,832 | -5.54% |
| Render/config_file | 6,782 | -1.27% | 1,751 | -0.34% | 0 |  | 45,084 | -0.63% |
| Load/dict_ops | 51 | 0% | 30 | 0% | 0 |  | 1,370 | -1.15% |
| Render/dict_ops | 669 | -0.30% | 426 | -0.23% | 0 |  | 835 | -0.12% |
| Load/expressions | 36 | 0% | 28 | 0% | 0 |  | 1,329 | -2.78% |
| Render/expressions | 3 | -25.00% | 1 | 0% | 0 |  | 355 | +1.72% |
| Load/filters | 109 | 0% | 49 | 0% | 0 |  | 1,663 | +0.36% |
| Render/filters | 11 | +10.00% | 2 | 0% | 0 |  | 788 | +0.77% |
| Load/for_filter_if | 29 | 0% | 13 | 0% | 0 |  | 1,117 | -4.77% |
| Render/for_filter_if | 1,740 | 0% | 134 | -2.90% | 0 |  | 180 | -9.09% |
| Load/for_loop_vars | 50 | 0% | 23 | 0% | 0 |  | 1,202 | -4.83% |
| Render/for_loop_vars | 5 | +25.00% | 1 | 0% | 0 |  | 165 | +14.58% |
| Load/for_range | 9 | 0% | 5 | 0% | 0 |  | 761 | -3.30% |
| Render/for_range | 2 | 0% | 0 |  | 0 |  | 145 | -9.38% |
| Load/hf_deepseek_v31 | 1,049 | 0% | 981 | 0% | 0 |  | 6,643 | -11.92% |
| Render/hf_deepseek_v31 | 368 | -1.87% | 92 | -2.13% | 0 |  | 1,469 | +4.04% |
| Load/hf_gpt_oss | 4,658 | -0.06% | 3,373 | -0.06% | 0 |  | 23,168 | -10.35% |
| Render/hf_gpt_oss | 1,268 | +0.48% | 414 | +3.24% | 0 |  | 4,774 | +0.82% |
| Load/hf_mistral_small | 1,924 | +0.05% | 1,650 | -0.06% | 0 |  | 11,675 | -6.76% |
| Render/hf_mistral_small | 971 | -0.21% | 309 | +2.66% | 0 |  | 3,751 | +1.96% |
| Load/hf_qwen3 | 1,502 | 0% | 1,346 | 0% | 0 |  | 9,504 | -7.59% |
| Render/hf_qwen3 | 714 | +1.71% | 224 | +0.45% | 0 |  | 2,681 | -0.52% |
| Load/html_autoescape | 509 | 0% | 540 | +0.19% | 0 |  | 4,539 | -5.95% |
| Render/html_autoescape | 5,104 | +3.30% | 1,345 | +4.75% | 0 |  | 35,197 | -5.56% |
| Load/inheritance | 5 | 0% | 3 | 0% | 0 |  | 712 | -6.44% |
| Render/inheritance | 602 | +0.17% | 113 | +1.80% | 0 |  | 968 | -0.41% |
| Load/int_arith | 85 | 0% | 65 | 0% | 0 |  | 2,079 | -2.90% |
| Render/int_arith | 9 | -10.00% | 5 | 0% | 0 |  | 759 | +1.74% |
| Module/keep | 1 | 0% | 3 | 0% | 0 |  | 442 | -4.33% |
| Module/keep_written | 1,135 | -0.18% | 561 | +0.18% | 0 |  | 722 | -5.74% |
| Load/large_static | 1,928 | 0% | 905 | 0% | 0 |  | 354 | +6.95% |
| Render/large_static | 707 | 0% | 576 | 0% | 0 |  | 6 | +20.00% |
| RenderSink/large_static | 1 | 0% | 0 |  | 0 |  | 4 | 0% |
| RenderTo/large_static | 684 | 0% | 578 | 0% | 0 |  | 6 | +20.00% |
| Load/macros | 93 | -3.12% | 81 | -5.81% | 0 |  | 1,762 | -1.23% |
| Render/macros | 798 | +1.92% | 265 | +1.92% | 0 |  | 997 | -19.73% |
| Load/many_tags | 29,680 | -0.71% | 22,701 | -0.05% | 0 |  | 241,782 | -2.83% |
| Render/many_tags | 6,148 | +0.20% | 794 | -0.75% | 0 |  | 114 | +7.55% |
| Load/mitsuhiko_table | 39 | 0% | 48 | 0% | 0 |  | 903 | +0.44% |
| Render/mitsuhiko_table | 10,038 | -0.01% | 6,256 | +0.03% | 0 |  | 363 | -7.16% |
| RenderSink/mitsuhiko_table | 9,981 | -0.08% | 1,200 | +0.17% | 0 |  | 322 | -6.94% |
| RenderTo/mitsuhiko_table | 10,051 | +0.01% | 6,265 | +0.02% | 0 |  | 349 | -7.67% |
| Load/mitsuhiko_table_wide | 87 | 0% | 76 | +1.33% | 0 |  | 955 | -2.55% |
| Render/mitsuhiko_table_wide | 10,194 | -0.15% | 22,690 | +0.02% | 0 |  | 385 | -2.28% |
| RenderSink/mitsuhiko_table_wide | 10,028 | -0.07% | 1,307 | +0.77% | 0 |  | 315 | -11.27% |
| RenderTo/mitsuhiko_table_wide | 10,232 | +0.06% | 22,695 | +0.03% | 0 |  | 353 | -8.55% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 34 | -32.00% |
| Render/plain_text | 1 | 0% | 0 |  | 0 |  | 7 | +16.67% |
| RenderSink/plain_text | 1 | 0% | 0 |  | 0 |  | 6 | 0% |
| RenderTo/plain_text | 1 | 0% | 0 |  | 0 |  | 7 | +16.67% |
| Load/prompt_budget | 90 | 0% | 45 | 0% | 0 |  | 1,406 | -2.09% |
| Render/prompt_budget | 4,814 | 0% | 558 | +0.54% | 0 |  | 908 | -0.44% |
| Load/strings | 144 | 0% | 110 | 0% | 0 |  | 2,729 | -3.06% |
| Render/strings | 413 | -0.72% | 476 | +0.42% | 0 |  | 1,589 | -0.56% |
| Load/substitute | 3 | 0% | 4 | 0% | 0 |  | 222 | -1.77% |
| Render/substitute | 1 | 0% | 0 |  | 0 |  | 6 | +50.00% |
| RenderSink/substitute | 0 |  | 0 |  | 0 |  | 4 | 0% |
| RenderTo/substitute | 1 | 0% | 0 |  | 0 |  | 5 | 0% |

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
