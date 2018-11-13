#ifndef GENERIC_ADAPTERS_H
#define GENERIC_ADAPTERS_H

#include <jinja2cpp/value.h>
#include "internal_value.h"

namespace jinja2
{
template<typename T>
class ListItemAccessorImpl : public ListItemAccessor
{
public:
    Value GetValueByIndex(int64_t idx) const
    {
        return IntValue2Value(static_cast<const T*>(this)->GetItem(idx));
    }
};

template<typename T>
class ListAccessorImpl : public IListAccessor, public ListItemAccessorImpl<T>
{
public:
//    GenericList CreateGenericList() const override
//    {
//        return GenericList([accessor = this]() -> const ListItemAccessor* {return accessor;});
//    }
};

template<typename T>
class MapItemAccessorImpl : public MapItemAccessor
{
public:
    Value GetValueByName(const std::string& name) const
    {
        return IntValue2Value(static_cast<const T*>(this)->GetItem(name));
    }
};

template<typename T>
class MapAccessorImpl : public IMapAccessor, public MapItemAccessorImpl<T>
{
public:
};

} // jinja2

#endif // GENERIC_ADAPTERS_H
