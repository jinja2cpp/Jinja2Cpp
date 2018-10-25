#ifndef JINJA2_VALUE_H
#define JINJA2_VALUE_H

#pragma once

#include <vector>
#include <unordered_map>
#include <string>
#include <functional>
#include <nonstd/variant.hpp>
#include <nonstd/value_ptr.hpp>

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
struct FunctionCallParams;

using UserFunction = std::function<Value (const FunctionCallParams&)>;

template<typename T>
using RecWrapper = nonstd::value_ptr<T>;

class Value {
public:
    using ValueData = nonstd::variant<EmptyValue, bool, std::string, std::wstring, int64_t, double, RecWrapper<ValuesList>, RecWrapper<ValuesMap>, GenericList, GenericMap, UserFunction>;

    Value();
    Value(const Value& val);
    Value(Value&& val) noexcept;
    ~Value();
    
    Value& operator =(const Value&);
    Value& operator =(Value&&) noexcept;
    template<typename T>
    Value(T&& val, typename std::enable_if<!std::is_same<std::decay_t<T>, Value>::value && !std::is_same<std::decay_t<T>, ValuesList>::value>::type* = nullptr)
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
    Value(const ValuesList& list)
        : m_data(RecWrapper<ValuesList>(list))
    {
    }
    Value(const ValuesMap& map)
        : m_data(RecWrapper<ValuesMap>(map))
    {
    }
    Value(ValuesList&& list) noexcept
        : m_data(RecWrapper<ValuesList>(std::move(list)))
    {
    }
    Value(ValuesMap&& map) noexcept
        : m_data(RecWrapper<ValuesMap>(std::move(map)))
    {
    }

    const ValueData& data() const {return m_data;}

    ValueData& data() {return m_data;}

    bool isString() const
    {
        return nonstd::get_if<std::string>(&m_data) != nullptr;
    }
    auto& asString()
    {
        return nonstd::get<std::string>(m_data);
    }
    auto& asString() const
    {
        return nonstd::get<std::string>(m_data);
    }

    bool isList() const
    {
        return nonstd::get_if<RecWrapper<ValuesList>>(&m_data) != nullptr || nonstd::get_if<GenericList>(&m_data) != nullptr;
    }
    auto& asList()
    {
        return *nonstd::get<RecWrapper<ValuesList>>(m_data).get();
    }
    auto& asList() const
    {
        return *nonstd::get<RecWrapper<ValuesList>>(m_data).get();
    }
    bool isMap() const
    {
        return nonstd::get_if<RecWrapper<ValuesMap>>(&m_data) != nullptr || nonstd::get_if<GenericMap>(&m_data) != nullptr;
    }
    auto& asMap()
    {
        return *nonstd::get<RecWrapper<ValuesMap>>(m_data).get();
    }
    auto& asMap() const
    {
        return *nonstd::get<RecWrapper<ValuesMap>>(m_data).get();
    }
    bool isEmpty() const
    {
        return nonstd::get_if<EmptyValue>(&m_data) != nullptr;
    }

    Value subscript(const Value& index) const;

private:
    ValueData m_data;
};

struct FunctionCallParams
{
    ValuesMap kwParams;
    ValuesList posParams;
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

inline Value::Value() = default;
inline Value::Value(const Value& val) = default;
inline Value::Value(Value&& val) noexcept = default;
inline Value::~Value() = default;
inline Value& Value::operator =(const Value&) = default;
inline Value& Value::operator =(Value&&) noexcept = default;


} // jinja2

#endif // JINJA2_VALUE_H
