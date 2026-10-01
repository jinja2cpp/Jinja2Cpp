"""Undefined values: default, strict and chainable undefined behaviour."""
CONTEXT = {"d": {"a": 1}, "n": None}
STRICT = {"env": {"undefined": "strict"}}
CHAIN = {"env": {"undefined": "chainable"}}
CASES = [
    ("print", "[{{ nope }}]"),
    ("attr_of_undefined", "[{{ nope.attr }}]"),
    ("item_of_undefined", "[{{ nope['x'] }}]"),
    ("attr_of_none", "[{{ n.attr }}]"),
    ("call_undefined", "[{{ nope() }}]"),
    ("method_of_undefined", "[{{ nope.upper() }}]"),
    ("iterate_undefined", "[{% for i in nope %}x{% endfor %}]"),
    ("length_undefined", "{{ nope|length }}"),
    ("undefined_in_concat", "[{{ 'a' ~ nope }}]"),
    ("undefined_plus", "[{{ nope + 1 }}]"),
    ("undefined_compare", "{{ nope == none }}"),
    ("undefined_truthiness", "{% if nope %}t{% else %}f{% endif %}"),
    ("undefined_in_test", "{{ nope is defined }}"),
    ("undefined_attr_in_test", "{{ nope.attr is defined }}"),
    ("undefined_string_filter", "[{{ nope|string }}]"),
    ("undefined_upper_filter", "[{{ nope|upper }}]"),
    ("undefined_list_filter", "{{ nope|list|length }}"),
    ("undefined_default_chain", "{{ nope.attr|default('dflt') }}"),
    ("undefined_join", "[{{ nope|join(',') }}]"),
    ("strict_print", "{{ nope }}", STRICT),
    ("strict_is_defined", "{{ nope is defined }}", STRICT),
    ("strict_default", "{{ nope|default('d') }}", STRICT),
    ("strict_if", "{% if nope %}x{% endif %}", STRICT),
    ("strict_defined_attr", "{{ d.a }}", STRICT),
    ("strict_missing_attr", "{{ d.zz }}", STRICT),
    ("chainable_deep", "[{{ nope.a.b.c }}]", CHAIN),
]
