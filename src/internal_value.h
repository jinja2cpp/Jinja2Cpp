#ifndef INTERNAL_VALUE_H
#define INTERNAL_VALUE_H

#include <jinja2cpp/value.h>
#include <functional>
#include <boost/iterator/iterator_facade.hpp>
// #include <nonstd/value_ptr.hpp>
#include <nonstd/variant.hpp>
#include <boost/variant/recursive_wrapper.hpp>

namespace jinja2
{

template <class T>
class ReferenceWrapper
{
public:
    using type = T;

    ReferenceWrapper(T& ref) noexcept
        : m_ptr(std::addressof(ref))
    {
    }

    ReferenceWrapper(T&&) = delete;
    ReferenceWrapper(const ReferenceWrapper&) noexcept = default;

    // assignment
    ReferenceWrapper& operator=(const ReferenceWrapper& x) noexcept = default;

    // access
    T& get() const noexcept
    {
        return *m_ptr;
    }

private:
    T* m_ptr;
};

template<typename T, size_t SizeHint = 48>
class RecursiveWrapper
{
public:
    RecursiveWrapper() = default;

    RecursiveWrapper(const T& value)
        : m_data(value)
    {}

    RecursiveWrapper(T&& value)
        : m_data(std::move(value))
    {}

    const T& GetValue() const {return m_data.get();}
    T& GetValue() {return m_data.get();}

private:
    boost::recursive_wrapper<T> m_data;

#if 0
    enum class State
    {
        Undefined,
        Inplace,
        Ptr
    };

    State m_state;

    union
    {
        uint64_t dummy;
        nonstd::value_ptr<T> ptr;
    } m_data;
#endif
};

template<typename T>
auto MakeWrapped(T&& val)
{
    return RecursiveWrapper<std::decay_t<T>>(std::forward<T>(val));
}

using ValueRef = ReferenceWrapper<const Value>;
using TargetString = nonstd::variant<std::string, std::wstring>;

class ListAdapter;
class MapAdapter;
class RenderContext;
class OutStream;
class Callable;
struct CallParams;
struct KeyValuePair;
class RendererBase;

class InternalValue;
using InternalValueData = nonstd::variant<EmptyValue, bool, std::string, TargetString, int64_t, double, ValueRef, ListAdapter, MapAdapter, RecursiveWrapper<KeyValuePair>, RecursiveWrapper<Callable>, std::shared_ptr<RendererBase>>;

using InternalValueRef = ReferenceWrapper<InternalValue>;
using InternalValueMap = std::unordered_map<std::string, InternalValue>;
using InternalValueList = std::vector<InternalValue>;

template<typename T, bool isRecursive = false>
struct ValueGetter
{
    template<typename V>
    static auto& Get(V&& val)
    {
        return nonstd::get<T>(std::forward<V>(val).GetData());
    }

    static auto GetPtr(const InternalValue* val);

    static auto GetPtr(InternalValue* val);

    template<typename V>
    static auto GetPtr(V* val, std::enable_if_t<!std::is_same<V, InternalValue>::value>* = nullptr)
    {
        return nonstd::get_if<T>(val);
    }
};

template<typename T>
struct ValueGetter<T, true>
{
    template<typename V>
    static auto& Get(V&& val)
    {
        auto& ref = nonstd::get<RecursiveWrapper<T>>(std::forward<V>(val));
        return ref.GetValue();
    }

    static auto GetPtr(const InternalValue* val);

    static auto GetPtr(InternalValue* val);

    template<typename V>
    static auto GetPtr(V* val, std::enable_if_t<!std::is_same<V, InternalValue>::value>* = nullptr)
    {
        auto ref = nonstd::get_if<RecursiveWrapper<T>>(val);
        return !ref ? nullptr : &ref->GetValue();
    }
};

template<typename T>
struct IsRecursive : std::false_type {};

template<>
struct IsRecursive<KeyValuePair> : std::true_type {};

template<>
struct IsRecursive<Callable> : std::true_type {};

struct IListAccessor
{
    virtual ~IListAccessor() {}

    virtual nonstd::optional<size_t> GetSize() const = 0;
    virtual InternalValue GetItem(int64_t idx) const = 0;
    virtual GenericList CreateGenericList() const = 0;
    virtual bool ShouldExtendLifetime() const = 0;
};

using ListAccessorProvider = std::function<const IListAccessor*()>;

struct IMapAccessor
{
    virtual size_t GetSize() const = 0;
    virtual bool HasValue(const std::string& name) const = 0;
    virtual InternalValue GetItem(const std::string& name) const = 0;
    virtual std::vector<std::string> GetKeys() const = 0;
    virtual bool SetValue(std::string, const InternalValue&) {return false;}
    virtual GenericMap CreateGenericMap() const = 0;
    virtual bool ShouldExtendLifetime() const = 0;
};

using MapAccessorProvider = std::function<IMapAccessor*()>;

class ListAdapter
{
public:
    ListAdapter() {}
    explicit ListAdapter(ListAccessorProvider prov) : m_accessorProvider(std::move(prov)) {}
    ListAdapter(const ListAdapter&) = default;
    ListAdapter(ListAdapter&&) = default;

    static ListAdapter CreateAdapter(InternalValueList&& values);
    static ListAdapter CreateAdapter(const GenericList& values);
    static ListAdapter CreateAdapter(const ValuesList& values);
    static ListAdapter CreateAdapter(GenericList&& values);
    static ListAdapter CreateAdapter(ValuesList&& values);

    ListAdapter& operator = (const ListAdapter&) = default;
    ListAdapter& operator = (ListAdapter&&) = default;

    nonstd::optional<size_t> GetSize() const
    {
        if (m_accessorProvider && m_accessorProvider())
        {
            return m_accessorProvider()->GetSize();
        }

        return 0;
    }
    InternalValue GetValueByIndex(int64_t idx) const;
    bool ShouldExtendLifetime() const
    {
        if (m_accessorProvider && m_accessorProvider())
        {
            return m_accessorProvider()->ShouldExtendLifetime();
        }

        return false;
    }

    ListAdapter ToSubscriptedList(const InternalValue& subscript, bool asRef = false) const;
    InternalValueList ToValueList() const;
    GenericList CreateGenericList() const
    {
        if (m_accessorProvider && m_accessorProvider())
            return m_accessorProvider()->CreateGenericList();

        return GenericList();
    }

    class Iterator;

    Iterator begin() const;
    Iterator end() const;

private:
    ListAccessorProvider m_accessorProvider;
};

class MapAdapter
{
public:
    MapAdapter() {}
    explicit MapAdapter(MapAccessorProvider prov) : m_accessorProvider(std::move(prov)) {}

    static MapAdapter CreateAdapter(InternalValueMap&& values);
    static MapAdapter CreateAdapter(const InternalValueMap* values);
    static MapAdapter CreateAdapter(const GenericMap& values);
    static MapAdapter CreateAdapter(GenericMap&& values);
    static MapAdapter CreateAdapter(const ValuesMap& values);
    static MapAdapter CreateAdapter(ValuesMap&& values);

    size_t GetSize() const
    {
        if (m_accessorProvider && m_accessorProvider())
        {
            return m_accessorProvider()->GetSize();
        }

        return 0;
    }
    // InternalValue GetValueByIndex(int64_t idx) const;
    bool HasValue(const std::string& name) const
    {
        if (m_accessorProvider && m_accessorProvider())
        {
            return m_accessorProvider()->HasValue(name);
        }

        return false;
    }
    InternalValue GetValueByName(const std::string& name) const;
    std::vector<std::string> GetKeys() const
    {
        if (m_accessorProvider && m_accessorProvider())
        {
            return m_accessorProvider()->GetKeys();
        }

        return std::vector<std::string>();
    }
    bool SetValue(std::string name, const InternalValue& val)
    {
        if (m_accessorProvider && m_accessorProvider())
        {
            return m_accessorProvider()->SetValue(name, val);
        }

        return false;
    }
    bool ShouldExtendLifetime() const
    {
        if (m_accessorProvider && m_accessorProvider())
        {
            return m_accessorProvider()->ShouldExtendLifetime();
        }

        return false;
    }

    GenericMap CreateGenericMap() const
    {
        if (m_accessorProvider && m_accessorProvider())
            return m_accessorProvider()->CreateGenericMap();

        return GenericMap();
    }

private:
    MapAccessorProvider m_accessorProvider;
};


class InternalValue
{
public:
    InternalValue() = default;

    template<typename T>
    InternalValue(T&& val, typename std::enable_if<!std::is_same<std::decay_t<T>, InternalValue>::value>::type* = nullptr)
        : m_data(std::forward<T>(val))
    {
    }

    auto& GetData() const {return m_data;}
    auto& GetData() {return m_data;}

    void SetParentData(const InternalValue& val)
    {
        m_parentData = val.GetData();
    }

    void SetParentData(InternalValue&& val)
    {
        m_parentData = std::move(val.GetData());
    }

    bool ShouldExtendLifetime() const
    {
        if (m_parentData.index() != 0)
            return true;

        const MapAdapter* ma = nonstd::get_if<MapAdapter>(&m_data);
        if (ma != nullptr)
            return ma->ShouldExtendLifetime();

        const ListAdapter* la = nonstd::get_if<ListAdapter>(&m_data);
        if (la != nullptr)
            return la->ShouldExtendLifetime();

        return false;
    }

    bool IsEmpty() const {return m_data.index() == 0;}

private:
    InternalValueData m_data;
    InternalValueData m_parentData;
};

class ListAdapter::Iterator
        : public boost::iterator_facade<
            Iterator,
            const InternalValue,
            boost::forward_traversal_tag>
{
public:
    Iterator()
        : m_current(0)
        , m_list(nullptr)
    {}

    explicit Iterator(const ListAdapter& list)
        : m_current(0)
        , m_list(&list)
        , m_currentVal(list.GetSize() == 0ULL ? InternalValue() : list.GetValueByIndex(0))
    {}

private:
    friend class boost::iterator_core_access;

    void increment()
    {
        ++ m_current;
        m_currentVal = m_current == static_cast<int64_t>(m_list->GetSize()) ? InternalValue() : m_list->GetValueByIndex(m_current);
    }

    bool equal(const Iterator& other) const
    {
        if (m_list == nullptr)
            return other.m_list == nullptr ? true : other.equal(*this);

        if (other.m_list == nullptr)
            return m_current == static_cast<int64_t>(m_list->GetSize());

        return this->m_list == other.m_list && this->m_current == other.m_current;
    }

    const InternalValue& dereference() const
    {
        return m_currentVal;
    }

    int64_t m_current = 0;
    const ListAdapter* m_list;
    mutable InternalValue m_currentVal;
};

template<typename T, bool V>
inline auto ValueGetter<T, V>::GetPtr(const InternalValue* val)
{
    return nonstd::get_if<T>(&val->GetData());
}

template<typename T, bool V>
inline auto ValueGetter<T, V>::GetPtr(InternalValue* val)
{
    return nonstd::get_if<T>(&val->GetData());
}

template<typename T>
inline auto ValueGetter<T, true>::GetPtr(const InternalValue* val)
{
    auto ref = nonstd::get_if<RecursiveWrapper<T>>(&val->GetData());
    return !ref ? nullptr : &ref->GetValue();
}

template<typename T>
inline auto ValueGetter<T, true>::GetPtr(InternalValue* val)
{
    auto ref = nonstd::get_if<RecursiveWrapper<T>>(&val->GetData());
    return !ref ? nullptr : &ref->GetValue();
}

template<typename T, typename V>
auto& Get(V&& val)
{
    return ValueGetter<T, IsRecursive<T>::value>::Get(std::forward<V>(val).GetData());
}

template<typename T, typename V>
auto GetIf(V* val)
{
    return ValueGetter<T, IsRecursive<T>::value>::GetPtr(val);
}


inline InternalValue ListAdapter::GetValueByIndex(int64_t idx) const
{
    if (m_accessorProvider && m_accessorProvider())
    {
        return m_accessorProvider()->GetItem(idx);
    }

    return InternalValue();
}

//inline InternalValue MapAdapter::GetValueByIndex(int64_t idx) const
//{
//    if (m_accessorProvider && m_accessorProvider())
//    {
//        return static_cast<const IListAccessor*>(m_accessorProvider())->GetItem(idx);
//    }

//    return InternalValue();
//}

inline InternalValue MapAdapter::GetValueByName(const std::string& name) const
{
    if (m_accessorProvider && m_accessorProvider())
    {
        return m_accessorProvider()->GetItem(name);
    }

    return InternalValue();
}

inline ListAdapter::Iterator ListAdapter::begin() const {return Iterator(*this);}
inline ListAdapter::Iterator ListAdapter::end() const {return Iterator();}


struct KeyValuePair
{
    std::string key;
    InternalValue value;
};


class Callable
{
public:
    enum Kind
    {
        GlobalFunc,
        SpecialFunc,
        Macro,
        UserCallable
    };
    using ExpressionCallable = std::function<InternalValue (const CallParams&, RenderContext&)>;
    using StatementCallable = std::function<void (const CallParams&, OutStream&, RenderContext&)>;

    using CallableHolder = nonstd::variant<ExpressionCallable, StatementCallable>;

    enum class Type
    {
        Expression,
        Statement
    };

    Callable(Kind kind, ExpressionCallable&& callable)
        : m_kind(kind)
        , m_callable(std::move(callable))
    {
    }

    Callable(Kind kind, StatementCallable&& callable)
        : m_kind(kind)
        , m_callable(std::move(callable))
    {
    }

    auto GetType() const
    {
        return m_callable.index() == 0 ? Type::Expression : Type::Statement;
    }

    auto GetKind() const
    {
        return m_kind;
    }

    auto& GetCallable() const
    {
        return m_callable;
    }

    auto& GetExpressionCallable() const
    {
        return nonstd::get<ExpressionCallable>(m_callable);
    }

    auto& GetStatementCallable() const
    {
        return nonstd::get<StatementCallable>(m_callable);
    }

private:
    Kind m_kind;
    CallableHolder m_callable;
};

inline bool IsEmpty(const InternalValue& val)
{
    return nonstd::get_if<EmptyValue>(&val.GetData()) != nullptr;
}

InternalValue Subscript(const InternalValue& val, const InternalValue& subscript);
InternalValue Subscript(const InternalValue& val, const std::string& subscript);
std::string AsString(const InternalValue& val);
ListAdapter ConvertToList(const InternalValue& val, bool& isConverted);
ListAdapter ConvertToList(const InternalValue& val, InternalValue subscipt, bool& isConverted);
Value IntValue2Value(const InternalValue& val);

} // jinja2

#endif // INTERNAL_VALUE_H
