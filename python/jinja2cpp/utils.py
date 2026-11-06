"""Helpers of jinja2.utils that need no engine."""

import typing as t


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
