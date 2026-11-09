#ifndef JINJA2CPP_SRC_SANDBOX_POLICY_H
#define JINJA2CPP_SRC_SANDBOX_POLICY_H

#include "internal_value.h"

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
// objects (a host object is asked through IEngineHostObject). Internal for now; docs/design/0357 makes it the public
// ISandboxPolicy, held by Settings. An environment holds it as a shared_ptr<const ISandboxPolicy> (LoadSettings), the
// render context reaches it through its renderer callback, and a render without one pays a null check where it would
// ask: after a method of a value is found, where an attribute is missing, and at str.format
class ISandboxPolicy
{
public:
    ISandboxPolicy() = default;
    ISandboxPolicy(const ISandboxPolicy&) = delete;
    ISandboxPolicy& operator=(const ISandboxPolicy&) = delete;
    ISandboxPolicy(ISandboxPolicy&&) = delete;
    ISandboxPolicy& operator=(ISandboxPolicy&&) = delete;
    virtual ~ISandboxPolicy() = default;

    // The attribute `name` of `obj`, a value of the engine, whether or not the engine has one of that name (a
    // method, or none): the replacement the sandbox reads instead (the unsafe undefined of a refused attribute, a
    // method the engine lacks), or nullopt to leave the engine's answer. Never asked for a host object
    [[nodiscard]] virtual std::optional<InternalValue> NativeAttribute(const InternalValue& obj, const std::string& name) const = 0;
    // s.format(*args, **kwargs), or s.format_map(mapping) when `formatMap`, of a string of the engine
    virtual InternalValue Format(const InternalValue& self, bool formatMap, const CallParams& params, RenderContext& context) const = 0;
};

} // namespace v2
} // namespace jinja2

#endif // JINJA2CPP_SRC_SANDBOX_POLICY_H
