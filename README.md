# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (32 so far, latest [448bb7c](https://github.com/jinja2cpp/Jinja2Cpp/commit/448bb7cfdde622fd50a49b247bbe1d7f0eadee85) on 2026-10-06). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 668,674 | -0.05% |  | 402 | 0% |  | 39,352 | 0% | +0.70% |
| [Render/chat_llama](#renderchat_llama) | 430,838 | 0% |  | 359 | 0% |  | 13,784 | 0% | -4.01% |
| [Load/chat_mistral](#loadchat_mistral) | 779,363 | -0.03% |  | 458 | 0% |  | 45,320 | 0% | +1.51% |
| [Render/chat_mistral](#renderchat_mistral) | 759,576 | 0% |  | 519 | 0% |  | 12,616 | 0% | -4.48% |
| [Load/chat_qwen](#loadchat_qwen) | 501,924 | -0.02% |  | 311 | 0% |  | 27,760 | 0% | +1.58% |
| [Render/chat_qwen](#renderchat_qwen) | 426,775 | 0% |  | 371 | 0% |  | 7,928 | 0% | -6.77% |
| [Load/config_file](#loadconfig_file) | 174,245 | -0.06% |  | 132 | 0% |  | 10,560 | 0% | 0% |
| [Render/config_file](#renderconfig_file) | 2,223,407 | 0% |  | 2,033 | 0% |  | 22,608 | 0% | -5.04% |
| [Load/dict_ops](#loaddict_ops) | 62,042 | -0.06% | -58.02% | 60 | 0% | -54.20% | 4,256 | 0% | 0% |
| [Render/dict_ops](#renderdict_ops) | 412,705 | 0% | -40.26% | 319 | 0% | -29.11% | 34,120 | 0% | -1.66% |
| [Load/expressions](#loadexpressions) | 84,715 | 0% | -58.91% | 81 | 0% | -44.52% | 6,248 | 0% | +1.83% |
| [Render/expressions](#renderexpressions) | 555,237 | 0% | -38.72% | 6 | 0% | -80.00% | 2,968 | 0% | -16.25% |
| [Load/filters](#loadfilters) | 129,000 | -0.36% | -52.99% | 113 | 0% | -50.44% | 11,416 | 0% | 0% |
| [Render/filters](#renderfilters) | 63,266 | 0% | -28.65% | 47 | 0% | -36.49% | 4,336 | 0% | -11.73% |
| [Load/for_filter_if](#loadfor_filter_if) | 52,503 | 0% | -61.22% | 55 | 0% | -55.28% | 3,464 | 0% | +0.46% |
| [Render/for_filter_if](#renderfor_filter_if) | 518,519 | 0% | -40.09% | 208 | 0% | -7.96% | 3,064 | 0% | -15.82% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 68,391 | 0% | -62.14% | 66 | 0% | -57.96% | 4,520 | 0% | 0% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 619,606 | 0% | -42.24% | 505 | 0% | -3.26% | 2,192 | 0% | -20.81% |
| [Load/for_range](#loadfor_range) | 27,908 | 0% | -50.76% | 36 | 0% | -35.71% | 2,000 | 0% | 0% |
| [Render/for_range](#renderfor_range) | 53,955 | -0.02% | -41.49% | 6 | 0% | -77.78% | 936 | 0% | -38.10% |
| [Load/html_autoescape](#loadhtml_autoescape) | 358,480 | -0.05% |  | 273 | 0% |  | 24,888 | 0% | +0.84% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,825,955 | 0% |  | 1,080 | 0% |  | 39,152 | 0% | -3.05% |
| [Load/inheritance](#loadinheritance) | 45,295 | 0% | -60.31% | 50 | 0% | -53.27% | 2,872 | 0% | 0% |
| [Render/inheritance](#renderinheritance) | 285,583 | 0% | -61.81% | 157 | 0% | -74.76% | 4,288 | 0% | -28.72% |
| [Load/large_static](#loadlarge_static) | 301,358 | 0% | -65.99% | 192 | 0% | -56.36% | 53,384 | 0% | 0% |
| [Render/large_static](#renderlarge_static) | 31,888 | 0% | -57.56% | 2 | 0% | -88.89% | 36,656 | 0% | -1.55% |
| [Load/macros](#loadmacros) | 97,879 | 0% | -57.06% | 94 | 0% | -49.46% | 6,232 | 0% | +0.26% |
| [Render/macros](#rendermacros) | 1,184,121 | 0% | -53.80% | 410 | 0% | -84.98% | 14,144 | 0% | -3.91% |
| [Load/many_tags](#loadmany_tags) | 16,460,875 | -0.12% | -62.73% | 9,658 | 0% | -71.80% | 1,060,120 | 0% | +0.46% |
| [Render/many_tags](#rendermany_tags) | 1,292,671 | 0% | -37.94% | 3 | 0% | -85.71% | 5,736 | 0% | -9.13% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 98,653 | -0.01% | -58.47% | 94 | 0% | -48.91% | 6,064 | 0% | 0% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 7,876,431 | 0% | -36.26% | 3,028 | 0% | -25.29% | 346,432 | 0% | -0.17% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 125,919 | 0% |  | 131 | 0% |  | 8,264 | 0% | 0% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 8,887,088 | 0% |  | 3,049 | 0% |  | 1,380,416 | 0% | -0.04% |
| [Load/plain_text](#loadplain_text) | 8,840 | 0% | -8.64% | 15 | 0% | -16.67% | 1,120 | 0% | 0% |
| [Render/plain_text](#renderplain_text) | 2,327 | 0% | -65.75% | 2 | 0% | -83.33% | 144 | 0% | -80.00% |
| [Load/strings](#loadstrings) | 123,559 | -0.20% | -57.66% | 105 | 0% | -53.12% | 9,328 | 0% | +3.19% |
| [Render/strings](#renderstrings) | 1,114,100 | -0.01% | -46.88% | 1,087 | 0% | -48.58% | 17,352 | 0% | -3.21% |
| [Load/substitute](#loadsubstitute) | 16,943 | 0% | -43.25% | 26 | 0% | -29.73% | 1,520 | 0% | 0% |
| [Render/substitute](#rendersubstitute) | 4,120 | 0% | -53.26% | 2 | 0% | -83.33% | 144 | 0% | -80.00% |

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
