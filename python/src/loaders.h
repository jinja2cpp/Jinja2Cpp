#ifndef JINJA2CPP_PYTHON_LOADERS_H
#define JINJA2CPP_PYTHON_LOADERS_H

#include "common.h"
#include "errors.h"

#include <jinja2cpp/template_env.h>

#include <memory>
#include <string>

namespace jinja2cpp_py
{
// A Python loader as filesystem handlers of the engine: DictLoader reads its mapping,
// FileSystemLoader its search path through ConfinedFileSystem, and any other loader (Jinja2's
// own, a BaseLoader subclass) answers get_source(environment, name). The loader object and the
// environment are borrowed: the environment owns both
class Loader
{
public:
    Loader() = default;
    Loader(const Loader&) = delete;
    Loader& operator=(const Loader&) = delete;
    Loader(Loader&&) = delete;
    Loader& operator=(Loader&&) = delete;
    virtual ~Loader() = default;

    virtual void Install(jinja2::TemplateEnv& env) const = 0;
    // The Python objects the loader keeps (a get_source's uptodate callback may close over the
    // environment): visited by the environment's tp_traverse and dropped by its tp_clear
    virtual int Traverse(visitproc /*visit*/, void* /*arg*/) const { return 0; }
    virtual void Clear() const noexcept {}
    // The name, filename and source of a template it gives, for TemplateSyntaxError; without `withSource`
    // the source is not read (a Python loader's get_source is not called again)
    [[nodiscard]] virtual ErrorSource SourceOf(const std::string& name, bool withSource) const = 0;
    // The TemplateNotFound a Python loader raised when `name` was last not found, or None: Jinja2 lets
    // the loader's own exception through, with its message
    [[nodiscard]] virtual nb::object NotFound(const std::string& /*name*/) const { return nb::none(); }
};
using LoaderPtr = std::shared_ptr<const Loader>;

LoaderPtr MakeLoader(nb::handle loader, nb::handle environment);
} // namespace jinja2cpp_py

#endif // JINJA2CPP_PYTHON_LOADERS_H
