"""Whitespace control: -/+ modifiers, trim_blocks, lstrip_blocks, keep_trailing_newline."""
TB = {"trim_blocks": True}
LB = {"lstrip_blocks": True}
BOTH = {"trim_blocks": True, "lstrip_blocks": True}
CASES = [
    ("expr_minus", "a {{- ' b ' -}} c"),
    ("block_minus", "a  {%- if true %} x {% endif -%}  b"),
    ("comment_minus", "a  {#- c -#}  b"),
    ("minus_strips_newlines", "a\n\n  {%- if true -%}\n\n  x\n{%- endif %}"),
    ("block_newline_default", "{% if true %}\nx\n{% endif %}\ny"),
    ("trailing_newline_default", "x\n"),
    ("trailing_newlines_two", "x\n\n"),
    ("trailing_newline_after_block", "{% if true %}\n  x\n{% endif %}\n"),
    ("keep_trailing_newline", "x\n", {"env": {"keep_trailing_newline": True}}),
    ("trim_blocks", "{% if true %}\nx\n{% endif %}\ny", {"env": TB}),
    ("trim_blocks_comment", "{# c #}\nx", {"env": TB}),
    ("trim_blocks_expression_untouched", "{{ 'a' }}\nb", {"env": TB}),
    ("trim_blocks_crlf", "{% if true %}\r\nx{% endif %}", {"env": TB}),
    ("lstrip_blocks", "  {% if true %}x{% endif %}", {"env": LB}),
    ("lstrip_blocks_tabs", "\t {% if true %}x{% endif %}", {"env": LB}),
    ("lstrip_blocks_not_after_text", "a {% if true %}x{% endif %}", {"env": LB}),
    ("lstrip_blocks_comment", "  {# c #}x", {"env": LB}),
    ("lstrip_blocks_expression_untouched", "  {{ 'x' }}", {"env": LB}),
    ("both", "<ul>\n  {% for i in [1, 2] %}\n  <li>{{ i }}</li>\n  {% endfor %}\n</ul>", {"env": BOTH}),
    ("plus_disables_lstrip", "  {%+ if true %}x{% endif %}", {"env": LB}),
    ("plus_disables_trim", "{% if true +%}\nx{% endif %}", {"env": TB}),
    ("minus_with_trim", "a\n  {%- if true %}\nx\n{% endif %}", {"env": BOTH}),
    ("raw_whitespace", "{%- raw -%}  {{ x }}  {%- endraw -%}"),
    ("raw_trim_blocks", "{% raw %}\nx{% endraw %}\ny", {"env": TB}),
    ("macro_whitespace", "{% macro m() %}\n  m\n{% endmacro %}[{{ m() }}]"),
    ("set_block_whitespace", "{% set v %}\n  v\n{% endset %}[{{ v }}]", {"env": BOTH}),
    ("crlf_text", "a\r\nb"),
    ("newline_sequence", "a\nb", {"env": {"newline_sequence": "\r\n"}}),
]
