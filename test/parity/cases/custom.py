"""Filters, tests and finalize registered by the application (env.filters, env.tests, finalize).

The functions are fixed pairs: FILTERS, TESTS and FINALIZE in generate.py, and their C++
counterparts in parity_test.cpp.
"""
CONTEXT = {"x": 3, "s": "ab", "l": [1, 2, 3]}
F = {"env": {"filters": ["double", "wrap", "upper"]}}
T = {"env": {"tests": ["big", "even"]}}
CASES = [
    ("filter_default_arg", "{{ x|double }}", F),
    ("filter_positional_arg", "{{ x|double(3) }}", F),
    ("filter_keyword_args", "{{ x|wrap(right='>', left='<') }}", F),
    ("filter_chain", "{{ x|double|wrap }}", F),
    ("filter_chain_with_builtin", "{{ s|wrap|length }}|{{ s|reverse|wrap }}", F),
    ("filter_replaces_builtin", "{{ s|upper }}", F),
    ("filter_builtin_when_not_registered", "{{ s|upper }}"),
    ("filter_block", "{% filter wrap %}{{ x }}{% endfilter %}", F),
    ("filter_in_set", "{% set y = x|double %}{{ y }}", F),
    ("filter_in_loop", "{% for i in l %}{{ i|double }},{% endfor %}", F),
    ("filter_in_map", "{% for i in l|map('double', 10) %}{{ i }},{% endfor %}", F),
    ("filter_in_macro", "{% macro m(v) %}{{ v|wrap }}{% endmacro %}{{ m(x) }}", F),
    ("filter_in_include", "{% include 'inc' %}", {**F, "templates": {"inc": "{{ x|double }}"}}),
    ("filter_is_filter", "{{ 'T' if 'double' is filter else 'F' }}{{ 'T' if 'nope' is filter else 'F' }}", F),
    ("test_default_arg", "{{ 'T' if x is big else 'F' }}{{ 'T' if 11 is big else 'F' }}", T),
    ("test_arg", "{{ 'T' if x is big(2) else 'F' }}", T),
    ("test_arg_without_parens", "{{ 'T' if x is big 2 else 'F' }}", T),
    ("test_negated", "{{ 'T' if x is not big else 'F' }}", T),
    ("test_replaces_builtin", "{{ 'T' if 3 is even else 'F' }}{{ 'T' if 2 is even else 'F' }}", T),
    ("test_in_select", "{% for i in l|select('big', 1) %}{{ i }},{% endfor %}", T),
    ("test_in_reject", "{% for i in l|reject('even') %}{{ i }},{% endfor %}", T),
    ("test_is_test", "{{ 'T' if 'big' is test else 'F' }}", T),
    ("finalize_none", "{{ none }}|{{ x }}|{{ missing }}", {"env": {"finalize": "none_to_empty"}}),
    ("finalize_expressions_only", "a{{ x }}b{{ 'p' ~ 'q' }}{% for i in l %}{{ i }}{% endfor %}", {"env": {"finalize": "brackets"}}),
    ("finalize_set_block", "{% set z %}q{{ 2 }}{% endset %}{{ z }}", {"env": {"finalize": "brackets"}}),
    ("finalize_after_filter", "{{ x|double }}", {"env": {"finalize": "brackets", "filters": ["double"]}}),
    ("finalize_macro_call", "{% macro m() %}m{{ 1 }}{% endmacro %}{{ m() }}", {"env": {"finalize": "brackets"}}),
    ("finalize_include", "{% include 'inc' %}", {"env": {"finalize": "brackets"}, "templates": {"inc": "{{ x }}"}}),
]
