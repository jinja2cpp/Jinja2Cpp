"""The fallback to real Jinja2 for what the engine does not do (docs/python.md, "Fallback").

The decision is made in two places and never in the middle of an output: when an environment
is constructed with an option outside the subset (Tier B), and when a template is loaded that the
engine cannot parse. A template the engine renders that includes one it cannot parse raises the
engine's error (task 0430 would route those names at load time). The policy is
``fallback=`` of the environment, or the module default (``set_fallback``, the environment
variable ``JINJA2CPP_FALLBACK``): ``"auto"``, ``"warn"`` or ``"never"``.
"""

import collections
import contextlib
import os
import sys
import threading
import typing as t
import warnings
import weakref

from . import _jinja2cpp
from .exceptions import TemplateError
from .exceptions import UnsupportedError

MODES = ("auto", "warn", "never")


class FallbackWarning(UserWarning):
    """The ``warn`` mode: jinja2cpp handed something to real Jinja2 (once for each reason)."""


# ---------------------------------------------------------------------------------------------
# Policy and statistics
# ---------------------------------------------------------------------------------------------

_default_mode: t.Optional[str] = None
_warned: t.Set[str] = set()
_counts: t.Dict[str, int] = {
    "environments_native": 0,
    "environments_fallback": 0,
    "loads_native": 0,
    "loads_fallback": 0,
    "renders_fallback": 0,
}
_native_base = 0
_lock = threading.Lock()
# Guards the checks and inserts of the caches below, never user code (loaders, extensions, finalize): a class,
# a Jinja2 environment or a wrapper is built outside it, and the first to insert wins
_cache_lock = threading.Lock()


def _check_mode(mode: t.Any) -> str:
    if mode not in MODES:
        raise ValueError(f"fallback must be one of {', '.join(map(repr, MODES))}, not {mode!r}")
    return t.cast(str, mode)


def get_fallback() -> str:
    """The module default of ``fallback=``: JINJA2CPP_FALLBACK when set, else ``"auto"``."""
    global _default_mode
    if _default_mode is None:
        value = os.environ.get("JINJA2CPP_FALLBACK", "auto").strip().lower() or "auto"
        _default_mode = _check_mode(value)
    return _default_mode


def set_fallback(mode: str) -> str:
    """Set the module default of ``fallback=`` (for environments that do not pass it) and return the previous one.

    A test suite calls ``set_fallback("never")`` so that every use of Jinja2 raises UnsupportedError.
    """
    global _default_mode
    previous = get_fallback()
    _default_mode = _check_mode(mode)
    return previous


def resolve_mode(mode: t.Optional[str]) -> str:
    return get_fallback() if mode is None else _check_mode(mode)


def stats(reset: bool = False) -> t.Dict[str, int]:
    """How often the engine and Jinja2 were used since the start (or the last reset).

    ``environments_*`` count constructed environments, ``loads_*`` the templates made by
    ``from_string``, ``get_template``, ``select_template`` and ``Template(...)``, and ``renders_*``
    the calls of ``render``.
    """
    global _native_base
    with _lock:
        result = dict(_counts)
        result["renders_native"] = _jinja2cpp._native_renders() - _native_base
        if reset:
            for key in _counts:
                _counts[key] = 0
            _native_base = _jinja2cpp._native_renders()
            _warned.clear()
    return {key: result[key] for key in sorted(result)}


def _count(key: str) -> None:
    with _lock:
        _counts[key] += 1


def _jinja2() -> t.Any:
    """The jinja2 package, or None when it is not installed."""
    try:
        import jinja2
        import jinja2.sandbox  # noqa: F401
    except ImportError:
        return None
    return jinja2


def _caller_stacklevel() -> int:
    """The stacklevel of warnings.warn that points at the first frame outside this package (the caller's)."""
    level = 2  # the caller of the function that calls this one is frame 2 of its warn
    frame: t.Any = sys._getframe(2)
    while frame is not None:
        name = frame.f_globals.get("__name__", "")
        if name != "jinja2cpp" and not name.startswith("jinja2cpp."):
            break
        level += 1
        frame = frame.f_back
    return level


def require(reason: str, mode: t.Optional[str] = None) -> t.Any:
    """Allow the use of Jinja2 for `reason` or raise UnsupportedError; returns the jinja2 package."""
    mode = resolve_mode(mode)
    if mode == "never":
        raise UnsupportedError(f"{reason} is outside the jinja2cpp subset and fallback='never' (docs/python.md)")
    jinja2 = _jinja2()
    if jinja2 is None:
        raise UnsupportedError(f"{reason} is outside the jinja2cpp subset, and Jinja2 is not installed for the fallback (docs/python.md)")
    if mode == "warn":
        with _lock:
            fresh = reason not in _warned
            _warned.add(reason)
        if fresh:
            warnings.warn(f"jinja2cpp falls back to Jinja2: {reason}", FallbackWarning, stacklevel=_caller_stacklevel())
    return jinja2


# ---------------------------------------------------------------------------------------------
# Exceptions
# ---------------------------------------------------------------------------------------------


def map_exception(exc: BaseException) -> BaseException:
    """The jinja2cpp exception of a Jinja2 one (same message and attributes), so that ``except jinja2cpp.X`` catches it."""
    from . import exceptions as ours

    jinja2 = _jinja2()
    if jinja2 is None or not isinstance(exc, jinja2.exceptions.TemplateError) or isinstance(exc, ours.TemplateError):
        return exc
    for cls in type(exc).__mro__:
        name = cls.__name__
        if cls.__module__ != "jinja2.exceptions" or not hasattr(ours, name):
            continue
        target = getattr(ours, name)
        if name in ("TemplateSyntaxError", "TemplateAssertionError"):
            mapped = target(exc.message, exc.lineno, exc.name, exc.filename)
            mapped.source = exc.source
            # translated stays False: str() shows the location, as for an error the engine raises
        elif name == "TemplateNotFound":
            mapped = target(exc.name, exc.message)
        elif name == "TemplatesNotFound":
            mapped = target(exc.templates, exc.message)
        else:
            mapped = target(exc.message)
        mapped.__cause__ = exc
        return t.cast(BaseException, mapped)
    return exc


@contextlib.contextmanager
def mapped() -> t.Iterator[None]:
    """Raise Jinja2's template errors as jinja2cpp's."""
    try:
        yield
    except TemplateError:
        raise
    except Exception as exc:
        new = map_exception(exc)
        if new is exc:
            raise
        raise new.with_traceback(exc.__traceback__)  # __cause__ is the Jinja2 exception


# ---------------------------------------------------------------------------------------------
# Building a Jinja2 environment from a jinja2cpp one
# ---------------------------------------------------------------------------------------------

#: Environment's keyword arguments (not enable_async, which is a Tier B reason itself)
OPTION_NAMES = (
    "block_start_string",
    "block_end_string",
    "variable_start_string",
    "variable_end_string",
    "comment_start_string",
    "comment_end_string",
    "line_statement_prefix",
    "line_comment_prefix",
    "trim_blocks",
    "lstrip_blocks",
    "newline_sequence",
    "keep_trailing_newline",
    "extensions",
    "optimized",
    "undefined",
    "finalize",
    "autoescape",
    "loader",
    "cache_size",
    "auto_reload",
    "bytecode_cache",
    "enable_async",
)

_OURS_EXTENSIONS = ("do", "loopcontrols", "i18n")
_EXTENSION_CLASSES = {"ExprStmtExtension": "do", "LoopControlExtension": "loopcontrols", "InternationalizationExtension": "i18n"}
_loader_class: t.Any = None


def is_own_extension(extension: t.Any) -> bool:
    """Whether the engine implements `extension`: one of jinja2.ext.do, loopcontrols and i18n, by name or class."""
    if isinstance(extension, str):
        module, _, name = extension.rpartition(".")
        return module in ("jinja2.ext", "jinja2cpp.ext") and name in _OURS_EXTENSIONS
    if isinstance(extension, type):
        return extension.__module__ in ("jinja2.ext", "jinja2cpp.ext") and extension.__name__ in _EXTENSION_CLASSES
    return False


def _jinja2_extension(extension: t.Any) -> t.Any:
    if not is_own_extension(extension):
        return extension
    name = extension.rpartition(".")[2] if isinstance(extension, str) else _EXTENSION_CLASSES[extension.__name__]
    return f"jinja2.ext.{name}"


def _wrap_loader(jinja2: t.Any, loader: t.Any) -> t.Any:
    """A jinja2 loader that asks `loader.get_source`: jinja2cpp's loaders compile with the engine, Jinja2's with its own."""
    global _loader_class
    if loader is None or isinstance(loader, jinja2.loaders.BaseLoader):
        return loader
    if _loader_class is None:
        from .loaders import BaseLoader as OurBase

        class WrappedLoader(jinja2.loaders.BaseLoader):  # type: ignore[name-defined, misc]
            def __init__(self, inner: t.Any) -> None:
                self.inner = inner
                self.has_source_access = getattr(inner, "has_source_access", True)

            def get_source(self, environment: t.Any, template: str) -> t.Any:
                return self.inner.get_source(environment, template)

            def list_templates(self) -> t.List[str]:
                return t.cast(t.List[str], self.inner.list_templates())

            def load(self, environment: t.Any, name: str, globals: t.Optional[t.MutableMapping[str, t.Any]] = None) -> t.Any:
                if type(self.inner).load is not OurBase.load:  # a loader of its own: it calls environment.compile
                    return self.inner.load(environment, name, globals)
                return super().load(environment, name, globals)

        _loader_class = WrappedLoader
    return _loader_class(loader)


_UNDEFINED_NAMES = ("Undefined", "ChainableUndefined", "DebugUndefined", "StrictUndefined")


def _convert_undefined(jinja2: t.Any, value: t.Any) -> t.Any:
    """Jinja2's Undefined class for the `undefined` option: the four classes of jinja2cpp are mapped, a class of Jinja2's is
    given as it is, any other is refused (Jinja2 asserts a ``jinja2.Undefined`` subclass, and a subclass of jinja2cpp's
    cannot be re-based on Jinja2's without breaking its ``super()`` calls)."""
    if isinstance(value, type) and issubclass(value, jinja2.Undefined):
        return value
    if isinstance(value, type) and value.__module__ == "jinja2cpp.runtime" and value.__name__ in _UNDEFINED_NAMES:
        return getattr(jinja2.runtime, value.__name__)
    name = getattr(value, "__qualname__", repr(value))
    raise UnsupportedError(f"the undefined class {name} is not a jinja2.Undefined subclass, so Jinja2 cannot use it (docs/python.md, \"Fallback\"); subclass jinja2.Undefined to use it with the fallback")


def _convert_option(jinja2: t.Any, name: str, value: t.Any) -> t.Any:
    if name == "extensions":
        return [_jinja2_extension(e) for e in value]
    if name == "undefined":
        return _convert_undefined(jinja2, value)
    if name == "loader":
        return _wrap_loader(jinja2, value)
    return value


def _custom_class(env_class: type, name: str) -> t.Any:
    """``context_class`` or ``code_generator_class`` of a jinja2cpp Environment class when it is not Jinja2's own, else None."""
    value = getattr(env_class, name, None)
    if value is None or (value.__module__.split(".")[0] in ("jinja2", "jinja2cpp") and value.__name__ in ("Context", "CodeGenerator")):
        return None
    return value


def _owner_of(jenv: t.Any) -> t.Any:
    """The jinja2cpp environment a delegate Jinja2 environment stands for (a strong reference: a ``compile_expression``
    result or a template can outlive every other reference to the jinja2cpp environment, and keeps its policy)."""
    owner = jenv.__dict__.get("_jinja2cpp_owner")
    if owner is None:
        from .exceptions import SecurityError

        raise SecurityError("the sandbox policy of this environment is not available")  # fail closed
    return owner


def _owner_override(owner: t.Any, name: str) -> t.Any:
    """The owner's method `name` when it is not the sandbox's own, else None: the stock one then runs, Jinja2's.

    Stock means the original function (``sandbox.is_stock_sandbox_hook`` and its snapshots) bound to this very environment: an override
    of the class, a patch of it, a function set on the instance, or a method of another environment is an override.
    """
    from .sandbox import _STOCK_OTHER_HOOK_FUNCTIONS
    from .sandbox import is_stock_sandbox_hook

    method = getattr(owner, name)
    if name in _STOCK_OTHER_HOOK_FUNCTIONS:
        stock = getattr(method, "__self__", None) is owner and getattr(method, "__func__", None) is _STOCK_OTHER_HOOK_FUNCTIONS[name]
    else:
        stock = is_stock_sandbox_hook(owner, name)
    if stock:
        return None
    return method


def _to_jinja2_undefined(jenv: t.Any, value: t.Any) -> t.Any:
    """`value` as a value of Jinja2's runtime: an undefined of jinja2cpp (made by ``super().getattr`` of an override,
    ``self.undefined``) becomes the delegate's undefined class, with its hint, object, name and exception."""
    from .runtime import Undefined as OurUndefined
    from .runtime import missing as our_missing

    if not isinstance(value, OurUndefined):
        return value
    obj = value._undefined_obj
    return jenv.undefined(
        hint=value._undefined_hint,
        obj=jenv_missing() if obj is our_missing else obj,
        name=value._undefined_name,
        exc=value._undefined_exception,
    )


def jenv_missing() -> t.Any:
    import jinja2.utils

    return jinja2.utils.missing


def _forwarding_sandbox(base: type) -> type:
    """`base` (a Jinja2 sandbox class) whose policy hooks ask the jinja2cpp environment that owns it, each time they run.

    All of them: ``is_safe_attribute``, ``is_safe_callable``, ``unsafe_undefined``, ``call``, ``getattr``, ``getitem``,
    ``wrap_str_format``, ``call_binop``, ``call_unop`` and the compile-time ``intercepted_binops`` and
    ``intercepted_unops``. So a hook overridden on the class, patched onto it or set on the instance, before or after a
    template was made, is what Jinja2 runs, as in Jinja2 (docs/python.md, "Fallback"). A hook the owner has not
    overridden is Jinja2's own (the owner's would make jinja2cpp undefined values); what an override returns as a
    jinja2cpp undefined value becomes the delegate's undefined class. ``call_binop`` and ``call_unop`` always go to the
    owner: the stock ones read its ``binop_table`` and ``unop_table``.
    """

    class ForwardingSandbox(base):  # type: ignore[valid-type, misc]
        @property  # type: ignore[misc]
        def intercepted_binops(self) -> t.Any:  # read by the compiler of Jinja2, at compile time
            return _owner_of(self).intercepted_binops

        @property  # type: ignore[misc]
        def intercepted_unops(self) -> t.Any:
            return _owner_of(self).intercepted_unops

        def is_safe_attribute(self, obj: t.Any, attr: str, value: t.Any) -> bool:
            return t.cast(bool, _owner_of(self).is_safe_attribute(obj, attr, value))

        def is_safe_callable(self, obj: t.Any) -> bool:
            return t.cast(bool, _owner_of(self).is_safe_callable(obj))

        def unsafe_undefined(self, obj: t.Any, attribute: str) -> t.Any:
            method = _owner_override(_owner_of(self), "unsafe_undefined")
            return _to_jinja2_undefined(self, method(obj, attribute)) if method is not None else super().unsafe_undefined(obj, attribute)

        def wrap_str_format(self, value: t.Any) -> t.Any:
            method = _owner_override(_owner_of(self), "wrap_str_format")
            return method(value) if method is not None else super().wrap_str_format(value)

        def getattr(self, obj: t.Any, attribute: str) -> t.Any:
            method = _owner_override(_owner_of(self), "getattr")
            return _to_jinja2_undefined(self, method(obj, attribute)) if method is not None else super().getattr(obj, attribute)

        def getitem(self, obj: t.Any, argument: t.Any) -> t.Any:
            method = _owner_override(_owner_of(self), "getitem")
            return _to_jinja2_undefined(self, method(obj, argument)) if method is not None else super().getitem(obj, argument)

        def call_binop(self, context: t.Any, operator: str, left: t.Any, right: t.Any) -> t.Any:
            return _owner_of(self).call_binop(context, operator, left, right)

        def call_unop(self, context: t.Any, operator: str, arg: t.Any) -> t.Any:
            return _owner_of(self).call_unop(context, operator, arg)

        def call(__self, __context: t.Any, __obj: t.Any, *args: t.Any, **kwargs: t.Any) -> t.Any:  # noqa: B902, N805
            method = _owner_override(_owner_of(__self), "call")
            if method is not None:
                return _to_jinja2_undefined(__self, method(__context, __obj, *args, **kwargs))
            return super().call(__context, __obj, *args, **kwargs)

    ForwardingSandbox.__name__ = base.__name__
    ForwardingSandbox.__qualname__ = base.__qualname__
    return ForwardingSandbox


_jinja2_classes: "weakref.WeakKeyDictionary[type, type]" = weakref.WeakKeyDictionary()


def jinja2_class(jinja2: t.Any, env_class: type) -> t.Any:
    """The Jinja2 class a jinja2cpp Environment class stands for (``_jinja2_class_path``), with its ``context_class`` and
    ``code_generator_class`` and, for a sandbox, hooks that forward to the jinja2cpp environment."""
    import importlib

    cached = _jinja2_classes.get(env_class)
    if cached is not None:
        return cached
    path: str = getattr(env_class, "_jinja2_class_path", "jinja2.Environment")
    module, _, name = path.rpartition(".")
    cls = getattr(importlib.import_module(module), name)
    if issubclass(cls, jinja2.sandbox.SandboxedEnvironment):
        cls = _forwarding_sandbox(cls)
    extra = {n: v for n in ("context_class", "code_generator_class") if (v := _custom_class(env_class, n)) is not None}
    if extra:
        cls = type(cls.__name__, (cls,), extra)
    with _cache_lock:
        return t.cast(type, _jinja2_classes.setdefault(env_class, cls))


def build_environment(env: t.Any, options: t.Optional[t.Dict[str, t.Any]] = None) -> t.Any:
    """A jinja2 Environment with `env`'s configuration: `options` (the constructor's arguments), else read from `env`.

    Its filters, tests, globals and policies are chains over `env`'s live dicts, so a change to them is seen by
    the templates made before it.
    """
    jinja2 = _jinja2()
    construction = options is not None
    if options is None:
        options = {name: getattr(env, name) for name in OPTION_NAMES}
    kwargs = {name: _convert_option(jinja2, name, value) for name, value in options.items()}
    jenv = jinja2_class(jinja2, type(env))(**kwargs)
    jenv.__dict__["_jinja2cpp_owner"] = env
    jenv.__dict__["_jinja2cpp_load_lock"] = threading.RLock()
    if construction:
        return jenv  # the environment has no filters, tests or globals of its own yet
    for name in ("filters", "tests", "globals"):
        setattr(jenv, name, collections.ChainMap(getattr(env, name), getattr(jenv, name)))
    jenv.policies = env.policies
    return jenv


def delegate_for(env: t.Any) -> t.Any:
    """The Jinja2 environment of a native `env`, made once and again when its options (or the dicts it holds) change.

    Jinja2 caches the templates it loads in it, so ``get_template`` is as cached as Jinja2's.
    """
    key = [getattr(env, name) for name in OPTION_NAMES]
    key += [type(env), env.filters, env.tests, env.globals, env.policies]
    def current() -> t.Any:
        cached = env.__dict__.get("_fb_delegate")
        if cached is not None and all(a is b or (not isinstance(a, dict) and a == b) for a, b in zip(cached[0], key)):
            return cached[1]
        return None

    with _cache_lock:
        found = current()
    if found is not None:
        return found
    jenv = build_environment(env)  # user code (extensions' __init__) runs here, outside the lock
    with _cache_lock:
        found = current()
        if found is None:
            env.__dict__["_fb_delegate"] = (key, jenv)
            found = jenv
    return found


# ---------------------------------------------------------------------------------------------
# What is outside the subset
# ---------------------------------------------------------------------------------------------

#: Jinja2's default policies (jinja2.defaults.DEFAULT_POLICIES): the engine implements none of them,
#: so a changed or added entry is a reason to use Jinja2
DEFAULT_POLICIES: t.Dict[str, t.Any] = {
    "compiler.ascii_str": True,
    "urlize.rel": "noopener",
    "urlize.target": None,
    "urlize.extra_schemes": None,
    "truncate.leeway": 5,
    "json.dumps_function": None,
    "json.dumps_kwargs": {"sort_keys": True},
    "ext.i18n.trimmed": False,
}


def policies_reason(policies: t.Mapping[str, t.Any]) -> t.Optional[str]:
    if policies != DEFAULT_POLICIES:
        extra = sorted(k for k in policies if k not in DEFAULT_POLICIES)
        changed = sorted(k for k in policies if k in DEFAULT_POLICIES and policies[k] != DEFAULT_POLICIES[k])
        return "env.policies with " + ", ".join(repr(k) for k in extra + changed)
    return None


def options_reason(env_class: type, options: t.Mapping[str, t.Any]) -> t.Optional[str]:
    """Why an environment with these constructor arguments needs Jinja2 (Tier B), or None."""
    reason: t.Optional[str] = getattr(env_class, "_tier_b_reason", None)
    if reason:
        return reason
    if options.get("enable_async"):
        return "enable_async=True"
    for name in ("context_class", "code_generator_class"):
        if _custom_class(env_class, name) is not None:
            return f"a custom {name}"
    for extension in options.get("extensions", ()):
        if not is_own_extension(extension):
            return f"the extension {extension!r}"
    return None


# ---------------------------------------------------------------------------------------------
# Templates
# ---------------------------------------------------------------------------------------------

def fallback_class(template_class: type) -> type:
    """The subclass of `template_class` whose instances are Jinja2's templates.

    ``environment.FallbackTemplate`` is put before `template_class`, so its methods win over the engine's. That is
    also over a user's override, which is refused: ``UnsupportedError`` names the methods (nanobind allows
    one nanobind base, so the mixin cannot sit between the user's class and the engine's ``Template``).
    """
    cls = template_class.__dict__.get("_jinja2cpp_fallback_class")  # on the class, so that it goes with it
    if cls is not None:
        return t.cast(type, cls)
    from . import environment

    stock = (environment.Template, _jinja2cpp.Template, object)
    overridden = sorted(
        name
        for name in environment.FallbackTemplate.__dict__
        if name not in ("__dict__", "__weakref__", "__module__", "__doc__", "__qualname__")
        and (owner := next((c for c in template_class.__mro__ if name in c.__dict__), None)) is not None
        and owner not in stock
    )
    if overridden:
        raise UnsupportedError(
            f"the template_class {template_class.__qualname__} overrides {', '.join(overridden)}, which a template made by Jinja2 "
            "(the fallback) implements itself: the override would be ignored (docs/python.md, \"Fallback\")"
        )
    # named as the user's class, so that repr() and tracebacks show it
    cls = type(template_class.__name__, (environment.FallbackTemplate, template_class), {"__module__": template_class.__module__})
    cls.__qualname__ = template_class.__qualname__
    with _cache_lock:
        cls = template_class.__dict__.get("_jinja2cpp_fallback_class") or cls
        template_class._jinja2cpp_fallback_class = cls  # type: ignore[attr-defined]
    return t.cast(type, cls)


def fallback_template(env: t.Any, jinja2_template: t.Any, template_class: t.Optional[type] = None, source: t.Any = None) -> t.Any:
    """The jinja2cpp template for a template of Jinja2 that `env` loaded: the same object while Jinja2 keeps the template
    (its ``get_template`` cache), counted as a load once."""
    template_class = template_class or env.template_class
    if not (isinstance(template_class, type) and issubclass(template_class, _jinja2cpp.Template)):
        raise TypeError(f"template_class must be a subclass of jinja2cpp.Template, not {template_class!r}")
    cls = fallback_class(template_class)
    with _cache_lock:
        wrappers = jinja2_template.__dict__.setdefault("_jinja2cpp_wrappers", {})
        found = wrappers.get(cls)
        if found is not None:
            return found
        instance = cls.__new__(cls)
        instance.__dict__["_jt"] = jinja2_template
        instance.__dict__["_fb_jt"] = jinja2_template
        instance.__dict__["_environment"] = env
        if isinstance(source, str):
            instance.__dict__["_fb_source"] = source
        wrappers[cls] = instance
    _count("loads_fallback")
    return instance


def _via_jinja2(env: t.Any, error: BaseException, make: t.Callable[[t.Any], t.Any], what: str) -> t.Any:
    """`error` is the engine's refusal to parse: `make(jinja2_env)` is the answer of Jinja2, whose own error is raised
    (as the jinja2cpp class) when it fails too. The engine's error stays when fallback is off or Jinja2 is missing."""
    mode = env._fallback_mode_
    if mode == "never" or _jinja2() is None:
        raise error
    try:
        jenv = delegate_for(env)
    except UnsupportedError as unsupported:
        raise unsupported from error
    except Exception as exc:
        raise error from exc
    with mapped(), jenv.__dict__["_jinja2cpp_load_lock"]:
        result = make(jenv)
    require(f"a {what} the engine cannot parse ({_first_line(error)})", mode)
    return result


def load_fallback(env: t.Any, error: BaseException, make: t.Callable[[t.Any], t.Any], template_class: t.Optional[type] = None, source: t.Any = None) -> t.Any:
    """`error` is the engine's refusal to parse a template: let Jinja2 parse it (``make(jinja2_env)``)."""
    jt = _via_jinja2(env, error, make, "template")
    return fallback_template(env, jt, template_class, source)


def parse_fallback(env: t.Any, error: BaseException, make: t.Callable[[t.Any], t.Any]) -> t.Any:
    """Jinja2's node tree of a source the engine cannot parse (``Environment.parse``)."""
    return _via_jinja2(env, error, make, "source")


def _first_line(error: BaseException) -> str:
    message = getattr(error, "message", None) or str(error)
    return str(message).splitlines()[0] if message else type(error).__name__


def jinja2_template_of(template: t.Any, reason: str) -> t.Any:
    """Jinja2's template for a template of the engine (made from its source or name), for what only Jinja2 has."""
    jt = template.__dict__.get("_fb_jt")
    if jt is not None:
        return jt
    env = template.environment
    require(reason, env._fallback_mode_)
    source = template.__dict__.get("_fb_source")
    name = template.__dict__.get("_fb_name")
    own_globals = dict(template.globals.maps[0])
    with mapped():
        jenv = delegate_for(env)
        if source is not None:
            jt = jenv.from_string(source, own_globals)
        elif name is not None:
            jt = jenv.get_template(name, None, own_globals)
        else:
            raise UnsupportedError(f"{reason}: this template was not made by from_string or get_template, so Jinja2 has no source for it")
    _count("loads_fallback")
    template.__dict__["_fb_jt"] = jt
    return jt


def remember(template: t.Any, source: t.Any = None, name: t.Any = None) -> t.Any:
    """Keep what Jinja2 needs to load `template` again (``blocks``, ``new_context``, ...)."""
    try:
        if source is not None:
            template.__dict__["_fb_source"] = source
        if name is not None:
            template.__dict__["_fb_name"] = name
    except (AttributeError, TypeError):  # pragma: no cover - a template class with __slots__
        pass
    return template
