// The output writer: values written without ValueRenderer, the buffer between the
// render and its string, and long fragments that skip it (docs/tasks/0113). Every case
// renders through RenderAsString, RenderTo a string and RenderTo a sink (docs/tasks/0257)

#include "gtest/gtest.h"

#include <jinja2cpp/exception.h>
#include <jinja2cpp/template.h>
#include <jinja2cpp/template_env.h>
#include <jinja2cpp/user_callable.h>
#include <jinja2cpp/value.h>

#include <fmt/format.h>

#include <cstddef>
#include <cstdint>
#include <exception>
#include <new>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
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
        EXPECT_FALSE(text.empty());
        chunks.emplace_back(text);
        output.append(text);
    }

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
