# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (34 so far, latest [8bd9d1f](https://github.com/jinja2cpp/Jinja2Cpp/commit/8bd9d1f8e21bd7d0444c96b6b80de6dcdbac4245) on 2026-10-06). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 669,016 | -0.01% |  | 403 | 0% |  | 39,408 | 0% | +0.84% |
| [Render/chat_llama](#renderchat_llama) | 432,202 | +0.03% |  | 359 | 0% |  | 13,784 | 0% | -4.01% |
| [Load/chat_mistral](#loadchat_mistral) | 778,303 | -0.02% |  | 459 | 0% |  | 45,312 | 0% | +1.49% |
| [Render/chat_mistral](#renderchat_mistral) | 759,834 | +0.01% |  | 519 | 0% |  | 12,616 | 0% | -4.48% |
| [Load/chat_qwen](#loadchat_qwen) | 501,672 | -0.03% |  | 312 | 0% |  | 27,816 | 0% | +1.79% |
| [Render/chat_qwen](#renderchat_qwen) | 427,624 | +0.02% |  | 371 | 0% |  | 7,928 | 0% | -6.77% |
| [Load/config_file](#loadconfig_file) | 173,888 | -0.03% |  | 133 | 0% |  | 10,584 | 0% | +0.23% |
| [Render/config_file](#renderconfig_file) | 2,217,635 | -0.05% |  | 2,033 | 0% |  | 22,624 | 0% | -4.97% |
| [Load/dict_ops](#loaddict_ops) | 62,465 | -0.01% | -57.73% | 61 | 0% | -53.44% | 4,280 | 0% | +0.56% |
| [Render/dict_ops](#renderdict_ops) | 412,855 | +0.01% | -40.24% | 319 | 0% | -29.11% | 34,136 | 0% | -1.61% |
| [Load/expressions](#loadexpressions) | 84,680 | 0% | -58.92% | 82 | 0% | -43.84% | 6,272 | 0% | +2.22% |
| [Render/expressions](#renderexpressions) | 555,577 | +0.07% | -38.69% | 6 | 0% | -80.00% | 2,968 | 0% | -16.25% |
| [Load/filters](#loadfilters) | 129,178 | 0% | -52.93% | 114 | 0% | -50.00% | 11,440 | 0% | +0.21% |
| [Render/filters](#renderfilters) | 63,465 | -0.01% | -28.42% | 47 | 0% | -36.49% | 4,336 | 0% | -11.73% |
| [Load/for_filter_if](#loadfor_filter_if) | 52,941 | -0.04% | -60.90% | 56 | 0% | -54.47% | 3,488 | 0% | +1.16% |
| [Render/for_filter_if](#renderfor_filter_if) | 519,203 | +0.05% | -40.01% | 208 | 0% | -7.96% | 3,064 | 0% | -15.82% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 68,326 | -0.03% | -62.18% | 67 | 0% | -57.32% | 4,560 | 0% | +0.88% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 619,395 | -0.37% | -42.26% | 505 | 0% | -3.26% | 2,192 | 0% | -20.81% |
| [Load/for_range](#loadfor_range) | 28,074 | 0% | -50.47% | 37 | 0% | -33.93% | 2,024 | 0% | +1.20% |
| [Render/for_range](#renderfor_range) | 52,331 | -3.01% | -43.25% | 6 | 0% | -77.78% | 936 | 0% | -38.10% |
| [Load/html_autoescape](#loadhtml_autoescape) | 358,662 | -0.04% |  | 274 | 0% |  | 24,912 | 0% | +0.94% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,826,594 | +0.01% |  | 1,080 | 0% |  | 39,152 | 0% | -3.05% |
| [Load/inheritance](#loadinheritance) | 45,435 | 0% | -60.19% | 51 | 0% | -52.34% | 2,896 | 0% | +0.84% |
| [Render/inheritance](#renderinheritance) | 284,286 | -0.52% | -61.99% | 157 | 0% | -74.76% | 4,288 | 0% | -28.72% |
| [Load/large_static](#loadlarge_static) | 301,465 | 0% | -65.97% | 193 | 0% | -56.14% | 53,392 | 0% | +0.01% |
| [Render/large_static](#renderlarge_static) | 31,979 | +0.33% | -57.44% | 2 | 0% | -88.89% | 36,656 | 0% | -1.55% |
| [Load/macros](#loadmacros) | 97,968 | -0.01% | -57.02% | 95 | 0% | -48.92% | 6,256 | 0% | +0.64% |
| [Render/macros](#rendermacros) | 1,181,705 | -0.21% | -53.90% | 410 | 0% | -84.98% | 14,144 | 0% | -3.91% |
| [Load/many_tags](#loadmany_tags) | 16,567,119 | -0.02% | -62.49% | 9,659 | 0% | -71.80% | 1,060,112 | 0% | +0.46% |
| [Render/many_tags](#rendermany_tags) | 1,293,876 | +0.09% | -37.88% | 3 | 0% | -85.71% | 5,736 | 0% | -9.13% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 98,660 | +0.01% | -58.47% | 95 | 0% | -48.37% | 6,104 | 0% | +0.66% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 7,771,739 | -1.33% | -37.10% | 3,028 | 0% | -25.29% | 346,432 | 0% | -0.17% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 125,767 | 0% |  | 132 | 0% |  | 8,272 | 0% | +0.10% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 8,123,310 | -8.60% |  | 3,049 | 0% |  | 1,380,416 | 0% | -0.04% |
| [Load/plain_text](#loadplain_text) | 9,006 | 0% | -6.92% | 16 | 0% | -11.11% | 1,144 | 0% | +2.14% |
| [Render/plain_text](#renderplain_text) | 2,333 | +0.26% | -65.66% | 2 | 0% | -83.33% | 144 | 0% | -80.00% |
| [Load/strings](#loadstrings) | 123,714 | -0.01% | -57.61% | 106 | 0% | -52.68% | 9,336 | 0% | +3.27% |
| [Render/strings](#renderstrings) | 1,114,972 | +0.01% | -46.84% | 1,087 | 0% | -48.58% | 17,352 | 0% | -3.21% |
| [Load/substitute](#loadsubstitute) | 17,109 | 0% | -42.70% | 27 | 0% | -27.03% | 1,544 | 0% | +1.58% |
| [Render/substitute](#rendersubstitute) | 4,111 | -0.22% | -53.36% | 2 | 0% | -83.33% | 144 | 0% | -80.00% |

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

### Load/large_static

![Load/large_static](charts/Load_large_static.svg)

### Render/large_static

![Render/large_static](charts/Render_large_static.svg)

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

### Load/mitsuhiko_table_wide

![Load/mitsuhiko_table_wide](charts/Load_mitsuhiko_table_wide.svg)

### Render/mitsuhiko_table_wide

![Render/mitsuhiko_table_wide](charts/Render_mitsuhiko_table_wide.svg)

### Load/plain_text

![Load/plain_text](charts/Load_plain_text.svg)

### Render/plain_text

![Render/plain_text](charts/Render_plain_text.svg)

### Load/strings

![Load/strings](charts/Load_strings.svg)

### Render/strings

![Render/strings](charts/Render_strings.svg)

### Load/substitute

![Load/substitute](charts/Load_substitute.svg)

### Render/substitute

![Render/substitute](charts/Render_substitute.svg)
