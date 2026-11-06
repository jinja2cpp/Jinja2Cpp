"""Shared helpers: each case renders with jinja2cpp and with Python Jinja2 (the oracle, 3.1.6)."""

import copy

import jinja2
import jinja2cpp
import pytest


def _env_kwargs(module, options):
    return options(module) if callable(options) else dict(options or {})


def render_both(source, context=None, options=None):
    """Renders `source` with both engines and returns (jinja2cpp's, Jinja2's) output or exception type.

    `options` is a dict of Environment arguments, or a function of the module (jinja2 or
    jinja2cpp) that returns one, for arguments that are classes of the module.
    """
    results = []
    for module in (jinja2cpp, jinja2):
        env = module.Environment(**_env_kwargs(module, options))
        try:
            # Each engine gets its own copy: a template may change the data ({% do l.append(1) %})
            results.append(env.from_string(source).render(**copy.deepcopy(context or {})))
        except Exception as error:  # noqa: BLE001 - compared by class name below
            results.append(type(error).__name__)
    return tuple(results)


def assert_same(source, context=None, options=None):
    got, expected = render_both(source, context, options)
    assert got == expected, f"{source!r}: jinja2cpp {got!r}, Jinja2 {expected!r}"
    return got


@pytest.fixture
def same():
    return assert_same
