#include "internal_value.h"
#include "public_host_object.h"
#include "sandbox_policy.h"
#include "value_visitors.h"

#include <jinja2cpp/sandbox_policy.h>
#include <jinja2cpp/user_callable.h>
#include <jinja2cpp/value.h>

#include <cstddef>
#include <memory>
#include <optional>
#include <string_view>
#include <vector>
#include <string>
#include <utility>

// The engine's side of the public sandbox policy (jinja2cpp/sandbox_policy.h, docs/tasks/0357 B7): one bridge
// implements the engine's IEngineSandboxPolicy over a public ISandboxPolicy, converting the values at each question

namespace jinja2
{
inline namespace v2
{
ISandboxPolicy::~ISandboxPolicy() = default;

std::optional<Value> ISandboxPolicy::Format(const Value& /*self*/, bool /*formatMap*/, const UserCallableParams& /*params*/) const
{
    return std::nullopt;
}

namespace
{
// What the policy is asked about for a template callable that has no value of the host's (a macro, `caller`, a
// global function such as `range`): an object of its Python class with no attributes (sandbox_policy.h)
class CallableStandIn final : public IMapItemAccessor
{
public:
    explicit CallableStandIn(const char* objectClass)
        : m_class(objectClass)
    {
    }

    [[nodiscard]] size_t GetSize() const override { return 0; }
    [[nodiscard]] std::optional<Value> Find(std::string_view /*name*/) const override { return std::nullopt; }
    [[nodiscard]] std::vector<std::string> GetKeys() const override { return {}; }
    [[nodiscard]] const char* GetObjectClass() const override { return m_class; }
    [[nodiscard]] bool IsEqual(const IComparable& other) const override { return this == &other; }

private:
    const char* m_class;
};

// `obj` as the policy gets it
Value PolicyValue(const InternalValue& obj)
{
    if (const auto* callable = GetIf<Callable>(&obj); callable && callable->GetKind() != Callable::UserCallable)
    {
        return Value(GenericMap(std::make_shared<const CallableStandIn>(callable->GetKind() == Callable::Macro ? "Macro" : "function")));
    }
    return ToHostValue(obj);
}

class SandboxPolicyBridge final : public IEngineSandboxPolicy
{
public:
    explicit SandboxPolicyBridge(std::shared_ptr<const ISandboxPolicy> policy)
        : m_policy(std::move(policy))
    {
    }

    [[nodiscard]] std::optional<InternalValue> NativeAttribute(const InternalValue& obj, const std::string& name) const override
    {
        auto replacement = m_policy->NativeAttribute(PolicyValue(obj), name);
        if (!replacement)
        {
            return std::nullopt;
        }
        return Value2IntValue(std::move(*replacement));
    }

    std::optional<InternalValue> Format(const InternalValue& self, bool formatMap, const CallParams& params, RenderContext& context) const override
    {
        // Through a callable of the engine, so that the arguments reach the policy as those of a host's callable do,
        // lent for the call, and its result is converted while they are
        bool declined = false;
        UserCallable call(
            [this, &declined, text = ToHostValue(self), formatMap](const UserCallableParams& callParams) -> Value {
                auto formatted = m_policy->Format(text, formatMap, callParams);
                if (!formatted)
                {
                    declined = true;
                    return Value();
                }
                return std::move(*formatted);
            },
            { ArgInfo(ArgInfo::VarArgs), ArgInfo(ArgInfo::VarKwArgs) });
        // An undefined argument formats as one
        call.keepUndefined = true;
        const auto callable = visitors::InputValueConvertor::ConvertUserCallable(call);
        auto result = GetIf<Callable>(&callable)->GetExpressionCallable()(params, context);
        if (declined)
        {
            return std::nullopt;
        }
        return result;
    }

private:
    std::shared_ptr<const ISandboxPolicy> m_policy;
};
} // namespace

std::shared_ptr<const IEngineSandboxPolicy> MakeEngineSandboxPolicy(std::shared_ptr<const ISandboxPolicy> policy)
{
    return std::make_shared<const SandboxPolicyBridge>(std::move(policy));
}
} // namespace v2
} // namespace jinja2
