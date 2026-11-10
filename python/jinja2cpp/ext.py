"""The extensions jinja2cpp implements natively, by Jinja2's names.

Environment(extensions=[...]) takes these, Jinja2's own classes or their dotted names
("jinja2.ext.do", "jinja2.ext.loopcontrols", "jinja2.ext.i18n").
"""

import re
import typing as t

from markupsafe import Markup

from . import _jinja2cpp
from .utils import pass_context


try:  # a custom extension subclasses this class and runs in Jinja2: the fallback (docs/python.md)
    from jinja2.ext import Extension as _Base
except ImportError:  # pragma: no cover - the tests cover it in a subprocess
    _Base = object  # type: ignore[assignment, misc]


class Extension(_Base):  # type: ignore[valid-type, misc]
    """Base class of the extensions. The engine implements three; a custom extension makes its
    environment a Jinja2 one (the fallback), which needs Jinja2 installed."""


class ExprStmtExtension(Extension):
    """The {% do %} statement."""


class LoopControlExtension(Extension):
    """{% break %} and {% continue %} in loops."""


do = ExprStmtExtension
loopcontrols = LoopControlExtension


class InternationalizationExtension(Extension):
    """{% trans %} and the gettext globals (jinja2.ext.i18n)."""


i18n = InternationalizationExtension

_GETTEXT_NAMES = ("gettext", "ngettext", "pgettext", "npgettext")


def _install(env: t.Any, translations: t.Any, newstyle: t.Optional[bool] = None) -> None:
    """Environment.install_gettext_translations."""
    gettext = getattr(translations, "ugettext", None)
    if gettext is None:
        gettext = translations.gettext
    ngettext = getattr(translations, "ungettext", None)
    if ngettext is None:
        ngettext = translations.ngettext
    pgettext = getattr(translations, "pgettext", None)
    npgettext = getattr(translations, "npgettext", None)
    _install_callables(env, gettext, ngettext, newstyle=newstyle, pgettext=pgettext, npgettext=npgettext)


def _install_null(env: t.Any, newstyle: t.Optional[bool] = None) -> None:
    """Environment.install_null_translations."""
    import gettext

    translations = gettext.NullTranslations()
    _install_callables(
        env,
        gettext=translations.gettext,
        ngettext=translations.ngettext,
        newstyle=newstyle,
        pgettext=translations.pgettext,
        npgettext=translations.npgettext,
    )


def _install_callables(
    env: t.Any,
    gettext: t.Callable[[str], str],
    ngettext: t.Callable[[str, str, int], str],
    newstyle: t.Optional[bool] = None,
    pgettext: t.Optional[t.Callable[[str, str], str]] = None,
    npgettext: t.Optional[t.Callable[[str, str, str, int], str]] = None,
) -> None:
    """Environment.install_gettext_callables.

    Newstyle gettext is the engine's own: it translates with the callables and formats the result
    with the variables of the call. Oldstyle (Jinja2's default) installs the callables as the
    ``gettext``, ``ngettext``, ``pgettext`` and ``npgettext`` globals, wrapped so that a call
    without variables returns the translation as it is, and a ``{% trans %}`` block formats it.
    """
    if newstyle is not None:
        env.newstyle_gettext = bool(newstyle)
    _remove_globals(env)
    if env.newstyle_gettext:
        env._set_gettext(gettext, ngettext, pgettext, npgettext)
        return
    # The oldstyle wrappers replace the engine's, which format every call
    env._set_gettext(None, None, None, None)
    wrappers = (_old_gettext(gettext), _old_ngettext(ngettext), _old_pgettext(pgettext), _old_npgettext(npgettext))
    for name, wrapper in zip(_GETTEXT_NAMES, wrappers):
        if wrapper is not None:
            env.globals[name] = wrapper
            wrapper._jinja2cpp_gettext = True  # type: ignore[attr-defined]


def _uninstall(env: t.Any, translations: t.Any = None) -> None:
    """Environment.uninstall_gettext_translations."""
    env._set_gettext(None, None, None, None)
    for name in _GETTEXT_NAMES:
        env.globals.pop(name, None)


def _remove_globals(env: t.Any) -> None:
    # A previous install's wrappers; a global the program set by hand is left alone
    for name in _GETTEXT_NAMES:
        if getattr(env.globals.get(name), "_jinja2cpp_gettext", False):
            del env.globals[name]


# A %-conversion: (key), flags, width, .precision, length modifier, type (printf-style formatting)
_PERCENT = re.compile(r"%(?:\((?P<key>[^)]*)\))?[#0\- +]*(?P<width>\*|[0-9]+)?(?:\.(?P<precision>\*|[0-9]+))?[hlL]?(?P<type>.?)", re.DOTALL)
_NUMBER_TYPES = frozenset("diouxXeEfFgGc")


def _check_percent_size(translated: str, variables: t.Mapping[str, t.Any]) -> None:
    # Python builds the string first; the render's cap on sequences (MaxSequenceSize) is checked before, as for
    # str.format in the sandbox (SandboxedFormatter): each field's width, a number's precision, and an estimate of
    # the whole, the literal text and each field's string (a field may repeat a long value)
    total = 0
    position = 0
    for match in _PERCENT.finditer(translated):
        total += match.start() - position
        position = match.end()
        if match.group("type") == "%":
            total += 1
            continue
        size = int(match.group("width")) if match.group("width") not in (None, "*") else 0
        if match.group("precision") not in (None, "*") and match.group("type") in _NUMBER_TYPES:
            size = max(size, int(match.group("precision")))
        _jinja2cpp._check_sequence_size(size)
        value = variables.get(match.group("key")) if match.group("key") is not None and isinstance(variables, dict) else None
        if isinstance(value, str):
            size = max(size, len(value))
        total += size
        _jinja2cpp._check_sequence_size(total)
    _jinja2cpp._check_sequence_size(total + len(translated) - position)


def _format(ctx: t.Any, translated: t.Any, variables: t.Dict[str, t.Any]) -> t.Any:
    if not variables:
        return translated
    if ctx.eval_ctx.autoescape:
        translated = Markup(translated)
    if isinstance(translated, str):
        _check_percent_size(translated, variables)
    return translated % variables


def _old_gettext(func: t.Optional[t.Callable[..., str]]) -> t.Optional[t.Callable[..., t.Any]]:
    if func is None:
        return None

    @pass_context
    def gettext(ctx: t.Any, string: str, **variables: t.Any) -> t.Any:
        return _format(ctx, ctx.call(func, string), variables)

    return gettext


def _old_ngettext(func: t.Optional[t.Callable[..., str]]) -> t.Optional[t.Callable[..., t.Any]]:
    if func is None:
        return None

    @pass_context
    def ngettext(ctx: t.Any, singular: str, plural: str, num: int, **variables: t.Any) -> t.Any:
        rv = ctx.call(func, singular, plural, num)
        if variables:
            variables.setdefault("num", num)
        return _format(ctx, rv, variables)

    return ngettext


def _old_pgettext(func: t.Optional[t.Callable[..., str]]) -> t.Optional[t.Callable[..., t.Any]]:
    if func is None:
        return None

    @pass_context
    def pgettext(ctx: t.Any, string_ctx: str, string: str, **variables: t.Any) -> t.Any:
        if variables:
            variables.setdefault("context", string_ctx)
        return _format(ctx, ctx.call(func, string_ctx, string), variables)

    return pgettext


def _old_npgettext(func: t.Optional[t.Callable[..., str]]) -> t.Optional[t.Callable[..., t.Any]]:
    if func is None:
        return None

    @pass_context
    def npgettext(ctx: t.Any, string_ctx: str, singular: str, plural: str, num: int, **variables: t.Any) -> t.Any:
        if variables:
            variables.setdefault("context", string_ctx)
            variables.setdefault("num", num)
        return _format(ctx, ctx.call(func, string_ctx, singular, plural, num), variables)

    return npgettext
