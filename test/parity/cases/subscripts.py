"""Attribute access, subscripts and slicing."""
CONTEXT = {"l": [3, 1, 2, 5], "s": "Hello World", "d": {"a": 1, "b": {"c": 2}, "key with space": 3, "items": "own"},
           "users": [{"name": "bob"}, {"name": "alice"}]}
CASES = [
    ("index", "{{ l[0] }}"),
    ("index_negative", "{{ l[-1] }}"),
    ("index_out_of_range", "[{{ l[10] }}]"),
    ("index_string", "{{ s[1] }}"),
    ("index_string_negative", "{{ s[-1] }}"),
    ("slice_from", "{{ l[1:] }}"),
    ("slice_to", "{{ l[:2] }}"),
    ("slice_step", "{{ l[::2] }}"),
    ("slice_reverse", "{{ l[::-1] }}"),
    ("slice_negative", "{{ l[-2:] }}"),
    ("slice_string", "{{ s[0:5] }}"),
    ("slice_string_reverse", "{{ s[::-1] }}"),
    ("dot_attr", "{{ d.a }}"),
    ("item_attr", "{{ d['a'] }}"),
    ("nested_dot", "{{ d.b.c }}"),
    ("nested_item", "{{ d['b']['c'] }}"),
    ("item_with_space", "{{ d['key with space'] }}"),
    ("item_variable_key", "{% set k = 'a' %}{{ d[k] }}"),
    ("dot_index", "{{ l.0 }}"),
    ("list_of_dicts", "{{ users[1].name }}"),
    ("missing_attr", "[{{ d.missing }}]"),
    ("missing_nested_attr", "[{{ d.missing.deeper }}]"),
    ("dict_key_shadows_method", "{{ d['items'] }}"),
    ("dot_shadows_method", "{{ d.items is callable }}"),
    ("literal_subscript", "{{ [1, 2, 3][1] }}"),
    ("literal_dict_subscript", "{{ {'a': 'x'}['a'] }}"),
    ("string_literal_index", "{{ 'abc'[0] }}"),
    ("subscript_int", "[{{ x[0] }}]", {"ctx": {"x": 3}}),
    ("subscript_on_call", "{{ range(5)[2] }}"),
    ("slice_step_only", "{{ l[::2] }}"),
    ("slice_both_ends", "{{ l[1:-1] }}|{{ l[:] }}"),
    ("slice_tuple", "{{ (1, 2, 3)[1:] }}"),
    ("slice_string_clamped", "{{ 'abc'[5:1:-1] }}"),
    ("slice_then_index", "{{ l[0:2][1] }}"),
    ("slice_step_zero", "{{ l[::0] }}"),
    ("slice_float_bound", "{{ l[1.5:] }}"),
    ("dot_index_chain", "{{ d.b.c }}{{ users.1.name }}"),
    ("slice_extreme_step", "{{ l[1::9223372036854775807] }}|{{ l[::-9223372036854775807-1] }}"),
    ("tuple_index", "[{{ l[1, 2] }}][{{ l[] }}]"),
    ('dict_unhashable_subscript', '[{{ {1:2}[[1]] }}]'),
    ('dict_subscript_float_and_bool', "{{ {1: 'a'}[1.0] }}|{{ {1: 'a'}[true] }}|{{ {1.5: 'f'}[1.5] }}"),
    ('dict_subscript_missing_non_string', "[{{ {1: 'a'}[2] }}][{{ {1: 'a'}[none] }}]"),
    # task 0270: an index beyond int64 finds nothing; slice bounds clip by their sign
    ('big_index_and_slice', "[{{ l[18446744073709551615] }}]|{{ l[18446744073709551615:] }}|{{ l[:18446744073709551615] }}|{{ l[-18446744073709551615:] }}|{{ {18446744073709551615: 'k'}[18446744073709551615] }}"),
    # task 0362: a slice goes to __getitem__ without getitem's fallback, so what has no items
    # raises; a slice of literals folds through getitem, which makes it undefined
    ("slice_int_variable", "{{ x[1:2] }}", {"ctx": {"x": 5}}),
    ("slice_none_variable", "{{ x[1:2] }}", {"ctx": {"x": None}}),
    ("slice_set_variable", "{% set x = 5 %}{{ x[1:2] }}"),
    ("slice_dict_key_error", "{{ d[1:2] }}"),
    ("slice_undefined", "{{ missing[1:2] }}"),
    ("slice_namespace", "{{ namespace(a=1)[0:1] }}"),
    ("slice_loop", "{% for i in [1] %}{{ loop[0:1] }}{% endfor %}"),
    ("slice_function", "{{ lipsum[0:1] }}"),
    ("slice_literals_fold", "[{{ 5[1:2] }}][{{ none[1:2] }}][{{ {'a': 1}[1:2] }}][{{ 1.5[0:1] is undefined }}]"),
    # task 0383: a lazy filter result is a generator in Python, which takes no slice
    ("slice_lazy_result", "{{ ([1, 2]|map('string'))[0:1] }}"),
    ("slice_reversed_list", "{{ ([1, 2]|select|reverse|reverse)[1:] }}"),
    # a slice of a folded literal reverse is getitem's undefined (Jinja2 folds |reverse)
    ("slice_reversed_literal_fold", "[{{ ([1, 2]|reverse)[0:1] }}][{{ ([1, 2]|reverse)[0] }}]"),
    # an undefined slice bound has no __index__; a literal slice folds to getitem's undefined,
    # a slice of which fails
    ("slice_undefined_bound", "{{ l[missing:] }}"),
    ("slice_of_folded_slice", "{{ 5[1:][1:] }}"),
    ("slice_folded_strict", "[{{ 5[1:] }}]", {"env": {"undefined": "strict"}}),
    ("slice_folded_debug", "[{{ 5[1:] }}]", {"env": {"undefined": "debug"}}),
    # a subscript by a tuple of slices (0445): Jinja2 folds it on a literal, to an undefined on a
    # list; of a name it compiles to invalid Python (SyntaxError at load)
    ("tuple_of_slices_literal", "[{{ [1, 2][1:2, 3] }}|{% if false %}{{ ()[:, :] }}{% endif %}]"),
    ("tuple_of_slices_name", "{{ l[1:2, 3] }}", {"ctx": {"l": [1, 2]}}),
    # only an output's whole value folds; any other use is invalid Python
    ("tuple_of_slices_folded_uses", "[{{ ([1][:, :]) ~ 'a' }}|{{ [1][:, :] is defined }}|{{ [1][:, :]|default('d') }}|{{ {'a': 1}[:, :] }}|{{ 'ab'[1:, 1] }}|{{ [1][1, :] }}|{{ [1][::2, 1:] }}]"),
    ("tuple_of_slices_in_set", "{% set x = [1, 2][1:2, 3] %}[{{ x }}]"),
    ("tuple_of_slices_in_if", "{% if [1][:, :] %}y{% endif %}"),
    ("tuple_of_slices_unfolded_part", "{{ [1][n:, 1] }}", {"ctx": {"n": 0}}),
    ("tuple_of_slices_strict", "[{{ [1][:, :] }}]", {"env": {"undefined": "strict"}}),
    ("tuple_of_slices_in_call", "{{ [1][:, :]|string }}"),
    ("tuple_of_slices_attribute", "{{ [1][:, :].x }}"),
    ("tuple_of_slices_item", "{{ [1][:, :][0] }}"),
    ("subscript_trailing_comma", "{{ [1][1,] }}"),
    # leftovers of 0445 (0471): Jinja2 folds by the branch a folded test picks also in a test, a
    # set and a print; strict undefined refuses more folds; an item of a folded undefined does not fold
    ("tuple_of_slices_in_test", "[{{ 1 if [1][:, :] is undefined }}]"),
    ("tuple_of_slices_set_by_choice", "{% set q = 1 if true else ()[:, :] %}[{{ q }}]"),
    ("tuple_of_slices_print", "[{% print ()[:, :] %}]"),
    ("tuple_of_slices_strict_concat", "[{{ ()[:, :] ~ 'a' }}]", {"env": {"undefined": "strict"}}),
    ("tuple_of_slices_of_folded_undefined", "[{{ ()[0][:, :] }}]"),
    ("tuple_of_slices_failing_part", "[{{ [1 / 0][:, :] }}|{{ ()[:, 1 / 0] }}]"),
    ("subscript_slice_trailing_comma", "{{ [1][1:2,] }}"),
    ("tuple_of_slices_folded_branch", "[{{ ()[:, :] if true else x }}|{{ ()[:, :] if false else 5 }}|{% autoescape x %}{{ ()[:, :] }}{% endautoescape %}]"),
    # Jinja2 folds a conditional whose test folds by the branch it picks; the engine decides that
    # only when it renders, so at load it rejects an unfolded tuple of slices there (0471)
    ("tuple_of_slices_fold_by_choice", "[{{ (()[:, :] if 1 > 0) is undefined and '' }}|{{ (()[:, :] if 1 > 0 else x) is undefined and '' }}]"),
    # 0471: the optimizer folds any expression around the subscript whose value has a safe repr (a
    # statement's, a test, a loop's filter, a part of an output); an undefined, or a fold that
    # fails, leaves it unfolded; with StrictUndefined a fold that uses the undefined fails the compile
    ("tuple_of_slices_other_branch", "{{ (()[:, :] if 1 < 0 else x) is undefined and '' }}"),
    ("tuple_of_slices_test_folds_away", "[{{ 1 if [1][:, :] is defined }}|{{ (1 if true else ()[:, :]) ~ x }}]"),
    ("tuple_of_slices_statements", "{% set q = [[1][:, :] is undefined] %}[{{ q }}|{% if [1][:, :] is undefined %}y{% endif %}|{% for i in [1] if [1][:, :] is undefined %}y{% endfor %}]"),
    ("tuple_of_slices_set_undefined", "{% set q = ()[:, :] if true else 1 %}[{{ q }}]"),
    ("tuple_of_slices_print_parts", "[{% print ()[:, :] ~ 'a', 1 if [1][:, :] is undefined %}]"),
    ("tuple_of_slices_unpicked_failing_part", "[{{ [1 / 0][:, :] if false else 5 }}|{{ none[:, :] }}|{{ 1[:, :] }}|{{ 'ab'[:, :] }}]"),
    ("tuple_of_slices_strict_tests", "[{{ ()[:, :] is undefined }}|{{ ()[:, :]|default('d') }}]", {"env": {"undefined": "strict"}}),
    ("tuple_of_slices_strict_compare", "[{{ ()[:, :] == 1 }}]", {"env": {"undefined": "strict"}}),
    ("tuple_of_slices_strict_not", "[{{ not ()[:, :] }}]", {"env": {"undefined": "strict"}}),
    ("tuple_of_slices_strict_or", "[{{ ()[:, :] or 1 }}]", {"env": {"undefined": "strict"}}),
    ("tuple_of_slices_strict_length", "[{{ ()[:, :]|length }}]", {"env": {"undefined": "strict"}}),
    ("tuple_of_slices_strict_branch", "[{{ ()[:, :] if true else 1 }}]", {"env": {"undefined": "strict"}}),
    ("tuple_of_slices_strict_test", "[{{ 1 if ()[:, :] else 2 }}]", {"env": {"undefined": "strict"}}),
    # an item or attribute of the undefined fails its own fold, but a fold around it that does not
    # evaluate it (`true or ...`) leaves valid code; the engine rejects any item of it (0484)
    ("tuple_of_slices_item_not_evaluated", "[{{ true or [1][:, :][0] }}|{{ (true or [1][:, :].x) and '' }}]"),
    # a filter chain or test folds with its operand (the verifier of 0471)
    ("tuple_of_slices_chain_operand", "[{{ [1][:, :]|length ~ x }}|{{ [1][:, :] is undefined ~ x }}|{{ x or [1][:, :] is undefined }}|{{ not [1][:, :]|e in 'ab' }}]"),
    ("tuple_of_slices_chain_statements", "{% if [1][:, :]|sort|first is undefined %}a{% endif %}{% if x and [1][:, :] is undefined %}b{% endif %}{% for i in [1, 2] if [1][:, :] is undefined and i %}{{ i }}{% endfor %}"),
    # filters that fail on an undefined fail the fold, which leaves invalid Python
    ("tuple_of_slices_int", "[{{ [1][:, :]|int }}]"),
    ("tuple_of_slices_tojson", "[{{ [1][:, :]|tojson is undefined }}]"),
    ("tuple_of_slices_debug", "[{{ [1][:, :] }}|{{ ()[1:2, 'a'] }}]", {"env": {"undefined": "debug"}}),
    ("tuple_of_slices_chainable_item", "[{{ [1][:, :].a }}]", {"env": {"undefined": "chainable"}}),
    ("tuple_of_slices_strict_short_circuit", "{% set q = ()[:, :] or true if 1 and none else 1 %}[{{ q }}]", {"env": {"undefined": "strict"}}),
    ("tuple_of_slices_strict_list_compare", "[{{ [1, 2] == ([()[:, :], ()[:, :]]) }}]", {"env": {"undefined": "strict"}}),
    ("tuple_of_slices_strict_set", "{% set q = ()[:, :] ~ 'a' %}[{{ q }}]", {"env": {"undefined": "strict"}}),
    # 0484: an operator node folds too (a `~`, a comparison, `not`, `and`/`or`), and an item or
    # attribute of the undefined is a plain subscript whose own fold fails
    ("tuple_of_slices_operator_folds", "[{{ x or 'ab'[1:, 1] ~ ()[:, :] }}|{{ x == [1][:, :] ~ ()[:, :] }}|{{ x ~ ([1][:, :] == 1) }}|{{ x ~ (not [1][:, :]) }}|{{ x ~ ([1][:, :] or 2) }}|{{ x ~ ([1][:, :] is undefined or y) }}]"),
    ("tuple_of_slices_operator_fails", "[{{ x ~ ([1][:, :] + 1) }}]"),
    ("tuple_of_slices_operator_unary_fails", "[{{ x ~ -[1][:, :] }}]"),
    ("tuple_of_slices_item_call", "[{{ [1][:, :].x() }}]"),
    ("tuple_of_slices_item_call_not_evaluated", "[{{ true or [1][:, :].x() }}]"),
    ("tuple_of_slices_item_test", "[{{ [1][:, :][0] is undefined }}]"),
    ("tuple_of_slices_item_chainable", "[{{ [1][:, :][0].b is undefined }}|{{ x ~ [1][:, :].a }}]", {"env": {"undefined": "chainable"}}),
    ("tuple_of_slices_item_strict", "[{{ true or [1][:, :][0] }}]", {"env": {"undefined": "strict"}}),
    ("tuple_of_slices_strict_item_fails", "[{{ [1][:, :][0] }}]", {"env": {"undefined": "strict"}}),
    ("tuple_of_slices_strict_operator_impossible", "[{{ true if 1 else ()[:, :] + 1 }}|{{ true if 1 else ()[:, :] == 1 }}|{{ true if 1 else not ()[:, :] }}]", {"env": {"undefined": "strict"}}),
    ("tuple_of_slices_strict_concat_inside", "{% set q = 1 if true else (()[:, :] ~ 'a') %}[{{ q }}]", {"env": {"undefined": "strict"}}),
    ("tuple_of_slices_strict_and", "[{{ 1 if true else (true and ()[:, :]) }}]", {"env": {"undefined": "strict"}}),
    # an output folds whole first (visit_Output), top-down; the optimizer folds a statement's
    # expression, or an output's that did not fold, bottom-up, where `and`, `or` and `~` let a
    # strict undefined's error out and a filter, a test or an operator gives Impossible
    ("tuple_of_slices_strict_output_first", "[{{ ()[:, :] ~ 1 if false else 2 }}|{{ 1 if true else (()[:, :] or 1) }}|{{ 1 or ()[:, :]|trim }}|{{ true if 1 else ()[:, :]|length }}|{{ true if 1 else ()[:, :][0] }}|{% print 1 if true else (()[:, :] and 1), 2 %}]", {"env": {"undefined": "strict"}}),
    ("tuple_of_slices_strict_statement_bottom_up", "{% set q = ()[:, :] ~ 1 if false else 2 %}[{{ q }}]", {"env": {"undefined": "strict"}}),
    ("tuple_of_slices_strict_output_bottom_up", "[{{ x ~ (()[:, :] ~ 1) }}]", {"env": {"undefined": "strict"}}),
    ("tuple_of_slices_strict_statement_filter", "{% set q = 1 if true else ()[:, :]|length %}[{{ q }}]", {"env": {"undefined": "strict"}}),
    ("tuple_of_slices_chainable_nested", "[{{ [1][:, :][:, :] is undefined }}|{{ [1][:, :].a[:, :] }}|{{ [1][:, :][:, :] }}]", {"env": {"undefined": "chainable"}}),
    # the parts of a tuple of slices of a ChainableUndefined are evaluated (Codex on #880)
    ("tuple_of_slices_chainable_failing_part", "[{{ [1][:, :][:, 1 / 0] }}]", {"env": {"undefined": "chainable"}}),
    ("tuple_of_slices_nested_default", "[{{ [1][:, :][:, :] is undefined }}]"),
    # a step of a filter chain folds on its own, where the whole chain does not (0497)
    ("tuple_of_slices_chain_step", "[{% if [1][:, :]|sort|first %}y{% endif %}]"),
    ("tuple_of_slices_chain_step_strict", "[{% if [1][:, :]|default([])|map('string') %}y{% endif %}]", {"env": {"undefined": "strict"}}),
    ("tuple_of_slices_chain_step_test", "[{% if [1][:, :] is undefined|string|first %}y{% endif %}|{% if [1][:, :]|sort|first is defined %}y{% endif %}|{% set v = [1][:, :]|sort|first %}{{ v }}|{{ x|default([1][:, :]|sort)|first }}]"),
    ("tuple_of_slices_chain_step_later", "[{% if [1][:, :]|length|string|first %}y{% endif %}|{% for i in [1][:, :]|sort|map('string') %}{{ i }}{% endfor %}|{{ [1][:, :]|sort|first }}]"),
    ("tuple_of_slices_chain_no_step_folds", "{% if x|default([1][:, :])|sort|first %}y{% endif %}"),
    ("tuple_of_slices_chain_step_strict_fails", "{% if [1][:, :]|sort|first %}y{% endif %}", {"env": {"undefined": "strict"}}),
    # with StrictUndefined the Load's failure is decided by the candidate's node kind, not by
    # the filter or test in a branch of a conditional (0527)
    ("tuple_of_slices_strict_conditional_branch", "{% if false and ([1][:, :]|length if 1 else x) %}a{% else %}b{% endif %}", {"env": {"undefined": "strict"}}),
    ("tuple_of_slices_strict_conditional_test", "{% if false and (1 if [1][:, :] else x) %}a{% else %}b{% endif %}", {"env": {"undefined": "strict"}}),
    # 0527: the conditional's fold follows CondExpr.as_const: the test, whose truth fails for a strict
    # undefined, then the branch it picks, whose filter or test gives Impossible; an else chain too
    ("tuple_of_slices_strict_conditional_test_filter", "{% if false and (1 if [1][:, :]|length else x) %}a{% else %}b{% endif %}|{% if false and (1 if [1][:, :] is defined else 2) %}a{% else %}b{% endif %}|{% if false and (1 if [1][:, :]|default(1) else x) %}a{% else %}b{% endif %}", {"env": {"undefined": "strict"}}),
    ("tuple_of_slices_strict_conditional_else_branch", "{% if false and (1 if 0 else [1][:, :]|length) %}a{% else %}b{% endif %}|{% if false and (1 if 0 else 2 if 0 else [1][:, :]|length) %}a{% else %}b{% endif %}|{% if false and ([1][:, :] if 0) %}a{% else %}b{% endif %}", {"env": {"undefined": "strict"}}),
    ("tuple_of_slices_strict_conditional_chained_test", "{% if false and (1 if 0 else 2 if [1][:, :] else 3) %}a{% else %}b{% endif %}", {"env": {"undefined": "strict"}}),
    ("tuple_of_slices_strict_conditional_or_test", "{% if false and (1 if ([1][:, :] or 1) else 2) %}a{% else %}b{% endif %}", {"env": {"undefined": "strict"}}),
    # a failing `and`/`or` test is Impossible when its failure is a filter's (verifier on 0527)
    ("tuple_of_slices_strict_conditional_logical_test", "{% if false and ([1][:, :] if 1 and 1|list) %}a{% endif %}b|{% if false and (x or ([1][:, :] if 1|list and 1)) %}a{% endif %}b|{% if false and (([1][:, :] if 1 and 1|list) if 0 else [1][:, :]) %}a{% endif %}b", {"env": {"undefined": "strict"}}),
    ("tuple_of_slices_strict_conditional_set", "{% set q = 1 if ()[:, :] else 2 %}[{{ q }}]", {"env": {"undefined": "strict"}}),
    ("tuple_of_slices_strict_conditional_output_concat", "[{{ x ~ (1 if ()[:, :] else 2) }}]", {"env": {"undefined": "strict"}}),
    # a strict filter chain: each step is a candidate, evaluated from the step before it (0527)
    ("tuple_of_slices_strict_filter_chain", "{% set q = [1][:, :]|default([3, 1])|sort|first|string %}[{{ q }}]", {"env": {"undefined": "strict"}}),
    # 0536: Jinja2's batch, slice, unique and items return generators, which fold on a strict undefined
    ("tuple_of_slices_strict_generator_filters", "{{ ([1][:, :]|batch(2)) is defined }}|{{ ([1][:, :]|slice(2)) is defined }}|{{ ([1][:, :]|unique) is defined }}|{{ ([1][:, :]|default([])|items) is defined }}", {"env": {"undefined": "strict"}}),
    # 0536: a filter's value error is Impossible in the fold too; a conditional whose branches do not both
    # fold still has its test's truth checked
    ("tuple_of_slices_strict_conditional_value_error", "{% if false and (1 if [1][:, :]|abs else 2) %}a{% endif %}b", {"env": {"undefined": "strict"}}),
    ("tuple_of_slices_strict_conditional_unfolded_branch", "{% if false and (1 if 1 else x if [1][:, :] else x) %}a{% endif %}b", {"env": {"undefined": "strict"}}),
    ("tuple_of_slices_strict_conditional_unfolded_alt", "{% if false and (0|list if 1 else [1][:, :] ~ x) %}a{% endif %}b", {"env": {"undefined": "strict"}}),
    ("tuple_of_slices_strict_and_unfolded", "{% if false and ([1][:, :] and x) %}a{% endif %}b", {"env": {"undefined": "strict"}}),
    ("tuple_of_slices_strict_or_unfolded", "{% if false and ([1][:, :] or x) %}a{% endif %}b", {"env": {"undefined": "strict"}}),
    ("tuple_of_slices_strict_concat_unfolded", "{% if false and ([1][:, :] ~ x) %}a{% endif %}b", {"env": {"undefined": "strict"}}),
    ("tuple_of_slices_strict_unfolded_operands_pass", "{% if false and (x ~ [1][:, :]) %}a{% endif %}b|{% if false and (x if 1 else [1][:, :]) %}a{% endif %}b|{% if false and (x if [1][:, :] is defined else 2) %}a{% endif %}b", {"env": {"undefined": "strict"}}),
    ("tuple_of_slices_strict_conditional_unfolded_test", "{% if false and (x if [1][:, :]) %}a{% endif %}b", {"env": {"undefined": "strict"}}),
    ("tuple_of_slices_strict_operator_chain", "{% if [1][:, :] is undefined or a or b or c %}y{% endif %}|{% if not not not ([1][:, :] is defined) %}n{% endif %}|{% set q = ([1][:, :] is undefined) + 1 + 2 + 3 %}{{ q }}", {"env": {"undefined": "strict"}}),
    # an operand's own failure is Impossible to an `and`, `or` or `~`; only bool() or str() of a strict undefined
    # value, its own or a nested `and`, `or`, `~` or conditional's, fails the compile (0546)
    ("tuple_of_slices_strict_impossible_operands", "{% if false and (none or (not (1, 2)[:, :])) %}a{% endif %}b|{% if false and (-([1, 2]) or ([1][:, :] + 1)) %}a{% endif %}b|{% if false and ((false) ~ ([1][:, :]|float)) %}a{% endif %}b|{% if false and ((-([]|max) ~ 1) or [1][:, :]) %}a{% endif %}b|{% if false and (1 and ([1][:, :]|abs)) %}a{% endif %}b", {"env": {"undefined": "strict"}}),
    ("tuple_of_slices_strict_nested_concat_undefined", "{% if false and (([]|max ~ 1) or [1][:, :]) %}a{% endif %}b", {"env": {"undefined": "strict"}}),
    ("tuple_of_slices_strict_nested_conditional_undefined", "{% if false and ((1 if []|max else 2) or [1][:, :]) %}a{% endif %}b", {"env": {"undefined": "strict"}}),
    ("tuple_of_slices_strict_nested_or_value_undefined", "{% if false and ((0 or []|max) or [1][:, :]) %}a{% endif %}b", {"env": {"undefined": "strict"}}),
    ("tuple_of_slices_strict_nested_and_value_concat", "{% if false and ((1 and []|max) ~ [1][:, :]) %}a{% endif %}b", {"env": {"undefined": "strict"}}),
    ("tuple_of_slices_strict_nested_or_left_undefined", "{% if false and (([1][:, :] or 1) or 2) %}a{% endif %}b", {"env": {"undefined": "strict"}}),
    ("tuple_of_slices_strict_nested_and_right_undefined", "{% if false and (0 or ([1][:, :] and 1)) %}a{% endif %}b", {"env": {"undefined": "strict"}}),
    ("tuple_of_slices_strict_nested_concat_leading", "{% if false and (1 ~ ([1][:, :] ~ 1)) %}a{% endif %}b", {"env": {"undefined": "strict"}}),
    # subscripts and tests of a step: their operand is the step before (0546)
    ("tuple_of_slices_strict_subscript_steps", "{% set q = ((((([1][:, :] is undefined)|string)[0]|lower)[0]) is string) is true %}{{ q }}|{{ ((([1][:, :] is undefined)|string)[1:]) }}|{{ (([1][:, :] is undefined) is sameas true) is true }}", {"env": {"undefined": "strict"}}),
    ("tuple_of_slices_strict_subscript_step_missing_item", "{% if false and ((([1][:, :] is undefined)|string)[9] or 1) %}a{% endif %}b", {"env": {"undefined": "strict"}}),
    ("tuple_of_slices_strict_subscript_step_missing_attr", "{% if false and ((([1][:, :] is undefined)|string).x ~ 1) %}a{% endif %}b", {"env": {"undefined": "strict"}}),
    ("tuple_of_slices_strict_subscript_step_impossible", "{% if false and (((([1][:, :] is undefined)|string)[9]|abs) or 1) %}a{% endif %}b|{% if false and ((([1][:, :] is undefined) is divisibleby(0)) or 1) %}a{% endif %}b", {"env": {"undefined": "strict"}}),
    # an output's own fold catches every error (visit_Output); only a strict undefined it prints fails the compile (0556)
    ("tuple_of_slices_strict_output_fold_error_unrendered", "{% if false %}{{ ([1][:, :] is undefined)|length }}{% endif %}b", {"env": {"undefined": "strict"}}),
    ("tuple_of_slices_subscript_steps", "{{ ((([1][:, :] is undefined)|string)[0]) ~ x }}|{{ ((([1][:, :])|default('ab'))[1]) is string }}"),
    # a filter drops an argument it does not take; the argument's fold is still checked (0497)
    ("tuple_of_slices_dropped_filter_argument", "{{ x|first([1][:, :]) }}"),
    ("tuple_of_slices_dropped_filter_argument_folds", "{{ x|first([1][:, :]|sort) }}"),
]
