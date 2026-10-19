#ifndef JINJA2CPP_VALUE_PTR_H
#define JINJA2CPP_VALUE_PTR_H

#include "jinja2cpp/polymorphic_value/indirect_cxx14.h"
#include "jinja2cpp/polymorphic_value/polymorphic_cxx14.h"

namespace jinja2
{
inline namespace v2
{
namespace types
{

// Only the names the library's headers use, not the whole vendored namespace
using detail::xyz::in_place_type_t;
using detail::xyz::polymorphic;

template<typename T>
using ValuePtr = polymorphic<T>;

template<class T, class... Ts>
auto MakeValuePtr(Ts&&... ts)
{
    return polymorphic<T>(in_place_type_t<T>{}, std::forward<Ts&&>(ts)...);
}

} // namespace types
} // namespace v2
} // namespace jinja2

namespace jinja2
{
inline namespace v2
{
namespace detail
{
// Owns one heap-allocated T and copies it deeply, as C++26 std::indirect does; T may be
// incomplete where the wrapper is declared. Holds the recursive alternatives of Value.
template<typename T>
using indirect = xyz::indirect<T>;
// The tag that selects indirect's in-place constructor
using in_place_t = xyz::in_place_t;
} // namespace detail
} // namespace v2
} // namespace jinja2

#endif // JINJA2CPP_VALUE_PTR_H
