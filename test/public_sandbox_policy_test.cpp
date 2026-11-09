// The public sandbox (jinja2cpp/sandbox_policy.h, docs/tasks/0357 B7) through public headers only: a policy in
// Settings::sandboxPolicy, Settings::immutableContainers and ErrorCode::SecurityError. The expected outputs are
// Python Jinja2 3.1.6's SandboxedEnvironment and ImmutableSandboxedEnvironment where they have one

#include <jinja2cpp/error_info.h>
#include <jinja2cpp/host_object.h>
#include <jinja2cpp/sandbox_policy.h>
#include <jinja2cpp/template.h>
#include <jinja2cpp/template_env.h>
#include <jinja2cpp/user_callable.h>
#include <jinja2cpp/value.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace jinja2;

namespace
{
// Refuses private names as Jinja2's is_safe_attribute does, replaces `upper`, answers `extra` and records what it
// was asked
class ToyPolicy final : public ISandboxPolicy
{
public:
    [[nodiscard]] std::optional<Value> NativeAttribute(const Value& obj, std::string_view name) const override
    {
        {
            const std::lock_guard<std::mutex> lock(m_guard);
            m_asked.push_back(std::string(name) + (obj.IsString() ? " of '" + obj.AsString() + "'" : std::string()));
        }
        if (!name.empty() && name[0] == '_')
        {
            UndefinedDescription refused;
            refused.name = std::string(name);
            refused.hint = "access to attribute '" + std::string(name) + "' is unsafe.";
            refused.security = true;
            return MakeUndefined(std::move(refused));
        }
        if (name == "upper")
        {
            return Value(UserCallable{ [](const UserCallableParams&) { return Value("replaced"); }, {} });
        }
        if (name == "extra")
        {
            return Value("an attribute the engine lacks");
        }
        return std::nullopt;
    }

    [[nodiscard]] std::vector<std::string> Asked() const
    {
        const std::lock_guard<std::mutex> lock(m_guard);
        return m_asked;
    }

private:
    mutable std::mutex m_guard;
    mutable std::vector<std::string> m_asked;
};

size_t CountOf(const Value& list)
{
    if (const auto* values = list.GetIf<ValuesList>())
    {
        return values->size();
    }
    const auto* generic = list.GetIf<GenericList>();
    return generic ? generic->GetSize().value_or(0) : 0;
}

// Formats every string as the count of its arguments
class FormattingPolicy final : public ISandboxPolicy
{
public:
    [[nodiscard]] std::optional<Value> NativeAttribute(const Value&, std::string_view) const override { return std::nullopt; }
    [[nodiscard]] std::optional<Value> Format(const Value& self, bool formatMap, const UserCallableParams& params) const override
    {
        return Value((formatMap ? "map " : "format ") + self.AsString() + " " + std::to_string(CountOf(params.extraPosArgs)));
    }
};

std::string RenderOrError(TemplateEnv& env, const std::string& source, ErrorInfo* errorOut = nullptr, ValuesMap params = {})
{
    auto tpl = env.FromString(source);
    if (!tpl)
    {
        return "load error: " + tpl.error().ToString();
    }
    auto result = tpl->RenderAsString(params);
    if (!result)
    {
        if (errorOut)
        {
            *errorOut = result.error();
        }
        return "error: " + result.error().ToString();
    }
    return *result;
}
} // namespace

TEST(PublicSandboxPolicyTest, ARefusedAttributeIsASecurityErrorOnUse)
{
    auto policy = std::make_shared<const ToyPolicy>();
    TemplateEnv env;
    env.GetSettings().sandboxPolicy = policy;
    // Printed or tested, a refused attribute is an undefined value
    EXPECT_EQ("[]", RenderOrError(env, "[{{ ''.__class__ }}]"));
    EXPECT_EQ("False", RenderOrError(env, "{{ x.__class__ is defined }}", nullptr, { { "x", 1 } }));
    // Called, it fails as Jinja2's SecurityError
    ErrorInfo error;
    const auto text = RenderOrError(env, "{{ ''.__class__() }}", &error);
    EXPECT_EQ(ErrorCode::SecurityError, error.GetCode()) << text;
    EXPECT_NE(text.find("access to attribute '__class__' is unsafe."), std::string::npos) << text;
    // An undefined value the host made is described with the flag
    EXPECT_TRUE(UndefinedOf(*policy->NativeAttribute(Value(), "_x"))->security);
}

TEST(PublicSandboxPolicyTest, ThePolicyIsAskedAboutAttributesAndMethodsFoundOrNot)
{
    auto policy = std::make_shared<const ToyPolicy>();
    TemplateEnv env;
    env.GetSettings().sandboxPolicy = policy;
    EXPECT_EQ("replaced|True|an attribute the engine lacks|b", RenderOrError(env, "{{ 'abc'.upper() }}|{{ 'abc'|attr('upper') is defined }}|{{ 'abc'.extra }}|{{ 'a b'.split()[1] }}"));
    const auto asked = policy->Asked();
    // The object reaches the policy as a public value: here the string
    EXPECT_NE(std::find(asked.begin(), asked.end(), "upper of 'abc'"), asked.end());
    EXPECT_NE(std::find(asked.begin(), asked.end(), "extra of 'abc'"), asked.end());
    EXPECT_NE(std::find(asked.begin(), asked.end(), "split of 'a b'"), asked.end());
}

TEST(PublicSandboxPolicyTest, TheEnginesFormatAsksAboutFieldAttributes)
{
    auto policy = std::make_shared<const ToyPolicy>();
    TemplateEnv env;
    env.GetSettings().sandboxPolicy = policy;
    // Jinja2: '{0.__class__}|'.format(x) is '|' in a sandbox; a field reads what the sandbox lets it read
    EXPECT_EQ("|", RenderOrError(env, "{{ '{0.__class__}|'.format('s') }}"));
    EXPECT_EQ("an attribute the engine lacks", RenderOrError(env, "{{ '{0.extra}'.format('s') }}"));
    EXPECT_EQ("a-1", RenderOrError(env, "{{ '{}-{n}'.format('a', n=1) }}"));
}

TEST(PublicSandboxPolicyTest, APolicyMayFormatItself)
{
    TemplateEnv env;
    env.GetSettings().sandboxPolicy = std::make_shared<const FormattingPolicy>();
    EXPECT_EQ("format {}-{} 2", RenderOrError(env, "{{ '{}-{}'.format(1, 2) }}"));
    // A Markup's format too; its result is the policy's, not made Markup
    EXPECT_EQ("format &lt;{}&gt; 1", RenderOrError(env, "{{ ('<{}>'|e).format('x') }}"));
}

TEST(PublicSandboxPolicyTest, ImmutableContainersRefuseTheMethodsThatChangeThem)
{
    TemplateEnv plain;
    EXPECT_EQ("None[1, 2]", RenderOrError(plain, "{% set l = [1] %}{{ l.append(2) }}{{ l }}"));
    TemplateEnv env;
    env.GetSettings().immutableContainers = true;
    for (const char* source : { "{{ [].append(1) }}", "{% set d = {} %}{{ d.update({'a': 1}) }}", "{{ [3, 1].sort() }}" })
    {
        ErrorInfo error;
        const auto text = RenderOrError(env, source, &error);
        EXPECT_EQ(ErrorCode::SecurityError, error.GetCode()) << source << ": " << text;
        EXPECT_NE(text.find("is unsafe."), std::string::npos) << source << ": " << text;
    }
    ErrorInfo error;
    RenderOrError(env, "{{ [].append(1) }}", &error);
    EXPECT_NE(error.ToString().find("access to attribute 'append' of 'list' object is unsafe."), std::string::npos) << error.ToString();
    EXPECT_EQ("11[3, 1]", RenderOrError(env, "{% set l = [3, 1] %}{{ l.index(1) }}{{ l.count(3) }}{{ l.copy() }}"));
}

TEST(PublicSandboxPolicyTest, WideTemplatesAreAskedToo)
{
    auto policy = std::make_shared<const ToyPolicy>();
    TemplateEnv env;
    env.GetSettings().sandboxPolicy = policy;
    auto tpl = env.FromString(L"[{{ x.__class__ }}]{{ 'abc'.upper() }}");
    ASSERT_TRUE(tpl) << tpl.error().ToString();
    EXPECT_EQ(L"[]replaced", tpl->RenderAsString({ { "x", 1 } }).value());
}

TEST(PublicSandboxPolicyTest, ATemplateKeepsThePolicyItWasLoadedWith)
{
    TemplateEnv env;
    auto loose = env.FromString("{{ 'abc'.upper() }}");
    ASSERT_TRUE(loose) << loose.error().ToString();
    env.GetSettings().sandboxPolicy = std::make_shared<const ToyPolicy>();
    auto sandboxed = env.FromString("{{ 'abc'.upper() }}");
    ASSERT_TRUE(sandboxed) << sandboxed.error().ToString();
    EXPECT_EQ("ABC", loose->RenderAsString({}).value());
    EXPECT_EQ("replaced", sandboxed->RenderAsString({}).value());
    // Unset again: templates loaded from now on have none
    env.GetSettings().sandboxPolicy.reset();
    EXPECT_EQ("ABC", RenderOrError(env, "{{ 'abc'.upper() }}"));
    EXPECT_EQ("replaced", sandboxed->RenderAsString({}).value());
}

TEST(PublicSandboxPolicyTest, SettingsCompareThePolicyByIdentity)
{
    Settings a;
    Settings b;
    EXPECT_EQ(a, b);
    auto policy = std::make_shared<const ToyPolicy>();
    a.sandboxPolicy = policy;
    EXPECT_NE(a, b);
    b.sandboxPolicy = policy;
    EXPECT_EQ(a, b);
    b.sandboxPolicy = std::make_shared<const ToyPolicy>();
    EXPECT_NE(a, b);
    b.sandboxPolicy = policy;
    b.immutableContainers = true;
    EXPECT_NE(a, b);
}
