#ifndef JINJA2CPP_SRC_INTERNAL_VALUE_H
#define JINJA2CPP_SRC_INTERNAL_VALUE_H

#include "ordered_map.h"

#include <jinja2cpp/config.h>
#include <jinja2cpp/generic_list.h>
#include <jinja2cpp/utils/i_comparable.h>
#include <jinja2cpp/value.h>
#include <jinja2cpp/value_ptr.h>

#include <boost/iterator/iterator_facade.hpp>
#include <boost/unordered_map.hpp>
#include <boost/variant/recursive_wrapper.hpp>
#include <fmt/core.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>

#if defined(_MSC_VER) && _MSC_VER <= 1900 // robin_hood hash map doesn't compatible with MSVC 14.0
#include <unordered_map>
#else
#include "robin_hood.h"
#endif

#include <optional>
#include <string_view>
#include <type_traits>
#include <variant>

#include <algorithm>
#include <functional>
#include <vector>

namespace jinja2
{

template<class T>
class ReferenceWrapper
{
public:
    using type = T;

    ReferenceWrapper(T& ref) noexcept // NOLINT(google-explicit-constructor)
        : m_ptr(std::addressof(ref))
    {
    }

    ReferenceWrapper(T&&) = delete;
    ReferenceWrapper(const ReferenceWrapper&) noexcept = default;
    ReferenceWrapper(ReferenceWrapper&&) noexcept = default;
    ~ReferenceWrapper() = default;

    // assignment
    ReferenceWrapper& operator=(const ReferenceWrapper& x) noexcept = default;
    ReferenceWrapper& operator=(ReferenceWrapper&& x) noexcept = default;

    // access
    [[nodiscard]] T& get() const noexcept
    {
        return *m_ptr;
    }

private:
    T* m_ptr;
};

template<typename T>
class RecursiveWrapper
{
public:
    RecursiveWrapper() = default;

    RecursiveWrapper(const T& value) // NOLINT(google-explicit-constructor)
        : m_data(value)
    {}

    RecursiveWrapper(T&& value) // NOLINT(google-explicit-constructor)
        : m_data(std::move(value))
    {}

    [[nodiscard]] const T& GetValue() const { return m_data.get(); }
    T& GetValue() { return m_data.get(); }

private:
    boost::recursive_wrapper<T> m_data;
};

template<typename T>
auto MakeWrapped(T&& val)
{
    return RecursiveWrapper<std::decay_t<T>>(std::forward<T>(val));
}

using ValueRef = ReferenceWrapper<const Value>;
using TargetString = std::variant<std::string, std::wstring>;
using TargetStringView = std::variant<std::string_view, std::wstring_view>;

class ListAdapter;
class MapAdapter;
class RenderContext;
class OutStream;
class Callable;
struct CallParams;
struct KeyValuePair;
class IRendererBase;

// What a missing name, attribute or item evaluates to (Python's Undefined). It is the
// default alternative, so every lookup miss and error path that returns InternalValue()
// is undefined, while EmptyValue is Python's None. A named lookup miss carries info
// (src/undefined.h): what was missing and the undefined policy that decides how its uses
// fail. Without info (internal error paths) it behaves like Python's default Undefined
// that is only printed or tested.
struct UndefinedInfo;
struct UndefinedValue
{
    std::shared_ptr<const UndefinedInfo> info;
};

inline bool operator==(const UndefinedValue&, const UndefinedValue&)
{
    return true;
}

inline bool operator!=(const UndefinedValue&, const UndefinedValue&)
{
    return false;
}

class InternalValue;
using InternalValueData = std::variant<
    UndefinedValue,
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
    std::shared_ptr<IRendererBase>>;


using InternalValueRef = ReferenceWrapper<InternalValue>;
using InternalValueList = std::vector<InternalValue>;
// Mappings a template can iterate (dict literals, kwargs) keep insertion order, as Python
// dicts do; scopes and other lookup-only maps stay InternalValueMap (docs/tasks/0031)
using InternalDict = OrderedMap<std::string, InternalValue>;

template<typename T, bool isRecursive = false>
struct ValueGetter
{
    template<typename V>
    static auto& Get(V&& val)
    {
        return std::get<T>(std::forward<V>(val).GetData());
    }

    static auto GetPtr(const InternalValue* val);

    static auto GetPtr(InternalValue* val);

    template<typename V>
    static auto GetPtr(V* val, std::enable_if_t<!std::is_same_v<V, InternalValue>>* = nullptr)
    {
        return std::get_if<T>(val);
    }
};

template<typename T>
struct ValueGetter<T, true>
{
    template<typename V>
    static auto& Get(V&& val)
    {
        auto& ref = std::get<RecursiveWrapper<T>>(std::forward<V>(val));
        return ref.GetValue();
    }

    static auto GetPtr(const InternalValue* val);

    static auto GetPtr(InternalValue* val);

    template<typename V>
    static auto GetPtr(V* val, std::enable_if_t<!std::is_same_v<V, InternalValue>>* = nullptr)
    {
        auto ref = std::get_if<RecursiveWrapper<T>>(val);
        return !ref ? nullptr : &ref->GetValue();
    }
};

template<typename T>
struct IsRecursive : std::false_type
{
};

template<>
struct IsRecursive<KeyValuePair> : std::true_type
{
};

template<>
struct IsRecursive<Callable> : std::true_type
{
};

struct IListAccessorEnumerator;
using ListAccessorEnumeratorPtr = types::ValuePtr<IListAccessorEnumerator>;
struct IListAccessorEnumerator : virtual IComparable
{
    ~IListAccessorEnumerator() override = default;

    virtual void Reset() = 0;

    virtual bool MoveNext() = 0;
    [[nodiscard]] virtual InternalValue GetCurrent() const = 0;

    [[nodiscard]] virtual std::optional<ListAccessorEnumeratorPtr> Clone() const = 0;
    virtual std::optional<ListAccessorEnumeratorPtr> Transfer() = 0;
    /*
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
    */
};

using ListAccessorEnumeratorPtr = types::ValuePtr<IListAccessorEnumerator>;

// The arguments of a range() call, kept so that the range prints as range(0, 3)
struct RangeInfo
{
    int64_t start;
    int64_t stop;
    int64_t step;
};

struct IListAccessor
{
    virtual ~IListAccessor() = default;

    [[nodiscard]] virtual std::optional<size_t> GetSize() const = 0;
    [[nodiscard]] virtual std::optional<InternalValue> GetItem(int64_t idx) const = 0;
    [[nodiscard]] virtual std::optional<ListAccessorEnumeratorPtr> CreateListAccessorEnumerator() const = 0;
    [[nodiscard]] virtual GenericList CreateGenericList() const = 0;
    [[nodiscard]] virtual bool ShouldExtendLifetime() const = 0;
    // The object behind the list: the same for two accessors that share their data, so
    // printing can tell a list that contains itself
    [[nodiscard]] virtual const void* GetIdentity() const { return this; }
    // Set only for the lists made by range()
    [[nodiscard]] virtual const RangeInfo* GetRangeInfo() const { return nullptr; }
    // The items of a list the template owns (shared by every copy of the value, as a
    // Python list is), so that methods like append() can change them; null for borrowed
    // and computed lists (docs/tasks/0020)
    [[nodiscard]] virtual InternalValueList* GetMutableItems() const { return nullptr; }
    // An accessor with iteration state of its own (a generator) is copied with the list
    // value, so that copies iterate independently; every other accessor is shared
    [[nodiscard]] virtual bool ClonesOnCopy() const { return false; }
    [[nodiscard]] virtual std::shared_ptr<const IListAccessor> Clone() const { return nullptr; }
};

// How x.name looks a name up on a map (docs/tasks/0020)
enum class MapAttrPolicy
{
    // An object stored as a map (loop, cycler, self, imported namespaces): keys only
    KeysOnly,
    // A Python dict (dict literals, kwargs, context mappings): dict methods first, as
    // Python's getattr finds them before the items
    MethodsFirst,
    // A host object (reflected structs, JSON bindings): its keys, then dict methods
    KeysFirst
};

struct IMapAccessor
{
    virtual ~IMapAccessor() = default;
    [[nodiscard]] virtual size_t GetSize() const = 0;
    [[nodiscard]] virtual bool HasValue(const std::string& name) const = 0;
    [[nodiscard]] virtual InternalValue GetItem(const std::string& name) const = 0;
    [[nodiscard]] virtual std::vector<std::string> GetKeys() const = 0;
    // By value: overrides store the key. NOLINTNEXTLINE(performance-unnecessary-value-param)
    virtual bool SetValue(std::string, const InternalValue&) { return false; }
    [[nodiscard]] virtual GenericMap CreateGenericMap() const = 0;
    [[nodiscard]] virtual bool ShouldExtendLifetime() const = 0;
    // Whether the names are attributes of an object (a user-provided map, such as a reflected
    // struct) rather than the keys of a dict, which Python's getattr does not see
    [[nodiscard]] virtual bool HasAttributes() const { return false; }
    // See IListAccessor::GetIdentity
    [[nodiscard]] virtual const void* GetIdentity() const { return this; }
    // See IListAccessor::GetMutableItems
    [[nodiscard]] virtual InternalDict* GetMutableItems() const { return nullptr; }
    // How x.name resolves against Python's dict methods (items, get, ...)
    [[nodiscard]] virtual MapAttrPolicy GetAttrPolicy() const { return MapAttrPolicy::KeysOnly; }
    // A namespace() object, the only one `set obj.attr = ...` can change
    [[nodiscard]] virtual bool IsNamespace() const { return false; }
};

class ListAdapter
{
public:
    ListAdapter() = default;
    // Copies of the list share the accessor, as names share a Python list, unless it has
    // iteration state of its own (IListAccessor::Clone)
    explicit ListAdapter(std::shared_ptr<const IListAccessor> accessor)
        : m_accessor(std::move(accessor))
        , m_clonesOnCopy(m_accessor && m_accessor->ClonesOnCopy())
    {
    }
    ListAdapter(const ListAdapter& other)
        : m_accessor(other.m_clonesOnCopy && other.m_accessor ? other.m_accessor->Clone() : other.m_accessor)
        , m_clonesOnCopy(other.m_clonesOnCopy)
        , m_isTuple(other.m_isTuple)
        , m_fieldNames(other.m_fieldNames)
    {
    }
    ListAdapter(ListAdapter&&) = default;

    static ListAdapter CreateAdapter(InternalValueList&& values);
    static ListAdapter CreateAdapter(const GenericList& values);
    static ListAdapter CreateAdapter(const ValuesList& values);
    static ListAdapter CreateAdapter(GenericList&& values);
    static ListAdapter CreateAdapter(ValuesList&& values);
    static ListAdapter CreateAdapter(std::function<std::optional<InternalValue>()> fn);
    static ListAdapter CreateAdapter(size_t listSize, std::function<InternalValue(size_t idx)> fn);
    // The lazy list of range(start, stop, step), as in Python; step must not be zero
    static ListAdapter CreateRange(int64_t start, int64_t stop, int64_t step);

    ListAdapter& operator=(const ListAdapter& other)
    {
        if (this != &other)
        {
            *this = ListAdapter(other);
        }
        return *this;
    }
    ListAdapter& operator=(ListAdapter&&) = default;
    ~ListAdapter() = default;

    [[nodiscard]] std::optional<size_t> GetSize() const
    {
        if (m_accessor)
        {
            return m_accessor->GetSize();
        }

        return 0;
    }
    [[nodiscard]] InternalValue GetValueByIndex(int64_t idx) const;
    [[nodiscard]] bool ShouldExtendLifetime() const
    {
        if (m_accessor)
        {
            return m_accessor->ShouldExtendLifetime();
        }

        return false;
    }

    [[nodiscard]] ListAdapter ToSubscriptedList(const InternalValue& subscript, bool asRef = false) const;
    [[nodiscard]] InternalValueList ToValueList() const;
    [[nodiscard]] const void* GetIdentity() const
    {
        if (m_accessor)
        {
            return m_accessor->GetIdentity();
        }

        return nullptr;
    }
    [[nodiscard]] const RangeInfo* GetRangeInfo() const
    {
        if (m_accessor)
        {
            return m_accessor->GetRangeInfo();
        }

        return nullptr;
    }
    [[nodiscard]] InternalValueList* GetMutableItems() const
    {
        if (m_accessor)
        {
            return m_accessor->GetMutableItems();
        }

        return nullptr;
    }
    [[nodiscard]] GenericList CreateGenericList() const
    {
        if (m_accessor)
        {
            return m_accessor->CreateGenericList();
        }

        return GenericList();
    }
    [[nodiscard]] std::optional<ListAccessorEnumeratorPtr> GetEnumerator() const;

    class Iterator;

    [[nodiscard]] Iterator begin() const;
    [[nodiscard]] Iterator end() const; // NOLINT(readability-convert-member-functions-to-static): container API

    // Tuples are lists that print as (a, b) instead of [a, b]
    [[nodiscard]] bool IsTuple() const { return m_isTuple; }
    ListAdapter& MarkAsTuple()
    {
        m_isTuple = true;
        return *this;
    }
    // A namedtuple: a tuple whose items can also be read by field name, like groupby's (grouper, list)
    ListAdapter& MarkAsNamedTuple(std::shared_ptr<const std::vector<std::string>> fieldNames)
    {
        m_isTuple = true;
        m_fieldNames = std::move(fieldNames);
        return *this;
    }
    [[nodiscard]] const std::vector<std::string>* GetFieldNames() const { return m_fieldNames.get(); }

private:
    std::shared_ptr<const IListAccessor> m_accessor;
    bool m_clonesOnCopy = false;
    bool m_isTuple = false;
    std::shared_ptr<const std::vector<std::string>> m_fieldNames;
};

class MapAdapter
{
public:
    MapAdapter() = default;
    // Copies of the map share the accessor, as names share a Python dict
    explicit MapAdapter(std::shared_ptr<IMapAccessor> accessor)
        : m_accessor(std::move(accessor))
    {
    }

    [[nodiscard]] size_t GetSize() const
    {
        if (m_accessor)
        {
            return m_accessor->GetSize();
        }

        return 0;
    }
    // InternalValue GetValueByIndex(int64_t idx) const;
    [[nodiscard]] bool HasValue(const std::string& name) const
    {
        if (m_accessor)
        {
            return m_accessor->HasValue(name);
        }

        return false;
    }
    [[nodiscard]] InternalValue GetValueByName(const std::string& name) const;
    [[nodiscard]] const void* GetIdentity() const
    {
        if (m_accessor)
        {
            return m_accessor->GetIdentity();
        }

        return nullptr;
    }
    [[nodiscard]] bool HasAttributes() const
    {
        if (m_accessor)
        {
            return m_accessor->HasAttributes();
        }

        return false;
    }
    [[nodiscard]] std::vector<std::string> GetKeys() const
    {
        if (m_accessor)
        {
            return m_accessor->GetKeys();
        }

        return std::vector<std::string>();
    }
    [[nodiscard]] InternalDict* GetMutableItems() const
    {
        if (m_accessor)
        {
            return m_accessor->GetMutableItems();
        }

        return nullptr;
    }
    [[nodiscard]] MapAttrPolicy GetAttrPolicy() const
    {
        if (m_accessor)
        {
            return m_accessor->GetAttrPolicy();
        }

        return MapAttrPolicy::KeysOnly;
    }
    [[nodiscard]] bool IsNamespace() const
    {
        if (m_accessor)
        {
            return m_accessor->IsNamespace();
        }

        return false;
    }
    bool SetValue(std::string name, const InternalValue& val)
    {
        if (m_accessor)
        {
            return m_accessor->SetValue(std::move(name), val);
        }

        return false;
    }
    [[nodiscard]] bool ShouldExtendLifetime() const
    {
        if (m_accessor)
        {
            return m_accessor->ShouldExtendLifetime();
        }

        return false;
    }

    [[nodiscard]] GenericMap CreateGenericMap() const
    {
        if (m_accessor)
        {
            return m_accessor->CreateGenericMap();
        }

        return GenericMap();
    }

private:
    std::shared_ptr<IMapAccessor> m_accessor;
};


class InternalValue
{
public:
    InternalValue() = default;


    InternalValue(bool val) // NOLINT(google-explicit-constructor)
        : m_data(InternalValueData(val))
    {
    }

    InternalValue(int64_t val) // NOLINT(google-explicit-constructor)
        : m_data(InternalValueData(val))
    {
    }

    InternalValue(double val) // NOLINT(google-explicit-constructor)
        : m_data(InternalValueData(val))
    {
    }

    template<typename T>
    InternalValue(T&& val, std::enable_if_t<!std::is_same_v<std::decay_t<T>, InternalValue>>* = nullptr) // NOLINT(google-explicit-constructor)
        : m_data(InternalValueData(std::forward<T>(val)))
    {
    }

    [[nodiscard]] auto& GetData() const { return m_data; }
    auto& GetData() { return m_data; }

    void SetParentData(const InternalValue& val);

    void SetParentData(InternalValue&& val);

    [[nodiscard]] bool ShouldExtendLifetime() const
    {
        if (m_parentData)
        {
            return true;
        }

        const MapAdapter* ma = std::get_if<MapAdapter>(&m_data);
        if (ma)
        {
            return ma->ShouldExtendLifetime();
        }

        const ListAdapter* la = std::get_if<ListAdapter>(&m_data);
        if (la)
        {
            return la->ShouldExtendLifetime();
        }

        return false;
    }

    [[nodiscard]] bool IsUndefined() const { return m_data.index() == 0; }
    [[nodiscard]] bool IsNone() const { return std::get_if<EmptyValue>(&m_data) != nullptr; }

    [[nodiscard]] bool IsEqual(const InternalValue& other) const;

    //! Python's Markup: a string already safe for HTML output. Autoescape leaves it as is
    [[nodiscard]] bool IsMarkup() const { return m_isMarkup; }
    InternalValue& SetMarkup(bool isMarkup = true)
    {
        m_isMarkup = isMarkup;
        return *this;
    }

private:
    InternalValueData m_data;
    // The value this one was taken from, kept alive for as long as this one lives (set only
    // when defined). Shared and immutable: copies of a value need not copy it, and a value
    // stays one pointer bigger instead of a whole second variant
    std::shared_ptr<const InternalValueData> m_parentData;
    bool m_isMarkup = false;
};

inline bool operator==(const InternalValue& lhs, const InternalValue& rhs)
{
    return lhs.IsEqual(rhs);
}
inline bool operator!=(const InternalValue& lhs, const InternalValue& rhs)
{
    return !(lhs == rhs);
}

class JINJA2CPP_EXPORT ListAdapter::Iterator
    : public boost::iterator_facade<
          Iterator,
          const InternalValue,
          boost::forward_traversal_tag>
{
public:
    Iterator();

    explicit Iterator(std::optional<ListAccessorEnumeratorPtr>&& iter)
        : m_iterator(std::move(iter))
        , m_isFinished(m_iterator ? !(*m_iterator)->MoveNext() : true)
        , m_currentVal(m_isFinished || !m_iterator ? InternalValue() : (*m_iterator)->GetCurrent())
    {}

private:
    friend class boost::iterator_core_access;

    void increment();

    bool equal(const Iterator& other) const;

    const InternalValue& dereference() const
    {
        return m_currentVal;
    }

    std::optional<ListAccessorEnumeratorPtr> m_iterator;
    bool m_isFinished = true;
    mutable uint64_t m_currentIndex = 0;
    mutable InternalValue m_currentVal;
};

// A variable name with its hash, computed once when the template is parsed, so that a
// lookup through several scopes hashes nothing
struct HashedName
{
    std::string_view name;
    size_t hash;

    static size_t Hash(std::string_view name) noexcept { return robin_hood::hash_bytes(name.data(), name.size()); }
};

struct NameHash
{
    using is_transparent = void;
    size_t operator()(const std::string& name) const noexcept { return HashedName::Hash(name); }
    size_t operator()(const HashedName& name) const noexcept { return name.hash; }
};

struct NameEqual
{
    using is_transparent = void;
    bool operator()(const std::string& lhs, const std::string& rhs) const noexcept { return lhs == rhs; }
    bool operator()(const std::string& lhs, const HashedName& rhs) const noexcept { return lhs == rhs.name; }
    bool operator()(const HashedName& lhs, const std::string& rhs) const noexcept { return lhs.name == rhs; }
};

using InternalValueMap = robin_hood::unordered_map<std::string, InternalValue, NameHash, NameEqual>;

MapAdapter CreateMapAdapter(InternalValueMap&& values);
MapAdapter CreateMapAdapter(InternalDict&& values);
// Jinja2's namespace(): a shared mapping whose attributes `set ns.attr = ...` assigns
MapAdapter CreateNamespaceAdapter(InternalDict&& values);
MapAdapter CreateMapAdapter(const InternalValueMap* values);
// Shares the map: the adapter keeps it alive (a loop object kept past its loop)
MapAdapter CreateMapAdapter(std::shared_ptr<InternalValueMap> values);
MapAdapter CreateMapAdapter(const GenericMap& values);
MapAdapter CreateMapAdapter(GenericMap&& values);
MapAdapter CreateMapAdapter(const ValuesMap& values);
MapAdapter CreateMapAdapter(ValuesMap&& values);

template<typename T, bool V>
inline auto ValueGetter<T, V>::GetPtr(const InternalValue* val)
{
    return std::get_if<T>(&val->GetData());
}

template<typename T, bool V>
inline auto ValueGetter<T, V>::GetPtr(InternalValue* val)
{
    return std::get_if<T>(&val->GetData());
}

template<typename T>
inline auto ValueGetter<T, true>::GetPtr(const InternalValue* val)
{
    auto ref = std::get_if<RecursiveWrapper<T>>(&val->GetData());
    return !ref ? nullptr : &ref->GetValue();
}

template<typename T>
inline auto ValueGetter<T, true>::GetPtr(InternalValue* val)
{
    auto ref = std::get_if<RecursiveWrapper<T>>(&val->GetData());
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
    if (m_accessor)
    {
        const auto& val = m_accessor->GetItem(idx);
        if (val)
        {
            return val.value();
        }

        return InternalValue();
    }

    return InternalValue();
}

//inline InternalValue MapAdapter::GetValueByIndex(int64_t idx) const
//{
//    if (m_accessor)
//    {
//        return static_cast<const IListAccessor*>(m_accessorProvider())->GetItem(idx);
//    }

//    return InternalValue();
//}

inline InternalValue MapAdapter::GetValueByName(const std::string& name) const
{
    if (m_accessor)
    {
        return m_accessor->GetItem(name);
    }

    return InternalValue();
}

inline std::optional<ListAccessorEnumeratorPtr> ListAdapter::GetEnumerator() const
{
    return m_accessor ? m_accessor->CreateListAccessorEnumerator() : std::optional<ListAccessorEnumeratorPtr>();
}
inline ListAdapter::Iterator ListAdapter::begin() const
{
    return m_accessor ? Iterator(m_accessor->CreateListAccessorEnumerator()) : Iterator();
}
inline ListAdapter::Iterator ListAdapter::end() const { return Iterator(); } // NOLINT(readability-convert-member-functions-to-static): container API


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
    using ExpressionCallable = std::function<InternalValue(const CallParams&, RenderContext&)>;
    using StatementCallable = std::function<void(const CallParams&, OutStream&, RenderContext&)>;

    using CallableHolder = std::variant<ExpressionCallable, StatementCallable>;

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

    [[nodiscard]] auto GetType() const
    {
        return m_callable.index() == 0 ? Type::Expression : Type::Statement;
    }

    [[nodiscard]] auto GetKind() const
    {
        return m_kind;
    }

    [[nodiscard]] auto& GetCallable() const
    {
        return m_callable;
    }

    [[nodiscard]] auto& GetExpressionCallable() const
    {
        return std::get<ExpressionCallable>(m_callable);
    }

    [[nodiscard]] auto& GetStatementCallable() const
    {
        return std::get<StatementCallable>(m_callable);
    }

    // Attributes visible through `callable.name` (macro.name, macro.arguments, ...)
    void SetAttributes(std::shared_ptr<const InternalValueMap> attributes)
    {
        m_attributes = std::move(attributes);
    }

    [[nodiscard]] const std::shared_ptr<const InternalValueMap>& GetAttributes() const
    {
        return m_attributes;
    }

private:
    Kind m_kind;
    CallableHolder m_callable;
    std::shared_ptr<const InternalValueMap> m_attributes;
};


inline bool IsEmpty(const InternalValue& val)
{
    return val.IsUndefined() || val.IsNone();
}

class RenderContext;

template<typename Fn>
auto MakeDynamicProperty(Fn&& fn)
{
    return CreateMapAdapter(InternalValueMap{
        { "value()", Callable(Callable::GlobalFunc, std::forward<Fn>(fn)) } });
}

// A "character" of a template string is a Unicode code point, as in Python: narrow strings
// are read as UTF-8, wide ones as UTF-16 or UTF-32 depending on the size of wchar_t. Malformed
// input never fails: a stray continuation unit stays with the code point before it.
inline bool IsCodePointTail(char ch)
{
    return (static_cast<unsigned char>(ch) & 0xC0) == 0x80;
}

// A code unit as a number, without sign extension: char is signed on most platforms, and
// wchar_t is signed on Linux and unsigned on Windows
inline uint32_t CodeUnit(char ch)
{
    return static_cast<unsigned char>(ch);
}

inline uint32_t CodeUnit(wchar_t ch)
{
    return static_cast<uint32_t>(static_cast<std::make_unsigned_t<wchar_t>>(ch));
}

inline bool IsCodePointTail(wchar_t ch)
{
    if constexpr (sizeof(wchar_t) != 2)
    {
        return false;
    }
    const auto unit = CodeUnit(ch);
    return unit >= 0xDC00 && unit <= 0xDFFF;
}

template<typename CharT>
size_t CodePointCount(std::basic_string_view<CharT> str)
{
    // A leading continuation unit still starts a character (see SplitCodePoints)
    auto starts = std::count_if(str.begin(), str.end(), [](CharT ch) { return !IsCodePointTail(ch); });
    return static_cast<size_t>(starts) + (!str.empty() && IsCodePointTail(str[0]) ? 1 : 0);
}

template<typename CharT>
std::vector<std::basic_string_view<CharT>> SplitCodePoints(std::basic_string_view<CharT> str)
{
    std::vector<std::basic_string_view<CharT>> result;
    size_t start = 0;
    for (size_t pos = 1; pos <= str.size(); ++pos)
    {
        if (pos == str.size() || !IsCodePointTail(str[pos]))
        {
            result.push_back(str.substr(start, pos - start));
            start = pos;
        }
    }
    return result;
}

InternalValue Subscript(const InternalValue& val, const InternalValue& subscript, RenderContext* values);
InternalValue Subscript(const InternalValue& val, const std::string& subscript, RenderContext* values);
// Python's val[start:stop:step] on lists and strings; an empty start, stop or step is omitted
InternalValue Slice(const InternalValue& val, const InternalValue& start, const InternalValue& stop, const InternalValue& step);
std::string AsString(const InternalValue& val);
ListAdapter ConvertToList(const InternalValue& val, bool& isConverted, bool strictConversion = true);
ListAdapter ConvertToList(const InternalValue& val, const InternalValue& subscipt, bool& isConverted, bool strictConversion = true);
Value IntValue2Value(const InternalValue& val);
Value OptIntValue2Value(std::optional<InternalValue> val);

} // namespace jinja2

#endif // JINJA2CPP_SRC_INTERNAL_VALUE_H
