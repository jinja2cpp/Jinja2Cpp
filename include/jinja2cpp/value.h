#ifndef JINJA2_VALUE_H
#define JINJA2_VALUE_H

#pragma once

#include <vector>
#include <unordered_map>
#include <string>
#include <functional>
#include <type_traits>
#include <nonstd/variant.hpp>
#include <nonstd/optional.hpp>
#include <nonstd/value_ptr.hpp>

namespace jinja2
{
struct EmptyValue
{
    template<typename T>
    operator T() const {return T{};}
};
class Value;

struct ListItemAccessor
{
    virtual ~ListItemAccessor() {}

    virtual size_t GetSize() const = 0;
    virtual Value GetValueByIndex(int64_t idx) const = 0;
};

struct MapItemAccessor
{
    virtual ~MapItemAccessor() {}
    virtual size_t GetSize() const = 0;
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
        return m_accessor ? m_accessor()->HasValue(name) : false;
    }

    Value GetValueByName(const std::string& name) const;
    size_t GetSize() const
    {
        return m_accessor ? m_accessor()->GetSize() : 0;
    }
    auto GetKeys() const
    {
        return m_accessor ? m_accessor()->GetKeys() : std::vector<std::string>();
    }
    auto GetAccessor() const
    {
        return m_accessor();
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
        return m_accessor ? m_accessor()->GetSize() : 0ULL;
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
struct UserCallableArgs;
struct ParamInfo;
struct UserCallable;

template<typename T>
using RecWrapper = nonstd::value_ptr<T>;

class Value
{
public:
    using ValueData = nonstd::variant<EmptyValue, bool, std::string, std::wstring, int64_t, double, RecWrapper<ValuesList>, RecWrapper<ValuesMap>, GenericList, GenericMap, RecWrapper<UserCallable>>;
    template<typename T, typename ... L>
    struct AnyOf : public std::false_type {};

    template<typename T, typename H, typename ... L>
    struct AnyOf<T, H, L...> : public std::integral_constant<bool, std::is_same<std::decay_t<T>, H>::value || AnyOf<T, L...>::value> {};

    Value();
    Value(const Value& val);
    Value(Value&& val);
    ~Value();

    Value& operator =(const Value&);
    Value& operator =(Value&&);
    template<typename T>
    Value(T&& val, typename std::enable_if<!AnyOf<T, Value, ValuesList, UserCallable>::value>::type* = nullptr)
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
    Value(const UserCallable& callable);
    Value(ValuesList&& list) noexcept
        : m_data(RecWrapper<ValuesList>(std::move(list)))
    {
    }
    Value(ValuesMap&& map) noexcept
        : m_data(RecWrapper<ValuesMap>(std::move(map)))
    {
    }
    Value(UserCallable&& callable);

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

struct UserCallableParams
{
    ValuesMap args;
    ValuesList extraPosArgs;
    ValuesMap extraKwArgs;
    bool paramsParsed = false;

    Value operator[](const std::string& paramName) const
    {
        auto p = args.find(paramName);
        if (p == args.end())
            return Value();

        return p->second;
    }
};

struct ArgInfo
{
   std::string paramName;
   bool isMandatory;
   Value defValue;

   ArgInfo(std::string name, bool isMandat = false, Value defVal = Value())
       : paramName(std::move(name))
       , isMandatory(isMandat)
       , defValue(std::move(defVal))
   {}
};

struct UserCallable
{
    std::function<Value (const UserCallableParams&)> callable;
    std::vector<ArgInfo> argsInfo;
};

inline Value::Value(const UserCallable& callable)
    : m_data(RecWrapper<UserCallable>(callable))
{
}

inline Value::Value(UserCallable&& callable)
    : m_data(RecWrapper<UserCallable>(std::move(callable)))
{
}

inline Value GenericMap::GetValueByName(const std::string& name) const
{
    return m_accessor ? m_accessor()->GetValueByName(name) : Value();
}

inline Value GenericList::GetValueByIndex(int64_t index) const
{
    return m_accessor ? m_accessor()->GetValueByIndex(index) : Value();
}

inline Value::Value() = default;
inline Value::Value(const Value& val) = default;
inline Value::Value(Value&& val) = default;
inline Value::~Value() = default;
inline Value& Value::operator =(const Value&) = default;
inline Value& Value::operator =(Value&&) = default;


} // jinja2

#endif // JINJA2_VALUE_H
