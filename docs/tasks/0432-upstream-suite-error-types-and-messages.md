---
status: open
priority: medium
area: parity
depends: [0355]
touches: [python/jinja2cpp/exceptions.py, python/src/errors.cpp, src/template_parser.cpp, src/statements.cpp]
shares: [python/tests/upstream/expected-failures.txt]
---
# Error classes, messages and tracebacks the upstream Jinja2 suite checks

**Problem.** The upstream suite (0355) checks the class, message or attributes of an error where
jinja2cpp raises a different one. Each is a line of `python/tests/upstream/expected-failures.txt`:

- Classes: `{% for x in none %}` raises TypeError in Jinja2, TemplateRuntimeError here
  (`test_core_tags.py::TestForLoop::test_noniter`); an invalid `xmlattr` key raises ValueError
  (`test_filters.py::TestFilter::test_xmlattr_key_invalid[...]`, seven ids); a trans block with
  two plural names raises TemplateAssertionError (`TestNewstyleInternationalization::test_complex_plural`);
  `{% include ["a", "b"] %}` raises TemplatesNotFound with `.templates` and `.name`
  (`test_imports.py::TestIncludes::test_choice_includes`).
- Messages: unknown and unclosed tags ("Encountered unknown tag 'endif'. Jinja was looking for
  ...", `TestParser::test_error_messages`, `test_debug.py::TestDebug::test_syntax_error`), bad
  trans blocks ("trans blocks can't be nested", "saw `wibble`"), a name a template does not export
  ("does not export the requested name"), a required block with content, and
  TemplateNotFound naming the search path (`TestFileSystemLoader::test_error_includes_paths`).
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
