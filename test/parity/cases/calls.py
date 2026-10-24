"""The call protocol: *args and **kwargs in calls (docs/tasks/0220) and the order Jinja2's
parser allows for the arguments of a call (docs/tasks/0294)."""
M = "{% macro m(a, b) %}{{ a }}-{{ b }}{% endmacro %}"
CASES = [
    # *args: any iterable, after the positional arguments
    ("star_args_after_positional", "{{ range(1, *[4])|list }}"),
    ("star_args_string", M + "{{ m(*'xy') }}"),
    ("star_args_tuple", M + "{{ m(*(1, 2)) }}"),
    ("star_args_dict_keys", M + "{{ m(*{'p': 1, 'q': 2}) }}"),
    ("star_args_range", M + "{{ m(*range(2)) }}"),
    ("star_args_empty", M + "{{ m(1, 2, *[]) }}"),
    ("star_args_trailing_comma", M + "{{ m(*[1, 2],) }}"),
    ("star_args_undefined", M + "{{ m(1, 2, *nothing) }}"),
    ("star_args_undefined_strict", M + "{{ m(1, 2, *nothing) }}", {"env": {"undefined": "strict"}}),
    ("star_args_none", M + "{{ m(*none) }}"),
    ("star_args_int", M + "{{ m(*1) }}"),
    ("star_args_namespace", M + "{{ m(*namespace(a=1)) }}"),
    # Python takes *args as positional arguments wherever it is written
    ("star_args_after_keyword", M + "{{ m(b=2, *[1]) }}"),
    ("star_args_and_keyword_same_name", M + "{{ m(a=2, *[1]) }}"),
    # **kwargs: a mapping with string keys, each name given once
    ("star_kwargs_with_positional", M + "{{ m(1, **{'b': 2}) }}"),
    ("star_kwargs_with_keyword", M + "{{ m(a=1, **{'b': 2}) }}"),
    ("star_kwargs_duplicate", M + "{{ m(a=1, **{'a': 2, 'b': 3}) }}"),
    ("star_kwargs_list", M + "{{ m(**[1, 2]) }}"),
    ("star_kwargs_int", M + "{{ m(**1) }}"),
    ("star_kwargs_none", M + "{{ m(**none) }}"),
    ("star_kwargs_undefined", M + "{{ m(1, 2, **nothing) }}"),
    ("star_kwargs_namespace", M + "{{ m(**namespace(a=1, b=2)) }}"),
    ("star_kwargs_non_string_key", "{{ dict(**{1: 2}) }}"),
    ("star_kwargs_into_dict", "{{ dict(b=2, **{'a': 1}) }}"),
    ("star_kwargs_from_context", M + "{{ m(**d) }}", {"ctx": {"d": {"a": 1, "b": 2}}}),
    ("star_both", M + "{{ m(*[1], **{'b': 2}) }}"),
    ("star_both_after_keyword", "{% macro m(a, b, c) %}{{ a }}{{ b }}{{ c }}{% endmacro %}{{ m(c=3, *[1], **{'b': 2}) }}"),
    # Forwarding the arguments a macro collects
    ("forward_varargs_kwargs", "{% macro inner(a, b, c) %}{{ a }}{{ b }}{{ c }}{% endmacro %}{% macro outer() %}{{ inner(*varargs, **kwargs) }}{% endmacro %}{{ outer(1, 2, c=3) }}"),
    ("caller_star_varargs", "{% macro m() %}{{ caller(*varargs) }}{% endmacro %}{% call(a, b) m(1, 2) %}{{ a }}{{ b }}{% endcall %}"),
    ("call_block_star_args", "{% macro m(a) %}{{ a }}{{ caller() }}{% endmacro %}{% call m(*[1]) %}x{% endcall %}"),
    ("call_block_kwargs_caller", "{% macro m() %}{{ caller() }}{% endmacro %}{% call m(**{'caller': 1}) %}x{% endcall %}"),
    # Methods and loop.cycle take the general call
    ("method_star_args", "{{ 'a-b-c'.split(*['-', 1]) }}"),
    ("method_star_kwargs", "{{ 'a-b-c'.split('-', **{'maxsplit': 1}) }}"),
    ("loop_cycle_star_args", "{% for i in [1, 2, 3] %}{{ loop.cycle(*['a', 'b']) }}{% endfor %}"),
    ("loop_cycle_star_args_empty", "{% for i in [1] %}{{ loop.cycle(*[]) }}{% endfor %}"),
    # Filters and tests take them too (docs/tasks/0304)
    ("filter_star_args", "{{ 'a'|replace(*['a', 'b']) }}"),
    ("filter_star_kwargs", "{{ 'a'|replace('a', **{'new': 'b'}) }}"),
    ("test_star_args", "{{ 4 is divisibleby(*[2]) }}"),
    ("filter_block_star_args", "{% filter replace(*['a', 'b']) %}a{% endfilter %}"),
    # The order of the arguments: Jinja2's "invalid syntax for function call expression"
    ("positional_after_keyword", "{{ range(stop=2, 1) }}"),
    ("positional_after_keyword_test", "{{ 1 is divisibleby(num=1, 2) }}"),
    ("positional_after_star_args", M + "{{ m(*[1], 2) }}"),
    ("positional_after_star_kwargs", M + "{{ m(**{'a': 1}, 2) }}"),
    ("keyword_after_star_kwargs", M + "{{ m(**{'a': 1}, b=2) }}"),
    ("star_args_after_star_kwargs", M + "{{ m(**{'a': 1}, *[2]) }}"),
    ("star_args_twice", M + "{{ m(*[1], *[2]) }}"),
    ("star_kwargs_twice", M + "{{ m(**{'a': 1}, **{'b': 2}) }}"),
    ("star_without_value", M + "{{ m(*) }}"),
]
