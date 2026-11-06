# Jinja2C++ for Python

`jinja2cpp` is a Python module that renders Jinja2 templates with the Jinja2C++ engine. Its
API is Jinja2's, so `import jinja2cpp as jinja2` works for the subset below. This page
describes phase P1b of docs/tasks/0248 (docs/tasks/0353). The rest of the subset, the fallback
to real Jinja2, the sandbox and the wheels are later phases (0354-0357).

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
  `jinja2.ext.do` and `jinja2.ext.loopcontrols` (by name or class, Jinja2's or ours).
  `optimized`, `bytecode_cache` and `enable_async=False` are accepted and change nothing.
- `env.from_string`, `env.get_template`, `Template(source, **options)`, and
  `render(*args, **kwargs)` with Jinja2's `dict(*args, **kwargs)` rule.
- `env.globals`, `env.filters` and `env.tests` as dicts of Python values and callables.
  A template binds the filters and tests when it loads, as Jinja2 does; globals are read at
  each render, and imported templates see them.
- The four Undefined classes, jinja2cpp's or Jinja2's own, and subclasses of them (the
  nearest of the four decides the policy). A Python callable given an undefined value gets an
  instance of the environment's class.
- `DictLoader` and `FileSystemLoader`, read by the engine (the file system through
  `ConfinedFileSystem`, which does not follow a link out of a search directory), and any other
  loader through its `get_source`. Jinja2's loader classes work too.
- Jinja2's exception classes with their attributes (`lineno`, `name`, `filename`, `source`).
  An exception that a Python filter, test, global or attribute raises is raised as itself.
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
- Not yet: the sandbox, `pass_context` and the other decorators, i18n, `make_module`,
  `select_template`, the other loaders' engine paths, custom extensions, async, the fallback to
  real Jinja2 (docs/tasks/0354).
