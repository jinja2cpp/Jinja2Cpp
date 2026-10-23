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

// A big integer (BigInt) reached a template, which cannot hold one yet (docs/tasks/0270):
// ErrorCode::IntegerOutOfRange
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
    using std::runtime_error::runtime_error;
};

// Python's UndefinedError: ErrorCode::UndefinedError
class UndefinedError : public std::exception
{
public:
    explicit UndefinedError(std::string message)
        : m_message(std::move(message))
    {
    }
    [[nodiscard]] const char* what() const noexcept override { return m_message.c_str(); }

private:
    std::string m_message;
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
} // namespace detail
} // namespace v2
} // namespace jinja2

#endif // JINJA2CPP_SRC_ENGINE_ERRORS_H
