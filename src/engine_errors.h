#ifndef JINJA2CPP_SRC_ENGINE_ERRORS_H
#define JINJA2CPP_SRC_ENGINE_ERRORS_H

#include <stdexcept>

namespace jinja2
{
// Errors the engine raises, which Load and Render report by their code with no exception
// attached (docs/api-2.0.md, 2.1); a standard exception of the same kind from host code keeps its own.

// A limit of the engine was reached (a sequence, an arena, the templates of one render):
// ErrorCode::ResourceLimitExceeded
class ResourceLimitError : public std::length_error
{
public:
    using std::length_error::length_error;
};

// A big integer (BigInt) reached a template, which cannot hold one yet (docs/tasks/0270):
// ErrorCode::IntegerOutOfRange
class IntegerOutOfRangeError : public std::overflow_error
{
public:
    using std::overflow_error::overflow_error;
};
} // namespace jinja2

#endif // JINJA2CPP_SRC_ENGINE_ERRORS_H
