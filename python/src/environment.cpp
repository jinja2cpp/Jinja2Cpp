#include "environment.h"

#include "convert.h"
#include "errors.h"
#include "loaders.h"
#include "poll.h"

#include "adapters.h"
#include "dialect.h"
#include "host_object.h"
#include "internal_value.h"
#include "recursion_guard.h"
#include "template_env_impl.h"
#include "template_impl.h"

#include <jinja2cpp/features.h>
#include <jinja2cpp/template.h>
#include <jinja2cpp/template_env.h>
#if JINJA2CPP_HAS_CHAT_TEMPLATES
#include <jinja2cpp/chat_template.h>
#endif

#include <chrono>

#include <cstdint>
#include <memory>
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

// The params of a render: the str keys of a mapping as values of the engine
void AddParams(jinja2::InternalValueMap& params, nb::handle mapping)
{
    for (auto [name, value] : nb::borrow<nb::dict>(mapping))
    {
        if (PyUnicode_Check(name.ptr()))
        {
            params[std::string(Utf8View(name))] = ToEngine(value);
        }
    }
}

// The globals of a render. A sandbox replaces the global `range` (jinja2cpp.sandbox.safe_range, marked
// `_native_range`) by the engine's own, which stops at the environment's Limits::maxRange without a call
// to Python
void AddGlobals(jinja2::InternalValueMap& params, nb::handle mapping, bool sandboxed)
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
        params[std::string(Utf8View(name))] = ToEngine(value);
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

PyEnvironment::~PyEnvironment() = default;

void PyEnvironment::Keep(nb::handle object)
{
    for (const auto& kept : m_kept)
    {
        if (kept.ptr() == object.ptr())
        {
            return;
        }
    }
    m_kept.push_back(nb::borrow(object));
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
            Keep(value);
            settings.finalize = jinja2::host::EngineUserCallable(BorrowedCallable(value));
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
    m_env.reset();
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
        Keep(value);
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
    // Templates loaded with an earlier loader keep reading it
    if (!loader.is_none())
    {
        Keep(loader);
    }
    m_loaderHandler.reset();
}

void PyEnvironment::Rebuild()
{
    m_env = std::make_unique<jinja2::TemplateEnv>();
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
    m_env->SetSettings(settings);
    if (immutable)
    {
        jinja2::detail::TemplateEnvAccess::GetImpl(*m_env)->SetDialect(jinja2::detail::Dialect::ImmutableSandbox);
    }
    m_sandboxed = sandboxed;
    if (sandboxed)
    {
        m_sandboxPublicNames = SandboxJudgesPublicNames(nb::find(this));
        jinja2::detail::TemplateEnvAccess::GetImpl(*m_env)->SetSandboxPolicy(MakePythonSandboxPolicy(nb::find(this), m_sandboxPublicNames));
    }
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
    ApplyGettext();
}

void PyEnvironment::ApplyChatTemplates()
{
#if JINJA2CPP_HAS_CHAT_TEMPLATES
    jinja2::ChatTemplateOptions options;
    if (m_chatClock)
    {
        // Called on the rendering thread, which holds the GIL (the render does)
        options.now = [clock = m_chatClock] {
            const double seconds = nb::cast<double>(nb::handle(clock)());
            return std::chrono::system_clock::time_point(std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::duration<double>(seconds)));
        };
    }
    jinja2::ConfigureForChatTemplates(*m_env, std::move(options));
    // A render passes its own globals, which replace the environment's: they are added to them
    auto globals = std::make_shared<jinja2::ValuesMap>();
    m_env->ApplyGlobals([&globals](const jinja2::ValuesMap& values) { *globals = values; });
    m_engineGlobals = std::move(globals);
#endif
}

void PyEnvironment::SetChatTemplates(nb::handle clock)
{
    CheckAlive();
#if JINJA2CPP_HAS_CHAT_TEMPLATES
    if (!clock.is_none())
    {
        Keep(clock);
    }
    m_chatClock = clock.is_none() ? nullptr : clock.ptr();
    m_chatTemplates = true;
    // Templates loaded before keep their environment; the next load rebuilds it with the preset
    m_env.reset();
#else
    (void)clock;
    RaiseTypeError("this jinja2cpp was built without chat templates (JINJA2CPP_WITH_CHAT_TEMPLATES=OFF)");
#endif
}

void PyEnvironment::ApplyGettext()
{
    if (!m_env)
    {
        return;
    }
    // A callable left out stays unset: the message is not translated
    const auto callable = [](PyObject* object) { return object ? jinja2::host::EngineUserCallable(BorrowedCallable(object)) : jinja2::UserCallable(); };
    m_env->InstallGettextCallables(callable(m_gettext[0]), callable(m_gettext[1]), callable(m_gettext[2]), callable(m_gettext[3]));
}

void PyEnvironment::SetGettext(nb::handle gettext, nb::handle ngettext, nb::handle pgettext, nb::handle npgettext)
{
    CheckAlive();
    const nb::handle callables[4] = { gettext, ngettext, pgettext, npgettext };
    for (size_t i = 0; i < 4; ++i)
    {
        if (callables[i].is_none())
        {
            m_gettext[i] = nullptr;
            continue;
        }
        Keep(callables[i]);
        m_gettext[i] = callables[i].ptr();
    }
    ApplyGettext();
}

void PyEnvironment::CheckSandboxHooks() const
{
    if (!m_sandboxed)
    {
        return;
    }
    CheckNativeSandboxHooks(nb::find(this).ptr());
}

jinja2::TemplateEnv& PyEnvironment::Prepared()
{
    CheckAlive();
    if (!m_env)
    {
        Rebuild();
    }
    if (m_sandboxed)
    {
        // is_safe_attribute may have been replaced since the last load, on the class or the instance
        const auto self = nb::find(this);
        if (const bool judges = SandboxJudgesPublicNames(self); judges != m_sandboxPublicNames)
        {
            m_sandboxPublicNames = judges;
            jinja2::detail::TemplateEnvAccess::GetImpl(*m_env)->SetSandboxPolicy(MakePythonSandboxPolicy(self, judges));
        }
    }
    // A template binds the filters and tests when it loads, as in Jinja2
    auto sync = [this](nb::handle dict, const char* what, std::vector<std::pair<std::string, PyObject*>>& bound, bool filters) {
        auto entries = Entries(dict, what);
        if (entries == bound)
        {
            return;
        }
        for (const auto& [name, object] : bound)
        {
            filters ? m_env->RemoveFilter(name) : m_env->RemoveTest(name);
        }
        for (const auto& [name, object] : entries)
        {
            Keep(object);
            auto callable = jinja2::host::EngineUserCallable(BorrowedCallable(object));
            filters ? m_env->AddFilter(name, std::move(callable)) : m_env->AddTest(name, std::move(callable));
        }
        bound = std::move(entries);
    };
    sync(m_filters, "filters", m_boundFilters, true);
    sync(m_tests, "tests", m_boundTests, false);
    return *m_env;
}

void PyEnvironment::CheckAlive() const
{
    if (m_cleared)
    {
        RaiseTemplateRuntimeError("the environment was cleared by the garbage collector");
    }
}

ErrorSource PyEnvironment::SourceOf(const std::string& name) const
{
    if (m_loaderHandler)
    {
        return m_loaderHandler->SourceOf(name);
    }
    ErrorSource source;
    source.name = MakeStr(name);
    return source;
}

nb::object PyEnvironment::MakeTemplate(jinja2::Template tpl, nb::handle globals, nb::object name, nb::object filename, nb::handle templateClass)
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
    auto made = std::make_unique<PyTemplate>(std::move(tpl), nb::borrow(self), std::move(templateGlobals), std::move(name), std::move(filename));
    nb::object instance = nb::inst_take_ownership(templateClass, made.get());
    made.release(); // NOLINT(bugprone-unused-return-value): the instance owns it now
    return instance;
}

jinja2::Template PyEnvironment::Load(nb::handle source, const std::string& name)
{
    auto& env = Prepared();
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
        if (!name.empty())
        {
            where.name = MakeStr(name);
        }
        RaiseError(result.error(), where);
    }
    return std::move(result).value();
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

nb::object PyEnvironment::LoadTemplate(nb::handle name, nb::handle globals, nb::handle templateClass)
{
    if (nb::isinstance<PyTemplate>(name))
    {
        return nb::borrow(name);
    }
    auto& env = Prepared();
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
            RaiseTemplateNotFound(name);
        }
        RaiseError(error, SourceOf(text));
    }
    auto where = SourceOf(text);
    return MakeTemplate(std::move(result).value(), globals, nb::borrow(name), where.filename, templateClass);
}

int PyEnvironment::Traverse(visitproc visit, void* arg) const
{
    for (const auto* object : { &m_globals, &m_filters, &m_tests, &m_loader, &m_undefined, &m_finalize, &m_autoescape, &m_extensions, &m_options })
    {
        Py_VISIT(object->ptr());
    }
    for (const auto& object : m_kept)
    {
        Py_VISIT(object.ptr());
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
    m_env.reset();
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
    m_autoescape = nb::none();
    m_extensions = nb::none();
    m_options = nb::none();
    m_kept.clear();
}

PyTemplate::PyTemplate(nb::handle source, nb::handle environment, nb::kwargs kwargs)
    : m_environment(environment.is_none() ? nb::cast(new PyEnvironment(kwargs), nb::rv_policy::take_ownership) : nb::borrow(environment))
    , m_globals(nb::dict())
    , m_name(nb::none())
    , m_filename(nb::none())
{
    // Jinja2 makes an environment of the arguments
    auto& env = nb::cast<PyEnvironment&>(m_environment);
    m_template = env.Load(source);
    m_envGlobals = env.Globals();
}

PyTemplate::PyTemplate(jinja2::Template tpl, nb::object environment, nb::object globals, nb::object name, nb::object filename)
    : m_template(std::move(tpl))
    , m_environment(std::move(environment))
    , m_globals(std::move(globals))
    , m_envGlobals(nb::cast<PyEnvironment&>(m_environment).Globals())
    , m_name(std::move(name))
    , m_filename(std::move(filename))
{
}

template<typename Target>
void PyTemplate::RenderWith(Target& target, nb::args args, nb::kwargs kwargs) const
{
    ++g_renders;
    auto& env = LiveEnvironment();
    env.CheckSandboxHooks();
    // dict(*args, **kwargs), as Jinja2's render
    jinja2::InternalValueMap params;
    if (args.size() > 1)
    {
        RaiseTypeError("dict expected at most 1 argument, got " + std::to_string(args.size()));
    }
    if (args.size() == 1)
    {
        nb::object first = args[0];
        AddParams(params, PyDict_CheckExact(first.ptr()) ? first : Names().dictType(first));
    }
    AddParams(params, kwargs);
    RenderState state;
    state.undefinedClass = env.UndefinedClass();
    InitRenderState(state, m_environment);
    state.templateGlobals = m_globals.ptr();
    // The globals: the environment's, then the template's over them
    jinja2::InternalValueMap globals;
    if (const auto& engineGlobals = env.EngineGlobals())
    {
        for (const auto& [name, value] : *engineGlobals)
        {
            globals[name] = jinja2::LendGlobal(value, engineGlobals);
        }
    }
    AddGlobals(globals, m_envGlobals, state.sandbox != nullptr);
    if (!m_globals.is_none())
    {
        AddGlobals(globals, m_globals, state.sandbox != nullptr);
    }
    std::optional<jinja2::ErrorInfo> error;
    {
        const RenderScope scope(state);
        const HostPollScope poll;
        error = jinja2::TemplateImpl<char>::RenderInternal(m_template, target, params, &globals);
    }
    if (error)
    {
        ErrorSource where;
        where.name = m_name;
        where.filename = m_filename;
        // The template itself parsed when it was loaded, so a syntax error here is in one it includes, imports or extends
        where.syntaxNote = kIncludedSyntaxNote;
        RaiseError(*error, where);
    }
}

nb::object PyTemplate::Render(nb::args args, nb::kwargs kwargs) const
{
    std::string output;
    RenderWith(output, std::move(args), std::move(kwargs));
    return MakeStr(output);
}

namespace
{
// Each chunk of the render to a Python callable; what the callable raises ends the render and is
// raised again as itself (the engine keeps a sink's exception)
class CallbackSink final : public jinja2::OutputSink
{
public:
    explicit CallbackSink(nb::handle write)
        : m_write(write)
    {
    }
    void Write(std::string_view text) override { m_write(MakeStr(text)); }

private:
    nb::handle m_write;
};
} // namespace

void PyTemplate::RenderTo(nb::handle write, nb::args args, nb::kwargs kwargs) const
{
    CallbackSink sink(write);
    RenderWith(sink, std::move(args), std::move(kwargs));
}

int PyTemplate::Traverse(visitproc visit, void* arg) const
{
    Py_VISIT(m_environment.ptr());
    Py_VISIT(m_globals.ptr());
    Py_VISIT(m_envGlobals.ptr());
    Py_VISIT(m_name.ptr());
    Py_VISIT(m_filename.ptr());
    return 0;
}

void PyTemplate::Clear() noexcept
{
    m_environment = nb::none();
    m_globals = nb::none();
    m_envGlobals = nb::none();
    m_name = nb::none();
    m_filename = nb::none();
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
                           .def(
                               "_load",
                               [](PyEnvironment& self, nb::handle name, nb::handle globals, nb::handle templateClass) { return self.LoadTemplate(name, globals, templateClass); },
                               "name"_a.none(),
                               "globals"_a.none(),
                               "template_class"_a)
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
