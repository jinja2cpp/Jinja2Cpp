# Jinja2C++ for Python

`jinja2cpp` is a Python module that renders Jinja2 templates with the Jinja2C++ engine. Its
API is Jinja2's, so `import jinja2cpp as jinja2` works for the subset below, and what is outside
the subset goes to real Jinja2 when it is installed (see "Fallback"). This page describes phases
P1b and P2 of docs/tasks/0248 (docs/tasks/0353, 0354). Streaming and the wheels are later phases
(0355, 0356); since 0357 the module is built on the library's public API only.

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

The module uses the library's public API only (`include/jinja2cpp/`, docs/tasks/0357).
`JINJA2CPP_BUILD_PYTHON` links the library statically and position-independent into the
module, as the wheel carries it. nanobind is pinned in
`thirdparty/internal_deps.cmake` and `pyproject.toml` (one version; the configure step checks
that they agree). pyproject.toml also turns on the built-in big-integer backend, so that
integer arithmetic beyond 64 bits is exact, as in Python.

### Building against an installed library

`python/` also builds alone, against an installed `jinja2cpp` package found with
`find_package(jinja2cpp)`; nanobind then comes from the build's Python (`pip install
nanobind==3.1.0`, the pin of `pyproject.toml`). For a static library, install it position-independent,
with the initial-exec model for its hot thread-locals (a module the interpreter loads with
`dlopen` would otherwise pay a call into the loader on each access to them) and the built-in big
integers, as the wheel has them:

```bash
cmake -S . -B build-lib -G Ninja -DCMAKE_BUILD_TYPE=Release -DJINJA2CPP_BUILD_TESTS=OFF \
      -DJINJA2CPP_PIC=ON -DJINJA2CPP_TLS_INITIAL_EXEC=ON -DJINJA2CPP_WITH_BIGINT=boost \
      -DCMAKE_INSTALL_PREFIX="$PREFIX"
cmake --build build-lib && cmake --install build-lib
cmake -S python -B build-module -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$PREFIX" \
      -DPython_EXECUTABLE="$(command -v python3)"
cmake --build build-module
cd build-module && PYTHONPATH=$PWD python3 -m pytest ../python/tests
```

A shared library works too (`-DJINJA2CPP_BUILD_SHARED=ON`, without `JINJA2CPP_TLS_INITIAL_EXEC`).
Without the big integers the module configures with a warning, and integer arithmetic beyond
int64 is not exact (docs/tasks/0272). `-DJINJA2CPP_STRICT_WARNINGS=ON` turns the library's
warnings, as errors, on for the module's own code. The `installed` job of `python.yml` builds the
module this way and checks that every header it includes comes from the installed prefix,
nanobind, Python or the system, never from the checkout's `include/` or `src/`
(`python/check_includes.py`).

### Validation against Jinja2

Two suites hold the module to Python Jinja2 beyond its own tests (docs/tasks/0355):

- `python/tests/test_parity_corpus.py` renders every case of the parity corpus
  (`test/parity`) through the module. Each case must end as it does in the C++ `ParityTest`:
  a match where `test/parity/divergences/` lists nothing, the listed kind where it does. A
  difference is a bug of the crossing, unless `python/tests/parity/module-divergences.txt`
  lists it with its task.
- `python/tests/upstream/run.py` runs the pallets/jinja 3.1.6 `tests/` (from the sdist, checked
  against its SHA256) with their `jinja2` imports pointed at `jinja2cpp` and `fallback="never"`.
  `python/tests/upstream/expected-failures.txt` lists each failing test with a class:
  `internals` (not counted), `async`, `extension`, `unsupported` (outside the subset),
  `deliberate` or `divergence` with its task. The run prints the pass rate,
  passed / (collected - internals), and the rate within the supported subset, and fails on an
  unlisted failure or a listed test that passes.

```bash
pip install -r python/tests/upstream/requirements.txt   # pytest and trio, as the suite pins them
cd build-py/python && PYTHONPATH=$PWD python3 ../../python/tests/upstream/run.py
```

`python/bench/bench_module.py` times Load and Render of every `bench/cases` case with both
engines on a Release build of the module.

## What works

- `Environment(...)` with the delimiters, `trim_blocks`, `lstrip_blocks`,
  `keep_trailing_newline`, `newline_sequence`, `line_statement_prefix`,
  `line_comment_prefix`, `autoescape` (a bool or a callable such as `select_autoescape()`),
  `undefined`, `finalize`, `loader`, `cache_size`, `auto_reload`, and the extensions
  `jinja2.ext.do`, `jinja2.ext.loopcontrols` and `jinja2.ext.i18n` (by name or class, Jinja2's or ours;
  other extensions go to Jinja2, see "Fallback").
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
  `enable_buffering`, `disable_buffering`). They yield Jinja2's items (a run of text, the value of
  a `{{ }}`, a macro or call block's output; the engine's `RenderOptions::writeEachPiece`, 0436),
  and an error after the items written before it, as Jinja2 does; but the whole render runs at the
  first item, on the caller's thread (lazy rendering is docs/tasks/0454).
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
- The sandbox: `jinja2cpp.sandbox` with `SandboxedEnvironment`, `ImmutableSandboxedEnvironment`,
  `SecurityError`, `is_internal_attribute`, `modifies_known_mutable`, `unsafe`, `safe_range`,
  `MAX_RANGE` and the `UNSAFE_*` sets, ported from Jinja2 3.1.6 (see "Sandbox" below).
- Ctrl-C (KeyboardInterrupt) stops a long render. The GIL is held for the whole render.

## The supported subset

Three tiers (docs/tasks/0248, D1). Tier A is what the engine does natively and the tests compare
with Jinja2 3.1.6; each row names the test file (in `python/tests/`). Tier B is outside the
engine and goes to real Jinja2 ("Fallback"). Tier C never works.

**Tier A: native**

| Feature | Tests |
|---|---|
| `Environment(...)` delimiters, `trim_blocks`, `lstrip_blocks`, `line_statement_prefix`, `line_comment_prefix`, `newline_sequence`, `keep_trailing_newline`, `cache_size`, `auto_reload`, `optimized`/`bytecode_cache` (ignored) | `test_render.py` |
| `autoescape` as a bool, a callable or `select_autoescape()` | `test_render.py` |
| `undefined` as the four Undefined classes (ours or Jinja2's) and their subclasses | `test_undefined.py` |
| `finalize` | `test_render.py`, `test_context.py` |
| `extensions`: `do`, `loopcontrols`, `i18n`, by name or class | `test_render.py`, `test_i18n.py` |
| `SandboxedEnvironment`, `ImmutableSandboxedEnvironment` | `test_sandbox.py` |
| `Template(source, **options)`, `from_string`, `get_template`, `select_template`, `get_or_select_template`, `list_templates`, `join_path` | `test_render.py`, `test_environment_api.py`, `test_loaders_more.py` |
| `render(*args, **kwargs)`, `generate`, `stream`, `.name`, `.filename`, `.globals`, `.environment` | `test_render.py`, `test_environment_api.py` |
| `make_module`, `module` | `test_module.py` |
| `env.filters`, `env.tests`, `env.globals` as live dicts with Jinja2's binding times | `test_render.py` |
| Python callables as filters, tests, globals and context values | `test_values.py`, `test_render.py` |
| `pass_context`, `pass_eval_context`, `pass_environment` and the read-only `Context` | `test_context.py` |
| Loaders: `FileSystemLoader`, `DictLoader`, `FunctionLoader`, `ChoiceLoader`, `PrefixLoader`, `PackageLoader`, any `BaseLoader` subclass, Jinja2's own loader instances | `test_loaders.py`, `test_loaders_more.py` |
| The exception hierarchy with Jinja2's attributes, subclassing `jinja2.exceptions` when Jinja2 is installed | `test_errors.py`, `test_exceptions_jinja2.py` |
| `Markup`, `escape`; `jinja2.meta.find_undeclared_variables` and `find_referenced_templates` | `test_values.py`, `test_module.py` |
| i18n: `{% trans %}`, `gettext` and its kin, `install_*` | `test_i18n.py` |
| Python subclasses of `Environment` and `Template`, `template_class` | `test_environment_api.py` |
| Lifetimes: references, cycles, threads, Ctrl-C | `test_lifetime.py` |
| The fallback itself | `test_fallback.py` |

**Tier B: real Jinja2 when it is installed** (otherwise `UnsupportedError` with the reason)

| Feature | What happens |
|---|---|
| A custom `Extension` class (not `do`, `loopcontrols`, `i18n`), a dotted name of one | the environment is a Jinja2 environment |
| `enable_async=True`, `render_async`, `generate_async` | the environment is a Jinja2 environment; on a native template `render_async` is Jinja2's (it raises as Jinja2 does without `enable_async`) |
| `context_class`, `code_generator_class` of your own on an `Environment` subclass | the environment is a Jinja2 environment |
| `env.policies` with a key Jinja2C++ has no equivalent for or a changed value (it has none) | the templates loaded from then on are Jinja2's |
| `jinja2cpp.nativetypes.NativeEnvironment` | the environment is `jinja2.nativetypes.NativeEnvironment` |
| `compile_expression`, `lex`, `preprocess`, `compile` | called on a Jinja2 environment of the same configuration |
| `Template.blocks`, `new_context`, `root_render_func` | read from the template loaded by Jinja2 from the same source or name |
| A template the engine cannot parse (for example a subscript of a literal by a tuple of slices, `[1, 2][1:2, 3]`) | Jinja2 parses it; if Jinja2 rejects it too, Jinja2's error is raised |

**Tier C: never.** Bytecode-level internals of the engine: `jinja2.compiler` and
`jinja2.nodes` as engine objects (`env.parse` of a native environment is opaque, for
`jinja2cpp.meta`), `ModuleLoader` (Jinja2's compiled
modules), and the sandbox's `intercepted_binops` and `intercepted_unops`. They raise
`UnsupportedError` (a `TemplateError` and a `NotImplementedError`), whatever `fallback` says.

## transformers' chat templates

`jinja2cpp.integrations.transformers.patch()` makes transformers compile chat templates with
jinja2cpp: it replaces `transformers.utils.chat_template_utils._compile_jinja_template`, which
`apply_chat_template` uses (vLLM and SGLang call it too). Nothing changes until it is called;
`unpatch()` restores transformers' function and `is_patched()` tells.

```python
import jinja2cpp.integrations.transformers
jinja2cpp.integrations.transformers.patch()
```

The templates load in `chat_template_environment()`: an `ImmutableSandboxedEnvironment` with
`trim_blocks`, `lstrip_blocks` and `loopcontrols`, and transformers' `tojson`,
`raise_exception` and `strftime_now` built into the engine (`ConfigureForChatTemplates`,
docs/tasks/0360), so a render calls no Python. `{% generation %}` renders its body. What the
engine does not do runs in transformers' own template of the same source, so the result is
transformers': a template jinja2cpp cannot load, a render with
`return_assistant_tokens_mask=True` (inside `activate_tracker`), and the environment's other
attributes (`parse`, which transformers uses to list a template's variables).
`python/tests/test_chat_templates.py` renders every template and conversation of
`test/chat_templates` this way and requires transformers' output, and checks `patch()` against
the pinned transformers.

## Fallback

The engine never runs custom extensions, async or Jinja2's code generator. When Jinja2 is
installed the module hands those to it instead of refusing. The policy is `fallback=`, an
`Environment` keyword argument (also of `SandboxedEnvironment` and `Template(...)`):

| Mode | Effect |
|---|---|
| `"auto"` (default) | use Jinja2 silently when the engine cannot |
| `"warn"` | the same, and a `jinja2cpp.FallbackWarning` the first time for each reason |
| `"never"` | raise `jinja2cpp.UnsupportedError` with the reason; for a template the engine cannot parse the engine's own `TemplateSyntaxError` stands |

The default of environments that do not pass `fallback=` is a module setting:
`jinja2cpp.set_fallback("never")` (returns the previous mode; `jinja2cpp.get_fallback()` reads
it), or the environment variable `JINJA2CPP_FALLBACK=never` read once at first use. A test suite
sets `never` so that every use of Jinja2 is an error and its pass rate measures the engine.
Without Jinja2 installed a Tier B use raises `UnsupportedError` whatever the mode says.

It is decided in two places, never in the middle of an output:

1. **When an environment is constructed** with a Tier B option (a custom extension, `enable_async`,
   a `context_class` or `code_generator_class`, `NativeEnvironment`). The environment still
   satisfies `isinstance(env, jinja2cpp.Environment)` and has the same attributes, but a real
   `jinja2.Environment` (`jinja2.sandbox` for the sandbox classes) does the work. The loader, filters, tests,
   globals, `finalize` and `undefined` you give are used inside it: `env.filters`, `env.tests`
   and `env.globals` are its live dicts, and the loaders of jinja2cpp work (their `get_source` is
   asked; a loader that overrides `load` has it called). Its templates are `jinja2cpp.Template`
   objects (`isinstance` holds) whose `environment` is yours; `generate` really streams;
   `blocks`, `new_context`, `render_async` and the other Jinja2 attributes are the Jinja2
   template's. Errors that are Jinja2's `TemplateError`s are raised as the jinja2cpp class of the
   same name (so `except jinja2cpp.UndefinedError` catches them; they are also Jinja2's class);
   Python's own errors from a delegated render (`ZeroDivisionError`) stay Python's, where the
   engine raises `TemplateRuntimeError`. `env.parse` gives Jinja2's node tree, which
   `jinja2cpp.meta` takes too.
2. **When a template is loaded** and the engine cannot parse it. Jinja2 parses it with an
   environment of the same configuration: if it succeeds, the template is a Jinja2 one (as
   above); if it fails too, Jinja2's error is raised, as the jinja2cpp class, with Jinja2's
   message, line, name and source. Without Jinja2, or with `never`, the engine's own error stands.
A native template that includes, imports or extends a template the engine cannot parse is **not**
retried with Jinja2: the render raises the engine's `TemplateSyntaxError`, as `fallback="never"` does
(a render cannot be proven free of Python side effects after it started, so there is no safe point to
switch). Use an option that makes Jinja2 the environment (Tier B), or avoid the syntax the engine lacks in
included templates; task 0430 would route static include names at load time.

A divergence that is deliberate never triggers the fallback: a runtime error of the engine is
never handed to Jinja2, only a failure to parse.

`jinja2cpp.stats(reset=False)` returns a dict of counters, `environments_native`,
`environments_fallback`, `loads_native`, `loads_fallback`, `renders_native` and `renders_fallback`.
Loads are the templates the API calls make (`from_string`, `get_template`, `select_template`,
`Template(...)`) and Jinja2 really loaded: a `get_template` answered from the cache is not counted
again, nor is the template an include loads inside a render. The counters are locked, so they are exact
under threads. `stats(reset=True)` returns the counts and starts from zero (and forgets which warnings
were given).

The Jinja2 environment that stands for a native one is made once and rebuilt when one of its options
changes; Jinja2's own `get_template` cache (with the loader's `uptodate` check) serves the templates, so
`env.get_template("a") is env.get_template("a")` holds as in Jinja2. No lock is held while a loader runs (a loader
may call back into any environment), so threads racing on the first load of a name may each load it, as in Jinja2;
only the first wrapper published is kept and counted in `loads_fallback`, and once they settle one template is served. Its filters, tests, globals and policies
are chains over the live dicts of your environment: a change after a template was made is seen by it.

**Untrusted template text and the sandbox.** Which engine renders a template can depend on the template text:
syntax the engine lacks (a subscript by a tuple of slices, an extension tag) sends the template to Jinja2's
sandbox, where the engine's caps (`MaxSequenceSize`, the `str.format` cap, `MAX_RANGE` as `Limits::maxRange`)
do not apply, only Jinja2's own checks. A sandbox that renders text you do not trust, and relies on
the engine's caps, should be constructed with `fallback="never"`: a template using such syntax then raises
the engine's error instead of reaching Jinja2.

What the fallback does not carry over:

- **Overrides.** A Python subclass of `Environment` that overrides `join_path`, `getattr`,
  `getitem`, `call` and the like is not consulted by the Jinja2 environment that takes over.
  A sandbox is different: the Jinja2 sandbox that takes over asks your environment for every hook each time it
  runs: `is_safe_attribute`, `is_safe_callable`, `unsafe_undefined`, `call`, `getattr`, `getitem`,
  `wrap_str_format`, `call_binop`, `call_unop`, `intercepted_binops` and `intercepted_unops`. An override of the
  class, a patch of the class, a function set on the instance, before or after a template was made, is what
  runs, on every route (render, `generate`, `make_module`, `module`, `new_context`, `root_render_func`,
  `render_async`, `compile_expression`), as in Jinja2 (the interception sets are read when a template is
  compiled, as Jinja2 reads them). The *native* route refuses an overridden `call`, `getattr` or `getitem`, and
  `intercepted_*`, with `UnsupportedError` when a template loads, because the engine would not call them. To
  use them, run the whole environment on Jinja2: there is no option for that alone, but any Tier B option does
  it (`extensions=[SomeExtension]`, even one that adds nothing, or `enable_async=True`). A jinja2cpp
  undefined value that an overridden `unsafe_undefined`, `getattr`, `getitem` or `call` returns (made by
  `self.undefined` or `super()`) becomes an undefined of Jinja2 of the same kind (the environment's own `undefined` class
  becomes the Jinja2 environment's; any other jinja2cpp `Undefined`, `ChainableUndefined`, `DebugUndefined` or
  `StrictUndefined`, or a subclass, its nearest stock base), with its hint, object, name and exception; one stored elsewhere (in a list, say) is not converted. `call_binop` and `call_unop` always go to your
  environment, so its `binop_table` and `unop_table` apply. A hook is stock only if it is the sandbox's own
  function bound to your environment itself: `env.getattr = other.getattr` is an override.
  `context_class` and `code_generator_class` are used by the Jinja2 environment. `template_class`
  is used for the fallback templates too, but one that overrides `render`, `generate`, `make_module`,
  `module` or the other methods a fallback template implements itself (nanobind allows one
  nanobind base, so the fallback cannot sit between your class and the engine's) raises
  `UnsupportedError` naming them when a template falls back; its native templates are unaffected.
- **Translations.** `install_gettext_translations` on a delegating environment is Jinja2's. For a
  native environment whose template falls back, the oldstyle wrappers in `env.globals` are copied,
  but newstyle gettext (which runs in the engine) is not.
- **Undefined subclasses.** The four classes of jinja2cpp are mapped to Jinja2's, and a subclass of
  `jinja2.Undefined` is used as it is. A subclass of jinja2cpp's `Undefined` cannot be given to Jinja2
  (it asserts a `jinja2.Undefined` subclass, and the subclass's `super()` calls cannot be moved onto
  another base): the fallback raises `UnsupportedError` naming the class. Subclass `jinja2.Undefined`
  to use a custom one with the fallback.
- **`parse` and `jinja2cpp.meta`.** `env.parse` of a source the engine cannot parse gives Jinja2's node
  tree, and the meta functions take a fallback template too (Jinja2 parses its source again).
- **Files.** `FileSystemLoader` does not follow a link out of its search directories in either
  engine (the Python `get_source` checks it too), where Jinja2's loader does.
- **`Template` subclasses.** A subclass with an `__init__` of its own makes its template with the engine, so a source the engine
  cannot parse, or an option that makes Jinja2 the environment, raises `UnsupportedError` naming the class (Jinja2 rejecting the
  source too gives Jinja2's error). Make such a template with `env.from_string()`, or drop the `__init__`.
- **An include of an unparsable template** raises the engine's `TemplateSyntaxError` with a note (`__notes__`) that says what to do.
- **Cost.** A template remembers its source for `blocks`, `new_context` and the other attributes only Jinja2 has.
- `env.extensions` of a delegating environment is the list you gave, not Jinja2's dict of instances.

## Sandbox

`from jinja2cpp.sandbox import SandboxedEnvironment, ImmutableSandboxedEnvironment` are Jinja2's
classes (`import jinja2cpp as jinja2` works for `jinja2.sandbox` code). They are Python
subclasses of `jinja2cpp.Environment`, and a subclass may override `is_safe_attribute`,
`is_safe_callable`, `unsafe_undefined` and `wrap_str_format`, as in Jinja2. The policy runs
inside the adapters that let the engine read Python objects, not in generated code:

- **One checkpoint for attributes.** Every attribute a template reads of a Python object
  (`x.a`, `x[a]` that falls back to the attribute, `|attr`, `attribute=` of `map`, `sort`,
  `groupby`, `selectattr`, `rejectattr`, `sum`, `unique`, `min`, `max`, the fields of
  `str.format` and `format_map`) is one `PyHost::GetAttr`, which calls
  `env._sandbox_attribute(obj, name, value)`: Jinja2's `wrap_str_format`, then
  `is_safe_attribute(obj, name, value)`, then `unsafe_undefined`. A refused attribute is the
  undefined value Jinja2 makes, with its message (`access to attribute '__class__' of 'int'
  object is unsafe.`), that raises `SecurityError` when used (printing it is empty under the
  default `Undefined`, an error under `StrictUndefined`). Items (`x['k']`, `d.k` of a dict) are
  not attributes and are not asked, as in Jinja2.
- **Calls.** Every call of a Python object (a global, a method, a callable in the context) first
  calls `is_safe_callable`; `@unsafe` and `alters_data` raise `SecurityError("... is not safely
  callable")`. Filters and tests are not checked, as in Jinja2.
- **`range`.** `env.globals['range']` is `safe_range`, but a sandboxed render uses the engine's
  own `range` with `Limits::maxRange = MAX_RANGE`, so no Python call is made; the error is
  Jinja2's `OverflowError("Range too big. The sandbox blocks ranges larger than MAX_RANGE
  (100000).")`.
- **The engine's own values** (the template's strings, numbers, lists, dicts, `loop`, `cycler`,
  macros) have no Python attributes. The environment's settings hold a public `ISandboxPolicy`
  (`jinja2cpp/sandbox_policy.h`, `Settings::sandboxPolicy`, docs/tasks/0357 B7) that the engine asks where a native value's attribute is
  missing, after it finds one of its methods, and at `str.format`. A private name (`__class__`,
  `__init__`, `_x`) on one is asked of `is_safe_attribute` with a stand-in object of the Python
  type and is refused with the same undefined; so is a public name when the subclass's
  `is_safe_attribute` is not Jinja2's. `format_map` of a string, which the engine lacks, and
  `str.format` are Python's, run through `SandboxedFormatter`. `ImmutableSandboxedEnvironment`
  also refuses the methods of the engine's lists and dicts that change them (`append`, `update`,
  ...) with Jinja2's message and `ErrorCode::SecurityError` (`Dialect::ImmutableSandbox`, the read-only containers of the
  transformers preset).
- **Cost.** A plain `Environment` pays a null check where the policy would be asked: the checks
  are behind a pointer the render already holds, which only a sandboxed environment sets. A sandboxed render calls `is_safe_attribute` once
  per attribute of a Python object (not per dict key).
- `intercepted_binops` / `intercepted_unops` are not supported: loading a template of an
  environment with either set raises `UnsupportedError`, since ignoring them would let the
  operators through (0412). `call_binop` and `call_unop` exist and work when called. This is the native
  route; an environment that Jinja2 runs (a Tier B option, "Fallback") honours both.


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
  copied (Jinja2's module shares the dicts) and nested at most 1000 deep (`RecursionError`, 0418), with at most 1048576 values per call once shared references are copied out (`MemoryError`), and `shared=True` leaves out the environment's
  globals but not the engine's own (`range`, `dict`, `cycler`, `namespace`, `lipsum`) (0411).
- `env.parse` is a full load: it also reports what the compile reports (an unknown filter or test
  raises `TemplateAssertionError`), and returns an opaque object that only `jinja2cpp.meta`
  takes, not Jinja2's node tree. `find_referenced_templates` gives the names of the tags in
  source order, `None` for a name that is not a literal.
- `generate()` runs the whole template at the first item, where Jinja2 runs it item by item (0454).
  Jinja2 folds constant output into the text around it (`a{{ 1 }}b` is one item, here three), and
  yields `super()`, `self.block()` and a recursive loop's call as one item, and an empty `{{ }}` as
  `''` (0452).
- Tracebacks name the classes `jinja2cpp.exceptions.*`, where Jinja2's say `jinja2.exceptions.*`
  (0432). They show the template's file and line as a frame, as Jinja2's do: the line of the call
  for an error a callable raised, and the line of a syntax error.
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
- Sandbox (0412, 0413, 0414): `intercepted_binops` and `intercepted_unops` raise
  `UnsupportedError` (Jinja2's `test_binary_operator_intercepting` and
  `test_unary_operator_intercepting` are `xfail`). A public name of the engine's own values
  (`s.upper`, `d.items`) is asked of `is_safe_attribute` only when the subclass overrides it,
  the immutable form's refusal of the engine's list and dict methods is the engine's own, a
  bound method of the engine's values (`s.upper`) has `__self__` (the string, which Jinja2's
  sandbox refuses as a private name; it hands the template nothing it did not hold), a dict key named like an
  attribute (`{'__class__': 1}.__class__`) reads the key, `f.__call__` of a macro or global
  function is the function itself (0214), and under `StrictUndefined` a private name an engine
  object lacks (`loop._x`) raises `SecurityError` where Jinja2 raises `UndefinedError`. An
  overridden `call`, `getattr` or `getitem` of the environment (on the subclass or the instance) is
  refused with `UnsupportedError` when a template loads, renders, makes a module, or a macro of a module or one handed to a Python callable runs (so an override set after the load counts; a hook is the stock one only if it is the sandbox's own function bound to that environment, so a method of another environment or a patched class is an override), as the engine would not call it and Jinja2
  does (a subclass that calls `super()` is refused too); override `is_safe_attribute` or
  `is_safe_callable`. A subclass whose `is_safe_attribute` is not Jinja2's is also asked about the
  public names of `loop`, `cycler`, a namespace and a macro (with `None` as the value, as the engine
  builds none for the question); that is read again at each load, so a method replaced on the class
  or the instance between loads counts for the templates loaded after it, not for those already
  loaded. The groupby tuple's `grouper` and `list` are not asked (0425).
  `str.format`, `format_map` and `Markup.format` go through Jinja2's `SandboxedFormatter`; a field's
  width or precision, and the text the fields write together, are checked against the render's
  sequence cap (64 MiB or `Limits.max_sequence_size`) before Python builds the string, and fail with
  the `TemplateRuntimeError` the engine's own `str.format` raises (Jinja2 itself builds it). Widths and precisions are read as CPython reads them, any Unicode decimal digit; a spec outside the standard grammar is bounded by the largest number in it. The template's `{{ x|attr('__class__') }}` of an undefined `x` is an
  `UndefinedError`. `sort` and `unique` of an unsafe or missing attribute under
  `StrictUndefined` do not raise as Jinja2's do (0413). `sum(attribute=)` of a missing attribute
  is now an `UndefinedError` as in Jinja2 (it was a `TemplateRuntimeError`).
