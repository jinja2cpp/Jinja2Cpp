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
};

// Raises the Python exception of an error of the engine (jinja2cpp.exceptions): a Python
// exception a callable raised is raised again as itself, with its traceback (0248 D9)
[[noreturn]] void RaiseError(const jinja2::ErrorInfo& error, const ErrorSource& source = {});
// The same for the exception being handled (call it from a catch handler)
[[noreturn]] void RaiseCurrentEngineError();

[[noreturn]] void RaiseTemplateRuntimeError(const std::string& message);
[[noreturn]] void RaiseTypeError(const std::string& message);
[[noreturn]] void RaiseTemplateNotFound(nb::handle name);

// Jinja2's message of the error, without the engine's location prefix
std::string ErrorMessage(const jinja2::ErrorInfo& error);
} // namespace jinja2cpp_py

#endif // JINJA2CPP_PYTHON_ERRORS_H
