#ifndef INTERNAL_VALUE_H
#define INTERNAL_VALUE_H

#include <jinja2cpp/value.h>
#include <functional>
#include <boost/iterator/iterator_facade.hpp>

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

using ValueRef = ReferenceWrapper<const Value>;
using TargetString = boost::variant<std::string, std::wstring>;

class ListAdapter;
class MapAdapter;
class RenderContext;
class OutStream;
class Callable;
struct CallParams;
struct KeyValuePair;
class RendererBase;

using InternalValue = boost::variant<EmptyValue, bool, std::string, TargetString, int64_t, double, ValueRef, ListAdapter, MapAdapter, boost::recursive_wrapper<KeyValuePair>, boost::recursive_wrapper<Callable>, RendererBase*>;
using InternalValueRef = ReferenceWrapper<InternalValue>;
using InternalValueMap = std::unordered_map<std::string, InternalValue>;
using InternalValueList = std::vector<InternalValue>;

struct IListAccessor
{
    virtual ~IListAccessor() {}

    virtual size_t GetSize() const = 0;
    virtual InternalValue GetValueByIndex(int64_t idx) const = 0;
};

using ListAccessorProvider = std::function<const IListAccessor*()>;

struct IMapAccessor : public IListAccessor
{
    virtual bool HasValue(const std::string& name) const = 0;
    virtual InternalValue GetValueByName(const std::string& name) const = 0;
    virtual std::vector<std::string> GetKeys() const = 0;
    virtual bool SetValue(std::string name, const InternalValue& val) {return false;}
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

    size_t GetSize() const
    {
        if (m_accessorProvider && m_accessorProvider())
        {
            return m_accessorProvider()->GetSize();
        }

        return 0;
    }
    InternalValue GetValueByIndex(int64_t idx) const;

    ListAdapter ToSubscriptedList(const InternalValue& subscript, bool asRef = false) const;
    InternalValueList ToValueList() const;

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
    InternalValue GetValueByIndex(int64_t idx) const;
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

private:
    MapAccessorProvider m_accessorProvider;
};

class ListAdapter::Iterator
        : public boost::iterator_facade<
            Iterator,
            const InternalValue,
            boost::single_pass_traversal_tag>
{
public:
    Iterator()
        : m_current(0)
        , m_list(nullptr)
    {}

    explicit Iterator(const ListAdapter& list)
        : m_current(0)
        , m_list(&list)
        , m_currentVal(list.GetSize() == 0 ? InternalValue() : list.GetValueByIndex(0))
    {}

private:
    friend class boost::iterator_core_access;

    void increment()
    {
        ++ m_current;
        m_currentVal = m_current == m_list->GetSize() ? InternalValue() : m_list->GetValueByIndex(m_current);
    }

    bool equal(const Iterator& other) const
    {
        if (m_list == nullptr)
            return other.m_list == nullptr ? true : other.equal(*this);

        if (other.m_list == nullptr)
            return m_current == m_list->GetSize();

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

inline InternalValue ListAdapter::GetValueByIndex(int64_t idx) const
{
    if (m_accessorProvider && m_accessorProvider())
    {
        return m_accessorProvider()->GetValueByIndex(idx);
    }

    return InternalValue();
}

inline InternalValue MapAdapter::GetValueByIndex(int64_t idx) const
{
    if (m_accessorProvider && m_accessorProvider())
    {
        return m_accessorProvider()->GetValueByIndex(idx);
    }

    return InternalValue();
}

inline InternalValue MapAdapter::GetValueByName(const std::string& name) const
{
    if (m_accessorProvider && m_accessorProvider())
    {
        return m_accessorProvider()->GetValueByName(name);
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
    using ExpressionCallable = std::function<InternalValue (const CallParams&, RenderContext&)>;
    using StatementCallable = std::function<void (const CallParams&, OutStream&, RenderContext&)>;

    using CallableHolder = boost::variant<ExpressionCallable, StatementCallable>;

    enum class Type
    {
        Expression,
        Statement
    };

    Callable(ExpressionCallable&& callable)
        : m_callable(std::move(callable))
    {
    }

    Callable(StatementCallable&& callable)
        : m_callable(std::move(callable))
    {
    }

    auto GetType() const
    {
        return m_callable.which() == 0 ? Type::Expression : Type::Statement;
    }

    auto& GetCallable() const
    {
        return m_callable;
    }

    auto& GetExpressionCallable() const
    {
        return boost::get<ExpressionCallable>(m_callable);
    }

    auto& GetStatementCallable() const
    {
        return boost::get<StatementCallable>(m_callable);
    }

private:
    CallableHolder m_callable;
};

inline bool IsEmpty(const InternalValue& val)
{
    return boost::get<EmptyValue>(&val) != nullptr;
}

InternalValue Subscript(const InternalValue& val, const InternalValue& subscript);
InternalValue Subscript(const InternalValue& val, const std::string& subscript);
std::string AsString(const InternalValue& val);
ListAdapter ConvertToList(const InternalValue& val, bool& isConverted);
ListAdapter ConvertToList(const InternalValue& val, InternalValue subscipt, bool& isConverted);

} // jinja2

#endif // INTERNAL_VALUE_H
