#ifndef JINJA2CPP_SRC_MAKE_UNEXPECTED_H
#define JINJA2CPP_SRC_MAKE_UNEXPECTED_H

#include <nonstd/expected.hpp>

#include <utility>

namespace jinja2
{

// The one place that names the backing `expected` library's error factory, so that
// switching it (docs/api-2.0.md, decision 4) touches this function and not every
// call site. Inlined away; no cost over calling nonstd::make_unexpected directly.
template<typename E>
auto MakeUnexpected(E&& error)
{
    return nonstd::make_unexpected(std::forward<E>(error));
}

} // namespace jinja2

#endif // JINJA2CPP_SRC_MAKE_UNEXPECTED_H
