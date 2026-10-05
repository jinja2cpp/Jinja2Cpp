# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (25 so far, latest [698f881](https://github.com/jinja2cpp/Jinja2Cpp/commit/698f881183e55c767be4a4b451ef8243c1d70bba) on 2026-10-05). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 853,732 | 0% |  | 585 | 0% |  |
| [Render/chat_llama](#renderchat_llama) | 433,166 | -0.43% |  | 361 | 0% |  |
| [Load/chat_mistral](#loadchat_mistral) | 1,011,550 | 0% |  | 687 | 0% |  |
| [Render/chat_mistral](#renderchat_mistral) | 773,626 | -0.78% |  | 529 | 0% |  |
| [Load/chat_qwen](#loadchat_qwen) | 634,552 | 0% |  | 458 | 0% |  |
| [Render/chat_qwen](#renderchat_qwen) | 431,213 | -0.51% |  | 373 | 0% |  |
| [Load/config_file](#loadconfig_file) | 235,023 | 0% |  | 202 | 0% |  |
| [Render/config_file](#renderconfig_file) | 2,376,756 | -8.05% |  | 2,579 | -4.90% |  |
| [Load/dict_ops](#loaddict_ops) | 82,431 | 0% | -44.22% | 88 | 0% | -32.82% |
| [Render/dict_ops](#renderdict_ops) | 417,414 | -0.99% | -39.58% | 323 | 0% | -28.22% |
| [Load/expressions](#loadexpressions) | 105,306 | 0% | -48.92% | 103 | 0% | -29.45% |
| [Render/expressions](#renderexpressions) | 583,491 | -2.47% | -35.61% | 10 | 0% | -66.67% |
| [Load/filters](#loadfilters) | 162,688 | 0% | -40.72% | 153 | 0% | -32.89% |
| [Render/filters](#renderfilters) | 63,318 | -0.10% | -28.59% | 49 | 0% | -33.78% |
| [Load/for_filter_if](#loadfor_filter_if) | 71,149 | 0% | -47.45% | 80 | 0% | -34.96% |
| [Render/for_filter_if](#renderfor_filter_if) | 531,025 | -2.67% | -38.64% | 210 | 0% | -7.08% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 89,555 | 0% | -50.43% | 92 | 0% | -41.40% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 649,693 | -1.38% | -39.44% | 507 | 0% | -2.87% |
| [Load/for_range](#loadfor_range) | 34,204 | 0% | -39.66% | 47 | 0% | -16.07% |
| [Render/for_range](#renderfor_range) | 55,157 | -2.49% | -40.18% | 10 | 0% | -62.96% |
| [Load/html_autoescape](#loadhtml_autoescape) | 459,718 | 0% |  | 379 | 0% |  |
| [Render/html_autoescape](#renderhtml_autoescape) | 2,252,826 | -0.82% |  | 1,960 | 0% |  |
| [Load/inheritance](#loadinheritance) | 59,400 | 0% | -47.95% | 68 | 0% | -36.45% |
| [Render/inheritance](#renderinheritance) | 321,524 | -28.59% | -57.01% | 265 | -15.87% | -57.40% |
| [Load/large_static](#loadlarge_static) | 607,339 | 0% | -31.45% | 244 | 0% | -44.55% |
| [Render/large_static](#renderlarge_static) | 32,106 | +1.27% | -57.27% | 4 | 0% | -77.78% |
| [Load/macros](#loadmacros) | 122,129 | 0% | -46.42% | 127 | 0% | -31.72% |
| [Render/macros](#rendermacros) | 1,192,716 | -1.85% | -53.47% | 412 | 0% | -84.90% |
| [Load/many_tags](#loadmany_tags) | 22,082,540 | 0% | -50.00% | 16,559 | 0% | -51.65% |
| [Render/many_tags](#rendermany_tags) | 1,324,911 | -2.31% | -36.39% | 5 | 0% | -76.19% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 125,874 | 0% | -47.01% | 133 | 0% | -27.72% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 7,911,087 | -2.13% | -35.97% | 3,030 | 0% | -25.24% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 178,213 | 0% |  | 176 | 0% |  |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 8,921,595 | -1.89% |  | 3,051 | 0% |  |
| [Load/plain_text](#loadplain_text) | 9,584 | 0% | -0.95% | 18 | 0% | 0% |
| [Render/plain_text](#renderplain_text) | 2,720 | +0.11% | -59.97% | 4 | 0% | -66.67% |
| [Load/strings](#loadstrings) | 158,542 | 0% | -45.68% | 142 | 0% | -36.61% |
| [Render/strings](#renderstrings) | 1,154,014 | -0.53% | -44.98% | 1,091 | 0% | -48.39% |
| [Load/substitute](#loadsubstitute) | 19,962 | 0% | -33.14% | 33 | 0% | -10.81% |
| [Render/substitute](#rendersubstitute) | 4,509 | +0.45% | -48.84% | 4 | 0% | -66.67% |

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
