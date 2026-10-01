"""Environment options beyond whitespace and autoescape: delimiters, line statements, extensions."""
CONTEXT = {"x": 3, "l": [1, 2]}
CASES = [
    ("custom_variable_delimiters", "<<= x >>", {"env": {"variable_start_string": "<<=", "variable_end_string": ">>"}}),
    ("custom_block_delimiters", "<% if true %>y<% endif %>", {"env": {"block_start_string": "<%", "block_end_string": "%>"}}),
    ("custom_comment_delimiters", "a<# c #>b", {"env": {"comment_start_string": "<#", "comment_end_string": "#>"}}),
    ("latex_style", r"\VAR{x}\BLOCK{if true}y\BLOCK{endif}",
     {"env": {"block_start_string": r"\BLOCK{", "block_end_string": "}",
              "variable_start_string": r"\VAR{", "variable_end_string": "}"}}),
    ("line_statement", "# for i in l\n{{ i }}\n# endfor\n", {"env": {"line_statement_prefix": "#"}}),
    ("line_comment", "a ## comment\nb", {"env": {"line_comment_prefix": "##"}}),
    ("i18n_trans", "{% trans %}Hello {{ x }}{% endtrans %}", {"env": {"extensions": ["i18n"]}}),
    ("i18n_trans_vars", "{% trans n=x %}{{ n }} item{% pluralize %}{{ n }} items{% endtrans %}", {"env": {"extensions": ["i18n"]}}),
    ("i18n_gettext", "{{ _('Hello') }}|{{ gettext('Hi %(n)s', n=1) }}", {"env": {"extensions": ["i18n"]}}),
    ("loopcontrols_without_extension", "{% for i in l %}{% break %}{% endfor %}"),
]
