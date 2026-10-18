#include "gtest/gtest.h"

#include <jinja2cpp/filesystem_handler.h>
#include <jinja2cpp/template.h>
#include <jinja2cpp/template_env.h>
#include <jinja2cpp/value.h>

#include <cstddef>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace jinja2;

namespace
{
template<typename CharT>
struct TemplateFor
{
    using type = Template;
};
template<>
struct TemplateFor<wchar_t>
{
    using type = TemplateW;
};

template<typename CharT>
std::basic_string<CharT> Render(const std::basic_string<CharT>& source, const ValuesMap& params, bool autoescape)
{
    TemplateEnv env;
    Settings settings;
    settings.autoescape = autoescape;
    env.SetSettings(settings);
    typename TemplateFor<CharT>::type tpl(&env);
    auto loaded = tpl.Load(source);
    EXPECT_TRUE(!!loaded);
    auto result = tpl.RenderAsString(params);
    EXPECT_TRUE(!!result);
    return result ? result.value() : std::basic_string<CharT>();
}

template<typename CharT>
std::basic_string<CharT> ParseError(const std::basic_string<CharT>& source)
{
    typename TemplateFor<CharT>::type tpl;
    auto loaded = tpl.Load(source);
    EXPECT_FALSE(!!loaded);
    return loaded ? std::basic_string<CharT>() : loaded.error().ToString();
}
} // namespace

TEST(AutoescapeTest, SettingTakesPartInEquality)
{
    Settings on;
    on.autoescape = true;
    EXPECT_FALSE(on == Settings());
    EXPECT_TRUE(on != Settings());
}

TEST(AutoescapeTest, EscapesValuesFromCpp)
{
    ValuesMap params{ { "html", "<a href='x'>&</a>" }, { "n", 5 } };
    const std::string tpl = "{{ html }}|{{ html|safe }}|{{ n }}|<p>";
    EXPECT_EQ("&lt;a href=&#39;x&#39;&gt;&amp;&lt;/a&gt;|<a href='x'>&</a>|5|<p>", Render(tpl, params, true));
    EXPECT_EQ("<a href='x'>&</a>|<a href='x'>&</a>|5|<p>", Render(tpl, params, false));
}

// The branch an inline if picks renders itself (docs/tasks/0100), escaped all the same
TEST(AutoescapeTest, EscapesInlineIfBranches)
{
    ValuesMap params{ { "html", "<b>" }, { "n", 5 } };
    const std::string tpl = "{{ html if n > 3 else 'x' }}|{{ 'x' if n > 9 else html }}|{{ html|safe if n else html }}|{{ html is string }}|{{ html if n > 9 }}|";
    EXPECT_EQ("&lt;b&gt;|&lt;b&gt;|<b>|True||", Render(tpl, params, true));
    EXPECT_EQ("<b>|<b>|<b>|True||", Render(tpl, params, false));
}

TEST(AutoescapeTest, EscapesWideTemplates)
{
    ValuesMap params{ { "html", std::wstring(L"<é>") } };
    EXPECT_EQ(L"&lt;é&gt;|<é>", Render<wchar_t>(L"{{ html }}|{{ html|safe }}", params, true));
}

// Strings of the template's width are escaped from their own text, others after rendering
// them (docs/tasks/0126): every representation and width escapes alike
TEST(AutoescapeTest, EscapesEveryStringKind)
{
    const std::string longText(5000, '<');
    std::string longEscaped;
    for (size_t n = 0; n != longText.size(); ++n)
    {
        longEscaped += "&lt;";
    }
    ValuesMap params{ { "narrow", "<a&>" }, { "wide", std::wstring(L"<e>") }, { "long", longText }, { "n", 5 } };
    const std::string tpl = "{% macro f() %}<{{ narrow }}{% endmacro %}{{ narrow }}|{{ wide }}|{{ '<'+narrow }}|{{ narrow|upper }}|{{ narrow|escape }}|"
                            "{{ narrow|e|e }}|{{ n }}|{{ f() }}|{{ narrow ~ n }}|{{ long }}";
    EXPECT_EQ("&lt;a&amp;&gt;|&lt;e&gt;|&lt;&lt;a&amp;&gt;|&lt;A&amp;&gt;|&lt;a&amp;&gt;|&lt;a&amp;&gt;|5|<&lt;a&amp;&gt;|&lt;a&amp;&gt;5|" + longEscaped,
              Render(tpl, params, true));
    const std::wstring wtpl = L"{% macro f() %}<{{ wide }}{% endmacro %}{{ narrow }}|{{ wide }}|{{ '<'+narrow }}|{{ narrow|upper }}|"
                              L"{{ narrow|escape }}|{{ n }}|{{ f() }}|{{ long }}";
    EXPECT_EQ(L"&lt;a&amp;&gt;|&lt;e&gt;|&lt;&lt;a&amp;&gt;|&lt;A&amp;&gt;|&lt;a&amp;&gt;|5|<&lt;e&gt;|" + std::wstring(longEscaped.begin(), longEscaped.end()),
              Render<wchar_t>(wtpl, params, true));
}

TEST(AutoescapeTest, BlockOverridesSetting)
{
    ValuesMap params{ { "html", "<b>" } };
    const std::string tpl = "{% autoescape false %}{{ html }}{% endautoescape %}{% autoescape true %}{{ html }}{% endautoescape %}{{ html }}";
    EXPECT_EQ("<b>&lt;b&gt;&lt;b&gt;", Render(tpl, params, true));
    EXPECT_EQ("<b>&lt;b&gt;<b>", Render(tpl, params, false));
}

TEST(AutoescapeTest, BlockSyntaxErrors)
{
    EXPECT_EQ(std::string("noname.j2tpl:1:5: error: Unexpected statement: 'endautoescape'\nx{% endautoescape %}\n ---^-------"),
              ParseError(std::string("x{% endautoescape %}")));
    EXPECT_FALSE(ParseError(std::string("{% autoescape %}x{% endautoescape %}")).empty());
    EXPECT_FALSE(ParseError(std::string("{% autoescape true %}x")).empty());
    EXPECT_FALSE(ParseError(std::string("{% autoescape true %}x{% endautoescape true %}")).empty());
}

namespace
{
// Templates by name, as Jinja2's DictLoader; each case below was rendered with Jinja2 3.1.6
// and `autoescape=select_autoescape(['html', 'xml'])`
std::shared_ptr<MemoryFileSystem> SelectorFiles()
{
    auto fs = std::make_shared<MemoryFileSystem>();
    const std::pair<const char*, const char*> files[] = {
        { "a.html", "{{ x }}" },
        { "a.txt", "{{ x }}" },
        { "inc.html", R"([{% include "a.txt" %}|{% include "a.html" %}])" },
        { "inc.txt", R"([{% include "a.txt" %}|{% include "a.html" %}])" },
        { "base.html", "B{{ x }}{% block b %}{{ x }}{% endblock %}" },
        { "child.txt", R"({% extends "base.html" %}{% block b %}C{{ x }}{% endblock %})" },
        { "base.txt", "B{{ x }}{% block b %}{{ x }}{% endblock %}" },
        { "child.html", R"({% extends "base.txt" %}{% block b %}C{{ x }}{% endblock %})" },
        { "mac.html", "{% macro m() %}{{ x }}{% endmacro %}{% macro k() %}{% set s %}<{% endset %}{{ s }}{% endmacro %}" },
        { "mac.txt", "{% macro m() %}{{ x }}{% endmacro %}" },
        { "imp.txt", R"({% from "mac.html" import m with context %}{{ m() }}|{% set s %}{{ x }}{% endset %}{{ s }})" },
        { "imp2.txt", R"({% import "mac.html" as M %}{{ M.k() }})" },
        { "imp3.html", R"({% import "mac.txt" as M with context %}{{ M.m() }})" },
        { "child2.txt", R"({% extends "base.html" %}{% block b %}{% set z %}{{ x }}{% endset %}[{{ z }}]{{ "<a>"|join }}{% endblock %})" },
        { "base3.html", R"({% set j = ["<"]|join %}{{ j }}{% block b %}{% endblock %})" },
        { "c3.txt", R"({% extends "base3.html" %}{% block b %}{{ ["<"]|join }}{% endblock %})" },
        { "UP.HTML", "{{ x }}" },
        { "n.HtMl.txt", "{{ x }}" },
        { "noext", "{{ x }}" },
        { "x.htm", "{{ x }}" },
        { "x.xml", "{{ x }}" },
    };
    for (const auto& [name, source] : files)
    {
        fs->AddFile(name, std::string(source));
    }
    return fs;
}

std::string RenderNamed(TemplateEnv& env, const std::string& name)
{
    auto tpl = env.LoadTemplate(name);
    EXPECT_TRUE(!!tpl) << name;
    if (!tpl)
    {
        return {};
    }
    auto result = tpl.value().RenderAsString({ { "x", "<b>" } });
    EXPECT_TRUE(!!result) << name;
    return result ? result.value() : std::string();
}
} // namespace

TEST(AutoescapeSelectorTest, ChoosesByTemplateName)
{
    TemplateEnv env;
    env.GetSettings().autoescapeSelector = SelectAutoescape({ "html", "xml" });
    env.AddFilesystemHandler({}, SelectorFiles());

    const std::pair<const char*, const char*> cases[] = {
        { "a.html", "&lt;b&gt;" },
        { "a.txt", "<b>" },
        { "inc.html", "[<b>|&lt;b&gt;]" },
        { "inc.txt", "[<b>|&lt;b&gt;]" },
        // Each template's code escapes by its own name, the blocks too
        { "child.txt", "B&lt;b&gt;C<b>" },
        { "child.html", "B<b>C&lt;b&gt;" },
        { "imp.txt", "&lt;b&gt;|<b>" },
        // An imported macro runs with its module's runtime setting: the set block is Markup
        { "imp2.txt", "<" },
        // and its result is Markup by the caller's
        { "imp3.html", "<b>" },
        { "child2.txt", "B&lt;b&gt;[<b>]<a>" },
        // A parent template runs with its child's runtime setting: `join` is a plain str there
        { "c3.txt", "&lt;<" },
        { "UP.HTML", "&lt;b&gt;" },
        { "n.HtMl.txt", "<b>" },
        { "noext", "<b>" },
        { "x.htm", "<b>" },
        { "x.xml", "&lt;b&gt;" },
    };
    for (const auto& [name, expected] : cases)
    {
        EXPECT_EQ(expected, RenderNamed(env, name)) << name;
    }

    // A template from a string has no name: `default_for_string`
    auto fromString = env.FromString("{{ x }}");
    ASSERT_TRUE(!!fromString);
    EXPECT_EQ("&lt;b&gt;", fromString.value().RenderAsString({ { "x", "<b>" } }).value());
}

TEST(AutoescapeSelectorTest, DisabledExtensionsAndDefaults)
{
    TemplateEnv env;
    env.GetSettings().autoescapeSelector = SelectAutoescape({ "html" }, { ".TXT" }, false, true);
    env.AddFilesystemHandler({}, SelectorFiles());
    EXPECT_EQ("<b>", RenderNamed(env, "a.txt"));
    EXPECT_EQ("&lt;b&gt;", RenderNamed(env, "noext"));
    EXPECT_EQ("&lt;b&gt;", RenderNamed(env, "x.xml"));

    Template tpl(&env);
    ASSERT_TRUE(!!tpl.Load("{{ x }}"));
    EXPECT_EQ("<b>", tpl.RenderAsString({ { "x", "<b>" } }).value());
}

TEST(AutoescapeSelectorTest, CallableSeesTheName)
{
    std::vector<std::string> names;
    TemplateEnv env;
    env.GetSettings().autoescapeSelector = [&names](const std::string& name) {
        names.push_back(name);
        return name == "a.txt";
    };
    env.AddFilesystemHandler({}, SelectorFiles());
    EXPECT_EQ("<b>", RenderNamed(env, "a.html"));
    EXPECT_EQ("&lt;b&gt;", RenderNamed(env, "a.txt"));

    TemplateW tpl(&env);
    ASSERT_TRUE(!!tpl.Load(L"{{ x }}", "named.txt"));
    EXPECT_EQ(L"<b>", tpl.RenderAsString({ { "x", "<b>" } }).value());
    const std::vector<std::string> expected{ "a.html", "a.txt", "named.txt" };
    EXPECT_EQ(expected, names);
}

TEST(AutoescapeSelectorTest, SelectorTakesPartInEquality)
{
    Settings selecting;
    selecting.autoescapeSelector = SelectAutoescape();
    EXPECT_TRUE(selecting != Settings());
    const Settings copy = selecting;
    EXPECT_TRUE(copy == selecting);
    Settings other;
    other.autoescapeSelector = SelectAutoescape();
    EXPECT_TRUE(other != selecting);
}

TEST(AutoescapeSelectorTest, ThrowingSelectorFailsLoad)
{
    TemplateEnv env;
    env.GetSettings().autoescapeSelector = [](const std::string&) -> bool { throw std::runtime_error("boom"); };
    auto tpl = env.FromString("x");
    ASSERT_FALSE(!!tpl);
    EXPECT_EQ(ErrorCode::UnexpectedException, tpl.error().GetCode());
    EXPECT_THROW(AutoescapeSelector()("a.html"), std::bad_function_call);
}
