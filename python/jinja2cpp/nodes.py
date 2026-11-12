"""jinja2.nodes: the evaluation context only.

Jinja2's ``jinja2.nodes`` holds the template AST, which the engine keeps in C++. ``EvalContext`` is
the one name of it that user code passes around: a ``pass_eval_context`` callable gets one, and a
module macro takes one as its first argument, as Jinja2's ``Macro`` does.
"""

from .runtime import EvalContext as EvalContext

__all__ = ["EvalContext"]
