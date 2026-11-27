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

A shared library works too (`-DJINJA2CPP_BUILD_SHARED=ON`, which with glibc takes the initial-exec
model by default, docs/tasks/0514).
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
  and `render(*args, **kwargs)` with Jinja2's `dict(*args, **kwargs)` rule. `env.cache` is Jinja2's
  (`None` for `cache_size=0`, a dict for a negative size, else an LRU of that size, keyed by
  `(weakref.ref(loader), name)`): `get_template` gives the same `Template` again while it is cached
  and the engine's cache holds it unchanged (not reloaded by `auto_reload`, the loader's `uptodate`),
  and a call with `globals` updates that template's globals, as Jinja2's `_load_template` (0433).
  Clearing `env.cache`, deleting an entry or replacing it drops the engine's cache as a whole, so the
  next `get_template` of every name reads the loader again and gives a new `Template` (Jinja2 keeps the
  other entries). A change of an option the engine environment is rebuilt for (`autoescape`, `filters`,
  ...) also gives new templates, which take the new options; Jinja2 gives the cached ones.
- Subclasses of `Environment` and `Template` in Python, as in Jinja2: an `__init__` that
  forwards its keyword arguments, attributes of any name, and `env.template_class` (the class
  `from_string`, `get_template` and `select_template` make; a subclass of `jinja2cpp.Template`).
  `get_template(name, parent=...)`, `select_template` and the tags `include`, `import`, `from` and
  `extends` call `env.join_path(name, parent)` (the tags with the name of the template whose source holds
  them, not for a template from `from_string`), which a subclass may override; the joined name is what
  is loaded and cached (0410). The engine learns of an override when a template loads: a `join_path`
  assigned or overridden after a load applies to the templates loaded after it, and a template loaded
  earlier keeps loading names as written. An exception of `join_path` fails the render with it, also
  a `TemplateNotFound` that Jinja2 would skip for `ignore missing` or a name of a list.
- `env.select_template`, `env.get_or_select_template`, `env.list_templates(extensions=None,
  filter_func=None)` and `env.join_path`.
- `template.generate()` and `template.stream()` with `TemplateStream` (`dump`,
  `enable_buffering`, `disable_buffering`). They yield Jinja2's items (a run of text, the value of
  a `{{ }}`, a macro or call block's output; the engine's `RenderOptions::writeEachPiece`, 0436),
  and an error after the items written before it, as Jinja2 does. The template runs as far as the
  items asked for, on the caller's thread (0457): the engine's render runs on a fiber of its own (an
  8 MiB stack reserve, committed as used) and switches back at each item, so callables, `threading.local`
  state, locks and `contextvars` are the consumer's, and `close()` (or collecting the generator) ends
  the render where it stopped.
- `template.make_module(vars=None, shared=False, locals=None)` and `template.module`, from the
  engine's `Template::MakeModule`: the exports are attributes of a `TemplateModule`, `str()` is
  the rendered body, `__html__` is the same as Markup. An exported macro (also inside an exported
  list or dict) is a callable that keeps its module, the template and the environment alive, so
  it works after every other reference is gone. As Jinja2's `Macro`, it takes an `EvalContext`
  (`jinja2cpp.nodes.EvalContext`, Jinja2's too) as an optional first argument, whose `autoescape`
  decides whether the result is Markup (0434).
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
  An exception that a Python filter, test, global or attribute raises is raised as itself, except
  a `StopIteration`: a call in the template makes it undefined (Jinja2's `Context.call`), and out of
  a filter, test or `finalize` it ends `render`, `generate`, `stream` and `module` as
  `RuntimeError: generator raised StopIteration` with the `StopIteration` as its cause, as Jinja2's
  generator render does (PEP 479, 0477). A `StopIteration` inside a call the template makes (a macro,
  `caller()`, `loop(...)`) is undefined there too, as in Jinja2, whatever raised it in the body
  (a filter, test, `finalize`, a property); out of an included, imported or extended template, a block,
  `self.b()` or `super()` it stays the `RuntimeError` (0479). A `generate()` or `stream()` holds a call's
  output back until the call returns, as Jinja2 buffers it, so a stopped call yields nothing of it there
  either (0499).
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
  (`TemplateEnv::InstallGettextCallables`); oldstyle, Jinja2's default, installs the callables
  themselves as the four globals, and a `{% trans %}` block loaded while `newstyle_gettext` is off
  formats their result in the engine (`Settings::Extensions::i18nNewstyle`), as Jinja2 does.
  Before an install and after `uninstall_gettext_translations` the four globals are undefined, as in
  Jinja2: `_()` and `{% trans %}` raise `UndefinedError` (0417).
- The sandbox: `jinja2cpp.sandbox` with `SandboxedEnvironment`, `ImmutableSandboxedEnvironment`,
  `SecurityError`, `is_internal_attribute`, `modifies_known_mutable`, `unsafe`, `safe_range`,
  `MAX_RANGE` and the `UNSAFE_*` sets, ported from Jinja2 3.1.6 (see "Sandbox" below).
- Ctrl-C (KeyboardInterrupt) stops a long render, `Template.module` and a call of a module's macro
  (0440). The GIL is held for the whole render.

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
| `enable_async=True`, `render_async`, `generate_async` | the environment is a Jinja2 environment; on a native template `render_async` is Jinja2's (it raises as Jinja2 does without `enable_async`), and `generate_async` streams the engine's lazy render (Differences, 0461) |
| `context_class`, `code_generator_class` of your own on an `Environment` subclass | the environment is a Jinja2 environment |
| `env.policies` with a value the engine cannot take (a number for `urlize.rel`, a one-shot iterator for `urlize.extra_schemes`, a missing key, a `json.dumps_kwargs` that is not a dict); `urlize`, `truncate`, `tojson` and `{% trans %}` read the others natively (0435) | the templates loaded from then on are Jinja2's; one set after a native load raises `UnsupportedError` naming the key when the template renders |
| `jinja2cpp.nativetypes.NativeEnvironment` | the environment is `jinja2.nativetypes.NativeEnvironment` |
| `compile_expression`, `lex`, `preprocess`, `compile` | called on a Jinja2 environment of the same configuration |
| `Template.blocks`, `new_context`, `root_render_func` | read from the template loaded by Jinja2 from the same source or name |
| A template the engine cannot parse (rare since task 0497: for example, with StrictUndefined, a tuple-of-slices subscript in a conditional's branch, docs/tasks/0527) | Jinja2 parses it; if Jinja2 rejects it too, Jinja2's error is raised |

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
syntax the engine lacks (an extension tag, or one of the tuple-of-slices folds of docs/tasks/0527) sends the template to Jinja2's
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
  `is_safe_attribute` is not Jinja2's. `str.format` and `str.format_map` of the engine's
  strings are its own methods, which ask the policy's `Format`: Python's, run through
  `SandboxedFormatter` (0466). `ImmutableSandboxedEnvironment`
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
| any other object | `getattr` first, then items; `str()`, `repr()` and `__html__` as Python uses them; `is number` is `isinstance(x, numbers.Number)` (a `complex`, `Decimal` or `Fraction` too), and the `int` and `float` filters call `int(x)` and `float(x)` (0434) |

Going the other way, the engine's lists and dicts become `list` and `dict`, Markup becomes
`markupsafe.Markup`, and undefined becomes an instance of the environment's Undefined class.
A macro, `caller`, `loop` or another template object passed to a Python callable is lent for
that call: using it after the call returns raises `TemplateRuntimeError`.

## Operators on Python objects

`+ - * / // % **`, `< <= > >=` and unary `-` and `+` on an object that is not a `dict`, `list`, `tuple` or one of the
engine's own values (a `Decimal`, a `Fraction`, a `datetime` or `timedelta`, a class with `__add__` or
`__lt__`) are run by Python: the module calls `PyNumber_Add` and its siblings and `PyObject_RichCompare`, so reflected
methods (`1 + d`, `1 > d`), the subclass-first rule and `NotImplemented` are Python's. The same holds where the engine
orders values: `sort`, `min`, `max`, `groupby`, `dictsort` and `sum`, and the tests `odd`, `even` and `divisibleby`. A
`dict`, `list` or `tuple` keeps the engine's rules (the parity corpus is unchanged).

- Unsupported operands raise Python's own `TypeError` with Python's text (`unsupported operand type(s) for +:
  'decimal.Decimal' and 'float'`), not `TemplateRuntimeError`; any other exception of the method is the template's.
  A `StopIteration` becomes a `RuntimeError`, as in Jinja2.
- `bytes`, `bytearray`, `array.array`, `collections.deque` and other types that repeat with `*` or concatenate with `+`
  are checked against the sequence cap before Python builds the result, so `{{ b * 10**9 }}`, or a loop that doubles a
  value with `+`, fails with "too long" instead of allocating, as a `str` or `list` does. Jinja2 would allocate it. A type that multiplies elementwise (numpy's `ndarray`) is not a repeat and is not checked.
  A subclass that overrides `__len__` is checked by the storage it repeats, and a `deque` with a `maxlen` by that bound.
- `%` formatting of a `bytes` or `bytearray` format counts its own text, its widths and numeric precisions (written or
  taken by `*`) and the bytes a `%s`, `%b`, `%r` or `%a` copies from a buffer against the same cap first:
  `{{ fmt % (10**9, b) }}` with `fmt=b'%*s'` fails with "too long", as does a loop of `fmt % (v, v)`. `bytes + memoryview`
  counts the view's bytes. A `bytes`, `bytearray`, `array` or `deque` result is checked against the cap as well. The
  check reads a `%(key)` value and exports a buffer once more than Python does, so a mapping or buffer with side effects
  sees an extra call. A class whose own
  Python `__mul__`, `__add__` or `__mod__` builds the result runs its own code first, as Jinja2 does; a `bytes`,
  `bytearray`, `array` or `deque` it returns is then checked.
- The cost of a number's own arithmetic (`Fraction ** 10**8`) is not capped by `MaxBigIntBits`; Python Jinja2 behaves
  the same (task 0412).

## Differences from Jinja2

- `env.policies` is read at each render (`urlize.rel`, `urlize.target`, `urlize.extra_schemes`, `truncate.leeway`,
  `json.dumps_function`, `json.dumps_kwargs`) and compared with a copy, so an in-place edit counts; `ext.i18n.trimmed` is read
  when a template loads, as Jinja2 reads it when it compiles `{% trans %}`. `compiler.ascii_str` and unknown keys are ignored,
  as Jinja2 3 does. A macro of `t.module` or `make_module` reads them at each call from Python (0557). A filter whose arguments are all constants (`{{ 'abcdefghijk'|truncate(10) }}`,
  `{{ {'a': 1}|tojson }}`) follows a policy changed after the load, where Jinja2 folds it to its value when it compiles the
  template; the engine does not fold. Only the current `json.dumps_function` is kept: a template loaded under an earlier one
  keeps it for as long as the template lives, and a replaced one is released with the last template that used it.
- The policies and the `dumps` function are read from Python into the engine at each entry: `render`, `generate`, `module`
  and a macro handed to Python. A change that Python code makes during a render (a callable that edits `env.policies`, a
  `dumps` function that replaces itself, a macro called back from Python in a plain `Environment`) applies from the next
  entry, where Jinja2 applies it at the next filter call.
- Template errors that Jinja2 raises as Python's own exceptions (`ZeroDivisionError`,
  `TypeError` from an operator of a template value) are `TemplateRuntimeError` with the same message, except where the engine
  names the class: `%` formatting (also of `format` and newstyle `gettext`) raises `ValueError`,
  `TypeError` or `OverflowError` as Python does (0510; a missing key is not a `KeyError` yet, 0511).
- `tojson` writes any mapping and any list or tuple; Python's `json.dumps` takes only `dict`.
- A dict literal of the template keyed by a Python object that defines its own `__hash__`
  (a tuple from Python, a frozen dataclass) keys it by identity (0248 plan, section 7).
- Methods of the engine's own values are the engine's. It has every public method of Python's
  `str`, `int` and `float` except those of `bytes` and one alternate constructor:
  `str.encode`, `int.to_bytes` and `int.from_bytes` (the engine has no `bytes` type), and
  `float.fromhex`. Calling one raises `TemplateRuntimeError`, for values from Python as for
  literals (0403).
- A template keeps the `Environment.globals` dict it was made with, as Jinja2's does, and so does
  the one `get_template` gives from its cache (0433), but an imported template sees the importer's
  dict: after `env.globals = {...}` Jinja2's imports of a cached template still see the old one
  (0405). Likewise the `globals` a `get_template` call gave reach that template's imports and
  `without context` includes, which in Jinja2 see the environment's. This is deliberate (0405,
  decided 2026-11-20): a render has one global scope, which its includes and imports share; a scope
  of their own would convert the globals at each nested load of every render, and keep them alive
  for the macros that outlive it. Change `env.globals` in place, not by replacing the dict, to see
  the same in both.
- A template binds the filters and tests when it loads and keeps them, with the `finalize`,
  `autoescape` callable and loader it was loaded with, where Jinja2 looks a filter up in
  `env.filters` at each render: a template loaded before `env.filters['f']` is replaced keeps
  calling the old `f`. The environment lets go of a replaced one once the templates loaded with
  it go (0406); a load after a filter or test is replaced or removed reads the loader again.
- An exported macro of `make_module` and `module` has `name`, `arguments`, `catch_kwargs`, `catch_varargs` and
  `caller` as Jinja2's `Macro` has them (0434).
- `make_module` and `module` give the exports as Jinja2 gives them: a Python object, and a list or dict read off one,
  stay themselves (`module.o is o`), an `Undefined` is the environment's `Undefined` class, a tuple and a `dictsort`
  entry are tuples, and a dict keeps its own keys (0411). A `namespace`, a `cycler` and a `joiner` are the module's own
  objects: `module.ns.count` is what a macro set last, `module.cyc.next()` advances the cycler the macros read, `next(module.cyc)`
  and `module.j()` work, and `repr(module.ns)` is `<Namespace {'count': 1}>`. They outlive the template, as a macro does. Still
  different: a `range` is a list; the object is a `ModuleObject`, not a `jinja2.utils.Namespace` (`isinstance` is false, `callable()`
  is true of every one); Python cannot set an attribute of `module.ns` and an exported list or dict is a copy, not shared with the macros (deliberate: no shared mutable host data, 0575); each read of a nested namespace
  (`module.ns.inner`) is a new object, though one the module shares; a namespace the module holds itself (`ns.me = ns`)
  reprs to a `RecursionError` where Jinja2 prints `{...}`; and one passed back to a macro as an argument is the module's object,
  but inside a list or dict argument it is a copy (a cycler or joiner there cannot be called). The params of a module are
  copied (Jinja2's module shares the dicts) and nested at most 1000 deep (`RecursionError`, 0418), with at most 1048576 values per call once shared references are copied out (`MemoryError`), and `shared=True` leaves out the environment's
  globals but not the engine's own (`range`, `dict`, `cycler`, `namespace`, `lipsum`).
  Calls and reads of one module from several threads take turns, waiting with the GIL released; a Python callable in
  a macro that joins a thread using the same module, or two modules whose macros call each other from two threads, deadlock (0411).
- A dict view (`keys()`, `values()`, `items()`) or a set (`d.keys() - []`) of the engine reaches a Python callable as a real
  `dict_keys`, `dict_values`, `dict_items` or `set`, a snapshot of its items at the call (a set of items Python cannot hash
  stays a list); its `.mapping` is then a snapshot dict, whose values are `None` for `dict_keys` and the item positions for
  `dict_values`. A host dict's own views have the engine's set operators (`-` on either side, `<`, `<=`, `>`, `>=` with another
  view) over the items they give at that moment, and their `.mapping` is Python's mappingproxy, which `pprint` prints as
  `mappingproxy({...})` and `tojson` refuses. `d | random` of a host dict indexes it as `random.choice` does (`KeyError: 0`
  unless it has the key 0) (0545).
- `env.parse` is a full load: it also reports what the compile reports (an unknown filter or test
  raises `TemplateAssertionError`), and returns an opaque object that only `jinja2cpp.meta`
  takes, not Jinja2's node tree. `find_referenced_templates` gives the names of the tags in
  source order, `None` for a name that is not a literal.
- A `generate()` generator renders on the thread of its first `next()`: `next()` on another thread
  raises `RuntimeError`, which ends the generator, where Jinja2 goes on there (0457 D2). To hand a
  render to another thread (Starlette's or FastAPI's `StreamingResponse` over a sync iterator calls
  `next()` in a thread pool), stream `t.generate_async()` instead, or `render()` the template. A generator closed or collected on another thread, or after the interpreter began to
  exit, is not resumed: its render stops where it was, and what it held is not released.
- `generate_async()` of a native template is `generate()` as an async iterator: each item renders when the
  event loop's thread asks for it, so `StreamingResponse(t.generate_async(...))` streams (0461). Jinja2
  allows it only with `enable_async=True` and raises `RuntimeError` without; here `enable_async=True`
  makes the environment Jinja2's, whose `generate_async` awaits what callables return. A native
  template's does not await them, as Jinja2 without async mode. `render_async` stays Jinja2's.
- A reference cycle through a suspended generator is collected, as Jinja2's is, when it runs
  through the generator's arguments or the globals of its render (a param that refers back to the
  generator, 0462); one through a value the template made from them while it runs (`{% set y = x.y %}`
  with the generator in `x.y`) is not, until the generator is closed.
- `generate()` runs the whole template at the first item, as before 0457, where lazy rendering is not
  available: when `greenlet` is imported (gevent, eventlet: its stack switching and a fiber cannot share
  a thread), on CPython 3.14.0 and versions after 3.15 (not yet checked), on platforms other than
  Windows and x86-64 or AArch64 Linux, macOS and the BSDs, under a CET or GCS shadow stack, past 8
  generators nested in each other's callables on a thread, or when no stack can be reserved.
- `generate()` yields `Markup` items where Jinja2 does (0493), but a `{% filter %}` block whose filter
  returns something else than a string (`length`) yields its `str()`, as the engine's output is text.
- A Python loader's `get_source` runs again when a template it gave fails to compile, to show
  the line of the error (twice more with the default fallback, which loads it through Jinja2);
  Jinja2 calls it once.
- Tracebacks name the classes `jinja2cpp.exceptions.*`, where Jinja2's say `jinja2.exceptions.*`
  (0432). They show the template's file and line as a frame, as Jinja2's do: the line of the call
  for an error a callable raised, and the line of a syntax error.
- The `Context` a `pass_context` callable gets holds, as Jinja2's, the template's top-level names in
  `vars` and its arguments and globals in `parent` (and in `keys()`, `items()`, `get_all()`), with no
  loop variable, `{% with %}` name or macro local (0416). A call written in a loop or block body after
  a `{% set %}` there, and any call in a `scoped` block, gets Jinja2's derived context (`vars` empty, the
  names the body stored and the block's call site sees on top in `parent`). When a body both sets a
  name and defines a macro or imports a name under it, the context shows the name's current value,
  where Jinja2's shows the value the `set` stored; a call in a loop body after an inner loop ran sees
  the outer body's names, where Jinja2's sees those of the inner loop's last pass (its `_loop_vars` is
  one variable each inner pass starts again); and a body with more than 65535 distinct `set` names
  shows the first 65535 only. `eval_ctx.volatile` is always False.
  `Context.derived`, `blocks` and `exported_vars` (empty) are not there.
- A Python callable gets Python's own `range` and `dict` for the template's globals, and Jinja2's
  `jinja2.utils.Cycler`, `Joiner`, `Namespace` and `generate_lorem_ipsum` for `cycler`, `joiner`, `namespace` and
  `lipsum`, as Jinja2 passes them (when Jinja2 is not installed, those four are lent as the other template callables
  are). Given back while that call runs, such an object is the template's global again (`f(range) is sameas range`);
  given to the template elsewhere (`render(r=range)`, or kept and given back later), it stays Python's, so that
  `r is sameas range` is False, where Jinja2's is True (0583). Calling `cycler` or `namespace` in Python makes
  Jinja2's object, a Python object to the template. A macro or `loop.cycle` prints as Jinja2's object does (0439), but
  its `type()` is the module's `TemplateValue`, as the module has no `jinja2.runtime.Macro`.
- Sandbox (0412, 0413, 0414): `intercepted_binops` and `intercepted_unops` raise
  `UnsupportedError` (Jinja2's `test_binary_operator_intercepting` and
  `test_unary_operator_intercepting` are `xfail`). A public name of the engine's own values
  (`s.upper`, `d.items`) is asked of `is_safe_attribute` only when the subclass overrides it,
  the immutable form's refusal of the engine's list and dict methods is the engine's own (it also refuses a set's
  `intersection_update`, which Jinja2 3.1.6's `_mutable_spec` leaves out of its `MutableSet` list, so that the
  immutable form there lets it change the set; the engine fails closed on purpose, 0545), a
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
  builds none for the question; the object is an empty one of its type for the engine's lists and dicts, 0425); that is read again at each render, module or macro call, so a method replaced
  on the class or the instance counts for the templates loaded before it too. The groupby tuple's `grouper` and `list` and a range's `start`, `stop` and `step` are asked as Jinja2 asks them (`_GroupTuple`, `range`), with `None` as the value (0425).
  `str.format`, `format_map` and `Markup.format` go through Jinja2's `SandboxedFormatter`; a field's
  width or precision, and the text the fields write together, are checked against the render's
  sequence cap (64 MiB or `Limits.max_sequence_size`) before Python builds the string, and fail with
  the `TemplateRuntimeError` the engine's own `str.format` raises (Jinja2 itself builds it). Widths and precisions are read as CPython reads them, any Unicode decimal digit; a spec outside the standard grammar is bounded by the largest number in it. The template's `{{ x|attr('__class__') }}` of an undefined `x` is an
  `UndefinedError`. `sort` and `unique` of an unsafe or missing attribute under
  `StrictUndefined` raise as Jinja2's do (0413). `sum(attribute=)` of a missing attribute
  is now an `UndefinedError` as in Jinja2 (it was a `TemplateRuntimeError`).
