#ifndef JINJA2_REFLECTED_VALUE_H
#define JINJA2_REFLECTED_VALUE_H

#include "value.h"

namespace jinja2
{

#if 0
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
#endif

namespace detail
{
template<typename T, typename Tag = void>
struct Reflector;

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
            return Reflect(m_value.begin() + idx);
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
    static Value CreateFromPtr(T&& cont)
    {
        return GenericList([accessor = ValueItemAccessor<T>(std::move(cont))]() {return &accessor;});
    }

    template<typename T>
    static Value CreateFromPtr(const T* cont)
    {
        return GenericList([accessor = PtrItemAccessor<T>(cont)]() {return &accessor;});
    }
};

template<typename T>
struct Reflector<std::set<T>>
{
    static auto Create(std::set<T> val)
    {
        return ContainerReflector::Create(std::move(val));
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
        return ContainerReflector::Create(std::move(val));
    }
    static auto CreateFromPtr(const std::vector<T>* val)
    {
        return ContainerReflector::CreateFromPtr(val);
    }
};

template<typename T>
struct Reflector<const T&>
{
    static auto Create(const T& val)
    {
        return Reflector<T>::CreateFromPtr(&val);
    }
};

template<typename T>
struct Reflector<const T*>
{
    static auto Create(const T* val)
    {
        return Reflector<T>::CreateFromPtr(val);
    }
};

template<typename T>
struct Reflector<T*>
{
    static auto Create(T* val)
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
} // detail

template<typename T>
Value Reflect(T&& val)
{
    return detail::Reflector<T>::Create(std::forward<T>(val));
    // return Value(ReflectedMap([accessor = ReflectedMapImpl<T>(std::forward<T>(val))]() -> const ReflectedMap::ItemAccessor* {return &accessor;}));
}

} // jinja2

#endif // JINJA2_REFLECTED_VALUE_H
