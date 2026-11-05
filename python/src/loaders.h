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
    // The name, filename and source of a template it gives, for TemplateSyntaxError
    [[nodiscard]] virtual ErrorSource SourceOf(const std::string& name) const = 0;
};
using LoaderPtr = std::shared_ptr<const Loader>;

LoaderPtr MakeLoader(nb::handle loader, nb::handle environment);
} // namespace jinja2cpp_py

#endif // JINJA2CPP_PYTHON_LOADERS_H
