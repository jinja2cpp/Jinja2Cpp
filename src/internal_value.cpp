#include "internal_value.h"

#include "expression_evaluator.h"
#include "generic_adapters.h"
#include "undefined.h"
#include "value_visitors.h"

#include <jinja2cpp/generic_list.h>
#include <jinja2cpp/string_helpers.h>
#include <jinja2cpp/utils/i_comparable.h>
#include <jinja2cpp/value.h>
#include <jinja2cpp/value_ptr.h>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace jinja2
{

void InternalValue::SetParentData(const InternalValue& val)
{
    m_parentData = val.IsUndefined() ? nullptr : std::make_shared<const InternalValueData>(val.GetData());
}

// NOLINTNEXTLINE(cppcoreguidelines-rvalue-reference-param-not-moved): moves val's data out
void InternalValue::SetParentData(InternalValue&& val)
{
    m_parentData = val.IsUndefined() ? nullptr : std::make_shared<const InternalValueData>(std::move(val.GetData()));
}

ListAdapter::Iterator::Iterator() = default;

void ListAdapter::Iterator::increment()
{
    m_isFinished = !m_iterator || !(*m_iterator)->MoveNext();
    ++m_currentIndex;
    m_currentVal = m_isFinished || !m_iterator ? InternalValue() : (*m_iterator)->GetCurrent();
}

bool ListAdapter::Iterator::equal(const Iterator& other) const
{
    if (!this->m_iterator)
    {
        return !other.m_iterator ? true : other.equal(*this);
    }

    if (!other.m_iterator)
    {
        return this->m_isFinished;
    }
    return (*this->m_iterator)->GetCurrent() == (*other.m_iterator)->GetCurrent() && this->m_currentIndex == other.m_currentIndex;
}

std::atomic_uint64_t UserCallable::m_gen{};

bool Value::IsEqual(const Value& rhs) const
{
    return this->m_data == rhs.m_data;
}

bool operator==(const Value& lhs, const Value& rhs)
{
    return lhs.IsEqual(rhs);
}

bool operator!=(const Value& lhs, const Value& rhs)
{
    return !(lhs == rhs);
}

namespace
{
// The accessor whose default Find is bridging to its 1.x lookup on this thread: if that
// lookup is the default too, the accessor overrides neither and the defaults would recurse
thread_local const IMapItemAccessor* g_bridgingAccessor = nullptr;

class BridgeGuard
{
public:
    explicit BridgeGuard(const IMapItemAccessor* accessor)
        : m_previous(g_bridgingAccessor)
    {
        g_bridgingAccessor = accessor;
    }
    BridgeGuard(const BridgeGuard&) = delete;
    BridgeGuard& operator=(const BridgeGuard&) = delete;
    BridgeGuard(BridgeGuard&&) = delete;
    BridgeGuard& operator=(BridgeGuard&&) = delete;
    ~BridgeGuard() { g_bridgingAccessor = m_previous; }

private:
    const IMapItemAccessor* m_previous;
};

void CheckNotBridging(const IMapItemAccessor* accessor)
{
    if (g_bridgingAccessor == accessor)
    {
        throw std::logic_error("jinja2::IMapItemAccessor: override Find(std::string_view), or HasValue and GetValueByName");
    }
}
} // namespace

std::optional<Value> IMapItemAccessor::Find(std::string_view name) const
{
    BridgeGuard guard(this);
    std::string key(name);
    if (!HasValue(key))
    {
        return std::nullopt;
    }
    return GetValueByName(key);
}

bool IMapItemAccessor::Contains(std::string_view name) const
{
    return HasValue(std::string(name));
}

bool IMapItemAccessor::HasValue(const std::string& name) const
{
    CheckNotBridging(this);
    return Find(name).has_value();
}

Value IMapItemAccessor::GetValueByName(const std::string& name) const
{
    CheckNotBridging(this);
    auto value = Find(name);
    return value ? std::move(*value) : Value();
}

bool operator==(const GenericMap& lhs, const GenericMap& rhs)
{
    const auto* lhsAccessor = lhs.GetAccessor();
    const auto* rhsAccessor = rhs.GetAccessor();
    return lhsAccessor != nullptr && rhsAccessor != nullptr && lhsAccessor->IsEqual(*rhsAccessor);
}

bool operator!=(const GenericMap& lhs, const GenericMap& rhs)
{
    return !(lhs == rhs);
}

bool operator==(const UserCallable& lhs, const UserCallable& rhs)
{
    // TODO: rework
    return lhs.IsEqual(rhs);
}

bool operator!=(const UserCallable& lhs, const UserCallable& rhs)
{
    return !(lhs == rhs);
}

bool operator==(const types::ValuePtr<UserCallable>& lhs, const types::ValuePtr<UserCallable>& rhs)
{
    return *lhs == *rhs;
}

bool operator!=(const types::ValuePtr<UserCallable>& lhs, const types::ValuePtr<UserCallable>& rhs)
{
    return !(lhs == rhs);
}

bool operator==(const types::ValuePtr<ValuesMap>& lhs, const types::ValuePtr<ValuesMap>& rhs)
{
    return *lhs == *rhs;
}

bool operator!=(const types::ValuePtr<ValuesMap>& lhs, const types::ValuePtr<ValuesMap>& rhs)
{
    return !(lhs == rhs);
}

bool operator==(const types::ValuePtr<Value>& lhs, const types::ValuePtr<Value>& rhs)
{
    return *lhs == *rhs;
}

bool operator!=(const types::ValuePtr<Value>& lhs, const types::ValuePtr<Value>& rhs)
{
    return !(lhs == rhs);
}

bool operator==(const types::ValuePtr<std::vector<Value>>& lhs, const types::ValuePtr<std::vector<Value>>& rhs)
{
    return *lhs == *rhs;
}

bool operator!=(const types::ValuePtr<std::vector<Value>>& lhs, const types::ValuePtr<std::vector<Value>>& rhs)
{
    return !(lhs == rhs);
}

bool InternalValue::IsEqual(const InternalValue& other) const
{
    if (m_data != other.m_data)
    {
        return false;
    }
    if (!m_parentData || !other.m_parentData)
    {
        return !m_parentData && !other.m_parentData;
    }
    return *m_parentData == *other.m_parentData;
}

InternalValue Value2IntValue(const Value& val);
InternalValue Value2IntValue(Value&& val);

struct SubscriptionVisitor : public visitors::BaseVisitor<>
{
    using BaseVisitor<>::operator();

    template<typename CharT>
    InternalValue operator()(const MapAdapter& values, const std::basic_string<CharT>& fieldName) const
    {
        if constexpr (std::is_same_v<CharT, char>)
        {
            return GetField(values, fieldName);
        }
        else
        {
            return GetField(values, ConvertString<std::string>(fieldName));
        }
    }

    template<typename CharT>
    InternalValue operator()(const MapAdapter& values, const std::basic_string_view<CharT>& fieldName) const
    {
        return GetField(values, ConvertString<std::string>(fieldName));
    }

    // Undefined for a missing name; a user's accessor (IMapItemAccessor) is asked through
    // Find, which a 1.x accessor answers with HasValue first, as its contract allows
    [[nodiscard]] static InternalValue GetField(const MapAdapter& values, const std::string& field) { return values.GetValueByName(field); }

    // Python indexing: a negative index counts from the end
    static bool NormalizeIndex(int64_t& index, size_t size)
    {
        if (index < 0)
        {
            index += static_cast<int64_t>(size);
        }
        return index >= 0 && static_cast<size_t>(index) < size;
    }

    InternalValue operator()(const ListAdapter& values, int64_t index) const
    {
        auto size = values.GetSize();
        if (!size || !NormalizeIndex(index, *size))
        {
            return InternalValue();
        }

        return values.GetValueByIndex(index);
    }

    InternalValue operator()(const MapAdapter& /*values*/, int64_t /*index*/) const { return InternalValue(); }

    // The fields of a namedtuple
    template<typename CharT>
    InternalValue operator()(const ListAdapter& values, const std::basic_string<CharT>& fieldName) const
    {
        return SubscriptField(values, ConvertString<std::string>(fieldName));
    }

    template<typename CharT>
    InternalValue operator()(const ListAdapter& values, const std::basic_string_view<CharT>& fieldName) const
    {
        return SubscriptField(values, ConvertString<std::string>(fieldName));
    }

    [[nodiscard]] static InternalValue SubscriptField(const ListAdapter& values, const std::string& field)
    {
        const auto* fields = values.GetFieldNames();
        if (!fields)
        {
            return InternalValue();
        }
        auto p = std::find(fields->begin(), fields->end(), field);
        if (p == fields->end())
        {
            return InternalValue();
        }
        return values.GetValueByIndex(p - fields->begin());
    }

    template<typename CharT>
    InternalValue operator()(const std::basic_string<CharT>& str, int64_t index) const
    {
        return StringItem(std::basic_string_view<CharT>(str), index);
    }

    template<typename CharT>
    InternalValue operator()(const std::basic_string_view<CharT>& str, int64_t index) const
    {
        return StringItem(str, index);
    }

    // Named apart from operator(): BaseVisitor's catch-all would take a temporary view
    template<typename CharT>
    static InternalValue StringItem(std::basic_string_view<CharT> str, int64_t index)
    {
        if (!NormalizeIndex(index, CodePointCount(str)))
        {
            return InternalValue();
        }

        // Find the index-th character without splitting the whole string
        size_t start = 0;
        for (int64_t seen = 0; seen != index; ++seen)
        {
            start = NextCodePoint(str, start);
        }
        const auto end = NextCodePoint(str, start);
        return TargetString(std::basic_string(str.substr(start, end - start)));
    }

    template<typename CharT>
    InternalValue operator()(const KeyValuePair& values, const std::basic_string<CharT>& fieldName) const
    {
        return SubscriptKvPair(values, ConvertString<std::string>(fieldName));
    }

    template<typename CharT>
    InternalValue operator()(const KeyValuePair& values, const std::basic_string_view<CharT>& fieldName) const
    {
        return SubscriptKvPair(values, ConvertString<std::string>(fieldName));
    }

    [[nodiscard]] static InternalValue SubscriptKvPair(const KeyValuePair& values, const std::string& field)
    {
        // std::cout << "operator() (const KeyValuePair& values, const std::string& field)" << ": field = " << field << std::endl;
        if (field == "key")
        {
            return InternalValue(values.key);
        }
        if (field == "value")
        {
            return values.value;
        }

        return InternalValue();
    }

    template<typename CharT>
    InternalValue operator()(const Callable& callable, const std::basic_string<CharT>& fieldName) const
    {
        return SubscriptCallable(callable, ConvertString<std::string>(fieldName));
    }

    template<typename CharT>
    InternalValue operator()(const Callable& callable, const std::basic_string_view<CharT>& fieldName) const
    {
        return SubscriptCallable(callable, ConvertString<std::string>(fieldName));
    }

    [[nodiscard]] static InternalValue SubscriptCallable(const Callable& callable, const std::string& field)
    {
        const auto& attributes = callable.GetAttributes();
        if (!attributes)
        {
            return InternalValue();
        }

        auto p = attributes->find(field);
        return p == attributes->end() ? InternalValue() : p->second;
    }
};

namespace
{
// A map with a callable "value()" item stands for the value that callable returns; it
// replaces result in place
void ResolveMapCallOperator(InternalValue& result, const MapAdapter* map, RenderContext* values)
{
    static const std::string callOperName = "value()";

    if (!map->HasValue(callOperName))
    {
        return;
    }

    auto callableVal = map->GetValueByName(callOperName);
    auto* callable = GetIf<Callable>(&callableVal);
    if (!callable || callable->GetKind() == Callable::Macro || callable->GetType() == Callable::Type::Statement)
    {
        return;
    }

    CallParams callParams;
    result = callable->GetExpressionCallable()(callParams, *values);
}

// The common case, a result that is no map, stays inline and cheap
inline void ResolveCallOperator(InternalValue& result, RenderContext* values)
{
    if (!values)
    {
        return;
    }
    if (const auto* map = GetIf<MapAdapter>(&result))
    {
        ResolveMapCallOperator(result, map, values);
    }
}
} // namespace

InternalValue Subscript(const InternalValue& val, const InternalValue& subscript, RenderContext* values)
{
    auto result = Apply2<SubscriptionVisitor>(val, subscript);
    ResolveCallOperator(result, values);
    return result;
}

InternalValue Subscript(const InternalValue& val, const std::string& subscript, RenderContext* values)
{
    // x.name of a mapping, the common case, without making the name a value first
    if (const auto* map = GetIf<MapAdapter>(&val))
    {
        auto result = SubscriptionVisitor::GetField(*map, subscript);
        ResolveCallOperator(result, values);
        return result;
    }
    return Subscript(val, InternalValue(subscript), values);
}

namespace
{
struct SliceVisitor : public visitors::BaseVisitor<>
{
    using BaseVisitor<>::operator();

    struct Indices
    {
        int64_t start = 0;
        int64_t step = 1;
        size_t count = 0;

        // Stepping past the last index could overflow with a huge step, so index directly
        [[nodiscard]] size_t At(size_t n) const { return static_cast<size_t>(start + (static_cast<int64_t>(n) * step)); }
    };

    SliceVisitor(const InternalValue& start, const InternalValue& stop, const InternalValue& step)
        : m_start(start)
        , m_stop(stop)
        , m_step(step)
    {
    }

    InternalValue operator()(const ListAdapter& values) const
    {
        auto size = values.GetSize();
        InternalValueList items;
        if (!size)
        {
            // A generator: materialise it first
            items = values.ToValueList();
            size = items.size();
        }

        Indices indices;
        if (!GetIndices(*size, indices))
        {
            return InternalValue();
        }

        InternalValueList result;
        ReserveHint(result, indices.count);
        for (size_t n = 0; n != indices.count; ++n)
        {
            const auto idx = indices.At(n);
            result.push_back(items.empty() ? values.GetValueByIndex(static_cast<int64_t>(idx)) : items[idx]);
        }

        auto list = ListAdapter::CreateAdapter(std::move(result));
        if (values.IsTuple())
        {
            list.MarkAsTuple();
        }
        return list;
    }

    template<typename CharT>
    InternalValue operator()(const std::basic_string<CharT>& str) const
    {
        return SliceString(std::basic_string_view<CharT>(str));
    }

    template<typename CharT>
    InternalValue operator()(const std::basic_string_view<CharT>& str) const
    {
        return SliceString(str);
    }

    // Strings are sliced by code point, like string indexing
    template<typename CharT>
    [[nodiscard]] InternalValue SliceString(std::basic_string_view<CharT> str) const
    {
        const auto length = CodePointCount(str);
        Indices indices;
        if (!GetIndices(length, indices))
        {
            return InternalValue();
        }

        std::basic_string<CharT> result;
        if (length == str.size())
        {
            // One unit per character (ASCII, or no surrogate pair): index the units
            if (indices.step == 1)
            {
                return TargetString(std::basic_string<CharT>(str.substr(static_cast<size_t>(indices.start), indices.count)));
            }
            result.reserve(indices.count);
            for (size_t n = 0; n != indices.count; ++n)
            {
                result.push_back(str[indices.At(n)]);
            }
            return TargetString(std::move(result));
        }

        if (indices.step == 1)
        {
            // One walk to the first character and on to the end of the last
            size_t begin = 0;
            for (int64_t n = 0; n != indices.start; ++n)
            {
                begin = NextCodePoint(str, begin);
            }
            size_t end = begin;
            for (size_t n = 0; n != indices.count; ++n)
            {
                end = NextCodePoint(str, end);
            }
            return TargetString(std::basic_string<CharT>(str.substr(begin, end - begin)));
        }

        auto chars = SplitCodePoints(str);
        for (size_t n = 0; n != indices.count; ++n)
        {
            auto ch = chars[indices.At(n)];
            result.append(ch.data(), ch.size());
        }
        return TargetString(std::move(result));
    }

    static bool GetIndex(const InternalValue& val, std::optional<int64_t>& index)
    {
        if (IsEmpty(val))
        {
            return true;
        }
        if (const auto* intVal = GetIf<int64_t>(&val))
        {
            index = *intVal;
        }
        else if (const auto* boolVal = GetIf<bool>(&val))
        {
            index = *boolVal ? 1 : 0;
        }
        else
        {
            return false;
        }
        return true;
    }

    // One slice bound clipped to [lower, upper] after counting a negative one from the end;
    // def when it is None
    static int64_t AdjustIndex(std::optional<int64_t> index, int64_t def, int64_t length, int64_t lower, int64_t upper)
    {
        if (!index)
        {
            return def;
        }
        int64_t result = *index;
        if (result < 0)
        {
            result = result < -length ? lower : result + length;
        }
        return result < lower ? lower : (result > upper ? upper : result);
    }

    // The number of items from start towards end in steps of step
    static size_t SliceLength(int64_t start, int64_t end, int64_t step)
    {
        if (step < 0)
        {
            return end < start ? static_cast<size_t>(((start - end - 1) / -step) + 1) : 0;
        }
        return start < end ? static_cast<size_t>(((end - start - 1) / step) + 1) : 0;
    }

    // CPython's PySlice_AdjustIndices
    bool GetIndices(size_t size, Indices& indices) const
    {
        std::optional<int64_t> start;
        std::optional<int64_t> stop;
        std::optional<int64_t> step;
        if (!GetIndex(m_start, start) || !GetIndex(m_stop, stop) || !GetIndex(m_step, step))
        {
            throw std::runtime_error("slice indices must be integers or None or have an __index__ method");
        }
        indices.step = step.value_or(1);
        if (indices.step == 0)
        {
            throw std::runtime_error("slice step cannot be zero");
        }
        // Any step at least as long as the sequence takes one item; this keeps -step defined
        if (indices.step < -std::numeric_limits<int64_t>::max())
        {
            indices.step = -std::numeric_limits<int64_t>::max();
        }

        const auto length = static_cast<int64_t>(size);
        const int64_t lower = indices.step < 0 ? -1 : 0;
        const int64_t upper = indices.step < 0 ? length - 1 : length;

        indices.start = AdjustIndex(start, indices.step < 0 ? upper : lower, length, lower, upper);
        const int64_t end = AdjustIndex(stop, indices.step < 0 ? lower : upper, length, lower, upper);
        indices.count = SliceLength(indices.start, end, indices.step);
        return true;
    }

    const InternalValue& m_start;
    const InternalValue& m_stop;
    const InternalValue& m_step;
};
} // namespace

InternalValue Slice(const InternalValue& val, const InternalValue& start, const InternalValue& stop, const InternalValue& step)
{
    return Apply<SliceVisitor>(val, start, stop, step);
}

struct StringGetter : public visitors::BaseVisitor<std::string>
{
    using BaseVisitor::operator();

    std::string operator()(const std::string& str) const { return str; }
    std::string operator()(const std::string_view& str) const { return std::string(str.begin(), str.end()); }
    std::string operator()(const std::wstring& str) const { return ConvertString<std::string>(str); }
    std::string operator()(const std::wstring_view& str) const { return ConvertString<std::string>(str); }
};

std::string AsString(const InternalValue& val)
{
    return Apply<StringGetter>(val);
}

struct ListConverter : public visitors::BaseVisitor<std::optional<ListAdapter>>
{
    using BaseVisitor::operator();

    using result_t = std::optional<ListAdapter>;

    bool strictConvertion;

    explicit ListConverter(bool strict)
        : strictConvertion(strict)
    {
    }

    result_t operator()(const ListAdapter& list) const { return list; }
    // Iterating undefined yields nothing, as in Python; StrictUndefined refuses
    result_t operator()(const UndefinedValue& val) const
    {
        CheckStrictUndefined(val);
        return ListAdapter::CreateAdapter(InternalValueList());
    }
    result_t operator()(const MapAdapter& map) const
    {
        if (strictConvertion)
        {
            return result_t();
        }

        InternalValueList list;
        for (auto& k : map.GetKeys())
        {
            list.emplace_back(TargetString(k));
        }

        return ListAdapter::CreateAdapter(std::move(list));
    }

    template<typename CharT>
    result_t operator()(const std::basic_string<CharT>& str) const
    {
        return FromString(std::basic_string_view<CharT>(str));
    }

    template<typename CharT>
    result_t operator()(const std::basic_string_view<CharT>& str) const
    {
        return FromString(str);
    }

    // Named apart from operator(): BaseVisitor's catch-all would take a temporary view
    template<typename CharT>
    [[nodiscard]] result_t FromString(std::basic_string_view<CharT> str) const
    {
        if (strictConvertion)
        {
            return result_t();
        }

        InternalValueList chars;
        ReserveHint(chars, CodePointCount(str));
        ForEachCodePoint(str, [&chars](auto ch) { chars.emplace_back(TargetString(std::basic_string(ch))); });
        return result_t(ListAdapter::CreateAdapter(std::move(chars)));
    }
};

ListAdapter ConvertToList(const InternalValue& val, bool& isConverted, bool strictConversion)
{
    auto result = Apply<ListConverter>(val, strictConversion);
    if (!result)
    {
        isConverted = false;
        return ListAdapter();
    }
    isConverted = true;
    return *result;
}

ListAdapter ConvertToList(const InternalValue& val, const InternalValue& subscipt, bool& isConverted, bool strictConversion)
{
    auto result = Apply<ListConverter>(val, strictConversion);
    if (!result)
    {
        isConverted = false;
        return ListAdapter();
    }
    isConverted = true;

    if (IsEmpty(subscipt))
    {
        return std::move(*result);
    }

    return result->ToSubscriptedList(subscipt, false);
}

template<typename T>
class ByRef
{
public:
    explicit ByRef(const T& val)
        : m_val(&val)
    {
    }

    [[nodiscard]] const T& Get() const { return *m_val; }
    T& Get() { return *const_cast<T*>(m_val); }
    [[nodiscard]] bool ShouldExtendLifetime() const { return false; }
    bool operator==(const ByRef<T>& other) const
    {
        if (m_val && other.m_val && m_val != other.m_val)
        {
            return false;
        }
        if ((m_val && !other.m_val) || (!m_val && other.m_val))
        {
            return false;
        }
        return true;
    }
    bool operator!=(const ByRef<T>& other) const
    {
        return !(*this == other);
    }
private:
    const T* m_val{};
};

template<typename T>
class ByVal
{
public:
    explicit ByVal(T&& val)
        : m_val(std::move(val))
    {
    }
    ~ByVal() = default;

    [[nodiscard]] const T& Get() const { return m_val; }
    T& Get() { return m_val; }
    [[nodiscard]] bool ShouldExtendLifetime() const { return false; }
    bool operator==(const ByVal<T>& other) const
    {
        return m_val == other.m_val;
    }
    bool operator!=(const ByVal<T>& other) const
    {
        return !(*this == other);
    }
private:
    T m_val;
};

template<typename T>
class BySharedVal
{
public:
    explicit BySharedVal(T&& val)
        : m_val(std::make_shared<T>(std::move(val)))
    {
    }
    explicit BySharedVal(std::shared_ptr<T> val)
        : m_val(std::move(val))
    {
    }
    ~BySharedVal() = default;

    [[nodiscard]] const T& Get() const { return *m_val; }
    T& Get() { return *m_val; }
    [[nodiscard]] const std::shared_ptr<T>& GetOwner() const { return m_val; }
    [[nodiscard]] bool ShouldExtendLifetime() const { return true; }

    bool operator==(const BySharedVal<T>& other) const
    {
        return m_val == other.m_val;
    }
    bool operator!=(const BySharedVal<T>& other) const
    {
        return !(*this == other);
    }
private:
    std::shared_ptr<T> m_val;
};

// The storage of a container the template owns: shared by every copy of the value (as
// Python containers are) and self-contained, so its items need no parent to stay alive
template<typename T>
class BySharedMutable
{
public:
    explicit BySharedMutable(T&& val)
        : m_val(std::make_shared<T>(std::move(val)))
    {
    }

    [[nodiscard]] T& Get() const { return *m_val; }
    [[nodiscard]] bool ShouldExtendLifetime() const { return false; }

    bool operator==(const BySharedMutable<T>& other) const { return *m_val == *other.m_val; }
    bool operator!=(const BySharedMutable<T>& other) const { return !(*this == other); }

private:
    std::shared_ptr<T> m_val;
};

template<template<typename> class Holder>
class GenericListAdapter : public IListAccessor
{
public:
    struct Enumerator : public IListAccessorEnumerator
    {
        std::optional<ListEnumeratorPtr> m_enum;

        explicit Enumerator(std::optional<ListEnumeratorPtr> e)
            : m_enum(std::move(e))
        {
        }

        // Inherited via IListAccessorEnumerator
        void Reset() override
        {
            if (m_enum)
            {
                (*m_enum)->Reset();
            }
        }
        bool MoveNext() override { return !m_enum ? false : (*m_enum)->MoveNext(); }
        [[nodiscard]] InternalValue GetCurrent() const override { return !m_enum ? InternalValue() : Value2IntValue((*m_enum)->GetCurrent()); }
        [[nodiscard]] std::optional<ListAccessorEnumeratorPtr> Clone() const override
        {
            return !m_enum ? std::optional<ListAccessorEnumeratorPtr>{} : std::make_optional<ListAccessorEnumeratorPtr>(types::in_place_type_t<Enumerator>{}, (*m_enum)->Clone());
        }
        std::optional<ListAccessorEnumeratorPtr> Transfer() override
        {
            return !m_enum ? std::optional<ListAccessorEnumeratorPtr>{} : std::make_optional<ListAccessorEnumeratorPtr>(types::in_place_type_t<Enumerator>{}, std::move(*m_enum));
        }
        [[nodiscard]] bool IsEqual(const IComparable& other) const override
        {
            auto* val = dynamic_cast<const Enumerator*>(&other);
            if (!val)
            {
                return false;
            }
            if (m_enum && val->m_enum && !(*m_enum)->IsEqual(**val->m_enum))
            {
                return false;
            }
            if ((m_enum && !val->m_enum) || (!m_enum && val->m_enum))
            {
                return false;
            }
            return true;
        }
    };

    // Constrained: an unconstrained U&& would take a copy of a non-const adapter
    template<typename U, typename = std::enable_if_t<!std::is_same_v<std::decay_t<U>, GenericListAdapter>>>
    explicit GenericListAdapter(U&& values)
        : m_values(std::forward<U>(values))
    {
    }

    [[nodiscard]] std::optional<size_t> GetSize() const override { return m_values.Get().GetSize(); }
    [[nodiscard]] std::optional<InternalValue> GetItem(int64_t idx) const override
    {
        const IListItemAccessor* accessor = m_values.Get().GetAccessor();
        const auto* indexer = accessor->GetIndexer();
        if (!indexer)
        {
            return std::optional<InternalValue>();
        }

        auto val = indexer->GetItemByIndex(idx);
        return visit(visitors::InputValueConvertor(true, false), std::move(val.data()));
    }
    [[nodiscard]] bool ShouldExtendLifetime() const override { return m_values.ShouldExtendLifetime(); }
    [[nodiscard]] const void* GetIdentity() const override { return m_values.Get().GetAccessor(); }
    [[nodiscard]] std::optional<ListAccessorEnumeratorPtr> CreateListAccessorEnumerator() const override
    {
        const IListItemAccessor* accessor = m_values.Get().GetAccessor();
        if (!accessor)
        {
            return {};
        }
        return ListAccessorEnumeratorPtr(Enumerator(m_values.Get().GetAccessor()->CreateEnumerator()));
    }
    [[nodiscard]] GenericList CreateGenericList() const override
    {
        // return m_values.Get();
        return GenericList([list = m_values]() -> const IListItemAccessor* { return list.Get().GetAccessor(); });
    }

private:
    Holder<GenericList> m_values;
};

ListAdapter LendNestedList(std::shared_ptr<ValuesList> list);
MapAdapter LendNestedMap(std::shared_ptr<ValuesMap> map);

// An item of user data held by a value (a user callable's result) rather than by the caller
// for the whole render: nothing keeps the container alive once the item escapes it (a loop
// target stored in a namespace, a sorted copy), so strings are copied and nested containers
// share the ownership of the root
template<typename T>
InternalValue LendOwnedItem(const std::shared_ptr<T>& owner, const Value& item)
{
    if (const auto* list = std::get_if<RecWrapper<ValuesList>>(&item.data()))
    {
        return InternalValue(LendNestedList(std::shared_ptr<ValuesList>(owner, const_cast<ValuesList*>(&**list))));
    }
    if (const auto* map = std::get_if<RecWrapper<ValuesMap>>(&item.data()))
    {
        return InternalValue(LendNestedMap(std::shared_ptr<ValuesMap>(owner, const_cast<ValuesMap*>(&**map))));
    }
    return visit(visitors::InputValueConvertor(true, false), item.data());
}

// Borrows the item when the caller owns the storage for the whole render (ByRef)
template<template<typename> class Holder, typename T>
InternalValue LendItem([[maybe_unused]] const Holder<T>& holder, const Value& item)
{
    if constexpr (std::is_same_v<Holder<T>, BySharedVal<T>>)
    {
        return LendOwnedItem(holder.GetOwner(), item);
    }
    else
    {
        // Scalars, the usual items of user data, skip the convertor's visit
        if (const auto* s = std::get_if<std::string>(&item.data()))
        {
            return InternalValue(TargetStringView(std::string_view(*s)));
        }
        if (const auto* i = std::get_if<int64_t>(&item.data()))
        {
            return InternalValue(*i);
        }
        return Value2IntValue(item);
    }
}

template<template<typename> class Holder>
class ValuesListAdapter final : public IndexedListAccessorImpl<ValuesListAdapter<Holder>>
{
public:
    // Constrained: an unconstrained U&& would take a copy of a non-const adapter
    template<typename U, typename = std::enable_if_t<!std::is_same_v<std::decay_t<U>, ValuesListAdapter>>>
    explicit ValuesListAdapter(U&& values)
        : m_values(std::forward<U>(values))
    {
    }

    [[nodiscard]] size_t GetItemsCountImpl() const { return m_values.Get().size(); }
    [[nodiscard]] std::optional<InternalValue> GetItem(int64_t idx) const override
    {
        return GetCurrentItem(idx);
    }
    [[nodiscard]] InternalValue GetCurrentItem(int64_t idx) const
    {
        return LendItem(m_values, m_values.Get()[static_cast<size_t>(idx)]);
    }
    [[nodiscard]] bool ShouldExtendLifetime() const override { return m_values.ShouldExtendLifetime(); }
    [[nodiscard]] const void* GetIdentity() const override { return &m_values.Get(); }
    [[nodiscard]] GenericList CreateGenericList() const override
    {
        // return m_values.Get();
        return GenericList([list = *this]() -> const IListItemAccessor* { return &list; });
    }

private:
    Holder<ValuesList> m_values;
};

ListAdapter ListAdapter::CreateAdapter(InternalValueList&& values)
{
    // The items are shared by every copy of the list, as a Python list is shared by its
    // names: an append() through one is seen through all, and `a is sameas b` holds
    class Adapter final : public IndexedListAccessorImpl<Adapter>
    {
    public:
        explicit Adapter(InternalValueList&& values)
            : m_values(std::make_shared<InternalValueList>(std::move(values)))
        {
        }

        [[nodiscard]] size_t GetItemsCountImpl() const { return m_values->size(); }
        [[nodiscard]] std::optional<InternalValue> GetItem(int64_t idx) const override
        {
            // A list can shrink while it is iterated (pop() in a loop body)
            if (idx < 0 || static_cast<size_t>(idx) >= m_values->size())
            {
                return std::optional<InternalValue>();
            }
            return (*m_values)[static_cast<size_t>(idx)];
        }
        [[nodiscard]] InternalValue GetCurrentItem(int64_t idx) const
        {
            if (idx < 0 || static_cast<size_t>(idx) >= m_values->size())
            {
                return InternalValue();
            }
            return (*m_values)[static_cast<size_t>(idx)];
        }
        [[nodiscard]] bool ShouldExtendLifetime() const override { return false; }
        [[nodiscard]] const void* GetIdentity() const override { return m_values.get(); }
        [[nodiscard]] InternalValueList* GetMutableItems() const override { return m_values.get(); }
        [[nodiscard]] GenericList CreateGenericList() const override
        {
            return GenericList([adapter = *this]() -> const IListItemAccessor* { return &adapter; });
        }

    private:
        std::shared_ptr<InternalValueList> m_values;
    };

    return ListAdapter(std::make_shared<Adapter>(std::move(values)));
}

ListAdapter ListAdapter::CreateAdapter(const GenericList& values)
{
    return ListAdapter(std::make_shared<GenericListAdapter<ByRef>>(values));
}

ListAdapter ListAdapter::CreateAdapter(const ValuesList& values)
{
    return ListAdapter(std::make_shared<ValuesListAdapter<ByRef>>(values));
}

ListAdapter ListAdapter::CreateAdapter(GenericList&& values)
{
    return ListAdapter(std::make_shared<GenericListAdapter<BySharedVal>>(std::move(values)));
}

ListAdapter ListAdapter::CreateAdapter(ValuesList&& values)
{
    return ListAdapter(std::make_shared<ValuesListAdapter<BySharedVal>>(std::move(values)));
}

ListAdapter ListAdapter::CreateAdapter(std::function<std::optional<InternalValue>()> fn)
{
    using GenFn = std::function<std::optional<InternalValue>()>;

    class Adapter : public IListAccessor
    {
    public:
        class Enumerator : public IListAccessorEnumerator
        {
        public:
            explicit Enumerator(const GenFn* fn)
                : m_fn(fn)
            {
                if (!fn)
                {
                    throw std::runtime_error("List enumerator couldn't be created without element accessor function!");
                }
            }

            Enumerator(const Enumerator& other)
                : m_fn(other.m_fn)
                , m_current(other.m_current)
                , m_isFinished(other.m_isFinished)
            {}

            Enumerator(Enumerator&& other) noexcept
                : m_fn(other.m_fn)
                , m_current(std::move(other.m_current))
                , m_isFinished(other.m_isFinished)
            {}

            ~Enumerator() override = default;
            Enumerator& operator=(const Enumerator&) = delete;
            Enumerator& operator=(Enumerator&&) = delete;

            void Reset() override {}

            bool MoveNext() override
            {
                if (m_isFinished)
                {
                    return false;
                }

                auto res = (*m_fn)();
                if (!res)
                {
                    return false;
                }

                m_current = *res;

                return true;
            }

            [[nodiscard]] InternalValue GetCurrent() const override { return m_current; }

            [[nodiscard]] std::optional<ListAccessorEnumeratorPtr> Clone() const override
            {
                return std::make_optional<ListAccessorEnumeratorPtr>(types::in_place_type_t<Enumerator>{}, *this);
            }

            std::optional<ListAccessorEnumeratorPtr> Transfer() override
            {
                return std::make_optional<ListAccessorEnumeratorPtr>(types::in_place_type_t<Enumerator>{}, std::move(*this));
            }

            [[nodiscard]] bool IsEqual(const IComparable& other) const override
            {
                const auto* val = dynamic_cast<const Enumerator*>(&other);
                if (!val)
                {
                    return false;
                }
                if (m_isFinished != val->m_isFinished)
                {
                    return false;
                }
                if (m_current != val->m_current)
                {
                    return false;
                }
                // TODO: compare fn?
                if (m_fn != val->m_fn)
                {
                    return false;
                }
                return true;
            }

        protected:
            const GenFn* m_fn{};
            InternalValue m_current;
            bool m_isFinished = false;
        };

        explicit Adapter(std::function<std::optional<InternalValue>()>&& fn)
            : m_fn(std::move(fn))
        {
        }

        [[nodiscard]] std::optional<size_t> GetSize() const override { return std::optional<size_t>(); }
        [[nodiscard]] std::optional<InternalValue> GetItem(int64_t /*idx*/) const override { return std::optional<InternalValue>(); }
        [[nodiscard]] bool ShouldExtendLifetime() const override { return false; }
        [[nodiscard]] std::optional<ListAccessorEnumeratorPtr> CreateListAccessorEnumerator() const override { return ListAccessorEnumeratorPtr(types::in_place_type_t<Enumerator>{}, Enumerator(&m_fn)); }

        [[nodiscard]] GenericList CreateGenericList() const override
        {
            return GenericList(); //  return GenericList([adapter = *this]() -> const ListItemAccessor* {return &adapter; });
        }
        // The generator function carries the iteration state: each copy of the list value
        // gets its own, as each copy of the old std::function-held accessor did
        [[nodiscard]] bool ClonesOnCopy() const override { return true; }
        [[nodiscard]] std::shared_ptr<const IListAccessor> Clone() const override { return std::make_shared<Adapter>(*this); }

    private:
        std::function<std::optional<InternalValue>()> m_fn;
    };

    return ListAdapter(std::make_shared<Adapter>(std::move(fn)));
}

ListAdapter ListAdapter::CreateAdapter(size_t listSize, std::function<InternalValue(size_t idx)> fn)
{
    using GenFn = std::function<InternalValue(size_t idx)>;

    class Adapter final : public IndexedListAccessorImpl<Adapter>
    {
    public:
        explicit Adapter(size_t listSize, GenFn&& fn)
            : m_listSize(listSize)
            , m_fn(std::move(fn))
        {
        }

        [[nodiscard]] size_t GetItemsCountImpl() const { return m_listSize; }
        [[nodiscard]] std::optional<InternalValue> GetItem(int64_t idx) const override { return m_fn(static_cast<size_t>(idx)); }
        [[nodiscard]] InternalValue GetCurrentItem(int64_t idx) const { return m_fn(static_cast<size_t>(idx)); }
        [[nodiscard]] bool ShouldExtendLifetime() const override { return false; }
        [[nodiscard]] GenericList CreateGenericList() const override
        {
            return GenericList([adapter = *this]() -> const IListItemAccessor* { return &adapter; });
        }

    private:
        size_t m_listSize;
        GenFn m_fn;
    };

    return ListAdapter(std::make_shared<Adapter>(listSize, std::move(fn)));
}

ListAdapter ListAdapter::CreateRange(int64_t start, int64_t stop, int64_t step)
{
    class Adapter final : public IndexedListAccessorImpl<Adapter>
    {
    public:
        explicit Adapter(RangeInfo info)
            : m_info(info)
        {
            // Unsigned arithmetic: stop - start overflows int64_t for the widest ranges
            auto distance = [](int64_t from, int64_t to) { return static_cast<uint64_t>(to) - static_cast<uint64_t>(from); };
            if (info.step > 0 && info.start < info.stop)
            {
                m_size = ((distance(info.start, info.stop) - 1) / static_cast<uint64_t>(info.step)) + 1;
            }
            else if (info.step < 0 && info.start > info.stop)
            {
                m_size = ((distance(info.stop, info.start) - 1) / (0 - static_cast<uint64_t>(info.step))) + 1;
            }
            // Python raises OverflowError for len() of such a range; lengths here are int64_t
            if (m_size > static_cast<uint64_t>(std::numeric_limits<int64_t>::max()))
            {
                throw std::runtime_error("range() has more items than fit in a 64-bit integer");
            }
        }

        [[nodiscard]] size_t GetItemsCountImpl() const { return static_cast<size_t>(m_size); }
        [[nodiscard]] std::optional<InternalValue> GetItem(int64_t idx) const override { return GetCurrentItem(idx); }
        [[nodiscard]] InternalValue GetCurrentItem(int64_t idx) const
        {
            auto value = static_cast<uint64_t>(m_info.start) + (static_cast<uint64_t>(m_info.step) * static_cast<uint64_t>(idx));
            return InternalValue(static_cast<int64_t>(value));
        }
        [[nodiscard]] bool ShouldExtendLifetime() const override { return false; }
        [[nodiscard]] const RangeInfo* GetRangeInfo() const override { return &m_info; }
        [[nodiscard]] GenericList CreateGenericList() const override
        {
            return GenericList([adapter = *this]() -> const IListItemAccessor* { return &adapter; });
        }

    private:
        RangeInfo m_info;
        uint64_t m_size = 0;
    };

    return ListAdapter(std::make_shared<Adapter>(RangeInfo{ start, stop, step }));
}

template<typename Holder>
auto CreateIndexedSubscribedList(Holder&& holder, const InternalValue& subscript, size_t size)
{
    return ListAdapter::CreateAdapter(
        size, [h = std::forward<Holder>(holder), subscript](size_t idx) -> InternalValue { return Subscript(h.Get().GetValueByIndex(static_cast<int64_t>(idx)), subscript, nullptr); });
}

template<typename Holder>
auto CreateGenericSubscribedList(Holder&& holder, const InternalValue& subscript)
{
    return ListAdapter::CreateAdapter([h = std::forward<Holder>(holder), e = std::optional<ListAccessorEnumeratorPtr>(), isFirst = true, isLast = false, subscript]() mutable {
        using ResultType = std::optional<InternalValue>;
        if (isFirst)
        {
            e = h.Get().GetEnumerator();
            isFirst = false;
        }
        // Advance before every item, not only the first: the source is walked once
        if (isLast || !(*e)->MoveNext())
        {
            isLast = true;
            return ResultType();
        }

        return ResultType(Subscript((*e)->GetCurrent(), subscript, nullptr));
    });
}

ListAdapter ListAdapter::ToSubscriptedList(const InternalValue& subscript, bool asRef) const
{
    auto listSize = GetSize();
    if (asRef)
    {
        ByRef<ListAdapter> holder(*this);
        return listSize ? CreateIndexedSubscribedList(holder, subscript, *listSize) : CreateGenericSubscribedList(holder, subscript);
    }

    ListAdapter tmp(*this);
    BySharedVal<ListAdapter> holder(std::move(tmp));
    return listSize ? CreateIndexedSubscribedList(std::move(holder), subscript, *listSize) : CreateGenericSubscribedList(std::move(holder), subscript);
}

InternalValueList ListAdapter::ToValueList() const
{
    InternalValueList result;
    if (!m_accessor)
    {
        return result;
    }
    if (auto size = m_accessor->GetSize())
    {
        ReserveHint(result, *size);
    }
    m_accessor->ForEach([&result](InternalValue&& item) {
        result.push_back(std::move(item));
        return true;
    });
    return result;
}

void ListAdapter::ForEach(IListAccessor::ItemVisitor fn) const
{
    if (m_accessor)
    {
        m_accessor->ForEach(fn);
    }
}

void IListAccessor::ForEach(ItemVisitor fn) const
{
    auto enumerator = CreateListAccessorEnumerator();
    if (!enumerator)
    {
        return;
    }
    while ((*enumerator)->MoveNext())
    {
        if (!fn((*enumerator)->GetCurrent()))
        {
            return;
        }
    }
}

std::vector<KeyValuePair> IMapAccessor::GetEntries() const
{
    std::vector<KeyValuePair> result;
    auto keys = GetKeys();
    result.reserve(keys.size());
    for (auto& key : keys)
    {
        auto value = GetItem(key);
        result.push_back(KeyValuePair{ std::move(key), std::move(value) });
    }

    return result;
}

template<template<typename> class Holder, bool canModify, typename Map = InternalValueMap>
class InternalValueMapAdapter : public MapAccessorImpl<InternalValueMapAdapter<Holder, canModify, Map>>
{
public:
    // Constrained: an unconstrained U&& would take a copy of a non-const adapter
    template<typename U, typename = std::enable_if_t<!std::is_same_v<std::decay_t<U>, InternalValueMapAdapter>>>
    explicit InternalValueMapAdapter(U&& values)
        : m_values(std::forward<U>(values))
    {
    }

    [[nodiscard]] size_t GetSize() const override { return m_values.Get().size(); }
    [[nodiscard]] bool HasValue(const std::string& name) const override { return m_values.Get().count(name) != 0; }
    [[nodiscard]] InternalValue GetItem(const std::string& name) const override
    {
        auto& vals = m_values.Get();
        auto p = vals.find(name);
        if (p == vals.end())
        {
            return InternalValue();
        }

        return p->second;
    }
    [[nodiscard]] std::vector<std::string> GetKeys() const override
    {
        std::vector<std::string> result;

        for (const auto& [key, value] : m_values.Get())
        {
            result.push_back(key);
        }

        return result;
    }
    [[nodiscard]] std::vector<KeyValuePair> GetEntries() const override
    {
        std::vector<KeyValuePair> result;
        result.reserve(m_values.Get().size());

        for (const auto& [key, value] : m_values.Get())
        {
            result.push_back(KeyValuePair{ key, value });
        }

        return result;
    }

    bool SetValue(std::string name, const InternalValue& val) override
    {
        if (canModify)
        {
            m_values.Get()[name] = val;
            return true;
        }
        return false;
    }
    [[nodiscard]] bool ShouldExtendLifetime() const override { return m_values.ShouldExtendLifetime(); }
    [[nodiscard]] GenericMap CreateGenericMap() const override
    {
        return GenericMap([accessor = *this]() -> const IMapItemAccessor* { return &accessor; });
    }
    [[nodiscard]] bool IsEqual(const IComparable& other) const override
    {
        auto* val = dynamic_cast<const InternalValueMapAdapter*>(&other);
        if (!val)
        {
            return false;
        }
        return m_values == val->m_values;
    }
protected:
    Holder<Map> m_values;
};

// A dict the template owns (dict literals, kwargs, dict()): shared like a Python dict
class SharedDictAdapter : public InternalValueMapAdapter<BySharedMutable, true, InternalDict>
{
public:
    using InternalValueMapAdapter::InternalValueMapAdapter;

    [[nodiscard]] const void* GetIdentity() const override { return &m_values.Get(); }
    [[nodiscard]] InternalDict* GetMutableItems() const override { return &m_values.Get(); }
    [[nodiscard]] MapAttrPolicy GetAttrPolicy() const override { return MapAttrPolicy::MethodsFirst; }
    [[nodiscard]] GenericMap CreateGenericMap() const override
    {
        return GenericMap([accessor = *this]() -> const IMapItemAccessor* { return &accessor; });
    }
};

// namespace(): shared like a dict, but an object with attributes rather than a dict to the
// template, so it exposes no dict methods
class NamespaceAdapter : public SharedDictAdapter
{
public:
    using SharedDictAdapter::SharedDictAdapter;

    [[nodiscard]] MapAttrPolicy GetAttrPolicy() const override { return MapAttrPolicy::KeysOnly; }
    [[nodiscard]] bool IsNamespace() const override { return true; }
    [[nodiscard]] GenericMap CreateGenericMap() const override
    {
        return GenericMap([accessor = *this]() -> const IMapItemAccessor* { return &accessor; });
    }
};

InternalValue Value2IntValue(const Value& val)
{
    return std::visit(visitors::InputValueConvertor(false, true), val.data());
}

// NOLINTNEXTLINE(cppcoreguidelines-rvalue-reference-param-not-moved): the convertor moves out of val.data()
InternalValue Value2IntValue(Value&& val)
{
    return std::visit(visitors::InputValueConvertor(true, false), val.data());
}

template<template<typename> class Holder>
class GenericMapAdapter : public MapAccessorImpl<GenericMapAdapter<Holder>>
{
public:
    // Constrained: an unconstrained U&& would take a copy of a non-const adapter
    template<typename U, typename = std::enable_if_t<!std::is_same_v<std::decay_t<U>, GenericMapAdapter>>>
    explicit GenericMapAdapter(U&& values)
        : m_values(std::forward<U>(values))
    {
    }

    [[nodiscard]] size_t GetSize() const override { return m_values.Get().GetSize(); }
    [[nodiscard]] bool HasValue(const std::string& name) const override { return m_values.Get().HasValue(name); }
    // One lookup; an absent item and an empty Value both read as Undefined
    [[nodiscard]] InternalValue GetItem(const std::string& name) const override
    {
        auto val = m_values.Get().Find(name);
        if (!val || val->isEmpty())
        {
            return InternalValue();
        }

        return Value2IntValue(std::move(*val));
    }
    [[nodiscard]] std::vector<std::string> GetKeys() const override { return m_values.Get().GetKeys(); }
    [[nodiscard]] bool ShouldExtendLifetime() const override { return m_values.ShouldExtendLifetime(); }
    [[nodiscard]] bool HasAttributes() const override { return true; }
    [[nodiscard]] const void* GetIdentity() const override { return m_values.Get().GetAccessor(); }
    [[nodiscard]] MapAttrPolicy GetAttrPolicy() const override { return MapAttrPolicy::KeysFirst; }
    [[nodiscard]] GenericMap CreateGenericMap() const override
    {
        return GenericMap([accessor = *this]() -> const IMapItemAccessor* { return accessor.m_values.Get().GetAccessor(); });
    }
    [[nodiscard]] bool IsEqual(const IComparable& other) const override
    {
        auto* val = dynamic_cast<const GenericMapAdapter*>(&other);
        if (!val)
        {
            return false;
        }
        return m_values == val->m_values;
    }
private:
    Holder<GenericMap> m_values;
};

template<template<typename> class Holder>
class ValuesMapAdapter : public MapAccessorImpl<ValuesMapAdapter<Holder>>
{
public:
    // Constrained: an unconstrained U&& would take a copy of a non-const adapter
    template<typename U, typename = std::enable_if_t<!std::is_same_v<std::decay_t<U>, ValuesMapAdapter>>>
    explicit ValuesMapAdapter(U&& values)
        : m_values(std::forward<U>(values))
    {
    }

    [[nodiscard]] size_t GetSize() const override { return m_values.Get().size(); }
    [[nodiscard]] bool HasValue(const std::string& name) const override { return m_values.Get().count(name) != 0; }
    [[nodiscard]] InternalValue GetItem(const std::string& name) const override
    {
        auto& vals = m_values.Get();
        auto p = vals.find(name);
        if (p == vals.end())
        {
            return InternalValue();
        }

        return LendItem(m_values, p->second);
    }
    [[nodiscard]] std::vector<std::string> GetKeys() const override
    {
        std::vector<std::string> result;

        for (const auto& [key, value] : m_values.Get())
        {
            result.push_back(key);
        }

        return result;
    }
    [[nodiscard]] std::vector<KeyValuePair> GetEntries() const override
    {
        std::vector<KeyValuePair> result;
        result.reserve(m_values.Get().size());

        for (const auto& [key, value] : m_values.Get())
        {
            result.push_back(KeyValuePair{ key, LendItem(m_values, value) });
        }

        return result;
    }
    // A mapping passed in the context is a Python dict to the template
    [[nodiscard]] MapAttrPolicy GetAttrPolicy() const override { return MapAttrPolicy::MethodsFirst; }
    [[nodiscard]] bool ShouldExtendLifetime() const override { return m_values.ShouldExtendLifetime(); }
    [[nodiscard]] const void* GetIdentity() const override { return &m_values.Get(); }
    [[nodiscard]] GenericMap CreateGenericMap() const override
    {
        return GenericMap([accessor = *this]() -> const IMapItemAccessor* { return &accessor; });
    }
    [[nodiscard]] bool IsEqual(const IComparable& other) const override
    {
        auto* val = dynamic_cast<const ValuesMapAdapter*>(&other);
        if (!val)
        {
            return false;
        }
        return m_values == val->m_values;
    }
private:
    Holder<ValuesMap> m_values;
};

ListAdapter LendNestedList(std::shared_ptr<ValuesList> list)
{
    return ListAdapter(std::make_shared<ValuesListAdapter<BySharedVal>>(BySharedVal<ValuesList>(std::move(list))));
}

MapAdapter LendNestedMap(std::shared_ptr<ValuesMap> map)
{
    return MapAdapter(std::make_shared<ValuesMapAdapter<BySharedVal>>(BySharedVal<ValuesMap>(std::move(map))));
}

MapAdapter CreateMapAdapter(InternalValueMap&& values)
{
    return MapAdapter(std::make_shared<InternalValueMapAdapter<ByVal, true>>(std::move(values)));
}

MapAdapter CreateMapAdapter(InternalDict&& values)
{
    return MapAdapter(std::make_shared<SharedDictAdapter>(std::move(values)));
}

MapAdapter CreateNamespaceAdapter(InternalDict&& values)
{
    return MapAdapter(std::make_shared<NamespaceAdapter>(std::move(values)));
}

MapAdapter CreateMapAdapter(const InternalValueMap* values)
{
    return MapAdapter(std::make_shared<InternalValueMapAdapter<ByRef, false>>(*values));
}

MapAdapter CreateMapAdapter(std::shared_ptr<InternalValueMap> values)
{
    return MapAdapter(std::make_shared<InternalValueMapAdapter<BySharedVal, false>>(std::move(values)));
}

MapAdapter CreateMapAdapter(const GenericMap& values)
{
    return MapAdapter(std::make_shared<GenericMapAdapter<ByRef>>(values));
}

MapAdapter CreateMapAdapter(GenericMap&& values)
{
    return MapAdapter(std::make_shared<GenericMapAdapter<BySharedVal>>(std::move(values)));
}

MapAdapter CreateMapAdapter(const ValuesMap& values)
{
    return MapAdapter(std::make_shared<ValuesMapAdapter<ByRef>>(values));
}

MapAdapter CreateMapAdapter(ValuesMap&& values)
{
    return MapAdapter(std::make_shared<ValuesMapAdapter<BySharedVal>>(std::move(values)));
}

struct OutputValueConvertor
{
    using result_t = Value;

    result_t operator()(const UndefinedValue&) const { return result_t(); }
    result_t operator()(const EmptyValue&) const { return result_t(); }
    result_t operator()(const MapAdapter& adapter) const { return result_t(adapter.CreateGenericMap()); }
    result_t operator()(const ListAdapter& adapter) const { return result_t(adapter.CreateGenericList()); }
    result_t operator()(const ValueRef& ref) const { return ref.get(); }
    result_t operator()(const TargetString& str) const
    {
        switch (str.index())
        {
        case 0:
            return std::get<std::string>(str);
        default:
            return std::get<std::wstring>(str);
        }
    }
    result_t operator()(const TargetStringView& str) const
    {
        switch (str.index())
        {
        case 0:
            return std::get<std::string_view>(str);
        default:
            return std::get<std::wstring_view>(str);
        }
    }
    result_t operator()(const KeyValuePair& pair) const { return ValuesMap{ { "key", Value(pair.key) }, { "value", IntValue2Value(pair.value) } }; }
    result_t operator()(const Callable&) const { return result_t(); }
    result_t operator()(const UserCallable&) const { return result_t(); }
    result_t operator()(const std::shared_ptr<IRendererBase>&) const { return result_t(); }

    template<typename T>
    result_t operator()(const RecWrapper<T>& val) const
    {
        return this->operator()(const_cast<const T&>(*val));
    }

    template<typename T>
    result_t operator()(RecWrapper<T>& val) const
    {
        return this->operator()(*val);
    }

    template<typename T>
    result_t operator()(T&& val) const
    {
        return result_t(std::forward<T>(val));
    }

    bool m_byValue;
};

Value OptIntValue2Value(std::optional<InternalValue> val)
{
    if (val)
    {
        return Apply<OutputValueConvertor>(val.value());
    }

    return Value();
}

Value IntValue2Value(const InternalValue& val)
{
    return Apply<OutputValueConvertor>(val);
}

class ContextMapper : public IMapItemAccessor
{
public:
    explicit ContextMapper(RenderContext* context)
        : m_context(context)
    {
    }

    [[nodiscard]] size_t GetSize() const override { return std::numeric_limits<size_t>::max(); }
    [[nodiscard]] bool HasValue(const std::string& name) const override
    {
        return static_cast<bool>(m_context->FindValue(name));
    }
    [[nodiscard]] Value GetValueByName(const std::string& name) const override
    {
        const auto value = m_context->FindValue(name);
        return value ? IntValue2Value(*value) : Value();
    }
    [[nodiscard]] std::vector<std::string> GetKeys() const override { return std::vector<std::string>(); }

    [[nodiscard]] bool IsEqual(const IComparable& other) const override
    {
        const auto* val = dynamic_cast<const ContextMapper*>(&other);
        if (!val)
        {
            return false;
        }
        if (m_context && val->m_context && !m_context->IsEqual(*val->m_context))
        {
            return false;
        }
        if ((m_context && !val->m_context) || (!m_context && val->m_context))
        {
            return false;
        }
        return true;
    }

private:
    RenderContext* m_context;
};

UserCallableParams PrepareUserCallableParams(const CallParams& params, RenderContext& context, const std::vector<ArgumentInfo>& argsInfo)
{
    UserCallableParams result;

    ParsedArguments args = helpers::ParseCallParams(argsInfo, params, result.paramsParsed);
    if (!result.paramsParsed)
    {
        return result;
    }

    for (const auto& argInfo : argsInfo)
    {
        if (argInfo.name.size() > 1 && argInfo.name[0] == '*')
        {
            continue;
        }

        auto p = args.args.find(argInfo.name);
        if (p == args.args.end())
        {
            result.args[argInfo.name] = IntValue2Value(argInfo.defaultVal);
            continue;
        }

        const auto& v = p->second;
        result.args[argInfo.name] = IntValue2Value(v);
    }

    ValuesMap extraKwArgs;
    for (auto& [name, value] : args.extraKwArgs)
    {
        extraKwArgs[name] = IntValue2Value(value);
    }
    result.extraKwArgs = Value(std::move(extraKwArgs));

    ValuesList extraPosArgs;
    for (auto& p : args.extraPosArgs)
    {
        extraPosArgs.push_back(IntValue2Value(p));
    }
    result.extraPosArgs = Value(std::move(extraPosArgs));
    result.context = GenericMap([accessor = ContextMapper(&context)]() -> const IMapItemAccessor* { return &accessor; });

    return result;
}

namespace visitors
{

InputValueConvertor::result_t InputValueConvertor::ConvertUserCallable(const UserCallable& val)
{
    std::vector<ArgumentInfo> args;
    args.reserve(val.argsInfo.size());
    for (const auto& pi : val.argsInfo)
    {
        // By value: the default must not refer to val, which may not outlive the callable made here
        args.emplace_back(pi.paramName, pi.isMandatory, Value2IntValue(Value(pi.defValue)));
    }

    return InternalValue(Callable(Callable::UserCallable, [val, argsInfo = std::move(args)](const CallParams& params, RenderContext& context) -> InternalValue {
        auto ucParams = PrepareUserCallableParams(params, context, argsInfo);
        return Value2IntValue(val.callable(ucParams));
    }));
}

} // namespace visitors

} // namespace jinja2
