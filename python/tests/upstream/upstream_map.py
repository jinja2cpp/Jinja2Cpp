"""Imports of ``jinja2`` in the upstream test suite, served by ``jinja2cpp`` (docs/tasks/0355).

``run.py`` rewrites every ``import jinja2...`` and ``from jinja2... import ...`` statement of
the pallets/jinja tests into calls of this module, so a test reaches jinja2cpp and never the
real Jinja2. The real Jinja2 stays importable as ``jinja2``: jinja2cpp's exceptions subclass
its exceptions and the fallback (``fallback="never"`` here) would use it.

A name that jinja2cpp does not have is a ``Missing`` placeholder rather than an ImportError,
so one missing name fails the tests that use it, not the collection of the whole file; it
fails with ``MissingName``, which ``upstream_plugin.py`` reports apart from wrong results.
"""

import importlib
import types


class MissingName(NotImplementedError):
    """A test used a name of Jinja2 that jinja2cpp does not provide."""


class Missing:
    """Stands for ``jinja2.<module>.<name>`` that jinja2cpp lacks; any use raises MissingName."""

    def __init__(self, qualname):
        object.__setattr__(self, "_qualname", qualname)

    def __str__(self):
        raise MissingName(f"jinja2cpp has no {self._qualname}")

    def _fail(self, *args, **kwargs):
        raise MissingName(f"jinja2cpp has no {self._qualname}")

    # A call at module level (a class body's `Token(1, "name", "x")`) gives another placeholder,
    # so the file collects; using the result fails the test
    def __call__(self, *args, **kwargs):
        return Missing(f"{self._qualname}()")

    __iter__ = _fail
    __enter__ = _fail
    __getitem__ = _fail
    __setattr__ = _fail
    __bool__ = _fail
    __eq__ = _fail
    __ne__ = _fail
    __hash__ = object.__hash__
    __len__ = _fail
    __contains__ = _fail

    def __getattr__(self, name):
        if name.startswith("__") and name.endswith("__"):
            raise AttributeError(name)
        return Missing(f"{self._qualname}.{name}")

    def __mro_entries__(self, bases):
        # `class X(Missing)` at module level: the class is defined (a plain object), so the
        # file still collects, and the tests that use X fail on what X lacks
        return (_MissingBase,)

    def __repr__(self):
        return f"<missing {self._qualname}>"


class _MissingBase:
    def __init__(self, *args, **kwargs):
        raise MissingName(f"jinja2cpp has no base class for {type(self).__name__}")


def _target(name):
    """jinja2cpp's module for Jinja2's module `name`, or None."""
    mapped = "jinja2cpp" + name[len("jinja2"):]
    try:
        return importlib.import_module(mapped)
    except ImportError:
        return None


def module(name):
    """``import jinja2.x``: jinja2cpp's module, or a placeholder module whose names are Missing."""
    target = _target(name)
    if target is not None:
        return target
    placeholder = types.ModuleType(name)
    placeholder.__getattr__ = lambda attr: Missing(f"{name}.{attr}")  # type: ignore[attr-defined]
    return placeholder


def names(name, *attrs):
    """``from jinja2.x import a, b``: a tuple of the names, Missing where jinja2cpp lacks one."""
    target = _target(name)
    values = []
    for attr in attrs:
        value = getattr(target, attr, None) if target is not None else None
        if value is None and target is not None:
            # A submodule (`from jinja2 import loaders`)
            sub = _target(f"{name}.{attr}")
            value = sub
        values.append(Missing(f"{name}.{attr}") if value is None else value)
    return tuple(values)
