---
status: done
pr: https://github.com/jinja2cpp/jinja2cpp/pull/944
priority: low
area: parity
depends: [0510]
touches: [src/engine_errors.h, src/error_boundary.h, src/python_format.cpp, src/value_methods.cpp, python/src/errors.cpp]
shares: [test/errors_test.cpp, python/tests/test_i18n.py]
---
# A missing format key raises no KeyError

**Problem.** `{{ '%(b)s' % {'a': 1} }}`, `{{ '{0[b]}'.format({'a': 1}) }}` and `{{ {'a': 1}.pop('b') }}` raise Python's
`KeyError: 'b'` in Jinja2. Jinja2C++ throws a plain `TemplateRuntimeError` whose message is
`KeyError: 'b'` (src/python_format.cpp `Key`; in src/value_methods.cpp the format field lookup and `dict.pop`), so the
Python module raises `jinja2cpp.exceptions.TemplateRuntimeError` and its `str()` carries the
`KeyError: ` prefix that Python's `str(KeyError('b'))` (`'b'`) does not. Left by 0510, whose
`TemplateRuntimeError::PythonClass` has no KeyError (TypeError, ValueError and AttributeError).

**Proposal.** Add `PythonClass::KeyError` (ExtraParams[1] `"KeyError"`), throw it with the key's repr
as the message at both sites (the C++ text keeps a `KeyError: ` prefix only if ErrorInfo's message
needs it), and map it to `PyExc_KeyError` with the key object in python/src/errors.cpp.

**Done when.** Both templates raise `KeyError` with `str()` `'b'` through the module as in Jinja2, with
rows in `ErrorsJinjaMessagesTest.RuntimeErrorsNameTheirPythonClass` and
`test_percent_formatting_raises_the_class_jinja2_raises`.

**Resolution** (PR #944). `TemplateRuntimeError::PythonClass::KeyError` (ExtraParams[1] `"KeyError"`), thrown by
`methods::ThrowKeyError` at the three sites (`%(b)s`, `{0[b]}`, `dict.pop`). The C++ message keeps
`KeyError: <repr>`, as ErrorInfo's text of a TemplateRuntimeError is the message alone. The key is
ExtraParams[2] when it is None, a bool, a number or a string, copied so that it outlives the render and the
template; the module raises `KeyError(key)`. A tuple, a callable or a host value as the missing key is not kept
(its items may be lent by the render), and the module then raises `KeyError` of the repr text, whose `str()`
adds quotes Jinja2's lacks. Rows: `ErrorsJinjaMessagesTest.RuntimeErrorsNameTheirPythonClass`,
`ErrorsJinjaMessagesTest.KeyErrorKeepsItsPlainKey` and `test_percent_formatting_raises_the_class_jinja2_raises`.
