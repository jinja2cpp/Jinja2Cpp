"""The Undefined classes (jinja2.runtime), as Python sees an undefined value of a template.

The engine applies the environment's policy itself; these objects are what a Python filter,
test or other callable gets for an undefined argument. Jinja2's own classes are accepted
wherever these are. Ported from Jinja2 3.1 (BSD-3-Clause, the Pallets Projects).
"""

import typing as t

from .exceptions import TemplateRuntimeError, UndefinedError

missing: t.Any = type("MissingType", (), {"__repr__": lambda x: "missing"})()


def _object_type_repr(obj: t.Any) -> str:
    if obj is None:
        return "None"
    if obj is Ellipsis:
        return "Ellipsis"
    cls = type(obj)
    if cls.__module__ == "builtins":
        return f"{cls.__name__} object"
    return f"{cls.__module__}.{cls.__name__} object"


class Undefined:
    """Prints as empty, is false and an empty sequence; any other use raises UndefinedError."""

    __slots__ = ("_undefined_hint", "_undefined_obj", "_undefined_name", "_undefined_exception")

    def __init__(
        self,
        hint: t.Optional[str] = None,
        obj: t.Any = missing,
        name: t.Optional[str] = None,
        exc: t.Type[TemplateRuntimeError] = UndefinedError,
    ) -> None:
        self._undefined_hint = hint
        self._undefined_obj = obj
        self._undefined_name = name
        self._undefined_exception = exc

    @property
    def _undefined_message(self) -> str:
        if self._undefined_hint:
            return self._undefined_hint
        if self._undefined_obj is missing:
            return f"{self._undefined_name!r} is undefined"
        if not isinstance(self._undefined_name, str):
            return f"{_object_type_repr(self._undefined_obj)} has no element {self._undefined_name!r}"
        return f"{_object_type_repr(self._undefined_obj)!r} has no attribute {self._undefined_name!r}"

    def _fail_with_undefined_error(self, *args: t.Any, **kwargs: t.Any) -> "t.NoReturn":
        raise self._undefined_exception(self._undefined_message)

    def __getattr__(self, name: str) -> t.Any:
        if name[:2] == "__" and name[-2:] == "__":
            raise AttributeError(name)
        return self._fail_with_undefined_error()

    __add__ = __radd__ = __sub__ = __rsub__ = _fail_with_undefined_error
    __mul__ = __rmul__ = __div__ = __rdiv__ = _fail_with_undefined_error
    __truediv__ = __rtruediv__ = _fail_with_undefined_error
    __floordiv__ = __rfloordiv__ = _fail_with_undefined_error
    __mod__ = __rmod__ = _fail_with_undefined_error
    __pos__ = __neg__ = _fail_with_undefined_error
    __call__ = __getitem__ = _fail_with_undefined_error
    __lt__ = __le__ = __gt__ = __ge__ = _fail_with_undefined_error
    __int__ = __float__ = __complex__ = _fail_with_undefined_error
    __pow__ = __rpow__ = _fail_with_undefined_error

    def __eq__(self, other: t.Any) -> bool:
        return type(self) is type(other)

    def __ne__(self, other: t.Any) -> bool:
        return not self.__eq__(other)

    def __hash__(self) -> int:
        return id(type(self))

    def __str__(self) -> str:
        return ""

    def __len__(self) -> int:
        return 0

    def __iter__(self) -> t.Iterator[t.Any]:
        yield from ()

    def __bool__(self) -> bool:
        return False

    def __repr__(self) -> str:
        return "Undefined"


class ChainableUndefined(Undefined):
    """An undefined whose attributes and items are itself."""

    __slots__ = ()

    def __html__(self) -> str:
        return str(self)

    def __getattr__(self, name: str) -> "ChainableUndefined":
        if name[:2] == "__" and name[-2:] == "__":
            raise AttributeError(name)
        return self

    def __getitem__(self, _name: str) -> "ChainableUndefined":  # type: ignore[override]
        return self


class DebugUndefined(Undefined):
    """An undefined that prints as {{ name }}."""

    __slots__ = ()

    def __str__(self) -> str:
        if self._undefined_hint:
            message = f"undefined value printed: {self._undefined_hint}"
        elif self._undefined_obj is missing:
            message = self._undefined_name  # type: ignore
        else:
            message = f"no such element: {_object_type_repr(self._undefined_obj)}[{self._undefined_name!r}]"
        return f"{{{{ {message} }}}}"


class StrictUndefined(Undefined):
    """An undefined that raises on printing, iteration, truth and comparison."""

    __slots__ = ()
    __iter__ = __str__ = __len__ = Undefined._fail_with_undefined_error
    __eq__ = __ne__ = __bool__ = __hash__ = Undefined._fail_with_undefined_error
    __contains__ = Undefined._fail_with_undefined_error


class EvalContext:
    """Per-render evaluation state (jinja2.nodes.EvalContext): the autoescape setting in force."""

    def __init__(self, environment: t.Any, template_name: t.Optional[str] = None) -> None:
        self.environment = environment
        if callable(environment.autoescape):
            self.autoescape = environment.autoescape(template_name)
        else:
            self.autoescape = environment.autoescape
        self.volatile = False

    def save(self) -> t.Dict[str, t.Any]:
        return self.__dict__.copy()

    def revert(self, old: t.Dict[str, t.Any]) -> None:
        self.__dict__.clear()
        self.__dict__.update(old)


def _make_eval_context(environment: t.Any, autoescape: bool) -> EvalContext:
    ctx = EvalContext.__new__(EvalContext)
    ctx.environment = environment
    ctx.autoescape = autoescape
    ctx.volatile = False
    return ctx


class Context:
    """A read-only view of the variables a template sees where a pass_context callable is called.

    It is lent for the call: using it after the call returned raises TemplateRuntimeError.
    Unlike Jinja2's Context, ``vars`` and ``parent`` both hold every name the call site sees (the
    engine does not tell a template's top-level names from its render arguments and globals), and
    it sees the loop variables and macro locals Jinja2 hides from it.
    """

    def __init__(
        self,
        handle: t.Any,
        environment: t.Any,
        name: t.Optional[str],
        eval_ctx: EvalContext,
        template_globals: t.Any = None,
    ) -> None:
        self._handle = handle
        self.environment = environment
        self.name = name
        self.eval_ctx = eval_ctx
        self._template_globals = template_globals

    def resolve_or_missing(self, key: str) -> t.Any:
        if not isinstance(key, str):
            return missing
        found, value = self._handle.lookup(key)
        return value if found else missing

    def resolve(self, key: str) -> t.Union[t.Any, Undefined]:
        rv = self.resolve_or_missing(key)
        if rv is missing:
            return self.environment.undefined(name=key)
        return rv

    def get(self, key: str, default: t.Any = None) -> t.Any:
        rv = self.resolve_or_missing(key)
        return default if rv is missing else rv

    def call(self, obj: t.Callable[..., t.Any], /, *args: t.Any, **kwargs: t.Any) -> t.Any:
        """Calls obj as a template does: a pass_context, pass_eval_context or pass_environment
        callable gets the argument it asks for first."""
        marker = getattr(obj, "jinja_pass_arg", None)
        if marker is None:
            marker = getattr(getattr(type(obj), "__call__", None), "jinja_pass_arg", None)
        kind = getattr(marker, "name", None)
        if kind == "context":
            args = (self, *args)
        elif kind == "eval_context":
            args = (self.eval_ctx, *args)
        elif kind == "environment":
            args = (self.environment, *args)
        return obj(*args, **kwargs)

    def get_all(self) -> t.Dict[str, t.Any]:
        names = self._handle.names()
        names.extend(self.environment.globals)
        if self._template_globals:
            names.extend(self._template_globals)
        names.extend(("cycler", "dict", "joiner", "lipsum", "namespace", "range"))
        result = {}
        for key in names:
            if isinstance(key, str) and key not in result:
                value = self.resolve_or_missing(key)
                if value is not missing:
                    result[key] = value
        return result

    @property
    def parent(self) -> t.Dict[str, t.Any]:
        return self.get_all()

    @property
    def vars(self) -> t.Dict[str, t.Any]:
        return self.get_all()

    @property
    def exported_vars(self) -> t.Set[str]:
        return set()

    def get_exported(self) -> t.Dict[str, t.Any]:
        return {}

    def keys(self) -> t.KeysView[str]:
        return self.get_all().keys()

    def values(self) -> t.ValuesView[t.Any]:
        return self.get_all().values()

    def items(self) -> t.ItemsView[str, t.Any]:
        return self.get_all().items()

    def __contains__(self, name: str) -> bool:
        return self.resolve_or_missing(name) is not missing

    def __getitem__(self, key: str) -> t.Any:
        item = self.resolve_or_missing(key)
        if item is missing:
            raise KeyError(key)
        return item

    def __iter__(self) -> t.Iterator[str]:
        return iter(self.get_all())

    def __len__(self) -> int:
        return len(self.get_all())

    def __repr__(self) -> str:
        return f"<{type(self).__name__} {self.name!r}>"


def _make_context(
    handle: t.Any, environment: t.Any, name: t.Optional[str], eval_ctx: EvalContext, template_globals: t.Any
) -> Context:
    return Context(handle, environment, name, eval_ctx, template_globals)
