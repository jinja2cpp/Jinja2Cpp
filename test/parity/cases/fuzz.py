"""Divergences the differential fuzzing check (fuzz/differential.py) found outside the other
areas, minimized by hand. docs/tasks/0094 owns them; a fix moves its case to its feature area."""
CONTEXT = {"x": 3, "t": True, "d": {"C": 3, "a": 1, "b": 2}, "s": "abc def"}
DO = {"env": {"extensions": ["do"]}}
HEADER = {"templates": {"header.j2": "[{{ x }}]"}}
CASES = [
    # Values Python cannot iterate
    ("for_over_bool", "{% for i in t %}{{ i }}{% endfor %}"),
    ("join_bool", "{{ t|join(',') }}"),
    ("join_number", "{{ 5|join }}"),
    ("join_none", "{{ none|join }}"),
    ("join_undefined", "{{ nope|join }}|"),
    # Sequence filters on a value Python cannot iterate (0177)
    ("list_int", "{{ 5|list }}"),
    ("first_bool", "{{ t|first }}"),
    ("sort_none", "{{ none|sort }}"),
    ("length_int", "{{ 5|length }}"),
    ("dictsort_int", "{{ 5|dictsort }}"),
    # Filter arguments outside their domain
    ("dictsort_by_unknown", "{{ d|dictsort(by='bogus')|length }}"),
    ("dictsort_by_not_string", "{{ d|dictsort(by=1)|length }}"),
    ("format_extra_argument", "{{ 'x'|format(1) }}"),
    ("format_safe_extra_argument", "{{ 'x'|safe|format(1) }}"),
    ("wordwrap_float_width_long_word", "{{ s|wordwrap(2.0) }}"),
    ("wordwrap_float_width_short_words", "{{ 'ab cd'|wordwrap(2.5) }}"),
    ("wordwrap_float_width_no_break", "{{ s|wordwrap(2.0, false) }}"),
    ("wordwrap_float_width_below_one", "{{ s|wordwrap(0.5) }}"),
    ("wordwrap_negative_float_width", "{{ s|wordwrap(-0.5) }}"),
    # select_template skips names that are not strings
    ("include_list_non_string_first", "{% include [x, 'header.j2'] %}", HEADER),
    ("include_list_none_first", "{% include [none, 'header.j2'] %}", HEADER),
    ("include_list_only_non_strings", "{% include [x, none] %}", HEADER),
    ("include_list_only_non_strings_ignore_missing", "{% include [x] ignore missing %}|", HEADER),
    # Syntax Jinja2 rejects that Jinja2C++ accepts
    ("print_dangling_plus", "{{ 'a' +}}"),
    ("print_plus_before_end", "{{ 1 + }}"),
    ("print_minus_before_end", "{{ 1 -}}\n|"),
    ("block_plus_before_end", "{% if 1 +%}\nx{% endif %}", {"env": {"trim_blocks": True}}),
    ("macro_without_parentheses", "{% macro m %}hi{% endmacro %}{{ m() }}"),
    ("call_without_parentheses", "{% macro m() %}[{{ caller() }}]{% endmacro %}{% call m %}x{% endcall %}"),
    ("dict_literal_with_equals", "{{ {'a' = 1}['a'] }}"),
    ("macro_name_then_junk", "{% macro m x %}{% endmacro %}"),
    ("call_name_then_junk", "{% macro m() %}{% endmacro %}{% call m x %}{% endcall %}"),
    # Found by the generated templates of fuzz/generate_templates.py (docs/tasks/0095)
    # `lower` and `upper` are str.islower and str.isupper: at least one cased letter, no titlecase (0464)
    ("lower_test_no_cased_letter", "{{ '3' is lower }}|{{ '' is lower }}|{{ '-7' is upper }}|{{ '3a' is lower }}"),
    ("upper_test_titlecase", "{{ '\u01c5' is upper }}|{{ '\u01c5' is lower }}"),
    # A filter or test name may be dotted (`x|ns.f`); an unknown one fails only when it runs (0465)
    ("dotted_filter_name_not_run", "{% if false %}{{ x|first.upper() }}{% endif %}ok"),
    ("dotted_test_name_not_run", "{% if false %}{{ x is ns.odd }}{% endif %}ok"),
]
