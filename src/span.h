#ifndef JINJA2CPP_SRC_SPAN_H
#define JINJA2CPP_SRC_SPAN_H

#include <boost/core/span.hpp>

#include <cstddef>

namespace jinja2
{
inline namespace v2
{

// The one place that names the backing span type, so that replacing it (docs/tasks/0230,
// step D4) touches this header and not every call site. An alias, not a wrapper: the
// generated code is that of boost::span.
template<typename T, std::size_t extent = boost::dynamic_extent>
using Span = boost::span<T, extent>;

} // namespace v2
} // namespace jinja2

#endif // JINJA2CPP_SRC_SPAN_H
