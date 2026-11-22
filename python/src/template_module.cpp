// Template.make_module, the macros a module exports, and the meta queries of a template
// (jinja2.meta): the parts of the Python layer (jinja2cpp/environment.py) that need the engine.
#include "adapters.h"
#include "common.h"
#include "convert.h"
#include "environment.h"
#include "errors.h"
#include "poll.h"

#include <nanobind/stl/string.h>

#include <jinja2cpp/generic_list_iterator.h>
#include <jinja2cpp/host_object.h>
#include <jinja2cpp/template.h>
#include <jinja2cpp/value.h>

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>

namespace jinja2cpp_py
{
using namespace nb::literals;

namespace
{
// A Python value as a value of the host API, which a module takes by value, as Python's own data
// would be copied: str, int, float, bool, None, Markup, list, tuple and dict item by item (a
// container that contains itself is cut where it repeats, an empty Value, as in the engine's
// exports). A callable or any other object goes in as itself (a host object, ToEngine), so a
// method is called with Python's own call and an attribute is Python's
class ModuleParams
{
public:
    // Nested containers stop at a depth
    void CheckStack() const
    {
        // The engine copies a module's Values recursively with no guard of its own (0418): the
        // depth stops at Python's own default recursion limit, far below the stack
        if (m_depth > kMaxDepth)
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
        // A callable or any other object stays itself in the module, as in a render: its attributes,
        // lists and methods are Python's
        return ToEngine(object);
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

// Whether `object` is an EvalContext of jinja2cpp or Jinja2 (jinja2.nodes.EvalContext) or of a subclass
bool IsEvalContext(nb::handle object)
{
    const auto mro = nb::steal(PyObject_GetAttrString(reinterpret_cast<PyObject*>(Py_TYPE(object.ptr())), "__mro__"));
    if (!mro)
    {
        PyErr_Clear();
        return false;
    }
    for (auto base : mro)
    {
        if (IsJinjaClass(base, "EvalContext"))
        {
            return true;
        }
    }
    return false;
}

// ModuleOptions::waitForTurn: a thread that waits for another thread's run in the module lets go of the GIL meanwhile,
// which that thread's Python callables may need. Called on an entry into the engine, which holds the GIL (RenderScope)
void WaitWithoutGil(const std::function<void()>& wait)
{
    if (!HoldsGil())
    {
        wait();
        return;
    }
    PyThreadState* saved = PyEval_SaveThread();
    try
    {
        wait();
    }
    catch (...)
    {
        PyEval_RestoreThread(saved);
        throw;
    }
    PyEval_RestoreThread(saved);
}

// What a macro or an object of a module needs to run in it, as ExportToPython passes it on
struct ExportContext
{
    nb::object bound;
    nb::object environment;
    nb::object templateGlobals;
    nb::object templateName;
};

nb::object ExportToPython(const jinja2::Value& value, const ExportContext& context, const std::string& name);

// A macro of a module (Template.make_module, Template.module): it runs in its module, which
// the callable keeps alive with the template, so it stays callable after the template, the
// module object and every other reference to them are gone (docs/tasks/0284). It holds the
// environment too, whose filters, tests and globals the macro may call, and what the engine environment the template
// was loaded from borrows (PyEnvironment::BoundObjects): the environment may have moved on to a new list since
class ModuleMacro
{
public:
    ModuleMacro(jinja2::UserCallable callable, const ExportContext& context, std::string name)
        : m_bound(context.bound)
        , m_callable(std::move(callable))
        , m_environment(context.environment)
        , m_templateGlobals(context.templateGlobals)
        , m_templateName(context.templateName)
        , m_name(std::move(name))
    {
        // Jinja2's Macro attributes, read once; the macro's own name, not the name it is exported under
        const auto attributes = jinja2::MacroAttributes(m_callable);
        const auto flag = [&attributes](const char* key) {
            const auto pos = attributes.find(key);
            const auto* value = pos == attributes.end() ? nullptr : pos->second.GetIf<bool>();
            return value && *value;
        };
        if (const auto pos = attributes.find("name"); pos != attributes.end() && pos->second.IsString())
        {
            m_name = pos->second.AsString();
        }
        if (const auto pos = attributes.find("arguments"); pos != attributes.end())
        {
            if (const auto* names = pos->second.GetIf<jinja2::ValuesList>())
            {
                for (const auto& item : *names)
                {
                    m_arguments.push_back(item.AsString());
                }
            }
        }
        m_catchKwargs = flag("catch_kwargs");
        m_catchVarargs = flag("catch_varargs");
        m_caller = flag("caller");
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
        const auto entry = env.Enter();
        // Jinja2's Macro.__call__: an EvalContext as the first argument is not an argument of the macro, and its
        // autoescape decides whether the result is Markup
        int autoescape = -1;
        size_t first = 0;
        if (args.size() != 0 && IsEvalContext(args[0]))
        {
            autoescape = PyObject_IsTrue(nb::getattr(args[0], "autoescape").ptr());
            if (autoescape < 0)
            {
                throw nb::python_error();
            }
            first = 1;
        }
        // One budget for all the arguments of the call
        ModuleParams convert;
        jinja2::ValuesList positional;
        positional.reserve(args.size() - first);
        for (size_t i = first; i < args.size(); ++i)
        {
            positional.push_back(convert.Convert(args[i]));
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
        state.sandboxAllNames = entry.sandboxAllNames;
        const RenderScope scope(state);
        // env.policies as they are now, as Jinja2 reads them each time a filter runs (docs/tasks/0557)
        jinja2::ModuleCallOptions callOptions;
        callOptions.filterPolicies = entry.policies;
        try
        {
            auto result = ExportToPython(jinja2::CallModuleMacro(m_callable, params, callOptions), Context(), m_name);
            RaiseAbort(state);
            if (autoescape >= 0 && PyUnicode_Check(result.ptr()))
            {
                const bool markup = nb::isinstance(result, Names().markupType);
                if (autoescape != 0 && !markup)
                {
                    return Names().markupType(result);
                }
                if (autoescape == 0 && markup)
                {
                    return nb::steal(PyObject_Str(result.ptr()));
                }
            }
            return result;
        }
        catch (const jinja2::BadResultAccess<char>& failure)
        {
            ErrorSource where;
            where.environment = m_environment;
            where.name = m_templateName;
            RaiseError(failure.error(), where);
        }
    }

    [[nodiscard]] ExportContext Context() const { return ExportContext{ m_bound, m_environment, m_templateGlobals, m_templateName }; }
    [[nodiscard]] const std::string& Name() const { return m_name; }
    [[nodiscard]] nb::tuple Arguments() const
    {
        nb::list names;
        for (const auto& argument : m_arguments)
        {
            names.append(nb::str(argument.c_str(), argument.size()));
        }
        return nb::tuple(names);
    }
    [[nodiscard]] bool CatchKwargs() const { return m_catchKwargs; }
    [[nodiscard]] bool CatchVarargs() const { return m_catchVarargs; }
    [[nodiscard]] bool Caller() const { return m_caller; }
    [[nodiscard]] std::string Repr() const { return "<macro '" + m_name + "'>"; }

    int Traverse(visitproc visit, void* arg) const
    {
        Py_VISIT(m_bound.ptr());
        Py_VISIT(m_environment.ptr());
        Py_VISIT(m_templateGlobals.ptr());
        Py_VISIT(m_templateName.ptr());
        return 0;
    }
    void Clear() noexcept
    {
        // The callable first: it borrows from m_bound
        m_callable = jinja2::UserCallable();
        m_bound = nb::none();
        m_environment = nb::none();
        m_templateGlobals = nb::none();
        m_templateName = nb::none();
    }

private:
    // Declared before m_callable, so it goes after it
    nb::object m_bound;
    jinja2::UserCallable m_callable;
    nb::object m_environment;
    nb::object m_templateGlobals;
    nb::object m_templateName;
    std::string m_name;
    std::vector<std::string> m_arguments;
    bool m_catchKwargs = false;
    bool m_catchVarargs = false;
    bool m_caller = false;
};

// Whether the engine's object class `objectClass` (or null) is one a module exports as a live object
bool IsModuleObjectClass(const char* objectClass)
{
    if (!objectClass)
    {
        return false;
    }
    const std::string_view name(objectClass);
    return name == "Namespace" || name == "Cycler" || name == "Joiner";
}

// A namespace, a cycler or a joiner of a module (Template.make_module, Template.module): the module's own object, not a
// copy, so that `module.ns.count` is what the macros set last and `module.cyc.next()` advances the cycler they read
// (docs/tasks/0411). It holds the engine's view of the object, which holds the module: like a macro it outlives the
// template and every other reference to the module. Every read runs in the module, as a macro call does, and gives Python
// a copy of the attribute (a nested namespace is a new object each time it is read)
class ModuleObject
{
public:
    ModuleObject(jinja2::Value value, const ExportContext& context, std::string name)
        : m_bound(context.bound)
        , m_value(std::move(value))
        , m_environment(context.environment)
        , m_templateGlobals(context.templateGlobals)
        , m_templateName(context.templateName)
        , m_name(std::move(name))
    {
    }
    ModuleObject(const ModuleObject&) = delete;
    ModuleObject& operator=(const ModuleObject&) = delete;
    ModuleObject(ModuleObject&&) = delete;
    ModuleObject& operator=(ModuleObject&&) = delete;
    ~ModuleObject() = default;

    [[nodiscard]] ExportContext Context() const { return ExportContext{ m_bound, m_environment, m_templateGlobals, m_templateName }; }
    // The engine's value, which the host API passes back to the module (ToEngine)
    [[nodiscard]] const jinja2::Value& Value() const { return m_value; }

    nb::object GetAttr(const std::string& name) const
    {
        return Read([&] {
            const auto* map = m_value.GetIf<jinja2::GenericMap>();
            std::optional<jinja2::Value> found = map ? map->Find(name) : std::nullopt;
            if (!found)
            {
                PyErr_Format(PyExc_AttributeError, "'%s' object has no attribute '%s'", ClassName(), name.c_str());
                throw nb::python_error();
            }
            return ExportToPython(*found, Context(), m_name + "." + name);
        });
    }

    // Jinja2's Namespace.__repr__ is <Namespace {attributes}>, the others' the default object repr
    nb::object Repr(nb::handle self) const
    {
        if (std::string_view(ClassName()) != "Namespace")
        {
            const auto text = "<jinja2.utils." + std::string(ClassName()) + " object at 0x" + AddressOf(self) + ">";
            return nb::str(text.c_str(), text.size());
        }
        nb::dict attributes = Read([&] {
            nb::dict result;
            const auto* map = m_value.GetIf<jinja2::GenericMap>();
            for (const auto& [key, item] : map->GetItems())
            {
                result[ToPython(key)] = ExportToPython(item, Context(), m_name);
            }
            return result;
        });
        const auto text = nb::steal<nb::str>(PyObject_Repr(attributes.ptr()));
        if (!text)
        {
            throw nb::python_error();
        }
        return nb::str(("<Namespace " + std::string(nb::cast<std::string>(text)) + ">").c_str());
    }

    // next(cycler) as Jinja2's Cycler, which has __next__ and no __iter__
    nb::object Next() const
    {
        RequireClass("Cycler", "is not an iterator");
        return GetAttr("next")();
    }
    // A joiner is called as it is in the template: '' the first time, then its separator
    nb::object Call(nb::args args, nb::kwargs kwargs) const
    {
        RequireClass("Joiner", "is not callable");
        return GetAttr("__call__")(*args, **kwargs);
    }

    int Traverse(visitproc visit, void* arg) const
    {
        Py_VISIT(m_bound.ptr());
        Py_VISIT(m_environment.ptr());
        Py_VISIT(m_templateGlobals.ptr());
        Py_VISIT(m_templateName.ptr());
        return 0;
    }
    void Clear() noexcept
    {
        // The view first: the module it holds borrows from m_bound
        m_value = jinja2::Value();
        m_bound = nb::none();
        m_environment = nb::none();
        m_templateGlobals = nb::none();
        m_templateName = nb::none();
    }

private:
    [[nodiscard]] const char* ClassName() const
    {
        const auto* map = m_value.GetIf<jinja2::GenericMap>();
        const char* name = map ? map->GetObjectClass() : nullptr;
        return name ? name : "object";
    }
    void RequireClass(const char* expected, const char* what) const
    {
        if (std::string_view(ClassName()) != expected)
        {
            PyErr_Format(PyExc_TypeError, "'%s' object %s", ClassName(), what);
            throw nb::python_error();
        }
    }
    static std::string AddressOf(nb::handle self)
    {
        char text[2 * sizeof(void*) + 1];
        snprintf(text, sizeof(text), "%llx", static_cast<unsigned long long>(reinterpret_cast<std::uintptr_t>(self.ptr())));
        return text;
    }

    // `read` in the module's environment as a macro's call runs, its engine errors raised as Jinja2's
    template<typename Fn>
    auto Read(Fn&& read) const -> std::invoke_result_t<Fn>
    {
        if (!m_environment.is_valid() || m_environment.is_none())
        {
            RaiseTemplateRuntimeError("the module object was cleared by the garbage collector");
        }
        auto& env = nb::cast<PyEnvironment&>(m_environment);
        env.CheckAlive();
        const auto entry = env.Enter();
        RenderState state;
        state.undefinedClass = env.UndefinedClass();
        state.environment = m_environment.ptr();
        state.templateGlobals = m_templateGlobals.ptr();
        state.sandbox = SandboxOf(m_environment);
        state.sandboxAllNames = entry.sandboxAllNames;
        const RenderScope scope(state);
        try
        {
            auto result = read();
            RaiseAbort(state);
            return result;
        }
        catch (const jinja2::BadResultAccess<char>& failure)
        {
            ErrorSource where;
            where.environment = m_environment;
            where.name = m_templateName;
            RaiseError(failure.error(), where);
        }
    }

    // Declared before m_value, so it goes after it
    nb::object m_bound;
    jinja2::Value m_value;
    nb::object m_environment;
    nb::object m_templateGlobals;
    nb::object m_templateName;
    std::string m_name;
};

// One level of a nested export, counted against Python's recursion limit
class ExportDepth
{
public:
    ExportDepth()
    {
        if (Py_EnterRecursiveCall(" while converting a module export") != 0)
        {
            throw nb::python_error();
        }
    }
    ~ExportDepth() { Py_LeaveRecursiveCall(); }
    ExportDepth(const ExportDepth&) = delete;
    ExportDepth& operator=(const ExportDepth&) = delete;
    ExportDepth(ExportDepth&&) = delete;
    ExportDepth& operator=(ExportDepth&&) = delete;
};

// An exported value as Python: the macros, also in lists and dicts, stay callable
// (ModuleMacro); the rest as the engine's values become Python's (ToPython)
nb::object ExportToPython(const jinja2::Value& value, const ExportContext& context, const std::string& name)
{
    const ExportDepth depth;
    if (const auto* callable = value.GetIf<jinja2::UserCallable>())
    {
        return nb::cast(new ModuleMacro(*callable, context, name), nb::rv_policy::take_ownership);
    }
    if (const auto* list = value.GetIf<jinja2::ValuesList>())
    {
        nb::list result;
        for (const auto& item : *list)
        {
            result.append(ExportToPython(item, context, name));
        }
        return result;
    }
    if (const auto* list = value.GetIf<jinja2::GenericList>())
    {
        nb::list result;
        for (const auto& item : *list)
        {
            result.append(ExportToPython(item, context, name));
        }
        // A tuple of the template, whose macros stay ModuleMacro as in a list
        if (!list->IsTuple())
        {
            return result;
        }
        auto tuple = nb::steal(PySequence_Tuple(result.ptr()));
        if (!tuple.is_valid())
        {
            throw nb::python_error();
        }
        return tuple;
    }
    if (const auto* map = value.GetIf<jinja2::ValuesMap>())
    {
        nb::dict result;
        for (const auto& [key, item] : *map)
        {
            result[MakeStr(key)] = ExportToPython(item, context, name);
        }
        return result;
    }
    if (const auto* map = value.GetIf<jinja2::GenericMap>())
    {
        // A Python object the template read, and an undefined value, are what they were (ModuleExports::Objects)
        if (AsPyHost(value) || jinja2::UndefinedOf(value))
        {
            return ToPython(value);
        }
        // A namespace, a cycler or a joiner is the module's own (ModuleExports::Objects)
        if (IsModuleObjectClass(map->GetObjectClass()))
        {
            return nb::cast(new ModuleObject(value, context, name), nb::rv_policy::take_ownership);
        }
        // The entries under their own keys: `{1: 'a', '1': 'b'}` stays two entries
        nb::dict result;
        for (const auto& [key, item] : map->GetItems())
        {
            auto pyKey = ToPython(key);
            if (PyDict_SetItem(result.ptr(), pyKey.ptr(), ExportToPython(item, context, name).ptr()) != 0)
            {
                throw nb::python_error();
            }
        }
        return result;
    }
    return ToPython(value);
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

int ObjectTraverse(PyObject* self, visitproc visit, void* arg)
{
    Py_VISIT(Py_TYPE(self));
    if (!nb::inst_ready(self))
    {
        return 0;
    }
    return nb::inst_ptr<ModuleObject>(self)->Traverse(visit, arg);
}

int ObjectClear(PyObject* self)
{
    if (nb::inst_ready(self))
    {
        nb::inst_ptr<ModuleObject>(self)->Clear();
    }
    return 0;
}

PyType_Slot g_objectSlots[] = { { Py_tp_traverse, reinterpret_cast<void*>(&ObjectTraverse) }, { Py_tp_clear, reinterpret_cast<void*>(&ObjectClear) }, { 0, nullptr } };

PyType_Slot g_macroSlots[] = { { Py_tp_traverse, reinterpret_cast<void*>(&MacroTraverse) }, { Py_tp_clear, reinterpret_cast<void*>(&MacroClear) }, { 0, nullptr } };
} // namespace

const jinja2::Value* ModuleObjectValue(nb::handle object)
{
    return nb::isinstance<ModuleObject>(object) ? &nb::cast<const ModuleObject&>(object).Value() : nullptr;
}

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
    const auto entry = env.Enter();
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
    // The globals are among the params here (Template.make_module)
    env.AddUninstalledGettext(values);
    RenderState state;
    state.undefinedClass = env.UndefinedClass();
    state.environment = m_environment.ptr();
    state.templateGlobals = m_globals.ptr();
    state.sandbox = SandboxOf(m_environment);
    state.sandboxAllNames = entry.sandboxAllNames;
    jinja2::TemplateModule module;
    nb::dict exports;
    auto result = [&] {
        const RenderScope scope(state);
        // Polled as a render is, and so are the calls of its macros, which keep the poll: Ctrl-C stops them
        jinja2::RenderOptions options;
        options.poll = RenderPoll();
        // And env.policies as they are now, which a call of a macro replaces with those of its own entry
        options.filterPolicies = entry.policies;
        // The exports keep what the template made of them: an object of the host, an undefined value, a tuple
        jinja2::ModuleOptions moduleOptions;
        moduleOptions.exports = jinja2::ModuleExports::Objects;
        moduleOptions.waitForTurn = &WaitWithoutGil;
        auto made = m_template.MakeModule(values, options, moduleOptions);
        // In the scope, which an undefined value needs for its class
        if (made)
        {
            module = std::move(made).value();
            const ExportContext context{ m_bound, m_environment, m_globals, m_name };
            for (auto& [name, value] : module.exports)
            {
                exports[MakeStr(name)] = ExportToPython(value, context, name);
            }
        }
        return made;
    }();
    RaiseAbort(state);
    if (!result)
    {
        ErrorSource where;
        where.environment = m_environment;
        where.name = m_name;
        where.filename = m_filename;
        where.syntaxNote = kIncludedSyntaxNote;
        where.render = true;
        RaiseError(result.error(), where);
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
        where.environment = m_environment;
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
    auto result = m_template.FindReferencedTemplates();
    if (!result)
    {
        ErrorSource where;
        where.environment = m_environment;
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
        .def_prop_ro("name", &ModuleMacro::Name)
        .def_prop_ro("arguments", &ModuleMacro::Arguments)
        .def_prop_ro("catch_kwargs", &ModuleMacro::CatchKwargs)
        .def_prop_ro("catch_varargs", &ModuleMacro::CatchVarargs)
        .def_prop_ro("caller", &ModuleMacro::Caller);
    nb::class_<ModuleObject>(module, "ModuleObject", nb::type_slots(g_objectSlots), nb::is_weak_referenceable())
        .def("__getattr__", &ModuleObject::GetAttr)
        .def("__repr__", [](nb::handle self) { return nb::cast<const ModuleObject&>(self).Repr(self); })
        .def("__next__", &ModuleObject::Next)
        .def("__call__", &ModuleObject::Call);
}
} // namespace jinja2cpp_py
