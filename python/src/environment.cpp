#include "environment.h"

#include "convert.h"
#include "errors.h"
#include "lazy_render.h"
#include "loaders.h"
#include "poll.h"

#include "adapters.h"

#include <jinja2cpp/features.h>
#include <jinja2cpp/host_object.h>
#include <jinja2cpp/template.h>
#include <jinja2cpp/template_env.h>
#include <jinja2cpp/value.h>
#if JINJA2CPP_HAS_CHAT_TEMPLATES
#include <jinja2cpp/chat_template.h>
#endif

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <new>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace jinja2cpp_py
{
using namespace nb::literals;

namespace
{
std::uint64_t g_renders = 0; // under the GIL

bool Truth(nb::handle value)
{
    const int truth = PyObject_IsTrue(value.ptr());
    if (truth < 0)
    {
        throw nb::python_error();
    }
    return truth != 0;
}

std::string OptionalText(nb::handle value)
{
    return value.is_none() ? std::string() : Utf8(value);
}

jinja2::UndefinedPolicy PolicyOf(nb::handle type)
{
    if (!PyType_Check(type.ptr()))
    {
        RaiseTypeError("undefined must be a class");
    }
    const auto mro = nb::steal(PyObject_GetAttrString(type.ptr(), "__mro__"));
    if (!mro)
    {
        throw nb::python_error();
    }
    for (auto base : mro)
    {
        if (IsJinjaClass(base, "StrictUndefined"))
        {
            return jinja2::UndefinedPolicy::Strict;
        }
        if (IsJinjaClass(base, "ChainableUndefined"))
        {
            return jinja2::UndefinedPolicy::Chainable;
        }
        if (IsJinjaClass(base, "DebugUndefined"))
        {
            return jinja2::UndefinedPolicy::Debug;
        }
        if (IsJinjaClass(base, "Undefined"))
        {
            return jinja2::UndefinedPolicy::Default;
        }
    }
    RaiseTypeError("undefined must be Undefined, StrictUndefined, ChainableUndefined, DebugUndefined or a subclass of them");
}

// Name/value pairs of a dict, in its order
std::vector<std::pair<std::string, PyObject*>> Entries(nb::handle mapping, const char* what)
{
    if (!PyDict_Check(mapping.ptr()))
    {
        RaiseTypeError(std::string("Environment.") + what + " must be a dict");
    }
    std::vector<std::pair<std::string, PyObject*>> entries;
    for (auto [name, value] : nb::borrow<nb::dict>(mapping))
    {
        entries.emplace_back(Utf8(name), value.ptr());
    }
    return entries;
}

// The object a value holds a strong reference to when it is a Python object read by the engine (PyHost)
PyObject* HeldObject(const jinja2::Value& value)
{
    const auto* map = value.GetIf<jinja2::GenericMap>();
    const auto* host = map && map->GetAccessor() ? dynamic_cast<const PyHost*>(map->GetAccessor()->AsHostObject()) : nullptr;
    return host ? host->Object() : nullptr;
}

// slot = ToEngine(value), keeping `held`, when it is not null, the list of the objects the values of the render
// hold (PyTemplate::RenderWith): what the slot held before goes from it
void AssignHeld(jinja2::Value& slot, nb::handle value, std::vector<PyObject*>* held)
{
    if (!held)
    {
        slot = ToEngine(value);
        return;
    }
    if (auto* before = HeldObject(slot))
    {
        // Each held one was pushed when its slot was assigned; a value the slot got otherwise is not in the list
        if (const auto pos = std::find(held->begin(), held->end(), before); pos != held->end())
        {
            held->erase(pos);
        }
    }
    slot = ToEngine(value);
    if (auto* object = HeldObject(slot))
    {
        held->push_back(object);
    }
}

// Clears the list of what a render's values hold (PieceOptions::held) when the render's params and globals start to go
struct HeldForget
{
    explicit HeldForget(std::vector<PyObject*>* list) noexcept
        : held(list)
    {
    }
    HeldForget(const HeldForget&) = delete;
    HeldForget& operator=(const HeldForget&) = delete;
    HeldForget(HeldForget&&) = delete;
    HeldForget& operator=(HeldForget&&) = delete;
    ~HeldForget()
    {
        if (held)
        {
            held->clear();
        }
    }

    std::vector<PyObject*>* held;
};

// The params of a render: the str keys of a mapping as values of the engine
void AddParams(jinja2::ValuesMap& params, nb::handle mapping, std::vector<PyObject*>* held)
{
    for (auto [name, value] : nb::borrow<nb::dict>(mapping))
    {
        if (PyUnicode_Check(name.ptr()))
        {
            AssignHeld(params[std::string(Utf8View(name))], value, held);
        }
    }
}

// The globals of a render. A sandbox replaces the global `range` (jinja2cpp.sandbox.safe_range, marked
// `_native_range`) by the engine's own, which stops at the environment's Limits::maxRange without a call
// to Python
void AddGlobals(jinja2::ValuesMap& params, nb::handle mapping, bool sandboxed, std::vector<PyObject*>* held = nullptr)
{
    for (auto [name, value] : nb::borrow<nb::dict>(mapping))
    {
        if (!PyUnicode_Check(name.ptr()))
        {
            continue;
        }
        if (sandboxed && PyCallable_Check(value.ptr()) && PyObject_HasAttrString(value.ptr(), "_native_range") != 0 && Utf8View(name) == "range")
        {
            continue;
        }
        AssignHeld(params[std::string(Utf8View(name))], value, held);
    }
}

int EnvironmentTraverse(PyObject* self, visitproc visit, void* arg)
{
    Py_VISIT(Py_TYPE(self));
    if (!nb::inst_ready(self))
    {
        return 0;
    }
    return nb::inst_ptr<PyEnvironment>(self)->Traverse(visit, arg);
}

int EnvironmentClear(PyObject* self)
{
    if (nb::inst_ready(self))
    {
        nb::inst_ptr<PyEnvironment>(self)->Clear();
    }
    return 0;
}

int TemplateTraverse(PyObject* self, visitproc visit, void* arg)
{
    Py_VISIT(Py_TYPE(self));
    if (!nb::inst_ready(self))
    {
        return 0;
    }
    return nb::inst_ptr<PyTemplate>(self)->Traverse(visit, arg);
}

int TemplateClear(PyObject* self)
{
    if (nb::inst_ready(self))
    {
        nb::inst_ptr<PyTemplate>(self)->Clear();
    }
    return 0;
}

PyType_Slot g_environmentSlots[] = { { Py_tp_traverse, reinterpret_cast<void*>(&EnvironmentTraverse) }, { Py_tp_clear, reinterpret_cast<void*>(&EnvironmentClear) }, { 0, nullptr } };
PyType_Slot g_templateSlots[] = { { Py_tp_traverse, reinterpret_cast<void*>(&TemplateTraverse) }, { Py_tp_clear, reinterpret_cast<void*>(&TemplateClear) }, { 0, nullptr } };

// Environment's keyword arguments that are settings of the engine
const char* const g_options[] = { "block_start_string", "block_end_string", "variable_start_string", "variable_end_string", "comment_start_string", "comment_end_string",
                                  "line_statement_prefix", "line_comment_prefix", "trim_blocks", "lstrip_blocks", "newline_sequence", "keep_trailing_newline",
                                  "extensions", "optimized", "undefined", "finalize", "autoescape", "loader", "cache_size", "auto_reload", "bytecode_cache", "enable_async" };
} // namespace

PyEnvironment::PyEnvironment(nb::kwargs kwargs)
    : m_globals(nb::dict())
    , m_filters(nb::dict())
    , m_tests(nb::dict())
    , m_undefined(Names().undefinedType)
    , m_extensions(nb::list())
    , m_options(nb::dict())
{
    for (size_t i = 0; i < 4; ++i)
    {
        m_gettext[i] = nb::none();
        m_gettextGlobals[i] = nb::none();
    }
    // Counts the steps of every render, so that Ctrl-C is polled (PollSignals): UINT64_MAX
    // steps is no limit, but a limit is set
    m_settings.limits.maxRenderSteps = UINT64_MAX;
    m_options["optimized"] = nb::bool_(true);
    m_options["bytecode_cache"] = nb::none();
    m_options["enable_async"] = nb::bool_(false);
    for (auto [name, value] : kwargs)
    {
        SetOption(Utf8(name), value);
    }
}

struct JoinPathTarget
{
    PyObject* env = nullptr;
};

PyEnvironment::~PyEnvironment()
{
    if (m_joinTarget)
    {
        m_joinTarget->env = nullptr;
    }
}

void PyEnvironment::Keep(nb::handle object)
{
    if (PyList_Append(m_bound.ptr(), object.ptr()) != 0)
    {
        throw nb::python_error();
    }
}

void PyEnvironment::SetOption(const std::string& name, nb::handle value)
{
    CheckAlive();
    auto& settings = m_settings;
    auto text = [&](std::string& field) { field = Utf8(value); };
    if (name == "block_start_string")
    {
        text(settings.blockStartString);
    }
    else if (name == "block_end_string")
    {
        text(settings.blockEndString);
    }
    else if (name == "variable_start_string")
    {
        text(settings.variableStartString);
    }
    else if (name == "variable_end_string")
    {
        text(settings.variableEndString);
    }
    else if (name == "comment_start_string")
    {
        text(settings.commentStartString);
    }
    else if (name == "comment_end_string")
    {
        text(settings.commentEndString);
    }
    else if (name == "line_statement_prefix")
    {
        settings.lineStatementPrefix = OptionalText(value);
    }
    else if (name == "line_comment_prefix")
    {
        settings.lineCommentPrefix = OptionalText(value);
    }
    else if (name == "trim_blocks")
    {
        settings.trimBlocks = Truth(value);
    }
    else if (name == "lstrip_blocks")
    {
        settings.lstripBlocks = Truth(value);
    }
    else if (name == "keep_trailing_newline")
    {
        settings.keepTrailingNewline = Truth(value);
    }
    else if (name == "newline_sequence")
    {
        auto sequence = Utf8(value);
        if (sequence != "\n" && sequence != "\r\n" && sequence != "\r")
        {
            // Jinja2's _environment_config_check
            PyErr_SetString(PyExc_AssertionError, "newline_sequence set to unknown line ending string.");
            throw nb::python_error();
        }
        settings.newlineSequence = std::move(sequence);
    }
    else if (name == "extensions")
    {
        SetExtensions(value);
    }
    else if (name == "undefined")
    {
        settings.undefinedPolicy = PolicyOf(value);
        m_undefined = nb::borrow(value);
    }
    else if (name == "finalize")
    {
        m_finalize = nb::borrow(value);
        if (value.is_none())
        {
            settings.finalize = jinja2::UserCallable();
        }
        else
        {
            // Kept by m_finalize, and by the list of each engine environment made with it (Rebuild)
            settings.finalize = BorrowedCallable(value);
        }
    }
    else if (name == "autoescape")
    {
        SetAutoescape(value);
    }
    else if (name == "loader")
    {
        SetLoader(value);
    }
    else if (name == "cache_size")
    {
        settings.cacheSize = nb::cast<int>(value);
    }
    else if (name == "auto_reload")
    {
        settings.autoReload = Truth(value);
    }
    else if (name == "enable_async")
    {
        if (Truth(value))
        {
            PyErr_SetString(PyExc_NotImplementedError, "jinja2cpp does not support enable_async yet (docs/python.md)");
            throw nb::python_error();
        }
        m_options["enable_async"] = nb::borrow(value);
    }
    else if (name == "optimized" || name == "bytecode_cache")
    {
        // They change Jinja2's speed, not its output
        m_options[name.c_str()] = nb::borrow(value);
    }
    else
    {
        RaiseTypeError("Environment.__init__() got an unexpected keyword argument '" + name + "'");
    }
    // Templates loaded from now on take the change; loaded ones keep what they had, as in Jinja2
    RetireEngine();
}

nb::object PyEnvironment::GetOption(const std::string& name) const
{
    const auto& settings = m_settings;
    auto optional = [](const std::string& text) { return text.empty() ? nb::none() : MakeStr(text); };
    if (name == "block_start_string")
    {
        return MakeStr(settings.blockStartString);
    }
    if (name == "block_end_string")
    {
        return MakeStr(settings.blockEndString);
    }
    if (name == "variable_start_string")
    {
        return MakeStr(settings.variableStartString);
    }
    if (name == "variable_end_string")
    {
        return MakeStr(settings.variableEndString);
    }
    if (name == "comment_start_string")
    {
        return MakeStr(settings.commentStartString);
    }
    if (name == "comment_end_string")
    {
        return MakeStr(settings.commentEndString);
    }
    if (name == "line_statement_prefix")
    {
        return optional(settings.lineStatementPrefix);
    }
    if (name == "line_comment_prefix")
    {
        return optional(settings.lineCommentPrefix);
    }
    if (name == "trim_blocks")
    {
        return nb::bool_(settings.trimBlocks);
    }
    if (name == "lstrip_blocks")
    {
        return nb::bool_(settings.lstripBlocks);
    }
    if (name == "keep_trailing_newline")
    {
        return nb::bool_(settings.keepTrailingNewline);
    }
    if (name == "newline_sequence")
    {
        return MakeStr(settings.newlineSequence);
    }
    if (name == "extensions")
    {
        return m_extensions;
    }
    if (name == "undefined")
    {
        return m_undefined;
    }
    if (name == "finalize")
    {
        return m_finalize;
    }
    if (name == "autoescape")
    {
        return m_autoescape;
    }
    if (name == "loader")
    {
        return m_loader;
    }
    if (name == "cache_size")
    {
        return nb::int_(settings.cacheSize);
    }
    if (name == "auto_reload")
    {
        return nb::bool_(settings.autoReload);
    }
    return m_options[name.c_str()];
}

nb::object PyEnvironment::GetDict(const std::string& name) const
{
    return name == "globals" ? m_globals : name == "filters" ? m_filters
                                                             : m_tests;
}

void PyEnvironment::SetDict(const std::string& name, nb::handle value)
{
    CheckAlive();
    if (!PyDict_Check(value.ptr()))
    {
        RaiseTypeError("Environment." + name + " must be a dict");
    }
    (name == "globals" ? m_globals : name == "filters" ? m_filters
                                                       : m_tests) = nb::borrow(value);
}

void PyEnvironment::SetAutoescape(nb::handle value)
{
    m_autoescape = nb::borrow(value);
    if (PyCallable_Check(value.ptr()))
    {
        // Kept by m_autoescape, and by the list of each engine environment made with it (Rebuild)
        PyObject* selector = value.ptr();
        // Jinja2 passes the template's name, None for a template from a string
        m_settings.autoescapeSelector = [selector](const std::string& templateName) {
            const auto name = templateName.empty() ? nb::none() : MakeStr(templateName);
            PyObject* result = PyObject_CallFunctionObjArgs(selector, name.ptr(), nullptr);
            if (!result)
            {
                throw nb::python_error();
            }
            return Truth(nb::steal(result));
        };
        m_settings.autoescape = false;
        return;
    }
    m_settings.autoescapeSelector = jinja2::AutoescapeSelector();
    m_settings.autoescape = Truth(value);
}

void PyEnvironment::SetExtensions(nb::handle value)
{
    auto& extensions = m_settings.extensions;
    extensions.doStatement = false;
    extensions.loopControls = false;
    extensions.i18n = false;
    nb::list list;
    for (auto extension : value)
    {
        std::string name;
        if (PyUnicode_Check(extension.ptr()))
        {
            name = Utf8(extension);
            // "jinja2.ext.do" and "jinja2cpp.ext.do" alike
            if (const auto dot = name.rfind('.'); dot != std::string::npos)
            {
                name = name.substr(dot + 1);
            }
        }
        else if (PyType_Check(extension.ptr()) && IsJinjaClass(extension, Utf8(extension.attr("__name__"))))
        {
            name = Utf8(extension.attr("__name__"));
        }
        if (name == "do" || name == "ExprStmtExtension")
        {
            extensions.doStatement = true;
        }
        else if (name == "loopcontrols" || name == "LoopControlExtension")
        {
            extensions.loopControls = true;
        }
        else if (name == "i18n" || name == "InternationalizationExtension")
        {
            extensions.i18n = true;
        }
        else
        {
            PyErr_Format(PyExc_NotImplementedError, "jinja2cpp does not support the extension %R yet (docs/python.md)", extension.ptr());
            throw nb::python_error();
        }
        list.append(extension);
    }
    m_extensions = std::move(list);
}

void PyEnvironment::SetLoader(nb::handle loader)
{
    m_loader = nb::borrow(loader);
    // Templates loaded with an earlier loader keep reading it: the lists of the engine environments it was in keep it
    // A template loaded before may keep the old handler, where the environment's traverse no longer
    // reaches: what it keeps of a miss goes now (Loader::ForgetMissed)
    if (m_loaderHandler)
    {
        m_loaderHandler->ForgetMissed();
    }
    m_loaderHandler.reset();
}

namespace
{
// A list that a weak reference can name (BoundObjects): a list subclass, as a list is not weakly referenceable
nb::object NewBoundList()
{
    static PyObject* const type = [] {
        nb::dict namespace_;
        namespace_["__slots__"] = nb::make_tuple("__weakref__");
        return nb::borrow<nb::object>(reinterpret_cast<PyObject*>(&PyType_Type))("BoundObjects", nb::make_tuple(nb::borrow<nb::object>(reinterpret_cast<PyObject*>(&PyList_Type))), namespace_).release().ptr();
    }();
    return nb::borrow<nb::object>(type)();
}

// The list a Retired entry names, or None once its templates are gone
nb::object RetiredList(const nb::object& ref)
{
    nb::object list = nb::steal(PyObject_CallNoArgs(ref.ptr()));
    if (!list.is_valid())
    {
        throw nb::python_error();
    }
    return list;
}
} // namespace

void PyEnvironment::RetireEngine() noexcept
{
    if (!m_env || !m_bound.is_valid() || m_bound.is_none() || m_cleared)
    {
        m_env.reset();
        return;
    }
    // An entry whose list went (its templates did) goes; the error of a weak reference that cannot be made is
    // cleared: its templates then keep the translations they have
    const auto gone = [](const Retired& retired) {
        const nb::object list = nb::steal(PyObject_CallNoArgs(retired.bound.ptr()));
        if (!list.is_valid())
        {
            PyErr_Clear();
            return true;
        }
        return list.is_none();
    };
    m_retired.erase(std::remove_if(m_retired.begin(), m_retired.end(), gone), m_retired.end());
    try
    {
        // The list owns the engine environment (and the loader's handler and what else it holds), so that it goes
        // with the templates; the entry only names both
        std::weak_ptr<jinja2::TemplateEnv> env = m_env;
        auto owner = std::make_unique<std::shared_ptr<jinja2::TemplateEnv>>(m_env);
        nb::object capsule = nb::steal(PyCapsule_New(owner.get(), nullptr, [](PyObject* object) {
            delete static_cast<std::shared_ptr<jinja2::TemplateEnv>*>(PyCapsule_GetPointer(object, nullptr));
        }));
        if (capsule.is_valid())
        {
            owner.release(); // NOLINT(bugprone-unused-return-value): the capsule owns it now
        }
        nb::object ref = capsule.is_valid() && PyList_Append(m_bound.ptr(), capsule.ptr()) == 0 ? nb::steal(PyWeakref_NewRef(m_bound.ptr(), nullptr)) : nb::object();
        if (ref.is_valid())
        {
            m_retired.push_back(Retired{ std::move(env), std::move(ref) });
        }
        else
        {
            PyErr_Clear();
        }
    }
    catch (const std::bad_alloc&)
    {
    }
    m_env.reset();
    m_bound = nb::object();
}

namespace
{
// `env.join_path(name, parent)` for the tags that load templates (TemplateEnv::SetJoinPath)
std::string CallJoinPath(PyObject* env, const std::string& name, const std::string& parent)
{
    try
    {
        const nb::object result = nb::steal(PyObject_CallMethod(env, "join_path", "OO", MakeStr(name).ptr(), MakeStr(parent).ptr()));
        if (!result.is_valid())
        {
            throw nb::python_error();
        }
        if (PyUnicode_Check(result.ptr()) == 0)
        {
            PyErr_SetString(PyExc_TypeError, "join_path must return a str");
            throw nb::python_error();
        }
        Py_ssize_t size = 0;
        const char* text = PyUnicode_AsUTF8AndSize(result.ptr(), &size);
        if (!text)
        {
            throw nb::python_error();
        }
        return std::string(text, static_cast<std::size_t>(size));
    }
    catch (nb::python_error& error)
    {
        if (!error.matches(PyExc_Exception))
        {
            // KeyboardInterrupt, SystemExit: noted for the render, as a callable's (CallPython)
            if (auto* render = CurrentRender(); render && !render->abort)
            {
                render->abort = std::make_exception_ptr(nb::python_error(error));
            }
        }
        throw;
    }
}
} // namespace

void PyEnvironment::SyncJoinPath()
{
    const auto self = nb::find(this);
    const nb::object overridden = nb::steal(PyObject_CallMethod(self.ptr(), "_join_path_overridden", nullptr));
    if (!overridden.is_valid())
    {
        throw nb::python_error();
    }
    const bool on = PyObject_IsTrue(overridden.ptr()) > 0;
    if (on == m_joinPathSet)
    {
        return;
    }
    if (on)
    {
        if (!m_joinTarget)
        {
            m_joinTarget = std::make_shared<JoinPathTarget>();
        }
        m_joinTarget->env = self.ptr();
        m_env->SetJoinPath([target = m_joinTarget](const std::string& name, const std::string& parent) {
            return target->env ? CallJoinPath(target->env, name, parent) : name;
        });
    }
    else
    {
        m_env->SetJoinPath({});
    }
    m_joinPathSet = on;
}

void PyEnvironment::Rebuild()
{
    RetireEngine();
    m_env = std::make_shared<jinja2::TemplateEnv>();
    m_joinPathSet = false;
    // What it borrows: the translations first (ApplyGettext), then the callables of the settings and the loader, and the
    // filters and tests as Prepared binds them. The templates loaded from the engine environment before keep its list
    m_bound = NewBoundList();
    Keep(nb::none());
    for (const auto* object : { &m_loader, &m_finalize })
    {
        if (!object->is_none())
        {
            Keep(*object);
        }
    }
    if (m_policiesDumps.is_valid())
    {
        Keep(m_policiesDumps);
    }
    if (PyCallable_Check(m_autoescape.ptr()) != 0)
    {
        Keep(m_autoescape);
    }
    auto settings = m_settings;
    bool immutable = false;
    bool sandboxed = false;
    if (const auto self = nb::find(this); self.is_valid() && SandboxOf(self))
    {
        sandboxed = true;
        // SandboxedEnvironment: safe_range's MAX_RANGE is the engine's range limit, and the immutable
        // form has the engine's list and dict methods that change their receiver refused
        settings.limits.maxRange = nb::cast<std::uint64_t>(nb::module_::import_("jinja2cpp.sandbox").attr("MAX_RANGE"));
        immutable = Truth(nb::getattr(self, "_sandbox_immutable", nb::bool_(false)));
    }
    settings.immutableContainers = immutable;
    settings.extensions.i18nNewstyle = m_newstyleGettext;
    // The module's tests of the fallback lower the engine's bracket depth, so that a template
    // Jinja2 compiles fails only the engine's parse (python/tests/test_fallback.py, docs/tasks/0497)
    if (const auto depth = nb::module_::import_("jinja2cpp.fallback").attr("_test_max_expression_depth"); !depth.is_none())
    {
        settings.limits.maxExpressionDepth = nb::cast<std::uint32_t>(depth);
    }
    m_sandboxed = sandboxed;
    if (sandboxed)
    {
        settings.sandboxPolicy = MakePythonSandboxPolicy(nb::find(this));
    }
    m_env->SetSettings(settings);
    if (m_chatTemplates)
    {
        ApplyChatTemplates();
    }
    if (!m_loaderHandler && !m_loader.is_none())
    {
        m_loaderHandler = MakeLoader(m_loader, nb::find(this));
    }
    if (m_loaderHandler)
    {
        m_loaderHandler->Install(*m_env);
    }
    m_boundFilters.clear();
    m_boundTests.clear();
    ApplyGettext(false);
}

void PyEnvironment::ApplyChatTemplates()
{
#if JINJA2CPP_HAS_CHAT_TEMPLATES
    jinja2::ChatTemplateOptions options;
    if (!m_chatClock.is_none())
    {
        Keep(m_chatClock);
        // Called on the rendering thread, which holds the GIL (the render does)
        options.now = [clock = m_chatClock.ptr()] {
            const double seconds = nb::cast<double>(nb::handle(clock)());
            return std::chrono::system_clock::time_point(std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::duration<double>(seconds)));
        };
    }
    jinja2::ConfigureForChatTemplates(*m_env, std::move(options));
    // A render passes its own globals, which replace the environment's: they are added to them
    auto globals = std::make_shared<jinja2::ValuesMap>();
    m_env->ApplyGlobals([&globals](const jinja2::ValuesMap& values) { *globals = values; });
    m_engineGlobals = std::move(globals);
    m_engineGlobalsBound = m_chatClock;
#endif
}

void PyEnvironment::SetChatTemplates(nb::handle clock)
{
    CheckAlive();
#if JINJA2CPP_HAS_CHAT_TEMPLATES
    m_chatClock = nb::borrow(clock);
    m_chatTemplates = true;
    // Templates loaded before keep their environment; the next load rebuilds it with the preset
    RetireEngine();
#else
    (void)clock;
    RaiseTypeError("this jinja2cpp was built without chat templates (JINJA2CPP_WITH_CHAT_TEMPLATES=OFF)");
#endif
}

void PyEnvironment::ApplyGettext(bool everyEngine)
{
    // A callable left out stays unset: the message is not translated
    const auto callable = [](const nb::object& object) { return object.is_none() ? jinja2::UserCallable() : BorrowedCallable(object); };
    const auto install = [&](jinja2::TemplateEnv& env, const nb::object& bound) {
        env.InstallGettextCallables(m_gettextStyle, callable(m_gettext[0]), callable(m_gettext[1]), callable(m_gettext[2]), callable(m_gettext[3]));
        // Item 0 of the list: the engine environment no longer calls the ones it replaces. A call running now holds
        // the callable it runs (BorrowedCallable)
        if (PyList_SetItem(bound.ptr(), 0, nb::make_tuple(m_gettext[0], m_gettext[1], m_gettext[2], m_gettext[3]).release().ptr()) != 0)
        {
            throw nb::python_error();
        }
    };
    if (m_env)
    {
        install(*m_env, m_bound);
    }
    if (!everyEngine)
    {
        return;
    }
    // The templates of the engine environments before, as Jinja2's templates all read env.globals
    const auto retired = m_retired;
    for (const auto& [weak, ref] : retired)
    {
        const auto bound = RetiredList(ref);
        if (const auto env = weak.lock(); env && !bound.is_none())
        {
            install(*env, bound);
        }
    }
}

void PyEnvironment::SetNewstyleGettext(bool newstyle)
{
    m_newstyleGettext = newstyle;
    if (m_env)
    {
        // A template keeps the style it was loaded with; the next loads take this one
        m_env->ModifySettings([newstyle](jinja2::Settings& settings) { settings.extensions.i18nNewstyle = newstyle; });
    }
}

void PyEnvironment::SetGettext(nb::handle gettext, nb::handle ngettext, nb::handle pgettext, nb::handle npgettext)
{
    CheckAlive();
    const nb::handle callables[4] = { gettext, ngettext, pgettext, npgettext };
    for (size_t i = 0; i < 4; ++i)
    {
        m_gettext[i] = nb::borrow(callables[i]);
    }
    // The gettext globals of every template take the style of this install, as the globals Jinja2's install puts in
    // env.globals do (docs/tasks/0509); `{% trans %}` blocks keep the style they were loaded with
    m_gettextStyle = m_newstyleGettext ? jinja2::GettextStyle::Newstyle : jinja2::GettextStyle::Oldstyle;
    ApplyGettext(true);
}

void PyEnvironment::AddUninstalledGettext(jinja2::ValuesMap& globals) const
{
    if (!m_settings.extensions.i18n)
    {
        return;
    }
    // An install, newstyle (the engine's callables) or oldstyle (its own in env.globals), sets all four in Jinja2, a
    // callable left out as None
    const auto installed = [](const nb::object& callable) { return !callable.is_none(); };
    if (std::any_of(std::begin(m_gettext), std::end(m_gettext), installed) || std::any_of(std::begin(m_gettextGlobals), std::end(m_gettextGlobals), installed))
    {
        return;
    }
    for (const char* name : { "gettext", "ngettext", "pgettext", "npgettext" })
    {
        jinja2::UndefinedDescription description;
        description.policy = m_settings.undefinedPolicy;
        description.name = name;
        globals.emplace(name, jinja2::MakeUndefined(std::move(description)));
    }
}

nb::tuple PyEnvironment::GettextGlobals() const
{
    nb::list result;
    for (const auto& object : m_gettextGlobals)
    {
        result.append(object);
    }
    return nb::tuple(result);
}

void PyEnvironment::SetGettextGlobals(nb::handle callables)
{
    CheckAlive();
    size_t idx = 0;
    for (auto callable : callables)
    {
        if (idx == 4)
        {
            RaiseTypeError("_gettext_globals takes four callables or None");
        }
        m_gettextGlobals[idx++] = nb::borrow(callable);
    }
    for (; idx < 4; ++idx)
    {
        m_gettextGlobals[idx] = nb::none();
    }
}

std::shared_ptr<const jinja2::FilterPolicies> PyEnvironment::RefreshPolicies()
{
    const auto self = nb::find(this);
    PyObject* const raw = PyObject_GetAttrString(self.ptr(), "policies");
    if (!raw)
    {
        // A bare _jinja2cpp.Environment has none: Jinja2's defaults, which the engine's are
        if (!PyErr_ExceptionMatches(PyExc_AttributeError))
        {
            throw nb::python_error();
        }
        PyErr_Clear();
        return m_policies;
    }
    const auto current = nb::steal(raw);
    if (m_policiesSnapshot.is_valid())
    {
        const int same = PyObject_RichCompareBool(m_policiesSnapshot.ptr(), current.ptr(), Py_EQ);
        if (same < 0)
        {
            throw nb::python_error();
        }
        if (same == 1)
        {
            return m_policies;
        }
    }
    const auto converted = nb::module_::import_("jinja2cpp.fallback").attr("native_policies")(current);
    if (!converted[2].is_none())
    {
        // Set after a template loaded natively (a load that sees it goes to Jinja2): the template cannot honour it
        const auto error = nb::module_::import_("jinja2cpp.exceptions").attr("UnsupportedError")(nb::object(converted[2]));
        PyErr_SetObject(error.type().ptr(), error.ptr());
        throw nb::python_error();
    }
    const nb::dict fields = nb::borrow<nb::dict>(nb::object(converted[1]));
    const auto field = [&fields](const char* key) -> nb::object { return fields[key]; };
    const auto optionalText = [](const nb::object& value) -> std::optional<std::string> {
        return value.is_none() ? std::nullopt : std::optional<std::string>(Utf8(value));
    };
    auto policies = std::make_shared<jinja2::FilterPolicies>();
    policies->urlizeRel = Utf8(field("rel"));
    policies->urlizeTarget = optionalText(field("target"));
    for (const auto scheme : field("schemes"))
    {
        policies->urlizeExtraSchemes.push_back(Utf8(scheme));
    }
    policies->truncateLeeway = nb::cast<std::int64_t>(field("leeway"));
    policies->jsonSortKeys = Truth(field("sort_keys"));
    policies->jsonEnsureAscii = Truth(field("ensure_ascii"));
    policies->jsonIndent = optionalText(field("indent"));
    if (const auto separators = field("separators"); !separators.is_none())
    {
        policies->jsonSeparators = std::make_pair(Utf8(nb::object(separators[0])), Utf8(nb::object(separators[1])));
    }
    const auto dumps = field("dumps");
    if (!dumps.is_none())
    {
        policies->jsonDumps = BorrowedCallable(dumps);
    }
    const bool trimmed = Truth(field("trimmed"));
    // Kept by the list of the engine environment the templates loaded from now on belong to (Rebuild keeps it in a new
    // one); the templates loaded before keep the callable they have in theirs
    m_policiesSnapshot = nb::object(converted[0]);
    m_policiesDumps = dumps.is_none() ? nb::object() : dumps;
    if (m_env && !dumps.is_none())
    {
        Keep(dumps);
    }
    m_policies = std::move(policies);
    m_settings.filterPolicies = *m_policies;
    m_settings.extensions.i18nTrimmed = trimmed;
    if (m_env)
    {
        // A template keeps the policies it was loaded with (MakeModule's macros use them); the next loads take these
        m_env->ModifySettings([&](jinja2::Settings& settings) {
            settings.filterPolicies = *m_policies;
            settings.extensions.i18nTrimmed = trimmed;
        });
    }
    return m_policies;
}

PyEnvironment::EntryState PyEnvironment::Enter()
{
    EntryState state;
    state.policies = RefreshPolicies();
    if (m_sandboxed)
    {
        state.sandboxAllNames = CheckNativeSandboxHooks(nb::find(this).ptr());
    }
    return state;
}

PyEnvironment::Engine PyEnvironment::Prepared()
{
    CheckAlive();
    // The policies a template loads with, which a MakeModule of it keeps
    RefreshPolicies();
    if (!m_env)
    {
        Rebuild();
    }
    SyncJoinPath();
    // A template binds the filters and tests when it loads, as in Jinja2
    auto filters = Entries(m_filters, "filters");
    auto tests = Entries(m_tests, "tests");
    // One that was bound and is not now, replaced or removed: the templates loaded before keep it, in the list of
    // the engine environment they were loaded from, and a new engine environment with a list of its own binds the
    // dicts as they are, so that a replaced callable goes with the last template that binds it (docs/tasks/0406)
    const auto dropsOne = [](const std::vector<std::pair<std::string, PyObject*>>& bound, const std::vector<std::pair<std::string, PyObject*>>& now) {
        return std::any_of(bound.begin(), bound.end(), [&now](const auto& entry) { return std::find(now.begin(), now.end(), entry) == now.end(); });
    };
    if (filters == m_boundFilters && tests == m_boundTests)
    {
        return Engine{ m_env, m_bound };
    }
    if (dropsOne(m_boundFilters, filters) || dropsOne(m_boundTests, tests))
    {
        // Rebuild may run Python, which may change the dicts: read again after it
        Rebuild();
        filters = Entries(m_filters, "filters");
        tests = Entries(m_tests, "tests");
        SyncJoinPath();
    }
    auto sync = [this](std::vector<std::pair<std::string, PyObject*>>& entries, std::vector<std::pair<std::string, PyObject*>>& bound, bool isFilter) {
        if (entries == bound)
        {
            return;
        }
        for (const auto& [name, object] : entries)
        {
            if (std::find(bound.begin(), bound.end(), std::make_pair(name, object)) != bound.end())
            {
                continue;
            }
            Keep(object);
            auto callable = BorrowedCallable(object);
            isFilter ? m_env->AddFilter(name, std::move(callable)) : m_env->AddTest(name, std::move(callable));
        }
        bound = std::move(entries);
    };
    sync(filters, m_boundFilters, true);
    sync(tests, m_boundTests, false);
    return Engine{ m_env, m_bound };
}

void PyEnvironment::CheckAlive() const
{
    if (m_cleared)
    {
        RaiseTemplateRuntimeError("the environment was cleared by the garbage collector");
    }
}

ErrorSource SourceOfTemplate(nb::handle environment, const std::string& name, bool withSource)
{
    auto source = nb::cast<const PyEnvironment&>(environment).SourceOf(name, withSource);
    source.environment = nb::borrow(environment);
    return source;
}

ErrorSource PyEnvironment::SourceOf(const std::string& name, bool withSource) const
{
    if (m_loaderHandler)
    {
        return m_loaderHandler->SourceOf(name, withSource);
    }
    ErrorSource source;
    source.name = MakeStr(name);
    return source;
}

nb::object PyEnvironment::LoaderNotFound(const std::string& name) const
{
    return m_loaderHandler ? m_loaderHandler->NotFound(name) : nb::none();
}

nb::object LoaderNotFoundOf(nb::handle environment, const std::string& name)
{
    if (environment.is_none() || !nb::isinstance<PyEnvironment>(environment))
    {
        return nb::none();
    }
    return nb::cast<const PyEnvironment&>(environment).LoaderNotFound(name);
}

nb::object PyEnvironment::MakeTemplate(Loaded tpl, nb::handle globals, nb::object name, nb::object filename, nb::handle templateClass)
{
    auto self = nb::find(this);
    // A copy, as Jinja2's make_globals takes a ChainMap of it
    // (one mapping per template, which Template.globals shows and Render reads)
    nb::object templateGlobals = globals.is_none() ? nb::object(nb::dict()) : Names().dictType(globals);
    if (!templateClass.is_valid() || !PyType_Check(templateClass.ptr()) || !PyType_IsSubtype(reinterpret_cast<PyTypeObject*>(templateClass.ptr()), reinterpret_cast<PyTypeObject*>(nb::type<PyTemplate>().ptr())))
    {
        RaiseTypeError("template_class must be jinja2cpp.Template or a subclass of it");
    }
    // Like Jinja2's Template.from_code: an instance of the class, without calling its __init__
    auto made = std::make_unique<PyTemplate>(std::move(tpl.engine), std::move(tpl.bound), nb::borrow(self), std::move(templateGlobals), std::move(name), std::move(filename));
    nb::object instance = nb::inst_take_ownership(templateClass, made.get());
    made.release(); // NOLINT(bugprone-unused-return-value): the instance owns it now
    return instance;
}

PyEnvironment::Loaded PyEnvironment::Load(nb::handle source, const std::string& name)
{
    auto engine = Prepared();
    auto& env = *engine.env;
    if (!PyUnicode_Check(source.ptr()))
    {
        // Jinja2 compiles a str; bytes and numbers are a TypeError, not their repr
        RaiseTypeError("source must be a str, not " + Utf8(nb::handle(reinterpret_cast<PyObject*>(Py_TYPE(source.ptr()))).attr("__name__")));
    }
    const auto text = Utf8(source);
    auto result = env.FromString(std::string_view(text), name);
    if (!result)
    {
        ErrorSource where;
        where.source = nb::borrow(source);
        where.environment = nb::find(this);
        if (!name.empty())
        {
            where.name = MakeStr(name);
        }
        RaiseError(result.error(), where);
    }
    return Loaded{ std::move(result).value(), std::move(engine.bound) };
}

nb::object PyEnvironment::FromString(nb::handle source, nb::handle globals, nb::handle templateClass)
{
    if (nb::isinstance<PyTemplate>(source))
    {
        return nb::borrow(source);
    }
    return MakeTemplate(Load(source), globals, nb::none(), nb::none(), templateClass);
}

nb::object PyEnvironment::Compile(nb::handle source, nb::handle name, nb::handle filename, nb::handle globals, nb::handle templateClass)
{
    return MakeTemplate(Load(source, name.is_none() ? std::string() : Utf8(name)), globals, nb::borrow(name), nb::borrow(filename), templateClass);
}

nb::object PyEnvironment::LoadTemplate(nb::handle name, nb::handle globals, nb::handle templateClass, nb::handle cached)
{
    if (nb::isinstance<PyTemplate>(name))
    {
        return nb::borrow(name);
    }
    auto engine = Prepared();
    auto& env = *engine.env;
    if (m_loader.is_none())
    {
        RaiseTypeError("no loader for this environment specified");
    }
    if (!PyUnicode_Check(name.ptr()))
    {
        // An Undefined name raises its UndefinedError, as in Jinja2
        if (nb::isinstance(name, Names().undefinedType))
        {
            name.attr("_fail_with_undefined_error")();
        }
        RaiseTemplateNotFound(name);
    }
    const auto text = Utf8(name);
    auto result = env.LoadTemplate(text);
    if (!result)
    {
        const auto& error = result.error();
        if (error.GetCode() == jinja2::ErrorCode::FileNotFound || error.GetCode() == jinja2::ErrorCode::TemplateNotFound)
        {
            RaiseTemplateNotFound(nb::find(this), name);
        }
        auto where = SourceOf(text);
        where.environment = nb::find(this);
        RaiseError(error, where);
    }
    if (cached.is_valid() && nb::isinstance<PyTemplate>(cached) && nb::inst_ready(cached) && nb::inst_ptr<PyTemplate>(cached)->Engine() == result.value())
    {
        return nb::borrow(cached);
    }
    // Only the filename: the source is read for an error alone
    auto where = SourceOf(text, false);
    return MakeTemplate(Loaded{ std::move(result).value(), std::move(engine.bound) }, globals, nb::borrow(name), where.filename, templateClass);
}

int PyEnvironment::Traverse(visitproc visit, void* arg) const
{
    for (const auto* object : { &m_globals, &m_filters, &m_tests, &m_loader, &m_undefined, &m_finalize, &m_autoescape, &m_extensions, &m_options, &m_policiesSnapshot, &m_policiesDumps })
    {
        Py_VISIT(object->ptr());
    }
    Py_VISIT(m_bound.ptr());
    Py_VISIT(m_engineGlobalsBound.ptr());
    Py_VISIT(m_chatClock.ptr());
    for (size_t i = 0; i < 4; ++i)
    {
        Py_VISIT(m_gettext[i].ptr());
        Py_VISIT(m_gettextGlobals[i].ptr());
    }
    if (m_loaderHandler)
    {
        return m_loaderHandler->Traverse(visit, arg);
    }
    return 0;
}

void PyEnvironment::Clear() noexcept
{
    // The engine borrows what goes now: it is dropped first, and the environment is unusable
    m_cleared = true;
    if (m_joinTarget)
    {
        m_joinTarget->env = nullptr;
    }
    m_env.reset();
    m_retired.clear();
    if (m_loaderHandler)
    {
        m_loaderHandler->Clear();
    }
    m_loaderHandler.reset();
    m_boundFilters.clear();
    m_boundTests.clear();
    m_globals = nb::none();
    m_filters = nb::none();
    m_tests = nb::none();
    m_loader = nb::none();
    m_undefined = nb::none();
    m_finalize = nb::none();
    m_policiesSnapshot = nb::object();
    m_policiesDumps = nb::object();
    m_policies.reset();
    m_autoescape = nb::none();
    m_extensions = nb::none();
    m_options = nb::none();
    m_engineGlobals.reset();
    m_bound = nb::none();
    m_engineGlobalsBound = nb::none();
    m_chatClock = nb::none();
    for (size_t i = 0; i < 4; ++i)
    {
        m_gettext[i] = nb::none();
        m_gettextGlobals[i] = nb::none();
    }
}

PyTemplate::PyTemplate(nb::handle source, nb::handle environment, nb::kwargs kwargs)
    : m_environment(environment.is_none() ? nb::cast(new PyEnvironment(kwargs), nb::rv_policy::take_ownership) : nb::borrow(environment))
    , m_globals(nb::dict())
    , m_name(nb::none())
    , m_filename(nb::none())
{
    // Jinja2 makes an environment of the arguments
    auto& env = nb::cast<PyEnvironment&>(m_environment);
    auto loaded = env.Load(source);
    m_template = std::move(loaded.engine);
    m_envGlobals = env.Globals();
    m_bound = std::move(loaded.bound);
}

PyTemplate::PyTemplate(jinja2::Template tpl, nb::object bound, nb::object environment, nb::object globals, nb::object name, nb::object filename)
    : m_template(std::move(tpl))
    , m_environment(std::move(environment))
    , m_globals(std::move(globals))
    , m_envGlobals(nb::cast<PyEnvironment&>(m_environment).Globals())
    , m_name(std::move(name))
    , m_filename(std::move(filename))
    , m_bound(std::move(bound))
{
}

template<typename Target>
void PyTemplate::RenderWith(Target& target, const nb::args& args, const nb::kwargs& kwargs, const PieceOptions* pieces) const
{
    ++g_renders;
    auto& env = LiveEnvironment();
    const auto entry = env.Enter();
    // dict(*args, **kwargs), as Jinja2's render
    jinja2::ValuesMap params;
    jinja2::RenderOptions options;
    std::vector<PyObject*>* held = pieces ? pieces->held : nullptr;
    // What the values of params and globals hold is no longer theirs to report once they start to go: before them
    const HeldForget forget(held);
    if (args.size() > 1)
    {
        RaiseTypeError("dict expected at most 1 argument, got " + std::to_string(args.size()));
    }
    if (args.size() == 1)
    {
        // Borrowed from `args`: a lazy render holds no reference of its own on its fiber (PyLazyRender::Traverse)
        const nb::handle first = PyTuple_GetItem(args.ptr(), 0);
        if (PyDict_CheckExact(first.ptr()))
        {
            AddParams(params, first, held);
        }
        else
        {
            AddParams(params, Names().dictType(first), held);
        }
    }
    AddParams(params, kwargs, held);
    RenderState state;
    state.undefinedClass = env.UndefinedClass();
    InitRenderState(state, m_environment);
    state.sandboxAllNames = entry.sandboxAllNames;
    state.templateGlobals = m_globals.ptr();
    state.maxSequenceSize = env.MaxSequenceSize();
    // The globals: the environment's, then the template's over them
    // Held for the render: a callable the render runs may start another engine environment
    const nb::object engineGlobalsBound = env.EngineGlobalsBound();
    auto globals = env.EngineGlobals() ? std::make_shared<jinja2::ValuesMap>(*env.EngineGlobals()) : std::make_shared<jinja2::ValuesMap>();
    env.AddUninstalledGettext(*globals);
    AddGlobals(*globals, m_envGlobals, state.sandbox != nullptr, held);
    if (!m_globals.is_none())
    {
        AddGlobals(*globals, m_globals, state.sandbox != nullptr, held);
    }
    options.globals = std::move(globals);
    options.filterPolicies = entry.policies;
    options.poll = RenderPoll();
    if (pieces)
    {
        options.writeEachPiece = true;
        options.suspension = pieces->suspension;
        options.pieceIsMarkup = pieces->pieceIsMarkup;
    }
    auto result = [&] {
        const RenderScope scope(state);
        return m_template.RenderTo(target, params, options);
    }();
    RaiseAbort(state);
    if (!result)
    {
        const auto& error = result.error();
        ErrorSource where;
        where.name = m_name;
        where.filename = m_filename;
        where.environment = m_environment;
        // The template itself parsed when it was loaded, so a syntax error here is in one it includes, imports or extends
        where.syntaxNote = kIncludedSyntaxNote;
        where.render = true;
        RaiseError(error, where);
    }
}

nb::object PyTemplate::Render(const nb::args& args, const nb::kwargs& kwargs) const
{
    std::string output;
    RenderWith(output, args, kwargs);
    return MakeStr(output);
}

namespace
{
// Each item of the render to a Python callable, a str or, where Jinja2's is, a Markup (docs/tasks/0493); what the
// callable raises ends the render and is raised again as itself (the engine keeps a sink's exception)
class CallbackSink final : public jinja2::OutputSink
{
public:
    explicit CallbackSink(nb::handle write)
        : m_write(write)
    {
    }
    void Write(std::string_view text) override { m_write(m_markup ? Names().markupType(MakeStr(text)) : MakeStr(text)); }
    // Set by the render before each Write (RenderOptions::pieceIsMarkup)
    bool* Markup() noexcept { return &m_markup; }

private:
    nb::handle m_write;
    bool m_markup = false;
};
} // namespace

void PyTemplate::RenderTo(nb::handle write, const nb::args& args, const nb::kwargs& kwargs) const
{
    CallbackSink sink(write);
    PieceOptions pieces;
    pieces.pieceIsMarkup = sink.Markup();
    RenderWith(sink, args, kwargs, &pieces);
}

void PyTemplate::RenderSuspended(jinja2::OutputSink& sink, const nb::args& args, const nb::kwargs& kwargs, const PieceOptions& pieces) const
{
    RenderWith(sink, args, kwargs, &pieces);
}

int PyTemplate::Traverse(visitproc visit, void* arg) const
{
    Py_VISIT(m_environment.ptr());
    Py_VISIT(m_globals.ptr());
    Py_VISIT(m_envGlobals.ptr());
    Py_VISIT(m_name.ptr());
    Py_VISIT(m_filename.ptr());
    Py_VISIT(m_bound.ptr());
    return 0;
}

void PyTemplate::Clear() noexcept
{
    m_environment = nb::none();
    m_globals = nb::none();
    m_envGlobals = nb::none();
    m_name = nb::none();
    m_filename = nb::none();
    m_bound = nb::none();
}

void BindEnvironment(nb::module_& module)
{
    module.def("_native_renders", [] { return g_renders; });
    auto environment = nb::class_<PyEnvironment>(module, "Environment", nb::type_slots(g_environmentSlots), nb::is_weak_referenceable())
                           .def(nb::init<nb::kwargs>())
                           // The Python layer (jinja2cpp/environment.py) has Jinja2's from_string,
                           // get_template and the rest, and passes the template class
                           .def(
                               "_from_string",
                               [](PyEnvironment& self, nb::handle source, nb::handle globals, nb::handle templateClass) { return self.FromString(source, globals, templateClass); },
                               "source"_a,
                               "globals"_a.none(),
                               "template_class"_a)
                           .def("_drop_engine_cache", &PyEnvironment::DropEngineCache)
                           .def(
                               "_load",
                               [](PyEnvironment& self, nb::handle name, nb::handle globals, nb::handle templateClass, nb::handle cached) { return self.LoadTemplate(name, globals, templateClass, cached); },
                               "name"_a.none(),
                               "globals"_a.none(),
                               "template_class"_a,
                               "cached"_a.none() = nb::none())
                           .def(
                               "_compile",
                               [](PyEnvironment& self, nb::handle source, nb::handle name, nb::handle filename, nb::handle globals, nb::handle templateClass) {
                                   return self.Compile(source, name, filename, globals, templateClass);
                               },
                               "source"_a,
                               "name"_a.none(),
                               "filename"_a.none(),
                               "globals"_a.none(),
                               "template_class"_a)
                           .def("_set_chat_templates", &PyEnvironment::SetChatTemplates, "clock"_a.none())
                           .def("_set_gettext", &PyEnvironment::SetGettext, "gettext"_a.none(), "ngettext"_a.none(), "pgettext"_a.none(), "npgettext"_a.none())
                           .def_prop_rw(
                               "_gettext_globals", [](const PyEnvironment& self) { return self.GettextGlobals(); }, [](PyEnvironment& self, nb::handle value) { self.SetGettextGlobals(value); })
                           .def_prop_rw(
                               "newstyle_gettext", [](const PyEnvironment& self) { return self.NewstyleGettext(); }, [](PyEnvironment& self, bool value) { self.SetNewstyleGettext(value); })
                           .def(
                               "install_gettext_translations",
                               [](nb::handle self, nb::handle translations, nb::handle newstyle) { return nb::module_::import_("jinja2cpp.ext").attr("_install")(self, translations, newstyle); },
                               "translations"_a,
                               "newstyle"_a.none() = nb::none())
                           .def(
                               "install_null_translations",
                               [](nb::handle self, nb::handle newstyle) { return nb::module_::import_("jinja2cpp.ext").attr("_install_null")(self, newstyle); },
                               "newstyle"_a.none() = nb::none())
                           .def(
                               "install_gettext_callables",
                               [](nb::handle self, nb::handle gettext, nb::handle ngettext, nb::handle newstyle, nb::handle pgettext, nb::handle npgettext) {
                                   return nb::module_::import_("jinja2cpp.ext").attr("_install_callables")(self, gettext, ngettext, newstyle, pgettext, npgettext);
                               },
                               "gettext"_a,
                               "ngettext"_a,
                               "newstyle"_a = nb::none(),
                               "pgettext"_a = nb::none(),
                               "npgettext"_a = nb::none())
                           .def(
                               "uninstall_gettext_translations",
                               [](nb::handle self, nb::handle translations) { return nb::module_::import_("jinja2cpp.ext").attr("_uninstall")(self, translations); },
                               "translations"_a.none())
                           .def_prop_rw(
                               "globals", [](const PyEnvironment& self) { return self.GetDict("globals"); }, [](PyEnvironment& self, nb::handle value) { self.SetDict("globals", value); })
                           .def_prop_rw(
                               "filters", [](const PyEnvironment& self) { return self.GetDict("filters"); }, [](PyEnvironment& self, nb::handle value) { self.SetDict("filters", value); })
                           .def_prop_rw(
                               "tests", [](const PyEnvironment& self) { return self.GetDict("tests"); }, [](PyEnvironment& self, nb::handle value) { self.SetDict("tests", value); });
    for (const char* option : g_options)
    {
        const std::string name = option;
        environment.def_prop_rw(
            option, [name](const PyEnvironment& self) { return self.GetOption(name); }, [name](PyEnvironment& self, nb::handle value) { self.SetOption(name, value); }, nb::arg("value").none());
    }

    nb::class_<PyTemplate>(module, "Template", nb::type_slots(g_templateSlots), nb::is_weak_referenceable())
        .def(nb::init<nb::handle, nb::handle, nb::kwargs>(), "source"_a, "_environment"_a.none() = nb::none(), "kwargs"_a)
        .def("render", &PyTemplate::Render)
        .def("_render_to", &PyTemplate::RenderTo)
        .def("_lazy", &MakeLazyRender)
        .def("_make_module", &PyTemplate::MakeModule, "params"_a)
        .def("_undeclared", &PyTemplate::FindUndeclared)
        .def("_referenced", &PyTemplate::FindReferenced)
        .def_prop_ro("environment", &PyTemplate::Environment)
        .def_prop_ro("name", &PyTemplate::Name)
        .def_prop_ro("filename", &PyTemplate::Filename)
        .def_prop_ro(
            "globals",
            [](const PyTemplate& self) {
                // Jinja2's ChainMap of the template's own globals over the environment's: a write
                // lands in the template's, which its renders read
                return nb::module_::import_("collections").attr("ChainMap")(self.TemplateGlobals(), self.EnvironmentGlobals());
            });
}
} // namespace jinja2cpp_py
