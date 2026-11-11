#ifndef JINJA2CPP_SRC_ENGINE_ERRORS_H
#define JINJA2CPP_SRC_ENGINE_ERRORS_H

#include <exception>
#include <stdexcept>
#include <string>
#include <utility>

namespace jinja2
{
inline namespace v2
{
// Errors the engine raises, which Load and Render report by their code with no exception
// attached (docs/api-2.0.md, 2.1); a standard exception of the same kind from host code keeps its own.

// A limit was reached: one of Settings::Limits, named by its field, or a built-in cap of the
// engine (a sequence, an arena, the templates of one render), with no field:
// ErrorCode::ResourceLimitExceeded, with the field's name as extraParams[1]
class ResourceLimitError : public std::length_error
{
public:
    explicit ResourceLimitError(const std::string& message)
        : std::length_error(message)
    {
    }
    ResourceLimitError(const char* field, const std::string& message)
        : std::length_error(message)
        , m_field(field)
    {
    }
    // The name of the Settings::Limits field, or null for a built-in cap
    [[nodiscard]] const char* Field() const noexcept { return m_field; }

private:
    const char* m_field = nullptr;
};

// Python's OverflowError for an integer: arithmetic whose result is outside int64 and has no
// big-integer backend to compute it, or a big integer where an index-sized one is needed
// (docs/tasks/0270): ErrorCode::IntegerOutOfRange
class IntegerOutOfRangeError : public std::overflow_error
{
public:
    using std::overflow_error::overflow_error;
};
// An operation of the template failed: Python's TypeError, ValueError and the like.
// ErrorCode::TemplateRuntimeError. A std::runtime_error, as the engine's errors were before it
// had codes of their own
class TemplateRuntimeError : public std::runtime_error
{
public:
    //! The Python exception class Jinja2 raises for the failure, when the engine knows it
    enum class PythonClass : unsigned char
    {
        Unknown,
        TypeError,
        ValueError
    };

    using std::runtime_error::runtime_error;
    TemplateRuntimeError(PythonClass pythonClass, const std::string& message)
        : std::runtime_error(message)
        , m_pythonClass(pythonClass)
    {
    }
    //! The name of the class (ExtraParams[1] of the error), or null when unknown
    [[nodiscard]] const char* PythonClassName() const noexcept
    {
        switch (m_pythonClass)
        {
        case PythonClass::TypeError:
            return "TypeError";
        case PythonClass::ValueError:
            return "ValueError";
        case PythonClass::Unknown:
            break;
        }
        return nullptr;
    }

private:
    PythonClass m_pythonClass = PythonClass::Unknown;
};

// Python's UndefinedError: ErrorCode::UndefinedError
class UndefinedError : public std::exception
{
public:
    explicit UndefinedError(std::string message, bool security = false)
        : m_message(std::move(message))
        , m_security(security)
    {
    }
    [[nodiscard]] const char* what() const noexcept override { return m_message.c_str(); }
    // Jinja2's SecurityError, which the sandbox's unsafe undefined raises on use (ErrorCode::SecurityError)
    [[nodiscard]] bool IsSecurity() const noexcept { return m_security; }

private:
    std::string m_message;
    bool m_security;
};

// Python's RecursionError: ErrorCode::RecursionLimitExceeded
class RecursionLimitError : public std::exception
{
public:
    [[nodiscard]] const char* what() const noexcept override { return "maximum recursion depth exceeded"; }
};

namespace detail
{
// What a host callable (a context or global callable, a filter or test the host adds,
// Settings::finalize, gettext) threw, the engine's own errors aside: ErrorCode::UserCallableException,
// with the exception kept. Not a std::exception, so that no catch of the engine's takes it for one
// of its own errors
class HostCallableFailure
{
public:
    explicit HostCallableFailure(std::exception_ptr exception) noexcept
        : m_exception(std::move(exception))
    {
    }
    [[nodiscard]] const std::exception_ptr& Exception() const noexcept { return m_exception; }

private:
    std::exception_ptr m_exception;
};

// A mix-in of a host's own exception type that no catch of the engine absorbs: Python's
// BaseException outside Exception (KeyboardInterrupt, SystemExit) through the module, which
// Jinja2's `except Exception` around the fold of a finalized output lets through (renderer.h)
class HostUnrecoverableError
{
public:
    virtual ~HostUnrecoverableError() = default;

protected:
    HostUnrecoverableError() = default;
    HostUnrecoverableError(const HostUnrecoverableError&) = default;
    HostUnrecoverableError(HostUnrecoverableError&&) noexcept = default;
    HostUnrecoverableError& operator=(const HostUnrecoverableError&) = default;
    HostUnrecoverableError& operator=(HostUnrecoverableError&&) noexcept = default;
};
} // namespace detail
} // namespace v2
} // namespace jinja2

#endif // JINJA2CPP_SRC_ENGINE_ERRORS_H
