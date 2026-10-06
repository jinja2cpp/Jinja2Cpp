# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (29 so far, latest [52d4543](https://github.com/jinja2cpp/Jinja2Cpp/commit/52d454318aec68bdf239c2c9e8e22bf6eb5f9675) on 2026-10-06). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 668,979 | 0% |  | 402 | 0% |  | 39,352 | 0% | +0.70% |
| [Render/chat_llama](#renderchat_llama) | 433,129 | 0% |  | 361 | 0% |  | 14,360 | 0% | 0% |
| [Load/chat_mistral](#loadchat_mistral) | 779,618 | 0% |  | 458 | 0% |  | 45,320 | 0% | +1.51% |
| [Render/chat_mistral](#renderchat_mistral) | 764,614 | 0% |  | 527 | 0% |  | 13,192 | 0% | -0.12% |
| [Load/chat_qwen](#loadchat_qwen) | 502,028 | 0% |  | 311 | 0% |  | 27,760 | 0% | +1.58% |
| [Render/chat_qwen](#renderchat_qwen) | 429,000 | 0% |  | 373 | 0% |  | 8,504 | 0% | 0% |
| [Load/config_file](#loadconfig_file) | 174,370 | 0% |  | 132 | 0% |  | 10,560 | 0% | 0% |
| [Render/config_file](#renderconfig_file) | 2,367,366 | 0% |  | 2,579 | 0% |  | 23,808 | 0% | 0% |
| [Load/dict_ops](#loaddict_ops) | 62,085 | 0% | -57.99% | 60 | 0% | -54.20% | 4,256 | 0% | 0% |
| [Render/dict_ops](#renderdict_ops) | 417,815 | 0% | -39.52% | 321 | 0% | -28.67% | 34,696 | 0% | 0% |
| [Load/expressions](#loadexpressions) | 84,717 | 0% | -58.90% | 81 | 0% | -44.52% | 6,248 | 0% | +1.83% |
| [Render/expressions](#renderexpressions) | 578,321 | 0% | -36.18% | 8 | 0% | -73.33% | 3,544 | 0% | 0% |
| [Load/filters](#loadfilters) | 129,480 | 0% | -52.82% | 113 | 0% | -50.44% | 11,416 | 0% | 0% |
| [Render/filters](#renderfilters) | 64,084 | 0% | -27.73% | 49 | 0% | -33.78% | 4,912 | 0% | 0% |
| [Load/for_filter_if](#loadfor_filter_if) | 52,503 | 0% | -61.22% | 55 | 0% | -55.28% | 3,464 | 0% | +0.46% |
| [Render/for_filter_if](#renderfor_filter_if) | 520,446 | 0% | -39.87% | 210 | 0% | -7.08% | 3,640 | 0% | 0% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 68,393 | 0% | -62.14% | 66 | 0% | -57.96% | 4,520 | 0% | 0% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 637,102 | 0% | -40.61% | 507 | 0% | -2.87% | 2,768 | 0% | 0% |
| [Load/for_range](#loadfor_range) | 27,908 | 0% | -50.76% | 36 | 0% | -35.71% | 2,000 | 0% | 0% |
| [Render/for_range](#renderfor_range) | 54,709 | 0% | -40.67% | 8 | 0% | -70.37% | 1,512 | 0% | 0% |
| [Load/html_autoescape](#loadhtml_autoescape) | 358,652 | 0% |  | 273 | 0% |  | 24,888 | 0% | +0.84% |
| [Render/html_autoescape](#renderhtml_autoescape) | 2,218,797 | 0% |  | 1,960 | 0% |  | 40,384 | 0% | 0% |
| [Load/inheritance](#loadinheritance) | 45,297 | 0% | -60.31% | 50 | 0% | -53.27% | 2,872 | 0% | 0% |
| [Render/inheritance](#renderinheritance) | 321,179 | 0% | -57.05% | 265 | 0% | -57.40% | 6,016 | 0% | 0% |
| [Load/large_static](#loadlarge_static) | 301,358 | 0% | -65.99% | 192 | 0% | -56.36% | 53,384 | 0% | 0% |
| [Render/large_static](#renderlarge_static) | 32,157 | 0% | -57.20% | 4 | 0% | -77.78% | 37,232 | 0% | 0% |
| [Load/macros](#loadmacros) | 97,882 | 0% | -57.06% | 94 | 0% | -49.46% | 6,232 | 0% | +0.26% |
| [Render/macros](#rendermacros) | 1,179,840 | 0% | -53.97% | 412 | 0% | -84.90% | 14,720 | 0% | 0% |
| [Load/many_tags](#loadmany_tags) | 16,486,075 | 0% | -62.67% | 9,658 | 0% | -71.80% | 1,060,120 | 0% | +0.46% |
| [Render/many_tags](#rendermany_tags) | 1,299,727 | 0% | -37.60% | 5 | 0% | -76.19% | 6,312 | 0% | 0% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 98,659 | 0% | -58.47% | 94 | 0% | -48.91% | 6,064 | 0% | 0% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 7,915,818 | 0% | -35.94% | 3,030 | 0% | -25.24% | 347,008 | 0% | 0% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 125,925 | 0% |  | 131 | 0% |  | 8,264 | 0% | 0% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 8,926,470 | 0% |  | 3,051 | 0% |  | 1,380,992 | 0% | 0% |
| [Load/plain_text](#loadplain_text) | 8,840 | 0% | -8.64% | 15 | 0% | -16.67% | 1,120 | 0% | 0% |
| [Render/plain_text](#renderplain_text) | 2,746 | 0% | -59.59% | 4 | 0% | -66.67% | 720 | 0% | 0% |
| [Load/strings](#loadstrings) | 123,803 | 0% | -57.58% | 105 | 0% | -53.12% | 9,328 | 0% | +3.19% |
| [Render/strings](#renderstrings) | 1,127,411 | 0% | -46.25% | 1,089 | 0% | -48.49% | 17,928 | 0% | 0% |
| [Load/substitute](#loadsubstitute) | 16,943 | 0% | -43.25% | 26 | 0% | -29.73% | 1,520 | 0% | 0% |
| [Render/substitute](#rendersubstitute) | 4,535 | 0% | -48.55% | 4 | 0% | -66.67% | 720 | 0% | 0% |

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
