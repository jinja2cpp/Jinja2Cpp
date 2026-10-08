#include "test_tools.h"

#include <jinja2cpp/error_info.h>
#include <jinja2cpp/template.h>
#include <jinja2cpp/template_env.h>

#include <gtest/gtest.h>

#include <string>
#include <type_traits>

using namespace jinja2;

namespace
{
template<typename CharT>
nonstd::expected<std::basic_string<CharT>, BasicErrorInfo<CharT>> RenderWithPolicy(const std::basic_string<CharT>& source, UndefinedPolicy policy)
{
    TemplateEnv env;
    env.GetSettings().undefinedPolicy = policy;
    std::conditional_t<std::is_same_v<CharT, char>, Template, TemplateW> tpl(&env);
    auto loaded = tpl.Load(source);
    if (!loaded)
        return nonstd::make_unexpected(loaded.error());
    return tpl.RenderAsString(ValuesMap{ { "d", ValuesMap{ { "a", 1 } } }, { "n", Value() } });
}

std::string Render(const std::string& source, UndefinedPolicy policy = UndefinedPolicy::Default)
{
    auto result = RenderWithPolicy(source, policy);
    return result ? result.value() : "error: " + result.error().GetExtraParams()[0].get<std::string>();
}
} // namespace

TEST(UndefinedPolicyTest, DefaultIsPythonUndefined)
{
    EXPECT_EQ(Settings().undefinedPolicy, UndefinedPolicy::Default);
    EXPECT_EQ("[]0[]", Render("[{{ nope }}]{{ nope|length }}{{ nope|list }}"));
    EXPECT_EQ("[][]", Render("[{{ d.zz }}][{{ n.a }}]"));
    EXPECT_EQ("error: 'nope' is undefined", Render("{{ nope.a }}"));
    EXPECT_EQ("error: 'nope' is undefined", Render("{{ nope() }}"));
    EXPECT_EQ("error: 'dict object' has no attribute 'zz'", Render("{{ d.zz.yy }}"));
    EXPECT_EQ("error: 'dict object' has no attribute 'zz'", Render("{% set x = d.zz %}{{ x['k'] }}"));
    EXPECT_EQ("error: 'None' has no attribute 'a'", Render("{{ n.a.b }}"));
    EXPECT_EQ("error: parameter 'a' was not provided", Render("{% macro m(a) %}{{ a.b }}{% endmacro %}{{ m() }}"));
}

TEST(UndefinedPolicyTest, ErrorCode)
{
    auto result = RenderWithPolicy<char>("{{ nope.a }}", UndefinedPolicy::Default);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(ErrorCode::UndefinedError, result.error().GetCode());
    EXPECT_EQ(0U, result.error().ToString().find("noname.j2tpl:1:1: error: Undefined value: 'nope' is undefined"));

    auto wide = RenderWithPolicy<wchar_t>(L"{{ nope.a }}", UndefinedPolicy::Strict);
    ASSERT_FALSE(wide.has_value());
    EXPECT_EQ(ErrorCode::UndefinedError, wide.error().GetCode());
}

TEST(UndefinedPolicyTest, Strict)
{
    const auto policy = UndefinedPolicy::Strict;
    EXPECT_EQ("error: 'nope' is undefined", Render("{{ nope }}", policy));
    EXPECT_EQ("error: 'nope' is undefined", Render("{% if nope %}{% endif %}", policy));
    EXPECT_EQ("error: 'nope' is undefined", Render("{% for i in nope %}{% endfor %}", policy));
    EXPECT_EQ("error: 'nope' is undefined", Render("{{ nope == 1 }}", policy));
    EXPECT_EQ("error: 'a_long_undefined_name_x' is undefined", Render("{{ a_long_undefined_name_x }}", policy));
    EXPECT_EQ("error: 'dict object' has no attribute 'zz'", Render("{{ d.zz ~ 'x' }}", policy));
    EXPECT_EQ("FalseTrue|d|1", Render("{{ nope is defined }}{{ nope is undefined }}|{{ nope|default('d') }}|{{ d.a }}", policy));
}

TEST(UndefinedPolicyTest, Chainable)
{
    const auto policy = UndefinedPolicy::Chainable;
    EXPECT_EQ("[][]d", Render("[{{ nope.a.b }}][{{ d.zz['k'].c }}]{{ nope.a|default('d') }}", policy));
    EXPECT_EQ("error: 'nope' is undefined", Render("{{ nope.a() }}", policy));
}

TEST(UndefinedPolicyTest, Debug)
{
    const auto policy = UndefinedPolicy::Debug;
    EXPECT_EQ("{{ nope }}|{{ no such element: dict object['zz'] }}|a{{ nope }}", Render("{{ nope }}|{{ d.zz }}|{{ 'a' ~ nope }}", policy));
    EXPECT_EQ("error: 'nope' is undefined", Render("{{ nope.a }}", policy));

    auto wide = RenderWithPolicy<wchar_t>(L"{{ nope }}", policy);
    ASSERT_TRUE(wide.has_value());
    EXPECT_EQ(L"{{ nope }}", wide.value());
}

TEST(UndefinedPolicyTest, SettingsCompare)
{
    Settings strict;
    strict.undefinedPolicy = UndefinedPolicy::Strict;
    EXPECT_FALSE(Settings() == strict);
}
