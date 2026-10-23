#include "internal_value.h"

#include "engine_errors.h"
#include "expression_evaluator.h"
#include "generic_adapters.h"
#include "recursion_guard.h"
#include "render_failure.h"
#include "template_dict.h"
#include "undefined.h"
#include "value_visitors.h"

#include <jinja2cpp/exception.h>
#include <jinja2cpp/generic_list.h>
#include <jinja2cpp/markup.h>
#include <jinja2cpp/string_helpers.h>
#include <jinja2cpp/utils/i_comparable.h>
#include <jinja2cpp/value.h>
#include <jinja2cpp/value_ptr.h>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <functional>
#include <limits>
#include <memory>
#include <new>
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
inline namespace v2
{
void ThrowSequenceTooLong(uint64_t size, const char* what)
{
    // Past the built-in cap, a render with a maxSequenceSize of its own reports its limit too
    if (size > MaxSequenceSize && detail::t_renderLimits.maxSequenceSize == MaxSequenceSize)
    {
        throw ResourceLimitError(std::string(what) + " of length " + std::to_string(size) + " is too long");
    }
    throw ResourceLimitError("maxSequenceSize",
                             std::string(what) + " of length " + std::to_string(size) + " is longer than " + std::to_string(detail::t_renderLimits.maxSequenceSize));
}


namespace detail
{
namespace
{
// Nested deletions past this depth wait on the thread's pending list. A level is an
// object's destructor and its container's: some hundred bytes of stack, more under ASan
constexpr unsigned MaxNestedDestroy = 64;

// The objects waiting to be deleted. Trivially destructible, so values that other
// thread_local objects release when the thread ends still find it
struct PendingDestroy
{
    unsigned depth;
    std::size_t size;
    std::size_t capacity;
    const ValueObject** items;
};
thread_local PendingDestroy t_pendingDestroy{};

// False when the list cannot grow: the caller deletes the object at once
bool DeferDestroy(PendingDestroy& pending, const ValueObject* obj) noexcept
{
    if (pending.size == pending.capacity)
    {
        const std::size_t capacity = pending.capacity == 0 ? 256 : pending.capacity * 2;
        // NOLINTNEXTLINE(cppcoreguidelines-no-malloc,cppcoreguidelines-owning-memory): no destructor, see PendingDestroy
        void* items = std::realloc(static_cast<void*>(pending.items), capacity * sizeof(*pending.items));
        if (!items)
        {
            return false;
        }
        pending.items = static_cast<const ValueObject**>(items);
        pending.capacity = capacity;
    }
    pending.items[pending.size++] = obj; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return true;
}
} // namespace

void DestroyValueObject(const ValueObject* obj) noexcept
{
    auto& pending = t_pendingDestroy;
    if (pending.depth >= MaxNestedDestroy && DeferDestroy(pending, obj))
    {
        return;
    }
    ++pending.depth;
    delete obj; // NOLINT(cppcoreguidelines-owning-memory): its last reference went
    if (pending.depth == 1 && pending.items)
    {
        // Each of these may defer more of its own
        while (pending.size != 0)
        {
            delete pending.items[--pending.size]; // NOLINT(cppcoreguidelines-owning-memory,cppcoreguidelines-pro-bounds-pointer-arithmetic)
        }
        std::free(static_cast<void*>(pending.items)); // NOLINT(cppcoreguidelines-no-malloc,cppcoreguidelines-owning-memory)
        pending.items = nullptr;
        pending.capacity = 0;
    }
    --pending.depth;
}
} // namespace detail

namespace
{
// What SetParentData makes: the value's own object and the value it was taken from, kept
// alive together. The value keeps pointing into its own object, so readers never see this
class Anchor final : public ValueObject
{
public:
    Anchor(ValueObjectPtr inner, InternalValue parent)
        : m_inner(std::move(inner))
        , m_parent(std::move(parent))
    {
        SetAnchor();
        SetClonesOnCopy(m_inner && m_inner->ClonesOnCopy());
    }

    [[nodiscard]] const ValueObjectPtr& Inner() const noexcept { return m_inner; }
    [[nodiscard]] const InternalValue& Parent() const noexcept { return m_parent; }

private:
    ValueObjectPtr m_inner;
    InternalValue m_parent;
};

const Anchor* AsAnchor(const ValueObject* obj)
{
    return obj && obj->IsAnchor() ? static_cast<const Anchor*>(obj) : nullptr;
}

// The object of a value without the anchor SetParentData gave it
ValueObjectPtr OwnObject(const ValueObjectPtr& obj)
{
    const auto* anchor = AsAnchor(obj.get());
    return anchor ? anchor->Inner() : obj;
}
} // namespace

void InternalValue::SetParentData(const InternalValue& val)
{
    ValueObjectPtr inner = OwnObject(m_obj);
    if (val.IsUndefined())
    {
        m_obj = std::move(inner);
        return;
    }
    // The parent is kept without its own parent, as the chain a.b.c needs only the last link
    InternalValue parent;
    parent.m_imm = val.m_imm;
    parent.m_obj = OwnObject(val.m_obj);
    m_obj = MakeRef<Anchor>(std::move(inner), std::move(parent));
}

InternalValue::InternalValue(const UndefinedValue& val) noexcept
    : m_imm(UndefinedValue{ val.info, false })
    , m_obj(val.info)
{
}

InternalValue::InternalValue(detail::RendererPtr val)
    : InternalValue(RecursiveWrapper<detail::RendererPtr>(std::move(val)))
{
}

void InternalValue::CloneObject()
{
    // Only a list's accessor clones (a generator)
    const auto* list = std::get_if<detail::ListHandle>(&m_imm);
    if (!list || !list->accessor)
    {
        return;
    }
    auto clone = list->accessor->Clone();
    if (!clone)
    {
        return;
    }
    const auto* anchor = AsAnchor(m_obj.get());
    m_imm = detail::ListHandle{ clone.get(), list->isTuple };
    if (anchor)
    {
        m_obj = MakeRef<Anchor>(std::move(clone), anchor->Parent());
    }
    else
    {
        m_obj = std::move(clone);
    }
}

std::optional<TargetString> TakeString(InternalValue&& val)
{
    if (auto str = AsStringView<char>(val))
    {
        return TargetString(std::string(*str));
    }
    if (auto str = AsStringView<wchar_t>(val))
    {
        return TargetString(std::wstring(*str));
    }
    return std::nullopt;
}

ListRef::Iterator::Iterator() = default;

void ListRef::Iterator::increment()
{
    m_isFinished = !m_iterator || !(*m_iterator)->MoveNext();
    ++m_currentIndex;
    m_currentVal = m_isFinished || !m_iterator ? InternalValue() : (*m_iterator)->GetCurrent();
}

bool ListRef::Iterator::equal(const Iterator& other) const
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

namespace
{
// Python's ==
bool IsEqualValue(const InternalValue& lhs, const InternalValue& rhs)
{
    return ConvertToBool(Apply2<visitors::BinaryMathOperation>(lhs, rhs, BinaryExpression::LogicalEq));
}
} // namespace

bool InternalValue::IsEqual(const InternalValue& other) const
{
    if (!IsEqualValue(*this, other))
    {
        return false;
    }
    const auto* anchor = AsAnchor(m_obj.get());
    const auto* otherAnchor = AsAnchor(other.m_obj.get());
    if (!anchor || !otherAnchor)
    {
        return anchor == nullptr && otherAnchor == nullptr;
    }
    return IsEqualValue(anchor->Parent(), otherAnchor->Parent());
}

InternalValue Value2IntValue(const Value& val);
InternalValue Value2IntValue(Value&& val);

struct SubscriptionVisitor : public visitors::BaseVisitor<>
{
    using BaseVisitor<>::operator();

    template<typename CharT>
    InternalValue operator()(const MapRef& values, const std::basic_string<CharT>& fieldName) const
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
    InternalValue operator()(const MapRef& values, const std::basic_string_view<CharT>& fieldName) const
    {
        return GetField(values, ConvertString<std::string>(fieldName));
    }

    // Undefined for a missing name; a user's accessor (IMapItemAccessor) is asked through
    // Find, which a 1.x accessor answers with HasValue first, as its contract allows
    [[nodiscard]] static InternalValue GetField(const MapRef& values, const std::string& field) { return values.GetValueByName(field); }

    // Python indexing: a negative index counts from the end
    static bool NormalizeIndex(int64_t& index, size_t size)
    {
        if (index < 0)
        {
            index += static_cast<int64_t>(size);
        }
        return index >= 0 && static_cast<size_t>(index) < size;
    }

    InternalValue operator()(const ListRef& values, int64_t index) const
    {
        auto size = values.GetSize();
        if (!size || !NormalizeIndex(index, *size))
        {
            return InternalValue();
        }

        return values.GetValueByIndex(index);
    }

    // A key that is not a string is a key of a dict the template owns (1, 1.5, True, none, a tuple);
    // other maps hold strings only, and an unhashable key reads as undefined, as Jinja2 swallows
    // the TypeError
    [[nodiscard]] static InternalValue FindKey(const MapRef& values, const InternalValue& key)
    {
        auto found = values.FindItem(key);
        return found ? std::move(*found) : InternalValue();
    }

    InternalValue operator()(const MapRef& values, int64_t key) const { return FindKey(values, InternalValue(key)); }
    InternalValue operator()(const MapRef& values, bool key) const { return FindKey(values, InternalValue(key)); }
    InternalValue operator()(const MapRef& values, double key) const { return FindKey(values, InternalValue(key)); }
    InternalValue operator()(const MapRef& values, NoneValue /*key*/) const { return FindKey(values, InternalValue(NoneValue{})); }
    InternalValue operator()(const MapRef& values, const KeyValuePair& key) const { return FindKey(values, InternalValue(key)); }
    InternalValue operator()(const MapRef& values, const ListRef& key) const
    {
        if (!key.IsTuple() && !key.GetRangeInfo())
        {
            return InternalValue();
        }
        return FindKey(values, InternalValue(key.ToAdapter()));
    }

    // A dictsort pair reads as the 2-tuple (key, value)
    InternalValue operator()(const KeyValuePair& pair, int64_t index) const
    {
        if (!NormalizeIndex(index, 2))
        {
            return InternalValue();
        }
        return index == 0 ? InternalValue(pair.key) : pair.value;
    }

    // The fields of a namedtuple
    template<typename CharT>
    InternalValue operator()(const ListRef& values, const std::basic_string<CharT>& fieldName) const
    {
        return SubscriptField(values, ConvertString<std::string>(fieldName));
    }

    template<typename CharT>
    InternalValue operator()(const ListRef& values, const std::basic_string_view<CharT>& fieldName) const
    {
        return SubscriptField(values, ConvertString<std::string>(fieldName));
    }

    [[nodiscard]] static InternalValue SubscriptField(const ListRef& values, const std::string& field)
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
        return InternalValue::FromText(str.substr(start, end - start));
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
void ResolveMapCallOperator(InternalValue& result, MapRef map, RenderContext* values)
{
    static const std::string callOperName = "value()";

    if (!map.HasValue(callOperName))
    {
        return;
    }

    auto callableVal = map.GetValueByName(callOperName);
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
    if (auto map = AsMap(result))
    {
        ResolveMapCallOperator(result, *map, values);
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
    if (auto map = AsMap(val))
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

    InternalValue operator()(const ListRef& values) const
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

    // A dictsort pair slices as the 2-tuple (key, value)
    InternalValue operator()(const KeyValuePair& pair) const
    {
        const auto tuple = visitors::PairAsTuple(pair);
        // A const lvalue, or BaseVisitor's forwarding template would win over operator()(const ListRef&)
        const ListRef ref = tuple.Ref();
        return this->operator()(ref);
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

        if (length == str.size())
        {
            // One unit per character (ASCII, or no surrogate pair): index the units
            if (indices.step == 1)
            {
                return InternalValue::FromText(str.substr(static_cast<size_t>(indices.start), indices.count));
            }
            return InternalValue::MakeText<CharT>(indices.count, [&str, &indices](boost::span<CharT> chars) {
                auto out = chars.begin();
                for (size_t n = 0; n != indices.count; ++n)
                {
                    *out++ = str[indices.At(n)];
                }
            });
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
            return InternalValue::FromText(str.substr(begin, end - begin));
        }

        auto chars = SplitCodePoints(str);
        std::basic_string<CharT> result;
        for (size_t n = 0; n != indices.count; ++n)
        {
            auto ch = chars[indices.At(n)];
            result.append(ch.data(), ch.size());
        }
        return InternalValue(result);
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

namespace visitors
{
ListAdapter PairAsTuple(const KeyValuePair& pair)
{
    return ListAdapter::CreateAdapter(InternalValueList{ InternalValue(pair.key), pair.value }).MarkAsTuple();
}
} // namespace visitors

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

    result_t operator()(const ListRef& list) const { return list.ToAdapter(); }
    // A dictsort pair iterates as the 2-tuple (key, value)
    result_t operator()(const KeyValuePair& pair) const
    {
        return ListAdapter::CreateAdapter(InternalValueList{ InternalValue(pair.key), pair.value });
    }
    // Iterating undefined yields nothing, as in Python; StrictUndefined refuses
    result_t operator()(const UndefinedValue& val) const
    {
        CheckStrictUndefined(val);
        return ListAdapter::CreateAdapter(InternalValueList());
    }
    result_t operator()(const MapRef& map) const
    {
        // An object such as namespace() is not iterable
        if (strictConvertion || map.GetObjectClass())
        {
            return result_t();
        }

        InternalValueList list;
        for (auto& k : map.GetKeyValues())
        {
            list.emplace_back(std::move(k));
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
        ForEachCodePoint(str, [&chars](auto ch) { chars.push_back(InternalValue::FromText(ch)); });
        return result_t(ListAdapter::CreateAdapter(std::move(chars)));
    }
};

ListAdapter ConvertToList(const InternalValue& val, bool& isConverted, bool strictConversion)
{
    // A list converts to a copy of itself, made in place
    if (auto list = AsList(val))
    {
        isConverted = true;
        return list->ToAdapter();
    }
    auto result = Apply<ListConverter>(val, strictConversion);
    if (!result)
    {
        isConverted = false;
        return ListAdapter();
    }
    isConverted = true;
    return *result;
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
    // Whether this holder alone keeps the storage
    [[nodiscard]] bool IsUnique() const { return m_val.use_count() == 1; }

    bool operator==(const BySharedMutable<T>& other) const { return *m_val == *other.m_val; }
    bool operator!=(const BySharedMutable<T>& other) const { return !(*this == other); }

private:
    std::shared_ptr<T> m_val;
};

// The owner of a generic container's storage when a value (a user callable's result) holds
// it, and null when the caller owns it for the whole render (ByRef)
template<template<typename> class Holder, typename T>
std::shared_ptr<const void> GenericOwner([[maybe_unused]] const Holder<T>& holder)
{
    if constexpr (std::is_same_v<Holder<T>, BySharedVal<T>>)
    {
        return holder.GetOwner();
    }
    else
    {
        return nullptr;
    }
}

// An item of a generic list or map: a nested container of reflected data reads its parent
// through raw pointers (Reflect of a field or an element), so once the item escapes the
// container (a loop target stored in a namespace, a sorted copy) its accessor must keep the
// owned root alive (docs/tasks/0125). Strings are copied by the convertor either way
InternalValue LendGenericItem(const std::shared_ptr<const void>& owner, Value&& item)
{
    if (owner)
    {
        // An invalid container stays invalid: a wrapper's provider would hand out a null accessor
        if (auto* list = std::get_if<GenericList>(&item.GetData()); list && list->IsValid())
        {
            return Value2IntValue(Value(GenericList([owner, nested = std::move(*list)]() { return nested.GetAccessor(); })));
        }
        if (auto* map = std::get_if<GenericMap>(&item.GetData()); map && map->GetAccessor())
        {
            return Value2IntValue(Value(GenericMap([owner, nested = std::move(*map)]() { return nested.GetAccessor(); })));
        }
    }
    return Value2IntValue(std::move(item));
}

template<template<typename> class Holder>
class GenericListAdapter : public IListAccessor
{
public:
    struct Enumerator : public IListAccessorEnumerator
    {
        std::optional<ListEnumeratorPtr> m_enum;
        std::shared_ptr<const void> m_owner;

        explicit Enumerator(std::optional<ListEnumeratorPtr> e, std::shared_ptr<const void> owner)
            : m_enum(std::move(e))
            , m_owner(std::move(owner))
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
        [[nodiscard]] InternalValue GetCurrent() const override { return !m_enum ? InternalValue() : LendGenericItem(m_owner, (*m_enum)->GetCurrent()); }
        [[nodiscard]] std::optional<ListAccessorEnumeratorPtr> Clone() const override
        {
            return !m_enum ? std::optional<ListAccessorEnumeratorPtr>{} : std::make_optional<ListAccessorEnumeratorPtr>(types::in_place_type_t<Enumerator>{}, (*m_enum)->Clone(), m_owner);
        }
        std::optional<ListAccessorEnumeratorPtr> Transfer() override
        {
            return !m_enum ? std::optional<ListAccessorEnumeratorPtr>{} : std::make_optional<ListAccessorEnumeratorPtr>(types::in_place_type_t<Enumerator>{}, std::move(*m_enum), std::move(m_owner));
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

        return LendGenericItem(GenericOwner(m_values), indexer->GetItemByIndex(idx));
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
        return ListAccessorEnumeratorPtr(Enumerator(m_values.Get().GetAccessor()->CreateEnumerator(), GenericOwner(m_values)));
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
    if (const auto* list = std::get_if<RecWrapper<ValuesList>>(&item.GetData()))
    {
        return InternalValue(LendNestedList(std::shared_ptr<ValuesList>(owner, const_cast<ValuesList*>(&**list))));
    }
    if (const auto* map = std::get_if<RecWrapper<ValuesMap>>(&item.GetData()))
    {
        return InternalValue(LendNestedMap(std::shared_ptr<ValuesMap>(owner, const_cast<ValuesMap*>(&**map))));
    }
    return visit(visitors::InputValueConvertor(true, false), item.GetData());
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
        // Scalars, the usual items of user data, skip the convertor's visit. The exact
        // std::get_if<std::string> leaves a Markup to the convertor: do not switch it to GetIf<std::string>
        if (const auto* s = std::get_if<std::string>(&item.GetData()))
        {
            return InternalValue(TargetStringView(std::string_view(*s)));
        }
        if (const auto* i = std::get_if<int64_t>(&item.GetData()))
        {
            return InternalValue(*i);
        }
        return Value2IntValue(item);
    }
}

struct ItemAdapters;

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
        return LendItem(m_values, m_values.Get()[static_cast<size_t>(idx)]);
    }
    // The item an enumeration reads
    [[nodiscard]] InternalValue GetCurrentItem(int64_t idx) const
    {
        const auto& item = m_values.Get()[static_cast<size_t>(idx)];
        if constexpr (std::is_same_v<Holder<ValuesList>, ByRef<ValuesList>>)
        {
            // LendItem's scalars first, as there: a map is the less common item. As there, the
            // exact std::get_if<std::string> leaves a Markup to the convertor
            if (const auto* s = std::get_if<std::string>(&item.GetData()))
            {
                return InternalValue(TargetStringView(std::string_view(*s)));
            }
            if (const auto* i = std::get_if<int64_t>(&item.GetData()))
            {
                return InternalValue(*i);
            }
            if (const auto* list = std::get_if<RecWrapper<ValuesList>>(&item.GetData()))
            {
                return LendNested(static_cast<size_t>(idx), **list);
            }
            if (const auto* map = std::get_if<RecWrapper<ValuesMap>>(&item.GetData()))
            {
                return LendNested(static_cast<size_t>(idx), **map);
            }
            return Value2IntValue(item);
        }
        else
        {
            return LendItem(m_values, item);
        }
    }
    [[nodiscard]] bool ShouldExtendLifetime() const override { return m_values.ShouldExtendLifetime(); }
    [[nodiscard]] const void* GetIdentity() const override { return &m_values.Get(); }
    [[nodiscard]] GenericList CreateGenericList() const override
    {
        // return m_values.Get();
        return GenericList([list = *this]() -> const IListItemAccessor* { return &list; });
    }

private:
    // A list or map item of the caller's list, lent from the block of adapters the list keeps
    template<typename T>
    [[nodiscard]] InternalValue LendNested(size_t idx, const T& container) const;

    Holder<ValuesList> m_values;
    // The adapters of the list and map items read lately, made in blocks of ItemAdapters::
    // BlockSize items, instead of an adapter allocated for each item each time it is read
    // (docs/tasks/0154). A list of the caller's data is converted for one render or held
    // by the thread's globals, so it is never read by two threads
    mutable boost::intrusive_ptr<ItemAdapters> m_items;
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
        [[nodiscard]] bool OwnsMutableItemsAlone() const override { return m_values.use_count() == 1; }
        [[nodiscard]] GenericList CreateGenericList() const override
        {
            return GenericList([adapter = *this]() -> const IListItemAccessor* { return &adapter; });
        }

    private:
        std::shared_ptr<InternalValueList> m_values;
    };

    return ListAdapter(MakeRef<Adapter>(std::move(values)));
}

ListAdapter ListAdapter::CreateAdapter(const GenericList& values)
{
    return ListAdapter(MakeRef<GenericListAdapter<ByRef>>(values));
}

ListAdapter ListAdapter::CreateAdapter(const ValuesList& values)
{
    return ListAdapter(MakeRef<ValuesListAdapter<ByRef>>(values));
}

ListAdapter ListAdapter::CreateAdapter(GenericList&& values)
{
    return ListAdapter(MakeRef<GenericListAdapter<BySharedVal>>(std::move(values)));
}

ListAdapter ListAdapter::CreateAdapter(ValuesList&& values)
{
    return ListAdapter(MakeRef<ValuesListAdapter<BySharedVal>>(std::move(values)));
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
            SetClonesOnCopy();
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
        // gets its own (SetClonesOnCopy), as each copy of the old std::function-held accessor did
        [[nodiscard]] boost::intrusive_ptr<const IListAccessor> Clone() const override { return MakeRef<Adapter>(*this); }

    private:
        std::function<std::optional<InternalValue>()> m_fn;
    };

    return ListAdapter(MakeRef<Adapter>(std::move(fn)));
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

    return ListAdapter(MakeRef<Adapter>(listSize, std::move(fn)));
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

    return ListAdapter(MakeRef<Adapter>(RangeInfo{ start, stop, step }));
}

InternalValueList ListRef::ToValueList() const
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

void ListRef::ForEach(IListAccessor::ItemVisitor fn) const
{
    if (m_accessor)
    {
        m_accessor->ForEach(fn);
    }
}

void ListAdapter::Clone()
{
    if (!m_accessor)
    {
        return;
    }
    if (auto clone = m_accessor->Clone())
    {
        m_accessor = clone.get();
        m_owner = std::move(clone);
    }
}

namespace
{
// A namedtuple's items, read through the list it was made from, and its field names
class NamedTupleAccessor final : public IListAccessor
{
public:
    NamedTupleAccessor(ListAdapter items, std::shared_ptr<const std::vector<std::string>> fieldNames)
        : m_items(std::move(items))
        , m_fieldNames(std::move(fieldNames))
    {
    }

    [[nodiscard]] std::optional<size_t> GetSize() const override { return m_items.GetSize(); }
    [[nodiscard]] std::optional<InternalValue> GetItem(int64_t idx) const override { return Accessor()->GetItem(idx); }
    [[nodiscard]] std::optional<ListAccessorEnumeratorPtr> CreateListAccessorEnumerator() const override { return m_items.GetEnumerator(); }
    void ForEach(ItemVisitor fn) const override { m_items.ForEach(fn); }
    [[nodiscard]] GenericList CreateGenericList() const override { return m_items.CreateGenericList(); }
    [[nodiscard]] bool ShouldExtendLifetime() const override { return m_items.ShouldExtendLifetime(); }
    [[nodiscard]] const void* GetIdentity() const override { return m_items.GetIdentity(); }
    [[nodiscard]] const RangeInfo* GetRangeInfo() const override { return m_items.GetRangeInfo(); }
    [[nodiscard]] InternalValueList* GetMutableItems() const override { return m_items.GetMutableItems(); }
    [[nodiscard]] const std::vector<std::string>* GetFieldNames() const override { return m_fieldNames.get(); }

private:
    [[nodiscard]] const IListAccessor* Accessor() const { return m_items.GetAccessor(); }

    ListAdapter m_items;
    std::shared_ptr<const std::vector<std::string>> m_fieldNames;
};
} // namespace

ListAdapter& ListAdapter::MarkAsNamedTuple(std::shared_ptr<const std::vector<std::string>> fieldNames)
{
    if (m_accessor)
    {
        *this = ListAdapter(MakeRef<NamedTupleAccessor>(std::move(*this), std::move(fieldNames)));
    }
    m_isTuple = true;
    return *this;
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
        result.push_back(KeyValuePair{ InternalValue(key), std::move(value) });
    }

    return result;
}

InternalValueList IMapAccessor::GetKeyValues() const
{
    InternalValueList result;
    auto keys = GetKeys();
    result.reserve(keys.size());
    for (auto& key : keys)
    {
        result.emplace_back(key);
    }

    return result;
}

std::optional<InternalValue> IMapAccessor::FindItem(const InternalValue& key) const
{
    if (IsStringValue(key))
    {
        const auto name = AsString(key);
        if (HasValue(name))
        {
            return GetItem(name);
        }
    }

    return std::nullopt;
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
            result.push_back(KeyValuePair{ InternalValue(key), value });
        }

        return result;
    }

    bool SetValue(std::string name, const InternalValue& val) const override
    {
        if constexpr (canModify)
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
    // Mutable for SetValue, the one writer (IMapAccessor::SetValue)
    mutable Holder<Map> m_values;
};

// A dict the template owns (dict literals, kwargs, dict()): shared like a Python dict, and keyed
// like one (TemplateDict). A string key costs one lookup as in any other map
class SharedDictAdapter : public MapAccessorImpl<SharedDictAdapter>
{
public:
    explicit SharedDictAdapter(TemplateDict&& values)
        : m_values(std::move(values))
    {
    }

    [[nodiscard]] size_t GetSize() const override { return m_values.Get().size(); }
    [[nodiscard]] bool HasValue(const std::string& name) const override { return m_values.Get().FindString(name) != m_values.Get().end(); }
    [[nodiscard]] InternalValue GetItem(const std::string& name) const override
    {
        auto& vals = m_values.Get();
        auto p = vals.FindString(name);
        return p == vals.end() ? InternalValue() : p->second;
    }
    // The keys spelled as str(), so that a reader of the string keyed view never sees a token
    [[nodiscard]] std::vector<std::string> GetKeys() const override
    {
        std::vector<std::string> result;
        auto& vals = m_values.Get();
        result.reserve(vals.size());
        for (const auto& [stored, value] : vals)
        {
            result.push_back(DictKeyText(vals.KeyOf(stored)));
        }

        return result;
    }
    [[nodiscard]] InternalValueList GetKeyValues() const override
    {
        InternalValueList result;
        auto& vals = m_values.Get();
        result.reserve(vals.size());
        for (const auto& [stored, value] : vals)
        {
            result.push_back(vals.KeyOf(stored));
        }

        return result;
    }
    [[nodiscard]] std::vector<KeyValuePair> GetEntries() const override
    {
        std::vector<KeyValuePair> result;
        auto& vals = m_values.Get();
        result.reserve(vals.size());
        for (const auto& [stored, value] : vals)
        {
            result.push_back(KeyValuePair{ vals.KeyOf(stored), value });
        }

        return result;
    }
    [[nodiscard]] std::optional<InternalValue> FindItem(const InternalValue& key) const override
    {
        auto& vals = m_values.Get();
        auto p = vals.Find(key);
        if (p == vals.end())
        {
            return std::nullopt;
        }
        return p->second;
    }

    bool SetValue(std::string name, const InternalValue& val) const override
    {
        m_values.Get().SetString(std::move(name), val);
        return true;
    }
    [[nodiscard]] bool ShouldExtendLifetime() const override { return m_values.ShouldExtendLifetime(); }
    [[nodiscard]] bool IsEqual(const IComparable& other) const override
    {
        const auto* val = dynamic_cast<const SharedDictAdapter*>(&other);
        if (!val)
        {
            return false;
        }
        return m_values == val->m_values;
    }
    [[nodiscard]] const void* GetIdentity() const override { return &m_values.Get(); }
    [[nodiscard]] TemplateDict* GetMutableItems() const override { return &m_values.Get(); }
    [[nodiscard]] bool OwnsMutableItemsAlone() const override { return m_values.IsUnique(); }
    [[nodiscard]] MapAttrPolicy GetAttrPolicy() const override { return MapAttrPolicy::MethodsFirst; }
    [[nodiscard]] GenericMap CreateGenericMap() const override
    {
        if (auto snapshot = CreateSnapshot())
        {
            return std::move(*snapshot);
        }
        return GenericMap([accessor = *this]() -> const IMapItemAccessor* { return &accessor; });
    }

protected:
    // The public map has string keys only (D13: docs/release-2.0.md, docs/tasks/0036). A dict
    // with a key that is not a string is handed over as a copy, not a live view, with the keys
    // spelled as str() does: {1: 'x', None: 'y'} has the keys "1" and "None". Keys that spell
    // alike ({1: 'x', '1': 'y'}) collapse, and the later value wins at the position of the first,
    // as Python's {str(k): v for k, v in d.items()} has it. A dict of string keys stays a live view
    [[nodiscard]] std::optional<GenericMap> CreateSnapshot() const
    {
        auto& vals = m_values.Get();
        if (!vals.HasKeyValues())
        {
            return std::nullopt;
        }
        ValuesMap snapshot;
        for (const auto& [stored, value] : vals)
        {
            snapshot[DictKeyText(vals.KeyOf(stored))] = IntValue2Value(value);
        }
        return CreateMapAdapter(std::move(snapshot)).CreateGenericMap();
    }

    // Mutable for SetValue, the one writer (IMapAccessor::SetValue)
    mutable BySharedMutable<TemplateDict> m_values;
};

// namespace(): shared like a dict, but an object with attributes rather than a dict to the
// template, so it exposes no dict methods
class NamespaceAdapter : public SharedDictAdapter
{
public:
    using SharedDictAdapter::SharedDictAdapter;

    // Its attributes are names: ns[1] is undefined, as in Python
    [[nodiscard]] std::optional<InternalValue> FindItem(const InternalValue& key) const override
    {
        return IsStringValue(key) ? SharedDictAdapter::FindItem(key) : std::nullopt;
    }
    [[nodiscard]] MapAttrPolicy GetAttrPolicy() const override { return MapAttrPolicy::KeysOnly; }
    [[nodiscard]] bool IsNamespace() const override { return true; }
    [[nodiscard]] const char* GetObjectClass() const override { return "Namespace"; }
    [[nodiscard]] GenericMap CreateGenericMap() const override
    {
        if (auto snapshot = CreateSnapshot())
        {
            return std::move(*snapshot);
        }
        return GenericMap([accessor = *this]() -> const IMapItemAccessor* { return &accessor; });
    }
};

// An object of a Python class whose attributes are kept as a map (cycler(), joiner(), `self`)
class ObjectMapAdapter : public InternalValueMapAdapter<ByVal, true>
{
public:
    ObjectMapAdapter(InternalValueMap&& values, const char* className, std::optional<InternalValue> reprArgument = {})
        : InternalValueMapAdapter(std::move(values))
        , m_className(className)
        , m_reprArgument(std::move(reprArgument))
    {
    }

    [[nodiscard]] const char* GetObjectClass() const override { return m_className; }
    [[nodiscard]] const InternalValue* GetReprArgument() const override { return m_reprArgument ? &*m_reprArgument : nullptr; }
    [[nodiscard]] GenericMap CreateGenericMap() const override
    {
        return GenericMap([accessor = *this]() -> const IMapItemAccessor* { return &accessor; });
    }

private:
    const char* m_className;
    std::optional<InternalValue> m_reprArgument;
};

InternalValue Value2IntValue(const Value& val)
{
    return std::visit(visitors::InputValueConvertor(false, true), val.GetData());
}

// NOLINTNEXTLINE(cppcoreguidelines-rvalue-reference-param-not-moved): the convertor moves out of val.data()
InternalValue Value2IntValue(Value&& val)
{
    return std::visit(visitors::InputValueConvertor(true, false), val.GetData());
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
    // One lookup; an absent item reads as Undefined, and so does an empty Value unless the accessor
    // says an empty Value is a real null (JSON), which is None
    [[nodiscard]] InternalValue GetItem(const std::string& name) const override
    {
        auto val = m_values.Get().Find(name);
        if (!val)
        {
            return InternalValue();
        }
        if (val->IsNone())
        {
            const auto* accessor = m_values.Get().GetAccessor();
            return accessor && accessor->EmptyIsNone() ? InternalValue(NoneValue{}) : InternalValue();
        }

        return LendGenericItem(GenericOwner(m_values), std::move(*val));
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

// A key of user data as LendItem lends its string items: a view while the data outlives the
// render, so reading entries copies no key text (Render/dict_ops)
template<template<typename> class Holder, typename T>
InternalValue LendKey([[maybe_unused]] const Holder<T>& holder, const std::string& key)
{
    if constexpr (std::is_same_v<Holder<T>, ByRef<T>>)
    {
        return InternalValue(TargetStringView(std::string_view(key)));
    }
    else
    {
        return InternalValue(key);
    }
}

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
            result.push_back(KeyValuePair{ LendKey(m_values, key), LendItem(m_values, value) });
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

// One object for the block: a lent item's value shares it (docs/tasks/0154, 0140)
struct ItemAdapters final : ValueObject
{
    using Adapter = std::variant<std::monostate, ValuesListAdapter<ByRef>, ValuesMapAdapter<ByRef>>;
    // Items per block: the list keeps only the block of the items it reads now, so a long list
    // holds a bounded number of adapters; a block an item was kept from stays with that item
    static constexpr size_t BlockSize = 256;

    ItemAdapters(size_t firstItem, size_t count)
        : adapters(std::make_unique<Adapter[]>(count))
        , first(firstItem)
        , size(count)
    {
    }

    std::unique_ptr<Adapter[]> adapters;
    size_t first;
    size_t size;
};

template<template<typename> class Holder>
template<typename T>
InternalValue ValuesListAdapter<Holder>::LendNested(size_t idx, const T& container) const
{
    using AdapterType = std::conditional_t<std::is_same_v<T, ValuesList>, ValuesListAdapter<ByRef>, ValuesMapAdapter<ByRef>>;
    using ResultType = std::conditional_t<std::is_same_v<T, ValuesList>, ListAdapter, MapAdapter>;
    if (!m_items || idx < m_items->first || idx - m_items->first >= m_items->size)
    {
        const auto first = idx - (idx % ItemAdapters::BlockSize);
        m_items = MakeRef<ItemAdapters>(first, std::min(ItemAdapters::BlockSize, m_values.Get().size() - first));
    }
    auto& slot = m_items->adapters[idx - m_items->first];
    auto* adapter = std::get_if<AdapterType>(&slot);
    if (!adapter)
    {
        adapter = &slot.template emplace<AdapterType>(ByRef<T>(container));
    }
    // Owned by the block: the item outlives the list adapter if the template keeps it
    return InternalValue(ResultType(adapter, ValueObjectPtr(m_items)));
}

ListAdapter LendNestedList(std::shared_ptr<ValuesList> list)
{
    return ListAdapter(MakeRef<ValuesListAdapter<BySharedVal>>(BySharedVal<ValuesList>(std::move(list))));
}

MapAdapter LendNestedMap(std::shared_ptr<ValuesMap> map)
{
    return MapAdapter(MakeRef<ValuesMapAdapter<BySharedVal>>(BySharedVal<ValuesMap>(std::move(map))));
}

MapAdapter CreateMapAdapter(InternalValueMap&& values)
{
    return MapAdapter(MakeRef<InternalValueMapAdapter<ByVal, true>>(std::move(values)));
}

MapAdapter CreateMapAdapter(InternalDict&& values)
{
    return MapAdapter(MakeRef<SharedDictAdapter>(TemplateDict(std::move(values))));
}

MapAdapter CreateMapAdapter(TemplateDict&& values)
{
    return MapAdapter(MakeRef<SharedDictAdapter>(std::move(values)));
}

MapAdapter CreateNamespaceAdapter(TemplateDict&& values)
{
    return MapAdapter(MakeRef<NamespaceAdapter>(std::move(values)));
}

MapAdapter CreateNamespaceAdapter(InternalDict&& values)
{
    return MapAdapter(MakeRef<NamespaceAdapter>(TemplateDict(std::move(values))));
}

MapAdapter CreateObjectAdapter(InternalValueMap&& values, const char* className)
{
    return MapAdapter(MakeRef<ObjectMapAdapter>(std::move(values), className));
}

MapAdapter CreateObjectAdapter(InternalValueMap&& values, const char* className, InternalValue reprArgument)
{
    return MapAdapter(MakeRef<ObjectMapAdapter>(std::move(values), className, std::move(reprArgument)));
}

MapAdapter CreateMapAdapter(const InternalValueMap* values)
{
    return MapAdapter(MakeRef<InternalValueMapAdapter<ByRef, false>>(*values));
}

MapAdapter CreateMapAdapter(std::shared_ptr<InternalValueMap> values)
{
    return MapAdapter(MakeRef<InternalValueMapAdapter<BySharedVal, false>>(std::move(values)));
}

MapAdapter CreateMapAdapter(const GenericMap& values)
{
    return MapAdapter(MakeRef<GenericMapAdapter<ByRef>>(values));
}

MapAdapter CreateMapAdapter(GenericMap&& values)
{
    return MapAdapter(MakeRef<GenericMapAdapter<BySharedVal>>(std::move(values)));
}

MapAdapter CreateMapAdapter(const ValuesMap& values)
{
    return MapAdapter(MakeRef<ValuesMapAdapter<ByRef>>(values));
}

MapAdapter CreateMapAdapter(ValuesMap&& values)
{
    return MapAdapter(MakeRef<ValuesMapAdapter<BySharedVal>>(std::move(values)));
}

struct OutputValueConvertor
{
    using result_t = Value;

    result_t operator()(const UndefinedValue&) const { return result_t(); }
    result_t operator()(const NoneValue&) const { return result_t(); }
    result_t operator()(const MapRef& adapter) const { return result_t(adapter.CreateGenericMap()); }
    result_t operator()(const ListRef& adapter) const { return result_t(adapter.CreateGenericList()); }
    result_t operator()(const ValueRef& ref) const { return ref.get(); }
    result_t operator()(const KeyValuePair& pair) const { return ValuesMap{ { "key", IntValue2Value(pair.key) }, { "value", IntValue2Value(pair.value) } }; }
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
        return IntValue2Value(*val);
    }

    return Value();
}

Value IntValue2Value(const InternalValue& val)
{
    // A string the value owns is visited as a view of its object (docs/tasks/0151): the Value
    // gets a copy. Only a view of host text stays a view
    // A Markup becomes a Markup, even when it only views host text: the flag has nowhere else to go
    if (IsStringValue(val) && (!IsStringView(val) || val.IsMarkup()))
    {
        const bool isMarkup = val.IsMarkup();
        if (auto text = AsStringView<char>(val))
        {
            return isMarkup ? Value(Markup(*text)) : Value(std::string(*text));
        }
        if (auto text = AsStringView<wchar_t>(val))
        {
            return isMarkup ? Value(WMarkup(*text)) : Value(std::wstring(*text));
        }
    }
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

namespace
{
// The function of a callable converted from a jinja2::UserCallable, which Callable::IsSame
// reads the identity of
struct UserCallableBody
{
    UserCallable val;
    std::vector<ArgumentInfo> argsInfo;

    InternalValue operator()(const CallParams& params, RenderContext& context) const
    {
        auto ucParams = PrepareUserCallableParams(params, context, argsInfo);
        auto result = Value2IntValue(CallHost(ucParams));
        AfterHostCall();
        return result;
    }

    // The host's call: what it throws is a HostCallableFailure, unless the engine raised it (a
    // callable may call back into a template or a macro, which keeps its own error)
    Value CallHost(const UserCallableParams& ucParams) const
    {
        try
        {
            return val.callable(ucParams);
        }
        catch (const std::bad_alloc&)
        {
            throw;
        }
        catch (const jinja2::detail::RenderFailure&)
        {
            throw;
        }
        catch (const jinja2::detail::HostCallableFailure&)
        {
            throw;
        }
        catch (const jinja2::Exception&)
        {
            throw;
        }
        catch (const UndefinedError&)
        {
            throw;
        }
        catch (const RecursionLimitError&)
        {
            throw;
        }
        catch (const std::length_error&)
        {
            // A limit, the engine's or the host's
            throw;
        }
        catch (const IntegerOutOfRangeError&)
        {
            throw;
        }
        catch (const ErrorInfo&)
        {
            throw;
        }
        catch (const ErrorInfoW&)
        {
            throw;
        }
        catch (...)
        {
            throw jinja2::detail::HostCallableFailure(std::current_exception());
        }
    }
};
} // namespace

InputValueConvertor::result_t InputValueConvertor::ConvertUserCallable(const UserCallable& val)
{
    std::vector<ArgumentInfo> args;
    args.reserve(val.argsInfo.size());
    for (const auto& pi : val.argsInfo)
    {
        // By value: the default must not refer to val, which may not outlive the callable made here
        args.emplace_back(pi.paramName, pi.isMandatory, Value2IntValue(Value(pi.defValue)));
    }

    return InternalValue(Callable(Callable::UserCallable, UserCallableBody{ val, std::move(args) }));
}

} // namespace visitors

bool Callable::IsSame(const Callable& other) const
{
    if (m_callable == other.m_callable)
    {
        return true;
    }
    // Each lookup of a user callable in the context may convert it anew
    if (m_kind != UserCallable || other.m_kind != UserCallable || GetType() != Type::Expression || other.GetType() != Type::Expression)
    {
        return false;
    }
    const auto* left = GetExpressionCallable().target<visitors::UserCallableBody>();
    const auto* right = other.GetExpressionCallable().target<visitors::UserCallableBody>();
    return left != nullptr && right != nullptr && left->val.IsEqual(right->val);
}

} // namespace v2
} // namespace jinja2
