"""Jinja2's exception classes (jinja2.exceptions), raised by jinja2cpp.

The hierarchy and attributes follow Jinja2 3.1, so code that catches Jinja2's exceptions by
name catches these. A Python exception that a filter, test or other callable raises during a
render is raised again as itself, with its traceback.
"""

import typing as t


class TemplateError(Exception):
    """Base class for all template errors."""

    def __init__(self, message: t.Optional[str] = None) -> None:
        super().__init__(message)

    @property
    def message(self) -> t.Optional[str]:
        return self.args[0] if self.args else None


class TemplateNotFound(IOError, LookupError, TemplateError):
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


class TemplatesNotFound(TemplateNotFound):
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
        super().__init__(names[-1] if names else None, message)
        self.templates = list(names)


class TemplateSyntaxError(TemplateError):
    """Raised to tell the user that there is a problem with the template."""

    def __init__(
        self,
        message: str,
        lineno: int,
        name: t.Optional[str] = None,
        filename: t.Optional[str] = None,
    ) -> None:
        super().__init__(message)
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


class TemplateAssertionError(TemplateSyntaxError):
    """A template's check failed at load: an unknown filter or test."""


class TemplateRuntimeError(TemplateError):
    """A generic runtime error in the template engine."""


class UndefinedError(TemplateRuntimeError):
    """Raised if a template tries to operate on Undefined."""


class SecurityError(TemplateRuntimeError):
    """Raised if a template tries to do something insecure in a sandbox (not yet: P2)."""


class FilterArgumentError(TemplateRuntimeError):
    """A filter was given an argument it cannot take."""
