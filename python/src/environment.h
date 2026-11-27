#ifndef JINJA2CPP_PYTHON_ENVIRONMENT_H
#define JINJA2CPP_PYTHON_ENVIRONMENT_H

#include "common.h"
#include "errors.h"
#include "loaders.h"

#include <jinja2cpp/features.h>
#include <jinja2cpp/template.h>
#include <jinja2cpp/template_env.h>

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace jinja2cpp_py
{
// jinja2cpp.Environment. Python objects the engine calls (filters, tests, finalize, the
// autoescape callable, the loader) are borrowed by the engine and kept by a list of them for each
// engine environment (BoundObjects), which the environment and every Template loaded from that engine
// environment hold. tp_traverse reports them, so a cycle through a closure in env.filters or
// env.globals is collected (0248 D4)
struct JoinPathTarget;

class PyEnvironment
{
public:
    explicit PyEnvironment(nb::kwargs kwargs);
    ~PyEnvironment();
    PyEnvironment(const PyEnvironment&) = delete;
    PyEnvironment& operator=(const PyEnvironment&) = delete;
    PyEnvironment(PyEnvironment&&) = delete;
    PyEnvironment& operator=(PyEnvironment&&) = delete;

    // One of Environment's keyword arguments, at construction or set later as an attribute
    void SetOption(const std::string& name, nb::handle value);
    [[nodiscard]] nb::object GetOption(const std::string& name) const;
    // env.globals, env.filters, env.tests: plain dicts, read when a template loads (filters,
    // tests) or renders (globals)
    [[nodiscard]] nb::object GetDict(const std::string& name) const;
    void SetDict(const std::string& name, nb::handle value);

    // The translation callables of the i18n extension, as the engine takes them (None for none):
    // gettext, ngettext, pgettext, npgettext. The environment keeps them alive
    void SetGettext(nb::handle gettext, nb::handle ngettext, nb::handle pgettext, nb::handle npgettext);
    // The callables an oldstyle install put in `globals` (ext.py), so that the next install removes them: kept by the
    // environment, where its traverse reaches, since one may hold the environment (a bound method of it)
    [[nodiscard]] nb::tuple GettextGlobals() const;
    void SetGettextGlobals(nb::handle callables);
    // With jinja2.ext.i18n and no translations installed, an undefined value for each gettext global, as Jinja2 has
    // none of them until an install (docs/tasks/0417): `_()` and `{% trans %}` then raise UndefinedError. Added
    // where `globals` has no value of that name, below env.globals, which may set one by hand
    void AddUninstalledGettext(jinja2::ValuesMap& globals) const;
    // transformers' chat-template environment (jinja2cpp.integrations.transformers): the engine's
    // ConfigureForChatTemplates on every rebuild, with `clock` (a callable giving a POSIX
    // timestamp, kept alive by the environment; None for the system clock) as strftime_now's clock
    void SetChatTemplates(nb::handle clock);
    // Environment.cache cleared, an entry deleted or the cache replaced: the engine's cache goes too, so that the next
    // get_template reads the loader again, as Jinja2's does (templates loaded before keep what they have)
    void DropEngineCache() noexcept { RetireEngine(); }
    [[nodiscard]] bool NewstyleGettext() const noexcept { return m_newstyleGettext; }
    // Templates loaded from now on read it, as Jinja2 reads it when it compiles a `{% trans %}` block
    void SetNewstyleGettext(bool newstyle);

    // The template of a source or of a loader's name, an instance of `templateClass` (Jinja2's
    // Environment.template_class: a subclass of Template)
    nb::object FromString(nb::handle source, nb::handle globals, nb::handle templateClass);
    // `cached`, the template an earlier load of `name` gave (Environment.cache), is given again while the engine
    // has the same template cached: not reloaded, its source not changed (0433)
    nb::object LoadTemplate(nb::handle name, nb::handle globals, nb::handle templateClass, nb::handle cached);
    // A source a loader gave, with its name and filename (BaseLoader.load)
    nb::object Compile(nb::handle source, nb::handle name, nb::handle filename, nb::handle globals, nb::handle templateClass);
    // An engine template and the list of what the engine environment it was loaded from borrows (BoundObjects), read
    // before the load: Python the load runs (a loader, an autoescape selector) may replace a filter and so start a
    // new engine environment with a list of its own
    struct Loaded
    {
        jinja2::Template engine;
        nb::object bound;
    };
    // The engine's template of a source named `name` in its errors
    Loaded Load(nb::handle source, const std::string& name = std::string());

    // The engine's environment with the filters and tests of the dicts, as a template binds
    // them when it loads, and its BoundObjects. The pointer keeps it while a load that runs Python
    // starts another
    struct Engine
    {
        std::shared_ptr<jinja2::TemplateEnv> env;
        nb::object bound;
    };
    Engine Prepared();
    [[nodiscard]] PyObject* UndefinedClass() const noexcept { return m_undefined.ptr(); }
    // A list of what the engine environment templates load from now borrows: a template keeps it, so the callables
    // it was compiled with live while it does, and the environment lets go of one replaced since (docs/tasks/0406)
    [[nodiscard]] const nb::object& BoundObjects() const noexcept { return m_bound; }
    // What an entry into the code of a template of this engine (a render, a module, a macro of one, a macro a Python
    // callable got) learns of the environment as it is now
    struct EntryState
    {
        // Whether the sandbox's is_safe_attribute judges the engine's public names too (RenderState::sandboxAllNames)
        bool sandboxAllNames = false;
        // env.policies as the engine takes them now, for RenderOptions::filterPolicies of the render: never null
        std::shared_ptr<const jinja2::FilterPolicies> policies;
    };
    // Run at each entry: env.policies is read again (compared with the copy the last entry took; the engine's are
    // rebuilt when they differ, UnsupportedError for a value it cannot take), and a SandboxedEnvironment's hooks (call, getattr, getitem, intercepted operators) are checked
    // again, as at a load (an override set after the load must not be ignored), and whether its is_safe_attribute is
    // Jinja2's is read, for the sandbox policy of every template the entry runs
    EntryState Enter();
    [[nodiscard]] const nb::object& Globals() const noexcept { return m_globals; }
    // The globals the engine's environment has of its own (ConfigureForChatTemplates' raise_exception
    // and strftime_now), which a render adds below env.globals; null when there are none
    [[nodiscard]] const std::shared_ptr<const jinja2::ValuesMap>& EngineGlobals() const noexcept { return m_engineGlobals; }
    // What EngineGlobals borrow: the clock of their strftime_now, which a later SetChatTemplates replaces
    [[nodiscard]] const nb::object& EngineGlobalsBound() const noexcept { return m_engineGlobalsBound; }
    // Limits::maxSequenceSize of the environment, or the engine's built-in cap
    [[nodiscard]] std::uint64_t MaxSequenceSize() const noexcept { return m_settings.limits.maxSequenceSize != 0 ? m_settings.limits.maxSequenceSize : std::uint64_t{ 1 } << 26; }
    // Where a template the loader gives came from, for its errors and Template.filename
    ErrorSource SourceOf(const std::string& name, bool withSource = true) const;
    // The TemplateNotFound the loader raised for `name`, or None (Loader::NotFound)
    [[nodiscard]] nb::object LoaderNotFound(const std::string& name) const;
    void CheckAlive() const;

    int Traverse(visitproc visit, void* arg) const;
    void Clear() noexcept;

private:
    void Rebuild();
    // Installs or removes the engine environment's join-path function as the environment's class (or instance)
    // overrides join_path or not (docs/tasks/0410)
    void SyncJoinPath();
    void SetLoader(nb::handle loader);
    void SetAutoescape(nb::handle value);
    void SetExtensions(nb::handle value);
    // Adds `object` to BoundObjects: the engine environment borrows it
    void Keep(nb::handle object);
    // everyEngine: also into the earlier engine environments (an install); a rebuild installs into its new one only
    void ApplyGettext(bool everyEngine);
    // m_env goes, for the next load to start another. While templates loaded from it live (its list of BoundObjects
    // does), it is kept for ApplyGettext: Jinja2's install changes env.globals, which every template reads, so a later
    // install reaches the templates of every engine environment
    void RetireEngine() noexcept;
    struct Retired
    {
        // Owned by its list (a capsule in it), which its templates own: it goes with them
        std::weak_ptr<jinja2::TemplateEnv> env;
        // A weak reference to its list
        nb::object bound;
    };
    void ApplyChatTemplates();
    // env.policies compared with the copy of the last read: the engine's policies (m_policies) rebuilt and written to
    // the settings of the engine environment when they differ. Raises UnsupportedError for a value the engine cannot take
    std::shared_ptr<const jinja2::FilterPolicies> RefreshPolicies();
    nb::object MakeTemplate(Loaded tpl, nb::handle globals, nb::object name, nb::object filename, nb::handle templateClass);

    // Shared with the load in progress (Prepared)
    std::shared_ptr<jinja2::TemplateEnv> m_env;
    // The engine environments before m_env that templates still use (RetireEngine)
    std::vector<Retired> m_retired;
    // A SandboxedEnvironment
    bool m_sandboxed = false;
    jinja2::Settings m_settings;
    // env.policies as the engine takes them (m_settings.filterPolicies, and the i18n trimming in its extensions): shared
    // with the renders that run with them. m_policiesSnapshot is the copy of the dict they were made from
    // (jinja2cpp.fallback.native_policies); m_policiesDumps the Python wrapper of json.dumps_function and
    // json.dumps_kwargs the policies call, kept by the list of every engine environment (Keep) and reported by Traverse
    std::shared_ptr<const jinja2::FilterPolicies> m_policies = std::make_shared<const jinja2::FilterPolicies>();
    nb::object m_policiesSnapshot;
    // Not an owning reference: m_policies owns it (RefreshPolicies), and Traverse reports that one
    nb::handle m_policiesDumps;
    // Whether the list m_bound holds the wrapper m_policiesDumps (or an earlier one): a change of it starts a new list
    bool m_dumpsKept = false;
    nb::object m_globals;
    nb::object m_filters;
    nb::object m_tests;
    nb::object m_loader = nb::none();
    nb::object m_undefined;
    nb::object m_finalize = nb::none();
    nb::object m_autoescape = nb::bool_(false);
    nb::object m_extensions;
    nb::object m_options; // the other keyword arguments, accepted as Jinja2 takes them
    LoaderPtr m_loaderHandler;
    // What m_env borrows (BoundObjects): a new list with each new m_env. A filter or test replaced or removed starts
    // a new m_env, since the templates loaded before keep the callables they bound; the translations, which a render
    // reads from the environment at each call, are item 0, replaced in place by a new install
    nb::object m_bound;
    std::vector<std::pair<std::string, PyObject*>> m_boundFilters;
    std::vector<std::pair<std::string, PyObject*>> m_boundTests;
    // The i18n callables; None for none
    nb::object m_gettext[4];
    // The oldstyle globals; None for none
    nb::object m_gettextGlobals[4];
    bool m_newstyleGettext = false;
    // The style of the gettext globals, fixed by the last install (SetGettext)
    jinja2::GettextStyle m_gettextStyle = jinja2::GettextStyle::FollowLoad;
    // SetChatTemplates: on, and its clock (None for the system clock)
    bool m_chatTemplates = false;
#if JINJA2CPP_HAS_CHAT_TEMPLATES
    nb::object m_chatClock = nb::none();
#endif
    std::shared_ptr<const jinja2::ValuesMap> m_engineGlobals;
    nb::object m_engineGlobalsBound = nb::none();
    bool m_cleared = false;
    // The borrowed environment object the join-path function calls, nulled with the environment (Clear): shared with
    // the functions of every engine environment made, which then leave names as they are
    std::shared_ptr<JoinPathTarget> m_joinTarget;
    // Whether m_env has the function
    bool m_joinPathSet = false;
};

// A render that writes each of Jinja2's items to a sink (PyTemplate::RenderSuspended)
struct PieceOptions
{
    // The stack of the fiber the render may be suspended on (RenderOptions::suspension), or null
    const jinja2::RenderSuspension* suspension = nullptr;
    // Gets whether each item is Markup (RenderOptions::pieceIsMarkup), or null
    bool* pieceIsMarkup = nullptr;
    // Gets each Python object a value of the render's params and globals holds a reference to while the render
    // runs, once for each reference, and is cleared before they go; null for none (a lazy render's tp_traverse)
    std::vector<PyObject*>* held = nullptr;
};

// jinja2cpp.Template
class PyTemplate
{
public:
    // `bound`: what the engine environment `tpl` was loaded from borrows (PyEnvironment::Loaded)
    PyTemplate(jinja2::Template tpl, nb::object bound, nb::object environment, nb::object globals, nb::object name, nb::object filename);
    // Template(source, **options): the template of a new environment, or of `environment` when
    // the Python layer made one (jinja2cpp.Template.__init__)
    PyTemplate(nb::handle source, nb::handle environment, nb::kwargs kwargs);
    ~PyTemplate() = default;
    PyTemplate(const PyTemplate&) = delete;
    PyTemplate& operator=(const PyTemplate&) = delete;
    PyTemplate(PyTemplate&&) = delete;
    PyTemplate& operator=(PyTemplate&&) = delete;

    nb::object Render(const nb::args& args, const nb::kwargs& kwargs) const;
    // Template.generate: renders with the arguments of render(), calling `write` with each piece of
    // output in order, Jinja2's items (RenderOptions::writeEachPiece), Markup where Jinja2's are; an error comes after the items
    // written before it
    void RenderTo(nb::handle write, const nb::args& args, const nb::kwargs& kwargs) const;
    // The same to an engine sink, which may suspend the render in its Write: a lazy generate() on a fiber
    // (lazy_render.cpp, docs/tasks/0457)
    // By reference: a lazy render's traverse reports the references its object holds, and none other may be on the fiber
    void RenderSuspended(jinja2::OutputSink& sink, const nb::args& args, const nb::kwargs& kwargs, const PieceOptions& pieces) const;
    // The environment, checked to be alive
    PyEnvironment& LiveEnvironment() const;
    // Template.make_module of the Python layer (template_module.cpp): (body, exports) for the
    // params, a dict of str names, over the environment's globals
    nb::tuple MakeModule(nb::handle params, bool shared) const;
    // jinja2.meta.find_undeclared_variables, as a sorted list; find_referenced_templates, as a
    // list of the names (None for one that is not a constant)
    nb::object FindUndeclared() const;
    nb::object FindReferenced() const;

    [[nodiscard]] const jinja2::Template& Engine() const { return m_template; }
    [[nodiscard]] const nb::object& Environment() const { return m_environment; }
    [[nodiscard]] const nb::object& Name() const { return m_name; }
    [[nodiscard]] const nb::object& Filename() const { return m_filename; }
    [[nodiscard]] const nb::object& TemplateGlobals() const { return m_globals; }
    [[nodiscard]] const nb::object& EnvironmentGlobals() const { return m_envGlobals; }

    int Traverse(visitproc visit, void* arg) const;
    void Clear() noexcept;

private:
    template<typename Target>
    // `pieces`: a render that writes each item (RenderTo, RenderSuspended); null for a render to a string
    void RenderWith(Target& target, const nb::args& args, const nb::kwargs& kwargs, const PieceOptions* pieces = nullptr) const;
    jinja2::Template m_template;
    nb::object m_environment;
    nb::object m_globals;
    // The environment's globals dict when the template was made: Jinja2's template.globals is a
    // ChainMap over that dict, so replacing Environment.globals later leaves it as it was
    nb::object m_envGlobals;
    nb::object m_name;
    nb::object m_filename;
    // What the engine environment the template was loaded from borrows (PyEnvironment::BoundObjects): its filters
    // and tests among them, which the template's tree borrows
    nb::object m_bound;
};

// Registers Environment and Template
void BindEnvironment(nb::module_& module);
// Registers the macros of a module (template_module.cpp)
void BindTemplateModule(nb::module_& module);
} // namespace jinja2cpp_py

#endif // JINJA2CPP_PYTHON_ENVIRONMENT_H
