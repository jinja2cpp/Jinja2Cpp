#include "errors.h"

#include "convert.h"

#include <jinja2cpp/error_info.h>
#include <jinja2cpp/exception.h>
#include <jinja2cpp/value.h>

#include <exception>
#include <new>
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

// A frame of the template at `location` in front of the traceback of `exception` (jinja2cpp._debug), as Jinja2's
// tracebacks show the template's line: `function` is Jinja2's name for the frame. Never fails: a frame that
// cannot be made is left out
void AddTemplateFrame(nb::handle exception, const jinja2::SourceLocation& location, nb::handle filename, const char* function)
{
    if (location.line == 0 || !exception)
    {
        return;
    }
    try
    {
        // A template without a file is "<template>", as in Jinja2: its name is no path linecache should read
        nb::module_::import_("jinja2cpp._debug").attr("add_template_frame")(exception, filename, location.line, function);
    }
    catch (const nb::python_error&) // NOLINT(bugprone-empty-catch): the frame is a diagnostic only
    {
    }
}

// Whether the error's template `name` is the one `source` describes: the engine names an unnamed template
// noname.j2tpl, which no loader gives
bool IsOwnTemplate(const std::string& name, const ErrorSource& source)
{
    if (source.name.is_none())
    {
        return name.empty() || name == "noname.j2tpl";
    }
    return name.empty() || Utf8(source.name) == name;
}

// The file of the template the error is in: the source's, when the error is in that template
nb::object FileOfError(const jinja2::ErrorInfo& error, const ErrorSource& source)
{
    const auto& name = error.GetErrorLocation().fileName;
    if (IsOwnTemplate(name, source))
    {
        return source.filename;
    }
    if (!source.environment.is_none())
    {
        try
        {
            return SourceOfTemplate(source.environment, name, false).filename;
        }
        catch (const nb::python_error&) // NOLINT(bugprone-empty-catch): a diagnostic only
        {
        }
    }
    return nb::none();
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
    case ErrorCode::SecurityError:
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

const char* const kIncludedSyntaxNote =
            "the error is in a template this one includes, imports or extends, which may use syntax the engine lacks. "
            "Such templates fall back to Jinja2 only when loaded directly (task 0430). To run the whole environment on "
            "Jinja2, give it a Tier B option, such as extensions=[an extension] or enable_async=True (docs/python.md, \"Fallback\")";

void RaiseError(const jinja2::ErrorInfo& error, const ErrorSource& source)
{
    // Only an exception the engine did not raise is kept: Python's own, raised again as itself
    const auto& names = Names();
    if (const auto& exception = error.GetException())
    {
        try
        {
            std::rethrow_exception(exception);
        }
        catch (const nb::python_error& raised)
        {
            // A callable's exception, with the template's line between the caller's frames and the callable's
            AddTemplateFrame(raised.value(), error.GetErrorLocation(), FileOfError(error, source), "top-level template code");
            if (source.render && raised.matches(PyExc_StopIteration))
            {
                const auto stop = nb::borrow(raised.value());
                const auto runtime = nb::steal(PyObject_CallFunction(PyExc_RuntimeError, "s", "generator raised StopIteration"));
                if (!runtime)
                {
                    throw nb::python_error();
                }
                PyException_SetCause(runtime.ptr(), stop.inc_ref().ptr());   // steals the reference
                PyException_SetContext(runtime.ptr(), stop.inc_ref().ptr()); // steals the reference
                Raise(runtime.type(), runtime);
            }
            throw;
        }
        catch (const jinja2::Exception& engine)
        {
            // A callable of the engine's own (ConfigureForChatTemplates' raise_exception) raised it,
            // reported by its code as the engine reports its own errors
            const auto text = MakeStr(engine.what());
            if (engine.Code() == ErrorCode::UndefinedError)
            {
                Raise(names.undefinedError, names.undefinedError(text));
            }
            Raise(names.templateRuntimeError, names.templateRuntimeError(text));
        }
    }
    const auto code = error.GetCode();
    const auto message = MakeStr(ErrorMessage(error));
    if (IsSyntaxError(code) || code == ErrorCode::UnknownFilter || code == ErrorCode::UnknownTest)
    {
        const bool assertion = code == ErrorCode::UnknownFilter || code == ErrorCode::UnknownTest || code == ErrorCode::TemplateAssertionError;
        const auto& type = assertion ? names.templateAssertionError : names.templateSyntaxError;
        const auto& location = error.GetErrorLocation();
        // An error in another template than the one loaded or rendered (one it includes) is reported with that
        // template's name, file and source, as Jinja2 reports it
        const ErrorSource* where = &source;
        ErrorSource other;
        if (!IsOwnTemplate(location.fileName, source) && !source.environment.is_none())
        {
            other = SourceOfTemplate(source.environment, location.fileName);
            other.syntaxNote = source.syntaxNote;
            where = &other;
        }
        const auto line = static_cast<int>(location.line);
        auto instance = type(message, line == 0 ? 1 : line, where->name, where->filename);
        if (!where->source.is_none())
        {
            nb::setattr(instance, "source", where->source);
        }
        if (where->syntaxNote)
        {
            instance.attr("add_note")(where->syntaxNote);
        }
        jinja2::SourceLocation frame = location;
        frame.line = line == 0 ? 1U : location.line;
        AddTemplateFrame(instance, frame, where->filename, "template");
        Raise(type, instance);
    }
    switch (code)
    {
    case ErrorCode::TemplateNotFound:
    case ErrorCode::FileNotFound:
        if (!error.GetExtraParams().empty())
        {
            RaiseTemplateNotFound(source.environment, ToPython(error.GetExtraParams()[0]));
        }
        Raise(names.templateNotFound, names.templateNotFound(message));
    case ErrorCode::UndefinedError:
        Raise(names.undefinedError, names.undefinedError(message));
    case ErrorCode::SecurityError:
        // The use of an attribute the sandbox refused, or of a list or dict method an immutable sandbox refuses
        Raise(names.securityError, names.securityError(message));
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
    case ErrorCode::TemplateRuntimeError:
        // Python's class of the error, when the engine names it (ExtraParams[1])
        if (const auto kind = ParamText(error, 1); kind == "TypeError" || kind == "ValueError")
        {
            PyErr_SetObject(kind == "TypeError" ? PyExc_TypeError : PyExc_ValueError, message.ptr());
            throw nb::python_error();
        }
        Raise(names.templateRuntimeError, names.templateRuntimeError(message));
    default:
        Raise(names.templateRuntimeError, names.templateRuntimeError(message));
    }
}

void RaiseCurrentEngineError()
{
    // What a callable of the template (a lent macro, `caller`) reports: the engine's ErrorInfo, or a
    // jinja2::Exception of a callable of its own
    try
    {
        throw;
    }
    catch (const nb::python_error&)
    {
        throw;
    }
    catch (const jinja2::ErrorInfo& error)
    {
        RaiseError(error);
    }
    catch (const jinja2::Exception& error)
    {
        const auto& names = Names();
        const auto text = MakeStr(error.what());
        if (error.Code() == ErrorCode::UndefinedError)
        {
            Raise(names.undefinedError, names.undefinedError(text));
        }
        Raise(names.templateRuntimeError, names.templateRuntimeError(text));
    }
    catch (const std::bad_alloc&)
    {
        PyErr_NoMemory();
        throw nb::python_error();
    }
    catch (const std::exception& error)
    {
        RaiseTemplateRuntimeError(error.what());
    }
    catch (...)
    {
        RaiseTemplateRuntimeError("unknown error of the engine");
    }
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

void RaiseTemplateNotFound(nb::handle environment, nb::handle names)
{
    auto instance = nb::module_::import_("jinja2cpp.loaders").attr("_not_found")(environment, names);
    Raise(reinterpret_cast<PyObject*>(Py_TYPE(instance.ptr())), instance);
}
} // namespace jinja2cpp_py
