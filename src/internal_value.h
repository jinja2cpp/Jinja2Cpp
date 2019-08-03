#ifndef INTERNAL_VALUE_H
#define INTERNAL_VALUE_H

#include <jinja2cpp/value.h>

#include <boost/iterator/iterator_facade.hpp>
#include <boost/variant/recursive_wrapper.hpp>
#include <boost/unordered_map.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/pool/object_pool.hpp>

#include <fmt/core.h>

#include <robin_hood.h>

#include <nonstd/string_view.hpp>
#include <nonstd/variant.hpp>

#include <functional>

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
using TargetStringView = nonstd::variant<nonstd::string_view, nonstd::wstring_view>;

class ListAdapter;
class MapAdapter;
class RenderContext;
class OutStream;
class Callable;
struct CallParams;
struct KeyValuePair;
class RendererBase;

class InternalValue;
using InternalValueData = nonstd::variant<
    EmptyValue,
    bool,
    std::string,
    TargetString,
    TargetStringView,
    int64_t,
    double,
    ValueRef,
    ListAdapter,
    MapAdapter,
    RecursiveWrapper<KeyValuePair>,
    RecursiveWrapper<Callable>,
    std::shared_ptr<RendererBase>>;

struct InternalValueDataHolder;
struct InternalValueDataPool;

using InternalValueDataPtr = boost::intrusive_ptr<InternalValueDataHolder>;

using InternalValueRef = ReferenceWrapper<InternalValue>;
// using InternalValueMap = std::unordered_map<std::string, InternalValue>;
// using InternalValueMap = robin_hood::unordered_map<std::string, InternalValue>;
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

struct IListAccessorEnumerator
{
    virtual ~IListAccessorEnumerator() {}

    virtual void Reset() = 0;

    virtual bool MoveNext() = 0;
    virtual InternalValue GetCurrent() const = 0;

    virtual IListAccessorEnumerator* Clone() const = 0;
    virtual IListAccessorEnumerator* Transfer() = 0;

    struct Cloner
    {
        Cloner() = default;

        IListAccessorEnumerator* operator()(const IListAccessorEnumerator &x) const
        {
            return x.Clone();
        }

        IListAccessorEnumerator* operator()(IListAccessorEnumerator &&x) const
        {
            return x.Transfer();
        }
    };
};

using ListAccessorEnumeratorPtr = nonstd::value_ptr<IListAccessorEnumerator, IListAccessorEnumerator::Cloner>;

struct IListAccessor
{
    virtual ~IListAccessor() {}

    virtual nonstd::optional<size_t> GetSize() const = 0;
    virtual nonstd::optional<InternalValue> GetItem(int64_t idx) const = 0;
    virtual ListAccessorEnumeratorPtr CreateListAccessorEnumerator() const = 0;
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

    ListAdapter ToSubscriptedList(const InternalValue& subscript, RenderContext& values, bool asRef = false) const;
    InternalValueList ToValueList() const;
    GenericList CreateGenericList() const
    {
        if (m_accessorProvider && m_accessorProvider())
            return m_accessorProvider()->CreateGenericList();

        return GenericList();
    }
    ListAccessorEnumeratorPtr GetEnumerator() const;

    class Iterator;

    Iterator begin() const;
    Iterator end() const;

private:
    ListAccessorProvider m_accessorProvider;
};

class MapAdapter
{
public:
    MapAdapter() = default;
    explicit MapAdapter(MapAccessorProvider prov) : m_accessorProvider(std::move(prov)) {}

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
            return m_accessorProvider()->SetValue(std::move(name), val);
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

//    template<typename T>
//    InternalValue(T&& val, typename std::enable_if<!std::is_same<std::decay_t<T>, InternalValue>::value>::type* = nullptr)
//        // : m_data(std::make_shared<InternalValueData>(std::forward<T>(val)))
//    {
//    }
    template<typename T>
    static InternalValue Create(T&& val, InternalValueDataPool* pool);
    static InternalValue CreateEmpty(InternalValueDataPool* pool);

    const InternalValueData& GetData() const;
    InternalValueData& GetData();

#if 0
    template<typename T>
    typename std::enable_if<!std::is_same<std::decay_t<T>, InternalValue>::value, InternalValue&>::type operator=(T&& val)
    {
        SetData(std::forward<T>(val));
        return *this;
    }
#endif

    template<typename T>
    void SetData(T&& val);

    void SetParentData(const InternalValue& val)
    {
        // m_parentData = val.GetData();
    }

    void SetParentData(InternalValue&& val)
    {
        // m_parentData = std::move(val.GetData());
    }

    bool ShouldExtendLifetime() const;

    bool IsEmpty() const;

    void SetTemporary(bool val) {m_isTemporary = val;}
    bool IsTemporary() const {return m_isTemporary;}

private:
    explicit InternalValue(InternalValueDataPtr data);

    static InternalValueData& GetEmptyData()
    {
        static InternalValueData data;
        return data;
    }

    InternalValueDataPtr m_data;
    InternalValueDataPtr m_parentData;
    bool m_isTemporary = false;
};

struct InternalValueDataHolder
{
    template<typename T>
    InternalValueDataHolder(T&& val, typename std::enable_if<!std::is_same<std::decay_t<T>, InternalValueDataHolder>::value>::type* = nullptr)
        : data(std::forward<T>(val))
    {
    }

    InternalValueData data;
    int counter = 1;
    InternalValueDataPool* pool = nullptr;
};

struct InternalValueDataPool
{
    boost::object_pool<InternalValueDataHolder> pool;

    template<typename ... Args>
    InternalValueDataHolder* allocate(Args&& ... args)
    {
        auto obj = pool.construct(std::forward<Args>(args)...);
        obj->pool = this;
        return obj;
    }

    void destroy(InternalValueDataHolder* obj)
    {
        pool.destroy(obj);
    }
};

inline void intrusive_ptr_add_ref(InternalValueDataHolder* p)
{
    ++ p->counter;
}

inline void intrusive_ptr_release(InternalValueDataHolder* p)
{
    if (!p)
        return;

    -- p->counter;
    if (!p->counter)
        reinterpret_cast<InternalValueDataPool*>(p->pool)->destroy(p);
}

inline InternalValue::InternalValue(InternalValueDataPtr data)
    : m_data(data)
{
}

inline const InternalValueData& InternalValue::GetData() const {return m_data ? m_data->data : GetEmptyData();}
inline InternalValueData& InternalValue::GetData() {return m_data ? m_data->data : GetEmptyData();}

inline bool InternalValue::ShouldExtendLifetime() const
{
    if (!m_parentData || !m_data)
        return false;

    if (m_parentData->data.index() != 0)
        return true;

    const MapAdapter* ma = nonstd::get_if<MapAdapter>(&m_data->data);
    if (ma != nullptr)
        return ma->ShouldExtendLifetime();

    const ListAdapter* la = nonstd::get_if<ListAdapter>(&m_data->data);
    if (la != nullptr)
        return la->ShouldExtendLifetime();

    return false;
}

inline bool InternalValue::IsEmpty() const {return !m_data || m_data->data.index() == 0;}

template<typename T>
InternalValue InternalValue::Create(T&& val, InternalValueDataPool* pool)
{
    return InternalValue(pool->allocate(std::forward<T>(val)));
}

inline InternalValue InternalValue::CreateEmpty(InternalValueDataPool* pool)
{
    return Create(EmptyValue(), pool);
}

template<typename T>
void InternalValue::SetData(T&& val)
{
    m_data->data = std::forward<T>(val);
}

class ListAdapter::Iterator
        : public boost::iterator_facade<
            Iterator,
            const InternalValue,
            boost::forward_traversal_tag>
{
public:
    Iterator() = default;

    explicit Iterator(ListAccessorEnumeratorPtr&& iter)
        : m_iterator(std::move(iter))
        , m_isFinished(!m_iterator->MoveNext())
        , m_currentVal(m_isFinished ? InternalValue() : m_iterator->GetCurrent())
    {}

private:
    friend class boost::iterator_core_access;

    void increment()
    {
        m_isFinished = !m_iterator->MoveNext();
        ++ m_currentIndex;
        m_currentVal = m_isFinished ? InternalValue() : m_iterator->GetCurrent();
    }

    bool equal(const Iterator& other) const
    {
        if (!this->m_iterator)
            return !other.m_iterator ? true : other.equal(*this);

        if (!other.m_iterator)
            return this->m_isFinished;

        return this->m_iterator.get() == other.m_iterator.get() && this->m_currentIndex == other.m_currentIndex;
    }

    const InternalValue& dereference() const
    {
        return m_currentVal;
    }

    ListAccessorEnumeratorPtr m_iterator;
    bool m_isFinished = true;
    mutable uint64_t m_currentIndex = 0;
    mutable InternalValue m_currentVal;
};

typedef robin_hood::unordered_map<std::string, InternalValue> InternalValueMap;


ListAdapter CreateListAdapter(InternalValueList&& values);
ListAdapter CreateListAdapter(const GenericList& values, InternalValueDataPool* pool);
ListAdapter CreateListAdapter(const ValuesList& values, InternalValueDataPool* pool);
ListAdapter CreateListAdapter(GenericList&& values, InternalValueDataPool* pool);
ListAdapter CreateListAdapter(ValuesList&& values, InternalValueDataPool* pool);
ListAdapter CreateListAdapter(std::function<nonstd::optional<InternalValue> ()> fn);
ListAdapter CreateListAdapter(size_t listSize, std::function<InternalValue (size_t idx)> fn);

MapAdapter CreateMapAdapter(InternalValueMap&& values);
MapAdapter CreateMapAdapter(const InternalValueMap* values);
MapAdapter CreateMapAdapter(const GenericMap& values, InternalValueDataPool* pool);
MapAdapter CreateMapAdapter(GenericMap&& values, InternalValueDataPool* pool);
MapAdapter CreateMapAdapter(const ValuesMap& values, InternalValueDataPool* pool);
MapAdapter CreateMapAdapter(ValuesMap&& values, InternalValueDataPool* pool);

template<typename ... Args>
InternalValue CreateListAdapterValue(InternalValueDataPool* pool, Args&& ... args)
{
    return InternalValue::Create(CreateListAdapter(std::forward<Args>(args)...), pool);
}

template<typename ... Args>
InternalValue CreateMapAdapterValue(InternalValueDataPool* pool, Args&& ... args)
{
    return InternalValue::Create(CreateMapAdapter(std::forward<Args>(args)...), pool);
}


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
        const auto& val = m_accessorProvider()->GetItem(idx);
        if (val)
            return std::move(val.value());

        return InternalValue();
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

inline ListAccessorEnumeratorPtr ListAdapter::GetEnumerator() const {return m_accessorProvider()->CreateListAccessorEnumerator();}
inline ListAdapter::Iterator ListAdapter::begin() const {return Iterator(m_accessorProvider()->CreateListAccessorEnumerator());}
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
    return val.IsEmpty() || nonstd::get_if<EmptyValue>(&val.GetData()) != nullptr;
}

class RenderContext;

template<typename Fn>
auto MakeDynamicProperty(Fn&& fn, InternalValueDataPool* pool)
{
    return CreateMapAdapterValue(pool, InternalValueMap{
        {"value()", InternalValue::Create(Callable(Callable::GlobalFunc, std::forward<Fn>(fn)), pool)}
    });
}

template<typename CharT>
auto sv_to_string(const nonstd::basic_string_view<CharT>& sv)
{
    return std::basic_string<CharT>(sv.begin(), sv.end());
}

InternalValue Subscript(const InternalValue& val, const InternalValue& subscript, RenderContext& values);
InternalValue Subscript(const InternalValue& val, const std::string& subscript, RenderContext& values);
std::string AsString(const InternalValue& val);
ListAdapter ConvertToList(const InternalValue& val, RenderContext& values, bool& isConverted);
ListAdapter ConvertToList(const InternalValue& val, InternalValue subscipt, RenderContext& values, bool& isConverted);
Value IntValue2Value(const InternalValue& val);
Value OptIntValue2Value(nonstd::optional<InternalValue> val);

} // jinja2

#endif // INTERNAL_VALUE_H
