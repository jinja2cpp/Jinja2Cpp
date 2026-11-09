#ifndef JINJA2CPP_SRC_SANDBOX_POLICY_H
#define JINJA2CPP_SRC_SANDBOX_POLICY_H

#include "internal_value.h"

#include <memory>
#include <optional>
#include <string>

namespace jinja2
{
inline namespace v2
{
struct CallParams;
class RenderContext;

// What a host decides about the operations of a template on the engine's own values, for a sandbox (docs/tasks/0354,
// 0248 D8): Jinja2's SandboxedEnvironment.is_safe_attribute and wrap_str_format, for the values that are not host
// objects (a host object is asked through IEngineHostObject). The engine's side of the public jinja2::ISandboxPolicy
// (jinja2cpp/sandbox_policy.h, docs/tasks/0357 B7), which Settings::sandboxPolicy holds and one bridge
// (MakeEngineSandboxPolicy) implements this over. An environment holds it as a shared_ptr<const IEngineSandboxPolicy>
// (LoadSettings), the render context reaches it through its renderer callback, and a render without one pays a null
// check where it would ask: after a method of a value is found, where an attribute is missing, and at str.format
class IEngineSandboxPolicy
{
public:
    IEngineSandboxPolicy() = default;
    IEngineSandboxPolicy(const IEngineSandboxPolicy&) = delete;
    IEngineSandboxPolicy& operator=(const IEngineSandboxPolicy&) = delete;
    IEngineSandboxPolicy(IEngineSandboxPolicy&&) = delete;
    IEngineSandboxPolicy& operator=(IEngineSandboxPolicy&&) = delete;
    virtual ~IEngineSandboxPolicy() = default;

    // The attribute `name` of `obj`, a value of the engine, whether or not the engine has one of that name (a
    // method, or none): the replacement the sandbox reads instead (the unsafe undefined of a refused attribute, a
    // method the engine lacks), or nullopt to leave the engine's answer. Never asked for a host object
    [[nodiscard]] virtual std::optional<InternalValue> NativeAttribute(const InternalValue& obj, const std::string& name) const = 0;
    // s.format(*args, **kwargs), or s.format_map(mapping) when `formatMap`, of a string of the engine; nullopt for the
    // engine's own str.format, which asks NativeAttribute about the attribute a field reads (SandboxedFormatter)
    virtual std::optional<InternalValue> Format(const InternalValue& self, bool formatMap, const CallParams& params, RenderContext& context) const = 0;
};

class ISandboxPolicy;
// The engine's policy over a public one (public_sandbox_policy.cpp)
std::shared_ptr<const IEngineSandboxPolicy> MakeEngineSandboxPolicy(std::shared_ptr<const ISandboxPolicy> policy);

} // namespace v2
} // namespace jinja2

#endif // JINJA2CPP_SRC_SANDBOX_POLICY_H
