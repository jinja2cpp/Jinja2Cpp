#include "../src/template_impl.h"
#include "test_tools.h"

#include <jinja2cpp/error_info.h>
#include <jinja2cpp/template.h>
#include <jinja2cpp/template_env.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

using namespace jinja2;

// The internal walk behind the Python module's jinja2.meta.find_referenced_templates
// (docs/tasks/0354) against Jinja2 3.1.6's: the names of `extends`, `include`, `import` and
// `from` in source order, "?" for a name that is no constant
namespace
{
struct ReferencedCase
{
    const char* source;
    const char* names;
};

std::vector<std::optional<std::string>> SplitNames(std::string_view names)
{
    std::vector<std::optional<std::string>> result;
    while (!names.empty())
    {
        const auto end = std::min(names.find(' '), names.size());
        const auto name = names.substr(0, end);
        result.push_back(name == "?" ? std::nullopt : std::optional<std::string>(std::string(name)));
        names.remove_prefix(std::min(end + 1, names.size()));
    }
    return result;
}

const ReferencedCase referencedCases[] = {
    // clang-format off
    { "{% extends 'base.html' %}{% include 'a.html' %}", "base.html a.html" },
    { "{% include 'a' %}{% import 'b' as b %}{% from 'c' import d %}", "a b c" },
    { "{% include ['a','b'] %}", "a b" },
    { "{% include ('a', 'b') %}", "a b" },
    { "{% include name %}{% include 'x' %}", "? x" },
    { "{% include 'a' ~ 'b' %}", "?" },
    { "{% include 'a' if x else 'b' %}", "?" },
    { "{% include 1 %}", "?" },
    { "{% if x %}{% include 'a' %}{% else %}{% include 'b' %}{% endif %}", "a b" },
    { "{% macro m() %}{% include 'in-macro' %}{% endmacro %}{% for i in x %}{% from 'lp' import q %}{% endfor %}", "in-macro lp" },
    { "{% include 'a' ignore missing without context %}{% import 'b' as b with context %}", "a b" },
    { "no references {{ 1 }}", "" },
    // clang-format on
};
} // namespace

class ReferencedTemplatesTest : public ::testing::TestWithParam<ReferencedCase>
{
};

TEST_P(ReferencedTemplatesTest, MatchesJinja2)
{
    TemplateEnv env;
    Template tpl(&env);
    ASSERT_TRUE(tpl.Load(GetParam().source).has_value());
    auto found = TemplateImpl<char>::ReferencedTemplatesOf(tpl);
    ASSERT_TRUE(found.has_value()) << ErrorToString(found.error());
    EXPECT_EQ(SplitNames(GetParam().names), *found) << GetParam().source;
}

INSTANTIATE_TEST_SUITE_P(Cases, ReferencedTemplatesTest, ::testing::ValuesIn(referencedCases));

TEST(ReferencedTemplatesNotLoadedTest, ReportsTheError)
{
    TemplateEnv env;
    Template tpl(&env);
    auto found = TemplateImpl<char>::ReferencedTemplatesOf(tpl);
    ASSERT_FALSE(found.has_value());
    EXPECT_EQ(ErrorCode::TemplateNotParsed, found.error().GetCode());
}
