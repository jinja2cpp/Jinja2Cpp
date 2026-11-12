# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (204 so far, latest [3c1b96b](https://github.com/jinja2cpp/Jinja2Cpp/commit/3c1b96b136a0192eb42e0ee75557cb86adffb2f2) on 2026-11-12). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 342,182 | -0.07% |  | 55 | 0% |  | 18,464 | -0.09% | -52.75% |
| [Render/chat_llama](#renderchat_llama) | 296,087 | -0.17% |  | 225 | 0% |  | 11,616 | 0% | -19.11% |
| [Load/chat_mistral](#loadchat_mistral) | 392,750 | 0% |  | 52 | 0% |  | 20,888 | 0% | -53.22% |
| [Render/chat_mistral](#renderchat_mistral) | 500,909 | 0% |  | 402 | 0% |  | 9,920 | -0.64% | -24.89% |
| [Load/chat_qwen](#loadchat_qwen) | 265,839 | 0% |  | 44 | 0% |  | 14,416 | 0% | -47.25% |
| [Render/chat_qwen](#renderchat_qwen) | 277,817 | +0.03% |  | 212 | 0% |  | 8,304 | 0% | -2.35% |
| [Load/config_file](#loadconfig_file) | 96,466 | 0% |  | 25 | 0% |  | 5,464 | 0% | -48.26% |
| [Render/config_file](#renderconfig_file) | 1,273,206 | +0.04% |  | 908 | 0% |  | 24,440 | 0% | +2.65% |
| [Load/dict_ops](#loaddict_ops) | 32,988 | -0.02% | -77.68% | 9 | 0% | -93.13% | 2,176 | 0% | -48.87% |
| [Render/dict_ops](#renderdict_ops) | 293,639 | 0% | -57.50% | 112 | 0% | -75.11% | 24,408 | 0% | -29.65% |
| [Load/expressions](#loadexpressions) | 44,924 | 0% | -78.21% | 16 | 0% | -89.04% | 2,728 | 0% | -55.54% |
| [Render/expressions](#renderexpressions) | 426,906 | 0% | -52.89% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 69,084 | -0.03% | -74.83% | 27 | 0% | -88.16% | 3,848 | 0% | -66.29% |
| [Render/filters](#renderfilters) | 27,336 | 0% | -69.17% | 23 | 0% | -68.92% | 752 | 0% | -84.69% |
| [Load/for_filter_if](#loadfor_filter_if) | 29,818 | +0.15% | -77.98% | 7 | 0% | -94.31% | 1,904 | 0% | -44.78% |
| [Render/for_filter_if](#renderfor_filter_if) | 274,386 | 0% | -68.30% | 4 | 0% | -98.23% | 16,560 | 0% | +354.95% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 39,910 | -0.14% | -77.91% | 10 | 0% | -93.63% | 2,368 | 0% | -47.61% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 180,540 | 0% | -83.17% | 2 | 0% | -99.62% | 1,568 | 0% | -43.35% |
| [Load/for_range](#loadfor_range) | 13,523 | 0% | -76.14% | 7 | 0% | -87.50% | 1,088 | 0% | -45.60% |
| [Render/for_range](#renderfor_range) | 38,357 | 0% | -58.40% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/hf_deepseek_v31](#loadhf_deepseek_v31) | 386,637 | +0.07% |  | 55 | 0% |  | 21,976 | 0% | +0.73% |
| [Render/hf_deepseek_v31](#renderhf_deepseek_v31) | 91,370 | +0.01% |  | 43 | 0% |  | 2,480 | 0% | 0% |
| [Load/hf_gpt_oss](#loadhf_gpt_oss) | 1,540,401 | +0.08% |  | 287 | 0% |  | 74,672 | 0% | +0.60% |
| [Render/hf_gpt_oss](#renderhf_gpt_oss) | 171,670 | +0.09% |  | 155 | 0% |  | 5,064 | -0.31% | +2.59% |
| [Load/hf_mistral_small](#loadhf_mistral_small) | 598,649 | +0.02% |  | 88 | 0% |  | 33,048 | 0% | +0.34% |
| [Render/hf_mistral_small](#renderhf_mistral_small) | 141,550 | -0.04% |  | 99 | 0% |  | 6,120 | 0% | -0.26% |
| [Load/hf_qwen3](#loadhf_qwen3) | 484,270 | +0.10% |  | 82 | 0% |  | 25,560 | 0% | +0.31% |
| [Render/hf_qwen3](#renderhf_qwen3) | 120,205 | -0.10% |  | 100 | 0% |  | 3,248 | 0% | 0% |
| [Load/html_autoescape](#loadhtml_autoescape) | 193,171 | +0.01% |  | 43 | 0% |  | 12,672 | 0% | -48.65% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,208,088 | -0.04% |  | 674 | 0% |  | 45,408 | 0% | +12.44% |
| [Load/inheritance](#loadinheritance) | 22,709 | 0% | -80.10% | 7 | 0% | -93.46% | 1,344 | 0% | -53.20% |
| [Render/inheritance](#renderinheritance) | 137,432 | -0.05% | -81.62% | 35 | 0% | -94.37% | 7,384 | 0% | +22.74% |
| [Load/int_arith](#loadint_arith) | 66,157 | +0.27% |  | 20 | 0% |  | 3,872 | 0% | +0.41% |
| [Render/int_arith](#renderint_arith) | 826,343 | 0% |  | 12 | 0% |  | 3,272 | 0% | -0.49% |
| [Module/keep](#modulekeep) | 7,054 | -0.10% |  | 7 | 0% |  |  |  |  |
| [Module/keep_written](#modulekeep_written) | 28,934 | -0.03% |  | 11 | 0% |  |  |  |  |
| [Load/large_static](#loadlarge_static) | 152,704 | 0% | -82.76% | 14 | 0% | -96.82% | 43,120 | 0% | -19.23% |
| [Render/large_static](#renderlarge_static) | 20,664 | 0% | -72.50% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [RenderSink/large_static](#rendersinklarge_static) | 12,702 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/large_static](#rendertolarge_static) | 20,275 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/macros](#loadmacros) | 56,421 | 0% | -75.25% | 27 | 0% | -85.48% | 4,392 | 0% | -29.34% |
| [Render/macros](#rendermacros) | 660,069 | +0.15% | -74.25% | 308 | 0% | -88.71% | 20,560 | 0% | +39.67% |
| [Load/many_tags](#loadmany_tags) | 8,982,820 | 0% | -79.66% | 951 | 0% | -97.22% | 438,160 | +0.01% | -58.48% |
| [Render/many_tags](#rendermany_tags) | 729,874 | 0% | -64.96% | 2 | 0% | -90.48% | 5,648 | 0% | -10.52% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 46,346 | 0% | -80.49% | 19 | 0% | -89.67% | 3,360 | 0% | -44.59% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,202,672 | 0% | -57.89% | 33 | 0% | -99.19% | 381,536 | 0% | +9.95% |
| [RenderSink/mitsuhiko_table](#rendersinkmitsuhiko_table) | 5,129,867 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table](#rendertomitsuhiko_table) | 5,202,178 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 54,036 | 0% |  | 35 | 0% |  | 5,688 | 0% | -31.17% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,548,839 | 0% |  | 33 | 0% |  | 1,414,928 | 0% | +2.46% |
| [RenderSink/mitsuhiko_table_wide](#rendersinkmitsuhiko_table_wide) | 5,372,131 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table_wide](#rendertomitsuhiko_table_wide) | 5,548,391 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/plain_text](#loadplain_text) | 3,706 | 0% | -61.70% | 4 | 0% | -77.78% | 672 | 0% | -40.00% |
| [Render/plain_text](#renderplain_text) | 1,022 | 0% | -84.96% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/plain_text](#rendersinkplain_text) | 724 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/plain_text](#rendertoplain_text) | 823 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/prompt_budget](#loadprompt_budget) | 35,055 | -0.02% |  | 18 | 0% |  | 2,224 | 0% | 0% |
| [Render/prompt_budget](#renderprompt_budget) | 1,315,548 | 0% |  | 76 | 0% |  | 36,416 | 0% | 0% |
| [Load/strings](#loadstrings) | 70,480 | -0.06% | -75.85% | 30 | 0% | -86.61% | 4,344 | 0% | -51.95% |
| [Render/strings](#renderstrings) | 581,816 | 0% | -72.26% | 344 | 0% | -83.73% | 9,088 | 0% | -49.31% |
| [Load/substitute](#loadsubstitute) | 7,643 | 0% | -74.40% | 6 | 0% | -83.78% | 832 | 0% | -45.26% |
| [Render/substitute](#rendersubstitute) | 2,255 | 0% | -74.41% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/substitute](#rendersinksubstitute) | 1,961 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/substitute](#rendertosubstitute) | 2,060 | 0% |  | 0 |  |  | 0 |  |  |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 747 | +1.63% | 761 | -0.52% | 0 |  | 7,497 | +0.36% |
| Render/chat_llama | 1,189 | +3.03% | 522 | +3.98% | 0 |  | 4,016 | +0.96% |
| Load/chat_mistral | 1,082 | +0.28% | 1,082 | +1.22% | 0 |  | 8,381 | 0% |
| Render/chat_mistral | 1,956 | +2.46% | 597 | +3.65% | 0 |  | 9,746 | -0.66% |
| Load/chat_qwen | 630 | +12.50% | 627 | 0% | 0 |  | 4,365 | +0.16% |
| Render/chat_qwen | 1,115 | +2.01% | 397 | +1.53% | 0 |  | 3,475 | +1.05% |
| Load/config_file | 230 | +0.88% | 205 | +0.99% | 0 |  | 2,674 | +0.83% |
| Render/config_file | 6,268 | +2.70% | 1,664 | +8.26% | 0 |  | 42,574 | +1.34% |
| Load/dict_ops | 53 | +26.19% | 25 | +19.05% | 0 |  | 1,308 | -1.36% |
| Render/dict_ops | 665 | -1.92% | 424 | -1.40% | 0 |  | 860 | -1.15% |
| Load/expressions | 26 | -27.78% | 27 | +3.85% | 0 |  | 1,279 | +0.31% |
| Render/expressions | 10 | -23.08% | 6 | -14.29% | 0 |  | 316 | +3.27% |
| Load/filters | 110 | -2.65% | 52 | -14.75% | 0 |  | 1,648 | -2.83% |
| Render/filters | 13 | -23.53% | 2 | -33.33% | 0 |  | 784 | +1.95% |
| Load/for_filter_if | 26 | -25.71% | 17 | -19.05% | 0 |  | 1,093 | +1.67% |
| Render/for_filter_if | 1,733 | -0.17% | 131 | -5.76% | 0 |  | 220 | +10.55% |
| Load/for_loop_vars | 46 | -11.54% | 29 | +31.82% | 0 |  | 1,094 | +0.92% |
| Render/for_loop_vars | 6 | -14.29% | 1 | -66.67% | 0 |  | 159 | +7.43% |
| Load/for_range | 3 | 0% | 4 | 0% | 0 |  | 756 | -0.66% |
| Render/for_range | 1 | 0% | 0 |  | 0 |  | 99 | +4.21% |
| Load/hf_deepseek_v31 | 966 | -4.36% | 981 | 0% | 0 |  | 6,197 | +1.51% |
| Render/hf_deepseek_v31 | 337 | -3.71% | 63 | -21.25% | 0 |  | 1,452 | +0.69% |
| Load/hf_gpt_oss | 4,654 | -0.87% | 3,360 | -0.53% | 0 |  | 21,615 | +0.44% |
| Render/hf_gpt_oss | 1,211 | +1.59% | 365 | -0.54% | 0 |  | 4,657 | +1.15% |
| Load/hf_mistral_small | 1,880 | +0.91% | 1,654 | +0.79% | 0 |  | 10,723 | +0.41% |
| Render/hf_mistral_small | 963 | +0.42% | 288 | -3.68% | 0 |  | 3,699 | -0.67% |
| Load/hf_qwen3 | 1,542 | +1.65% | 1,342 | +0.07% | 0 |  | 8,567 | +1.70% |
| Render/hf_qwen3 | 659 | -2.23% | 213 | -0.47% | 0 |  | 2,595 | -0.95% |
| Load/html_autoescape | 487 | -2.21% | 540 | -1.46% | 0 |  | 3,807 | -0.78% |
| Render/html_autoescape | 4,239 | -1.35% | 1,348 | -0.81% | 0 |  | 35,574 | -0.84% |
| Load/inheritance | 3 | 0% | 2 | -33.33% | 0 |  | 720 | +1.55% |
| Render/inheritance | 594 | +1.54% | 89 | -3.26% | 0 |  | 953 | +0.21% |
| Load/int_arith | 82 | -22.64% | 65 | -5.80% | 0 |  | 2,026 | +0.30% |
| Render/int_arith | 9 | -47.06% | 2 | -77.78% | 0 |  | 731 | +1.95% |
| Module/keep | 1 | -50.00% | 2 | 0% | 0 |  | 443 | +2.31% |
| Module/keep_written | 1,128 | 0% | 562 | 0% | 0 |  | 742 | -1.85% |
| Load/large_static | 1,934 | +0.05% | 913 | 0% | 0 |  | 283 | +5.99% |
| Render/large_static | 713 | 0% | 572 | 0% | 0 |  | 7 | 0% |
| RenderSink/large_static | 0 |  | 0 |  | 0 |  | 2 | -33.33% |
| RenderTo/large_static | 689 | 0% | 568 | 0% | 0 |  | 6 | 0% |
| Load/macros | 90 | -9.09% | 74 | -17.78% | 0 |  | 1,701 | -0.53% |
| Render/macros | 763 | +1.06% | 212 | -10.17% | 0 |  | 2,362 | +145.28% |
| Load/many_tags | 28,514 | -1.76% | 22,861 | +0.31% | 0 |  | 233,059 | -1.05% |
| Render/many_tags | 6,243 | +0.31% | 864 | +0.12% | 0 |  | 244 | -6.15% |
| Load/mitsuhiko_table | 37 | -17.78% | 51 | -7.27% | 0 |  | 828 | +0.61% |
| Render/mitsuhiko_table | 10,041 | -0.02% | 6,274 | -0.05% | 0 |  | 343 | -2.00% |
| RenderSink/mitsuhiko_table | 9,974 | +0.13% | 1,179 | +0.08% | 0 |  | 273 | +0.37% |
| RenderTo/mitsuhiko_table | 10,057 | +0.07% | 6,284 | +0.03% | 0 |  | 323 | +0.94% |
| Load/mitsuhiko_table_wide | 64 | -11.11% | 90 | -4.26% | 0 |  | 1,040 | +0.19% |
| Render/mitsuhiko_table_wide | 10,225 | 0% | 22,689 | +0.01% | 0 |  | 346 | +1.17% |
| RenderSink/mitsuhiko_table_wide | 10,050 | +0.01% | 1,312 | +0.31% | 0 |  | 271 | +5.86% |
| RenderTo/mitsuhiko_table_wide | 10,223 | -0.03% | 22,692 | 0% | 0 |  | 318 | +1.27% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 16 | 0% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 6 | 0% |
| RenderSink/plain_text | 0 |  | 0 |  | 0 |  | 3 | 0% |
| RenderTo/plain_text | 0 |  | 0 |  | 0 |  | 5 | 0% |
| Load/prompt_budget | 77 | 0% | 44 | -2.22% | 0 |  | 1,401 | -0.43% |
| Render/prompt_budget | 4,815 | +0.06% | 528 | +1.73% | 0 |  | 917 | -5.66% |
| Load/strings | 144 | -18.64% | 122 | -3.94% | 0 |  | 2,774 | +0.36% |
| Render/strings | 431 | +1.89% | 483 | 0% | 0 |  | 1,593 | +0.70% |
| Load/substitute | 1 | 0% | 3 | 0% | 0 |  | 121 | -4.72% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 5 | -16.67% |
| RenderSink/substitute | 0 |  | 0 |  | 0 |  | 3 | 0% |
| RenderTo/substitute | 0 |  | 0 |  | 0 |  | 5 | -16.67% |

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
