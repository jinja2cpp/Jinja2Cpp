"""transformers' chat templates rendered by jinja2cpp (0248 plan D9, docs/tasks/0355).

``patch()`` replaces ``transformers.utils.chat_template_utils._compile_jinja_template``, which
``apply_chat_template`` (and vLLM and SGLang through it) uses to compile a chat template, with
one that compiles it in jinja2cpp's equivalent of transformers' environment::

    import jinja2cpp.integrations.transformers
    jinja2cpp.integrations.transformers.patch()

The environment is an ``ImmutableSandboxedEnvironment`` with ``trim_blocks``, ``lstrip_blocks``
and ``loopcontrols``, and transformers' ``tojson``, ``raise_exception`` and ``strftime_now``
built into the engine (``ConfigureForChatTemplates``, docs/tasks/0360), so a chat template
renders without calling back into Python. What jinja2cpp does not do goes to transformers' own
function, so the result is always what transformers gives:

- a template jinja2cpp cannot load (a syntax it lacks) is compiled by transformers;
- ``return_assistant_tokens_mask=True``, which needs ``{% generation %}`` to record where the
  assistant's text is: the render inside ``activate_tracker`` runs in transformers' template;
- ``environment.parse`` and the other attributes of the environment (transformers reads the
  template's variables and literals from Jinja2's AST) are those of transformers' environment.

``unpatch()`` restores transformers' function.
"""

from __future__ import annotations

import contextlib
import functools
import threading
import typing as t

from .. import exceptions
from ..sandbox import ImmutableSandboxedEnvironment

_lock = threading.Lock()
_original: t.Optional[t.Callable[[str], t.Any]] = None
_environment: t.Optional[ImmutableSandboxedEnvironment] = None


def chat_template_environment(clock: t.Optional[t.Callable[[], float]] = None) -> ImmutableSandboxedEnvironment:
    """A new environment configured as transformers configures its chat-template environment.

    `clock` returns the POSIX timestamp ``strftime_now`` formats (local time, as
    ``datetime.now()``); None reads the system clock. Templates use no fallback to Jinja2: one
    that does not load raises, and ``patch()`` hands it to transformers instead.
    """
    env = ImmutableSandboxedEnvironment(trim_blocks=True, lstrip_blocks=True, extensions=["jinja2.ext.loopcontrols"], fallback="never")
    env._set_chat_templates(clock)
    return env


class _EnvironmentProxy:
    """``compiled.environment``: transformers' environment for what jinja2cpp does not provide."""

    def __init__(self, owner: ChatTemplate) -> None:
        self._owner = owner

    @contextlib.contextmanager
    def activate_tracker(self, rendered_blocks: t.List[str], generation_indices: t.List[t.Tuple[int, int]]) -> t.Iterator[None]:
        # The assistant mask: generate() inside the tracker runs in transformers' template
        with self._owner._transformers().environment.activate_tracker(rendered_blocks, generation_indices):
            self._owner._tracking.depth = getattr(self._owner._tracking, "depth", 0) + 1
            try:
                yield
            finally:
                self._owner._tracking.depth -= 1

    def __getattr__(self, name: str) -> t.Any:
        return getattr(self._owner._transformers().environment, name)


class ChatTemplate:
    """A chat template compiled by jinja2cpp, as ``_compile_jinja_template`` returns it."""

    def __init__(self, source: str, template: t.Any, original: t.Callable[[str], t.Any]) -> None:
        self._source = source
        self._template = template
        self._original = original
        self._jinja2: t.Any = None
        self._tracking = threading.local()
        self.environment = _EnvironmentProxy(self)

    def _transformers(self) -> t.Any:
        """transformers' own compiled template of the same source, made on first use."""
        if self._jinja2 is None:
            self._jinja2 = self._original(self._source)
        return self._jinja2

    def _tracked(self) -> bool:
        return getattr(self._tracking, "depth", 0) > 0

    def render(self, *args: t.Any, **kwargs: t.Any) -> str:
        if self._tracked():
            return self._transformers().render(*args, **kwargs)  # type: ignore[no-any-return]
        return self._template.render(*args, **kwargs)  # type: ignore[no-any-return]

    def generate(self, *args: t.Any, **kwargs: t.Any) -> t.Iterator[str]:
        if self._tracked():
            return self._transformers().generate(*args, **kwargs)  # type: ignore[no-any-return]
        return self._template.generate(*args, **kwargs)  # type: ignore[no-any-return]

    def __getattr__(self, name: str) -> t.Any:
        if name.startswith("_"):
            # Not set yet (copy.copy, unpickling): no recursion through self._template
            raise AttributeError(name)
        return getattr(self._template, name)


def _shared_environment() -> ImmutableSandboxedEnvironment:
    global _environment
    with _lock:
        if _environment is None:
            _environment = chat_template_environment()
        return _environment


@functools.lru_cache(maxsize=128)
def _compile(chat_template: str, original: t.Callable[[str], t.Any]) -> t.Any:
    try:
        template = _shared_environment().from_string(chat_template)
    except (exceptions.TemplateSyntaxError, exceptions.UnsupportedError):
        # A syntax or feature jinja2cpp lacks: transformers compiles it, and raises if Jinja2 fails too
        return original(chat_template)
    return ChatTemplate(chat_template, template, original)


def compile_jinja_template(chat_template: str) -> t.Any:
    """``_compile_jinja_template`` in jinja2cpp: what ``patch()`` installs."""
    with _lock:
        original = _original
    if original is None:
        # unpatch() ran while this call was on its way: transformers' own function
        return _module()._compile_jinja_template(chat_template)
    return _compile(chat_template, original)


def _module() -> t.Any:
    import transformers.utils.chat_template_utils as utils

    return utils


def _clear_caches(utils: t.Any) -> None:
    # transformers caches renders (render_jinja_template is an lru_cache), which hold the old engine's output
    for name in ("render_jinja_template", "_get_template_variables", "_get_template_literal_text"):
        cache_clear = getattr(getattr(utils, name, None), "cache_clear", None)
        if cache_clear is not None:
            cache_clear()


def patch() -> None:
    """Make transformers compile chat templates with jinja2cpp (idempotent)."""
    global _original
    utils = _module()
    with _lock:
        if utils._compile_jinja_template is compile_jinja_template:
            return
        _original = utils._compile_jinja_template
        utils._compile_jinja_template = compile_jinja_template
    _compile.cache_clear()
    _clear_caches(utils)


def unpatch() -> None:
    """Restore transformers' own ``_compile_jinja_template``."""
    global _original
    utils = _module()
    with _lock:
        if utils._compile_jinja_template is not compile_jinja_template or _original is None:
            return
        utils._compile_jinja_template = _original
        _original = None
    _compile.cache_clear()
    _clear_caches(utils)


def is_patched() -> bool:
    """Whether ``patch()`` is in effect."""
    return _module()._compile_jinja_template is compile_jinja_template
