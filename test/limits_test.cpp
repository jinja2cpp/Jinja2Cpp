// Settings::Limits (docs/tasks/0262): every limit fails the render with
// ErrorCode::ResourceLimitExceeded and names its field, the depth limits keep
// ErrorCode::RecursionLimitExceeded, and every template here renders with the default limits.
#include "test_tools.h"

#include <jinja2cpp/error_info.h>
#include <jinja2cpp/template.h>
#include <jinja2cpp/template_env.h>
#include <jinja2cpp/user_callable.h>
#include <jinja2cpp/value.h>

#include <gtest/gtest.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <istream>
#include <sstream>
#include <streambuf>
#include <string>
#include <thread>
#include <vector>

#if defined(__SANITIZE_ADDRESS__) || defined(__SANITIZE_THREAD__)
#define JINJA2CPP_TEST_SANITIZED 1
#endif
#ifdef __has_feature
#if __has_feature(address_sanitizer) || __has_feature(thread_sanitizer) || __has_feature(undefined_behavior_sanitizer)
#define JINJA2CPP_TEST_SANITIZED 1
#endif
#endif

using namespace jinja2;

namespace
{
struct LimitCase
{
    std::string name;
    std::string source;
    std::function<void(Settings::Limits&)> set;
    // The field the render fails on; empty when it renders
    std::string field;
    // Load fails instead of Render
    bool atLoad = false;
};

std::string Repeat(const std::string& part, std::size_t count)
{
    std::string result;
    for (std::size_t i = 0; i < count; ++i)
    {
        result += part;
    }
    return result;
}

constexpr const char* DoublingMacro = "{% macro f(n) %}{% if n %}{{ f(n - 1) }}{{ f(n - 1) }}{% endif %}{% endmacro %}{{ f(40) }}";
constexpr const char* MacroDepth = "{% macro f(n) %}{% if n %}{{ f(n - 1) }}{% endif %}{% endmacro %}";

std::vector<LimitCase> Cases()
{
    return {
        { "RangeOver", "{{ range(100001)|list|length }}", [](auto& l) { l.maxRange = 100000; }, "maxRange" },
        { "RangeAt", "{{ range(100000)|list|length }}", [](auto& l) { l.maxRange = 100000; }, "" },
        { "RangeNegativeStep", "{{ range(0, -100001, -1)|length }}", [](auto& l) { l.maxRange = 100000; }, "maxRange" },
        { "RangeStep", "{{ range(0, 200000, 2)|length }}", [](auto& l) { l.maxRange = 100000; }, "" },
        { "RangeEmpty", "{{ range(10, 0)|length }}", [](auto& l) { l.maxRange = 1; }, "" },
        { "RepeatOver", "{{ 'x' * 2000 }}", [](auto& l) { l.maxSequenceSize = 1000; }, "maxSequenceSize" },
        { "RepeatAt", "{{ ('x' * 1000)|length }}", [](auto& l) { l.maxSequenceSize = 1000; }, "" },
        { "ListOver", "{{ range(2000)|list|length }}", [](auto& l) { l.maxSequenceSize = 1000; }, "maxSequenceSize" },
        { "SequenceAboveCap", "{{ ('x' * 1000)|length }}", [](auto& l) { l.maxSequenceSize = UINT64_MAX; }, "" },
        // Past the built-in cap as well: the render's own limit is reported
        { "RepeatAboveCap", "{{ 'x' * 100000000 }}", [](auto& l) { l.maxSequenceSize = 1000; }, "maxSequenceSize" },
        { "CenterAboveCap", "{{ 'x'|center(100000000) }}", [](auto& l) { l.maxSequenceSize = 1000; }, "maxSequenceSize" },
        // f(*args) builds the argument list from the iterable (docs/tasks/0220)
        { "UnpackedArgsOver", "{% macro m() %}{{ varargs|length }}{% endmacro %}{{ m(*range(2000)) }}", [](auto& l) { l.maxSequenceSize = 1000; }, "maxSequenceSize" },
        { "UnpackedArgsAt", "{% macro m() %}{{ varargs|length }}{% endmacro %}{{ m(1, *range(999)) }}", [](auto& l) { l.maxSequenceSize = 1000; }, "" },
        { "UnpackedGeneratorOver", "{% macro m() %}{{ varargs|length }}{% endmacro %}{{ m(*range(2000)|select) }}", [](auto& l) { l.maxSequenceSize = 1000; }, "maxSequenceSize" },
        { "OutputOver", "{% for i in range(2000) %}x{% endfor %}", [](auto& l) { l.maxOutputSize = 1000; }, "maxOutputSize" },
        { "OutputAt", "{% for i in range(1000) %}x{% endfor %}", [](auto& l) { l.maxOutputSize = 1000; }, "" },
        { "OutputLongText", "{{ 'x' * 2000 }}", [](auto& l) { l.maxOutputSize = 1000; }, "maxOutputSize" },
        { "TemplateSizeOver", Repeat("x", 2000), [](auto& l) { l.maxTemplateSize = 1000; }, "maxTemplateSize", true },
        { "TemplateSizeAt", Repeat("x", 1000), [](auto& l) { l.maxTemplateSize = 1000; }, "" },
        { "IncludedSizeOver", "{% include 'big' %}", [](auto& l) { l.maxTemplateSize = 1000; }, "maxTemplateSize" },
        { "LoadsOver", "{% include 'a' %}{% include 'b' %}{% include 'c' %}", [](auto& l) { l.maxTemplateLoads = 2; }, "maxTemplateLoads" },
        { "LoadsAt", "{% include 'a' %}{% include 'b' %}", [](auto& l) { l.maxTemplateLoads = 2; }, "" },
        { "LoadsSameName", "{% include 'a' %}{% include 'a' %}{% include 'a' %}", [](auto& l) { l.maxTemplateLoads = 1; }, "" },
        { "LoadsImport", "{% import 'a' as x %}{% from 'b' import y %}{% extends 'c' %}", [](auto& l) { l.maxTemplateLoads = 2; }, "maxTemplateLoads" },
        { "StepsMacro", DoublingMacro, [](auto& l) { l.maxRenderSteps = 100000; }, "maxRenderSteps" },
        { "StepsLoop", "{% for i in range(1000000) %}{% endfor %}", [](auto& l) { l.maxRenderSteps = 100000; }, "maxRenderSteps" },
        { "StepsMaxIsNone", "{% for i in range(1000) %}{% endfor %}", [](auto& l) { l.maxRenderSteps = UINT64_MAX; }, "" },
        { "StepsMaxWithTime", "{% for i in range(10**9) %}{% endfor %}",
          [](auto& l) {
              l.maxRenderSteps = UINT64_MAX;
              l.maxRenderTime = std::chrono::milliseconds(50);
          },
          "maxRenderTime" },
        { "StepsMaxLessOne", "{% for i in range(1000) %}{% endfor %}", [](auto& l) { l.maxRenderSteps = UINT64_MAX - 1; }, "" },
        { "TimeNegative", "{% for i in range(5000) %}{% endfor %}", [](auto& l) { l.maxRenderTime = std::chrono::milliseconds(-5); }, "" },
        { "TimeForever", "{% for i in range(5000) %}{% endfor %}", [](auto& l) { l.maxRenderTime = std::chrono::milliseconds(INT64_MAX); }, "" },
        { "StepsLoopFilter", "{% for i in range(1000000) if false %}{% endfor %}", [](auto& l) { l.maxRenderSteps = 100000; }, "maxRenderSteps" },
        // The template, its body, the call of range() and ten iterations
        { "StepsAt", "{% for i in range(10) %}{% endfor %}", [](auto& l) { l.maxRenderSteps = 13; }, "" },
        { "StepsOneOver", "{% for i in range(10) %}{% endfor %}", [](auto& l) { l.maxRenderSteps = 12; }, "maxRenderSteps" },
        // The template and nine calls
        { "RenderDepthOver", std::string(MacroDepth) + "{{ f(9) }}", [](auto& l) { l.maxRenderDepth = 10; }, "RecursionLimitExceeded" },
        { "RenderDepthAt", std::string(MacroDepth) + "{{ f(8) }}", [](auto& l) { l.maxRenderDepth = 10; }, "" },
        { "RenderDepthStackOnly", std::string(MacroDepth) + "{{ f(400) }}", [](auto& l) { l.maxRenderDepth = 0; }, "" },
        { "ExpressionDepthOver", "{{ ((((1)))) }}", [](auto& l) { l.maxExpressionDepth = 4; }, "RecursionLimitExceeded", true },
        { "ExpressionDepthAt", "{{ (((1))) }}", [](auto& l) { l.maxExpressionDepth = 4; }, "" },
        { "TargetDepthOver", "{% for (((a))) in [1] %}{% endfor %}", [](auto& l) { l.maxExpressionDepth = 3; }, "RecursionLimitExceeded", true },
        { "OperatorsOver", "{{ 1 + 1 + 1 + 1 + 1 }}", [](auto& l) { l.maxExpressionOperators = 3; }, "RecursionLimitExceeded", true },
        { "OperatorsAt", "{{ 1 + 1 + 1 + 1 }}", [](auto& l) { l.maxExpressionOperators = 3; }, "" },
        // The template's top level is a block too
        { "BlocksOver", "{% if 1 %}{% if 1 %}{% if 1 %}{% endif %}{% endif %}{% endif %}", [](auto& l) { l.maxBlockNesting = 3; }, "RecursionLimitExceeded", true },
        { "BlocksAt", "{% if 1 %}{% if 1 %}{% endif %}{% endif %}", [](auto& l) { l.maxBlockNesting = 3; }, "" },
    };
}

class LimitsTest
    : public TemplateEnvFixture
    , public ::testing::WithParamInterface<LimitCase>
{
protected:
    void SetUp() override
    {
        TemplateEnvFixture::SetUp();
        AddFile("big", Repeat("y", 2000));
        AddFile("a", "{% macro y() %}{% endmacro %}a");
        AddFile("b", "{% macro y() %}{% endmacro %}b");
        AddFile("c", "c");
    }
};

// Load and Render of `source`; the error of the step that fails
std::optional<ErrorInfo> LoadAndRender(TemplateEnv& env, const std::string& source, bool& failedAtLoad)
{
    Template tpl(&env);
    auto loaded = tpl.Load(source);
    failedAtLoad = !loaded;
    if (!loaded)
    {
        return loaded.error();
    }
    auto rendered = tpl.RenderAsString({});
    if (!rendered)
    {
        return rendered.error();
    }
    return std::nullopt;
}
} // namespace

TEST_P(LimitsTest, Enforced)
{
    const auto& param = GetParam();
    Settings settings;
    param.set(settings.limits);
    m_env.SetSettings(settings);

    bool failedAtLoad = false;
    const auto error = LoadAndRender(m_env, param.source, failedAtLoad);
    if (param.field.empty())
    {
        if (error)
        {
            ADD_FAILURE() << ErrorToString(*error);
        }
        return;
    }
    if (!error)
    {
        FAIL() << "no error";
    }
    EXPECT_EQ(param.atLoad, failedAtLoad);
    if (param.field == "RecursionLimitExceeded")
    {
        EXPECT_EQ(ErrorCode::RecursionLimitExceeded, error->GetCode()) << ErrorToString(*error);
        return;
    }
    EXPECT_EQ(ErrorCode::ResourceLimitExceeded, error->GetCode()) << ErrorToString(*error);
    ASSERT_EQ(2U, error->GetExtraParams().size());
    EXPECT_EQ(param.field, ToString(error->GetExtraParams()[1]));
}

TEST_P(LimitsTest, DefaultsRender)
{
    const auto& param = GetParam();
    if (param.name == "StepsMacro" || param.name == "RenderDepthStackOnly" || param.name == "RepeatAboveCap" || param.name == "StepsMaxWithTime" || param.name == "CenterAboveCap")
    {
        // 2^40 calls and 10^9 iterations would render, in time; 400 levels pass the default depth; the
        // above-cap rows fail on the internal cap without a limit
        return;
    }
    bool failedAtLoad = false;
    if (const auto error = LoadAndRender(m_env, param.source, failedAtLoad))
    {
        ADD_FAILURE() << ErrorToString(*error);
    }
}

INSTANTIATE_TEST_SUITE_P(Limits, LimitsTest, ::testing::ValuesIn(Cases()), [](const auto& p) { return p.param.name; });

using LimitsEnvTest = TemplateEnvFixture;

// Unlimited operators (maxExpressionOperators = 0): checking whether a long chain folds, or
// giving a constant to its first filter, must not overflow the stack while the template loads
// (docs/tasks/0216). The parser builds these chains without recursing; whether the evaluator
// then runs out of stack depends on the platform's stack and frames, so either outcome passes
TEST_F(LimitsEnvTest, LongChainsWithUnlimitedOperators)
{
    Settings settings;
    settings.limits.maxExpressionOperators = 0;
    m_env.SetSettings(settings);
#ifdef JINJA2CPP_TEST_SANITIZED
    // LLVM 18's ThreadSanitizer corrupts its trace on a history this long (docs/tasks/0093)
    constexpr unsigned chain = 20000;
#else
    constexpr unsigned chain = 200000;
#endif
    const std::string sources[] = {
        "{% autoescape 1" + Repeat(" + 1", chain) + " %}x{% endautoescape %}",
        "{% autoescape x %}{{ 1" + Repeat(" + 1", chain) + " }}{% endautoescape %}",
        "{% autoescape 'a'" + Repeat("[:]", chain) + " %}x{% endautoescape %}",
        "{% autoescape x %}{{ ['a']" + Repeat("[0][:]", chain / 2) + " }}{% endautoescape %}",
        "{{ 'a'" + Repeat("|upper", chain) + " }}",
        "{% autoescape x %}{{ 'a'" + Repeat("|upper", chain) + " }}{% endautoescape %}",
    };
    for (const auto& source : sources)
    {
        bool failedAtLoad = false;
        if (const auto error = LoadAndRender(m_env, source, failedAtLoad))
        {
            EXPECT_EQ(ErrorCode::RecursionLimitExceeded, error->GetCode()) << source.substr(0, 40) << ": " << ErrorToString(*error);
        }
    }
}

TEST_F(LimitsEnvTest, ErrorMessage)
{
    Settings settings;
    settings.limits.maxRange = 10;
    m_env.SetSettings(settings);
    Template tpl(&m_env);
    ASSERT_TRUE(tpl.Load("{{ range(11)|list }}").has_value());
    auto result = tpl.RenderAsString({});
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ("noname.j2tpl:1:1: error: Resource limit exceeded: range() of 11 items is longer than 10\n{{ range(11)|list }}\n^-------", ErrorToString(result.error()));

    TemplateW wide(&m_env);
    ASSERT_TRUE(wide.Load(L"{{ range(11)|list }}").has_value());
    auto wideResult = wide.RenderAsString({});
    ASSERT_FALSE(wideResult.has_value());
    EXPECT_EQ(ErrorCode::ResourceLimitExceeded, wideResult.error().GetCode());
}

TEST_F(LimitsEnvTest, LoaderSourcedTemplateSize)
{
    Settings settings;
    settings.limits.maxTemplateSize = 1000;
    m_env.SetSettings(settings);
    AddFile("big", std::string(2000, 'y'));
    auto result = m_env.LoadTemplate("big");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(ErrorCode::ResourceLimitExceeded, result.error().GetCode());
    EXPECT_EQ("maxTemplateSize", ToString(result.error().GetExtraParams()[1]));
}

// A stream that never ends: Load must stop reading it at the limit
class EndlessSource : public std::streambuf
{
public:
    EndlessSource() { setg(m_chunk.data(), m_chunk.data(), m_chunk.data() + m_chunk.size()); }

protected:
    int_type underflow() override
    {
        setg(m_chunk.data(), m_chunk.data(), m_chunk.data() + m_chunk.size());
        return traits_type::to_int_type(m_chunk[0]);
    }

private:
    std::string m_chunk = std::string(4096, 'x');
};

TEST_F(LimitsEnvTest, EndlessStreamStopsAtTemplateSize)
{
    Settings settings;
    settings.limits.maxTemplateSize = 1000;
    m_env.SetSettings(settings);
    EndlessSource source;
    std::istream stream(&source);
    Template tpl(&m_env);
    auto result = tpl.Load(stream, "endless");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(ErrorCode::ResourceLimitExceeded, result.error().GetCode());
    EXPECT_EQ("maxTemplateSize", ToString(result.error().GetExtraParams()[1]));
}

TEST_F(LimitsEnvTest, FromStringTemplateSize)
{
    Settings settings;
    settings.limits.maxTemplateSize = 1000;
    m_env.SetSettings(settings);
    auto narrow = m_env.FromString(std::string(2000, 'x'));
    ASSERT_FALSE(narrow.has_value());
    EXPECT_EQ("maxTemplateSize", ToString(narrow.error().GetExtraParams()[1]));
    auto wide = m_env.FromString(std::wstring(2000, L'x'));
    ASSERT_FALSE(wide.has_value());
    EXPECT_EQ(ErrorCode::ResourceLimitExceeded, wide.error().GetCode());
}

TEST_F(LimitsEnvTest, UndeclaredVariablesUseTheTemplatesLimits)
{
    // Deeper than the default maxExpressionDepth (70), allowed by the template's settings
    Settings settings;
    settings.limits.maxExpressionDepth = 200;
    m_env.SetSettings(settings);
    Template tpl(&m_env);
    ASSERT_TRUE(tpl.Load("{{ " + std::string(100, '(') + "x" + std::string(100, ')') + " }}").has_value());
    auto names = tpl.FindUndeclaredVariables();
    ASSERT_TRUE(names.has_value()) << ErrorToString(names.error());
    EXPECT_EQ(std::vector<std::string>{ "x" }, *names);
}

TEST_F(LimitsEnvTest, OutputCountsThisRenderOnly)
{
    Settings settings;
    settings.limits.maxOutputSize = 1000;
    m_env.SetSettings(settings);
    Template tpl(&m_env);
    ASSERT_TRUE(tpl.Load("{% for i in range(1000) %}x{% endfor %}").has_value());
    // Each render writes 1000 characters; the stream holds the earlier ones too
    std::ostringstream os;
    for (int i = 0; i < 3; ++i)
    {
        ASSERT_TRUE(tpl.Render(os, {}).has_value());
    }
    EXPECT_EQ(3000U, os.str().size());
}

TEST_F(LimitsEnvTest, RenderTime)
{
    Settings settings;
    settings.limits.maxRenderTime = std::chrono::milliseconds(200);
    m_env.SetSettings(settings);
    Template tpl(&m_env);
    ASSERT_TRUE(tpl.Load("{% for i in range(10**9) %}{% endfor %}").has_value());
    const auto start = std::chrono::steady_clock::now();
    auto result = tpl.RenderAsString({});
    const auto elapsed = std::chrono::steady_clock::now() - start;
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(ErrorCode::ResourceLimitExceeded, result.error().GetCode());
    EXPECT_EQ("maxRenderTime", ToString(result.error().GetExtraParams()[1]));
    EXPECT_GE(elapsed, std::chrono::milliseconds(200));
#if defined(NDEBUG) && !defined(JINJA2CPP_TEST_SANITIZED)
    // Loose: a loaded CI runner may stall the render between two polls
    EXPECT_LT(elapsed, std::chrono::milliseconds(1000));
#endif
}

TEST_F(LimitsEnvTest, RenderTimeHeavySteps)
{
    // Few steps, each a long filter: the clock is polled when the render ends
    Settings settings;
    settings.limits.maxRenderTime = std::chrono::milliseconds(1);
    m_env.SetSettings(settings);
    Template tpl(&m_env);
    ASSERT_TRUE(tpl.Load("{{ ('x' * 300000)|replace('x', 'yy')|length }}").has_value());
    auto result = tpl.RenderAsString({});
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ("maxRenderTime", ToString(result.error().GetExtraParams()[1]));
}

TEST_F(LimitsEnvTest, HostCallableIsNotInterrupted)
{
    Settings settings;
    settings.limits.maxRenderTime = std::chrono::milliseconds(50);
    m_env.SetSettings(settings);
    int returned = 0;
    ValuesMap params{ { "sleep", MakeCallable([&returned] {
                            std::this_thread::sleep_for(std::chrono::milliseconds(150));
                            ++returned;
                            return Value(std::string("slept"));
                        }) } };
    Template tpl(&m_env);
    ASSERT_TRUE(tpl.Load("{{ sleep() }}{{ sleep() }}").has_value());
    auto result = tpl.RenderAsString(params);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ("maxRenderTime", ToString(result.error().GetExtraParams()[1]));
    // The first call ran to its end and the second never started
    EXPECT_EQ(1, returned);
}

TEST_F(LimitsEnvTest, NestedRenderKeepsItsOwnLimits)
{
    // A render with a small step budget calls a host callable that renders a template of an
    // environment without limits, and the other way round
    TemplateEnv freeEnv;
    Template inner(&freeEnv);
    ASSERT_TRUE(inner.Load("{% for i in range(1000) %}{% endfor %}inner").has_value());
    Settings limited;
    limited.limits.maxRenderSteps = 20;
    TemplateEnv limitedEnv;
    limitedEnv.SetSettings(limited);
    Template limitedInner(&limitedEnv);
    ASSERT_TRUE(limitedInner.Load("{% for i in range(1000) %}{% endfor %}").has_value());

    ValuesMap params{ { "inner", MakeCallable([&inner] { return Value(inner.RenderAsString({}).value()); }) },
                      { "limitedInner", MakeCallable([&limitedInner] {
                            auto result = limitedInner.RenderAsString({});
                            return Value(std::string(!result && result.error().GetCode() == ErrorCode::ResourceLimitExceeded ? "stopped" : "ran"));
                        }) } };

    Template outer(&limitedEnv);
    ASSERT_TRUE(outer.Load("{{ inner() }}{% for i in range(5) %}{% endfor %}").has_value());
    EXPECT_EQ("inner", outer.RenderAsString(params).value());

    Template free(&freeEnv);
    ASSERT_TRUE(free.Load("{{ limitedInner() }}{% for i in range(1000) %}{% endfor %}").has_value());
    EXPECT_EQ("stopped", free.RenderAsString(params).value());
}

TEST_F(LimitsEnvTest, UnpackedKeywordsAreHeldToTheSequenceCap)
{
    // f(**d) builds a dict of keyword arguments from a host mapping (docs/tasks/0220)
    ValuesMap big;
    for (int idx = 0; idx < 2000; ++idx)
    {
        big["k" + std::to_string(idx)] = idx;
    }
    Settings limited;
    limited.limits.maxSequenceSize = 1000;
    TemplateEnv env;
    env.SetSettings(limited);
    Template tpl(&env);
    ASSERT_TRUE(tpl.Load("{% macro m() %}{{ kwargs|length }}{% endmacro %}{{ m(**big) }}").has_value());
    auto result = tpl.RenderAsString({ { "big", big } });
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(ErrorCode::ResourceLimitExceeded, result.error().GetCode());
    big.erase("k0");
    for (int idx = 1000; idx < 2000; ++idx)
    {
        big.erase("k" + std::to_string(idx));
    }
    EXPECT_EQ("999", tpl.RenderAsString({ { "big", big } }).value());
}

TEST_F(LimitsEnvTest, FailedRenderLeavesNoLimits)
{
    // A render that fails on its limits leaves none behind for the next one on the thread
    TemplateEnv limitedEnv;
    Settings limited;
    limited.limits.maxRenderSteps = 50;
    limited.limits.maxSequenceSize = 10;
    limited.limits.maxRenderDepth = 3;
    limitedEnv.SetSettings(limited);
    Template failing(&limitedEnv);
    ASSERT_TRUE(failing.Load("{% for i in range(1000) %}{% endfor %}").has_value());
    EXPECT_FALSE(failing.RenderAsString({}).has_value());

    EXPECT_EQ("2000", Render("{% for i in range(2000) %}{% endfor %}{{ ('x' * 2000)|length }}"));
    EXPECT_EQ("", Render("{% macro f(n) %}{% if n %}{{ f(n - 1) }}{% endif %}{% endmacro %}{{ f(20) }}"));
}

TEST_F(LimitsEnvTest, LoadsEveryUse)
{
    // With EveryUse each use loads the template again, and each load counts
    Settings settings;
    settings.templateLookup = TemplateLookup::EveryUse;
    settings.limits.maxTemplateLoads = 2;
    m_env.SetSettings(settings);
    AddFile("a", "a");
    Template tpl(&m_env);
    ASSERT_TRUE(tpl.Load("{% include 'a' %}{% include 'a' %}").has_value());
    EXPECT_EQ("aa", tpl.RenderAsString({}).value());
    ASSERT_TRUE(tpl.Load("{% include 'a' %}{% include 'a' %}{% include 'a' %}").has_value());
    auto result = tpl.RenderAsString({});
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ("maxTemplateLoads", ToString(result.error().GetExtraParams()[1]));
}

TEST_F(LimitsEnvTest, WideOutput)
{
    Settings settings;
    settings.limits.maxOutputSize = 1000;
    m_env.SetSettings(settings);
    TemplateW tpl(&m_env);
    ASSERT_TRUE(tpl.Load(L"{% for i in range(2000) %}x{% endfor %}").has_value());
    auto result = tpl.RenderAsString({});
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(ErrorCode::ResourceLimitExceeded, result.error().GetCode());
}

TEST_F(LimitsEnvTest, Presets)
{
    const auto limits = UntrustedLimits();
    EXPECT_EQ(100000U, limits.maxRange);
    EXPECT_EQ(std::uint64_t{ 1 } << 20, limits.maxSequenceSize);
    EXPECT_EQ(std::uint64_t{ 16 } << 20, limits.maxOutputSize);
    EXPECT_EQ(10000000U, limits.maxRenderSteps);
    EXPECT_EQ(0, limits.maxRenderTime.count());

    const auto settings = UntrustedSettings();
    EXPECT_TRUE(settings.autoescape);
    Settings expected;
    expected.limits = limits;
    expected.autoescape = true;
    EXPECT_EQ(expected, settings);
    expected.limits.maxRenderSteps = 1;
    EXPECT_NE(expected, settings);

    m_env.SetSettings(settings);
    EXPECT_EQ(ErrorCode::ResourceLimitExceeded, Load("{{ range(100001)|list }}").RenderAsString({}).error().GetCode());
    EXPECT_EQ("&lt;b&gt;", Load("{{ x }}").RenderAsString({ { "x", "<b>" } }).value());
}
