#include "gtest/gtest.h"

#include <jinja2cpp/template.h>
#include <jinja2cpp/template_env.h>
#include <jinja2cpp/user_callable.h>
#include <jinja2cpp/value.h>

#include <cstdint>
#include <map>
#include <string>
#include <vector>

using namespace jinja2;

// The i18n extension with translations installed through TemplateEnv::InstallGettextCallables
// (Jinja2 install_gettext_callables, newstyle). Null translations are covered by the parity
// corpus (test/parity/cases/i18n.py).
class I18nTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_env.GetSettings().extensions.i18n = true;
        m_env.InstallGettextCallables(
            MakeCallable(
                [](const std::string& message) {
                    static const std::map<std::string, std::string> catalog = { { "Hello %(name)s!", "Hallo %(name)s!" }, { "100%%", "100 %%" } };
                    auto p = catalog.find(message);
                    return p == catalog.end() ? message : p->second;
                },
                ArgInfo{ "message" }),
            MakeCallable([](const std::string& singular, const std::string&, int64_t n) { return n == 1 ? "ein " + singular : "%(num)d " + singular + "e"; },
                         ArgInfo{ "singular" },
                         ArgInfo{ "plural" },
                         ArgInfo{ "n" }),
            MakeCallable([](const std::string& context, const std::string& message) { return context + ":" + message; }, ArgInfo{ "context" }, ArgInfo{ "message" }));
    }

    std::string Render(const std::string& source, const ValuesMap& params = {})
    {
        Template tpl(&m_env);
        auto parsed = tpl.Load(source);
        if (!parsed)
            return "load error: " + parsed.error().ToString();
        auto result = tpl.RenderAsString(params);
        return result ? result.value() : "render error: " + result.error().ToString();
    }

    TemplateEnv m_env;
};

TEST_F(I18nTest, TransUsesInstalledGettext)
{
    EXPECT_EQ("Hallo Welt!", Render("{% trans %}Hello {{ name }}!{% endtrans %}", { { "name", "Welt" } }));
    EXPECT_EQ("100 %", Render("{% trans %}100%{% endtrans %}"));
    EXPECT_EQ("Untranslated", Render("{% trans %}Untranslated{% endtrans %}"));
}

TEST_F(I18nTest, TransUsesInstalledNgettext)
{
    EXPECT_EQ("ein Apfel", Render("{% trans n=1 %}Apfel{% pluralize %}Apfels{% endtrans %}"));
    EXPECT_EQ("3 Apfele", Render("{% trans n=3 %}Apfel{% pluralize %}Apfels{% endtrans %}"));
}

// Variables named past any short string buffer, kept in the template's tree (0118 P5b-3c)
TEST_F(I18nTest, TransWithLongVariableNames)
{
    const std::string body = "{{ a_long_trans_user_name }} Apfel{% pluralize %}{{ a_long_trans_user_name }} Apfels{% endtrans %}";
    EXPECT_EQ("2 x Apfele", Render("{% trans a_long_trans_count_name=2, a_long_trans_user_name='x' %}" + body));
    EXPECT_EQ("ein x Apfel", Render("{% trans a_long_trans_count_name=1, a_long_trans_user_name='x' %}" + body));
}

TEST_F(I18nTest, GlobalsUseInstalledCallables)
{
    EXPECT_EQ("Hallo Welt!|Hallo Welt!", Render("{{ _('Hello %(name)s!', name='Welt') }}|{{ gettext('Hello %(name)s!', name='Welt') }}"));
    EXPECT_EQ("2 Birnee", Render("{{ ngettext('Birne', 'Birnen', 2) }}"));
    EXPECT_EQ("menu:Open", Render("{{ pgettext('menu', 'Open') }}"));
}

TEST_F(I18nTest, MissingCallableKeepsMessage)
{
    // npgettext was not installed: the message is not translated, as with null translations
    EXPECT_EQ("2 files", Render("{{ npgettext('ctx', '%(num)s file', '%(num)s files', 2) }}"));
}

TEST_F(I18nTest, ContextOverridesGlobals)
{
    auto shout = MakeCallable([](const std::string& message, const std::string& name) { return message + "/" + name; }, ArgInfo{ "message" }, ArgInfo{ "name" });
    EXPECT_EQ("Hello %(name)s!/x", Render("{% trans name='x' %}Hello {{ name }}!{% endtrans %}", { { "gettext", shout } }));
}

TEST_F(I18nTest, WideTemplate)
{
    TemplateW tpl(&m_env);
    ASSERT_TRUE(tpl.Load(L"{% trans name=L %}Hello {{ name }}!{% endtrans %}|{% trans n=2 %}x{% pluralize %}xs{% endtrans %}").has_value());
    auto result = tpl.RenderAsString({ { "L", "Welt" } });
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(L"Hallo Welt!|2 xe", result.value());
}

TEST(I18nDisabledTest, TransIsUnknownTag)
{
    Template tpl;
    EXPECT_FALSE(tpl.Load("{% trans %}a{% endtrans %}").has_value());
}

// Oldstyle gettext, Jinja2's default (Settings::Extensions::i18nNewstyle off, task 0415): the globals return what the
// installed callable returns, and a `{% trans %}` block formats the translation itself
class I18nOldstyleTest : public I18nTest
{
protected:
    void SetUp() override
    {
        I18nTest::SetUp();
        m_env.GetSettings().extensions.i18nNewstyle = false;
    }
};

TEST_F(I18nOldstyleTest, TransFormatsTheTranslation)
{
    EXPECT_EQ("Hallo Welt!", Render("{% trans %}Hello {{ name }}!{% endtrans %}", { { "name", "Welt" } }));
    // Without a variable the message keeps its single `%`, and the translation is not formatted
    EXPECT_EQ("100%", Render("{% trans %}100%{% endtrans %}"));
    EXPECT_EQ("100 %%", Render("{% trans %}100%%{% endtrans %}"));
    // The variables are the block's: ngettext's `num` is not one unless the count is named so
    EXPECT_NE(Render("{% trans n=3 %}Apfel{% pluralize %}Apfels{% endtrans %}").find("KeyError: 'num'"), std::string::npos);
    EXPECT_EQ("3 Apfele", Render("{% trans num=3 %}Apfel{% pluralize %}Apfels{% endtrans %}"));
    EXPECT_EQ("2 2 xe", Render("{% trans num=2 %}{{ num }} x{% pluralize %}{{ num }} xs{% endtrans %}"));
}

TEST_F(I18nOldstyleTest, GlobalsAreTheInstalledCallables)
{
    EXPECT_EQ("Hallo %(name)s!|Hallo %(name)s!", Render("{{ _('Hello %(name)s!') }}|{{ gettext('Hello %(name)s!') }}"));
    EXPECT_EQ("%(num)d Birnee|menu:Open", Render("{{ ngettext('Birne', 'Birnen', 2) }}|{{ pgettext('menu', 'Open') }}"));
    // A keyword argument goes to the callable, which ignores it here; nothing formats the translation
    EXPECT_EQ("Hallo %(name)s!", Render("{{ gettext('Hello %(name)s!', name='x') }}"));
}

TEST_F(I18nOldstyleTest, NullTranslationsReturnTheMessage)
{
    // npgettext was not installed: gettext.NullTranslations, which takes no keyword arguments
    EXPECT_EQ("%(num)s files", Render("{{ npgettext('ctx', '%(num)s file', '%(num)s files', 2) }}"));
    EXPECT_NE(Render("{{ npgettext('ctx', 'a', 'b', 2, num=2) }}").find("unexpected keyword argument 'num'"), std::string::npos);
}

TEST_F(I18nOldstyleTest, Autoescape)
{
    m_env.GetSettings().autoescape = true;
    EXPECT_EQ("<i>|&lt;b&gt; Apfel", Render("{% trans %}<i>{% endtrans %}|{% trans n='<b>' %}{{ n }} Apfel{% endtrans %}"));
    EXPECT_EQ("&lt;i&gt;", Render("{{ _('<i>') }}"));
}

TEST_F(I18nOldstyleTest, WideTemplate)
{
    TemplateW tpl(&m_env);
    ASSERT_TRUE(tpl.Load(L"{% trans name=L %}Hello {{ name }}!{% endtrans %}|{% trans %}100%{% endtrans %}|{{ _('a %(b)s') }}").has_value());
    auto result = tpl.RenderAsString({ { "L", "Welt" } });
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(L"Hallo Welt!|100%|a %(b)s", result.value());
}

// An install that fixes the style (GettextStyle, docs/tasks/0509) gives every template's gettext globals that style,
// as Jinja2's globals are what the last install put; `{% trans %}` blocks keep the style they were loaded with
TEST_F(I18nOldstyleTest, InstallFixesTheStyleOfTheGlobals)
{
    // A call with a keyword argument and a block without one, loaded under each style
    const char* call = "{{ gettext('Hello %(name)s!', name='x') }}";
    const char* block = "{% trans %}100%{% endtrans %}";
    std::vector<Template> oldstyle(2, Template(&m_env));
    ASSERT_TRUE(oldstyle[0].Load(call).has_value());
    ASSERT_TRUE(oldstyle[1].Load(block).has_value());
    m_env.GetSettings().extensions.i18nNewstyle = true;
    std::vector<Template> newstyle(2, Template(&m_env));
    ASSERT_TRUE(newstyle[0].Load(call).has_value());
    ASSERT_TRUE(newstyle[1].Load(block).has_value());
    const auto render = [](const Template& tpl) {
        auto result = tpl.RenderAsString({});
        return result ? result.value() : result.error().ToString();
    };
    const auto expect = [&](const char* what, const std::vector<std::string>& expected) {
        EXPECT_EQ(render(oldstyle[0]), expected[0]) << what;
        EXPECT_NE(render(oldstyle[1]).find(expected[1]), std::string::npos) << what << ": " << render(oldstyle[1]);
        EXPECT_EQ(render(newstyle[0]), expected[2]) << what;
        EXPECT_EQ(render(newstyle[1]), expected[3]) << what;
    };
    // Following the load: each template its own style
    expect("load", { "Hallo %(name)s!", "100%", "Hallo x!", "100 %" });

    auto gettext = MakeCallable([](const std::string& message) { return message.rfind("Hello", 0) == 0 ? "Hallo" + message.substr(5) : message; }, ArgInfo{ "message" });
    auto ngettext = MakeCallable([](const std::string& s, const std::string& p, int n) { return n == 1 ? s : p; }, ArgInfo{ "s" }, ArgInfo{ "p" }, ArgInfo{ "n" });
    // Newstyle globals format every call; an oldstyle block passes its `%` unescaped, which Jinja2's newstyle
    // gettext then fails to format (ValueError: incomplete format)
    m_env.InstallGettextCallables(GettextStyle::Newstyle, gettext, ngettext);
    expect("newstyle install", { "Hallo x!", "incomplete format", "Hallo x!", "100%" });
    // Oldstyle globals return the translation as it is: a newstyle block's doubled `%` stays
    m_env.InstallGettextCallables(GettextStyle::Oldstyle, gettext, ngettext);
    expect("oldstyle install", { "Hallo %(name)s!", "100%", "Hallo %(name)s!", "100%%" });
    m_env.InstallGettextCallables(gettext, ngettext);
    expect("install following the load", { "Hallo %(name)s!", "100%", "Hallo x!", "100%" });
}

// A template loaded again after the style changed is parsed again: the setting takes part in Settings ==
TEST_F(I18nOldstyleTest, ReloadAfterTheStyleChanged)
{
    EXPECT_EQ("100%", Render("{% trans %}100%{% endtrans %}"));
    m_env.GetSettings().extensions.i18nNewstyle = true;
    EXPECT_EQ("100 %", Render("{% trans %}100%{% endtrans %}"));
    auto newstyle = m_env.GetSettings();
    auto oldstyle = newstyle;
    oldstyle.extensions.i18nNewstyle = false;
    EXPECT_FALSE(newstyle == oldstyle);
}
