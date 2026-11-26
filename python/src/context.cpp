#include "context.h"

#include "convert.h"
#include "errors.h"

#include <nanobind/stl/string.h>

#include <jinja2cpp/template.h>

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

nb::object MakeEvalContext(const CallLease& lease)
{
    auto* state = CurrentRender();
    if (!state || !state->environment)
    {
        RaiseTemplateRuntimeError("pass_eval_context needs a render of an environment");
    }
    return RuntimeFunction("_make_eval_context")(nb::borrow(state->environment), lease.autoescape);
}

ContextHandle::Part PartOf(int part)
{
    if (part < 0 || part > 4)
    {
        RaiseTypeError("a context part is 0 to 4");
    }
    return static_cast<ContextHandle::Part>(part);
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

nb::object MakePassArgument(PassArg kind)
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
    auto lease = LeaseScope::Current();
    if (!lease)
    {
        RaiseTemplateRuntimeError("pass_context needs a call of the template");
    }
    auto evalContext = MakeEvalContext(*lease);
    if (kind == PassArg::EvalContext)
    {
        return evalContext;
    }
    // The template's own name; None for a template made from a string
    nb::object name = lease->templateName.empty() ? nb::none() : MakeStr(lease->templateName);
    auto handle = nb::cast(new ContextHandle(std::move(lease)), nb::rv_policy::take_ownership);
    nb::object globals = state->templateGlobals ? nb::borrow(state->templateGlobals) : nb::none();
    return RuntimeFunction("_make_context")(handle, nb::borrow(state->environment), name, evalContext, globals);
}

ContextHandle::ContextHandle(std::shared_ptr<CallLease> lease)
    : m_lease(std::move(lease))
{
}

const jinja2::GenericMap& ContextHandle::Live() const
{
    const auto* context = m_lease && m_lease->active ? m_lease->context.GetIf<jinja2::GenericMap>() : nullptr;
    if (!context)
    {
        RaiseTemplateRuntimeError("the context was used after the call it was passed to ended");
    }
    return *context;
}

jinja2::Value ContextHandle::View(Part part) const
{
    const auto& context = Live();
    switch (part)
    {
    case Part::Vars:
        return jinja2::ContextView(jinja2::Value(context), jinja2::ContextNames::TopLevel);
    case Part::Parent:
        return jinja2::ContextView(jinja2::Value(context), jinja2::ContextNames::Parent);
    case Part::Body:
        return jinja2::ContextView(jinja2::Value(context), jinja2::ContextNames::BodyVars);
    case Part::All:
    case Part::Resolve:
        break;
    }
    return jinja2::Value(context);
}

nb::object ContextHandle::Lookup(const std::string& name, Part part, bool& found) const
{
    if (part == Part::Resolve)
    {
        // Jinja2's Context.call derives the context of a call in such a body, which puts the body's names over
        // the others; vars hide parent either way
        for (const auto each : { Part::Body, Part::Vars, Part::Parent })
        {
            auto value = Lookup(name, each, found);
            if (found)
            {
                return value;
            }
        }
        return nb::none();
    }
    const auto view = View(part);
    const auto* map = view.GetIf<jinja2::GenericMap>();
    const auto value = map ? map->Find(name) : std::nullopt;
    found = static_cast<bool>(value);
    if (!found)
    {
        return nb::none();
    }
    return ToPython(*value);
}

nb::list ContextHandle::Names(Part part) const
{
    if (part == Part::Resolve)
    {
        RaiseTypeError("a lookup part has no names");
    }
    nb::list result;
    const auto view = View(part);
    if (const auto* map = view.GetIf<jinja2::GenericMap>())
    {
        for (const auto& name : map->GetKeys())
        {
            result.append(MakeStr(name));
        }
    }
    return result;
}

void BindContext(nb::module_& module)
{
    nb::class_<ContextHandle>(module, "_ContextHandle")
        // part: 0 every name the template sees, 1 Context.vars, 2 Context.parent, 3 the body's names, 4 a lookup as
        // Context.resolve makes it (ContextHandle::Part)
        .def(
            "lookup",
            [](const ContextHandle& self, const std::string& name, int part) {
                bool found = false;
                auto value = self.Lookup(name, PartOf(part), found);
                return nb::make_tuple(found, std::move(value));
            },
            nb::arg("name"), nb::arg("part") = 0)
        .def(
            "names", [](const ContextHandle& self, int part) { return self.Names(PartOf(part)); }, nb::arg("part") = 0);
}
} // namespace jinja2cpp_py
