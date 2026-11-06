"""A sandbox layer that ensures unsafe operations cannot be performed (jinja2.sandbox).

Useful when the template itself comes from an untrusted source. Ported from Jinja2 3.1.6
(BSD-3-Clause, the Pallets Projects): the policy functions and environment classes are Jinja2's.
What differs is where the checks run (docs/python.md, "Sandbox"): the engine asks the
environment's ``is_safe_attribute`` for every attribute a template reads of a Python object, and
``is_safe_callable`` for every call of one, through the ``_sandbox_*`` methods below, instead of
compiled code that calls ``getattr`` and ``call``. The engine's own values (the str, list and dict
of a template) have no Python attributes to refuse: their ``__class__`` and the like are undefined
there, and the ``ImmutableSandboxedEnvironment`` refuses the list and dict methods that change
their receiver.
"""

import operator
import types
import typing as t
from collections import abc
from collections import deque
from functools import update_wrapper
from string import Formatter

from markupsafe import EscapeFormatter
from markupsafe import Markup

try:
    from _string import formatter_field_name_split  # type: ignore
except ImportError:  # pragma: no cover - CPython has it
    formatter_field_name_split = None

from .environment import Environment
from .exceptions import SecurityError
from .exceptions import UnsupportedError
from .runtime import Undefined

F = t.TypeVar("F", bound=t.Callable[..., t.Any])

#: maximum number of items a range may produce
MAX_RANGE = 100000

#: Unsafe function attributes.
UNSAFE_FUNCTION_ATTRIBUTES: t.Set[str] = set()

#: Unsafe method attributes. Function attributes are unsafe for methods too.
UNSAFE_METHOD_ATTRIBUTES: t.Set[str] = set()

#: unsafe generator attributes.
UNSAFE_GENERATOR_ATTRIBUTES = {"gi_frame", "gi_code"}

#: unsafe attributes on coroutines
UNSAFE_COROUTINE_ATTRIBUTES = {"cr_frame", "cr_code"}

#: unsafe attributes on async generators
UNSAFE_ASYNC_GENERATOR_ATTRIBUTES = {"ag_code", "ag_frame"}

_mutable_spec: t.Tuple[t.Tuple[t.Type[t.Any], t.FrozenSet[str]], ...] = (
    (
        abc.MutableSet,
        frozenset(["add", "clear", "difference_update", "discard", "pop", "remove", "symmetric_difference_update", "update"]),
    ),
    (
        abc.MutableMapping,
        frozenset(["clear", "pop", "popitem", "setdefault", "update"]),
    ),
    (
        abc.MutableSequence,
        frozenset(["append", "clear", "pop", "reverse", "insert", "sort", "extend", "remove"]),
    ),
    (
        deque,
        frozenset(["append", "appendleft", "clear", "extend", "extendleft", "pop", "popleft", "remove", "rotate"]),
    ),
)


def safe_range(*args: int) -> range:
    """A range that can't generate ranges with a length of more than MAX_RANGE items.

    A sandboxed render does not call this function: the engine's own ``range`` stops at MAX_RANGE
    and raises the same OverflowError. It is what ``env.globals["range"]`` holds, and what a
    Python caller gets.
    """
    rng = range(*args)

    if len(rng) > MAX_RANGE:
        raise OverflowError(f"Range too big. The sandbox blocks ranges larger than MAX_RANGE ({MAX_RANGE}).")

    return rng


# Marks the function the engine replaces by its own range (jinja2cpp/src environment.cpp)
safe_range._native_range = True  # type: ignore[attr-defined]


def unsafe(f: F) -> F:
    """Marks a function or method as unsafe.

    .. code-block: python

        @unsafe
        def delete(self):
            pass
    """
    f.unsafe_callable = True  # type: ignore
    return f


def is_internal_attribute(obj: t.Any, attr: str) -> bool:
    """Test if the attribute given is an internal python attribute.  For
    example this function returns `True` for the `func_code` attribute of
    python objects.  This is useful if the environment method
    :meth:`~SandboxedEnvironment.is_safe_attribute` is overridden.

    >>> from jinja2cpp.sandbox import is_internal_attribute
    >>> is_internal_attribute(str, "mro")
    True
    >>> is_internal_attribute(str, "upper")
    False
    """
    if isinstance(obj, types.FunctionType):
        if attr in UNSAFE_FUNCTION_ATTRIBUTES:
            return True
    elif isinstance(obj, types.MethodType):
        if attr in UNSAFE_FUNCTION_ATTRIBUTES or attr in UNSAFE_METHOD_ATTRIBUTES:
            return True
    elif isinstance(obj, type):
        if attr == "mro":
            return True
    elif isinstance(obj, (types.CodeType, types.TracebackType, types.FrameType)):
        return True
    elif isinstance(obj, types.GeneratorType):
        if attr in UNSAFE_GENERATOR_ATTRIBUTES:
            return True
    elif hasattr(types, "CoroutineType") and isinstance(obj, types.CoroutineType):
        if attr in UNSAFE_COROUTINE_ATTRIBUTES:
            return True
    elif hasattr(types, "AsyncGeneratorType") and isinstance(obj, types.AsyncGeneratorType):
        if attr in UNSAFE_ASYNC_GENERATOR_ATTRIBUTES:
            return True
    return attr.startswith("__")


def modifies_known_mutable(obj: t.Any, attr: str) -> bool:
    """This function checks if an attribute on a builtin mutable object
    (list, dict, set or deque) or the corresponding ABCs would modify it
    if called.

    >>> modifies_known_mutable({}, "clear")
    True
    >>> modifies_known_mutable({}, "keys")
    False
    >>> modifies_known_mutable([], "append")
    True
    >>> modifies_known_mutable([], "index")
    False

    If called with an unsupported object, ``False`` is returned.

    >>> modifies_known_mutable("foo", "upper")
    False
    """
    for typespec, unsafe in _mutable_spec:
        if isinstance(obj, typespec):
            return attr in unsafe
    return False


class SandboxedEnvironment(Environment):
    """The sandboxed environment.  It works like the regular environment but
    asks the runtime to check every attribute and call of a Python object.
    Subclasses of this environment may override the methods that tell the
    runtime what attributes or functions are safe to access.

    If the template tries to access insecure code a :exc:`SecurityError` is
    raised.  However also other exceptions may occur during the rendering so
    the caller has to ensure that all exceptions are caught.
    """

    sandboxed = True

    #: whether the engine's lists and dicts refuse the methods that change them
    #: (ImmutableSandboxedEnvironment)
    _sandbox_immutable = False

    #: default callback table for the binary operators.  A copy of this is
    #: available on each instance of a sandboxed environment as
    #: :attr:`binop_table`
    default_binop_table: t.Dict[str, t.Callable[[t.Any, t.Any], t.Any]] = {
        "+": operator.add,
        "-": operator.sub,
        "*": operator.mul,
        "/": operator.truediv,
        "//": operator.floordiv,
        "**": operator.pow,
        "%": operator.mod,
    }

    #: default callback table for the unary operators.  A copy of this is
    #: available on each instance of a sandboxed environment as
    #: :attr:`unop_table`
    default_unop_table: t.Dict[str, t.Callable[[t.Any], t.Any]] = {
        "+": operator.pos,
        "-": operator.neg,
    }

    #: a set of binary operators that should be intercepted. Not supported:
    #: loading a template of an environment with a non-empty set raises
    #: UnsupportedError, as ignoring it would let the operators through
    #: (docs/python.md, "Sandbox")
    intercepted_binops: t.FrozenSet[str] = frozenset()

    #: a set of unary operators that should be intercepted. Not supported, as
    #: :attr:`intercepted_binops`
    intercepted_unops: t.FrozenSet[str] = frozenset()

    def __init__(self, *args: t.Any, **kwargs: t.Any) -> None:
        super().__init__(*args, **kwargs)
        self.globals["range"] = safe_range
        self.binop_table = self.default_binop_table.copy()
        self.unop_table = self.default_unop_table.copy()

    def _check_interception(self) -> None:
        if self.intercepted_binops or self.intercepted_unops:
            raise UnsupportedError(
                "jinja2cpp does not support intercepted_binops and intercepted_unops of the sandbox yet (docs/python.md)"
            )

    def from_string(self, source: t.Any, globals: t.Optional[t.Mapping[str, t.Any]] = None, template_class: t.Any = None) -> t.Any:
        self._check_interception()
        return super().from_string(source, globals, template_class)

    def _load_template(self, name: t.Any, globals: t.Optional[t.Mapping[str, t.Any]]) -> t.Any:
        self._check_interception()
        return super()._load_template(name, globals)

    def is_safe_attribute(self, obj: t.Any, attr: str, value: t.Any) -> bool:
        """The sandboxed environment will call this method to check if the
        attribute of an object is safe to access.  Per default all attributes
        starting with an underscore are considered private as well as the
        special attributes of internal python objects as returned by the
        :func:`is_internal_attribute` function.
        """
        return not (attr.startswith("_") or is_internal_attribute(obj, attr))

    def is_safe_callable(self, obj: t.Any) -> bool:
        """Check if an object is safely callable. By default callables
        are considered safe unless decorated with :func:`unsafe`.

        This also recognizes the Django convention of setting
        ``func.alters_data = True``.
        """
        return not (getattr(obj, "unsafe_callable", False) or getattr(obj, "alters_data", False))

    def call_binop(self, context: t.Any, operator: str, left: t.Any, right: t.Any) -> t.Any:
        """For intercepted binary operator calls (:meth:`intercepted_binops`)
        this function is executed instead of the builtin operator."""
        return self.binop_table[operator](left, right)

    def call_unop(self, context: t.Any, operator: str, arg: t.Any) -> t.Any:
        """For intercepted unary operator calls (:meth:`intercepted_unops`)
        this function is executed instead of the builtin operator."""
        return self.unop_table[operator](arg)

    def getitem(self, obj: t.Any, argument: t.Union[str, t.Any]) -> t.Union[t.Any, Undefined]:
        """Subscribe an object from sandboxed code."""
        try:
            return obj[argument]
        except (TypeError, LookupError):
            if isinstance(argument, str):
                try:
                    attr = str(argument)
                except Exception:
                    pass
                else:
                    try:
                        value = getattr(obj, attr)
                    except AttributeError:
                        pass
                    else:
                        fmt = self.wrap_str_format(value)
                        if fmt is not None:
                            return fmt
                        if self.is_safe_attribute(obj, argument, value):
                            return value
                        return self.unsafe_undefined(obj, argument)
        return self.undefined(obj=obj, name=argument)

    def getattr(self, obj: t.Any, attribute: str) -> t.Union[t.Any, Undefined]:
        """Subscribe an object from sandboxed code and prefer the
        attribute.  The attribute passed *must* be a bytestring.
        """
        try:
            value = getattr(obj, attribute)
        except AttributeError:
            try:
                return obj[attribute]
            except (TypeError, LookupError):
                pass
        else:
            fmt = self.wrap_str_format(value)
            if fmt is not None:
                return fmt
            if self.is_safe_attribute(obj, attribute, value):
                return value
            return self.unsafe_undefined(obj, attribute)
        return self.undefined(obj=obj, name=attribute)

    def _sandbox_attribute(self, obj: t.Any, attribute: str, value: t.Any) -> t.Any:
        """What the engine reads when a template asks the Python object `obj` for the existing
        attribute `attribute` (`value`): the one checkpoint of the sandbox. The body of the
        ``else`` branch of ``getattr``."""
        fmt = self.wrap_str_format(value)
        if fmt is not None:
            return fmt
        if self.is_safe_attribute(obj, attribute, value):
            return value
        return self.unsafe_undefined(obj, attribute)

    def _sandbox_check_call(self, obj: t.Any) -> None:
        """Raises SecurityError for a call of `obj` that is not safe: what ``call`` checks."""
        if not self.is_safe_callable(obj):
            raise SecurityError(f"{obj!r} is not safely callable")

    def unsafe_undefined(self, obj: t.Any, attribute: str) -> Undefined:
        """Return an undefined object for unsafe attributes."""
        return self.undefined(  # type: ignore[no-any-return]
            f"access to attribute {attribute!r} of {type(obj).__name__!r} object is unsafe.",
            name=attribute,
            obj=obj,
            exc=SecurityError,
        )

    def wrap_str_format(self, value: t.Any) -> t.Optional[t.Callable[..., str]]:
        """If the given value is a ``str.format`` or ``str.format_map`` method,
        return a new function than handles sandboxing. This is done at access
        rather than in :meth:`call`, so that calls made without ``call`` are
        also sandboxed.
        """
        if not isinstance(value, (types.MethodType, types.BuiltinMethodType)) or value.__name__ not in ("format", "format_map"):
            return None

        f_self: t.Any = value.__self__

        if not isinstance(f_self, str):
            return None

        str_type: t.Type[str] = type(f_self)
        is_format_map = value.__name__ == "format_map"
        formatter: SandboxedFormatter

        if isinstance(f_self, Markup):
            formatter = SandboxedEscapeFormatter(self, escape=f_self.escape)
        else:
            formatter = SandboxedFormatter(self)

        vformat = formatter.vformat

        def wrapper(*args: t.Any, **kwargs: t.Any) -> str:
            if is_format_map:
                if kwargs:
                    raise TypeError("format_map() takes no keyword arguments")

                if len(args) != 1:
                    raise TypeError(f"format_map() takes exactly one argument ({len(args)} given)")

                kwargs = args[0]
                args = ()

            return str_type(vformat(f_self, args, kwargs))

        return update_wrapper(wrapper, value)

    def call(
        __self,  # noqa: B902
        __context: t.Any,
        __obj: t.Any,
        *args: t.Any,
        **kwargs: t.Any,
    ) -> t.Any:
        """Call an object from sandboxed code."""

        # the double prefixes are to avoid double keyword argument
        # errors when proxying the call.
        if not __self.is_safe_callable(__obj):
            raise SecurityError(f"{__obj!r} is not safely callable")
        return __context.call(__obj, *args, **kwargs)


class ImmutableSandboxedEnvironment(SandboxedEnvironment):
    """Works exactly like the regular `SandboxedEnvironment` but does not
    permit modifications on the builtin mutable objects `list`, `set`, and
    `dict` by using the :func:`modifies_known_mutable` function.

    The lists and dicts a template makes are the engine's own: they refuse the same methods
    whatever ``is_safe_attribute`` does.
    """

    _sandbox_immutable = True

    def is_safe_attribute(self, obj: t.Any, attr: str, value: t.Any) -> bool:
        if not super().is_safe_attribute(obj, attr, value):
            return False

        return not modifies_known_mutable(obj, attr)


class SandboxedFormatter(Formatter):
    def __init__(self, env: Environment, **kwargs: t.Any) -> None:
        self._env = env
        super().__init__(**kwargs)

    def get_field(self, field_name: str, args: t.Sequence[t.Any], kwargs: t.Mapping[str, t.Any]) -> t.Tuple[t.Any, str]:
        first, rest = formatter_field_name_split(field_name)
        obj = self.get_value(first, args, kwargs)
        for is_attr, i in rest:
            if is_attr:
                obj = self._env.getattr(obj, i)
            else:
                obj = self._env.getitem(obj, i)
        return obj, first


class SandboxedEscapeFormatter(SandboxedFormatter, EscapeFormatter):
    pass
