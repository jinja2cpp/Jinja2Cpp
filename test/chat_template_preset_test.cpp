// ConfigureForChatTemplates (docs/tasks/0360): what the preset changes, in narrow and wide
// templates, and that plain environments keep Jinja2's behaviour. Expected texts are
// transformers 5.22.1's (test/chat_templates/requirements.txt).

#include "test_tools.h"

#include <jinja2cpp/chat_template.h>
#include <jinja2cpp/error_info.h>
#include <jinja2cpp/exception.h>
#include <jinja2cpp/features.h>
#include <jinja2cpp/string_helpers.h>
#include <jinja2cpp/template.h>
#include <jinja2cpp/template_env.h>
#include <jinja2cpp/user_callable.h>
#include <jinja2cpp/value.h>

#include <gtest/gtest.h>

#include <chrono>
#include <cstddef>
#include <ctime>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace
{

std::chrono::system_clock::time_point FixedNow()
{
    std::tm t{};
    t.tm_year = 2025 - 1900;
    t.tm_mon = 2;
    t.tm_mday = 14;
    t.tm_hour = 9;
    t.tm_min = 26;
    t.tm_sec = 53;
    t.tm_isdst = -1;
    return std::chrono::system_clock::from_time_t(std::mktime(&t)) + std::chrono::microseconds(42);
}

struct ChatCase
{
    const char* tpl;
    const char* expected; // output, or "error: <text the error contains>"
};

template<typename CharT>
void CheckCase(jinja2::TemplateEnv& env, const ChatCase& c, const jinja2::ValuesMap& params)
{
    using Tpl = std::conditional_t<std::is_same_v<CharT, char>, jinja2::Template, jinja2::TemplateW>;
    Tpl tpl(&env);
    const std::string expected = c.expected;
    const bool expectError = expected.rfind("error: ", 0) == 0;
    auto load = tpl.Load(jinja2::ConvertString<std::basic_string<CharT>>(std::string(c.tpl)));
    if (!load)
    {
        ASSERT_TRUE(expectError) << c.tpl << ": " << jinja2::ToString(load.error().ToString());
        EXPECT_NE(jinja2::ToString(load.error().ToString()).find(expected.substr(7)), std::string::npos) << c.tpl << ": " << jinja2::ToString(load.error().ToString());
        return;
    }
    auto result = tpl.RenderAsString(params);
    if (expectError)
    {
        ASSERT_FALSE(result) << c.tpl << " rendered " << jinja2::ToString(*result);
        EXPECT_NE(jinja2::ToString(result.error().ToString()).find(expected.substr(7)), std::string::npos) << c.tpl << ": " << jinja2::ToString(result.error().ToString());
        return;
    }
    ASSERT_TRUE(result) << c.tpl << ": " << jinja2::ToString(result.error().ToString());
    EXPECT_EQ(jinja2::ToString(*result), expected) << c.tpl;
}

class ChatTemplatePresetTest : public ::testing::TestWithParam<ChatCase>
{
};

jinja2::ValuesMap Params()
{
    return { { "messages", jinja2::ValuesList{ jinja2::ValuesMap{ { "role", "user" }, { "content", "<é>" } } } },
             { "tool", jinja2::ValuesMap{ { "name", "f" }, { "b", 1 }, { "a", jinja2::ValuesList{ 1, "x", 2.5, jinja2::Value(), true } } } } };
}

TEST_P(ChatTemplatePresetTest, Narrow)
{
    jinja2::TemplateEnv env;
    jinja2::ConfigureForChatTemplates(env, { FixedNow });
    CheckCase<char>(env, GetParam(), Params());
}

TEST_P(ChatTemplatePresetTest, Wide)
{
    jinja2::TemplateEnv env;
    jinja2::ConfigureForChatTemplates(env, { FixedNow });
    CheckCase<wchar_t>(env, GetParam(), Params());
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(Preset, ChatTemplatePresetTest, ::testing::Values(
    // tojson: json.dumps(ensure_ascii=False, indent=None, separators=None, sort_keys=False)
    ChatCase{ "{{ tool|tojson }}", R"({"name": "f", "b": 1, "a": [1, "x", 2.5, null, true]})" },
    ChatCase{ "{{ messages[0].content|tojson }}", R"("<é>")" },
    ChatCase{ "{{ '\x7f\n\"\\\\'|tojson }}", "\"\x7f\\n\\\"\\\\\"" },
    ChatCase{ "{{ messages[0].content|tojson(true) }}", R"("<\u00e9>")" },
    ChatCase{ "{{ messages[0].content|tojson(ensure_ascii=true) }}", R"("<\u00e9>")" },
    ChatCase{ "{{ tool|tojson(sort_keys=true) }}", R"({"a": [1, "x", 2.5, null, true], "b": 1, "name": "f"})" },
    ChatCase{ "{{ {'b': 1}|tojson(indent=2) }}", "{\n  \"b\": 1\n}" },
    ChatCase{ "{{ [1, {'a': 2}]|tojson(indent='\t') }}", "[\n\t1,\n\t{\n\t\t\"a\": 2\n\t}\n]" },
    ChatCase{ "{{ [1]|tojson(indent=0) }}|{{ []|tojson(indent=2) }}", "[\n1\n]|[]" },
    ChatCase{ "{{ [1, {'a': 2}]|tojson(separators=(',', ':')) }}", R"([1,{"a":2}])" },
    ChatCase{ "{{ {'a': 2}|tojson(indent=1, separators=[';', '=']) }}", "{\n \"a\"=2\n}" },
    ChatCase{ "{{ 's'|tojson(indent=1.5) }}", R"("s")" },
    ChatCase{ "{{ 's'|tojson(separators=1) }}", "error: cannot unpack non-iterable int object" },
    ChatCase{ "{{ {1: 2, none: 3, true: 4, 2.5: 5}|tojson }}", R"({"1": 4, "null": 3, "2.5": 5})" },
    ChatCase{ "{{ [1]|tojson(separators=(',',)) }}", "error: not enough values to unpack (expected 2, got 1)" },
    ChatCase{ "{{ [1]|tojson(separators=1) }}", "error: cannot unpack non-iterable int object" },
    ChatCase{ "{{ [1]|tojson(separators='abcdefgh') }}", "error: too many values to unpack (expected 2)" },
    ChatCase{ "{{ [1]|tojson(separators={'a': 1, 'b': 2, 'c': 3}) }}", "error: too many values to unpack (expected 2)" },
    ChatCase{ "{{ [1, {'a': 2}]|tojson(separators={',': 0, '=': 0}) }}", R"([1,{"a"=2}])" },
    ChatCase{ "{{ 's'|tojson(indent=raise_exception('bad indent')) }}", "error: bad indent" },
    ChatCase{ "{{ x|tojson }}", "error: Object of type Undefined is not JSON serializable" },
    ChatCase{ "{{ ['a']|map('tojson')|join }}", R"("a")" },
    // `{% generation %}` renders its body in a scope of its own
    ChatCase{ "{% for i in [1, 2] %}{% generation %}{{ loop.index }}{% set x = 5 %}{% endgeneration %}{{ x }}{% endfor %}", "12" },
    ChatCase{ "{% set ns = namespace(a=1) %}{% generation %}{% set ns.a = 2 %}{% endgeneration %}{{ ns.a }}", "2" },
    ChatCase{ "{% generation %}a{% endgeneration %}\nb", "ab" },
    ChatCase{ "{% generation %}{% endgeneration %}x{% generation %}{% generation %}y{% endgeneration %}{% endgeneration %}", "xy" },
    ChatCase{ "{% for i in [1] %}{% generation %}{% break %}{% endgeneration %}{% endfor %}", "error: " },
    ChatCase{ "{% generation x %}{% endgeneration %}", "error: " },
    ChatCase{ "{% endgeneration %}", "error: " },
    ChatCase{ "{% generation %}", "error: " },
    // ImmutableSandboxedEnvironment: the methods that change a list or dict are unsafe
    ChatCase{ "{% set l = [1] %}{{ l.append }}|{{ l.append is defined }}|{{ l['append'] }}|{{ (l|attr('append')) is defined }}|{{ l.index(1) }}", "|False||False|0" },
    ChatCase{ "{% set l = [1] %}{{ l.append(2) }}", "error: access to attribute 'append' of 'list' object is unsafe." },
    ChatCase{ "{% set d = {} %}{{ d.update({'a': 1}) }}", "error: access to attribute 'update' of 'dict' object is unsafe." },
    ChatCase{ "{% set f = messages.pop %}{{ f() }}", "error: access to attribute 'pop' of 'list' object is unsafe." },
    ChatCase{ "{{ messages[0].get('role') }}{{ messages|length }}", "user1" },
    ChatCase{ "{{ range(100001)|length }}", "error: " },
    ChatCase{ "{{ range(100000)|length }}", "100000" },
    // trim_blocks, lstrip_blocks and loop controls
    ChatCase{ "{% for i in [1, 2, 3] %}\n  {% if i == 2 %}\n{% continue %}\n{% endif %}\n{{ i }}{% endfor %}", "13" },
    // the globals
    ChatCase{ "{{ raise_exception('Conversation roles must alternate') }}", "error: Conversation roles must alternate" },
    ChatCase{ "{{ [1, {'a': 2}]|tojson(separators=',=') }}", R"([1,{"a"=2}])" },
    ChatCase{ "{{ strftime_now('%d %b %Y %H:%M:%S %a %f|%z|%Z|%%') }}", "14 Mar 2025 09:26:53 Fri 000042|||%" },
    ChatCase{ "{{ strftime_now('%-d/%-m %-H:%M %Q %:z 100%') }}", "14/3 9:26 %Q  100%" },
    ChatCase{ "{{ strftime_now(5) }}", "error: strftime() argument 1 must be str, not int" },
    ChatCase{ "{{ raise_exception(5) }}", "error: 5" }));
// clang-format on

TEST(ChatTemplatePreset, RaiseExceptionIsTemplateRuntimeError)
{
    jinja2::TemplateEnv env;
    jinja2::ConfigureForChatTemplates(env);
    jinja2::Template tpl(&env);
    ASSERT_TRUE(tpl.Load("{{ raise_exception('bad role') }}"));
    auto result = tpl.RenderAsString({});
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().GetCode(), jinja2::ErrorCode::TemplateRuntimeError);
}

// strftime_now's result counts against maxSequenceSize as it grows, written or not
TEST(ChatTemplatePreset, StrftimeNowRespectsSequenceLimit)
{
    jinja2::TemplateEnv env;
    jinja2::ConfigureForChatTemplates(env, { FixedNow });
    env.GetSettings().limits.maxSequenceSize = 8;
    jinja2::Template tpl(&env);
    ASSERT_TRUE(tpl.Load("{% set t = strftime_now(f) %}{{ t|length }}"));
    auto ok = tpl.RenderAsString({ { "f", "%f" } });
    ASSERT_TRUE(ok) << jinja2::ToString(ok.error().ToString());
    EXPECT_EQ(*ok, "6");
    auto result = tpl.RenderAsString({ { "f", "%f%f" } });
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().GetCode(), jinja2::ErrorCode::ResourceLimitExceeded) << jinja2::ToString(result.error().ToString());
}

// Separators and the indent are host strings of any length: the document stays under the cap
TEST(ChatTemplatePreset, JsonSeparatorsRespectSequenceLimit)
{
    jinja2::TemplateEnv env;
    jinja2::ConfigureForChatTemplates(env);
    env.GetSettings().limits.maxSequenceSize = 8;
    jinja2::Template tpl(&env);
    ASSERT_TRUE(tpl.Load("{% set s = [1, 2]|tojson(separators=(sep, ':')) %}{{ s|length }}|{% set t = {'a': 1}|tojson(separators=(',', sep)) %}"));
    auto ok = tpl.RenderAsString({ { "sep", "," } });
    ASSERT_TRUE(ok) << jinja2::ToString(ok.error().ToString());
    EXPECT_EQ(*ok, "5|");
    for (const char* t : { "{% set s = [1, 2]|tojson(separators=(sep, ':')) %}", "{% set s = {'a': 1}|tojson(separators=(',', sep)) %}", "{% set s = [[1]]|tojson(indent=sep) %}" })
    {
        jinja2::Template big(&env);
        ASSERT_TRUE(big.Load(t));
        auto result = big.RenderAsString({ { "sep", "12345678" } });
        ASSERT_FALSE(result) << t;
        EXPECT_EQ(result.error().GetCode(), jinja2::ErrorCode::ResourceLimitExceeded) << t << ": " << jinja2::ToString(result.error().ToString());
    }
}

// The dialect lives in the environment's load settings (docs/tasks/0384): set before any Load, it
// survives a settings change that rebuilds them, and a template keeps the dialect it was loaded in
TEST(ChatTemplatePreset, DialectSurvivesSettingsChange)
{
    jinja2::TemplateEnv env;
    jinja2::ConfigureForChatTemplates(env);
    env.GetSettings().limits.maxRange = 10;
    jinja2::Template tpl(&env);
    ASSERT_TRUE(tpl.Load("{% set l = [1] %}{{ l.append(2) }}"));
    auto result = tpl.RenderAsString({});
    ASSERT_FALSE(result);
    EXPECT_NE(jinja2::ToString(result.error().ToString()).find("access to attribute 'append' of 'list' object is unsafe."), std::string::npos);
    env.GetSettings().limits.maxRange = 20;
    jinja2::Template again(&env);
    ASSERT_TRUE(again.Load("{{ {'b': 1, 'a': '<'}|tojson }}"));
    EXPECT_EQ(again.RenderAsString({}).value(), R"({"b": 1, "a": "<"})");
}

TEST(ChatTemplatePreset, SettingsAndVersion)
{
    jinja2::TemplateEnv env;
    env.GetSettings().limits.maxRange = 10;
    env.GetSettings().autoescape = true;
    jinja2::ConfigureForChatTemplates(env);
    const auto& settings = env.GetSettings();
    EXPECT_TRUE(settings.trimBlocks);
    EXPECT_TRUE(settings.lstripBlocks);
    EXPECT_TRUE(settings.extensions.loopControls);
    EXPECT_FALSE(settings.autoescape);
    EXPECT_EQ(settings.limits.maxRange, 10U); // a lower limit stays
    EXPECT_STREQ(JINJA2CPP_CHAT_TEMPLATES_TRANSFORMERS, "5.22.1");
}

// The dialect is the environment's: templates of other environments, and templates loaded
// before the preset, keep Jinja2's tojson, mutable lists and no generation tag
TEST(ChatTemplatePreset, PlainEnvironmentsStayJinja2)
{
    jinja2::TemplateEnv plain;
    jinja2::Template before(&plain);
    ASSERT_TRUE(before.Load("{% set l = [1] %}{% set _ = l.append(2) %}{{ {'b': 1, 'a': '<'}|tojson }}{{ l }}"));
    jinja2::ConfigureForChatTemplates(plain);
    EXPECT_EQ(before.RenderAsString({}).value(), R"({"a": "\u003c", "b": 1}[1, 2])");

    jinja2::TemplateEnv other;
    jinja2::Template tpl(&other);
    EXPECT_FALSE(tpl.Load("{% generation %}{% endgeneration %}"));
}

// The error of a method the sandbox refuses is transformers' SecurityError, with the sandbox's message
TEST(ChatTemplatePreset, UnsafeMethodIsSecurityError)
{
    for (const char* source : { "{% set _ = messages.append(1) %}", "{{ messages.append(1) }}", "{% set d = {'a': 1} %}{% set _ = d.update({'b': 2}) %}" })
    {
        {
            jinja2::TemplateEnv env;
            jinja2::ConfigureForChatTemplates(env);
            jinja2::Template tpl(&env);
            ASSERT_TRUE(tpl.Load(source)) << source;
            auto result = tpl.RenderAsString({ { "messages", jinja2::ValuesList{ 1 } } });
            ASSERT_FALSE(result) << source;
            EXPECT_EQ(result.error().GetCode(), jinja2::ErrorCode::SecurityError) << source;
            EXPECT_NE(jinja2::ToString(result.error().ToString()).find("is unsafe."), std::string::npos) << source;
        }
        {
            jinja2::TemplateEnv env;
            jinja2::ConfigureForChatTemplates(env);
            jinja2::TemplateW tpl(&env);
            ASSERT_TRUE(tpl.Load(jinja2::ConvertString<std::wstring>(std::string(source)))) << source;
            auto result = tpl.RenderAsString({ { "messages", jinja2::ValuesList{ 1 } } });
            ASSERT_FALSE(result) << source;
            EXPECT_EQ(result.error().GetCode(), jinja2::ErrorCode::SecurityError) << source;
        }
    }
}

// The assistant token mask (docs/tasks/0377): RenderOptions::generationSpans. Expected spans are
// what transformers 5.22.1 records in activate_tracker's generation_indices, in code points
struct SpanCase
{
    const char* tpl;
    std::vector<jinja2::GenerationSpan> spans;
};

std::shared_ptr<jinja2::MemoryFileSystem> SpanFiles()
{
    auto fs = std::make_shared<jinja2::MemoryFileSystem>();
    fs->AddFile("inc", "i{% generation %}gg{% endgeneration %}");
    fs->AddFile("base", "b{% block x %}{% endblock %}e");
    fs->AddFile("base2", "b{% block x %}p{% generation %}r{% endgeneration %}{% endblock %}e");
    return fs;
}

// Converts an offset in the units of `out` to code points (UTF-8 bytes, or UTF-16 units where wchar_t has two bytes)
template<typename CharT>
std::size_t CodePoints(const std::basic_string<CharT>& out, std::size_t units)
{
    std::size_t count = 0;
    // A block in a captured part (a set or filter block) has a span past the output, as in transformers: count the
    // missing units as one code point each, which the ASCII rows that test it rely on
    if (units > out.size())
    {
        count = units - out.size();
        units = out.size();
    }
    for (std::size_t i = 0; i < units; ++i)
    {
        const auto c = static_cast<unsigned long>(static_cast<std::make_unsigned_t<CharT>>(out[i]));
        if constexpr (sizeof(CharT) == 1)
        {
            count += (c & 0xC0U) != 0x80U;
        }
        else if constexpr (sizeof(CharT) == 2)
        {
            count += (c & 0xFC00U) != 0xDC00U;
        }
        else
        {
            ++count;
        }
    }
    return count;
}

template<typename CharT>
void CheckSpanCase(const SpanCase& c)
{
    using Tpl = std::conditional_t<std::is_same_v<CharT, char>, jinja2::Template, jinja2::TemplateW>;
    jinja2::TemplateEnv env;
    jinja2::ConfigureForChatTemplates(env);
    env.AddFilesystemHandler(std::string(), SpanFiles());
    struct SpanStop
        : std::runtime_error
        , jinja2::StopIteration
    {
        SpanStop()
            : std::runtime_error("stop")
        {
        }
    };
    env.AddFilter("stop", jinja2::MakeCallable([](const jinja2::Value&) -> jinja2::Value { throw SpanStop(); }, jinja2::ArgInfo{ "v" }));
    Tpl tpl(&env);
    ASSERT_TRUE(tpl.Load(jinja2::ConvertString<std::basic_string<CharT>>(std::string(c.tpl)))) << c.tpl;
    std::vector<jinja2::GenerationSpan> spans{ { 7, 7 } }; // cleared by the render
    jinja2::RenderOptions options;
    options.generationSpans = &spans;
    std::basic_string<CharT> out;
    auto result = tpl.RenderTo(out, {}, options);
    ASSERT_TRUE(result) << c.tpl << ": " << jinja2::ToString(result.error().ToString());
    ASSERT_EQ(spans.size(), c.spans.size()) << c.tpl;
    for (std::size_t i = 0; i < spans.size(); ++i)
    {
        EXPECT_EQ(CodePoints(out, spans[i].start), c.spans[i].start) << c.tpl << " #" << i;
        EXPECT_EQ(CodePoints(out, spans[i].end), c.spans[i].end) << c.tpl << " #" << i;
    }
}

class GenerationSpansTest : public ::testing::TestWithParam<SpanCase>
{
};

TEST_P(GenerationSpansTest, Narrow)
{
    CheckSpanCase<char>(GetParam());
}

TEST_P(GenerationSpansTest, Wide)
{
    CheckSpanCase<wchar_t>(GetParam());
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(Rows, GenerationSpansTest, ::testing::Values(
    SpanCase{ "ab{% generation %}cd{% endgeneration %}e{% generation %}\xC3\xA9\xE2\x82\xAC\xF0\x9F\x98\x80{% endgeneration %}", { { 2, 4 }, { 5, 8 } } },
    SpanCase{ "a{% generation %}b{% generation %}cc{% endgeneration %}d{% endgeneration %}", { { 1, 3 }, { 1, 5 } } },
    SpanCase{ "ab{% set x %}{% generation %}cd{% endgeneration %}{% endset %}", { { 2, 4 } } },
    SpanCase{ "ab{% set x %}q{% endset %}{% generation %}{{ x }}c{% endgeneration %}", { { 2, 4 } } },
    SpanCase{ "ab{% filter upper %}{% generation %}xxyy{% endgeneration %}{% endfilter %}", { { 2, 6 } } },
    SpanCase{ "ab{% for i in [1,2] %}x{% generation %}y{% endgeneration %}{% endfor %}", { { 3, 4 }, { 5, 6 } } },
    SpanCase{ "abc{% with a=1 %}{% generation %}{{ a }}{% endgeneration %}{% endwith %}", { { 3, 4 } } },
    SpanCase{ "abc{% if true %}{% generation %}d{% endgeneration %}{% endif %}", { { 3, 4 } } },
    SpanCase{ "a{% generation %}{% endgeneration %}b", { { 1, 1 } } },
    SpanCase{ "a{% block x %}bb{% generation %}cc{% endgeneration %}{% endblock %}", { { 3, 5 } } },
    SpanCase{ "{% macro m() %}xx{% generation %}yy{% endgeneration %}{% endmacro %}ab{% generation %}{{ m() }}{% endgeneration %}", { { 2, 4 }, { 2, 6 } } },
    SpanCase{ "ab\n{% generation %}\n  cd\n{% endgeneration %}\nef", { { 3, 8 } } },
    SpanCase{ "{% for i in [1,2,3] %}{% if i == 2 %}{% continue %}{% endif %}{% generation %}{{ i }}{% endgeneration %}{% endfor %}", { { 0, 1 }, { 1, 2 } } },
    SpanCase{ "{% macro m() %}xx{% generation %}yy{% endgeneration %}{% endmacro %}ab{{ 'q' ~ m() }}", { { 2, 4 } } },
    // Jinja2 renders these into a buffer that is yielded whole: the block begins where the buffer did
    SpanCase{ "{% macro m() %}xx{% generation %}yy{% endgeneration %}{% endmacro %}ab{{ m() }}", { { 2, 4 } } },
    SpanCase{ "{% macro m() %}xx{{ caller() }}{% endmacro %}ab{% call m() %}z{% generation %}yy{% endgeneration %}{% endcall %}", { { 2, 4 } } },
    SpanCase{ "{% macro m() %}xx{{ caller() }}{% endmacro %}ab{% generation %}{% call m() %}z{% generation %}y{% endgeneration %}{% endcall %}{% endgeneration %}", { { 2, 3 }, { 2, 6 } } },
    SpanCase{ "ab{% for i in [[1],[2]] recursive %}x{% generation %}y{{ i }}{% endgeneration %}{% if i is iterable %}{{ loop(i) }}{% endif %}{% endfor %}", { { 2, 6 }, { 2, 4 }, { 2, 6 }, { 2, 4 } } },
    SpanCase{ "{% block x %}x{% generation %}yy{% endgeneration %}{% endblock %}{{ self.x() }}", { { 1, 3 }, { 3, 5 } } },
    SpanCase{ "ab{% include 'inc' %}c", { { 3, 5 } } },
    SpanCase{ "{% macro m() %}zz{% include 'inc' %}{% endmacro %}ab{{ m() }}", { { 2, 4 } } },
    // A call that stops (docs/tasks/0479) drops the macro's output, and the blocks it held
    SpanCase{ "{% macro m() %}xx{% generation %}yy{% endgeneration %}{{ 1|stop }}{% endmacro %}ab{{ m() }}c{% generation %}d{% endgeneration %}", { { 3, 4 } } },
    SpanCase{ "{% extends 'base' %}{% block x %}x{% generation %}yy{% endgeneration %}{% endblock %}", { { 2, 4 } } },
    SpanCase{ "{% extends 'base2' %}{% block x %}q{{ super() }}{% endblock %}", { { 2, 3 } } },
    SpanCase{ "abc", {} }));
// clang-format on

template<typename CharT>
class AppendingSink final : public jinja2::BasicOutputSink<CharT>
{
public:
    void Write(std::basic_string_view<CharT> text) override { output.append(text); }
    std::basic_string<CharT> output;
};

TEST(ChatTemplatePreset, GenerationSpansRawUnitsAndTargets)
{
    jinja2::TemplateEnv env;
    jinja2::ConfigureForChatTemplates(env);
    const std::string source = "ab{% generation %}cd{% endgeneration %}e{% generation %}\xC3\xA9\xE2\x82\xAC\xF0\x9F\x98\x80{% endgeneration %}";
    jinja2::Template tpl(&env);
    ASSERT_TRUE(tpl.Load(source));
    std::vector<jinja2::GenerationSpan> spans;
    jinja2::RenderOptions options;
    options.generationSpans = &spans;

    // UTF-8 bytes
    std::string out;
    ASSERT_TRUE(tpl.RenderTo(out, {}, options));
    ASSERT_EQ(spans.size(), 2U);
    EXPECT_EQ(spans[1].start, 5U);
    EXPECT_EQ(spans[1].end, 14U);

    // Counted from what this render writes, whatever the string held
    std::string prefilled = "pre";
    ASSERT_TRUE(tpl.RenderTo(prefilled, {}, options));
    ASSERT_EQ(spans.size(), 2U);
    EXPECT_EQ(spans[0].start, 2U);
    EXPECT_EQ(spans[1].end, 14U);

    // A sink, which receives the output in chunks
    AppendingSink<char> sink;
    spans.assign(3, jinja2::GenerationSpan{ 1, 1 });
    ASSERT_TRUE(tpl.RenderTo(sink, {}, options));
    EXPECT_EQ(sink.output, out);
    ASSERT_EQ(spans.size(), 2U);
    EXPECT_EQ(spans[1].end, 14U);

    // wchar_t units
    jinja2::TemplateW wide(&env);
    ASSERT_TRUE(wide.Load(jinja2::ConvertString<std::wstring>(source)));
    std::wstring wout;
    ASSERT_TRUE(wide.RenderTo(wout, {}, options));
    ASSERT_EQ(spans.size(), 2U);
    EXPECT_EQ(spans[1].start, 5U);
    EXPECT_EQ(spans[1].end, sizeof(wchar_t) == 2 ? 9U : 8U);
    AppendingSink<wchar_t> wsink;
    ASSERT_TRUE(wide.RenderTo(wsink, {}, options));
    EXPECT_EQ(spans[1].end, sizeof(wchar_t) == 2 ? 9U : 8U);

    // Nothing asked, nothing recorded
    std::string plain;
    ASSERT_TRUE(tpl.RenderTo(plain, {}, jinja2::RenderOptions{}));
    EXPECT_EQ(plain, out);
}

TEST(ChatTemplatePreset, GenerationSpansOnErrorAndLimit)
{
    jinja2::TemplateEnv env;
    jinja2::ConfigureForChatTemplates(env);
    std::vector<jinja2::GenerationSpan> spans;
    jinja2::RenderOptions options;
    options.generationSpans = &spans;

    jinja2::Template failing(&env);
    ASSERT_TRUE(failing.Load("{% generation %}a{% endgeneration %}{% generation %}{{ raise_exception('x') }}{% endgeneration %}"));
    std::string out;
    spans.push_back({ 7, 7 });
    ASSERT_FALSE(failing.RenderTo(out, {}, options));
    // No span may point into output the failed render did not leave
    EXPECT_TRUE(spans.empty());
    EXPECT_TRUE(out.empty());

    jinja2::Template unloaded(&env);
    spans.push_back({ 7, 7 });
    ASSERT_FALSE(unloaded.RenderTo(out, {}, options));
    EXPECT_TRUE(spans.empty());

    env.GetSettings().limits.maxSequenceSize = 3;
    jinja2::Template many(&env);
    ASSERT_TRUE(many.Load("{% generation %}a{% endgeneration %}{% generation %}b{% endgeneration %}{% generation %}c{% endgeneration %}{% generation %}d{% endgeneration %}"));
    out.clear();
    auto result = many.RenderTo(out, {}, options);
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().GetCode(), jinja2::ErrorCode::ResourceLimitExceeded) << jinja2::ToString(result.error().ToString());
    EXPECT_TRUE(spans.empty());
}

TEST(ChatTemplatePreset, GenerationSpansPlainEnvironmentAndNoTag)
{
    jinja2::TemplateEnv plain;
    jinja2::Template tpl(&plain);
    ASSERT_TRUE(tpl.Load("abc"));
    std::vector<jinja2::GenerationSpan> spans{ { 1, 2 } };
    jinja2::RenderOptions options;
    options.generationSpans = &spans;
    std::string out;
    ASSERT_TRUE(tpl.RenderTo(out, {}, options));
    EXPECT_TRUE(spans.empty());
    EXPECT_EQ(out, "abc");
}

// A callable that renders another template starts a render of its own: it records nothing in the outer vector
TEST(ChatTemplatePreset, GenerationSpansIgnoreNestedHostRenders)
{
    jinja2::TemplateEnv env;
    jinja2::ConfigureForChatTemplates(env);
    jinja2::Template inner(&env);
    ASSERT_TRUE(inner.Load("x{% generation %}y{% endgeneration %}"));
    env.AddGlobal("inner", jinja2::UserCallable([&inner](const jinja2::UserCallableParams&) { return jinja2::Value(inner.RenderAsString({}).value()); }, {}));
    jinja2::Template outer(&env);
    ASSERT_TRUE(outer.Load("ab{% generation %}{{ inner() }}{% endgeneration %}"));
    std::vector<jinja2::GenerationSpan> spans;
    jinja2::RenderOptions options;
    options.generationSpans = &spans;
    std::string out;
    ASSERT_TRUE(outer.RenderTo(out, {}, options));
    EXPECT_EQ(out, "abxy");
    ASSERT_EQ(spans.size(), 1U);
    EXPECT_EQ(spans[0].start, 2U);
    EXPECT_EQ(spans[0].end, 4U);
}

} // namespace
