#ifndef JINJA2_REFLECTED_VALUE_H
#define JINJA2_REFLECTED_VALUE_H

#include "value.h"

namespace jinja2
{

template<typename T>
Value Reflect(T&& val);

template<typename T, bool val>
struct TypeReflectedImpl : std::integral_constant<bool, val>
{
};

template<typename T>
struct TypeReflected : TypeReflectedImpl<T, true>
{
    using FieldAccessor = std::function<Value (const T& value)>;
};



template<typename T>
struct TypeReflection : TypeReflectedImpl<T, false>
{
};

template<typename Derived>
class ReflectedMapImplBase : public MapItemAccessor
{
public:
    bool HasValue(const std::string& name) const override
    {
        return Derived::GetAccessors().count(name) != 0;
    }
    Value GetValueByName(const std::string& name) const override
    {
        const auto& accessors = Derived::GetAccessors();
        auto p = accessors.find(name);
        if (p == accessors.end())
            throw std::runtime_error("Invalid field access");

        return static_cast<const Derived*>(this)->GetField(p->second);
    }
    std::vector<std::string> GetKeys() const override
    {
        std::vector<std::string> result;
        const auto& accessors = Derived::GetAccessors();
        for (auto& i : accessors)
            result.push_back(i.first);

        return result;
    }
    size_t GetSize() const override
    {
        return Derived::GetAccessors().size();
    }
    virtual Value GetValueByIndex(int64_t idx) const override
    {
        const auto& accessors = Derived::GetAccessors();
        auto p = accessors.begin();
        std::advance(p, idx);

        ValuesMap result;
        result["key"] = p->first;
        result["value"] = static_cast<const Derived*>(this)->GetField(p->second);

        return result;
    }
};

template<typename T>
class ReflectedMapImpl : public ReflectedMapImplBase<ReflectedMapImpl<T>>
{
public:
    ReflectedMapImpl(T val) : m_value(val) {}
    ReflectedMapImpl(const T* val) : m_valuePtr(val) {}

    static auto GetAccessors() {return TypeReflection<T>::GetAccessors();}
    template<typename Fn>
    Value GetField(Fn&& accessor) const
    {
        return accessor(m_valuePtr ? *m_valuePtr : m_value);
    }

private:
    T m_value;
    const T* m_valuePtr = nullptr;
};

namespace detail
{
template<typename T, typename Tag = void>
struct Reflector;

template<typename T>
using IsReflectedType = std::enable_if_t<TypeReflection<T>::value>;

// using IsReflectedType = std::enable_if_t<std::is_same<decltype(ReflectedMapImpl<T>::GetAccessors())::key_type, std::string>::value>;
// using IsReflectedType = typename Type2Void<typename Type2TypeT<decltype(TypeReflection<T>::GetAccessors())>::key_type>::type;

struct ContainerReflector
{
    template<typename T>
    struct ValueItemAccessor : ListItemAccessor
    {
        T m_value;

        explicit ValueItemAccessor(T&& cont) noexcept
            : m_value(std::move(cont))
        {
        }

        size_t GetSize() const override
        {
            return m_value.size();
        }
        Value GetValueByIndex(int64_t idx) const override
        {
            auto p = m_value.begin();
            std::advance(p, static_cast<size_t>(idx));
            return Reflect(*p);
        }
    };

    template<typename T>
    struct PtrItemAccessor : ListItemAccessor
    {
        const T* m_value;

        PtrItemAccessor(const T* ptr)
            : m_value(ptr)
        {
        }
        size_t GetSize() const override
        {
            return m_value->size();
        }
        Value GetValueByIndex(int64_t idx) const override
        {
            auto p = m_value->begin();
            std::advance(p, idx);
            return Reflect(*p);
        }
    };

    template<typename T>
    static Value CreateFromValue(T&& cont)
    {
        return GenericList([accessor = ValueItemAccessor<T>(std::move(cont))]() {return &accessor;});
    }

    template<typename T>
    static Value CreateFromPtr(const T* cont)
    {
        return GenericList([accessor = PtrItemAccessor<T>(cont)]() {return &accessor;});
    }

    template<typename T>
    static Value CreateFromPtr(std::shared_ptr<T> cont)
    {
        return GenericList([ptr = std::move(cont), accessor = PtrItemAccessor<T>(cont.get())]() {return &accessor;});
    }
};

template<typename T>
struct Reflector<std::set<T>>
{
    static auto Create(std::set<T> val)
    {
        return ContainerReflector::CreateFromValue(std::move(val));
    }
    static auto CreateFromPtr(const std::set<T>* val)
    {
        return ContainerReflector::CreateFromPtr(val);
    }
};

template<typename T>
struct Reflector<std::vector<T>>
{
    static auto Create(std::vector<T> val)
    {
        return ContainerReflector::CreateFromValue(std::move(val));
    }
    template<typename U>
    static auto CreateFromPtr(U&& val)
    {
        return ContainerReflector::CreateFromPtr(std::forward<U>(val));
    }
};

template<typename T>
struct Reflector<T, IsReflectedType<T>>
{
    static auto Create(const T& val)
    {
        return GenericMap([accessor = ReflectedMapImpl<T>(val)]() {return &accessor;});
    }

    static auto CreateFromPtr(const T* val)
    {
        return GenericMap([accessor = ReflectedMapImpl<T>(static_cast<const T*>(val))]() {return &accessor;});
    }

    static auto CreateFromPtr(std::shared_ptr<T> val)
    {
        return GenericMap([ptr = val, accessor = ReflectedMapImpl<T>(val.get())]() {return &accessor;});
    }
};

template<typename T>
struct Reflector<const T&>
{
    static auto Create(const T& val)
    {
        return Reflector<T>::CreateFromPtr(&val);
    }
    static auto Create(const T*& val)
    {
        return Reflector<T>::CreateFromPtr(val);
    }
};

template<typename T>
struct Reflector<const T*&>
{
    static auto Create(const T*& val)
    {
        return Reflector<T>::CreateFromPtr(val);
    }

};

template<typename T>
struct Reflector<const T*const&>
{
    static auto Create(const T*const& val)
    {
        return Reflector<T>::CreateFromPtr(val);
    }

};

template<typename T>
struct Reflector<const std::shared_ptr<T>&>
{
    static auto Create(const std::shared_ptr<T>& val)
    {
        return Reflector<T>::CreateFromPtr(val.get());
    }
};

template<typename T>
struct Reflector<T&>
{
    static auto Create(T& val)
    {
        return Reflector<T>::Create(val);
    }
};

template<typename T>
struct Reflector<const T*>
{
    static auto Create(const T* val)
    {
        return Reflector<T>::CreateFromPtr(val);
    }
    static auto CreateFromPtr(const T* val)
    {
        return Reflector<T>::CreateFromPtr(val);
    }
//    static auto CreateFromPtr(const T*const val)
//    {
//        return Reflector<T>::CreateFromPtr(val);
//    }
};

template<typename T>
struct Reflector<T*>
{
    static auto Create(T* val)
    {
        return Reflector<T>::CreateFromPtr(val);
    }
};

template<typename T>
struct Reflector<std::shared_ptr<T>>
{
    static auto Create(std::shared_ptr<T> val)
    {
        return Reflector<T>::CreateFromPtr(val);
    }
};

template<>
struct Reflector<std::string>
{
    static auto Create(std::string str)
    {
        return Value(std::move(str));
    }
    static auto CreateFromPtr(const std::string* str)
    {
        return Value(*str);
    }
};

template<>
struct Reflector<bool>
{
    static auto Create(bool val)
    {
        return Value(val);
    }
    static auto CreateFromPtr(const bool* val)
    {
        return Value(*val);
    }
};

#define JINJA2_INT_REFLECTOR(Type) \
template<> \
struct Reflector<Type> \
{ \
    static auto Create(Type val) \
    { \
        return Value(static_cast<int64_t>(val)); \
    } \
    static auto CreateFromPtr(const Type* val) \
    { \
        return Value(static_cast<int64_t>(*val)); \
    } \
}

JINJA2_INT_REFLECTOR(int8_t);
JINJA2_INT_REFLECTOR(uint8_t);
JINJA2_INT_REFLECTOR(int16_t);
JINJA2_INT_REFLECTOR(uint16_t);
JINJA2_INT_REFLECTOR(int32_t);
JINJA2_INT_REFLECTOR(uint32_t);
JINJA2_INT_REFLECTOR(int64_t);
JINJA2_INT_REFLECTOR(uint64_t);
} // detail

template<typename T>
Value Reflect(T&& val)
{
    return detail::Reflector<T>::Create(std::forward<T>(val));
    // return Value(ReflectedMap([accessor = ReflectedMapImpl<T>(std::forward<T>(val))]() -> const ReflectedMap::ItemAccessor* {return &accessor;}));
}

} // jinja2

#endif // JINJA2_REFLECTED_VALUE_H
