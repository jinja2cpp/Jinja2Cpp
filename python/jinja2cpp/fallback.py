"""The fallback to real Jinja2 for what the engine does not do (docs/python.md, "Fallback").

The decision is made in three places and never in the middle of an output: when an environment
is constructed with an option outside the subset (Tier B), when a template is loaded that the
engine cannot parse, and when a render of the buffering ``render()`` failed on an include,
import or extends of such a template before any Python callable ran. The policy is
``fallback=`` of the environment, or the module default (``set_fallback``, the environment
variable ``JINJA2CPP_FALLBACK``): ``"auto"``, ``"warn"`` or ``"never"``.
"""

import contextlib
import os
import threading
import typing as t
import warnings

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
# renders the engine started and a retry with Jinja2 replaced
_native_adjust = 0
_native_base = 0
_lock = threading.Lock()


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
    the calls of ``render``. A render that was retried with Jinja2 counts once, as a fallback.
    """
    global _native_base, _native_adjust
    with _lock:
        result = dict(_counts)
        result["renders_native"] = _jinja2cpp._native_renders() - _native_adjust - _native_base
        if reset:
            for key in _counts:
                _counts[key] = 0
            _native_base = _jinja2cpp._native_renders()
            _native_adjust = 0
            _warned.clear()
    return {key: result[key] for key in sorted(result)}


def _count(key: str) -> None:
    _counts[key] += 1


def _jinja2() -> t.Any:
    """The jinja2 package, or None when it is not installed."""
    try:
        import jinja2
        import jinja2.sandbox  # noqa: F401
    except ImportError:
        return None
    return jinja2


def require(reason: str, mode: t.Optional[str] = None, stacklevel: int = 4) -> t.Any:
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
            warnings.warn(f"jinja2cpp falls back to Jinja2: {reason}", FallbackWarning, stacklevel=stacklevel)
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
        raise new.with_traceback(exc.__traceback__) from None


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


def _jinja2_extension(jinja2: t.Any, extension: t.Any) -> t.Any:
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


def _convert_option(jinja2: t.Any, name: str, value: t.Any) -> t.Any:
    if name == "extensions":
        return [_jinja2_extension(jinja2, e) for e in value]
    if name == "undefined" and isinstance(value, type) and value.__module__ == "jinja2cpp.runtime" and hasattr(jinja2.runtime, value.__name__):
        return getattr(jinja2.runtime, value.__name__)
    if name == "loader":
        return _wrap_loader(jinja2, value)
    return value


def jinja2_class(jinja2: t.Any, env_class: type) -> t.Any:
    """The Jinja2 class a jinja2cpp Environment class stands for (``_jinja2_class_path``)."""
    import importlib

    path: str = getattr(env_class, "_jinja2_class_path", "jinja2.Environment")
    module, _, name = path.rpartition(".")
    return getattr(importlib.import_module(module), name)


def build_environment(env: t.Any, options: t.Optional[t.Dict[str, t.Any]] = None) -> t.Any:
    """A jinja2 Environment with `env`'s configuration: `options` (the constructor's arguments), else read from `env`."""
    jinja2 = _jinja2()
    construction = options is not None
    if options is None:
        options = {name: getattr(env, name) for name in OPTION_NAMES}
    kwargs = {name: _convert_option(jinja2, name, value) for name, value in options.items()}
    jenv = jinja2_class(jinja2, type(env))(**kwargs)
    if construction:
        return jenv  # the environment has no filters, tests or globals of its own yet
    for name in ("filters", "tests", "globals"):
        target = getattr(jenv, name)
        for key, value in getattr(env, name).items():
            if getattr(value, "_native_range", False) and key == "range":
                continue  # the sandbox's own range
            target[key] = value
    jenv.policies.update(env.policies)
    return jenv


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
        value = getattr(env_class, name, None)
        if value is not None and not (value.__module__.split(".")[0] in ("jinja2", "jinja2cpp") and value.__name__ in ("Context", "CodeGenerator")):
            return f"a custom {name}"
    for extension in options.get("extensions", ()):
        if not is_own_extension(extension):
            return f"the extension {extension!r}"
    return None


# ---------------------------------------------------------------------------------------------
# Templates
# ---------------------------------------------------------------------------------------------

_fallback_classes: t.Dict[type, type] = {}


class FallbackTemplate:
    """A template that real Jinja2 renders: the mixin before the template class of its environment.

    It stands for a ``jinja2cpp.Template`` (``isinstance`` holds): ``environment`` is the jinja2cpp
    environment, ``generate`` streams, and what the engine has no equivalent of (``blocks``,
    ``new_context``, ``root_render_func``, ``render_async``) is Jinja2's own.
    """

    def __init__(self, *args: t.Any, **kwargs: t.Any) -> None:
        # Made by fallback_template, which has set everything
        pass

    def render(self, *args: t.Any, **kwargs: t.Any) -> str:
        _count("renders_fallback")
        with mapped():
            return t.cast(str, self._jinja2_template.render(*args, **kwargs))

    def _render_once(self, *args: t.Any, **kwargs: t.Any) -> str:
        return self.render(*args, **kwargs)

    def generate(self, *args: t.Any, **kwargs: t.Any) -> t.Iterator[str]:
        _count("renders_fallback")
        with mapped():
            yield from self._jinja2_template.generate(*args, **kwargs)

    def stream(self, *args: t.Any, **kwargs: t.Any) -> t.Any:
        from .environment import TemplateStream

        return TemplateStream(self.generate(*args, **kwargs))

    def make_module(self, vars: t.Optional[t.Dict[str, t.Any]] = None, shared: bool = False, locals: t.Optional[t.Mapping[str, t.Any]] = None) -> t.Any:
        with mapped():
            return self._jinja2_template.make_module(vars, shared, locals)

    @property
    def module(self) -> t.Any:
        with mapped():
            return self._jinja2_template.module

    @property
    def name(self) -> t.Optional[str]:
        return t.cast(t.Optional[str], self._jinja2_template.name)

    @property
    def filename(self) -> t.Optional[str]:
        return t.cast(t.Optional[str], self._jinja2_template.filename)

    @property
    def globals(self) -> t.Any:
        return self._jinja2_template.globals

    @property
    def environment(self) -> t.Any:
        return self.__dict__["_environment"]

    @property
    def _jinja2_template(self) -> t.Any:
        return self.__dict__["_jt"]

    def __getattr__(self, name: str) -> t.Any:
        if name.startswith("__") or name in ("_jt", "_environment"):
            raise AttributeError(name)
        return getattr(self.__dict__["_jt"], name)

    def __repr__(self) -> str:
        return f"<{type(self).__name__} {self.name!r}>" if self.name is not None else f"<{type(self).__name__} memory:{id(self):x}>"


def fallback_class(template_class: type) -> type:
    """The subclass of `template_class` whose instances are Jinja2's templates."""
    cls = _fallback_classes.get(template_class)
    if cls is None:
        cls = _fallback_classes[template_class] = type(f"Fallback{template_class.__name__}", (FallbackTemplate, template_class), {})
    return cls


def fallback_template(env: t.Any, jinja2_template: t.Any, template_class: t.Optional[type] = None, cls: t.Optional[type] = None) -> t.Any:
    """The jinja2cpp template for a template of Jinja2 that `env` loaded."""
    cls = cls or fallback_class(template_class or env.template_class)
    instance = cls.__new__(cls)
    instance.__dict__["_jt"] = jinja2_template
    instance.__dict__["_fb_jt"] = jinja2_template
    instance.__dict__["_environment"] = env
    return instance


def load_fallback(env: t.Any, error: BaseException, make: t.Callable[[t.Any], t.Any], template_class: t.Optional[type] = None) -> t.Any:
    """`error` is the engine's refusal to parse a template: let Jinja2 parse it (``make(jinja2_env)``).

    Jinja2's error is raised, as the jinja2cpp class, when it fails too; its template is returned
    when it parses. The engine's own error stays when fallback is off or Jinja2 is missing.
    """
    mode = env._fallback_mode_
    if mode == "never" or _jinja2() is None:
        raise error
    try:
        jenv = build_environment(env)
    except Exception:
        raise error from None
    with mapped():
        jt = make(jenv)
    blocker = env._fallback_blocker()
    if blocker:
        raise UnsupportedError(f"the template needs Jinja2 ({error}), which is not used for {blocker}") from error
    require(f"a template the engine cannot parse ({_first_line(error)})", mode, stacklevel=5)
    _count("loads_fallback")
    return fallback_template(env, jt, template_class)


def _first_line(error: BaseException) -> str:
    message = getattr(error, "message", None) or str(error)
    return str(message).splitlines()[0] if message else type(error).__name__


def jinja2_template_of(template: t.Any, reason: str) -> t.Any:
    """Jinja2's template for a template of the engine (made from its source or name), for what only Jinja2 has."""
    jt = template.__dict__.get("_fb_jt")
    if jt is not None:
        return jt
    env = template.environment
    require(reason, env._fallback_mode_, stacklevel=5)
    blocker = env._fallback_blocker()
    if blocker:
        raise UnsupportedError(f"{reason} is not available for {blocker}")
    source = template.__dict__.get("_fb_source")
    name = template.__dict__.get("_fb_name")
    own_globals = dict(template.globals.maps[0])
    with mapped():
        jenv = build_environment(env)
        if source is not None:
            jt = jenv.from_string(source, own_globals)
        elif name is not None:
            jt = jenv.get_template(name, None, own_globals)
        else:
            raise UnsupportedError(f"{reason}: this template was not made by from_string or get_template, so Jinja2 has no source for it")
    _count("loads_fallback")
    template.__dict__["_fb_jt"] = jt
    return jt


def render_retry(template: t.Any, args: t.Tuple[t.Any, ...], kwargs: t.Dict[str, t.Any]) -> t.Optional[str]:
    """The engine failed to parse a template the render included: render it all with Jinja2 (None: the engine's error stands).

    Called by the engine, only when no Python callable has run in the render.
    """
    global _native_adjust
    env = template.environment
    if env._fallback_mode_ == "never" or _jinja2() is None or env._fallback_blocker():
        return None
    if template.__dict__.get("_fb_source") is None and template.__dict__.get("_fb_name") is None:
        return None
    require("an included template the engine cannot parse", env._fallback_mode_, stacklevel=6)
    jt = jinja2_template_of(template, "an included template the engine cannot parse")
    with mapped():
        output = t.cast(str, jt.render(*args, **kwargs))
    with _lock:
        _native_adjust += 1
        _counts["renders_fallback"] += 1
    return output


def remember(template: t.Any, source: t.Any = None, name: t.Any = None) -> t.Any:
    """Keep what Jinja2 needs to load `template` again (``render_retry``, ``blocks``, ...)."""
    try:
        if source is not None:
            template.__dict__["_fb_source"] = source
        if name is not None:
            template.__dict__["_fb_name"] = name
    except (AttributeError, TypeError):  # pragma: no cover - a template class with __slots__
        pass
    return template
