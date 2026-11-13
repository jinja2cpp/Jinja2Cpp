#ifndef JINJA2CPP_SANDBOX_POLICY_H
#define JINJA2CPP_SANDBOX_POLICY_H

#include "config.h"
#include "user_callable.h"
#include "value.h"

#include <optional>
#include <string_view>

JINJA2CPP_SUPPRESS_DLL_INTERFACE_WARNINGS_BEGIN

namespace jinja2
{
inline namespace v2
{
/*!
 * \brief A host's sandbox for the values of the engine (Jinja2's `SandboxedEnvironment`)
 *
 * Set as \ref Settings::sandboxPolicy, it decides what a template may do with the engine's own values: strings,
 * numbers, lists, dicts, and template objects such as `loop`, a macro or a namespace. A host object (\ref IHostObject)
 * is never asked about: it applies its own policy in \ref IHostObject::GetAttr and \ref IHostObject::Call. Unset,
 * the default, a render pays one null check where a policy would be asked.
 *
 * The policy is asked:
 * - for every attribute or method a template reads from a value of the engine by name (`s.upper`, `l.append`,
 *   `x|attr('n')`, `loop.index`), found or not, before the engine's own answer is used;
 * - for `str.format` and `str.format_map`, which it may perform itself (\ref Format).
 *
 * Together with \ref Settings::immutableContainers, \ref ErrorCode::SecurityError and the `security` flag of
 * \ref UndefinedDescription it is what the Python module's `SandboxedEnvironment` and
 * `ImmutableSandboxedEnvironment` are built on (docs/python.md). A template keeps the policy it was loaded with,
 * as it keeps its other settings. Called from any thread that renders, so it must be thread-safe.
 */
class JINJA2CPP_EXPORT ISandboxPolicy
{
public:
    ISandboxPolicy() = default;
    ISandboxPolicy(const ISandboxPolicy&) = delete;
    ISandboxPolicy& operator=(const ISandboxPolicy&) = delete;
    ISandboxPolicy(ISandboxPolicy&&) = delete;
    ISandboxPolicy& operator=(ISandboxPolicy&&) = delete;
    virtual ~ISandboxPolicy();

    /*!
     * \brief The attribute `name` of `obj`, as the sandbox lets the template read it
     *
     * Jinja2's `is_safe_attribute`: return `std::nullopt` to leave the engine's answer (the attribute, the bound
     * method, or an undefined value when there is none), or the value the template reads instead. A refused name
     * is usually `MakeUndefined` with `security` set, whose use fails with \ref ErrorCode::SecurityError; a value
     * the engine lacks (a method of Python's `str`) may be any value, a \ref UserCallable among them.
     * The engine answers `__class__` of its own values with their Python class (`<class 'str'>`): a policy that
     * leaves the name alone lets the template read it, where Jinja2 refuses every name with an underscore.
     *
     * @param obj   The value, as a host callable gets it: `loop` and a namespace as a snapshot of their
     *              attributes (\ref GenericMap::GetObjectClass names their class), a macro, `caller` or a
     *              global function such as `range` as an empty map of class "Macro" or "function"
     * @param name  The attribute's name
     */
    [[nodiscard]] virtual std::optional<Value> NativeAttribute(const Value& obj, std::string_view name) const = 0;

    /*!
     * \brief `self.format(*args, **kwargs)`, or `self.format_map(mapping)` when `formatMap`, in the sandbox
     *
     * Return the result, or `std::nullopt`, the default, for the engine's own `str.format`, which then reads the
     * attribute a format field names (`{0.name}`) through \ref NativeAttribute, as Jinja2's `SandboxedFormatter`
     * does: a refused one formats as its undefined value. `format_map` asks with `formatMap` set, its one
     * positional argument the mapping.
     *
     * @param self       The string
     * @param formatMap  Whether the call is `format_map`, whose one argument is the mapping
     * @param params     The call's arguments, as a callable declared with ArgInfo::VarArgs and ArgInfo::VarKwArgs
     *                   (and UserCallable::keepUndefined) gets them
     */
    [[nodiscard]] virtual std::optional<Value> Format(const Value& self, bool formatMap, const UserCallableParams& params) const;
};
} // namespace v2
} // namespace jinja2

JINJA2CPP_SUPPRESS_DLL_INTERFACE_WARNINGS_END

#endif // JINJA2CPP_SANDBOX_POLICY_H
