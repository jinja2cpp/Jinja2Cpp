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

// The Python class of a callable's stand-in: Macro, function, or type for a class that is a global (dict, cycler, ...),
// whose `mro` Jinja2's is_internal_attribute refuses; Jinja2's range is a function
const char* StandInClass(const Callable& callable, const InternalValue& obj)
{
    if (callable.GetKind() == Callable::Macro)
    {
        return "Macro";
    }
    const auto& attributes = callable.GetAttributes();
    const auto name = attributes ? attributes->find("__name__") : InternalValueMap::const_iterator();
    const bool isRange = attributes && name != attributes->end() && IsStringValue(name->second) && AsString(name->second) == "range";
    return !isRange && ClassObjectAttribute(obj, "__bases__") ? "type" : "function";
}

// `obj` as the policy gets it
Value PolicyValue(const InternalValue& obj)
{
    if (const auto* callable = GetIf<Callable>(&obj); callable && callable->GetKind() != Callable::UserCallable)
    {
        return Value(GenericMap(std::make_shared<const CallableStandIn>(StandInClass(*callable, obj))));
    }
    // A groupby group and a range have attributes of their own (grouper, list; start, stop, step) that the policy is asked
    // about, as Jinja2's is_safe_attribute is for _GroupTuple and range: stand-ins of those classes, whose attributes the
    // policy judges by name (docs/tasks/0425)
    if (const auto list = AsList(obj))
    {
        if (list->GetFieldNames())
        {
            return Value(GenericMap(std::make_shared<const CallableStandIn>("_GroupTuple")));
        }
        if (list->GetRangeInfo())
        {
            return Value(GenericMap(std::make_shared<const CallableStandIn>("range")));
        }
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
