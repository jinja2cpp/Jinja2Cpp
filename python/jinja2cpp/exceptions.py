"""Jinja2's exception classes (jinja2.exceptions), raised by jinja2cpp.

The hierarchy and attributes follow Jinja2 3.1, so code that catches Jinja2's exceptions by
name catches these. A Python exception that a filter, test or other callable raises during a
render is raised again as itself, with its traceback.
"""

import typing as t

try:  # Jinja2's classes, when it is installed: ours are also subclasses of them
    import jinja2.exceptions as _jinja2_exceptions
except ImportError:  # pragma: no cover - the tests cover it in a subprocess
    _jinja2_exceptions = None  # type: ignore[assignment]


def _bases(name: str, *own: type) -> t.Tuple[type, ...]:
    """The bases of the class `name`: Jinja2's class of that name when it is importable, then ours.

    `except jinja2.TemplateNotFound` then catches ours. Jinja2's class comes first because its own
    bases (OSError for TemplateNotFound) must precede the ones we list again.
    """
    if _jinja2_exceptions is None:
        return own
    return (getattr(_jinja2_exceptions, name), *own)


class TemplateError(*_bases("TemplateError", Exception)):
    """Base class for all template errors."""

    def __init__(self, message: t.Optional[str] = None) -> None:
        super().__init__(message)

    @property
    def message(self) -> t.Optional[str]:
        return self.args[0] if self.args else None


class TemplateNotFound(*_bases("TemplateNotFound", IOError, LookupError, TemplateError)):
    """Raised if a template does not exist."""

    message: t.Optional[str] = None

    def __init__(self, name: t.Any, message: t.Optional[str] = None) -> None:
        IOError.__init__(self, name)
        if message is None:
            from .runtime import Undefined

            if isinstance(name, Undefined):
                name._fail_with_undefined_error()
            message = name
        self.message = message
        self.name = name
        self.templates = [name]

    def __str__(self) -> str:
        return str(self.message)


class TemplatesNotFound(*_bases("TemplatesNotFound", TemplateNotFound)):
    """Like TemplateNotFound, raised when several templates are selected."""

    def __init__(self, names: t.Sequence[t.Any] = (), message: t.Optional[str] = None) -> None:
        if message is None:
            from .runtime import Undefined

            parts = []
            for name in names:
                if isinstance(name, Undefined):
                    parts.append(name._undefined_message)
                else:
                    parts.append(name)
            parts_str = ", ".join(map(str, parts))
            message = f"none of the templates given were found: {parts_str}"
        # Not super(): Jinja2's class, when it is a base, takes other arguments
        TemplateNotFound.__init__(self, names[-1] if names else None, message)
        self.templates = list(names)


class TemplateSyntaxError(*_bases("TemplateSyntaxError", TemplateError)):
    """Raised to tell the user that there is a problem with the template."""

    def __init__(
        self,
        message: str,
        lineno: int,
        name: t.Optional[str] = None,
        filename: t.Optional[str] = None,
    ) -> None:
        TemplateError.__init__(self, message)
        self.lineno = lineno
        self.name = name
        self.filename = filename
        self.source: t.Optional[str] = None
        self.translated = False

    def __str__(self) -> str:
        if self.translated:
            return t.cast(str, self.message)
        location = f"line {self.lineno}"
        name = self.filename or self.name
        if name:
            location = f'File "{name}", {location}'
        lines = [t.cast(str, self.message), "  " + location]
        if self.source is not None:
            try:
                line = self.source.splitlines()[self.lineno - 1]
            except IndexError:
                pass
            else:
                lines.append("    " + line.strip())
        return "\n".join(lines)

    def __reduce__(self):  # type: ignore
        return self.__class__, (self.message, self.lineno, self.name, self.filename)


class TemplateAssertionError(*_bases("TemplateAssertionError", TemplateSyntaxError)):
    """A template's check failed at load: an unknown filter or test."""


class TemplateRuntimeError(*_bases("TemplateRuntimeError", TemplateError)):
    """A generic runtime error in the template engine."""


class UndefinedError(*_bases("UndefinedError", TemplateRuntimeError)):
    """Raised if a template tries to operate on Undefined."""


class SecurityError(*_bases("SecurityError", TemplateRuntimeError)):
    """Raised if a template tries to do something insecure in a sandbox."""


class FilterArgumentError(*_bases("FilterArgumentError", TemplateRuntimeError)):
    """A filter was given an argument it cannot take."""


class UnsupportedError(TemplateError, NotImplementedError):
    """The feature is not in the jinja2cpp subset (docs/python.md); the fallback to Jinja2 handles it."""
