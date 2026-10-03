#include "gtest/gtest.h"

#include <jinja2cpp/filesystem_handler.h>
#include <jinja2cpp/template.h>
#include <jinja2cpp/template_env.h>
#include <jinja2cpp/user_callable.h>

#include <cctype>
#include <cstddef>
#include <string>

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
    EXPECT_EQ(5U, CountOf(html, "<p>"));
    EXPECT_EQ(5U, CountOf(html, ".</p>"));
    EXPECT_EQ(4U, CountOf(html, "</p>\n<p>"));
    EXPECT_TRUE(std::isupper(static_cast<unsigned char>(html[3]))) << html;

    auto plain = Render("{{ lipsum(n=2, html=False, min=3, max=4) }}");
    EXPECT_EQ(0U, CountOf(plain, "<p>"));
    EXPECT_EQ(1U, CountOf(plain, ".\n\n"));
    // two paragraphs of three words each
    EXPECT_EQ(4U, CountOf(plain, " ")) << plain;
    EXPECT_EQ('.', plain.back());

    EXPECT_EQ(html, Render("{{ lipsum() }}"));
    EXPECT_NE(std::string::npos, Render("{{ lipsum(1, min=5, max=5) }}").find("error")) << "min must be less than max";
}

// The builtins are one table shared by all renders (docs/tasks/0104): imported templates see
// them, a template variable shadows them, and lipsum's generator belongs to the render, so an
// included template continues its sequence and the next render starts it afresh
TEST(GlobalFunctionsTest, BuiltinsAreSharedByTheRender)
{
    auto fs = std::make_shared<MemoryFileSystem>();
    fs->AddFile("m.j2", "{% macro r(n) %}{{ range(n)|list }}{% endmacro %}");
    fs->AddFile("l.j2", "{{ lipsum(1, False, 3, 4) }}");
    TemplateEnv env;
    env.AddFilesystemHandler({}, fs);

    EXPECT_EQ("[0, 1]", Render("{% import 'm.j2' as m %}{{ m.r(2) }}", &env));
    EXPECT_EQ("5", Render("{% set range = 5 %}{{ range }}", &env));

    auto twice = Render("{% include 'l.j2' %}|{% include 'l.j2' %}", &env);
    auto bar = twice.find('|');
    ASSERT_NE(std::string::npos, bar) << twice;
    EXPECT_NE(twice.substr(0, bar), twice.substr(bar + 1));
    EXPECT_EQ(twice, Render("{% include 'l.j2' %}|{% include 'l.j2' %}", &env));
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

// A two-character string is a pair; characters are code points, not UTF-8 bytes. Narrow only:
// the parity corpus cannot hold this case, because a wide key goes through the
// locale-dependent ConvertString (task 0035) and the wide result differs by platform.
TEST(GlobalFunctionsTest, DictFromUnicodeStringPairs)
{
    EXPECT_EQ("{'\xC3\xA9': '\xE4\xB8\xAD'}|\xE4\xB8\xAD"
              "1",
              Render("{% set d = dict(['\xC3\xA9\xE4\xB8\xAD']) %}{{ d }}|{{ d['\xC3\xA9'] }}{{ d|length }}"));
    EXPECT_NE(std::string::npos, Render("{{ dict(['\xC3\xA9\xE4\xB8\xADx']) }}").find("error"));
}
