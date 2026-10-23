#ifndef JINJA2CPP_SRC_ENGINE_ERRORS_H
#define JINJA2CPP_SRC_ENGINE_ERRORS_H

#include <stdexcept>
#include <string>

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
} // namespace v2
} // namespace jinja2

#endif // JINJA2CPP_SRC_ENGINE_ERRORS_H
