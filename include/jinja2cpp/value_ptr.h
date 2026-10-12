#ifndef JINJA2CPP_VALUE_PTR_H
#define JINJA2CPP_VALUE_PTR_H

#include "jinja2cpp/polymorphic_value/polymorphic_cxx14.h"

namespace jinja2::types
{

// Only the names the library's headers use, not the whole vendored namespace
using xyz::in_place_type_t;
using xyz::polymorphic;

template<typename T>
using ValuePtr = polymorphic<T>;

template<class T, class... Ts>
auto MakeValuePtr(Ts&&... ts)
{
    return polymorphic<T>(xyz::in_place_type_t<T>{}, std::forward<Ts&&>(ts)...);
}

} // namespace jinja2::types

#endif // JINJA2CPP_VALUE_PTR_H
