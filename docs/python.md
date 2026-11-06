# Jinja2C++ for Python

`jinja2cpp` is a Python module that renders Jinja2 templates with the Jinja2C++ engine. Its
API is Jinja2's, so `import jinja2cpp as jinja2` works for the subset below. This page
describes phases P1b and P2 part A of docs/tasks/0248 (docs/tasks/0353, 0354). The decorators and
i18n, the fallback to real Jinja2, the sandbox, streaming and the wheels are later phases
(0354-0357).

## Install and test

The module needs CPython 3.12 or later and builds one stable-ABI (abi3) extension for all of
them. `markupsafe` is its one dependency; Jinja2 itself is only needed for the tests.

```bash
pip install .                       # scikit-build-core runs CMake with -DJINJA2CPP_BUILD_PYTHON=ON
pip install ".[test]" && pytest     # the tests compare with Python Jinja2 3.1.6
```

A development build in a CMake build directory:

```bash
cmake -S . -B build-py -G Ninja -DCMAKE_BUILD_TYPE=Release -DJINJA2CPP_BUILD_PYTHON=ON \
      -DJINJA2CPP_BUILD_TESTS=OFF -DJINJA2CPP_WITH_BIGINT=boost -DPython_EXECUTABLE="$(command -v python3)"
cmake --build build-py
cd build-py/python && PYTHONPATH=$PWD python3 -m pytest ../../python/tests
```

The build puts the package, with the module next to its sources, in `build-py/python/jinja2cpp`.
Run pytest from outside `python/`, or it imports the sources without the module. Under
AddressSanitizer (`-DJINJA2CPP_WITH_SANITIZERS=address+undefined`), preload the runtime into the
interpreter, and libstdc++ after it (an interpreter that links no libstdc++ otherwise aborts at the
first C++ exception): `LD_PRELOAD="$(gcc -print-file-name=libasan.so) $(gcc -print-file-name=libstdc++.so)"
ASAN_OPTIONS=detect_leaks=0`.
`.github/workflows/python.yml` runs the tests that way, then runs `pip install .`.

`JINJA2CPP_BUILD_PYTHON` links the library statically and position-independent into the
module, which uses its private headers (`src/host_object.h`). nanobind is pinned in
`thirdparty/internal_deps.cmake` and `pyproject.toml` (one version; the configure step checks
that they agree). pyproject.toml also turns on the built-in big-integer backend, so that
integer arithmetic beyond 64 bits is exact, as in Python.

`python/bench/bench_module.py` times Load and Render of every `bench/cases` case with both
engines on a Release build of the module.

## What works

- `Environment(...)` with the delimiters, `trim_blocks`, `lstrip_blocks`,
  `keep_trailing_newline`, `newline_sequence`, `line_statement_prefix`,
  `line_comment_prefix`, `autoescape` (a bool or a callable such as `select_autoescape()`),
  `undefined`, `finalize`, `loader`, `cache_size`, `auto_reload`, and the extensions
  `jinja2.ext.do`, `jinja2.ext.loopcontrols` and `jinja2.ext.i18n` (by name or class, Jinja2's or ours).
  `optimized`, `bytecode_cache` and `enable_async=False` are accepted and change nothing.
- `env.from_string`, `env.get_template(name, parent=None, globals=None)`, `Template(source, **options)`,
  and `render(*args, **kwargs)` with Jinja2's `dict(*args, **kwargs)` rule.
- Subclasses of `Environment` and `Template` in Python, as in Jinja2: an `__init__` that
  forwards its keyword arguments, attributes of any name, and `env.template_class` (the class
  `from_string`, `get_template` and `select_template` make; a subclass of `jinja2cpp.Template`).
  `get_template(name, parent=...)` and `select_template` call `env.join_path`, which a subclass
  may override.
- `env.select_template`, `env.get_or_select_template`, `env.list_templates(extensions=None,
  filter_func=None)` and `env.join_path`.
- `template.generate()` and `template.stream()` with `TemplateStream` (`dump`,
  `enable_buffering`, `disable_buffering`), which give the whole output as one item until
  streaming lands (docs/tasks/0355).
- `template.make_module(vars=None, shared=False, locals=None)` and `template.module`, from the
  engine's `Template::MakeModule`: the exports are attributes of a `TemplateModule`, `str()` is
  the rendered body, `__html__` is the same as Markup. An exported macro (also inside an exported
  list or dict) is a callable that keeps its module, the template and the environment alive, so
  it works after every other reference is gone.
- `jinja2cpp.meta.find_undeclared_variables` and `find_referenced_templates`, which take the
  result of `env.parse(source, name=None, filename=None)` (an opaque object).
- `env.globals`, `env.filters` and `env.tests` as dicts of Python values and callables.
  A template binds the filters and tests when it loads, as Jinja2 does; globals are read at
  each render, and imported templates see them.
- The four Undefined classes, jinja2cpp's or Jinja2's own, and subclasses of them (the
  nearest of the four decides the policy). A Python callable given an undefined value gets an
  instance of the environment's class.
- `DictLoader` and `FileSystemLoader`, read by the engine (the file system through
  `ConfinedFileSystem`, which does not follow a link out of a search directory), and any other
  loader through its `get_source`: `ChoiceLoader`, `PrefixLoader`, `FunctionLoader` and
  `PackageLoader` (a directory package or one imported from a zip file), also nested, and any
  `BaseLoader` subclass. Jinja2's loader classes work too. The `uptodate` function a `get_source`
  returns decides on a reload when `auto_reload` is on, as in Jinja2. `loader.load(env, name)`
  compiles a template from `get_source`. `ModuleLoader` raises `UnsupportedError`: it needs
  Jinja2's compiled modules.
- Jinja2's exception classes with their attributes (`lineno`, `name`, `filename`, `source`).
  When Jinja2 is importable each is also a subclass of the Jinja2 class of its name, so
  `except jinja2.TemplateNotFound` catches ours; without Jinja2 they stand alone.
  `jinja2cpp.UnsupportedError` is a `TemplateError` and a `NotImplementedError`: what the subset
  does not do raises it or `NotImplementedError`.
  An exception that a Python filter, test, global or attribute raises is raised as itself.
- `pass_context`, `pass_eval_context` and `pass_environment` (`jinja2cpp.pass_context`, also in
  `jinja2cpp.utils`; a function marked by Jinja2's own decorators works too, the marker
  `jinja_pass_arg` is read by its name). They apply as in Jinja2 to filters, tests, globals,
  `finalize`, callables passed as render values and methods of objects. The callable gets the
  environment (the object itself), an `EvalContext` (`autoescape`, `volatile`, `environment`),
  or a read-only `Context`: `get`, `[]`, `in`, `resolve`, `resolve_or_missing`, `get_all`, `call`,
  `keys`, `values`, `items`, `parent`, `vars`, `name`, `environment`, `eval_ctx`. A `Context` is
  lent for the call that received it: using it afterwards raises `TemplateRuntimeError`.
- i18n: `{% trans %}` (variables, `pluralize`, `trimmed`, a message context) and the `_`,
  `gettext`, `ngettext`, `pgettext` and `npgettext` globals, with `install_gettext_translations`,
  `install_null_translations`, `install_gettext_callables` and `uninstall_gettext_translations`.
  Newstyle gettext (`newstyle=True`, Flask-Babel's choice) runs in the engine
  (`TemplateEnv::InstallGettextCallables`); oldstyle, Jinja2's default, installs Python wrappers
  as the four globals.
- Ctrl-C (KeyboardInterrupt) stops a long render. The GIL is held for the whole render.

## How values cross

Python values are not copied: the engine asks the object, as Python's own operations would.
A dict, a list and any other object are seen live, so a change on either side is visible to
the other. An object that comes back to Python, as a callable's argument for example, is the
same object (`x is y`). Strings are copied in UTF-8, and a lone surrogate raises
`UnicodeEncodeError`.

| Python | In the template |
|---|---|
| `None`, `bool`, `int`, `float`, `str` | native values; an `int` beyond 64 bits is a big integer |
| `markupsafe.Markup` (`jinja2cpp.Markup` is the same class) | Markup |
| `dict` and other `collections.abc.Mapping` | a mapping: keys of any hashable type, `x.name` as `getattr` then item |
| `list`, `tuple`, namedtuple | sequences; a tuple prints as a tuple, namedtuple fields are attributes; `+`, `*`, `<`, `sort`, `min`, `max` and `%` treat them as Jinja2 does, and a loop follows the live length of a list it changes |
| generators and iterators | read once, as in Python |
| callables | called with Python's own argument binding |
| any other object | `getattr` first, then items; `str()`, `repr()` and `__html__` as Python uses them |

Going the other way, the engine's lists and dicts become `list` and `dict`, Markup becomes
`markupsafe.Markup`, and undefined becomes an instance of the environment's Undefined class.
A macro, `caller`, `loop` or another template object passed to a Python callable is lent for
that call: using it after the call returns raises `TemplateRuntimeError`.

## Differences from Jinja2

- Template errors that Jinja2 raises as Python's own exceptions (`ZeroDivisionError`,
  `TypeError` from an operator) are `TemplateRuntimeError` with the same message.
- `tojson` writes any mapping and any list or tuple; Python's `json.dumps` takes only `dict`.
- A dict literal of the template keyed by a Python object that defines its own `__hash__`
  (a tuple from Python, a frozen dataclass) keys it by identity (0248 plan, section 7).
- Methods of the engine's own values are the engine's: `x.hex()` on a float (and other
  Python methods Jinja2 lets a template call on an `int`, `float` or `str` the engine has no
  method for) raises `TemplateRuntimeError`, for values from Python as for literals (0403).
- `urlencode` reads any mapping as a dict; Jinja2 does so only for a `dict` and iterates
  another `Mapping`, which gives its keys (0404).
- A template keeps the `Environment.globals` dict it was made with, as Jinja2's does, but
  `get_template` makes a new `Template` with the dict of that moment, and an imported template
  sees the importer's: after `env.globals = {...}` Jinja2's cached templates still see the old
  dict (0405).
- An environment keeps every filter, test, `finalize`, `autoescape` callable and loader it was
  ever given alive for as long as it lives, because a template loaded earlier may still call
  one: replacing `env.filters` entries in a loop grows the environment (0406).
- `{% include %}`, `{% import %}`, `{% from %}` and `{% extends %}` read the loader through the
  engine, not through `env.get_template(name, parent=...)`, so an overridden `join_path` is not
  called for them (Jinja2 calls it with the including template's name); `get_template` and
  `select_template` call it (0410).
- `make_module` and `module` give the exports as Python values the host API can carry: a
  `namespace` becomes a dict, a `cycler` a dict, a `range` a list, a tuple a list, an `Undefined` is
  `None`, a dict key that is no str is spelled as `str()` spells it, and a Python object (not a str, number,
  list, dict, Markup or callable) stays itself in the module (attributes and methods are Python's), but a list or dict read off it and exported with `{% set %}` arrives as `{}`, and `dictsort` entries as `{key, value}` dicts. The params of a module are
  copied (Jinja2's module shares the dicts) and nested at most 1000 deep (`RecursionError`, 0418), and `shared=True` leaves out the environment's
  globals but not the engine's own (`range`, `dict`, `cycler`, `namespace`, `lipsum`) (0411).
- `env.parse` is a full load: it also reports what the compile reports (an unknown filter or test
  raises `TemplateAssertionError`), and returns an opaque object that only `jinja2cpp.meta`
  takes, not Jinja2's node tree. `find_referenced_templates` gives the names of the tags in
  source order, `None` for a name that is not a literal.
- `generate()` gives the output as one item, so `stream().enable_buffering(n)` has nothing to
  combine and an error in the template surfaces at the first `next()` (0355).
- The `Context` a `pass_context` callable gets sees every variable of the call site, the loop
  variables, `{% with %}` names and macro locals included, where Jinja2's holds only the
  arguments, the globals and the template's top-level names; its `vars` and `parent` both hold
  all of them (0416). `eval_ctx.volatile` is always False. `Context.derived`, `blocks` and
  `exported_vars` (empty) are not there.
- Oldstyle gettext is emulated with Python wrappers, which cannot tell a `{% trans %}` block from
  a direct call: `{% trans %}100% sure{% endtrans %}` without variables passes `100%% sure` to the
  translations and prints it; a count named `num` is not formatted (`%(num)s` stays); a block
  without variables is escaped under autoescape; `gettext('x %(a)s', a=1)` formats instead of
  raising TypeError (0415). Newstyle is exact.
- Without installed translations (and after `uninstall_gettext_translations`) the engine's
  `_()` and `{% trans %}` render the message untranslated; Jinja2 raises `UndefinedError`
  because it has no `gettext` global yet (0417). A `gettext` set in `env.globals` replaces the
  installed one in both.
- Not yet: the sandbox, custom extensions, async, the fallback to real Jinja2
  (docs/tasks/0354).
