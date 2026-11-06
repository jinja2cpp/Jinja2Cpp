"""Helpers of jinja2.utils that need no engine."""

import enum
import typing as t

F = t.TypeVar("F", bound=t.Callable[..., t.Any])


try:  # Jinja2's own marker, so that a function marked here also works in Jinja2 (the fallback)
    from jinja2.utils import _PassArg
except ImportError:  # pragma: no cover - the tests cover it in a subprocess

    class _PassArg(enum.Enum):  # type: ignore[no-redef]
        """What a callable marked by pass_context and its kin gets as its first argument."""

        context = enum.auto()
        eval_context = enum.auto()
        environment = enum.auto()

        @classmethod
        def from_obj(cls, obj: t.Any) -> t.Optional["_PassArg"]:
            if hasattr(obj, "jinja_pass_arg"):
                return obj.jinja_pass_arg  # type: ignore[no-any-return]
            return None


def pass_context(f: F) -> F:
    """Pass the template Context as the first argument of a filter, test or global."""
    f.jinja_pass_arg = _PassArg.context  # type: ignore[attr-defined]
    return f


def pass_eval_context(f: F) -> F:
    """Pass the EvalContext (autoescape state) as the first argument."""
    f.jinja_pass_arg = _PassArg.eval_context  # type: ignore[attr-defined]
    return f


def pass_environment(f: F) -> F:
    """Pass the Environment as the first argument."""
    f.jinja_pass_arg = _PassArg.environment  # type: ignore[attr-defined]
    return f


def select_autoescape(
    enabled_extensions: t.Collection[str] = ("html", "htm", "xml"),
    disabled_extensions: t.Collection[str] = (),
    default_for_string: bool = True,
    default: bool = False,
) -> t.Callable[[t.Optional[str]], bool]:
    """Autoescape by the template's file extension, as Jinja2's select_autoescape."""
    enabled_patterns = tuple(f".{x.lstrip('.').lower()}" for x in enabled_extensions)
    disabled_patterns = tuple(f".{x.lstrip('.').lower()}" for x in disabled_extensions)

    def autoescape(template_name: t.Optional[str]) -> bool:
        if template_name is None:
            return default_for_string
        template_name = template_name.lower()
        if template_name.endswith(enabled_patterns):
            return True
        if template_name.endswith(disabled_patterns):
            return False
        return default

    return autoescape


def is_undefined(obj: t.Any) -> bool:
    """Whether obj is an Undefined, ours or Jinja2's."""
    from .runtime import Undefined

    if isinstance(obj, Undefined):
        return True
    return any(c.__name__ == "Undefined" and c.__module__.startswith("jinja2") for c in type(obj).__mro__)
