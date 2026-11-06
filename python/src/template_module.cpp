// Template.make_module, the macros a module exports, and the meta queries of a template
// (jinja2.meta): the parts of the Python layer (jinja2cpp/environment.py) that need the engine.
#include "common.h"
#include "convert.h"
#include "environment.h"
#include "errors.h"
#include "poll.h"

#include "host_object.h"
#include "internal_value.h"
#include "template_impl.h"

#include <nanobind/stl/string.h>

#include <jinja2cpp/generic_list_iterator.h>
#include <jinja2cpp/template.h>
#include <jinja2cpp/value.h>

#include <cstdint>
#include <string>
#include <unordered_set>
#include <utility>

namespace jinja2
{
inline namespace v2
{
// internal_value.cpp: a host Value as a value of the engine
InternalValue Value2IntValue(const Value& val);
InternalValue Value2IntValue(Value&& val);
} // namespace v2
} // namespace jinja2

namespace jinja2cpp_py
{
using namespace nb::literals;

namespace
{
// A Python value as a value of the host API, which a module takes by value, as Python's own data
// would be copied: str, int, float, bool, None, Markup, list, tuple and dict item by item (a
// container that contains itself is cut where it repeats, an empty Value, as in the engine's
// exports). A callable or any other object goes in as itself (host::EngineUserCallable is a Value
// the engine takes back as the object), so a method is called with Python's own call and an
// attribute is Python's
class ModuleParams
{
public:
    // Nested containers stop where the engine's own recursion does: when the stack runs low
    void CheckStack() const
    {
        // The engine copies a module's Values recursively with no guard of its own (0418): the
        // depth stops at Python's own default recursion limit, far below the stack
        if (m_depth > kMaxDepth || jinja2::StackNearlyExhausted())
        {
            PyErr_SetString(PyExc_RecursionError, "maximum recursion depth exceeded while converting a module argument");
            throw nb::python_error();
        }
    }

    jinja2::Value Convert(nb::handle object)
    {
        CheckStack();
        CountValue();
        PyObject* raw = object.ptr();
        if (raw == Py_None)
        {
            return jinja2::Value();
        }
        if (PyBool_Check(raw))
        {
            return jinja2::Value(raw == Py_True);
        }
        if (PyLong_CheckExact(raw))
        {
            int overflow = 0;
            const long long number = PyLong_AsLongLongAndOverflow(raw, &overflow);
            if (overflow == 0 && !(number == -1 && PyErr_Occurred()))
            {
                return jinja2::Value(static_cast<std::int64_t>(number));
            }
            PyErr_Clear();
        }
        else if (PyFloat_CheckExact(raw))
        {
            return jinja2::Value(PyFloat_AsDouble(raw));
        }
        else if (PyUnicode_CheckExact(raw))
        {
            return jinja2::Value(std::string(Utf8View(object)));
        }
        else if (PyList_CheckExact(raw) || PyTuple_CheckExact(raw))
        {
            if (!Enter(raw))
            {
                return jinja2::Value();
            }
            jinja2::ValuesList items;
            // A tuple becomes a list: the host API has no tuple. The items are read from a
            // snapshot that owns them: converting a dict key runs its __str__, which may change
            // this list
            const auto snapshot = nb::steal(PySequence_Tuple(raw));
            if (!snapshot)
            {
                throw nb::python_error();
            }
            const Py_ssize_t size = PyTuple_Size(snapshot.ptr());
            items.reserve(static_cast<size_t>(size));
            for (Py_ssize_t i = 0; i < size; ++i)
            {
                items.push_back(Convert(PyTuple_GetItem(snapshot.ptr(), i)));
            }
            Leave(raw);
            return jinja2::Value(std::move(items));
        }
        else if (PyDict_CheckExact(raw))
        {
            if (!Enter(raw))
            {
                return jinja2::Value();
            }
            jinja2::ValuesMap items;
            // A snapshot of the entries that owns them, for the same reason (a key's __str__ may
            // change the dict)
            const auto entries = nb::steal(PyDict_Items(raw));
            if (!entries)
            {
                throw nb::python_error();
            }
            const Py_ssize_t size = PyList_Size(entries.ptr());
            for (Py_ssize_t i = 0; i < size; ++i)
            {
                PyObject* entry = PyList_GetItem(entries.ptr(), i);
                items[Utf8(PyTuple_GetItem(entry, 0))] = Convert(PyTuple_GetItem(entry, 1));
            }
            Leave(raw);
            return jinja2::Value(std::move(items));
        }
        else if (nb::isinstance(object, Names().markupType))
        {
            return jinja2::Value(jinja2::Markup(Utf8(object)));
        }
        if (PyCallable_Check(raw))
        {
            return jinja2::Value(jinja2::host::EngineUserCallable(ToEngine(object)));
        }
        // Any other object stays itself in the module, as in a render: its attributes, lists and
        // methods are Python's (the Value carries the engine's own value of it back in)
        return jinja2::Value(jinja2::host::EngineUserCallable(ToEngine(object)));
    }

private:
    static constexpr std::size_t kMaxDepth = 1000;
    // The copy repeats a container each time it is reached (`x = [x, x]` thirty times is a
    // small graph and a billion copies): the values of one argument are counted, and a copy
    // that grows past this many stops before it allocates them
    static constexpr std::size_t kMaxValues = std::size_t{ 1 } << 20;
    void CountValue()
    {
        if (++m_values > kMaxValues)
        {
            PyErr_SetString(PyExc_MemoryError, "a module argument expands to more than 1048576 values");
            throw nb::python_error();
        }
    }
    bool Enter(PyObject* container)
    {
        if (!m_path.insert(container).second)
        {
            return false;
        }
        ++m_depth;
        return true;
    }
    void Leave(PyObject* container)
    {
        --m_depth;
        m_path.erase(container);
    }

    std::size_t m_depth = 0;
    std::size_t m_values = 0;
    std::unordered_set<PyObject*> m_path;
};

// A macro of a module (Template.make_module, Template.module): it runs in its module, which
// the callable keeps alive with the template, so it stays callable after the template, the
// module object and every other reference to them are gone (docs/tasks/0284). It holds the
// environment too, whose filters, tests and globals the macro may call
class ModuleMacro
{
public:
    ModuleMacro(jinja2::UserCallable callable, nb::object environment, nb::object templateGlobals, nb::object templateName, std::string name)
        : m_callable(std::move(callable))
        , m_environment(std::move(environment))
        , m_templateGlobals(std::move(templateGlobals))
        , m_templateName(std::move(templateName))
        , m_name(std::move(name))
    {
    }
    ModuleMacro(const ModuleMacro&) = delete;
    ModuleMacro& operator=(const ModuleMacro&) = delete;
    ModuleMacro(ModuleMacro&&) = delete;
    ModuleMacro& operator=(ModuleMacro&&) = delete;
    ~ModuleMacro() = default;

    nb::object Call(nb::args args, nb::kwargs kwargs) const
    {
        if (!m_environment.is_valid() || m_environment.is_none())
        {
            RaiseTemplateRuntimeError("the macro was cleared by the garbage collector");
        }
        auto& env = nb::cast<PyEnvironment&>(m_environment);
        env.CheckAlive();
        // One budget for all the arguments of the call
        ModuleParams convert;
        jinja2::ValuesList positional;
        positional.reserve(args.size());
        for (auto arg : args)
        {
            positional.push_back(convert.Convert(arg));
        }
        jinja2::ValuesMap keywords;
        for (auto [name, value] : kwargs)
        {
            keywords[Utf8(name)] = convert.Convert(value);
        }
        jinja2::UserCallableParams params;
        params.extraPosArgs = jinja2::Value(std::move(positional));
        params.extraKwArgs = jinja2::Value(std::move(keywords));
        RenderState state;
        state.undefinedClass = env.UndefinedClass();
        // What pass_environment, pass_eval_context and pass_context callables of the macro need
        state.environment = m_environment.ptr();
        state.templateGlobals = m_templateGlobals.ptr();
        state.sandbox = SandboxOf(m_environment);
        const RenderScope scope(state);
        const HostPollScope poll;
        try
        {
            return ToPython(jinja2::Value2IntValue(m_callable.callable(params)));
        }
        catch (const jinja2::BadResultAccess<char>& failure)
        {
            ErrorSource where;
            where.name = m_templateName;
            RaiseError(failure.error(), where);
        }
    }

    [[nodiscard]] const std::string& Name() const { return m_name; }
    [[nodiscard]] std::string Repr() const { return "<macro '" + m_name + "'>"; }

    int Traverse(visitproc visit, void* arg) const
    {
        Py_VISIT(m_environment.ptr());
        Py_VISIT(m_templateGlobals.ptr());
        Py_VISIT(m_templateName.ptr());
        return 0;
    }
    void Clear() noexcept
    {
        m_environment = nb::none();
        m_templateGlobals = nb::none();
        m_templateName = nb::none();
        m_callable = jinja2::UserCallable();
    }

private:
    jinja2::UserCallable m_callable;
    nb::object m_environment;
    nb::object m_templateGlobals;
    nb::object m_templateName;
    std::string m_name;
};

// An exported value as Python: the macros, also in lists and dicts, stay callable
// (ModuleMacro); the rest as the engine's values become Python's (ToPython)
nb::object ExportToPython(const jinja2::Value& value, const nb::object& environment, const nb::object& templateGlobals, const nb::object& templateName, const std::string& name)
{
    if (jinja2::StackNearlyExhausted())
    {
        PyErr_SetString(PyExc_RecursionError, "maximum recursion depth exceeded while converting a module export");
        throw nb::python_error();
    }
    if (const auto* callable = value.GetIf<jinja2::UserCallable>())
    {
        return nb::cast(new ModuleMacro(*callable, environment, templateGlobals, templateName, name), nb::rv_policy::take_ownership);
    }
    if (const auto* list = value.GetIf<jinja2::ValuesList>())
    {
        nb::list result;
        for (const auto& item : *list)
        {
            result.append(ExportToPython(item, environment, templateGlobals, templateName, name));
        }
        return result;
    }
    if (const auto* list = value.GetIf<jinja2::GenericList>())
    {
        nb::list result;
        for (const auto& item : *list)
        {
            result.append(ExportToPython(item, environment, templateGlobals, templateName, name));
        }
        return result;
    }
    if (const auto* map = value.GetIf<jinja2::ValuesMap>())
    {
        nb::dict result;
        for (const auto& [key, item] : *map)
        {
            result[MakeStr(key)] = ExportToPython(item, environment, templateGlobals, templateName, name);
        }
        return result;
    }
    if (const auto* map = value.GetIf<jinja2::GenericMap>())
    {
        nb::dict result;
        for (const auto& [key, item] : *map)
        {
            result[MakeStr(key)] = ExportToPython(item, environment, templateGlobals, templateName, name);
        }
        return result;
    }
    return ToPython(jinja2::Value2IntValue(value));
}

int MacroTraverse(PyObject* self, visitproc visit, void* arg)
{
    Py_VISIT(Py_TYPE(self));
    if (!nb::inst_ready(self))
    {
        return 0;
    }
    return nb::inst_ptr<ModuleMacro>(self)->Traverse(visit, arg);
}

int MacroClear(PyObject* self)
{
    if (nb::inst_ready(self))
    {
        nb::inst_ptr<ModuleMacro>(self)->Clear();
    }
    return 0;
}

PyType_Slot g_macroSlots[] = { { Py_tp_traverse, reinterpret_cast<void*>(&MacroTraverse) }, { Py_tp_clear, reinterpret_cast<void*>(&MacroClear) }, { 0, nullptr } };
} // namespace

PyEnvironment& PyTemplate::LiveEnvironment() const
{
    if (!m_environment.is_valid() || m_environment.is_none())
    {
        RaiseTemplateRuntimeError("the template was cleared by the garbage collector");
    }
    auto& env = nb::cast<PyEnvironment&>(m_environment);
    env.CheckAlive();
    return env;
}

nb::tuple PyTemplate::MakeModule(nb::handle params) const
{
    auto& env = LiveEnvironment();
    if (!PyDict_Check(params.ptr()))
    {
        RaiseTypeError("make_module takes a dict of variables");
    }
    ModuleParams convert;
    jinja2::ValuesMap values;
    for (auto [name, value] : nb::borrow<nb::dict>(params))
    {
        if (PyUnicode_Check(name.ptr()))
        {
            values[std::string(Utf8View(name))] = convert.Convert(value);
        }
    }
    RenderState state;
    state.undefinedClass = env.UndefinedClass();
    state.environment = m_environment.ptr();
    state.templateGlobals = m_globals.ptr();
    state.sandbox = SandboxOf(m_environment);
    const RenderScope scope(state);
    const HostPollScope poll;
    auto result = m_template.MakeModule(values);
    if (!result)
    {
        ErrorSource where;
        where.name = m_name;
        where.filename = m_filename;
        RaiseError(result.error(), where);
    }
    auto module = std::move(result).value();
    nb::dict exports;
    for (auto& [name, value] : module.exports)
    {
        exports[MakeStr(name)] = ExportToPython(value, m_environment, m_globals, m_name, name);
    }
    return nb::make_tuple(MakeStr(module.body), exports);
}

nb::object PyTemplate::FindUndeclared() const
{
    LiveEnvironment();
    auto result = m_template.FindUndeclaredVariables();
    if (!result)
    {
        ErrorSource where;
        where.name = m_name;
        where.filename = m_filename;
        RaiseError(result.error(), where);
    }
    nb::list names;
    for (const auto& name : *result)
    {
        names.append(MakeStr(name));
    }
    return names;
}

nb::object PyTemplate::FindReferenced() const
{
    LiveEnvironment();
    auto result = jinja2::TemplateImpl<char>::ReferencedTemplatesOf(m_template);
    if (!result)
    {
        ErrorSource where;
        where.name = m_name;
        where.filename = m_filename;
        RaiseError(result.error(), where);
    }
    nb::list names;
    for (const auto& name : *result)
    {
        names.append(name ? MakeStr(*name) : nb::none());
    }
    return names;
}

void BindTemplateModule(nb::module_& module)
{
    nb::class_<ModuleMacro>(module, "ModuleMacro", nb::type_slots(g_macroSlots), nb::is_weak_referenceable())
        .def("__call__", &ModuleMacro::Call)
        .def("__repr__", &ModuleMacro::Repr)
        .def_prop_ro("name", &ModuleMacro::Name);
}
} // namespace jinja2cpp_py
