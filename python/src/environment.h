#ifndef JINJA2CPP_PYTHON_ENVIRONMENT_H
#define JINJA2CPP_PYTHON_ENVIRONMENT_H

#include "common.h"
#include "errors.h"
#include "loaders.h"

#include <jinja2cpp/template.h>
#include <jinja2cpp/template_env.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace jinja2cpp_py
{
// jinja2cpp.Environment. Python objects the engine calls (filters, tests, finalize, the
// autoescape callable, the loader) are owned here and borrowed by the engine: the environment
// lives while anything of it runs, since every Template holds it. tp_traverse reports them, so
// a cycle through a closure in env.filters or env.globals is collected (0248 D4)
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
    [[nodiscard]] bool NewstyleGettext() const noexcept { return m_newstyleGettext; }
    void SetNewstyleGettext(bool newstyle) noexcept { m_newstyleGettext = newstyle; }

    // The template of a source or of a loader's name, an instance of `templateClass` (Jinja2's
    // Environment.template_class: a subclass of Template)
    nb::object FromString(nb::handle source, nb::handle globals, nb::handle templateClass);
    nb::object LoadTemplate(nb::handle name, nb::handle globals, nb::handle templateClass);
    // A source a loader gave, with its name and filename (BaseLoader.load)
    nb::object Compile(nb::handle source, nb::handle name, nb::handle filename, nb::handle globals, nb::handle templateClass);
    // The engine's template of a source named `name` in its errors
    jinja2::Template Load(nb::handle source, const std::string& name = std::string());

    // The engine's environment with the filters and tests of the dicts, as a template binds
    // them when it loads
    jinja2::TemplateEnv& Prepared();
    [[nodiscard]] PyObject* UndefinedClass() const noexcept { return m_undefined.ptr(); }
    // A SandboxedEnvironment: whether its hooks (call, getattr, getitem, intercepted operators) are the engine's
    // are checked again, as at a load, before a template renders or makes a module (an override set after the
    // load must not be ignored); nothing for another environment
    void CheckSandboxHooks() const;
    [[nodiscard]] const nb::object& Globals() const noexcept { return m_globals; }
    // Where a template the loader gives came from, for its errors and Template.filename
    ErrorSource SourceOf(const std::string& name) const;
    void CheckAlive() const;

    int Traverse(visitproc visit, void* arg) const;
    void Clear() noexcept;

private:
    void Rebuild();
    void SetLoader(nb::handle loader);
    void SetAutoescape(nb::handle value);
    void SetExtensions(nb::handle value);
    void Keep(nb::handle object);
    void ApplyGettext();
    nb::object MakeTemplate(jinja2::Template tpl, nb::handle globals, nb::object name, nb::object filename, nb::handle templateClass);

    std::unique_ptr<jinja2::TemplateEnv> m_env;
    // A SandboxedEnvironment, and whether its is_safe_attribute judges the engine's public names (read at each load)
    bool m_sandboxed = false;
    bool m_sandboxPublicNames = false;
    jinja2::Settings m_settings;
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
    // Every callable the engine was given: a template keeps the filters it bound when it
    // loaded, so none is dropped while the environment lives
    std::vector<nb::object> m_kept;
    std::vector<std::pair<std::string, PyObject*>> m_boundFilters;
    std::vector<std::pair<std::string, PyObject*>> m_boundTests;
    // The i18n callables, borrowed from m_kept; null for none
    PyObject* m_gettext[4] = {};
    bool m_newstyleGettext = false;
    bool m_cleared = false;
};

// jinja2cpp.Template
class PyTemplate
{
public:
    PyTemplate(jinja2::Template tpl, nb::object environment, nb::object globals, nb::object name, nb::object filename);
    // Template(source, **options): the template of a new environment, or of `environment` when
    // the Python layer made one (jinja2cpp.Template.__init__)
    PyTemplate(nb::handle source, nb::handle environment, nb::kwargs kwargs);
    ~PyTemplate() = default;
    PyTemplate(const PyTemplate&) = delete;
    PyTemplate& operator=(const PyTemplate&) = delete;
    PyTemplate(PyTemplate&&) = delete;
    PyTemplate& operator=(PyTemplate&&) = delete;

    nb::object Render(nb::args args, nb::kwargs kwargs) const;
    // The environment, checked to be alive
    PyEnvironment& LiveEnvironment() const;
    // Template.make_module of the Python layer (template_module.cpp): (body, exports) for the
    // params, a dict of str names, over the environment's globals
    nb::tuple MakeModule(nb::handle params) const;
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
    jinja2::Template m_template;
    nb::object m_environment;
    nb::object m_globals;
    // The environment's globals dict when the template was made: Jinja2's template.globals is a
    // ChainMap over that dict, so replacing Environment.globals later leaves it as it was
    nb::object m_envGlobals;
    nb::object m_name;
    nb::object m_filename;
};

// Registers Environment and Template
void BindEnvironment(nb::module_& module);
// Registers the macros of a module (template_module.cpp)
void BindTemplateModule(nb::module_& module);
} // namespace jinja2cpp_py

#endif // JINJA2CPP_PYTHON_ENVIRONMENT_H
