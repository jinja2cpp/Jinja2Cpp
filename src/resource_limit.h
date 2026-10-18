#ifndef JINJA2CPP_SRC_RESOURCE_LIMIT_H
#define JINJA2CPP_SRC_RESOURCE_LIMIT_H

#include <stdexcept>

namespace jinja2
{
// A limit of the engine was reached (a sequence, an arena, the templates of one render).
// Load and Render report it as ErrorCode::ResourceLimitExceeded with no exception attached,
// since the engine raised it (docs/api-2.0.md, 2.1); a std::length_error from host code keeps its own.
class ResourceLimitError : public std::length_error
{
public:
    using std::length_error::length_error;
};
} // namespace jinja2

#endif // JINJA2CPP_SRC_RESOURCE_LIMIT_H
