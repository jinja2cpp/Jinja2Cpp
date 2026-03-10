#ifndef JINJA2CPP_VALUE_PTR_H
#define JINJA2CPP_VALUE_PTR_H

#include "jinja2cpp/polymorphic_value/polymorphic_cxx14.h"
#if __cplusplus == 201402L
#include "polymorphic_value/polymorphic_cxx14.h"
#else
#include "polymorphic_value/polymorphic.h"
#endif

namespace jinja2 {
namespace types {

using namespace xyz;

template<typename T>
using ValuePtr = polymorphic<T>;

template<class T, class... Ts>
auto MakeValuePtr(Ts&&... ts)
{
    return polymorphic<T>(xyz::in_place_type_t<T>{}, std::forward<Ts&&>(ts)...);
}

} // namespace types
} // namespace jinja2

#endif // JINJA2CPP_VALUE_PTR_H
