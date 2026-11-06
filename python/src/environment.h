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

    nb::object FromString(nb::handle source, nb::handle globals);
    // The engine's template of a source
    jinja2::Template Load(nb::handle source);
    nb::object GetTemplate(nb::handle name, nb::handle parent, nb::handle globals);

    // The engine's environment with the filters and tests of the dicts, as a template binds
    // them when it loads
    jinja2::TemplateEnv& Prepared();
    [[nodiscard]] PyObject* UndefinedClass() const noexcept { return m_undefined.ptr(); }
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
    nb::object MakeTemplate(jinja2::Template tpl, nb::handle globals, nb::object name, nb::object filename);

    std::unique_ptr<jinja2::TemplateEnv> m_env;
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
    bool m_cleared = false;
};

// jinja2cpp.Template
class PyTemplate
{
public:
    PyTemplate(jinja2::Template tpl, nb::object environment, nb::object globals, nb::object name, nb::object filename);
    // Template(source, **options): the template of a new environment
    PyTemplate(nb::handle source, nb::kwargs kwargs);
    ~PyTemplate() = default;
    PyTemplate(const PyTemplate&) = delete;
    PyTemplate& operator=(const PyTemplate&) = delete;
    PyTemplate(PyTemplate&&) = delete;
    PyTemplate& operator=(PyTemplate&&) = delete;

    nb::object Render(nb::args args, nb::kwargs kwargs) const;

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
} // namespace jinja2cpp_py

#endif // JINJA2CPP_PYTHON_ENVIRONMENT_H
