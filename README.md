# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (26 so far, latest [f3af5dc](https://github.com/jinja2cpp/Jinja2Cpp/commit/f3af5dc66536716f1249fe4a9264c8e6cc10a1cf) on 2026-10-06). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 666,188 | -21.97% |  | 402 | -31.28% |  |
| [Render/chat_llama](#renderchat_llama) | 430,536 | -0.61% |  | 361 | 0% |  |
| [Load/chat_mistral](#loadchat_mistral) | 776,543 | -23.23% |  | 458 | -33.33% |  |
| [Render/chat_mistral](#renderchat_mistral) | 760,250 | -1.73% |  | 527 | -0.38% |  |
| [Load/chat_qwen](#loadchat_qwen) | 499,121 | -21.34% |  | 311 | -32.10% |  |
| [Render/chat_qwen](#renderchat_qwen) | 425,927 | -1.23% |  | 373 | 0% |  |
| [Load/config_file](#loadconfig_file) | 172,358 | -26.66% |  | 132 | -34.65% |  |
| [Render/config_file](#renderconfig_file) | 2,351,256 | -1.07% |  | 2,579 | 0% |  |
| [Load/dict_ops](#loaddict_ops) | 61,719 | -25.13% | -58.23% | 60 | -31.82% | -54.20% |
| [Render/dict_ops](#renderdict_ops) | 415,552 | -0.45% | -39.85% | 321 | -0.62% | -28.67% |
| [Load/expressions](#loadexpressions) | 84,769 | -19.50% | -58.88% | 81 | -21.36% | -44.52% |
| [Render/expressions](#renderexpressions) | 574,175 | -1.60% | -36.63% | 8 | -20.00% | -73.33% |
| [Load/filters](#loadfilters) | 128,007 | -21.32% | -53.36% | 113 | -26.14% | -50.44% |
| [Render/filters](#renderfilters) | 63,822 | +0.80% | -28.02% | 49 | 0% | -33.78% |
| [Load/for_filter_if](#loadfor_filter_if) | 52,151 | -26.70% | -61.48% | 55 | -31.25% | -55.28% |
| [Render/for_filter_if](#renderfor_filter_if) | 519,182 | -2.23% | -40.01% | 210 | 0% | -7.08% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 67,974 | -24.10% | -62.37% | 66 | -28.26% | -57.96% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 634,060 | -2.41% | -40.89% | 507 | 0% | -2.87% |
| [Load/for_range](#loadfor_range) | 27,701 | -19.01% | -51.13% | 36 | -23.40% | -35.71% |
| [Render/for_range](#renderfor_range) | 54,661 | -0.90% | -40.72% | 8 | -20.00% | -70.37% |
| [Load/html_autoescape](#loadhtml_autoescape) | 354,519 | -22.88% |  | 271 | -28.50% |  |
| [Render/html_autoescape](#renderhtml_autoescape) | 2,221,402 | -1.39% |  | 1,960 | 0% |  |
| [Load/inheritance](#loadinheritance) | 45,016 | -24.22% | -60.55% | 50 | -26.47% | -53.27% |
| [Render/inheritance](#renderinheritance) | 319,998 | -0.47% | -57.21% | 265 | 0% | -57.40% |
| [Load/large_static](#loadlarge_static) | 300,004 | -50.60% | -66.14% | 192 | -21.31% | -56.36% |
| [Render/large_static](#renderlarge_static) | 32,121 | +0.05% | -57.25% | 4 | 0% | -77.78% |
| [Load/macros](#loadmacros) | 96,784 | -20.75% | -57.54% | 94 | -25.98% | -49.46% |
| [Render/macros](#rendermacros) | 1,177,164 | -1.30% | -54.07% | 412 | 0% | -84.90% |
| [Load/many_tags](#loadmany_tags) | 16,396,184 | -25.75% | -62.87% | 9,658 | -41.68% | -71.80% |
| [Render/many_tags](#rendermany_tags) | 1,298,797 | -1.97% | -37.65% | 5 | 0% | -76.19% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 98,099 | -22.07% | -58.71% | 94 | -29.32% | -48.91% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 7,897,513 | -0.17% | -36.08% | 3,030 | 0% | -25.24% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 125,041 | -29.84% |  | 131 | -25.57% |  |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 8,908,288 | -0.15% |  | 3,051 | 0% |  |
| [Load/plain_text](#loadplain_text) | 8,750 | -8.70% | -9.57% | 15 | -16.67% | -16.67% |
| [Render/plain_text](#renderplain_text) | 2,722 | +0.07% | -59.94% | 4 | 0% | -66.67% |
| [Load/strings](#loadstrings) | 121,461 | -23.39% | -58.38% | 102 | -28.17% | -54.46% |
| [Render/strings](#renderstrings) | 1,137,546 | -1.43% | -45.77% | 1,089 | -0.18% | -48.49% |
| [Load/substitute](#loadsubstitute) | 16,787 | -15.91% | -43.78% | 26 | -21.21% | -29.73% |
| [Render/substitute](#rendersubstitute) | 4,511 | +0.04% | -48.82% | 4 | 0% | -66.67% |

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
