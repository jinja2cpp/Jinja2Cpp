// The output writer: values written without ValueRenderer, the buffer between the
// render and its string, and long fragments that skip it (docs/tasks/0113). Every case
// renders through RenderAsString, RenderTo a string and RenderTo a sink (docs/tasks/0257)

#include "gtest/gtest.h"

#include <jinja2cpp/exception.h>
#include <jinja2cpp/filesystem_handler.h>
#include <jinja2cpp/template.h>
#include <jinja2cpp/template_env.h>
#include <jinja2cpp/user_callable.h>
#include <jinja2cpp/value.h>

#include <fmt/format.h>

#include <cstddef>
#include <cstdint>
#include <exception>
#include <memory>
#include <new>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

using namespace jinja2;

namespace
{
std::wstring Widen(const std::string& str)
{
    return { str.begin(), str.end() };
}

// Keeps every chunk a render writes, and checks the sink contract on each
template<typename CharT>
class RecordingSink final : public BasicOutputSink<CharT>
{
public:
    void Write(std::basic_string_view<CharT> text) override
    {
        // Only a render that writes each piece writes an empty one, an item Jinja2's generate() yields
        EXPECT_TRUE(eachPiece || !text.empty());
        chunks.emplace_back(text);
        output.append(text);
    }

    bool eachPiece = false;
    std::vector<std::basic_string<CharT>> chunks;
    std::basic_string<CharT> output;
};

// Renders `tpl` with RenderAsString, RenderTo a string that holds a prefix already, and RenderTo
// a sink, and checks that all three write `expected`
template<typename CharT>
void ExpectRenderPaths(const BasicTemplate<CharT>& tpl, const std::basic_string<CharT>& expected, const ValuesMap& params)
{
    auto asString = tpl.RenderAsString(params);
    ASSERT_TRUE(asString);
    EXPECT_EQ(expected, asString.value());

    std::basic_string<CharT> appended(3, CharT('>'));
    ASSERT_TRUE(tpl.RenderTo(appended, params));
    EXPECT_EQ(std::basic_string<CharT>(3, CharT('>')) + expected, appended);

    RecordingSink<CharT> sink;
    ASSERT_TRUE(tpl.RenderTo(sink, params));
    EXPECT_EQ(expected, sink.output);
}

// Renders `tpl` narrow and wide, twice each (the second render reserves from the first), and
// checks the output
void ExpectRender(const std::string& tpl, const std::string& expected, const ValuesMap& params = {})
{
    Template narrow;
    ASSERT_TRUE(narrow.Load(tpl));
    TemplateW wide;
    ASSERT_TRUE(wide.Load(Widen(tpl)));
    for (int pass = 0; pass != 2; ++pass)
    {
        ExpectRenderPaths(narrow, expected, params);
        ExpectRenderPaths(wide, Widen(expected), params);
    }
}
} // namespace

TEST(OutputTest, Integers)
{
    ExpectRender("{{ 0 }} {{ 7 }} {{ 10 }} {{ 99 }} {{ 100 }} {{ 12345 }} {{ -1 }} {{ -10 }} {{ -101 }}", "0 7 10 99 100 12345 -1 -10 -101");
    ExpectRender("{{ 9223372036854775807 }} {{ -9223372036854775807 - 1 }}", "9223372036854775807 -9223372036854775808");
    ValuesMap params{ { "small", int64_t{ 5 } }, { "big", INT64_MIN }, { "list", ValuesList{ int64_t{ 42 }, int64_t{ -3 } } } };
    ExpectRender("{{ small }} {{ big }} {{ list[0] }}{{ list[1] }}", "5 -9223372036854775808 42-3", params);
}

TEST(OutputTest, OtherValues)
{
    // Booleans have their own path; the rest go through ValueRenderer after the buffer
    ExpectRender("a{{ true }}b{{ false }}c{{ none }}d{{ 1.5 }}e{{ [1, 'x'] }}f{{ {'k': 2} }}g{{ undefined_name }}h",
                 "aTruebFalsecNoned1.5e[1, 'x']f{'k': 2}gh");
    ValuesMap params{ { "flag", true }, { "real", 2.25 }, { "nothing", Value() } };
    ExpectRender("{{ flag }}|{{ real }}|{{ nothing }}|", "True|2.25|None|", params);
}

TEST(OutputTest, Strings)
{
    ValuesMap params{ { "str", std::string("text") }, { "view", std::string_view("view") }, { "empty", std::string() } };
    ExpectRender("<{{ str }}><{{ view }}><{{ empty }}><{{ 'literal' }}><{{ str ~ '!' }}><{{ str | upper }}>",
                 "<text><view><><literal><text!><TEXT>",
                 params);
}

TEST(OutputTest, OutputLongerThanTheBuffer)
{
    std::string expected;
    for (int idx = 0; idx != 2000; ++idx)
    {
        expected += "<td>" + std::to_string(idx) + "</td>";
    }
    ExpectRender("{% for i in range(2000) %}<td>{{ i }}</td>{% endfor %}", expected);
}

TEST(OutputTest, LongFragments)
{
    // Text fragments around the buffer and the long-fragment thresholds, between values
    // that sit in the buffer when the long text arrives
    std::string tpl;
    std::string expected;
    for (size_t length : { 1U, 15U, 16U, 17U, 127U, 128U, 129U, 511U, 512U, 513U, 2000U })
    {
        const std::string text(length, static_cast<char>('a' + (length % 26)));
        tpl += "{{ " + std::to_string(length) + " }}" + text;
        expected += std::to_string(length) + text;
    }
    ExpectRender(tpl, expected);
    ExpectRender(tpl + "{% set captured %}" + tpl + "{% endset %}[{{ captured }}]", expected + "[" + expected + "]");
}

TEST(OutputTest, CapturedOutput)
{
    // Blocks rendered to a string of their own: set blocks, macros, call blocks, filter blocks
    const std::string tpl = R"({% set s %}{% for i in range(300) %}{{ i }},{% endfor %}{% endset %}{{ s | length }}
{% macro m(n) %}{% for i in range(n) %}{{ i }}{% endfor %}{% endmacro %}{{ m(200) | length }}
{% macro wrap() %}[{{ caller() }}]{% endmacro %}{% call wrap() %}{{ m(3) }}{% endcall %}
{% filter upper %}{% for i in range(100) %}ab{{ i }}{% endfor %}{% endfilter %})";
    std::string filtered;
    for (int idx = 0; idx != 100; ++idx)
    {
        filtered += "AB" + std::to_string(idx);
    }
    ExpectRender(tpl, "1090\n490\n[012]\n" + filtered);
}

TEST(OutputTest, IntegersAtTheBufferEnd)
{
    // Numbers and booleans written straight into the buffer, narrow and wide, when it has
    // only a few characters of room left (docs/tasks/0132). The padding is short values:
    // text of LongLength characters or more would flush the buffer first
    std::string tpl;
    std::string expected;
    for (size_t pad = 470; pad != 531; ++pad)
    {
        for (size_t idx = 0; idx != pad / 10; ++idx)
        {
            tpl += "{{ 'xxxxxxxxxx' }}";
        }
        for (size_t idx = 0; idx != pad % 10; ++idx)
        {
            tpl += "{{ 'x' }}";
        }
        tpl += "{{ -9223372036854775807 - 1 }}{{ 12 }}{{ 7 }}{{ true }}|";
        expected += std::string(pad, 'x') + "-9223372036854775808127True|";
    }
    ExpectRender(tpl, expected);
}

namespace
{
// A sink that fails on its `failAt`th chunk, with what `fail` throws
class ThrowingSink final : public OutputSink
{
public:
    ThrowingSink(size_t failAt, void (*fail)())
        : m_failAt(failAt)
        , m_fail(fail)
    {
    }

    void Write(std::string_view text) override
    {
        if (++m_calls == m_failAt)
        {
            m_fail();
        }
        output.append(text);
    }

    std::string output;

private:
    size_t m_calls = 0;
    size_t m_failAt;
    void (*m_fail)();
};

// 2000 numbers, then a call of `fail`, which throws
constexpr const char* failingTemplate = "{% for i in range(2000) %}{{ i }},{% endfor %}{{ fail() }}tail";

std::string Numbers(int count)
{
    std::string out;
    for (int idx = 0; idx != count; ++idx)
    {
        out += std::to_string(idx) + ",";
    }
    return out;
}

ValuesMap FailingParams()
{
    return { { "fail", MakeCallable([]() -> Value { throw std::runtime_error("callable failed"); }) } };
}
} // namespace

TEST(OutputTest, SinkGetsBufferedChunks)
{
    // Short fragments are gathered into chunks of the buffer's size: 512 characters at most
    Template tpl;
    ASSERT_TRUE(tpl.Load("{% for i in range(2000) %}<td>{{ i }}</td>{% endfor %}"));
    RecordingSink<char> sink;
    ASSERT_TRUE(tpl.RenderTo(sink, {}));
    EXPECT_GT(sink.chunks.size(), 20U);
    for (const auto& chunk : sink.chunks)
    {
        EXPECT_LE(chunk.size(), 512U);
    }

    TemplateW wide;
    ASSERT_TRUE(wide.Load(L"{% for i in range(2000) %}<td>{{ i }}</td>{% endfor %}"));
    RecordingSink<wchar_t> wideSink;
    ASSERT_TRUE(wide.RenderTo(wideSink, {}));
    EXPECT_EQ(sink.chunks.size(), wideSink.chunks.size());
    for (const auto& chunk : wideSink.chunks)
    {
        EXPECT_LE(chunk.size(), 512U);
    }
}

TEST(OutputTest, SinkGetsEachPieceWhenAsked)
{
    // RenderOptions::writeEachPiece: one Write for each piece, Jinja2's generate() items (docs/tasks/0436); a
    // macro's or a call block's output is one piece, as the value its call returns
    const char* source = "<ul>{% for item in seq %}<li>{{ loop.index }} - {{ item }}</li>{%- endfor %}</ul>"
                         "{% macro m(a) %}[{{ a }}]{% endmacro %}{{ m(1) }}{% macro c() %}[{{ caller() }}]{% endmacro %}{% call c() %}{{ 2 }}{% endcall %}|{% set s %}a{{ 1 }}b{% endset %}{{ s }}";
    const std::vector<std::string> expected = { "<ul>", "<li>", "1", " - ", "0", "</li>", "<li>", "2", " - ", "1", "</li>", "</ul>", "[1]", "[2]", "|", "a1b" };
    const ValuesMap params = { { "seq", ValuesList{ 0, 1 } } };
    RenderOptions options;
    options.writeEachPiece = true;

    Template tpl;
    ASSERT_TRUE(tpl.Load(source));
    RecordingSink<char> sink;
    ASSERT_TRUE(tpl.RenderTo(sink, params, options));
    EXPECT_EQ(sink.chunks, expected);

    TemplateW wide;
    const std::string narrow = source;
    ASSERT_TRUE(wide.Load(std::wstring(narrow.begin(), narrow.end())));
    RecordingSink<wchar_t> wideSink;
    ASSERT_TRUE(wide.RenderTo(wideSink, params, options));
    ASSERT_EQ(wideSink.chunks.size(), expected.size());
    for (std::size_t idx = 0; idx != expected.size(); ++idx)
    {
        EXPECT_EQ(wideSink.chunks[idx], std::wstring(expected[idx].begin(), expected[idx].end())) << idx;
    }

    // A render to a string ignores it, and the default gathers the pieces
    std::string text;
    ASSERT_TRUE(tpl.RenderTo(text, params, options));
    EXPECT_EQ(text, tpl.RenderAsString(params).value());
    RecordingSink<char> gathered;
    ASSERT_TRUE(tpl.RenderTo(gathered, params));
    EXPECT_EQ(gathered.chunks.size(), 1U);
}

TEST(OutputTest, SinkGetsThePiecesJinja2Folds)
{
    // docs/tasks/0456, Jinja2 3.1.6's generate(): its compiler folds constant output into the text next to it, a
    // value it evaluates is an item even when empty, and super(), self.b(), a recursive loop and a macro's output
    // are one item however long
    const std::string longMacro = "{% macro m() %}{% for i in range(300) %}ab{{ i }}{% endfor %}{% endmacro %}{{ m() }}";
    std::string longOutput;
    for (int idx = 0; idx != 300; ++idx)
    {
        longOutput += "ab" + std::to_string(idx);
    }
    const std::vector<std::pair<std::string, std::vector<std::string>>> rows = {
        { "a{{ 1 }}b", { "a1b" } },
        { "{{ none }}{{ true }}{{ 1.5 }}", { "NoneTrue1.5" } },
        { "a{# c #}b", { "ab" } },
        { "a{% raw %}r{% endraw %}b", { "arb" } },
        { "a{{ 1 + 2 }}b{{ 'q'|upper }}c", { "a3bQc" } },
        { "{{ 1 if true else y }}a", { "1a" } },
        { "a{{ y }}b{{ 1 }}c", { "a", "", "b1c" } },
        { "{% for i in [1, 2] %}a{{ 1 }}b{% endfor %}", { "a1b", "a1b" } },
        { "{% if 1 %}a{{ 2 }}{% endif %}c", { "a2", "c" } },
        { "{{ 1 }}{% if 1 %}{% endif %}{{ 2 }}", { "1", "2" } },
        { "{% set q = 1 %}a{{ q }}b", { "a", "1", "b" } },
        { "{{ y }}{{ 1 }}{{ y }}", { "", "1", "" } },
        { "{{ '' }}", { "" } },
        { "{% macro m() %}{% endmacro %}{{ m() }}", { "" } },
        { "{% filter upper %}{% endfilter %}", { "" } },
        { "{% filter upper %}a{{ y }}b{% endfilter %}", { "AB" } },
        { "{% macro m() %}{{ caller() }}{% endmacro %}{% call m() %}{% endcall %}|", { "", "|" } },
        { "{% set x %}{% endset %}a{{ 1 }}", { "a1" } },
        { "{% autoescape y %}a{{ 1 }}b{{ 2 }}{% endautoescape %}", { "a", "1", "b", "2" } },
        { "{% for i in [[1, [2]], 3] recursive %}({{ i if i is number else loop(i) }}){% endfor %}", { "((1)((2)))(3)" } },
        { "x{% for i in [1, 2] recursive %}{{ i }}{% endfor %}y", { "x", "12", "y" } },
        { "{% block b %}A{{ 5 }}B{% endblock %}{{ self.b() }}", { "A5B", "A5B" } },
        { "{% macro m() %}{{ 'x' * 900 }}{% endmacro %}{{ m() }}", { std::string(900, 'x') } },
        { longMacro, { longOutput } },
    };
    RenderOptions options;
    options.writeEachPiece = true;
    for (const auto& [source, expected] : rows)
    {
        Template tpl;
        ASSERT_TRUE(tpl.Load(source)) << source;
        RecordingSink<char> sink;
        sink.eachPiece = true;
        ASSERT_TRUE(tpl.RenderTo(sink, {}, options)) << source;
        EXPECT_EQ(sink.chunks, expected) << source;

        TemplateW wide;
        ASSERT_TRUE(wide.Load(std::wstring(source.begin(), source.end()))) << source;
        RecordingSink<wchar_t> wideSink;
        wideSink.eachPiece = true;
        ASSERT_TRUE(wide.RenderTo(wideSink, {}, options)) << source;
        ASSERT_EQ(wideSink.chunks.size(), expected.size()) << source;
        for (std::size_t idx = 0; idx != expected.size(); ++idx)
        {
            EXPECT_EQ(wideSink.chunks[idx], std::wstring(expected[idx].begin(), expected[idx].end())) << source << " " << idx;
        }

        // The default gathers the pieces, and gives the sink no empty one
        RecordingSink<char> gathered;
        ASSERT_TRUE(tpl.RenderTo(gathered, {})) << source;
    }

    // super() of a parent block, in a template that extends another
    TemplateEnv env;
    auto handler = std::make_shared<MemoryFileSystem>();
    handler->AddFile("base", "A{% block b %}P{{ x }}Q{% endblock %}Z");
    env.AddFilesystemHandler(std::string(), handler);
    Template child(&env);
    ASSERT_TRUE(child.Load("{% extends 'base' %}{% block b %}<{{ super() }}>{% endblock %}"));
    RecordingSink<char> sink;
    ASSERT_TRUE(child.RenderTo(sink, { { "x", 1 } }, options));
    EXPECT_EQ(sink.chunks, (std::vector<std::string>{ "A", "<", "P1Q", ">", "Z" }));
}

TEST(OutputTest, EachPieceRenderKeepsItsPiecesAroundOthersOnTheThread)
{
    // The piece flush rides the stack check's slow path (EachPieceScope): a render with limits, and a render a
    // callable runs inside it, leave the outer render's pieces as they are, and the next render gathers again
    Template inner;
    ASSERT_TRUE(inner.Load("x{{ 1 }}y"));
    TemplateEnv env;
    env.GetSettings().limits = UntrustedLimits();
    Template limited(&env);
    ASSERT_TRUE(limited.Load("p{{ 2 }}q"));
    const ValuesMap params = { { "f", MakeCallable([&inner]() { return inner.RenderAsString({}).value(); }) },
                               { "g", MakeCallable([&limited]() { return limited.RenderAsString({}).value(); }) } };
    RenderOptions options;
    options.writeEachPiece = true;

    Template outer;
    ASSERT_TRUE(outer.Load("a{{ f() }}b{{ g() }}c{{ 3 }}"));
    RecordingSink<char> sink;
    ASSERT_TRUE(outer.RenderTo(sink, params, options));
    // c{{ 3 }} is one piece: Jinja2 folds the constant into the text (0456)
    EXPECT_EQ(sink.chunks, (std::vector<std::string>{ "a", "x1y", "b", "p2q", "c3" }));

    // With limits of its own
    Template outerLimited(&env);
    ASSERT_TRUE(outerLimited.Load("a{{ f() }}b{{ 3 }}"));
    RecordingSink<char> limitedSink;
    ASSERT_TRUE(outerLimited.RenderTo(limitedSink, params, options));
    EXPECT_EQ(limitedSink.chunks, (std::vector<std::string>{ "a", "x1y", "b3" }));

    RecordingSink<char> gathered;
    ASSERT_TRUE(outer.RenderTo(gathered, params));
    EXPECT_EQ(gathered.chunks, (std::vector<std::string>{ "ax1ybp2qc3" }));
}

TEST(OutputTest, SinkGetsLongFragmentsAsTheirOwnChunks)
{
    // A text fragment of 128 characters or more skips the buffer: what the buffer holds goes first
    const std::string text(1000, 't');
    Template tpl;
    ASSERT_TRUE(tpl.Load("{{ 1 }}{{ 'ab' }}" + text + "{{ 2 }}"));
    RecordingSink<char> sink;
    ASSERT_TRUE(tpl.RenderTo(sink, {}));
    ASSERT_EQ(3U, sink.chunks.size());
    EXPECT_EQ("1ab", sink.chunks[0]);
    EXPECT_EQ(text, sink.chunks[1]);
    EXPECT_EQ("2", sink.chunks[2]);

    TemplateW wide;
    ASSERT_TRUE(wide.Load(L"{{ 1 }}{{ 'ab' }}" + Widen(text) + L"{{ 2 }}"));
    RecordingSink<wchar_t> wideSink;
    ASSERT_TRUE(wide.RenderTo(wideSink, {}));
    ASSERT_EQ(3U, wideSink.chunks.size());
    EXPECT_EQ(Widen(text), wideSink.chunks[1]);
}

TEST(OutputTest, EmptyOutput)
{
    // A sink is never given an empty chunk, and a render that writes nothing never calls it
    ExpectRender("", "");
    ExpectRender("{{ '' }}{% if false %}x{% endif %}", "");
    Template tpl;
    ASSERT_TRUE(tpl.Load("{{ '' }}"));
    RecordingSink<char> sink;
    ASSERT_TRUE(tpl.RenderTo(sink, {}));
    EXPECT_TRUE(sink.chunks.empty());
}

TEST(OutputTest, ErrorMidRenderTruncatesTheString)
{
    Template tpl;
    ASSERT_TRUE(tpl.Load(failingTemplate));
    std::string out = "prefix";
    auto result = tpl.RenderTo(out, FailingParams());
    ASSERT_FALSE(result);
    EXPECT_EQ("prefix", out);

    // Wide too
    TemplateW wide;
    ASSERT_TRUE(wide.Load(Widen(failingTemplate)));
    std::wstring wideOut = L"prefix";
    ASSERT_FALSE(wide.RenderTo(wideOut, FailingParams()));
    EXPECT_EQ(L"prefix", wideOut);

    // The same template renders afterwards on this thread
    out.clear();
    ASSERT_TRUE(tpl.RenderTo(out, { { "fail", MakeCallable([]() { return Value(std::string("ok")); }) } }));
    EXPECT_EQ(Numbers(2000) + "oktail", out);
}

TEST(OutputTest, ErrorMidRenderKeepsWhatTheSinkGot)
{
    // As Python's generate(): the chunks written before the error stay written
    const auto expected = Numbers(2000);
    Template tpl;
    ASSERT_TRUE(tpl.Load(failingTemplate));
    RecordingSink<char> sink;
    ASSERT_FALSE(tpl.RenderTo(sink, FailingParams()));
    EXPECT_FALSE(sink.chunks.empty());
    EXPECT_LT(sink.output.size(), expected.size());
    EXPECT_EQ(expected.substr(0, sink.output.size()), sink.output);

    // ContainerSink keeps them too: it does not truncate
    std::string kept;
    ContainerSink containerSink(kept);
    ASSERT_FALSE(tpl.RenderTo(containerSink, FailingParams()));
    EXPECT_EQ(sink.output, kept);
}

TEST(OutputTest, SinkExceptionsAreHostExceptions)
{
    Template tpl;
    ASSERT_TRUE(tpl.Load("{% for i in range(2000) %}{{ i }},{% endfor %}"));

    // Any exception: kept in the error, as one from a user callable
    ThrowingSink failing(2, [] { throw std::runtime_error("sink failed"); });
    auto result = tpl.RenderTo(failing, {});
    ASSERT_FALSE(result);
    EXPECT_EQ(ErrorCode::UnexpectedException, result.error().GetCode());
    ASSERT_TRUE(result.error().GetException());
    EXPECT_THROW(std::rethrow_exception(result.error().GetException()), std::runtime_error);
    EXPECT_EQ(Numbers(2000).substr(0, failing.output.size()), failing.output);

    // A jinja2::Exception keeps its code: a sink enforcing an output budget
    ThrowingSink budget(3, [] { throw Exception(ErrorCode::ResourceLimitExceeded, "output budget spent"); });
    result = tpl.RenderTo(budget, {});
    ASSERT_FALSE(result);
    EXPECT_EQ(ErrorCode::ResourceLimitExceeded, result.error().GetCode());

    // Thrown by the final flush, after the template's body ended
    Template shortTpl;
    ASSERT_TRUE(shortTpl.Load("short"));
    ThrowingSink atFlush(1, [] { throw Exception(ErrorCode::ResourceLimitExceeded, "output budget spent"); });
    result = shortTpl.RenderTo(atFlush, {});
    ASSERT_FALSE(result);
    EXPECT_EQ(ErrorCode::ResourceLimitExceeded, result.error().GetCode());

    // std::bad_alloc goes on to the caller
    ThrowingSink outOfMemory(1, [] { throw std::bad_alloc(); });
    EXPECT_THROW(static_cast<void>(shortTpl.RenderTo(outOfMemory, {})), std::bad_alloc);
    // and leaves the template and the thread usable
    RecordingSink<char> sink;
    ASSERT_TRUE(shortTpl.RenderTo(sink, {}));
    EXPECT_EQ("short", sink.output);
}

TEST(OutputTest, BadAllocTruncatesTheString)
{
    Template tpl;
    ASSERT_TRUE(tpl.Load("{% for i in range(2000) %}{{ i }},{% endfor %}{{ fail() }}"));
    std::string out = "prefix";
    EXPECT_THROW(static_cast<void>(tpl.RenderTo(out, { { "fail", MakeCallable([]() -> Value { throw std::bad_alloc(); }) } })), std::bad_alloc);
    EXPECT_EQ("prefix", out);
}

TEST(OutputTest, OutputLimitCountsWhatTheSinkGot)
{
    Settings settings;
    settings.limits.maxOutputSize = 1000;
    TemplateEnv env;
    env.SetSettings(settings);
    Template tpl(&env);
    ASSERT_TRUE(tpl.Load("{% for i in range(2000) %}{{ i }},{% endfor %}"));
    RecordingSink<char> sink;
    auto result = tpl.RenderTo(sink, {});
    ASSERT_FALSE(result);
    EXPECT_EQ(ErrorCode::ResourceLimitExceeded, result.error().GetCode());
    EXPECT_GE(sink.output.size(), 1000U);
    EXPECT_LT(sink.output.size(), 2000U);

    // A string limit counts from where the render started appending
    std::string out(5000, '-');
    result = tpl.RenderTo(out, {});
    ASSERT_FALSE(result);
    EXPECT_EQ(ErrorCode::ResourceLimitExceeded, result.error().GetCode());
    EXPECT_EQ(std::string(5000, '-'), out);

    Template small(&env);
    ASSERT_TRUE(small.Load("{% for i in range(10) %}{{ i }}{% endfor %}"));
    RecordingSink<char> smallSink;
    ASSERT_TRUE(small.RenderTo(smallSink, {}));
    EXPECT_EQ("0123456789", smallSink.output);
}

TEST(OutputTest, CallableRendersAnotherTemplateToASink)
{
    // A render started inside another one, on the same thread, writes to its own sink
    Template inner;
    ASSERT_TRUE(inner.Load("{% for i in range(300) %}[{{ i }}]{% endfor %}"));
    RecordingSink<char> innerSink;
    Template outer;
    ASSERT_TRUE(outer.Load("{% for i in range(300) %}{{ i }}{% endfor %}|{{ nested() }}|{{ 'end' }}"));
    RecordingSink<char> outerSink;
    ValuesMap params{ { "nested", MakeCallable([&]() {
                            auto result = inner.RenderTo(innerSink, {});
                            return Value(result ? int64_t{ 1 } : int64_t{ 0 });
                        }) } };
    ASSERT_TRUE(outer.RenderTo(outerSink, params));

    std::string innerExpected;
    std::string outerExpected;
    for (int idx = 0; idx != 300; ++idx)
    {
        innerExpected += "[" + std::to_string(idx) + "]";
        outerExpected += std::to_string(idx);
    }
    EXPECT_EQ(innerExpected, innerSink.output);
    EXPECT_EQ(outerExpected + "|1|end", outerSink.output);
}

TEST(OutputTest, ContainerSinkAppendsToContainers)
{
    Template tpl;
    ASSERT_TRUE(tpl.Load("{% for i in range(500) %}{{ i }} {% endfor %}" + std::string(300, 'x')));
    auto expected = std::string(">");
    for (int idx = 0; idx != 500; ++idx)
    {
        expected += std::to_string(idx) + " ";
    }
    expected += std::string(300, 'x');

    std::string str = ">";
    ContainerSink strSink(str);
    ASSERT_TRUE(tpl.RenderTo(strSink, {}));
    EXPECT_EQ(expected, str);

    std::vector<char> vec{ '>' };
    ContainerSink vecSink(vec);
    ASSERT_TRUE(tpl.RenderTo(vecSink, {}));
    EXPECT_EQ(expected, std::string(vec.begin(), vec.end()));

    fmt::memory_buffer buffer;
    buffer.push_back('>');
    ContainerSink bufferSink(buffer);
    ASSERT_TRUE(tpl.RenderTo(bufferSink, {}));
    EXPECT_EQ(expected, fmt::to_string(buffer));

    TemplateW wide;
    ASSERT_TRUE(wide.Load(L"{{ 'w' }}" + std::wstring(200, L'y')));
    std::wstring wideStr;
    ContainerSink wideSink(wideStr);
    ASSERT_TRUE(wide.RenderTo(wideSink, {}));
    EXPECT_EQ(L"w" + std::wstring(200, L'y'), wideStr);

    static_assert(std::is_base_of_v<OutputSink, ContainerSink<std::string>>);
    static_assert(std::is_base_of_v<OutputSinkW, ContainerSink<std::wstring>>);
    static_assert(std::is_base_of_v<OutputSink, ContainerSink<fmt::memory_buffer>>);
}

TEST(OutputTest, SinkRenderingDuringWriteKeepsEscapedText)
{
    // A sink that renders an escaped template of its own on every chunk: the per-thread escape
    // buffer must not hold text the outer stream still has to write (Codex on #616)
    Template inner;
    ASSERT_TRUE(inner.Load("{% autoescape true %}{{ y }}{% endautoescape %}"));
    const ValuesMap innerParams{ { "y", std::string(3000, '<') } };
    struct RenderingSink final : OutputSink
    {
        void Write(std::string_view text) override
        {
            output.append(text);
            auto rendered = nested->RenderAsString(*nestedParams);
            ok = ok && rendered && rendered.value().size() == 12000;
        }
        const Template* nested = nullptr;
        const ValuesMap* nestedParams = nullptr;
        std::string output;
        bool ok = true;
    };

    // A long escaped fragment after buffered text: the flush before it calls the sink
    Template longFragment;
    ASSERT_TRUE(longFragment.Load("{% autoescape true %}prefix{{ x }}{% endautoescape %}"));
    RenderingSink sink;
    sink.nested = &inner;
    sink.nestedParams = &innerParams;
    ASSERT_TRUE(longFragment.RenderTo(sink, { { "x", std::string(200, '&') } }));
    std::string expected = "prefix";
    for (int idx = 0; idx != 200; ++idx)
    {
        expected += "&amp;";
    }
    EXPECT_EQ(expected, sink.output);
    EXPECT_TRUE(sink.ok);

    // Short escaped fragments that fill the buffer: each flush calls the sink
    Template shortFragments;
    ASSERT_TRUE(shortFragments.Load("{% autoescape true %}{% for i in range(300) %}{{ v }}{% endfor %}{% endautoescape %}"));
    RenderingSink loopSink;
    loopSink.nested = &inner;
    loopSink.nestedParams = &innerParams;
    ASSERT_TRUE(shortFragments.RenderTo(loopSink, { { "v", std::string("<&>") } }));
    expected.clear();
    for (int idx = 0; idx != 300; ++idx)
    {
        expected += "&lt;&amp;&gt;";
    }
    EXPECT_EQ(expected, loopSink.output);
    EXPECT_TRUE(loopSink.ok);

    // Through an extends: the top-level writer passes the fragments on to the sink stream
    TemplateEnv env;
    auto fs = std::make_shared<MemoryFileSystem>();
    fs->AddFile("base.j2", "{% autoescape true %}[{% block b %}{% endblock %}]{% endautoescape %}");
    env.AddFilesystemHandler({}, fs);
    Template child(&env);
    ASSERT_TRUE(child.Load("{% extends 'base.j2' %}{{ x }}{% block b %}{% autoescape true %}pre{{ x }}{% endautoescape %}{% endblock %}"));
    RenderingSink childSink;
    childSink.nested = &inner;
    childSink.nestedParams = &innerParams;
    ASSERT_TRUE(child.RenderTo(childSink, { { "x", std::string(200, '&') } }));
    expected = "[pre";
    for (int idx = 0; idx != 200; ++idx)
    {
        expected += "&amp;";
    }
    EXPECT_EQ(expected + "]", childSink.output);
    EXPECT_TRUE(childSink.ok);
}
