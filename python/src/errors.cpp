#include "errors.h"

#include "error_boundary.h"

#include "engine_errors.h"

#include <jinja2cpp/error_info.h>
#include <jinja2cpp/value.h>

#include <exception>
#include <string>

namespace jinja2cpp_py
{
using jinja2::ErrorCode;

namespace
{
[[noreturn]] void Raise(nb::handle type, nb::handle instance)
{
    PyErr_SetObject(type.ptr(), instance.ptr());
    throw nb::python_error();
}

std::string ParamText(const jinja2::ErrorInfo& error, size_t index)
{
    const auto& params = error.GetExtraParams();
    if (index >= params.size())
    {
        return std::string();
    }
    if (const auto* text = params[index].GetIf<std::string>())
    {
        return *text;
    }
    return std::string();
}

bool IsSyntaxError(ErrorCode code)
{
    return static_cast<int>(code) >= 1001 || code == ErrorCode::Unspecified || code == ErrorCode::ExtensionDisabled || code == ErrorCode::MetadataParseError || code == ErrorCode::YetUnsupported;
}
} // namespace

std::string ErrorMessage(const jinja2::ErrorInfo& error)
{
    switch (error.GetCode())
    {
    case ErrorCode::UndefinedError:
    case ErrorCode::TemplateRuntimeError:
    case ErrorCode::IntegerOutOfRange:
    case ErrorCode::ResourceLimitExceeded:
    case ErrorCode::UserCallableException:
    case ErrorCode::UnexpectedException:
    case ErrorCode::DataReadError:
    case ErrorCode::TemplateNotFound:
    case ErrorCode::FileNotFound:
        if (auto text = ParamText(error, 0); !text.empty())
        {
            return text;
        }
        break;
    case ErrorCode::UnknownFilter:
        return "No filter named '" + ParamText(error, 0) + "'.";
    case ErrorCode::UnknownTest:
        return "No test named '" + ParamText(error, 0) + "'.";
    default:
        break;
    }
    // The engine's text: "<file>:<line>:<col>: error: <message>\n<location>"
    auto text = error.ToString();
    if (const auto start = text.find(": error: "); start != std::string::npos)
    {
        text.erase(0, start + 9);
    }
    if (const auto end = text.find('\n'); end != std::string::npos)
    {
        text.erase(end);
    }
    return text;
}

void RaiseError(const jinja2::ErrorInfo& error, const ErrorSource& source)
{
    // Only an exception the engine did not raise is kept: Python's own, raised again as itself
    if (const auto& exception = error.GetException())
    {
        std::rethrow_exception(exception);
    }
    const auto& names = Names();
    const auto code = error.GetCode();
    const auto message = MakeStr(ErrorMessage(error));
    if (IsSyntaxError(code) || code == ErrorCode::UnknownFilter || code == ErrorCode::UnknownTest)
    {
        const auto& type = code == ErrorCode::UnknownFilter || code == ErrorCode::UnknownTest ? names.templateAssertionError : names.templateSyntaxError;
        const auto line = static_cast<int>(error.GetErrorLocation().line);
        auto instance = type(message, line == 0 ? 1 : line, source.name, source.filename);
        if (!source.source.is_none())
        {
            nb::setattr(instance, "source", source.source);
        }
        if (source.syntaxNote)
        {
            instance.attr("add_note")(source.syntaxNote);
        }
        Raise(type, instance);
    }
    switch (code)
    {
    case ErrorCode::TemplateNotFound:
    case ErrorCode::FileNotFound:
        Raise(names.templateNotFound, names.templateNotFound(message));
    case ErrorCode::UndefinedError:
        // The use of an attribute the sandbox refused (jinja2::SecurityErrorMarker)
        if (ParamText(error, 1) == jinja2::SecurityErrorMarker)
        {
            Raise(names.securityError, names.securityError(message));
        }
        Raise(names.undefinedError, names.undefinedError(message));
    case ErrorCode::ResourceLimitExceeded:
        // The sandbox's range(): safe_range's OverflowError
        if (ParamText(error, 1) == "maxRange")
        {
            const auto limit = Utf8(nb::steal(PyObject_Str(nb::module_::import_("jinja2cpp.sandbox").attr("MAX_RANGE").ptr())));
            PyErr_SetObject(PyExc_OverflowError, MakeStr("Range too big. The sandbox blocks ranges larger than MAX_RANGE (" + limit + ").").ptr());
            throw nb::python_error();
        }
        Raise(names.templateRuntimeError, names.templateRuntimeError(message));
    case ErrorCode::IntegerOutOfRange:
        PyErr_SetObject(PyExc_OverflowError, message.ptr());
        throw nb::python_error();
    case ErrorCode::RecursionLimitExceeded:
        PyErr_SetString(PyExc_RecursionError, "maximum recursion depth exceeded");
        throw nb::python_error();
    default:
        Raise(names.templateRuntimeError, names.templateRuntimeError(message));
    }
}

void RaiseCurrentEngineError()
{
    RaiseError(jinja2::detail::CurrentExceptionToError<char>(std::string()));
}

void RaiseTemplateRuntimeError(const std::string& message)
{
    const auto& type = Names().templateRuntimeError;
    Raise(type, type(MakeStr(message)));
}

void RaiseTypeError(const std::string& message)
{
    PyErr_SetString(PyExc_TypeError, message.c_str());
    throw nb::python_error();
}

void RaiseTemplateNotFound(nb::handle name)
{
    const auto& type = Names().templateNotFound;
    Raise(type, type(name));
}
} // namespace jinja2cpp_py
