"""Environment and Template (jinja2.environment): the Python layer over the native classes.

The engine's classes (``_jinja2cpp.Environment`` and ``Template``) render; what needs no engine
lives here, so that a Python subclass of Environment works as in Jinja2 and the template classes
of an environment are its ``template_class``. Ported from Jinja2 3.1.6 (BSD-3-Clause, the Pallets
Projects) where the code is Jinja2's: the stream, select_template and list_templates.
"""

import threading
import typing as t
import weakref
from collections import OrderedDict
from functools import partial
from itertools import chain

from markupsafe import Markup

from . import _jinja2cpp
from . import fallback
from .exceptions import TemplateNotFound
from .exceptions import TemplatesNotFound
from .exceptions import TemplateSyntaxError
from .exceptions import UndefinedError
from .exceptions import UnsupportedError
from .runtime import Undefined

concat = "".join


class TemplateStream:
    """A stream of the rendered template: like a generator that can buffer several items.

    The output is not buffered by default. With a buffer size of 5, five items are combined into
    one string, which is useful to stream a big template to a client that flushes at each item.
    """

    def __init__(self, gen: t.Iterator[str]) -> None:
        self._gen = gen
        self.disable_buffering()

    def dump(self, fp: t.Union[str, t.IO[bytes]], encoding: t.Optional[str] = None, errors: t.Optional[str] = "strict") -> None:
        """Dump the complete stream into a file or file-like object.

        Strings are written, unless an `encoding` is given. ``Template('Hello {{ name }}!').stream(name='foo').dump('hello.html')``
        """
        close = False
        if isinstance(fp, str):
            if encoding is None:
                encoding = "utf-8"
            real_fp: t.IO[bytes] = open(fp, "wb")
            close = True
        else:
            real_fp = fp
        try:
            if encoding is not None:
                iterable = (x.encode(encoding, errors) for x in self)  # type: ignore
            else:
                iterable = self  # type: ignore
            if hasattr(real_fp, "writelines"):
                real_fp.writelines(iterable)
            else:
                for item in iterable:
                    real_fp.write(item)
        finally:
            if close:
                real_fp.close()

    def disable_buffering(self) -> None:
        """Disable the output buffering."""
        self._next = partial(next, self._gen)
        self.buffered = False

    def _buffered_generator(self, size: int) -> t.Iterator[str]:
        buf: t.List[str] = []
        c_size = 0
        push = buf.append
        while True:
            try:
                while c_size < size:
                    c = next(self._gen)
                    push(c)
                    if c:
                        c_size += 1
            except StopIteration:
                if not c_size:
                    return
            yield concat(buf)
            del buf[:]
            c_size = 0

    def enable_buffering(self, size: int = 5) -> None:
        """Enable buffering.  Buffer `size` items before yielding them."""
        if size <= 1:
            raise ValueError("buffer size too small")
        self.buffered = True
        self._next = partial(next, self._buffered_generator(size))

    def __iter__(self) -> "TemplateStream":
        return self

    def __next__(self) -> str:
        return self._next()  # type: ignore


class TemplateModule:
    """What a template exports, as Jinja2's ``Template.module`` and ``make_module``.

    The exported variables and macros are attributes. ``str()`` is the output of the template's
    top level. A macro stays callable for as long as it is referenced, whatever happens to the
    module, the template and the environment.
    """

    def __init__(self, template: "Template", body: str, exports: t.Dict[str, t.Any]) -> None:
        self._body = body
        self.__dict__.update(exports)
        self.__name__ = template.name

    def __html__(self) -> Markup:
        return Markup(self._body)

    def __str__(self) -> str:
        return self._body

    def __repr__(self) -> str:
        if self.__name__ is None:
            name = f"memory:{id(self):x}"
        else:
            name = repr(self.__name__)
        return f"<{type(self).__name__} {name}>"


class ParsedTemplate:
    """The result of ``Environment.parse``: opaque, for the functions of jinja2cpp.meta."""

    __slots__ = ("_template",)

    def __init__(self, template: "Template") -> None:
        self._template = template

    def __repr__(self) -> str:
        return f"<ParsedTemplate {self._template.name!r}>"


async def _mapped_await(awaitable: t.Any) -> t.Any:
    """Await Jinja2's coroutine with its template errors raised as jinja2cpp's (``fallback.mapped``)."""
    with fallback.mapped():
        return await awaitable


async def _mapped_aiter(iterator: t.Any) -> t.AsyncIterator[str]:
    """Iterate Jinja2's async generator with its template errors raised as jinja2cpp's."""
    try:
        with fallback.mapped():
            async for item in iterator:
                yield item
    finally:
        await iterator.aclose()


class Template(_jinja2cpp.Template):
    """A compiled template: ``render``, ``generate``, ``stream`` and ``make_module``."""

    # The class Template(source, ...) makes its environment of (set below)
    environment_class: t.Type["Environment"]

    def __new__(cls, *args: t.Any, **kwargs: t.Any) -> "Template":
        if not args and "source" in kwargs:
            args = (kwargs.pop("source"),)
        if len(args) == 1 and cls.__init__ is Template.__init__:
            # Template(source, **options) makes the environment and loads through it, so that an
            # option or a source outside the subset reaches Jinja2 (docs/python.md, "Fallback")
            template = cls.environment_class(**kwargs).from_string(args[0], None, cls)
            template.__dict__["_made"] = True
            return template
        return super().__new__(cls)  # type: ignore[no-any-return]

    def __init__(self, source: t.Any, **kwargs: t.Any) -> None:
        if self.__dict__.pop("_made", False):
            return  # made by __new__
        env = self.environment_class(**kwargs)
        if type(self).__init__ is not Template.__init__ and env._uses_jinja2():
            # The engine would take the source of a subclass with an __init__ of its own: Jinja2's environment cannot
            raise UnsupportedError(
                f"{type(self).__qualname__} has an __init__ of its own, so a Template(...) call cannot use Jinja2 for these options "
                "(the fallback); make the template with env.from_string() or drop the __init__ (docs/python.md, \"Fallback\")"
            )
        try:
            super().__init__(source, env)
        except TemplateSyntaxError as error:
            if type(self).__init__ is not Template.__init__ and env._fallback_mode_ != "never" and fallback._jinja2() is not None:
                # Jinja2's own error when it rejects the source too
                fallback.parse_fallback(env, error, lambda j: j.parse(source))
                raise UnsupportedError(
                    f"{type(self).__qualname__} has an __init__ of its own, so a source the engine cannot parse cannot fall back to Jinja2 "
                    f"({fallback._first_line(error)}); make the template with env.from_string() or drop the __init__ (docs/python.md, \"Fallback\")"
                ) from error
            raise

    def generate(self, *args: t.Any, **kwargs: t.Any) -> t.Iterator[str]:
        """Render the template as a generator of strings: Jinja2's items (a run of text, the value of a
        ``{{ }}``, a macro's output). The template runs as far as the items asked for, on the caller's
        thread, as in Jinja2; an error comes after the items before it. Where that is not available (see
        docs/python.md) the whole render runs at the first item."""
        lazy = self._lazy(*args, **kwargs)
        if lazy is not None:
            try:
                while True:
                    item = lazy._next()
                    if item is None:
                        return
                    yield item
            finally:
                lazy.close()
        items: t.List[str] = []
        error: t.Optional[BaseException] = None
        try:
            self._render_to(items.append, *args, **kwargs)
        except BaseException as raised:  # noqa: BLE001 - raised again after the items before it
            error = raised
        yield from items
        if error is not None:
            raise error

    def stream(self, *args: t.Any, **kwargs: t.Any) -> TemplateStream:
        """Like ``generate``, as a TemplateStream."""
        return TemplateStream(self.generate(*args, **kwargs))

    def make_module(self, vars: t.Optional[t.Dict[str, t.Any]] = None, shared: bool = False, locals: t.Optional[t.Mapping[str, t.Any]] = None) -> TemplateModule:
        """Render the template with `vars` and return what it exports (docs/python.md for ``shared``)."""
        params: t.Dict[str, t.Any] = {}
        if not shared:
            params.update(self.globals)
        if vars:
            params.update(vars)
        if locals:
            params.update(locals)
        body, exports = self._make_module(params)
        return TemplateModule(self, body, exports)

    @property
    def module(self) -> TemplateModule:
        """The module of the template with no variables, made once."""
        module = self.__dict__.get("_module")
        if module is None:
            module = self.__dict__["_module"] = self.make_module()
        return module

    # What only Jinja2 has: a Jinja2 template is made of the source (docs/python.md, "Fallback")
    @property
    def blocks(self) -> t.Any:
        """The block render functions: Jinja2's, for the template loaded again by Jinja2."""
        return fallback.jinja2_template_of(self, "Template.blocks").blocks

    @property
    def root_render_func(self) -> t.Any:
        """Jinja2's root render function."""
        return fallback.jinja2_template_of(self, "Template.root_render_func").root_render_func

    def new_context(self, vars: t.Optional[t.Dict[str, t.Any]] = None, shared: bool = False, locals: t.Optional[t.Mapping[str, t.Any]] = None) -> t.Any:
        """Jinja2's ``new_context``."""
        return fallback.jinja2_template_of(self, "Template.new_context").new_context(vars, shared, locals)

    def render_async(self, *args: t.Any, **kwargs: t.Any) -> t.Any:
        """Jinja2's ``render_async`` (it needs ``enable_async=True``)."""
        return _mapped_await(fallback.jinja2_template_of(self, "Template.render_async").render_async(*args, **kwargs))

    def generate_async(self, *args: t.Any, **kwargs: t.Any) -> t.Any:
        """Jinja2's ``generate_async`` (it needs ``enable_async=True``)."""
        return _mapped_aiter(fallback.jinja2_template_of(self, "Template.generate_async").generate_async(*args, **kwargs))

    def __repr__(self) -> str:
        if self.name is None:
            name = f"memory:{id(self):x}"
        else:
            name = repr(self.name)
        return f"<{type(self).__name__} {name}>"


class FallbackTemplate:
    """The methods of a template that real Jinja2 renders (docs/python.md, "Fallback"): a plain mixin that
    ``fallback.fallback_class`` puts before the environment's ``template_class`` (nanobind allows one nanobind
    base, so it cannot be a subclass of ``Template``; a ``template_class`` that overrides one of these methods is
    refused with ``UnsupportedError``).

    The instance stands for a ``jinja2cpp.Template`` (``isinstance`` holds): ``environment`` is the jinja2cpp
    environment, ``generate`` streams, and what the engine has no equivalent of (``blocks``, ``new_context``,
    ``root_render_func``, ``render_async``) is Jinja2's own.
    """

    def render(self, *args: t.Any, **kwargs: t.Any) -> str:
        fallback._count("renders_fallback")
        with fallback.mapped():
            return t.cast(str, self._jinja2_template.render(*args, **kwargs))

    def generate(self, *args: t.Any, **kwargs: t.Any) -> t.Iterator[str]:
        fallback._count("renders_fallback")
        with fallback.mapped():
            yield from self._jinja2_template.generate(*args, **kwargs)

    def make_module(self, vars: t.Optional[t.Dict[str, t.Any]] = None, shared: bool = False, locals: t.Optional[t.Mapping[str, t.Any]] = None) -> t.Any:
        with fallback.mapped():
            return self._jinja2_template.make_module(vars, shared, locals)

    @property
    def module(self) -> t.Any:
        with fallback.mapped():
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

    def _jinja2_ast(self) -> t.Any:
        """Jinja2's node tree of the template (``jinja2cpp.meta``): parsed again from its source."""
        jt = self._jinja2_template
        env = jt.environment
        source = self.__dict__.get("_fb_source")
        with fallback.mapped():
            if source is None and jt.name is not None and env.loader is not None:
                source = env.loader.get_source(env, jt.name)[0]
            if source is None:
                raise UnsupportedError("jinja2cpp.meta needs the source of the template, which Jinja2 does not keep for it")
            return env.parse(source, jt.name, jt.filename)

    def _undeclared(self) -> t.Set[str]:
        import jinja2.meta

        return t.cast(t.Set[str], jinja2.meta.find_undeclared_variables(self._jinja2_ast()))

    def _referenced(self) -> t.List[t.Optional[str]]:
        import jinja2.meta

        return list(jinja2.meta.find_referenced_templates(self._jinja2_ast()))

    def _make_module(self, params: t.Dict[str, t.Any]) -> t.Tuple[str, t.Dict[str, t.Any]]:
        with fallback.mapped():
            module = self._jinja2_template.make_module(params, True)
        return str(module), {k: v for k, v in vars(module).items() if not k.startswith("_")}

    def __getattr__(self, name: str) -> t.Any:
        if name.startswith("__") or name in ("_jt", "_environment"):
            raise AttributeError(name)
        return getattr(self.__dict__["_jt"], name)


# Environment's parameters in Jinja2's order, for the positional form
_ENVIRONMENT_PARAMETERS = (
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


class _Delegate:
    """The Jinja2 environment an Environment with a Tier B option stands for, and the options as the caller gave them."""

    __slots__ = ("env", "orig")

    def __init__(self, env: t.Any, orig: t.Dict[str, t.Any]) -> None:
        self.env = env
        self.orig = orig


# The properties of the engine's environment; a delegating environment forwards them to Jinja2's
_FORWARDED = tuple(fallback.OPTION_NAMES) + ("globals", "filters", "tests", "newstyle_gettext")
# What a delegating environment answers as the caller gave it, not as Jinja2 holds it
_ORIGINAL = ("extensions", "undefined", "finalize", "loader")


def _forwarded(name: str) -> property:
    native = getattr(_jinja2cpp.Environment, name)

    def fget(self: "Environment") -> t.Any:
        delegate = self.__dict__.get("_delegate")
        if delegate is None:
            return native.__get__(self, type(self))
        if name in delegate.orig and name in _ORIGINAL:
            return delegate.orig[name]
        return getattr(delegate.env, name)

    def fset(self: "Environment", value: t.Any) -> None:
        delegate = self.__dict__.get("_delegate")
        if delegate is None:
            native.__set__(self, value)
            return
        if name in _ORIGINAL:
            delegate.orig[name] = value
        if name != "extensions":
            setattr(delegate.env, name, fallback._convert_option(fallback._jinja2(), name, value))

    return property(fget, fset, doc=f"Environment.{name}")


def _drop_engine_cache(env_ref: "t.Optional[weakref.ref[t.Any]]") -> None:
    env = env_ref() if env_ref is not None else None
    if env is not None:
        env._drop_engine_cache()


class _LRUCache:
    """Jinja2's ``Environment.cache`` of a ``cache_size`` above 0 (jinja2.utils.LRUCache): the templates
    ``get_template`` gave by ``(weakref.ref(loader), name)``, the least recently used one dropped past the size.
    Clearing it or deleting an entry also drops the engine's cache, so that the next load reads the loader again."""

    def __init__(self, capacity: int, env_ref: "t.Optional[weakref.ref[t.Any]]" = None) -> None:
        self.capacity = capacity
        self._mapping: "OrderedDict[t.Any, t.Any]" = OrderedDict()
        self._lock = threading.Lock()
        self._env_ref = env_ref

    def copy(self) -> "_LRUCache":
        rv = type(self)(self.capacity, self._env_ref)
        with self._lock:
            rv._mapping.update(self._mapping)
        return rv

    def get(self, key: t.Any, default: t.Any = None) -> t.Any:
        try:
            return self[key]
        except KeyError:
            return default

    def setdefault(self, key: t.Any, default: t.Any = None) -> t.Any:
        try:
            return self[key]
        except KeyError:
            self[key] = default
            return default

    def __getitem__(self, key: t.Any) -> t.Any:
        with self._lock:
            value = self._mapping[key]
            self._mapping.move_to_end(key)
            return value

    def __setitem__(self, key: t.Any, value: t.Any) -> None:
        with self._lock:
            self._mapping[key] = value
            self._mapping.move_to_end(key)
            while len(self._mapping) > self.capacity:
                self._mapping.popitem(last=False)

    def __delitem__(self, key: t.Any) -> None:
        with self._lock:
            del self._mapping[key]
        _drop_engine_cache(self._env_ref)

    def clear(self) -> None:
        with self._lock:
            self._mapping.clear()
        _drop_engine_cache(self._env_ref)

    def __contains__(self, key: t.Any) -> bool:
        return key in self._mapping

    def __len__(self) -> int:
        return len(self._mapping)

    def items(self) -> t.List[t.Tuple[t.Any, t.Any]]:
        """The items, the most recently used first."""
        with self._lock:
            return list(reversed(self._mapping.items()))

    def values(self) -> t.List[t.Any]:
        return [value for _, value in self.items()]

    def keys(self) -> t.List[t.Any]:
        return [key for key, _ in self.items()]

    def __iter__(self) -> t.Iterator[t.Any]:
        return iter(self.keys())

    def __reversed__(self) -> t.Iterator[t.Any]:
        with self._lock:
            return iter(list(self._mapping))

    def __repr__(self) -> str:
        return f"<{type(self).__name__} {self._mapping!r}>"


class _UnboundedCache(dict):  # type: ignore[type-arg]
    """Jinja2's dict cache of a negative ``cache_size``; clearing it or deleting an entry drops the engine's too."""

    def __init__(self, env_ref: "t.Optional[weakref.ref[t.Any]]" = None) -> None:
        super().__init__()
        self._env_ref = env_ref

    def __delitem__(self, key: t.Any) -> None:
        super().__delitem__(key)
        _drop_engine_cache(self._env_ref)

    def pop(self, *args: t.Any) -> t.Any:
        value = super().pop(*args)
        _drop_engine_cache(self._env_ref)
        return value

    def popitem(self) -> t.Any:
        item = super().popitem()
        _drop_engine_cache(self._env_ref)
        return item

    def clear(self) -> None:
        super().clear()
        _drop_engine_cache(self._env_ref)


def _create_cache(size: int, env: t.Any = None) -> t.Any:
    """Jinja2's create_cache: none for 0, a dict for a negative size, an LRU cache otherwise."""
    env_ref = weakref.ref(env) if env is not None else None
    if size == 0:
        return None
    if size < 0:
        return _UnboundedCache(env_ref)
    return _LRUCache(size, env_ref)


def _default_policies() -> t.Dict[str, t.Any]:
    policies = dict(fallback.DEFAULT_POLICIES)
    policies["json.dumps_kwargs"] = dict(policies["json.dumps_kwargs"])
    return policies


class Environment(_jinja2cpp.Environment):
    """The configuration and the loaded templates; subclass it as Jinja2's.

    ``fallback`` (``"auto"``, ``"warn"`` or ``"never"``; the default of the module otherwise, see
    ``jinja2cpp.set_fallback``) says what happens with what the engine cannot do and real Jinja2
    can: docs/python.md, "Fallback".
    """

    #: the Jinja2 class that stands for this one when Jinja2 has to take over
    _jinja2_class_path = "jinja2.Environment"

    def __init__(self, *args: t.Any, **kwargs: t.Any) -> None:
        if len(args) > len(_ENVIRONMENT_PARAMETERS):
            raise TypeError(f"Environment() takes at most {len(_ENVIRONMENT_PARAMETERS)} positional arguments but {len(args)} were given")
        for name, value in zip(_ENVIRONMENT_PARAMETERS, args):
            if name in kwargs:
                raise TypeError(f"Environment() got multiple values for argument '{name}'")
            kwargs[name] = value
        mode = kwargs.pop("fallback", None)
        if mode is not None:
            fallback._check_mode(mode)
        self.__dict__["_fallback"] = mode
        self.__dict__["_policies"] = _default_policies()
        if kwargs.get("extensions") is not None:  # read twice (the choice of engine, then the constructor): a one-shot iterator would be spent
            kwargs["extensions"] = list(kwargs["extensions"])
        reason = fallback.options_reason(type(self), kwargs)
        if reason is None:
            super().__init__(**kwargs)
            # The templates get_template gave, given again while the engine's cache has them unchanged (0433)
            self.__dict__["_cache"] = _create_cache(self.cache_size, self)
            fallback._count("environments_native")
            return
        # Tier B: real Jinja2 is the environment
        fallback.require(reason, mode)
        jenv = fallback.build_environment(self, kwargs)
        self.__dict__["_delegate"] = _Delegate(jenv, {k: v for k, v in kwargs.items() if k in _ORIGINAL})
        super().__init__()
        fallback._count("environments_fallback")

    #: whether the environment is a sandbox (jinja2cpp.sandbox), which checks every attribute and call
    #: of a Python object a template reaches
    sandboxed = False

    #: the class of the templates ``from_string`` and ``get_template`` make
    template_class: t.Type[Template] = Template

    @property
    def _fallback_mode_(self) -> str:
        return fallback.resolve_mode(self.__dict__.get("_fallback"))

    @property
    def cache(self) -> t.Any:
        """Jinja2's cache of the templates ``get_template`` gave (0433): ``None`` for ``cache_size=0``."""
        delegate = self.__dict__.get("_delegate")
        if delegate is not None:
            return delegate.env.cache
        return self.__dict__.get("_cache")

    @cache.setter
    def cache(self, value: t.Any) -> None:
        delegate = self.__dict__.get("_delegate")
        if delegate is not None:
            delegate.env.cache = value
            return
        self.__dict__["_cache"] = value
        # Another cache than the environment's own (None, a mapping of the caller's) is the only one: loads go past the
        # engine's cache, so that its misses read the loader as Jinja2's do (0433)
        own = isinstance(value, (_LRUCache, _UnboundedCache)) and value._env_ref is not None and value._env_ref() is self
        self.__dict__["_foreign_cache"] = not own
        self.__dict__.pop("_foreign_uptodate", None)
        self._drop_engine_cache()  # a replaced cache is an empty one: the next loads read the loader again

    @property
    def policies(self) -> t.Dict[str, t.Any]:
        """Jinja2's policies. The engine implements none of them: a changed or added entry sends templates to Jinja2."""
        delegate = self.__dict__.get("_delegate")
        if delegate is not None:
            return t.cast(t.Dict[str, t.Any], delegate.env.policies)
        return t.cast(t.Dict[str, t.Any], self.__dict__["_policies"])

    @policies.setter
    def policies(self, value: t.Dict[str, t.Any]) -> None:
        delegate = self.__dict__.get("_delegate")
        if delegate is not None:
            delegate.env.policies = value
        else:
            self.__dict__["_policies"] = value

    def __getattr__(self, name: str) -> t.Any:
        # Called only for a name the environment lacks: a delegating one has Jinja2's
        delegate = self.__dict__.get("_delegate") if not name.startswith("__") else None
        if delegate is None:
            raise AttributeError(f"{type(self).__name__!r} object has no attribute {name!r}")
        return getattr(delegate.env, name)

    def _from_jinja2(self, make: t.Callable[[t.Any], t.Any], template_class: t.Optional[type] = None, source: t.Any = None) -> t.Any:
        """The template ``make(jinja2_environment)`` loads, as a jinja2cpp template."""
        delegate = self.__dict__.get("_delegate")
        if delegate is not None:
            env = delegate.env
        else:
            reason = fallback.policies_reason(self.policies)
            assert reason is not None
            fallback.require(reason, self._fallback_mode_)
            env = fallback.delegate_for(self)
        # No lock across the load: a loader may call back into any environment. Racing first loads both compile
        # (as in Jinja2); the wrappers of the template Jinja2 kept are the ones later calls get
        with fallback.mapped():
            jt = make(env)
        return fallback.fallback_template(self, jt, template_class, source)

    def _uses_jinja2(self) -> bool:
        return "_delegate" in self.__dict__ or self.policies != fallback.DEFAULT_POLICIES

    def from_string(self, source: t.Any, globals: t.Optional[t.Mapping[str, t.Any]] = None, template_class: t.Optional[t.Type[Template]] = None) -> Template:
        """Load a template from a source string without using ``loader``."""
        if self._uses_jinja2() and not isinstance(source, _jinja2cpp.Template):
            return self._from_jinja2(lambda j: j.from_string(source, globals), template_class, source)  # type: ignore[no-any-return]
        try:
            template = self._from_string(source, globals, template_class or self.template_class)
        except TemplateSyntaxError as error:
            if isinstance(source, _jinja2cpp.Template):
                raise
            return fallback.load_fallback(self, error, lambda j: j.from_string(source, globals), template_class, source)  # type: ignore[no-any-return]
        fallback._count("loads_native")
        if isinstance(source, str):
            fallback.remember(template, source=source)
        return template  # type: ignore[no-any-return]

    def getitem(self, obj: t.Any, argument: t.Union[str, t.Any]) -> t.Union[t.Any, Undefined]:
        """Get an item or attribute of an object but prefer the item."""
        try:
            return obj[argument]
        except (AttributeError, TypeError, LookupError):
            if isinstance(argument, str):
                try:
                    attr = str(argument)
                except Exception:
                    pass
                else:
                    try:
                        return getattr(obj, attr)
                    except AttributeError:
                        pass
            return self.undefined(obj=obj, name=argument)  # type: ignore[attr-defined, no-any-return]

    def getattr(self, obj: t.Any, attribute: str) -> t.Any:
        """Get an item or attribute of an object but prefer the attribute."""
        try:
            return getattr(obj, attribute)
        except AttributeError:
            pass
        try:
            return obj[attribute]
        except (TypeError, LookupError, AttributeError):
            return self.undefined(obj=obj, name=attribute)  # type: ignore[attr-defined, no-any-return]

    def join_path(self, template: str, parent: str) -> str:
        """Join a template with the parent. By default all the lookups are relative to the loader root, so this is a noop; override it to change that."""
        return template

    def _load_template(self, name: t.Any, globals: t.Optional[t.Mapping[str, t.Any]]) -> Template:
        if self._uses_jinja2() and isinstance(name, str):
            return self._from_jinja2(lambda j: j.get_template(name, None, globals))  # type: ignore[no-any-return]
        # Jinja2's Environment._load_template: the template an earlier call gave while it is cached and up to date
        # (the engine's cache decides: the same compiled template), its globals updated with the call's
        cache = self.__dict__.get("_cache")
        key = None
        cached = None
        if cache is not None and isinstance(name, str) and self.loader is not None:
            key = (weakref.ref(self.loader), name)
            cached = cache.get(key)
        if self.__dict__.get("_foreign_cache") and isinstance(name, str) and self.loader is not None:
            return self._load_past_the_engine_cache(name, globals, cache, key, cached)
        try:
            template = self._load(name, globals, self.template_class, cached)
        except TemplateSyntaxError as error:
            if not isinstance(name, str):
                raise
            return fallback.load_fallback(self, error, lambda j: j.get_template(name, None, globals))  # type: ignore[no-any-return]
        if template is cached and cached is not None:
            if globals:
                template.globals.update(globals)
            return template  # type: ignore[no-any-return]
        if key is not None:
            cache[key] = template
        fallback._count("loads_native")
        if isinstance(name, str):
            fallback.remember(template, name=name)
        return template  # type: ignore[no-any-return]

    def _load_past_the_engine_cache(self, name: str, globals: t.Optional[t.Mapping[str, t.Any]], cache: t.Any, key: t.Any, cached: t.Any) -> Template:
        """A load with a cache the caller set (``env.cache = None`` or a mapping of theirs): that cache alone decides,
        as in Jinja2, so a miss reads the loader past the engine's cache, and a hit is checked with the loader's
        ``uptodate`` under ``auto_reload``."""
        uptodate = self.__dict__.setdefault("_foreign_uptodate", {})
        if cached is not None:
            check = uptodate.get(key, (None, None))
            if check[0] is not None and check[0]() is cached and (not self.auto_reload or check[1] is None or check[1]()):
                if globals:
                    cached.globals.update(globals)
                return cached  # type: ignore[no-any-return]
        source, filename, is_up_to_date = self.loader.get_source(self, name)
        try:
            template = self._compile(source, name, filename, globals, self.template_class)
        except TemplateSyntaxError as error:
            return fallback.load_fallback(self, error, lambda j: j.get_template(name, None, globals))  # type: ignore[no-any-return]
        if cache is not None:
            cache[key] = template
            # The template weakly: the entry goes with it, once the caller's cache drops it (Codex on #859)
            def forget(ref: t.Any, key: t.Any = key, uptodate: t.Dict[t.Any, t.Any] = uptodate) -> None:
                if uptodate.get(key, (None,))[0] is ref:
                    del uptodate[key]

            uptodate[key] = (weakref.ref(template, forget), is_up_to_date)
        fallback._count("loads_native")
        fallback.remember(template, name=name)
        return template  # type: ignore[no-any-return]

    def get_template(self, name: t.Any, parent: t.Optional[str] = None, globals: t.Optional[t.Mapping[str, t.Any]] = None) -> Template:
        """Load a template by name with ``loader`` and return it."""
        if isinstance(name, _jinja2cpp.Template):
            return name
        if parent is not None:
            name = self.join_path(name, parent)
        return self._load_template(name, globals)

    def select_template(
        self, names: t.Iterable[t.Union[str, Template]], parent: t.Optional[str] = None, globals: t.Optional[t.Mapping[str, t.Any]] = None
    ) -> Template:
        """The first of the templates that exists; TemplatesNotFound if none does."""
        if isinstance(names, Undefined):
            names._fail_with_undefined_error()
        if not names:
            raise TemplatesNotFound(message="Tried to select from an empty list of templates.")
        for name in names:
            if isinstance(name, _jinja2cpp.Template):
                return name  # type: ignore[return-value]
            if parent is not None:
                name = self.join_path(name, parent)
            try:
                return self._load_template(name, globals)
            except (TemplateNotFound, UndefinedError):
                pass
        raise TemplatesNotFound(names)  # type: ignore[arg-type]

    def get_or_select_template(
        self, template_name_or_list: t.Union[str, Template, t.List[t.Union[str, Template]]], parent: t.Optional[str] = None, globals: t.Optional[t.Mapping[str, t.Any]] = None
    ) -> Template:
        """``get_template`` for a name or a template, ``select_template`` for a list."""
        if isinstance(template_name_or_list, (str, Undefined)):
            return self.get_template(template_name_or_list, parent, globals)
        elif isinstance(template_name_or_list, _jinja2cpp.Template):
            return template_name_or_list  # type: ignore[return-value]
        return self.select_template(template_name_or_list, parent, globals)

    def list_templates(self, extensions: t.Optional[t.Collection[str]] = None, filter_func: t.Optional[t.Callable[[str], bool]] = None) -> t.List[str]:
        """The names of the templates the loader can give, sorted, filtered by the extensions or the function."""
        assert self.loader is not None, "No loader configured."
        names = self.loader.list_templates()
        if extensions is not None:
            if filter_func is not None:
                raise TypeError("either extensions or filter_func can be passed, but not both")

            def filter_func(x: str) -> bool:
                return "." in x and x.rsplit(".", 1)[1] in extensions  # type: ignore[operator]

        if filter_func is not None:
            names = [name for name in names if filter_func(name)]
        return names  # type: ignore[no-any-return]

    def parse(self, source: str, name: t.Optional[str] = None, filename: t.Optional[str] = None) -> ParsedTemplate:
        """Parse a source for ``jinja2cpp.meta``. Unlike Jinja2's, it also checks what the compile checks (an unknown filter or test raises).

        A delegating environment (docs/python.md, "Fallback") gives Jinja2's node tree, which
        ``jinja2cpp.meta`` takes as well.
        """
        delegate = self.__dict__.get("_delegate")
        if delegate is not None:
            with fallback.mapped():
                return delegate.env.parse(source, name, filename)  # type: ignore[no-any-return]
        try:
            return ParsedTemplate(self._compile(source, name, filename, None, self.template_class))
        except TemplateSyntaxError as error:
            # a source the engine cannot parse: Jinja2's node tree, which jinja2cpp.meta takes as well
            return fallback.parse_fallback(self, error, lambda j: j.parse(source, name, filename))  # type: ignore[no-any-return]

    # What the engine has no equivalent of: Jinja2's own, called on an environment of the same
    # configuration (these read or produce code and run no template)
    def _call_jinja2(self, method: str, *args: t.Any, **kwargs: t.Any) -> t.Any:
        delegate = self.__dict__.get("_delegate")
        if delegate is not None:
            env = delegate.env
        else:
            fallback.require(f"Environment.{method}", self._fallback_mode_)
            env = fallback.delegate_for(self)
        with fallback.mapped():
            return getattr(env, method)(*args, **kwargs)

    def compile_expression(self, source: str, undefined_to_none: bool = True) -> t.Any:
        """A callable that evaluates the expression: Jinja2's ``TemplateExpression``."""
        return self._call_jinja2("compile_expression", source, undefined_to_none)

    def lex(self, source: str, name: t.Optional[str] = None, filename: t.Optional[str] = None) -> t.Any:
        """Jinja2's token stream of the source."""
        return self._call_jinja2("lex", source, name, filename)

    def preprocess(self, source: str, name: t.Optional[str] = None, filename: t.Optional[str] = None) -> str:
        """The source after the extensions' preprocessing (Jinja2's)."""
        return t.cast(str, self._call_jinja2("preprocess", source, name, filename))

    def compile(self, source: t.Any, name: t.Optional[str] = None, filename: t.Optional[str] = None, raw: bool = False, defer_init: bool = False) -> t.Any:
        """Jinja2's compiled code of the source."""
        return self._call_jinja2("compile", source, name, filename, raw, defer_init)

    def install_gettext_translations(self, translations: t.Any, newstyle: t.Optional[bool] = None) -> None:
        delegate = self.__dict__.get("_delegate")
        if delegate is not None:
            return delegate.env.install_gettext_translations(translations, newstyle)  # type: ignore[no-any-return]
        return _jinja2cpp.Environment.install_gettext_translations(self, translations, newstyle)  # type: ignore[no-any-return]

    def install_null_translations(self, newstyle: t.Optional[bool] = None) -> None:
        delegate = self.__dict__.get("_delegate")
        if delegate is not None:
            return delegate.env.install_null_translations(newstyle)  # type: ignore[no-any-return]
        return _jinja2cpp.Environment.install_null_translations(self, newstyle)  # type: ignore[no-any-return]

    def install_gettext_callables(self, gettext: t.Any, ngettext: t.Any, newstyle: t.Optional[bool] = None, pgettext: t.Any = None, npgettext: t.Any = None) -> None:
        delegate = self.__dict__.get("_delegate")
        if delegate is not None:
            return delegate.env.install_gettext_callables(gettext, ngettext, newstyle, pgettext, npgettext)  # type: ignore[no-any-return]
        return _jinja2cpp.Environment.install_gettext_callables(self, gettext, ngettext, newstyle, pgettext, npgettext)  # type: ignore[no-any-return]

    def uninstall_gettext_translations(self, translations: t.Any) -> None:
        delegate = self.__dict__.get("_delegate")
        if delegate is not None:
            return delegate.env.uninstall_gettext_translations(translations)  # type: ignore[no-any-return]
        return _jinja2cpp.Environment.uninstall_gettext_translations(self, translations)  # type: ignore[no-any-return]


for _name in _FORWARDED:
    setattr(Environment, _name, _forwarded(_name))
del _name

Template.environment_class = Environment
