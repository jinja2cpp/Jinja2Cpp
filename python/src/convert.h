#ifndef JINJA2CPP_PYTHON_CONVERT_H
#define JINJA2CPP_PYTHON_CONVERT_H

#include "common.h"

#include <jinja2cpp/host_object.h>
#include <jinja2cpp/user_callable.h>
#include <jinja2cpp/value.h>

#include <cstdint>
#include <exception>
#include <memory>
#include <string>

namespace jinja2cpp_py
{
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
    // The environment's Limits::maxSequenceSize (0 for the engine's built-in cap), which the sandbox's
    // formatter checks the strings it builds against (_check_sequence_size)
    std::uint64_t maxSequenceSize = 0;
    // A Python BaseException outside Exception (KeyboardInterrupt, SystemExit) that a callable raised: the
    // engine's fold of a finalized output absorbs a failed call as Jinja2's `except Exception` does, so the
    // render's poll and its end raise it again (Jinja2 lets it through)
    std::exception_ptr abort;
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
};

// Runs the sandbox environment's `_check_native_hooks` (jinja2cpp/sandbox.py): an override of call, getattr or getitem
// set after a template loaded is refused at each entry into the template's code, as at a load
void CheckNativeSandboxHooks(PyObject* sandbox);

// What a Python call the engine makes was handed besides its arguments: the template's context and the call site,
// valid for the duration of the call only (UserCallableParams::context). A pass_context argument reads them while
// the call runs and raises once it returned
struct CallLease
{
    bool active = true;
    jinja2::Value context;
    std::string templateName;
    bool autoescape = false;
};

// Installs a lease for the Python call it lives for and ends it after the call
class LeaseScope
{
public:
    explicit LeaseScope(const jinja2::UserCallableParams& params) noexcept;
    ~LeaseScope();
    LeaseScope(const LeaseScope&) = delete;
    LeaseScope& operator=(const LeaseScope&) = delete;
    LeaseScope(LeaseScope&&) = delete;
    LeaseScope& operator=(LeaseScope&&) = delete;

    // The lease of the innermost call on this thread, made on first use; null outside a call
    static std::shared_ptr<CallLease> Current();

private:
    const jinja2::UserCallableParams& m_params;
    std::shared_ptr<CallLease> m_lease;
    LeaseScope* m_outer;
};

// A value of the template that has no Python type of its own, seen from Python: a macro, `caller`,
// `loop`, a cycler, a namespace. A macro or `caller` is lent by the engine for the call it was passed
// to and raises once that call returned (docs/tasks/0300); `loop` and the other objects are snapshots
// that stay readable (the plan's D5)
class TemplateValue
{
public:
    explicit TemplateValue(jinja2::Value value);
    ~TemplateValue() = default;
    TemplateValue(const TemplateValue&) = delete;
    TemplateValue& operator=(const TemplateValue&) = delete;
    TemplateValue(TemplateValue&&) = delete;
    TemplateValue& operator=(TemplateValue&&) = delete;

    // The value, or a TemplateRuntimeError once the garbage collector cleared it
    [[nodiscard]] const jinja2::Value& Value() const;
    void Expire() noexcept;

    nb::object Call(nb::args args, nb::kwargs kwargs) const;
    nb::object GetAttr(const std::string& name) const;
    [[nodiscard]] std::string Str() const;
    [[nodiscard]] std::string Repr() const;
    // type(x).__name__ in Jinja2 (macro, LoopContext, ...)
    [[nodiscard]] std::string TypeName() const;

private:
    jinja2::Value m_value;
    bool m_alive = true;
};

// Python -> engine (0248 D4): scalars natively, a str copied to UTF-8, Markup as Markup, an
// Undefined as undefined, a TemplateValue as its value, anything else as a host object
jinja2::Value ToEngine(nb::handle object);
// The same, written where the engine reads a host object's attribute or item (IHostObject::GetAttrTo)
void WriteToEngine(nb::handle object, jinja2::HostResult& out);
// Engine -> Python: values from Python as the same object, scalars natively, a template's lists
// and dicts as list and dict, Markup as markupsafe.Markup, undefined as an instance of the
// render's Undefined class, macros and other template objects as TemplateValue
nb::object ToPython(const jinja2::Value& value);

// A callable of the engine that calls `object`, which it borrows: the environment's filters, tests,
// finalize and translation callables, which the environment keeps alive (environment.h). An undefined
// argument reaches Python as an Undefined
jinja2::UserCallable BorrowedCallable(nb::handle object);

} // namespace jinja2cpp_py

#endif // JINJA2CPP_PYTHON_CONVERT_H
