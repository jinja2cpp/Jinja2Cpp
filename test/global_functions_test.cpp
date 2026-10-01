#include <cctype>
#include <string>

#include "gtest/gtest.h"

#include "jinja2cpp/template.h"
#include "jinja2cpp/template_env.h"
#include "jinja2cpp/user_callable.h"

using namespace jinja2;

namespace
{
std::string Render(const std::string& source, TemplateEnv* env = nullptr)
{
    Template tpl(env);
    auto loaded = tpl.Load(source);
    if (!loaded)
        return "Load error: " + loaded.error().ToString();
    auto result = tpl.RenderAsString(ValuesMap{});
    if (!result)
        return "Render error: " + result.error().ToString();
    return result.value();
}

size_t CountOf(const std::string& text, const std::string& what)
{
    size_t count = 0;
    for (auto pos = text.find(what); pos != std::string::npos; pos = text.find(what, pos + what.size()))
        ++count;
    return count;
}
} // namespace

// The builtin globals are defaults: a global of the same name set on the environment wins
TEST(GlobalFunctionsTest, EnvironmentGlobalOverridesBuiltin)
{
    TemplateEnv env;
    env.AddGlobal("cycler", MakeCallable([]() { return std::string("own cycler"); }));
    env.AddGlobal("range", MakeCallable([]() { return std::string("own range"); }));

    EXPECT_EQ("own cycler|own range", Render("{{ cycler() }}|{{ range() }}", &env));
    EXPECT_EQ("ab", Render("{% set j = joiner('') %}{{ j() }}a{{ j() }}b", &env));
}

// lipsum is random in Jinja2, so the parity corpus can only check its shape; check the
// rest of it here
TEST(GlobalFunctionsTest, LipsumShape)
{
    auto html = Render("{{ lipsum() }}");
    EXPECT_EQ(5u, CountOf(html, "<p>"));
    EXPECT_EQ(5u, CountOf(html, ".</p>"));
    EXPECT_EQ(4u, CountOf(html, "</p>\n<p>"));
    EXPECT_TRUE(std::isupper(static_cast<unsigned char>(html[3]))) << html;

    auto plain = Render("{{ lipsum(n=2, html=False, min=3, max=4) }}");
    EXPECT_EQ(0u, CountOf(plain, "<p>"));
    EXPECT_EQ(1u, CountOf(plain, ".\n\n"));
    // two paragraphs of three words each
    EXPECT_EQ(4u, CountOf(plain, " ")) << plain;
    EXPECT_EQ('.', plain.back());

    EXPECT_EQ(html, Render("{{ lipsum() }}"));
    EXPECT_NE(std::string::npos, Render("{{ lipsum(1, min=5, max=5) }}").find("error")) << "min must be less than max";
}

TEST(GlobalFunctionsTest, StatefulObjectsAreSharedByCopies)
{
    EXPECT_EQ("a,b,a", Render("{% set c = cycler('a', 'b') %}{% set d = c %}{{ c.next() }},{{ d.next() }},{{ c.next() }}"));
    EXPECT_EQ("x;y", Render("{% set j = joiner(';') %}{% set k = j %}{{ j() }}x{{ k() }}y"));
}

TEST(GlobalFunctionsTest, ExtremeArguments)
{
    // more than INT64_MAX items: Python's len() raises OverflowError
    EXPECT_NE(std::string::npos, Render("{{ range(-9223372036854775807, 9223372036854775807)|length }}").find("error"));
    EXPECT_NE(std::string::npos, Render("{{ range(1, 2, 3, 4) }}").find("error"));
    // the span of the word count does not overflow; the count drawn here is negative, so the
    // paragraph is empty and only gets its full stop
    EXPECT_EQ(".", Render("{{ lipsum(1, False, -9223372036854775807, 2) }}"));
}
