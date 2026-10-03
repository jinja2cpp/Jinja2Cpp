"""Divergences the differential fuzzing check (fuzz/differential.py) found outside the other
areas, minimized by hand. docs/tasks/0094 owns them; a fix moves its case to its feature area."""
CONTEXT = {"x": 3, "t": True, "d": {"C": 3, "a": 1, "b": 2}, "s": "abc def"}
DO = {"env": {"extensions": ["do"]}}
HEADER = {"templates": {"header.j2": "[{{ x }}]"}}
CASES = [
    # Values Python cannot iterate
    ("for_over_bool", "{% for i in t %}{{ i }}{% endfor %}"),
    ("join_bool", "{{ t|join(',') }}"),
    # Filter arguments outside their domain
    ("dictsort_by_unknown", "{{ d|dictsort(by='bogus')|length }}"),
    ("format_extra_argument", "{{ 'x'|format(1) }}"),
    ("format_safe_extra_argument", "{{ 'x'|safe|format(1) }}"),
    ("wordwrap_float_width_long_word", "{{ s|wordwrap(2.0) }}"),
    # select_template skips names that are not strings
    ("include_list_non_string_first", "{% include [x, 'header.j2'] %}", HEADER),
    # A list that contains itself
    ("list_contains_itself", "{% set l = [] %}{% do l.append(l) %}{{ l|length }}", DO),
    # Syntax Jinja2 rejects that Jinja2C++ accepts
    ("print_dangling_plus", "{{ 'a' +}}"),
    ("macro_without_parentheses", "{% macro m %}hi{% endmacro %}{{ m() }}"),
    ("call_without_parentheses", "{% macro m() %}[{{ caller() }}]{% endmacro %}{% call m %}x{% endcall %}"),
    ("dict_literal_with_equals", "{{ {'a' = 1}['a'] }}"),
]
