#include <string>

#include "gtest/gtest.h"

#include "jinja2cpp/template.h"
#include "jinja2cpp/template_env.h"

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

TEST(AutoescapeTest, EscapesWideTemplates)
{
    ValuesMap params{ { "html", std::wstring(L"<é>") } };
    EXPECT_EQ(L"&lt;é&gt;|<é>", Render<wchar_t>(L"{{ html }}|{{ html|safe }}", params, true));
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
