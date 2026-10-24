#ifndef JINJA2CPP_SRC_SMALL_VECTOR_H
#define JINJA2CPP_SRC_SMALL_VECTOR_H

#include <boost/container/small_vector.hpp>

#include <cstddef>

namespace jinja2
{
inline namespace v2
{

// The one place that names the backing small-buffer vector, so that replacing it
// (docs/tasks/0230, step D5, behind the bench gate) touches this header and not every
// call site. Aliases, not wrappers: the generated code is that of boost::container.

// Growth policy that doubles the capacity instead of the default 1.6x.
using SmallVectorDoubling =
    boost::container::small_vector_options_t<boost::container::growth_factor<boost::container::growth_factor_100>>;

template<typename T, std::size_t N, typename Options = void>
using SmallVector = boost::container::small_vector<T, N, void, Options>;

// Capacity-erased base of every SmallVector<T, N>, for functions that take any of them.
template<typename T>
using SmallVectorBase = boost::container::small_vector_base<T>;

} // namespace v2
} // namespace jinja2

#endif // JINJA2CPP_SRC_SMALL_VECTOR_H
