#include "gtest/gtest.h"

#include <jinja2cpp/template.h>
#include <jinja2cpp/template_env.h>
#include <jinja2cpp/value.h>

#include <string>

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
// them (docs/tasks/0126): every representation and width escapes alike. The values are ASCII:
// a non-ASCII one does not convert between widths in the C locale (docs/tasks/0035)
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
