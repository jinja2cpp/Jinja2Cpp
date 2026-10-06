# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (33 so far, latest [449e8c6](https://github.com/jinja2cpp/Jinja2Cpp/commit/449e8c623ab51bb90a96257500bd3a6a2d502b3b) on 2026-10-06). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 669,087 | +0.06% |  | 403 | +0.25% |  | 39,408 | +0.14% | +0.84% |
| [Render/chat_llama](#renderchat_llama) | 432,083 | +0.29% |  | 359 | 0% |  | 13,784 | 0% | -4.01% |
| [Load/chat_mistral](#loadchat_mistral) | 778,446 | -0.12% |  | 459 | +0.22% |  | 45,312 | -0.02% | +1.49% |
| [Render/chat_mistral](#renderchat_mistral) | 759,762 | +0.02% |  | 519 | 0% |  | 12,616 | 0% | -4.48% |
| [Load/chat_qwen](#loadchat_qwen) | 501,806 | -0.02% |  | 312 | +0.32% |  | 27,816 | +0.20% | +1.79% |
| [Render/chat_qwen](#renderchat_qwen) | 427,538 | +0.18% |  | 371 | 0% |  | 7,928 | 0% | -6.77% |
| [Load/config_file](#loadconfig_file) | 173,948 | -0.17% |  | 133 | +0.76% |  | 10,584 | +0.23% | +0.23% |
| [Render/config_file](#renderconfig_file) | 2,218,731 | -0.21% |  | 2,033 | 0% |  | 22,624 | +0.07% | -4.97% |
| [Load/dict_ops](#loaddict_ops) | 62,474 | +0.70% | -57.72% | 61 | +1.67% | -53.44% | 4,280 | +0.56% | +0.56% |
| [Render/dict_ops](#renderdict_ops) | 412,828 | +0.03% | -40.24% | 319 | 0% | -29.11% | 34,136 | +0.05% | -1.61% |
| [Load/expressions](#loadexpressions) | 84,678 | -0.04% | -58.92% | 82 | +1.23% | -43.84% | 6,272 | +0.38% | +2.22% |
| [Render/expressions](#renderexpressions) | 555,197 | -0.01% | -38.73% | 6 | 0% | -80.00% | 2,968 | 0% | -16.25% |
| [Load/filters](#loadfilters) | 129,173 | +0.13% | -52.93% | 114 | +0.88% | -50.00% | 11,440 | +0.21% | +0.21% |
| [Render/filters](#renderfilters) | 63,469 | +0.32% | -28.42% | 47 | 0% | -36.49% | 4,336 | 0% | -11.73% |
| [Load/for_filter_if](#loadfor_filter_if) | 52,961 | +0.87% | -60.88% | 56 | +1.82% | -54.47% | 3,488 | +0.69% | +1.16% |
| [Render/for_filter_if](#renderfor_filter_if) | 518,922 | +0.08% | -40.04% | 208 | 0% | -7.96% | 3,064 | 0% | -15.82% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 68,349 | -0.06% | -62.17% | 67 | +1.52% | -57.32% | 4,560 | +0.88% | +0.88% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 621,719 | +0.34% | -42.04% | 505 | 0% | -3.26% | 2,192 | 0% | -20.81% |
| [Load/for_range](#loadfor_range) | 28,074 | +0.59% | -50.47% | 37 | +2.78% | -33.93% | 2,024 | +1.20% | +1.20% |
| [Render/for_range](#renderfor_range) | 53,955 | 0% | -41.49% | 6 | 0% | -77.78% | 936 | 0% | -38.10% |
| [Load/html_autoescape](#loadhtml_autoescape) | 358,788 | +0.09% |  | 274 | +0.37% |  | 24,912 | +0.10% | +0.94% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,826,442 | +0.03% |  | 1,080 | 0% |  | 39,152 | 0% | -3.05% |
| [Load/inheritance](#loadinheritance) | 45,433 | +0.31% | -60.19% | 51 | +2.00% | -52.34% | 2,896 | +0.84% | +0.84% |
| [Render/inheritance](#renderinheritance) | 285,773 | +0.07% | -61.79% | 157 | 0% | -74.76% | 4,288 | 0% | -28.72% |
| [Load/large_static](#loadlarge_static) | 301,465 | +0.04% | -65.97% | 193 | +0.52% | -56.14% | 53,392 | +0.01% | +0.01% |
| [Render/large_static](#renderlarge_static) | 31,873 | -0.05% | -57.58% | 2 | 0% | -88.89% | 36,656 | 0% | -1.55% |
| [Load/macros](#loadmacros) | 97,979 | +0.10% | -57.02% | 95 | +1.06% | -48.92% | 6,256 | +0.39% | +0.64% |
| [Render/macros](#rendermacros) | 1,184,201 | +0.01% | -53.80% | 410 | 0% | -84.98% | 14,144 | 0% | -3.91% |
| [Load/many_tags](#loadmany_tags) | 16,571,019 | +0.67% | -62.48% | 9,659 | +0.01% | -71.80% | 1,060,112 | 0% | +0.46% |
| [Render/many_tags](#rendermany_tags) | 1,292,671 | 0% | -37.94% | 3 | 0% | -85.71% | 5,736 | 0% | -9.13% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 98,654 | 0% | -58.47% | 95 | +1.06% | -48.37% | 6,104 | +0.66% | +0.66% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 7,876,217 | 0% | -36.26% | 3,028 | 0% | -25.29% | 346,432 | 0% | -0.17% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 125,761 | -0.13% |  | 132 | +0.76% |  | 8,272 | +0.10% | +0.10% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 8,887,178 | 0% |  | 3,049 | 0% |  | 1,380,416 | 0% | -0.04% |
| [Load/plain_text](#loadplain_text) | 9,006 | +1.88% | -6.92% | 16 | +6.67% | -11.11% | 1,144 | +2.14% | +2.14% |
| [Render/plain_text](#renderplain_text) | 2,327 | 0% | -65.75% | 2 | 0% | -83.33% | 144 | 0% | -80.00% |
| [Load/strings](#loadstrings) | 123,722 | +0.13% | -57.61% | 106 | +0.95% | -52.68% | 9,336 | +0.09% | +3.27% |
| [Render/strings](#renderstrings) | 1,114,854 | +0.07% | -46.85% | 1,087 | 0% | -48.58% | 17,352 | 0% | -3.21% |
| [Load/substitute](#loadsubstitute) | 17,109 | +0.98% | -42.70% | 27 | +3.85% | -27.03% | 1,544 | +1.58% | +1.58% |
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
