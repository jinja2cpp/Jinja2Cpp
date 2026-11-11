# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (203 so far, latest [4787c27](https://github.com/jinja2cpp/Jinja2Cpp/commit/4787c274bb87dd177ba6bbd86720a97d89186845) on 2026-11-11). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 342,431 | 0% |  | 55 | 0% |  | 18,480 | 0% | -52.71% |
| [Render/chat_llama](#renderchat_llama) | 296,599 | 0% |  | 225 | 0% |  | 11,616 | 0% | -19.11% |
| [Load/chat_mistral](#loadchat_mistral) | 392,733 | 0% |  | 52 | 0% |  | 20,888 | 0% | -53.22% |
| [Render/chat_mistral](#renderchat_mistral) | 500,902 | 0% |  | 402 | 0% |  | 9,984 | 0% | -24.41% |
| [Load/chat_qwen](#loadchat_qwen) | 265,835 | 0% |  | 44 | 0% |  | 14,416 | 0% | -47.25% |
| [Render/chat_qwen](#renderchat_qwen) | 277,721 | 0% |  | 212 | 0% |  | 8,304 | 0% | -2.35% |
| [Load/config_file](#loadconfig_file) | 96,462 | 0% |  | 25 | 0% |  | 5,464 | 0% | -48.26% |
| [Render/config_file](#renderconfig_file) | 1,272,660 | 0% |  | 908 | 0% |  | 24,440 | 0% | +2.65% |
| [Load/dict_ops](#loaddict_ops) | 32,993 | 0% | -77.67% | 9 | 0% | -93.13% | 2,176 | 0% | -48.87% |
| [Render/dict_ops](#renderdict_ops) | 293,634 | 0% | -57.50% | 112 | 0% | -75.11% | 24,408 | 0% | -29.65% |
| [Load/expressions](#loadexpressions) | 44,924 | 0% | -78.21% | 16 | 0% | -89.04% | 2,728 | 0% | -55.54% |
| [Render/expressions](#renderexpressions) | 426,891 | 0% | -52.89% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 69,102 | 0% | -74.82% | 27 | 0% | -88.16% | 3,848 | 0% | -66.29% |
| [Render/filters](#renderfilters) | 27,336 | 0% | -69.17% | 23 | 0% | -68.92% | 752 | 0% | -84.69% |
| [Load/for_filter_if](#loadfor_filter_if) | 29,773 | 0% | -78.01% | 7 | 0% | -94.31% | 1,904 | 0% | -44.78% |
| [Render/for_filter_if](#renderfor_filter_if) | 274,384 | 0% | -68.30% | 4 | 0% | -98.23% | 16,560 | 0% | +354.95% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 39,966 | 0% | -77.88% | 10 | 0% | -93.63% | 2,368 | 0% | -47.61% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 180,540 | 0% | -83.17% | 2 | 0% | -99.62% | 1,568 | 0% | -43.35% |
| [Load/for_range](#loadfor_range) | 13,523 | 0% | -76.14% | 7 | 0% | -87.50% | 1,088 | 0% | -45.60% |
| [Render/for_range](#renderfor_range) | 38,357 | 0% | -58.40% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/hf_deepseek_v31](#loadhf_deepseek_v31) | 386,379 | 0% |  | 55 | 0% |  | 21,976 | 0% | +0.73% |
| [Render/hf_deepseek_v31](#renderhf_deepseek_v31) | 91,358 | 0% |  | 43 | 0% |  | 2,480 | 0% | 0% |
| [Load/hf_gpt_oss](#loadhf_gpt_oss) | 1,539,194 | 0% |  | 287 | 0% |  | 74,672 | 0% | +0.60% |
| [Render/hf_gpt_oss](#renderhf_gpt_oss) | 171,521 | 0% |  | 155 | 0% |  | 5,080 | 0% | +2.92% |
| [Load/hf_mistral_small](#loadhf_mistral_small) | 598,529 | 0% |  | 88 | 0% |  | 33,048 | 0% | +0.34% |
| [Render/hf_mistral_small](#renderhf_mistral_small) | 141,611 | 0% |  | 99 | 0% |  | 6,120 | 0% | -0.26% |
| [Load/hf_qwen3](#loadhf_qwen3) | 483,799 | 0% |  | 82 | 0% |  | 25,560 | 0% | +0.31% |
| [Render/hf_qwen3](#renderhf_qwen3) | 120,331 | 0% |  | 100 | 0% |  | 3,248 | 0% | 0% |
| [Load/html_autoescape](#loadhtml_autoescape) | 193,159 | 0% |  | 43 | 0% |  | 12,672 | 0% | -48.65% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,208,627 | 0% |  | 674 | 0% |  | 45,408 | 0% | +12.44% |
| [Load/inheritance](#loadinheritance) | 22,709 | 0% | -80.10% | 7 | 0% | -93.46% | 1,344 | 0% | -53.20% |
| [Render/inheritance](#renderinheritance) | 137,504 | 0% | -81.61% | 35 | 0% | -94.37% | 7,384 | 0% | +22.74% |
| [Load/int_arith](#loadint_arith) | 65,977 | 0% |  | 20 | 0% |  | 3,872 | 0% | +0.41% |
| [Render/int_arith](#renderint_arith) | 826,343 | 0% |  | 12 | 0% |  | 3,272 | 0% | -0.49% |
| [Module/keep](#modulekeep) | 7,061 | 0% |  | 7 | 0% |  |  |  |  |
| [Module/keep_written](#modulekeep_written) | 28,942 | 0% |  | 11 | 0% |  |  |  |  |
| [Load/large_static](#loadlarge_static) | 152,704 | 0% | -82.76% | 14 | 0% | -96.82% | 43,120 | 0% | -19.23% |
| [Render/large_static](#renderlarge_static) | 20,664 | 0% | -72.50% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [RenderSink/large_static](#rendersinklarge_static) | 12,702 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/large_static](#rendertolarge_static) | 20,275 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/macros](#loadmacros) | 56,421 | 0% | -75.25% | 27 | 0% | -85.48% | 4,392 | 0% | -29.34% |
| [Render/macros](#rendermacros) | 659,107 | 0% | -74.28% | 308 | 0% | -88.71% | 20,560 | 0% | +39.67% |
| [Load/many_tags](#loadmany_tags) | 8,982,454 | 0% | -79.66% | 951 | 0% | -97.22% | 438,128 | 0% | -58.48% |
| [Render/many_tags](#rendermany_tags) | 729,869 | 0% | -64.96% | 2 | 0% | -90.48% | 5,648 | 0% | -10.52% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 46,346 | 0% | -80.49% | 19 | 0% | -89.67% | 3,360 | 0% | -44.59% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,202,650 | 0% | -57.89% | 33 | 0% | -99.19% | 381,536 | 0% | +9.95% |
| [RenderSink/mitsuhiko_table](#rendersinkmitsuhiko_table) | 5,129,845 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table](#rendertomitsuhiko_table) | 5,202,156 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 54,036 | 0% |  | 35 | 0% |  | 5,688 | 0% | -31.17% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,548,817 | 0% |  | 33 | 0% |  | 1,414,928 | 0% | +2.46% |
| [RenderSink/mitsuhiko_table_wide](#rendersinkmitsuhiko_table_wide) | 5,372,109 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table_wide](#rendertomitsuhiko_table_wide) | 5,548,369 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/plain_text](#loadplain_text) | 3,706 | 0% | -61.70% | 4 | 0% | -77.78% | 672 | 0% | -40.00% |
| [Render/plain_text](#renderplain_text) | 1,022 | 0% | -84.96% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/plain_text](#rendersinkplain_text) | 724 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/plain_text](#rendertoplain_text) | 823 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/prompt_budget](#loadprompt_budget) | 35,062 | 0% |  | 18 | 0% |  | 2,224 | 0% | 0% |
| [Render/prompt_budget](#renderprompt_budget) | 1,315,493 | 0% |  | 76 | 0% |  | 36,416 | 0% | 0% |
| [Load/strings](#loadstrings) | 70,522 | 0% | -75.84% | 30 | 0% | -86.61% | 4,344 | 0% | -51.95% |
| [Render/strings](#renderstrings) | 581,806 | 0% | -72.26% | 344 | 0% | -83.73% | 9,088 | 0% | -49.31% |
| [Load/substitute](#loadsubstitute) | 7,643 | 0% | -74.40% | 6 | 0% | -83.78% | 832 | 0% | -45.26% |
| [Render/substitute](#rendersubstitute) | 2,255 | 0% | -74.41% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/substitute](#rendersinksubstitute) | 1,961 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/substitute](#rendertosubstitute) | 2,060 | 0% |  | 0 |  |  | 0 |  |  |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 735 | 0% | 765 | 0% | 0 |  | 7,470 | -2.87% |
| Render/chat_llama | 1,154 | 0% | 502 | 0% | 0 |  | 3,978 | +2.00% |
| Load/chat_mistral | 1,079 | 0% | 1,069 | 0% | 0 |  | 8,381 | -3.47% |
| Render/chat_mistral | 1,909 | 0% | 576 | 0% | 0 |  | 9,811 | +1.14% |
| Load/chat_qwen | 560 | 0% | 627 | 0% | 0 |  | 4,358 | -6.52% |
| Render/chat_qwen | 1,093 | 0% | 391 | 0% | 0 |  | 3,439 | +4.40% |
| Load/config_file | 228 | 0% | 203 | 0% | 0 |  | 2,652 | -5.89% |
| Render/config_file | 6,103 | 0% | 1,537 | 0% | 0 |  | 42,010 | +0.17% |
| Load/dict_ops | 42 | 0% | 21 | 0% | 0 |  | 1,326 | -3.84% |
| Render/dict_ops | 678 | 0% | 430 | 0% | 0 |  | 870 | +1.16% |
| Load/expressions | 36 | 0% | 26 | 0% | 0 |  | 1,275 | -9.32% |
| Render/expressions | 13 | 0% | 7 | 0% | 0 |  | 306 | +9.68% |
| Load/filters | 113 | 0% | 61 | 0% | 0 |  | 1,696 | -7.83% |
| Render/filters | 17 | 0% | 3 | 0% | 0 |  | 769 | +2.40% |
| Load/for_filter_if | 35 | 0% | 21 | 0% | 0 |  | 1,075 | -0.37% |
| Render/for_filter_if | 1,736 | 0% | 139 | 0% | 0 |  | 199 | +3.11% |
| Load/for_loop_vars | 52 | 0% | 22 | 0% | 0 |  | 1,084 | -3.56% |
| Render/for_loop_vars | 7 | 0% | 3 | 0% | 0 |  | 148 | -9.20% |
| Load/for_range | 3 | 0% | 4 | 0% | 0 |  | 761 | 0% |
| Render/for_range | 1 | 0% | 0 |  | 0 |  | 95 | -40.62% |
| Load/hf_deepseek_v31 | 1,010 | 0% | 981 | 0% | 0 |  | 6,105 | +0.30% |
| Render/hf_deepseek_v31 | 350 | 0% | 80 | 0% | 0 |  | 1,442 | +7.93% |
| Load/hf_gpt_oss | 4,695 | 0% | 3,378 | 0% | 0 |  | 21,521 | -5.85% |
| Render/hf_gpt_oss | 1,192 | 0% | 367 | 0% | 0 |  | 4,604 | +1.93% |
| Load/hf_mistral_small | 1,863 | 0% | 1,641 | 0% | 0 |  | 10,679 | -4.52% |
| Render/hf_mistral_small | 959 | 0% | 299 | 0% | 0 |  | 3,724 | +3.39% |
| Load/hf_qwen3 | 1,517 | 0% | 1,341 | 0% | 0 |  | 8,424 | -6.26% |
| Render/hf_qwen3 | 674 | 0% | 214 | 0% | 0 |  | 2,620 | +2.14% |
| Load/html_autoescape | 498 | 0% | 548 | 0% | 0 |  | 3,837 | -7.56% |
| Render/html_autoescape | 4,297 | 0% | 1,359 | 0% | 0 |  | 35,876 | +0.32% |
| Load/inheritance | 3 | 0% | 3 | 0% | 0 |  | 709 | +2.16% |
| Render/inheritance | 585 | 0% | 92 | 0% | 0 |  | 951 | -19.27% |
| Load/int_arith | 106 | 0% | 69 | 0% | 0 |  | 2,020 | -9.58% |
| Render/int_arith | 17 | 0% | 9 | 0% | 0 |  | 717 | +0.42% |
| Module/keep | 2 | 0% | 2 | 0% | 0 |  | 433 | -2.04% |
| Module/keep_written | 1,128 | 0% | 562 | 0% | 0 |  | 756 | +0.53% |
| Load/large_static | 1,933 | 0% | 913 | 0% | 0 |  | 267 | -14.70% |
| Render/large_static | 713 | 0% | 572 | 0% | 0 |  | 7 | -36.36% |
| RenderSink/large_static | 0 |  | 0 |  | 0 |  | 3 | 0% |
| RenderTo/large_static | 689 | 0% | 568 | 0% | 0 |  | 6 | +50.00% |
| Load/macros | 99 | 0% | 90 | 0% | 0 |  | 1,710 | -3.23% |
| Render/macros | 755 | 0% | 236 | 0% | 0 |  | 963 | -8.46% |
| Load/many_tags | 29,026 | 0% | 22,791 | 0% | 0 |  | 235,534 | -1.50% |
| Render/many_tags | 6,224 | 0% | 863 | 0% | 0 |  | 260 | +71.05% |
| Load/mitsuhiko_table | 45 | 0% | 55 | 0% | 0 |  | 823 | -10.05% |
| Render/mitsuhiko_table | 10,043 | 0% | 6,277 | 0% | 0 |  | 350 | -0.28% |
| RenderSink/mitsuhiko_table | 9,961 | 0% | 1,178 | 0% | 0 |  | 272 | -11.97% |
| RenderTo/mitsuhiko_table | 10,050 | 0% | 6,282 | 0% | 0 |  | 320 | -4.19% |
| Load/mitsuhiko_table_wide | 72 | 0% | 94 | 0% | 0 |  | 1,038 | +0.19% |
| Render/mitsuhiko_table_wide | 10,225 | 0% | 22,687 | 0% | 0 |  | 342 | -7.07% |
| RenderSink/mitsuhiko_table_wide | 10,049 | 0% | 1,308 | 0% | 0 |  | 256 | -20.00% |
| RenderTo/mitsuhiko_table_wide | 10,226 | 0% | 22,691 | 0% | 0 |  | 314 | -10.80% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 16 | -5.88% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 6 | 0% |
| RenderSink/plain_text | 0 |  | 0 |  | 0 |  | 3 | -40.00% |
| RenderTo/plain_text | 0 |  | 0 |  | 0 |  | 5 | -16.67% |
| Load/prompt_budget | 77 | 0% | 45 | 0% | 0 |  | 1,407 | +1.30% |
| Render/prompt_budget | 4,812 | 0% | 519 | 0% | 0 |  | 972 | +16.27% |
| Load/strings | 177 | 0% | 127 | 0% | 0 |  | 2,764 | -1.29% |
| Render/strings | 423 | 0% | 483 | 0% | 0 |  | 1,582 | +0.70% |
| Load/substitute | 1 | 0% | 3 | 0% | 0 |  | 127 | -33.16% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 6 | +200.00% |
| RenderSink/substitute | 0 |  | 0 |  | 0 |  | 3 | 0% |
| RenderTo/substitute | 0 |  | 0 |  | 0 |  | 6 | +200.00% |

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
