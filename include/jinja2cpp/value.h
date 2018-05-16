#ifndef JINJA2_VALUE_H
#define JINJA2_VALUE_H

#pragma once

#include <vector>
#include <unordered_map>
#include <string>
#include <functional>
#include <boost/variant.hpp>

namespace jinja2
{
struct EmptyValue {};
class Value;

struct ListItemAccessor
{
    virtual ~ListItemAccessor() {}

    virtual size_t GetSize() const = 0;
    virtual Value GetValueByIndex(int64_t idx) const = 0;
};

struct MapItemAccessor : public ListItemAccessor
{
    virtual ~MapItemAccessor() {}
    virtual bool HasValue(const std::string& name) const = 0;
    virtual Value GetValueByName(const std::string& name) const = 0;
    virtual std::vector<std::string> GetKeys() const = 0;
};

class GenericMap
{
public:
    GenericMap() = default;
    GenericMap(std::function<const MapItemAccessor* ()> accessor)
        : m_accessor(std::move(accessor))
    {
    }

    bool HasValue(const std::string& name) const
    {
        return m_accessor()->HasValue(name);
    }

    Value GetValueByName(const std::string& name) const;
    size_t GetSize() const
    {
        return m_accessor()->GetSize();
    }
    Value GetValueByIndex(int64_t index) const;
    auto GetKeys() const
    {
        return m_accessor()->GetKeys();
    }

    std::function<const MapItemAccessor* ()> m_accessor;
};

class GenericList
{
public:
    GenericList() = default;
    GenericList(std::function<const ListItemAccessor* ()> accessor)
        : m_accessor(std::move(accessor))
    {
    }

    size_t GetSize() const
    {
        return m_accessor()->GetSize();
    }

    Value GetValueByIndex(int64_t idx) const;

    auto GetAccessor() const
    {
        return m_accessor();
    }

    bool IsValid() const
    {
        return !(!m_accessor);
    }

    std::function<const ListItemAccessor* ()> m_accessor;
};

using ValuesList = std::vector<Value>;
using ValuesMap = std::unordered_map<std::string, Value>;
using ValueData = boost::variant<EmptyValue, bool, std::string, std::wstring, int64_t, double, boost::recursive_wrapper<ValuesList>, boost::recursive_wrapper<ValuesMap>, GenericList, GenericMap>;

class Value {
public:
    Value() = default;
    template<typename T>
    Value(T&& val, typename std::enable_if<!std::is_same<std::decay_t<T>, Value>::value>::type* = nullptr)
        : m_data(std::forward<T>(val))
    {
    }
    Value(const char* val)
        : m_data(std::string(val))
    {
    }
    template<size_t N>
    Value(char (&val)[N])
        : m_data(std::string(val))
    {
    }
    Value(int val)
        : m_data(static_cast<int64_t>(val))
    {
    }

    const ValueData& data() const {return m_data;}

    ValueData& data() {return m_data;}

    bool isString() const
    {
        return boost::get<std::string>(&m_data) != nullptr;
    }
    auto& asString()
    {
        return boost::get<std::string>(m_data);
    }
    auto& asString() const
    {
        return boost::get<std::string>(m_data);
    }

    bool isList() const
    {
        return boost::get<ValuesList>(&m_data) != nullptr || boost::get<GenericList>(&m_data) != nullptr;
    }
    auto& asList()
    {
        return boost::get<ValuesList>(m_data);
    }
    auto& asList() const
    {
        return boost::get<ValuesList>(m_data);
    }
    bool isMap() const
    {
        return boost::get<ValuesMap>(&m_data) != nullptr || boost::get<GenericMap>(&m_data) != nullptr;
    }
    auto& asMap()
    {
        return boost::get<ValuesMap>(m_data);
    }
    auto& asMap() const
    {
        return boost::get<ValuesMap>(m_data);
    }
    bool isEmpty() const
    {
        return boost::get<EmptyValue>(&m_data) != nullptr;
    }

    Value subscript(const Value& index) const;

private:
    ValueData m_data;
};

inline Value GenericMap::GetValueByName(const std::string& name) const
{
    return m_accessor()->GetValueByName(name);
}

inline Value GenericMap::GetValueByIndex(int64_t index) const
{
    return m_accessor()->GetValueByIndex(index);
}

inline Value GenericList::GetValueByIndex(int64_t index) const
{
    return m_accessor()->GetValueByIndex(index);
}

} // jinja2

#endif // JINJA2_VALUE_H
