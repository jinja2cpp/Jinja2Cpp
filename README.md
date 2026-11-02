# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (170 so far, latest [a35a0e3](https://github.com/jinja2cpp/Jinja2Cpp/commit/a35a0e3f553251e7385c635a71124ac9c060f6a9) on 2026-11-02). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 343,690 | -0.04% |  | 55 | 0% |  | 18,368 | 0% | -53.00% |
| [Render/chat_llama](#renderchat_llama) | 293,247 | -0.51% |  | 225 | 0% |  | 11,616 | 0% | -19.11% |
| [Load/chat_mistral](#loadchat_mistral) | 392,954 | +0.07% |  | 52 | 0% |  | 20,824 | +0.15% | -53.36% |
| [Render/chat_mistral](#renderchat_mistral) | 498,863 | -0.27% |  | 402 | 0% |  | 9,952 | -0.32% | -24.65% |
| [Load/chat_qwen](#loadchat_qwen) | 265,579 | -0.32% |  | 44 | 0% |  | 14,416 | -0.11% | -47.25% |
| [Render/chat_qwen](#renderchat_qwen) | 277,517 | -0.83% |  | 212 | 0% |  | 8,304 | 0% | -2.35% |
| [Load/config_file](#loadconfig_file) | 97,064 | +0.17% |  | 25 | 0% |  | 5,464 | 0% | -48.26% |
| [Render/config_file](#renderconfig_file) | 1,270,751 | +0.01% |  | 908 | 0% |  | 24,264 | 0% | +1.92% |
| [Load/dict_ops](#loaddict_ops) | 33,233 | +0.16% | -77.51% | 9 | 0% | -93.13% | 2,176 | 0% | -48.87% |
| [Render/dict_ops](#renderdict_ops) | 297,597 | +0.06% | -56.92% | 112 | 0% | -75.11% | 24,408 | 0% | -29.65% |
| [Load/expressions](#loadexpressions) | 45,298 | +0.11% | -78.03% | 16 | 0% | -89.04% | 2,728 | 0% | -55.54% |
| [Render/expressions](#renderexpressions) | 427,456 | +0.01% | -52.83% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 70,498 | 0% | -74.31% | 27 | 0% | -88.16% | 3,832 | 0% | -66.43% |
| [Render/filters](#renderfilters) | 30,225 | 0% | -65.91% | 26 | 0% | -64.86% | 1,080 | 0% | -78.01% |
| [Load/for_filter_if](#loadfor_filter_if) | 29,847 | +0.24% | -77.95% | 7 | 0% | -94.31% | 1,904 | 0% | -44.78% |
| [Render/for_filter_if](#renderfor_filter_if) | 274,434 | -0.07% | -68.29% | 4 | 0% | -98.23% | 16,560 | 0% | +354.95% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 40,163 | +0.09% | -77.77% | 10 | 0% | -93.63% | 2,368 | 0% | -47.61% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 192,694 | +0.01% | -82.04% | 2 | 0% | -99.62% | 1,568 | 0% | -43.35% |
| [Load/for_range](#loadfor_range) | 13,574 | +0.04% | -76.05% | 7 | 0% | -87.50% | 1,088 | 0% | -45.60% |
| [Render/for_range](#renderfor_range) | 38,814 | +0.04% | -57.91% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/hf_deepseek_v31](#loadhf_deepseek_v31) | 385,201 |  |  | 55 |  |  | 21,816 |  | 0% |
| [Render/hf_deepseek_v31](#renderhf_deepseek_v31) | 90,420 |  |  | 43 |  |  | 2,480 |  | 0% |
| [Load/hf_gpt_oss](#loadhf_gpt_oss) | 1,535,778 |  |  | 285 |  |  | 74,224 |  | 0% |
| [Render/hf_gpt_oss](#renderhf_gpt_oss) | 172,094 |  |  | 155 |  |  | 4,936 |  | 0% |
| [Load/hf_mistral_small](#loadhf_mistral_small) | 596,516 |  |  | 88 |  |  | 32,936 |  | 0% |
| [Render/hf_mistral_small](#renderhf_mistral_small) | 145,198 |  |  | 105 |  |  | 6,136 |  | 0% |
| [Load/hf_qwen3](#loadhf_qwen3) | 484,854 |  |  | 82 |  |  | 25,480 |  | 0% |
| [Render/hf_qwen3](#renderhf_qwen3) | 119,697 |  |  | 100 |  |  | 3,248 |  | 0% |
| [Load/html_autoescape](#loadhtml_autoescape) | 193,347 | +0.08% |  | 43 | 0% |  | 12,640 | 0% | -48.78% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,207,666 | +0.02% |  | 674 | 0% |  | 45,296 | 0% | +12.16% |
| [Load/inheritance](#loadinheritance) | 22,767 | +0.13% | -80.05% | 7 | 0% | -93.46% | 1,328 | 0% | -53.76% |
| [Render/inheritance](#renderinheritance) | 136,462 | +0.11% | -81.75% | 35 | 0% | -94.37% | 7,352 | 0% | +22.21% |
| [Load/int_arith](#loadint_arith) | 66,316 | +0.53% |  | 20 | 0% |  | 3,872 | +0.41% | +0.41% |
| [Render/int_arith](#renderint_arith) | 825,958 | +0.13% |  | 12 | 0% |  | 3,272 | 0% | -0.49% |
| [Module/keep](#modulekeep) | 6,926 | +0.48% |  | 7 | 0% |  |  |  |  |
| [Module/keep_written](#modulekeep_written) | 36,319 | +0.16% |  | 11 | 0% |  |  |  |  |
| [Load/large_static](#loadlarge_static) | 155,122 | +0.15% | -82.49% | 14 | 0% | -96.82% | 43,120 | 0% | -19.23% |
| [Render/large_static](#renderlarge_static) | 20,874 | 0% | -72.22% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [RenderSink/large_static](#rendersinklarge_static) | 12,676 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/large_static](#rendertolarge_static) | 20,253 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/macros](#loadmacros) | 55,555 | -0.18% | -75.63% | 27 | 0% | -85.48% | 4,344 | 0% | -30.12% |
| [Render/macros](#rendermacros) | 652,700 | +0.06% | -74.53% | 308 | 0% | -88.71% | 20,528 | 0% | +39.46% |
| [Load/many_tags](#loadmany_tags) | 8,944,813 | +0.06% | -79.75% | 951 | 0% | -97.22% | 436,096 | +0.02% | -58.67% |
| [Render/many_tags](#rendermany_tags) | 725,357 | -0.02% | -65.18% | 2 | 0% | -90.48% | 5,648 | 0% | -10.52% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 46,305 | -0.23% | -80.51% | 19 | 0% | -89.67% | 3,360 | 0% | -44.59% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,225,363 | 0% | -57.71% | 33 | 0% | -99.19% | 381,536 | 0% | +9.95% |
| [RenderSink/mitsuhiko_table](#rendersinkmitsuhiko_table) | 5,152,563 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table](#rendertomitsuhiko_table) | 5,224,838 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 54,772 | -0.40% |  | 35 | 0% |  | 5,688 | 0% | -31.17% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,571,362 | 0% |  | 33 | 0% |  | 1,414,928 | 0% | +2.46% |
| [RenderSink/mitsuhiko_table_wide](#rendersinkmitsuhiko_table_wide) | 5,394,594 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table_wide](#rendertomitsuhiko_table_wide) | 5,570,856 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/plain_text](#loadplain_text) | 3,692 | 0% | -61.84% | 4 | 0% | -77.78% | 672 | 0% | -40.00% |
| [Render/plain_text](#renderplain_text) | 1,007 | 0% | -85.18% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/plain_text](#rendersinkplain_text) | 707 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/plain_text](#rendertoplain_text) | 808 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/strings](#loadstrings) | 71,760 | +0.60% | -75.41% | 30 | 0% | -86.61% | 4,328 | 0% | -52.12% |
| [Render/strings](#renderstrings) | 583,543 | -0.02% | -72.18% | 344 | 0% | -83.73% | 9,088 | 0% | -49.31% |
| [Load/substitute](#loadsubstitute) | 7,709 | 0% | -74.18% | 6 | 0% | -83.78% | 832 | 0% | -45.26% |
| [Render/substitute](#rendersubstitute) | 2,223 | 0% | -74.78% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/substitute](#rendersinksubstitute) | 1,926 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/substitute](#rendertosubstitute) | 2,028 | 0% |  | 0 |  |  | 0 |  |  |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 738 | +2.22% | 749 | +0.40% | 0 |  | 7,168 | -4.17% |
| Render/chat_llama | 1,199 | +3.54% | 493 | +6.02% | 0 |  | 3,890 | +9.58% |
| Load/chat_mistral | 1,075 | -2.98% | 1,035 | -0.48% | 0 |  | 8,262 | -5.10% |
| Render/chat_mistral | 1,933 | +2.66% | 555 | +1.46% | 0 |  | 9,692 | +0.82% |
| Load/chat_qwen | 465 | -3.12% | 588 | -2.65% | 0 |  | 4,448 | -2.13% |
| Render/chat_qwen | 1,100 | -2.14% | 419 | +6.08% | 0 |  | 3,205 | +7.77% |
| Load/config_file | 187 | +2.75% | 165 | -0.60% | 0 |  | 2,619 | -2.86% |
| Render/config_file | 6,208 | +4.46% | 1,526 | +5.10% | 0 |  | 40,808 | -1.27% |
| Load/dict_ops | 52 | +40.54% | 40 | +135.29% | 0 |  | 1,280 | -0.93% |
| Render/dict_ops | 753 | -1.44% | 408 | -2.16% | 0 |  | 838 | -4.45% |
| Load/expressions | 37 | -40.32% | 22 | -45.00% | 0 |  | 1,268 | -4.52% |
| Render/expressions | 11 | +57.14% | 4 | +100.00% | 0 |  | 330 | +6.45% |
| Load/filters | 135 | +2.27% | 82 | +1.23% | 0 |  | 1,754 | +4.47% |
| Render/filters | 20 | +25.00% | 3 | -25.00% | 0 |  | 782 | +2.89% |
| Load/for_filter_if | 21 | +61.54% | 10 | +42.86% | 0 |  | 1,072 | +6.03% |
| Render/for_filter_if | 1,800 | +3.33% | 98 | +4.26% | 0 |  | 163 | -10.93% |
| Load/for_loop_vars | 42 | +23.53% | 31 | +82.35% | 0 |  | 1,061 | -3.98% |
| Render/for_loop_vars | 8 | -11.11% | 0 | -100.00% | 0 |  | 91 | -48.30% |
| Load/for_range | 14 | 0% | 8 | -11.11% | 0 |  | 712 | +3.34% |
| Render/for_range | 1 | 0% | 1 | 0% | 0 |  | 110 | +5.77% |
| Load/hf_deepseek_v31 | 959 |  | 936 |  | 0 |  | 5,906 |  |
| Render/hf_deepseek_v31 | 387 |  | 83 |  | 0 |  | 1,412 |  |
| Load/hf_gpt_oss | 4,625 |  | 3,272 |  | 0 |  | 20,663 |  |
| Render/hf_gpt_oss | 1,279 |  | 388 |  | 0 |  | 4,472 |  |
| Load/hf_mistral_small | 1,789 |  | 1,585 |  | 0 |  | 10,802 |  |
| Render/hf_mistral_small | 983 |  | 274 |  | 0 |  | 3,788 |  |
| Load/hf_qwen3 | 1,452 |  | 1,264 |  | 0 |  | 8,554 |  |
| Render/hf_qwen3 | 692 |  | 197 |  | 0 |  | 2,504 |  |
| Load/html_autoescape | 455 | +2.02% | 512 | +1.79% | 0 |  | 3,932 | +0.72% |
| Render/html_autoescape | 3,368 | -31.61% | 1,096 | -26.74% | 0 |  | 35,043 | -4.13% |
| Load/inheritance | 3 | -72.73% | 3 | 0% | 0 |  | 658 | +2.97% |
| Render/inheritance | 580 | -4.76% | 97 | -6.73% | 0 |  | 869 | +2.12% |
| Load/int_arith | 93 | -16.22% | 63 | -31.52% | 0 |  | 1,972 | -0.10% |
| Render/int_arith | 9 | -50.00% | 3 | +50.00% | 0 |  | 747 | -2.23% |
| Module/keep | 2 | -66.67% | 1 | -50.00% | 0 |  | 348 | -5.18% |
| Module/keep_written | 2,933 | -0.03% | 563 | +0.54% | 0 |  | 731 | +5.64% |
| Load/large_static | 1,950 | +0.31% | 915 | +0.11% | 0 |  | 264 | -10.20% |
| Render/large_static | 706 | -0.42% | 570 | 0% | 0 |  | 4 | 0% |
| RenderSink/large_static | 1 | 0% | 0 |  | 0 |  | 3 | +50.00% |
| RenderTo/large_static | 684 | -0.44% | 569 | -0.18% | 0 |  | 4 | 0% |
| Load/macros | 77 | -12.50% | 72 | -7.69% | 0 |  | 1,667 | -2.46% |
| Render/macros | 771 | +1.72% | 261 | -0.76% | 0 |  | 1,592 | -36.22% |
| Load/many_tags | 28,209 | -0.52% | 22,302 | -1.55% | 0 |  | 221,349 | -0.74% |
| Render/many_tags | 6,199 | +0.24% | 870 | -0.23% | 0 |  | 161 | -2.42% |
| Load/mitsuhiko_table | 48 | +4.35% | 43 | -28.33% | 0 |  | 809 | -3.11% |
| Render/mitsuhiko_table | 10,102 | +0.31% | 6,297 | -1.05% | 0 |  | 356 | -0.56% |
| RenderSink/mitsuhiko_table | 10,011 | +0.58% | 1,207 | -0.74% | 0 |  | 291 | +1.04% |
| RenderTo/mitsuhiko_table | 10,092 | +0.72% | 6,299 | -0.02% | 0 |  | 326 | -1.51% |
| Load/mitsuhiko_table_wide | 72 | -8.86% | 81 | -10.99% | 0 |  | 983 | +2.61% |
| Render/mitsuhiko_table_wide | 10,281 | +0.69% | 22,623 | -0.27% | 0 |  | 342 | -6.81% |
| RenderSink/mitsuhiko_table_wide | 10,135 | +1.07% | 1,288 | +0.16% | 0 |  | 274 | -6.80% |
| RenderTo/mitsuhiko_table_wide | 10,292 | +0.90% | 22,697 | +0.05% | 0 |  | 331 | -0.90% |
| Load/plain_text | 1 |  | 2 | 0% | 0 |  | 7 | +16.67% |
| Render/plain_text | 1 |  | 0 |  | 0 |  | 5 | +25.00% |
| RenderSink/plain_text | 1 |  | 0 |  | 0 |  | 4 | +33.33% |
| RenderTo/plain_text | 1 |  | 0 |  | 0 |  | 4 | 0% |
| Load/strings | 181 | +11.04% | 114 | +16.33% | 0 |  | 2,688 | -1.03% |
| Render/strings | 441 | -2.22% | 470 | +0.64% | 0 |  | 2,016 | +31.34% |
| Load/substitute | 6 | +200.00% | 5 | +66.67% | 0 |  | 145 | -3.97% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 4 | 0% |
| RenderSink/substitute | 0 |  | 0 |  | 0 |  | 2 | 0% |
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
