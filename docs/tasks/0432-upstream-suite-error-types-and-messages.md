---
status: done
priority: medium
area: parity
depends: [0355]
touches: [python/jinja2cpp/_debug.py, python/jinja2cpp/loaders.py, python/src/errors.cpp, python/src/loaders.cpp, src/template_parser.cpp, src/statements.cpp, src/error_info.cpp, include/jinja2cpp/error_info.h]
shares: [python/tests/upstream/expected-failures.txt]
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/830
---
# Error classes, messages and tracebacks the upstream Jinja2 suite checks

**Problem.** The upstream suite (0355) checks the class, message or attributes of an error where
jinja2cpp raises a different one. Each is a line of `python/tests/upstream/expected-failures.txt`:

- Classes: `{% for x in none %}` raises TypeError in Jinja2, TemplateRuntimeError here
  (`test_core_tags.py::TestForLoop::test_noniter`); an invalid `xmlattr` key raises ValueError
  (`test_filters.py::TestFilter::test_xmlattr_key_invalid[...]`, seven ids); a trans block with
  two plural names raises TemplateAssertionError (`TestNewstyleInternationalization::test_complex_plural`);
  `{% include ["a", "b"] %}` raises TemplatesNotFound with `.templates` and `.name`
  (`test_imports.py::TestIncludes::test_choice_includes`). Also `{{ s[missing:] }}` on a string
  raises TemplateRuntimeError where Jinja2 raises TypeError (the engine's own slice path; the
  host-object path was fixed in 0355).
- Messages: unknown and unclosed tags ("Encountered unknown tag 'endif'. Jinja was looking for
  ...", `TestParser::test_error_messages`, `test_debug.py::TestDebug::test_syntax_error`), bad
  trans blocks ("trans blocks can't be nested", "saw `wibble`"), a name a template does not export
  ("does not export the requested name"), a required block with content, and
  TemplateNotFound naming the search path (`TestFileSystemLoader::test_error_includes_paths`).
- `{% macro x(caller) %}`, whose body calls `caller`, is a TemplateAssertionError ("When defining
  macros or call blocks the special \"caller\" argument must be omitted or be given a default.");
  the engine reports UnexpectedToken. Calling a macro with a caller parameter both by position
  and as `caller=` raises TypeError, TemplateRuntimeError here
  (`test_regression.py::TestBug::test_double_caller_no_default`, moved from 0431).
- `TemplateSyntaxError.source` of an included template is None
  (`TestDebug::test_include_syntax_error_source`).
- Tracebacks: Jinja2 rewrites the traceback so it shows the template's file and line as a frame
  (`TestDebug::test_runtime_error`, `test_regular_syntax_error`).

**Proposal.** Map the engine's error codes to Jinja2's classes and messages where the engine
knows the case (most need a new ErrorCode or a parameter, not string matching); set `source`
for errors raised in included templates; add a template frame to the traceback the module
raises (CPython allows a synthetic frame through `types.TracebackType`).

**Done when.** The listed upstream tests pass through the module, and the C++ messages are
the same where the C++ API reports them.

## Decisions

Engine half (additive public API):

- `ErrorCode::TemplateSyntaxError = 1023` and `ErrorCode::TemplateAssertionError = 1024`: a construct
  Jinja2 rejects with a message of its own (its `TemplateSyntaxError`, and the compile-time
  `TemplateAssertionError`). `ExtraParams[0]` is Jinja2's message. A template that ends inside a
  statement, trans-block misuse and a required block with content are `TemplateSyntaxError`; an unknown
  pluralization variable, a trans variable defined twice and a `caller` parameter with no default that the
  body uses are `TemplateAssertionError`.
- `UnexpectedStatement` keeps `ExtraParams[0]` = the tag and gains `ExtraParams[1]` = Jinja2's sentence
  (`Encountered unknown tag 'endif'. Jinja was looking for ...`, parser.py `_fail_ut_eof`); `ToString` prints
  it. A tag name that is no tag at all now reports it too (it was `UnexpectedToken`).
- `ErrorCode::TemplateRuntimeError` gets `ExtraParams[1]` = `"TypeError"` or `"ValueError"` where the engine
  knows the class Jinja2 raises (not iterable, undefined slice bound, doubled `caller`, invalid `xmlattr`
  key); absent otherwise. The `xmlattr` message is Jinja2's, with the key's repr.
- A name a `from ... import` cannot find is an undefined value with Jinja2's message (`the template 'm'
  (imported on line 1) does not export the requested name 'x'`).
- `{% include x %}` tries the names of a host sequence in turn, as for a template list.
- A block name with a hyphen is `TemplateSyntaxError` with Jinja2's message.

Module half:

- The module raises `TemplateAssertionError` for 1024, Python's `TypeError`/`ValueError` for 17 with that
  `ExtraParams[1]`, `TemplatesNotFound` (`.templates`, `.name`) for an include of several names, and a
  FileSystemLoader's TemplateNotFound names its search path as it is at the raise (`jinja2cpp.loaders._not_found`).
- A syntax error in a template the render includes carries that template's name, file and source; the source
  of a template from a Python loader is read again from its `get_source` for the diagnostic.
- Tracebacks get a template frame (`jinja2cpp._debug.add_template_frame`, Jinja2's fake_traceback): the call's
  line between the caller and a callable's frames, and the line of a syntax error.
- Deliberate: tracebacks name `jinja2cpp.exceptions.*` (`test_debug.py::TestDebug::test_regular_syntax_error`,
  `test_syntax_error`); setting `__module__` to `jinja2.exceptions` would make pickling resolve to Jinja2's class.

**Result.** Upstream suite 607 -> 626 of 856 with this task (all listed tests pass but the two deliberate ones).
