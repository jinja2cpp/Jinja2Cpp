"""The extensions jinja2cpp implements natively, by Jinja2's names.

Environment(extensions=[...]) takes these, Jinja2's own classes or their dotted names
("jinja2.ext.do", "jinja2.ext.loopcontrols", "jinja2.ext.i18n").
"""

import typing as t


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
    with the variables of the call. Oldstyle (Jinja2's default) installs the callables themselves as
    the ``gettext``, ``ngettext``, ``pgettext`` and ``npgettext`` globals, as Jinja2 does; a
    ``{% trans %}`` block loaded while ``newstyle_gettext`` is off formats their result in the
    engine (Settings::Extensions::i18nNewstyle).
    """
    if newstyle is not None:
        env.newstyle_gettext = bool(newstyle)
    _remove_globals(env)
    if env.newstyle_gettext:
        env._set_gettext(gettext, ngettext, pgettext, npgettext)
        return
    env._set_gettext(None, None, None, None)
    callables = (gettext, ngettext, pgettext, npgettext)
    for name, func in zip(_GETTEXT_NAMES, callables):
        if func is not None:
            env.globals[name] = func
    # Kept by the environment (its traverse reaches them): a bound method of it holds it
    env._gettext_globals = callables


def _uninstall(env: t.Any, translations: t.Any = None) -> None:
    """Environment.uninstall_gettext_translations."""
    env._set_gettext(None, None, None, None)
    env._gettext_globals = (None, None, None, None)
    for name in _GETTEXT_NAMES:
        env.globals.pop(name, None)


def _remove_globals(env: t.Any) -> None:
    # A previous oldstyle install's callables; a global the program set by hand is left alone
    for name, func in zip(_GETTEXT_NAMES, env._gettext_globals):
        if func is not None and env.globals.get(name) is func:
            del env.globals[name]
    env._gettext_globals = (None, None, None, None)
