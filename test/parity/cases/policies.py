"""Environment.policies (task 0435): the defaults urlize, truncate, tojson and the i18n trans block read.

Keys are Jinja2's own (``env.policies.update``); parity_test.cpp maps them onto Settings::filterPolicies and
Settings::Extensions::i18nTrimmed. A json.dumps_kwargs dict replaces Jinja2's default ``{"sort_keys": True}``, so
``{}`` keeps insertion order.
"""
CONTEXT = {"d": {"b": 1, "a": "<é>"}, "s": "x" * 20}
URL = "{{ 'see http://example.com/c now'|urlize }}"
SCHEMES = "{{ 'ssh:a@b tel:123 http://example.com'|urlize }}"
DICT = "{{ {'b': 1, 'a': ['<é>', 2]}|tojson }}"
TRUNCATE = "{{ s|truncate(10) }}|{{ 'abcdefghijk'|truncate(10) }}|{{ 'abcdefghijklmnop'|truncate(10) }}"
TRANS = "{% trans %}  a\n   b  \n c {% endtrans %}|{% trans notrimmed %}  a\n   b  \n c {% endtrans %}|{% trans trimmed %}  a\n   b  \n c {% endtrans %}"


def pol(**policies):
    return {"env": {"policies": {k.replace("__", "."): v for k, v in policies.items()}}}


CASES = [
    # urlize.rel
    ("urlize_default", URL),
    ("urlize_rel_none", URL, pol(urlize__rel=None)),
    ("urlize_rel_words", URL, pol(urlize__rel="x y")),
    ("urlize_rel_words_nofollow", "{{ 'http://example.com'|urlize(nofollow=true) }}", pol(urlize__rel="x y")),
    ("urlize_rel_none_nofollow", "{{ 'http://example.com'|urlize(nofollow=true) }}", pol(urlize__rel=None)),
    ("urlize_rel_argument", "{{ 'http://example.com'|urlize(rel='z x') }}", pol(urlize__rel="x y")),
    ("urlize_rel_argument_none_policy", "{{ 'http://example.com'|urlize(rel='z') }}", pol(urlize__rel=None)),
    ("urlize_rel_empty_policy", URL, pol(urlize__rel="")),
    # urlize.target
    ("urlize_target_policy", URL, pol(urlize__target="_blank")),
    ("urlize_target_argument", "{{ 'http://example.com'|urlize(target='_top') }}", pol(urlize__target="_blank")),
    ("urlize_target_argument_none", "{{ 'http://example.com'|urlize(target=none) }}", pol(urlize__target="_blank")),
    ("urlize_target_escaped", URL, pol(urlize__target='a"b')),
    # urlize.extra_schemes
    ("urlize_schemes_default", SCHEMES),
    ("urlize_schemes_policy", SCHEMES, pol(urlize__extra_schemes=["ssh:", "tel:"])),
    ("urlize_schemes_argument", "{{ 'ssh:a@b tel:123'|urlize(extra_schemes=['tel:']) }}", pol(urlize__extra_schemes=["ssh:"])),
    ("urlize_schemes_argument_none", "{{ 'ssh:a@b tel:123'|urlize(extra_schemes=none) }}", pol(urlize__extra_schemes=["ssh:"])),
    ("urlize_schemes_argument_empty_list", "{{ 'ssh:a@b tel:123'|urlize(extra_schemes=[]) }}", pol(urlize__extra_schemes=["ssh:"])),
    ("urlize_schemes_policy_string", SCHEMES, pol(urlize__extra_schemes="ab")),
    ("urlize_schemes_policy_invalid", SCHEMES, pol(urlize__extra_schemes=["1x"])),
    # truncate.leeway
    ("truncate_default", TRUNCATE),
    ("truncate_leeway_0", TRUNCATE, pol(truncate__leeway=0)),
    ("truncate_leeway_1", TRUNCATE, pol(truncate__leeway=1)),
    ("truncate_leeway_large", TRUNCATE, pol(truncate__leeway=100)),
    ("truncate_leeway_negative", TRUNCATE, pol(truncate__leeway=-1)),
    ("truncate_leeway_argument_none", "{{ 'abcdefghijklmnop'|truncate(10, leeway=none) }}", pol(truncate__leeway=0)),
    ("truncate_leeway_argument_wins", "{{ 'abcdefghijklmnop'|truncate(10, leeway=9) }}", pol(truncate__leeway=0)),
    # json.dumps_kwargs
    ("tojson_default", DICT),
    ("tojson_kwargs_empty", DICT, pol(json__dumps_kwargs={})),
    ("tojson_kwargs_sort_keys", DICT, pol(json__dumps_kwargs={"sort_keys": True})),
    ("tojson_kwargs_indent_2", DICT, pol(json__dumps_kwargs={"sort_keys": True, "indent": 2})),
    ("tojson_kwargs_indent_text", DICT, pol(json__dumps_kwargs={"indent": "\t"})),
    ("tojson_kwargs_indent_filter_wins", "{{ {'b': 1, 'a': [1, 2]}|tojson(4) }}", pol(json__dumps_kwargs={"indent": 2})),
    ("tojson_kwargs_indent_filter_none", "{{ {'b': 1, 'a': [1, 2]}|tojson(none) }}", pol(json__dumps_kwargs={"indent": 2})),
    ("tojson_kwargs_ensure_ascii_false", DICT, pol(json__dumps_kwargs={"ensure_ascii": False, "sort_keys": True})),
    ("tojson_kwargs_separators", DICT, pol(json__dumps_kwargs={"separators": [";", "="]})),
    ("tojson_kwargs_separators_indent", DICT, pol(json__dumps_kwargs={"separators": [",", ":"], "indent": 1})),
    ("tojson_kwargs_string_value", "{{ s|tojson }}{{ 'é<'|tojson }}", pol(json__dumps_kwargs={"ensure_ascii": False})),
    # ext.i18n.trimmed
    ("trans_default", TRANS, {"env": {"extensions": ["i18n"]}}),
    ("trans_trimmed_policy", TRANS, {"env": {"extensions": ["i18n"], "policies": {"ext.i18n.trimmed": True}}}),
    ("trans_trimmed_policy_plural",
     "{% trans n=2 %}  {{ n }}\n item{% pluralize %}\n {{ n }} items {% endtrans %}",
     {"env": {"extensions": ["i18n"], "policies": {"ext.i18n.trimmed": True}}}),
]
