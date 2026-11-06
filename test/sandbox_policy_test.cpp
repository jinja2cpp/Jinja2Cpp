// The engine's part of the Python module's sandbox (docs/tasks/0354): the SecurityError marker of an unsafe
// attribute's error, Dialect::ImmutableSandbox, the undefined item of sum(attribute=), and the ISandboxPolicy
// callback an environment holds (src/sandbox_policy.h), tried with a toy policy written in C++

#include "../src/dialect.h"
#include "../src/engine_errors.h"
#include "../src/expression_evaluator.h"
#include "../src/internal_value.h"
#include "../src/sandbox_policy.h"
#include "../src/template_env_impl.h"

#include "gtest/gtest.h"

#include <jinja2cpp/error_info.h>
#include <jinja2cpp/template.h>
#include <jinja2cpp/template_env.h>
#include <jinja2cpp/value.h>

#include <memory>
#include <optional>
#include <string>

// Internal functions are not exported from a shared library; the Python module links the library statically
#ifndef JINJA2CPP_LINK_AS_SHARED

using namespace jinja2;

namespace
{
std::string RenderOrError(TemplateEnv& env, const std::string& source, ErrorInfo* errorOut = nullptr)
{
    Template tpl(&env);
    auto load = tpl.Load(source);
    if (!load)
    {
        return "load error: " + load.error().ToString();
    }
    auto result = tpl.RenderAsString(ValuesMap{});
    if (!result)
    {
        if (errorOut)
        {
            *errorOut = result.error();
        }
        return "error: " + result.error().ToString();
    }
    return result.value();
}

void UseDialect(TemplateEnv& env, detail::Dialect dialect)
{
    detail::TemplateEnvAccess::GetImpl(env)->SetDialect(dialect);
}

// Refuses `secret`, answers a method it replaces by a callable, answers a missing name, and formats by itself
class ToyPolicy final : public ISandboxPolicy
{
public:
    [[nodiscard]] std::optional<InternalValue> NativeAttribute(const InternalValue&, const std::string& name) const override
    {
        ++asked;
        if (name == "upper")
        {
            return InternalValue(Callable(Callable::UserCallable, [](const CallParams&, RenderContext&) { return InternalValue(std::string("replaced upper")); }));
        }
        if (name == "index" || name == "judged")
        {
            return InternalValue(std::string("judged ") + name);
        }
        if (name == "nothing_here")
        {
            return InternalValue(std::string("a missing name, answered"));
        }
        return std::nullopt;
    }
    InternalValue Format(const InternalValue&, bool formatMap, const CallParams& params, RenderContext&) const override
    {
        return InternalValue(std::string(formatMap ? "map:" : "format:") + std::to_string(params.posParams.size()));
    }
    mutable int asked = 0;
};
} // namespace

TEST(SandboxEngineTest, UnsafeAttributeErrorCarriesTheSecurityMarker)
{
    TemplateEnv env;
    UseDialect(env, detail::Dialect::ImmutableSandbox);
    ErrorInfo error;
    const auto text = RenderOrError(env, "{{ [].append(1) }}", &error);
    EXPECT_NE(text.find("access to attribute 'append' of 'list' object is unsafe."), std::string::npos) << text;
    EXPECT_EQ(error.GetCode(), ErrorCode::UndefinedError);
    ASSERT_EQ(error.GetExtraParams().size(), 2u);
    EXPECT_EQ(*error.GetExtraParams()[1].GetIf<std::string>(), SecurityErrorMarker);
}

TEST(SandboxEngineTest, PlainUndefinedErrorHasNoMarker)
{
    TemplateEnv env;
    ErrorInfo error;
    RenderOrError(env, "{{ x.y.z }}", &error);
    EXPECT_EQ(error.GetCode(), ErrorCode::UndefinedError);
    EXPECT_EQ(error.GetExtraParams().size(), 1u);
}

TEST(SandboxEngineTest, ImmutableSandboxDialectRefusesOnlyTheMutatingMethods)
{
    TemplateEnv plain;
    EXPECT_EQ(RenderOrError(plain, "{% set l = [1] %}{{ l.append(2) }}{{ l }}"), "None[1, 2]");
    TemplateEnv env;
    UseDialect(env, detail::Dialect::ImmutableSandbox);
    for (const char* source : { "{% set l = [1] %}{{ l.append(2) }}", "{% set l = [1] %}{{ l.pop() }}", "{% set d = {} %}{{ d.update({'a': 1}) }}", "{% set d = {'a': 1} %}{{ d.clear() }}", "{{ [].sort() }}" })
    {
        ErrorInfo error;
        const auto text = RenderOrError(env, source, &error);
        EXPECT_NE(text.find("is unsafe."), std::string::npos) << source << ": " << text;
        EXPECT_EQ(error.GetExtraParams().size(), 2u) << source;
    }
    EXPECT_EQ(RenderOrError(env, "{% set l = [3, 1] %}{{ l.index(1) }}{{ l.count(3) }}{{ l.copy() }}{% set d = {'a': 1} %}{{ d.get('a') }}{{ d.keys()|list }}"), "11[3, 1]1['a']");
    // Not the transformers dialect: tojson stays Jinja2's
    EXPECT_EQ(RenderOrError(env, "{{ {'a': '<'}|tojson }}"), RenderOrError(plain, "{{ {'a': '<'}|tojson }}"));
}

TEST(SandboxEngineTest, SumOfAnUndefinedItemIsAnUndefinedError)
{
    TemplateEnv env;
    ErrorInfo error;
    const auto text = RenderOrError(env, "{{ [{'a': 1}]|sum(attribute='z') }}", &error);
    EXPECT_EQ(error.GetCode(), ErrorCode::UndefinedError) << text;
    EXPECT_NE(text.find("has no attribute 'z'"), std::string::npos) << text;
    EXPECT_EQ(RenderOrError(env, "{{ [{'a': 1}, {'a': 2}]|sum(attribute='a') }}"), "3");
}

TEST(SandboxEngineTest, NoPolicyAsksNothing)
{
    TemplateEnv env;
    EXPECT_EQ(RenderOrError(env, "{{ 'x'.upper() }}|{{ 'x'.nothing_here is defined }}|{{ '{}'.format(1) }}"), "X|False|1");
}

TEST(SandboxEngineTest, PolicyIsAskedOnTheAttributeAndTheMethodCallPath)
{
    TemplateEnv env;
    auto policy = std::make_shared<ToyPolicy>();
    detail::TemplateEnvAccess::GetImpl(env)->SetSandboxPolicy(policy);
    // The method call path (CallExpression), the attribute path, a method read through [] and |attr, a missing name
    EXPECT_EQ(RenderOrError(env, "{{ 'x'.upper() }}"), "replaced upper");
    EXPECT_EQ(RenderOrError(env, "{% set f = 'x'.upper %}{{ f() }}"), "replaced upper");
    EXPECT_EQ(RenderOrError(env, "{{ 'x'['upper']() }}"), "replaced upper");
    EXPECT_EQ(RenderOrError(env, "{{ ('x'|attr('upper'))() }}"), "replaced upper");
    EXPECT_EQ(RenderOrError(env, "{{ 'x'.nothing_here }}"), "a missing name, answered");
    EXPECT_EQ(RenderOrError(env, "{{ 'x'.lower() }}|{{ 'x'.absent is defined }}"), "x|False");
    EXPECT_GT(policy->asked, 0);
    // A host that takes the policy away gets Jinja2's engine again; templates loaded before keep theirs
    detail::TemplateEnvAccess::GetImpl(env)->SetSandboxPolicy(nullptr);
    EXPECT_EQ(RenderOrError(env, "{{ 'x'.upper() }}"), "X");
}

TEST(SandboxEngineTest, PolicyIsAskedAboutTheAttributesOfLoopNamespaceAndMacro)
{
    TemplateEnv env;
    EXPECT_EQ(RenderOrError(env, "{% for i in [1] %}{{ loop.index }}{% endfor %}"), "1");
    detail::TemplateEnvAccess::GetImpl(env)->SetSandboxPolicy(std::make_shared<ToyPolicy>());
    EXPECT_EQ(RenderOrError(env, "{% for i in [1] %}{{ loop.index }}{% endfor %}"), "judged index");
    EXPECT_EQ(RenderOrError(env, "{% set ns = namespace(judged=1) %}{{ ns.judged }}"), "judged judged");
    EXPECT_EQ(RenderOrError(env, "{% macro m() %}x{% endmacro %}{{ m.judged }}"), "judged judged");
    // a name the policy leaves alone is read as it was
    EXPECT_EQ(RenderOrError(env, "{% for i in [1] %}{{ loop.first }}{% endfor %}"), "True");
}

TEST(SandboxEngineTest, PolicyFormatsStringsBothWays)
{
    TemplateEnv env;
    detail::TemplateEnvAccess::GetImpl(env)->SetSandboxPolicy(std::make_shared<ToyPolicy>());
    EXPECT_EQ(RenderOrError(env, "{{ '{} {}'.format(1, 2) }}"), "format:2");
    EXPECT_EQ(RenderOrError(env, "{{ ('{}'|safe).format(1) }}"), "format:1");
}

TEST(SandboxEngineTest, EnvironmentsWithDifferentPoliciesAreNotEqual)
{
    TemplateEnv one;
    TemplateEnv two;
    const auto policy = std::make_shared<ToyPolicy>();
    EXPECT_EQ(detail::TemplateEnvAccess::GetImpl(one)->GetSandboxPolicy(), nullptr);
    detail::TemplateEnvAccess::GetImpl(one)->SetSandboxPolicy(policy);
    EXPECT_EQ(detail::TemplateEnvAccess::GetImpl(one)->GetSandboxPolicy(), policy);
    EXPECT_FALSE(detail::TemplateEnvAccess::GetImpl(one)->IsEqual(*detail::TemplateEnvAccess::GetImpl(two)));
}

#endif
