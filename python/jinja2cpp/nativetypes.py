"""jinja2.nativetypes: the native-types environment, which the engine does not do.

``NativeEnvironment`` is an Environment whose every template is rendered by real Jinja2
(docs/python.md, "Fallback"): without Jinja2, or with ``fallback="never"``, it raises UnsupportedError.
"""

from .environment import Environment


class NativeEnvironment(Environment):
    """An environment that renders to native Python types (jinja2.nativetypes.NativeEnvironment)."""

    _jinja2_class_path = "jinja2.nativetypes.NativeEnvironment"
    _tier_b_reason = "NativeEnvironment"
