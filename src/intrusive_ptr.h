#ifndef JINJA2CPP_SRC_INTRUSIVE_PTR_H
#define JINJA2CPP_SRC_INTRUSIVE_PTR_H

#include <boost/smart_ptr/intrusive_ptr.hpp>

namespace jinja2
{
inline namespace v2
{

// The one place that names the backing intrusive pointer, so that replacing it
// (docs/tasks/0230, step D4) touches this header and not every call site. An alias, not
// a wrapper: the pointee still provides the intrusive_ptr_add_ref/intrusive_ptr_release
// hooks, found by argument-dependent lookup.
template<typename T>
using IntrusivePtr = boost::intrusive_ptr<T>;

} // namespace v2
} // namespace jinja2

#endif // JINJA2CPP_SRC_INTRUSIVE_PTR_H
