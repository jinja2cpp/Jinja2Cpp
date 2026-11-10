"""Template frames in the tracebacks the module raises, as Jinja2's jinja2.debug (docs/tasks/0432).

Jinja2 compiles a template to Python, so an error shows the template's file and line as a frame of
the traceback. The engine runs no Python code for a template; the module adds such a frame by
raising the exception from a code object compiled at the template's file and line, which is what
Jinja2's ``fake_traceback`` does.
"""

import sys
import typing as t


def add_template_frame(exc: BaseException, filename: t.Optional[str], lineno: int, function: str) -> None:
    """Put a frame of `function` at `filename`:`lineno` in front of `exc`'s traceback.

    A traceback that holds the template frame then reads: the caller's frames, the template's line,
    then where the exception came from (the callable the template called), as in Jinja2.
    """
    if lineno < 1:
        return
    # A str and nothing else names a file: compile() refuses an embedded NUL
    if not isinstance(filename, str) or "\0" in filename:
        filename = "<template>"
    code = compile("\n" * (lineno - 1) + "raise __jinja_exception__", filename, "exec")
    code = code.replace(co_name=function)
    namespace = {"__name__": filename, "__file__": filename, "__jinja_exception__": exc}
    # The traceback of the raise continues the one exc has: this frame, the template's, then exc's own
    try:
        exec(code, namespace)  # noqa: S102 - a fixed raise statement
    except BaseException:  # noqa: BLE001 - only exc is raised there
        traceback = sys.exc_info()[2]
    else:  # pragma: no cover - the code always raises
        return
    # The frame keeps its globals: without exc in them, exc and its traceback make no cycle
    namespace.pop("__jinja_exception__", None)
    exc.__traceback__ = traceback.tb_next if traceback is not None else None
