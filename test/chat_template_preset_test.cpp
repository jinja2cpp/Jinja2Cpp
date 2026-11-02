// ConfigureForChatTemplates (docs/tasks/0360): what the preset changes, in narrow and wide
// templates, and that plain environments keep Jinja2's behaviour. Expected texts are
// transformers 5.22.1's (test/chat_templates/requirements.txt).

#include "test_tools.h"

#include <jinja2cpp/chat_template.h>
#include <jinja2cpp/error_info.h>
#include <jinja2cpp/features.h>
#include <jinja2cpp/template.h>
#include <jinja2cpp/template_env.h>
#include <jinja2cpp/value.h>

#include <gtest/gtest.h>

#include <chrono>
#include <ctime>
#include <string>
#include <type_traits>

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

} // namespace
