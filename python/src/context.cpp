#include "context.h"

#include "convert.h"
#include "errors.h"

#include "render_context.h"

#include <nanobind/stl/string.h>

#include <string>
#include <utility>

namespace jinja2cpp_py
{
namespace
{
PyObject* PassArgName()
{
    // Interned once; the interpreter owns it for good
    static PyObject* name = PyUnicode_InternFromString("jinja_pass_arg");
    return name;
}

PyObject* CallName()
{
    static PyObject* name = PyUnicode_InternFromString("__call__");
    return name;
}

// The kind a marker object names (a _PassArg member); None for anything else
PassArg KindOf(PyObject* marker)
{
    PyObject* name = PyObject_GetAttrString(marker, "name");
    if (!name)
    {
        PyErr_Clear();
        return PassArg::None;
    }
    const auto owned = nb::steal(name);
    if (!PyUnicode_Check(name))
    {
        return PassArg::None;
    }
    const auto text = Utf8View(owned);
    if (text == "context")
    {
        return PassArg::Context;
    }
    if (text == "eval_context")
    {
        return PassArg::EvalContext;
    }
    if (text == "environment")
    {
        return PassArg::Environment;
    }
    return PassArg::None;
}

// The marker of `object`, if it has one
PassArg MarkerOf(PyObject* object)
{
    if (PyObject_HasAttr(object, PassArgName()) == 0)
    {
        return PassArg::None;
    }
    PyObject* marker = PyObject_GetAttr(object, PassArgName());
    if (!marker)
    {
        PyErr_Clear();
        return PassArg::None;
    }
    const auto owned = nb::steal(marker);
    return KindOf(marker);
}

nb::object RuntimeFunction(const char* name)
{
    return nb::module_::import_("jinja2cpp.runtime").attr(name);
}

nb::object MakeEvalContext(jinja2::RenderContext& context)
{
    auto* state = CurrentRender();
    if (!state || !state->environment)
    {
        RaiseTemplateRuntimeError("pass_eval_context needs a render of an environment");
    }
    return RuntimeFunction("_make_eval_context")(nb::borrow(state->environment), context.IsAutoescape());
}
} // namespace

PassArg PassArgOf(PyObject* callable)
{
    if (const auto kind = MarkerOf(callable); kind != PassArg::None)
    {
        return kind;
    }
    // An object whose __call__ is marked: looked up on the type, which binds nothing
    PyObject* call = PyObject_GetAttr(reinterpret_cast<PyObject*>(Py_TYPE(callable)), CallName());
    if (!call)
    {
        PyErr_Clear();
        return PassArg::None;
    }
    const auto owned = nb::steal(call);
    return MarkerOf(call);
}

nb::object MakePassArgument(PassArg kind, jinja2::RenderContext& context)
{
    auto* state = CurrentRender();
    if (kind == PassArg::Environment)
    {
        if (!state || !state->environment)
        {
            RaiseTemplateRuntimeError("pass_environment needs a render of an environment");
        }
        return nb::borrow(state->environment);
    }
    auto evalContext = MakeEvalContext(context);
    if (kind == PassArg::EvalContext)
    {
        return evalContext;
    }
    auto lease = LeaseScope::Current();
    if (!lease)
    {
        RaiseTemplateRuntimeError("pass_context needs a call of the template");
    }
    // The template's own name; None for a template made from a string
    nb::object name = nb::none();
    if (const auto* frame = context.GetTemplateFrame(); frame && !frame->name.empty())
    {
        name = MakeStr(frame->name);
    }
    auto handle = nb::cast(new ContextHandle(std::move(lease)), nb::rv_policy::take_ownership);
    nb::object globals = state->templateGlobals ? nb::borrow(state->templateGlobals) : nb::none();
    return RuntimeFunction("_make_context")(handle, nb::borrow(state->environment), name, evalContext, globals);
}

ContextHandle::ContextHandle(std::shared_ptr<CallLease> lease)
    : m_lease(std::move(lease))
{
}

jinja2::RenderContext& ContextHandle::Live() const
{
    if (!m_lease || !m_lease->active || !m_lease->context)
    {
        RaiseTemplateRuntimeError("the context was used after the call it was passed to ended");
    }
    return *m_lease->context;
}

nb::object ContextHandle::Lookup(const std::string& name, bool& found) const
{
    auto& context = Live();
    const auto value = context.FindValue(name);
    found = static_cast<bool>(value);
    if (!found)
    {
        return nb::none();
    }
    return ToPython(*value);
}

nb::list ContextHandle::Names() const
{
    auto& context = Live();
    jinja2::InternalValueMap names;
    context.CollectVisibleNames(names);
    nb::list result;
    for (const auto& entry : names)
    {
        result.append(MakeStr(entry.first));
    }
    return result;
}

void BindContext(nb::module_& module)
{
    nb::class_<ContextHandle>(module, "_ContextHandle")
        .def("lookup",
             [](const ContextHandle& self, const std::string& name) {
                 bool found = false;
                 auto value = self.Lookup(name, found);
                 return nb::make_tuple(found, std::move(value));
             })
        .def("names", &ContextHandle::Names);
}
} // namespace jinja2cpp_py
