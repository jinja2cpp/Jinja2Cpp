"""jinja2.ext.i18n: {% trans %}, the gettext globals and the install methods, compared with Jinja2."""

import gettext

import jinja2
import jinja2.ext
import jinja2cpp
import jinja2cpp.ext
import pytest
import time

MODULES = (jinja2, jinja2cpp)


class Fake(gettext.NullTranslations):
    """A small catalog that marks what it translated."""

    CATALOG = {
        "Hello": "Bonjour",
        "Hello %(name)s": "Bonjour %(name)s",
        "%(num)s apple": "%(num)s pomme",
        "%(num)s apples": "%(num)s pommes",
        "Save": "Enregistrer",
    }

    def gettext(self, message):
        return self.CATALOG.get(message, message)

    def ngettext(self, singular, plural, n):
        return self.CATALOG.get(singular if n == 1 else plural, singular if n == 1 else plural)

    def pgettext(self, context, message):
        return f"{context}:{self.CATALOG.get(message, message)}"

    def npgettext(self, context, singular, plural, n):
        return f"{context}:" + self.ngettext(singular, plural, n)


def render(source, install, autoescape=False, **context):
    """Renders with both engines; install(env) sets the translations up."""
    results = []
    for module in MODULES:
        env = module.Environment(extensions=["jinja2.ext.i18n"], autoescape=autoescape)
        install(env)
        try:
            results.append(env.from_string(source).render(**context))
        except Exception as error:  # noqa: BLE001 - compared by class name
            results.append(type(error).__name__)
    return results


def same(source, install, **kwargs):
    got = render(source, install, **kwargs)
    assert got[0] == got[1], f"{source!r}: Jinja2 {got[0]!r}, jinja2cpp {got[1]!r}"
    return got[1]


def translations(newstyle):
    return lambda env: env.install_gettext_translations(Fake(), newstyle=newstyle)


STYLES = pytest.mark.parametrize("newstyle", [None, False, True], ids=["default", "oldstyle", "newstyle"])
ESCAPE = pytest.mark.parametrize("autoescape", [False, True])

COMMON = [
    "{{ _('Hello') }}",
    "{{ gettext('Hello') }}|{{ gettext('Nope') }}",
    "{% trans %}Hello{% endtrans %}",
    "{% trans name='Bob' %}Hello {{ name }}{% endtrans %}",
    "{% trans %}Hello {{ name }}{% endtrans %}",
    "{% trans n=3 %}{{ n }} apple{% pluralize %}{{ n }} apples{% endtrans %}",
    "{% trans n=1 %}{{ n }} apple{% pluralize %}{{ n }} apples{% endtrans %}",
    "{% trans count=2 %}{{ count }} apple{% pluralize %}{{ count }} apples{% endtrans %}",
    "{% trans trimmed %}  Hello  \n   {% endtrans %}|{% trans notrimmed %} Hello {% endtrans %}",
    "{% trans context='menu' %}Save{% endtrans %}",
    "{{ pgettext('menu', 'Save') }}|{{ ngettext('%(num)s apple', '%(num)s apples', 1) }}",
    "{{ npgettext('menu', 'a', 'b', 2) }}",
    "{% trans x='<b>' %}Hi {{ x }}{% endtrans %}",
    "{{ _('Hello')|upper }}{% set m = _('Hello') %}{{ m }}",
    "{% for i in range(2) %}{% trans i=i %}Hello {{ i }}{% endtrans %}{% endfor %}",
    "{% macro m() %}{% trans %}Hello{% endtrans %}{% endmacro %}{{ m() }}",
]


@STYLES
@ESCAPE
@pytest.mark.parametrize("source", COMMON)
def test_translations_as_jinja2(source, newstyle, autoescape):
    same(source, translations(newstyle), autoescape=autoescape, name="Al")


@pytest.mark.parametrize("source", ["{{ gettext('Hello %(name)s', name='x') }}", "{{ ngettext('%(num)s apple', '%(num)s apples', 3) }}"])
@ESCAPE
def test_newstyle_formats_calls(source, autoescape):
    assert same(source, translations(True), autoescape=autoescape) in ("Bonjour x", "3 pommes")


@ESCAPE
def test_oldstyle_calls_without_variables_return_the_translation(autoescape):
    # the % in the message is the template's; Jinja2's oldstyle formats nothing
    same("{{ _('100%') }}|{{ _('Hello %(name)s')|format(name='<x>') }}", translations(False), autoescape=autoescape)


def test_a_count_named_num():
    source = "{% trans num=2 %}{{ num }} apple{% pluralize %}{{ num }} apples{% endtrans %}"
    assert same(source, translations(True)) == "2 pommes"
    # oldstyle: the engine passes no variable for it, so the wrapper cannot tell the block from a call (docs/python.md)
    assert render(source, translations(False))[0] == "2 pommes"


def test_oldstyle_is_the_default_and_newstyle_gettext_is_an_attribute():
    for module in MODULES:
        env = module.Environment(extensions=["jinja2.ext.i18n"])
        assert env.newstyle_gettext is False
        env.install_null_translations()
        assert env.newstyle_gettext is False
        env.install_null_translations(newstyle=True)
        assert env.newstyle_gettext is True
        # a call without newstyle keeps the last choice
        env.install_gettext_translations(Fake())
        assert env.newstyle_gettext is True


@STYLES
def test_null_translations(newstyle):
    for source in ("{{ _('Hello') }}", "{% trans %}Hello{% endtrans %}", "{% trans n=2 %}{{ n }} a{% pluralize %}{{ n }} b{% endtrans %}", "{{ pgettext('c', 'x') }}{{ npgettext('c', 'a', 'b', 1) }}"):
        same(source, lambda env: env.install_null_translations(newstyle=newstyle))


@STYLES
def test_install_gettext_callables(newstyle):
    calls = []

    def install(env):
        def gettext(message):
            calls.append(("gettext", message))
            return message.upper()

        def ngettext(singular, plural, n):
            calls.append(("ngettext", singular, plural, n))
            return singular if n == 1 else plural

        env.install_gettext_callables(gettext, ngettext, newstyle=newstyle, pgettext=lambda c, m: f"{c}/{m}", npgettext=lambda c, s, p, n: f"{c}/{s}/{p}/{n}")

    for source in ("{{ _('abc') }}", "{% trans %}abc{% endtrans %}", "{% trans n=2 %}{{ n }} x{% pluralize %}{{ n }} y{% endtrans %}", "{{ pgettext('c', 'm') }}{{ npgettext('c', 'a', 'b', 3) }}"):
        same(source, install)
    assert ("gettext", "abc") in calls and ("ngettext", "%(n)s x", "%(n)s y", 2) in calls


@STYLES
def test_translation_callables_may_be_pass_context(newstyle):
    for module in MODULES:
        env = module.Environment(extensions=["jinja2.ext.i18n"])

        @module.pass_context
        def gettext(ctx, message):
            return f"{message}@{ctx.get('lang')}"

        env.install_gettext_callables(gettext, lambda s, p, n: s, newstyle=newstyle)
        assert env.from_string("{{ _('Hi') }}").render(lang="fr") == "Hi@fr"


@STYLES
def test_uninstall_removes_the_globals(newstyle):
    for module in MODULES:
        env = module.Environment(extensions=["jinja2.ext.i18n"])
        env.install_gettext_translations(Fake(), newstyle=newstyle)
        assert "gettext" in env.globals or newstyle
        env.uninstall_gettext_translations(Fake())
        assert "gettext" not in env.globals and "ngettext" not in env.globals


def test_reinstall_replaces_the_translations():
    for module in MODULES:
        env = module.Environment(extensions=["jinja2.ext.i18n"])
        template = env.from_string("{{ _('Hello') }}")
        env.install_gettext_translations(Fake(), newstyle=True)
        assert template.render() == "Bonjour"
        env.install_null_translations(newstyle=True)
        assert template.render() == "Hello"
        env.install_gettext_translations(Fake(), newstyle=False)
        assert template.render() == "Bonjour"


def test_a_global_gettext_replaces_the_installed_one():
    for module in MODULES:
        env = module.Environment(extensions=["jinja2.ext.i18n"])
        env.install_gettext_translations(Fake(), newstyle=True)
        env.globals["gettext"] = lambda m: "mine"
        assert env.from_string("{{ _('Hello') }}{{ gettext('x') }}").render() == "minemine"


def test_extension_by_name_class_and_module():
    sources = ["{% trans %}Hello{% endtrans %}"]
    for extension in ("jinja2.ext.i18n", "jinja2cpp.ext.i18n", jinja2.ext.InternationalizationExtension, jinja2cpp.ext.InternationalizationExtension, jinja2cpp.ext.i18n):
        env = jinja2cpp.Environment(extensions=[extension])
        env.install_null_translations()
        assert env.from_string(sources[0]).render() == "Hello"
    # without the extension a trans block is a syntax error, as in Jinja2
    for module in MODULES:
        with pytest.raises(module.TemplateSyntaxError):
            module.Environment().from_string(sources[0])


def test_other_extensions_still_work_together():
    env = jinja2cpp.Environment(extensions=["jinja2.ext.i18n", "jinja2.ext.do", "jinja2.ext.loopcontrols"])
    env.install_null_translations()
    assert env.from_string("{% set l = [] %}{% do l.append(1) %}{% trans %}x{% endtrans %}{{ l }}").render() == "x[1]"


def test_syntax_errors_as_jinja2():
    for source in ("{% trans %}a{% if x %}b{% endif %}{% endtrans %}", "{% trans %}a", "{% trans %}{% trans %}{% endtrans %}{% endtrans %}", "{% trans %}{% pluralize %}b{% endtrans %}"):
        kinds = []
        for module in MODULES:
            env = module.Environment(extensions=["jinja2.ext.i18n"])
            try:
                env.from_string(source)
                kinds.append(None)
            except module.TemplateSyntaxError as error:
                kinds.append(type(error).__name__)
        assert kinds[0] == kinds[1] == "TemplateSyntaxError", source


def test_without_installed_translations_the_engine_leaves_messages_as_they_are():
    # Jinja2 has no gettext until one is installed and raises UndefinedError (docs/python.md)
    env = jinja2cpp.Environment(extensions=["jinja2.ext.i18n"])
    assert env.from_string("{{ _('Hello') }}{% trans %}Hi{% endtrans %}").render() == "HelloHi"
    with pytest.raises(jinja2.exceptions.UndefinedError):
        jinja2.Environment(extensions=["jinja2.ext.i18n"]).from_string("{{ _('Hello') }}").render()


@pytest.mark.parametrize(
    "source",
    [
        "{{ gettext('%(a)1000000000s', a=1)|length }}",
        "{{ gettext('%(a).1000000000f', a=1.5)|length }}",
        "{{ gettext('%(a)s%(a)s%(a)s', a=s)|length }}",
    ],
)
def test_formatting_a_translation_is_capped_before_it_allocates(source):
    # Oldstyle calls format in Python (newstyle ones in the engine, capped there): Python would build the string
    # first (a 1 GB width); the render's cap (MaxSequenceSize) is checked from the widths, a number's precision and
    # the strings the fields repeat, as for str.format in the sandbox
    env = jinja2cpp.Environment(extensions=["jinja2.ext.i18n"])
    env.install_null_translations(newstyle=False)
    with pytest.raises(jinja2cpp.exceptions.TemplateRuntimeError, match="too long"):
        env.from_string(source).render(s="y" * 30000000)


@pytest.mark.parametrize("autoescape", [False, True])
def test_formatting_a_translation_that_fits_is_as_jinja2(autoescape):
    same("{{ gettext('%(a)5s|%(b).2f|100%%|%(c)s', a='x', b=1.5, c='<i>') }}", translations(True), autoescape=autoescape)


def test_formatting_a_translation_checks_what_fits_in_one_pass():
    # Codex on #823: a precision truncates a repeated long string, and a run of unclosed keys is not rescanned
    env = jinja2cpp.Environment(extensions=["jinja2.ext.i18n"])
    env.install_null_translations(newstyle=False)
    assert env.from_string("{{ gettext('%(a).1s%(a).1s%(a).1s|%((b))s', a=s, **{'(b)': 2}) }}").render(s="y" * 30000000) == "yyy|2"
    started = time.perf_counter()
    with pytest.raises(ValueError, match="incomplete format key"):
        env.from_string("{{ gettext(t, a=1) }}").render(t="%(" * 80000)
    assert time.perf_counter() - started < 2
