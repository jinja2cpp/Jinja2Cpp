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

class ReflectedMap
{
public:
    struct ItemAccessor
    {
        virtual ~ItemAccessor() {}
        virtual bool HasValue(const std::string& name) const = 0;
        virtual Value GetValue(const std::string& name) const = 0;
        virtual std::string ToString() const = 0;
    };

    ReflectedMap() = default;
    ReflectedMap(std::function<const ItemAccessor* ()> accessor)
        : m_accessor(std::move(accessor))
    {
    }

    bool HasValue(const std::string& name) const
    {
        return m_accessor()->HasValue(name);
    }

    Value GetValue(const std::string& name) const;

    std::string ToString() const
    {
        return m_accessor()->ToString();
    }

    std::function<const ItemAccessor* ()> m_accessor;
};

class ReflectedList
{
public:
    struct ItemAccessor
    {
        virtual ~ItemAccessor() {}
        virtual size_t GetSize() const = 0;
        virtual Value GetValue(size_t idx) const = 0;
    };

    ReflectedList() = default;
    ReflectedList(std::function<const ItemAccessor* ()> accessor)
        : m_accessor(std::move(accessor))
    {
    }

    bool GetSize() const
    {
        return m_accessor()->GetSize();
    }

    Value GetValue(int idx) const;

    std::function<const ItemAccessor* ()> m_accessor;
};

using ValuesList = std::vector<Value>;
using ValuesMap = std::unordered_map<std::string, Value>;
class ReflectedMap;
using ValueData = boost::variant<EmptyValue, bool, std::string, std::wstring, int64_t, double, ValuesList, ValuesMap, ReflectedMap>;

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
        return boost::get<ValuesList>(&m_data) != nullptr;
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
        return boost::get<ValuesMap>(&m_data) != nullptr || boost::get<ReflectedMap>(&m_data) != nullptr;
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

inline Value ReflectedMap::GetValue(const std::string& name) const
{
    return m_accessor()->GetValue(name);
}


template<typename Derived>
class ReflectedMapImplBase : public ReflectedMap::ItemAccessor
{
public:
    bool HasValue(const std::string& name) const override
    {
        return Derived::GetAccessors().count(name) != 0;
    }
    Value GetValue(const std::string& name) const override
    {
        auto& accessors = Derived::GetAccessors();
        auto p = accessors.find(name);
        if (p == accessors.end())
            throw std::runtime_error("Invalid field access");

        return static_cast<const Derived*>(this)->GetField(p->second);
    }
};

template<typename T>
class ReflectedMapImpl : public ReflectedMapImplBase<ReflectedMapImpl<T>>
{
public:
    ReflectedMapImpl(T val) : m_value(val) {}
    using FieldAccessor = std::function<Value (const T& value)>;

    static auto& GetAccessors();
    static std::string ToString(const T& value);
    template<typename Fn>
    Value GetField(Fn&& accessor) const
    {
        return accessor(m_value);
    }

    std::string ToString() const override
    {
        return ToString(m_value);
    }
private:
    T m_value;
};

template<typename T>
Value AsReflectedMap(T&& val)
{
    return Value(ReflectedMap([accessor = ReflectedMapImpl<T>(std::forward<T>(val))]() -> const ReflectedMap::ItemAccessor* {return &accessor;}));
}

} // jinja2

#endif // JINJA2_VALUE_H
