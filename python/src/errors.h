#ifndef JINJA2CPP_PYTHON_ERRORS_H
#define JINJA2CPP_PYTHON_ERRORS_H

#include "common.h"

#include <jinja2cpp/error_info.h>

#include <string>

namespace jinja2cpp_py
{
// Where a failed template came from, for TemplateSyntaxError's attributes: None when unknown
struct ErrorSource
{
    nb::object name = nb::none();
    nb::object filename = nb::none();
    nb::object source = nb::none();
    // A note for a TemplateSyntaxError or TemplateAssertionError (a template the render loaded failed to parse)
    const char* syntaxNote = nullptr;
    // The jinja2cpp.Environment the template is in, or None: the error's template, when it is another one
    // (an included template), is looked up in its loader, and TemplateNotFound names its search path
    nb::object environment = nb::none();
    // A render (render, generate, make_module) raised it: Jinja2 renders through a generator, so a StopIteration
    // ends it as `RuntimeError: generator raised StopIteration` (PEP 479), with the StopIteration as its cause (0477)
    bool render = false;
};

// The source of the template `name` in the loader of `environment` (a jinja2cpp.Environment)
ErrorSource SourceOfTemplate(nb::handle environment, const std::string& name, bool withSource = true);

// The note of a syntax error raised by a render: it is in a template the render included, imported or extended
extern const char* const kIncludedSyntaxNote;

// Raises the Python exception of an error of the engine (jinja2cpp.exceptions): a Python
// exception a callable raised is raised again as itself, with its traceback (0248 D9)
[[noreturn]] void RaiseError(const jinja2::ErrorInfo& error, const ErrorSource& source = {});
// The same for the exception being handled (call it from a catch handler)
[[noreturn]] void RaiseCurrentEngineError();

[[noreturn]] void RaiseTemplateRuntimeError(const std::string& message);
[[noreturn]] void RaiseTypeError(const std::string& message);
[[noreturn]] void RaiseTemplateNotFound(nb::handle name);
// Jinja2's exception for templates not found: TemplatesNotFound for several names, and the search path of
// a FileSystemLoader of `environment` (None when unknown) in the message (jinja2cpp.loaders._not_found)
[[noreturn]] void RaiseTemplateNotFound(nb::handle environment, nb::handle names);

// Jinja2's message of the error, without the engine's location prefix
std::string ErrorMessage(const jinja2::ErrorInfo& error);
} // namespace jinja2cpp_py

#endif // JINJA2CPP_PYTHON_ERRORS_H
