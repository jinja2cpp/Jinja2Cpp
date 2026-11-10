// The engine's part of a sandbox (docs/tasks/0354): ErrorCode::SecurityError of an unsafe attribute's error,
// Settings::immutableContainers (Dialect::ImmutableSandbox) and the undefined item of sum(attribute=). A sandbox
// policy is tried in public_sandbox_policy_test.cpp, through the public jinja2::ISandboxPolicy (docs/tasks/0357)

#include "gtest/gtest.h"

#include <jinja2cpp/error_info.h>
#include <jinja2cpp/template.h>
#include <jinja2cpp/template_env.h>
#include <jinja2cpp/value.h>

#include <string>

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

// Dialect::ImmutableSandbox, through its public switch
void UseImmutableContainers(TemplateEnv& env)
{
    env.GetSettings().immutableContainers = true;
}
} // namespace

TEST(SandboxEngineTest, UnsafeAttributeErrorIsASecurityError)
{
    TemplateEnv env;
    UseImmutableContainers(env);
    ErrorInfo error;
    const auto text = RenderOrError(env, "{{ [].append(1) }}", &error);
    EXPECT_NE(text.find("access to attribute 'append' of 'list' object is unsafe."), std::string::npos) << text;
    EXPECT_EQ(error.GetCode(), ErrorCode::SecurityError);
    EXPECT_EQ(error.GetExtraParams().size(), 1u);
}

TEST(SandboxEngineTest, PlainUndefinedErrorIsNoSecurityError)
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
    UseImmutableContainers(env);
    for (const char* source : { "{% set l = [1] %}{{ l.append(2) }}", "{% set l = [1] %}{{ l.pop() }}", "{% set d = {} %}{{ d.update({'a': 1}) }}", "{% set d = {'a': 1} %}{{ d.clear() }}", "{{ [].sort() }}" })
    {
        ErrorInfo error;
        const auto text = RenderOrError(env, source, &error);
        EXPECT_NE(text.find("is unsafe."), std::string::npos) << source << ": " << text;
        EXPECT_EQ(error.GetCode(), ErrorCode::SecurityError) << source;
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
