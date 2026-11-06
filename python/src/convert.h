#ifndef JINJA2CPP_PYTHON_CONVERT_H
#define JINJA2CPP_PYTHON_CONVERT_H

#include "common.h"

#include "host_object.h"
#include "internal_value.h"

#include <memory>
#include <vector>

namespace jinja2
{
inline namespace v2
{
class RenderContext;
class CycleCollector;
}
} // namespace jinja2

namespace jinja2cpp_py
{
using jinja2::InternalValue;

// What a render on this thread converts with: the Undefined class of its environment, which an
// undefined value passed to Python becomes (Template.render installs it, RenderScope)
struct RenderState
{
    PyObject* undefinedClass = nullptr; // borrowed from the environment, which the render holds
    // The jinja2cpp.Environment of the render (pass_environment, Context.environment) and the
    // dict of the template's own globals (Context.get_all); borrowed from the template, which
    // the render holds
    PyObject* environment = nullptr;
    PyObject* templateGlobals = nullptr;
    // The environment when it is sandboxed (SandboxOf), whose is_safe_attribute and is_safe_callable the
    // render asks for every attribute and call of a Python object (0354, the plan's D8); null for any other
    PyObject* sandbox = nullptr;
    RenderState* outer = nullptr;
};
RenderState* CurrentRender() noexcept;
// `environment` when its `sandboxed` attribute is true, else null
PyObject* SandboxOf(nb::handle environment);
// Sets the fields of `state` that depend on the environment object
void InitRenderState(RenderState& state, nb::handle environment);
// Whether this thread is known to hold the GIL (a render runs): PyRef::Release then needs no
// PyGILState_Ensure, which the stable ABI has instead of PyGILState_Check
bool HoldsGil() noexcept;

class RenderScope
{
public:
    explicit RenderScope(RenderState& state) noexcept;
    ~RenderScope();
    RenderScope(const RenderScope&) = delete;
    RenderScope& operator=(const RenderScope&) = delete;
    RenderScope(RenderScope&&) = delete;
    RenderScope& operator=(RenderScope&&) = delete;

private:
    RenderState& m_state;
    // The sandbox of a SandboxedEnvironment render (RenderState::sandbox)
    jinja2::HostSandboxScope m_sandbox;
};

class TemplateValue;

// The template values a Python call got as arguments (a macro, `caller`, `loop`), lent for the
// duration of that call: they refer to the render that made them, which may be gone once the
// call returns. Afterwards each one is empty and using it raises (as the engine's own lease,
// docs/tasks/0300)
struct CallLease
{
    jinja2::RenderContext* context = nullptr;
    // The render's, which the values' code notes its writes to
    jinja2::CycleCollector* collector = nullptr;
    bool active = true;
    std::vector<TemplateValue*> values;
};

// Installs a lease for the Python call it lives for and ends it after the call
class LeaseScope
{
public:
    explicit LeaseScope(jinja2::RenderContext& context) noexcept;
    ~LeaseScope();
    LeaseScope(const LeaseScope&) = delete;
    LeaseScope& operator=(const LeaseScope&) = delete;
    LeaseScope(LeaseScope&&) = delete;
    LeaseScope& operator=(LeaseScope&&) = delete;

    // The lease of the innermost call on this thread, made on first use; null outside a call
    static std::shared_ptr<CallLease> Current();

private:
    jinja2::RenderContext& m_context;
    std::shared_ptr<CallLease> m_lease;
    LeaseScope* m_outer;
};

// A value of the template that has no Python type of its own, seen from Python: a macro,
// `caller`, `loop`, a cycler, a namespace. It holds the value while its lease is active
class TemplateValue
{
public:
    TemplateValue(InternalValue value, std::shared_ptr<CallLease> lease);
    ~TemplateValue();
    TemplateValue(const TemplateValue&) = delete;
    TemplateValue& operator=(const TemplateValue&) = delete;
    TemplateValue(TemplateValue&&) = delete;
    TemplateValue& operator=(TemplateValue&&) = delete;

    // The value, or a TemplateRuntimeError once its call ended
    [[nodiscard]] const InternalValue& Value() const;
    [[nodiscard]] bool Alive() const noexcept { return m_alive; }
    void Expire() noexcept;

    nb::object Call(nb::args args, nb::kwargs kwargs) const;
    nb::object GetAttr(const std::string& name) const;
    [[nodiscard]] std::string Str() const;
    [[nodiscard]] std::string Repr() const;
    // type(x).__name__ in Jinja2 (macro, LoopContext, ...)
    [[nodiscard]] std::string TypeName() const;

private:
    InternalValue m_value;
    std::shared_ptr<CallLease> m_lease;
    bool m_alive = true;
};

// Python -> engine (0248 D4): scalars natively, a str copied to UTF-8, Markup as Markup, an
// Undefined as undefined, a TemplateValue as its value, anything else as a host object
InternalValue ToEngine(nb::handle object);
// Engine -> Python: values from Python as the same object, scalars natively, a template's lists
// and dicts as list and dict, Markup as markupsafe.Markup, undefined as an instance of the
// render's Undefined class, macros and other template objects as TemplateValue
nb::object ToPython(const InternalValue& value);

// A host value of `object` the engine does not own a reference to: the environment's filters,
// tests and finalize, which the environment keeps alive (environment.h)
InternalValue BorrowedHostValue(nb::handle object);
// A callable of the engine (kind UserCallable) that calls `object`, which it borrows
InternalValue BorrowedCallable(nb::handle object);

} // namespace jinja2cpp_py

#endif // JINJA2CPP_PYTHON_CONVERT_H
