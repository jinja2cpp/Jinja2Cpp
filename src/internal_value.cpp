#include "internal_value.h"

#include "expression_evaluator.h"
#include "generic_adapters.h"
#include "helpers.h"
#include "value_visitors.h"

#include <limits>

namespace jinja2
{

void InternalValue::SetParentData(const InternalValue& val)
{
    m_parentData = val.GetData();
}

void InternalValue::SetParentData(InternalValue&& val)
{
    m_parentData = std::move(val.GetData());
}

ListAdapter::Iterator::Iterator()
{
}

void ListAdapter::Iterator::increment()
{
    m_isFinished = !(*m_iterator)->MoveNext();
    ++m_currentIndex;
    m_currentVal = m_isFinished ? InternalValue() : (*m_iterator)->GetCurrent();
}

bool ListAdapter::Iterator::equal(const Iterator& other) const
{
    if (!this->m_iterator)
        return !other.m_iterator ? true : other.equal(*this);

    if (!other.m_iterator)
        return this->m_isFinished;
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

bool operator==(const GenericMap& lhs, const GenericMap& rhs)
{
    auto* lhsAccessor = lhs.GetAccessor();
    auto* rhsAccessor = rhs.GetAccessor();
    return lhsAccessor && rhsAccessor && lhsAccessor->IsEqual(*rhsAccessor);
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
        return false;
    return m_parentData == other.m_parentData;
}

InternalValue Value2IntValue(const Value& val);
InternalValue Value2IntValue(Value&& val);

struct SubscriptionVisitor : public visitors::BaseVisitor<>
{
    using BaseVisitor<>::operator();

    template<typename CharT>
    InternalValue operator()(const MapAdapter& values, const std::basic_string<CharT>& fieldName) const
    {
        auto field = ConvertString<std::string>(fieldName);
        if (!values.HasValue(field))
            return InternalValue();

        return values.GetValueByName(field);
    }

    template<typename CharT>
    InternalValue operator()(const MapAdapter& values, const nonstd::basic_string_view<CharT>& fieldName) const
    {
        auto field = ConvertString<std::string>(fieldName);
        if (!values.HasValue(field))
            return InternalValue();

        return values.GetValueByName(field);
    }

    // Python indexing: a negative index counts from the end
    static bool NormalizeIndex(int64_t& index, size_t size)
    {
        if (index < 0)
            index += static_cast<int64_t>(size);
        return index >= 0 && static_cast<size_t>(index) < size;
    }

    InternalValue operator()(const ListAdapter& values, int64_t index) const
    {
        auto size = values.GetSize();
        if (!size || !NormalizeIndex(index, *size))
            return InternalValue();

        return values.GetValueByIndex(index);
    }

    InternalValue operator()(const MapAdapter& /*values*/, int64_t /*index*/) const { return InternalValue(); }

    template<typename CharT>
    InternalValue operator()(const std::basic_string<CharT>& str, int64_t index) const
    {
        return StringItem(nonstd::basic_string_view<CharT>(str), index);
    }

    template<typename CharT>
    InternalValue operator()(const nonstd::basic_string_view<CharT>& str, int64_t index) const
    {
        return StringItem(str, index);
    }

    // Named apart from operator(): BaseVisitor's catch-all would take a temporary view
    template<typename CharT>
    static InternalValue StringItem(nonstd::basic_string_view<CharT> str, int64_t index)
    {
        if (!NormalizeIndex(index, CodePointCount(str)))
            return InternalValue();

        // Find the index-th character without splitting the whole string
        size_t start = 0;
        for (int64_t seen = 0; seen != index; ++seen)
        {
            ++start;
            while (start < str.size() && IsCodePointTail(str[start]))
                ++start;
        }
        size_t end = start + 1;
        while (end < str.size() && IsCodePointTail(str[end]))
            ++end;
        return TargetString(sv_to_string(str.substr(start, end - start)));
    }

    template<typename CharT>
    InternalValue operator()(const KeyValuePair& values, const std::basic_string<CharT>& fieldName) const
    {
        return SubscriptKvPair(values, ConvertString<std::string>(fieldName));
    }

    template<typename CharT>
    InternalValue operator()(const KeyValuePair& values, const nonstd::basic_string_view<CharT>& fieldName) const
    {
        return SubscriptKvPair(values, ConvertString<std::string>(fieldName));
    }

    InternalValue SubscriptKvPair(const KeyValuePair& values, const std::string& field) const
    {
        // std::cout << "operator() (const KeyValuePair& values, const std::string& field)" << ": field = " << field << std::endl;
        if (field == "key")
            return InternalValue(values.key);
        else if (field == "value")
            return values.value;

        return InternalValue();
    }

    template<typename CharT>
    InternalValue operator()(const Callable& callable, const std::basic_string<CharT>& fieldName) const
    {
        return SubscriptCallable(callable, ConvertString<std::string>(fieldName));
    }

    template<typename CharT>
    InternalValue operator()(const Callable& callable, const nonstd::basic_string_view<CharT>& fieldName) const
    {
        return SubscriptCallable(callable, ConvertString<std::string>(fieldName));
    }

    InternalValue SubscriptCallable(const Callable& callable, const std::string& field) const
    {
        auto& attributes = callable.GetAttributes();
        if (!attributes)
            return InternalValue();

        auto p = attributes->find(field);
        return p == attributes->end() ? InternalValue() : p->second;
    }
};

InternalValue Subscript(const InternalValue& val, const InternalValue& subscript, RenderContext* values)
{
    static const std::string callOperName = "value()";
    auto result = Apply2<SubscriptionVisitor>(val, subscript);

    if (!values)
        return result;

    auto map = GetIf<MapAdapter>(&result);
    if (!map || !map->HasValue(callOperName))
        return result;

    auto callableVal = map->GetValueByName(callOperName);
    auto callable = GetIf<Callable>(&callableVal);
    if (!callable || callable->GetKind() == Callable::Macro || callable->GetType() == Callable::Type::Statement)
        return result;

    CallParams callParams;
    return callable->GetExpressionCallable()(callParams, *values);
}

InternalValue Subscript(const InternalValue& val, const std::string& subscript, RenderContext* values)
{
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
        size_t At(size_t n) const { return static_cast<size_t>(start + static_cast<int64_t>(n) * step); }
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
            return InternalValue();

        InternalValueList result;
        result.reserve(indices.count);
        for (size_t n = 0; n != indices.count; ++n)
        {
            const auto idx = indices.At(n);
            result.push_back(items.empty() ? values.GetValueByIndex(static_cast<int64_t>(idx)) : items[idx]);
        }

        auto list = ListAdapter::CreateAdapter(std::move(result));
        if (values.IsTuple())
            list.MarkAsTuple();
        return list;
    }

    template<typename CharT>
    InternalValue operator()(const std::basic_string<CharT>& str) const
    {
        return SliceString(nonstd::basic_string_view<CharT>(str));
    }

    template<typename CharT>
    InternalValue operator()(const nonstd::basic_string_view<CharT>& str) const
    {
        return SliceString(str);
    }

    // Strings are sliced by code point, like string indexing
    template<typename CharT>
    InternalValue SliceString(nonstd::basic_string_view<CharT> str) const
    {
        auto chars = SplitCodePoints(str);
        Indices indices;
        if (!GetIndices(chars.size(), indices))
            return InternalValue();

        std::basic_string<CharT> result;
        for (size_t n = 0; n != indices.count; ++n)
        {
            auto ch = chars[indices.At(n)];
            result.append(ch.data(), ch.size());
        }
        return TargetString(std::move(result));
    }

    static bool GetIndex(const InternalValue& val, nonstd::optional<int64_t>& index)
    {
        if (IsEmpty(val))
            return true;
        if (auto* intVal = GetIf<int64_t>(&val))
            index = *intVal;
        else if (auto* boolVal = GetIf<bool>(&val))
            index = *boolVal ? 1 : 0;
        else
            return false;
        return true;
    }

    // CPython's PySlice_AdjustIndices
    bool GetIndices(size_t size, Indices& indices) const
    {
        nonstd::optional<int64_t> start, stop, step;
        if (!GetIndex(m_start, start) || !GetIndex(m_stop, stop) || !GetIndex(m_step, step))
            throw std::runtime_error("slice indices must be integers or None or have an __index__ method");
        indices.step = step.value_or(1);
        if (indices.step == 0)
            throw std::runtime_error("slice step cannot be zero");
        // Any step at least as long as the sequence takes one item; this keeps -step defined
        if (indices.step < -std::numeric_limits<int64_t>::max())
            indices.step = -std::numeric_limits<int64_t>::max();

        const auto length = static_cast<int64_t>(size);
        const int64_t lower = indices.step < 0 ? -1 : 0;
        const int64_t upper = indices.step < 0 ? length - 1 : length;
        auto adjust = [length, lower, upper](nonstd::optional<int64_t> index, int64_t def) {
            if (!index)
                return def;
            int64_t result = *index;
            if (result < 0)
                result = result < -length ? lower : result + length;
            return result < lower ? lower : (result > upper ? upper : result);
        };

        indices.start = adjust(start, indices.step < 0 ? upper : lower);
        const int64_t end = adjust(stop, indices.step < 0 ? lower : upper);
        if (indices.step < 0)
            indices.count = end < indices.start ? static_cast<size_t>((indices.start - end - 1) / -indices.step + 1) : 0;
        else
            indices.count = indices.start < end ? static_cast<size_t>((end - indices.start - 1) / indices.step + 1) : 0;
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
    std::string operator()(const nonstd::string_view& str) const { return std::string(str.begin(), str.end()); }
    std::string operator()(const std::wstring& str) const { return ConvertString<std::string>(str); }
    std::string operator()(const nonstd::wstring_view& str) const { return ConvertString<std::string>(str); }
};

std::string AsString(const InternalValue& val)
{
    return Apply<StringGetter>(val);
}

struct ListConverter : public visitors::BaseVisitor<boost::optional<ListAdapter>>
{
    using BaseVisitor::operator();

    using result_t = boost::optional<ListAdapter>;

    bool strictConvertion;

    ListConverter(bool strict)
        : strictConvertion(strict)
    {
    }

    result_t operator()(const ListAdapter& list) const { return list; }
    result_t operator()(const MapAdapter& map) const
    {
        if (strictConvertion)
            return result_t();

        InternalValueList list;
        for (auto& k : map.GetKeys())
            list.push_back(TargetString(k));

        return ListAdapter::CreateAdapter(std::move(list));
    }

    template<typename CharT>
    result_t operator()(const std::basic_string<CharT>& str) const
    {
        return FromString(nonstd::basic_string_view<CharT>(str));
    }

    template<typename CharT>
    result_t operator()(const nonstd::basic_string_view<CharT>& str) const
    {
        return FromString(str);
    }

    // Named apart from operator(): BaseVisitor's catch-all would take a temporary view
    template<typename CharT>
    result_t FromString(nonstd::basic_string_view<CharT> str) const
    {
        if (strictConvertion)
            return result_t();

        InternalValueList chars;
        for (auto ch : SplitCodePoints(str))
            chars.push_back(TargetString(sv_to_string(ch)));
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
    return result.get();
}

ListAdapter ConvertToList(const InternalValue& val, InternalValue subscipt, bool& isConverted, bool strictConversion)
{
    auto result = Apply<ListConverter>(val, strictConversion);
    if (!result)
    {
        isConverted = false;
        return ListAdapter();
    }
    isConverted = true;

    if (IsEmpty(subscipt))
        return std::move(result.get());

    return result.get().ToSubscriptedList(subscipt, false);
}

template<typename T>
class ByRef
{
public:
    explicit ByRef(const T& val)
        : m_val(&val)
    {
    }

    const T& Get() const { return *m_val; }
    T& Get() { return *const_cast<T*>(m_val); }
    bool ShouldExtendLifetime() const { return false; }
    bool operator==(const ByRef<T>& other) const
    {
        if (m_val && other.m_val && m_val != other.m_val)
            return false;
        if ((m_val && !other.m_val) || (!m_val && other.m_val))
            return false;
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

    const T& Get() const { return m_val; }
    T& Get() { return m_val; }
    bool ShouldExtendLifetime() const { return false; }
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
    ~BySharedVal() = default;

    const T& Get() const { return *m_val; }
    T& Get() { return *m_val; }
    bool ShouldExtendLifetime() const { return true; }

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

template<template<typename> class Holder>
class GenericListAdapter : public IListAccessor
{
public:
    struct Enumerator : public IListAccessorEnumerator
    {
        nonstd::optional<ListEnumeratorPtr> m_enum;

        explicit Enumerator(nonstd::optional<ListEnumeratorPtr> e)
            : m_enum(std::move(e))
        {
        }

        // Inherited via IListAccessorEnumerator
        void Reset() override
        {
            if (m_enum)
                (*m_enum)->Reset();
        }
        bool MoveNext() override { return !m_enum ? false : (*m_enum)->MoveNext(); }
        InternalValue GetCurrent() const override { return !m_enum ? InternalValue() : Value2IntValue((*m_enum)->GetCurrent()); }
        nonstd::optional<ListAccessorEnumeratorPtr> Clone() const override
        {
            return !m_enum ? nonstd::optional<ListAccessorEnumeratorPtr>{} : nonstd::make_optional<ListAccessorEnumeratorPtr>(types::in_place_type_t<Enumerator>{}, (*m_enum)->Clone());
        }
        nonstd::optional<ListAccessorEnumeratorPtr> Transfer() override
        {
            return nonstd::make_optional<ListAccessorEnumeratorPtr>(types::in_place_type_t<Enumerator>{}, std::move(*m_enum));
        }
        bool IsEqual(const IComparable& other) const override
        {
            auto* val = dynamic_cast<const Enumerator*>(&other);
            if (!val)
                return false;
            if (m_enum && val->m_enum && !(*m_enum)->IsEqual(**val->m_enum))
                return false;
            if ((m_enum && !val->m_enum) || (!m_enum && val->m_enum))
                return false;
            return true;
        }
    };

    template<typename U>
    GenericListAdapter(U&& values)
        : m_values(std::forward<U>(values))
    {
    }

    nonstd::optional<size_t> GetSize() const override { return m_values.Get().GetSize(); }
    nonstd::optional<InternalValue> GetItem(int64_t idx) const override
    {
        const IListItemAccessor* accessor = m_values.Get().GetAccessor();
        auto indexer = accessor->GetIndexer();
        if (!indexer)
            return nonstd::optional<InternalValue>();

        auto val = indexer->GetItemByIndex(idx);
        return visit(visitors::InputValueConvertor(true, false), std::move(val.data())).get();
    }
    bool ShouldExtendLifetime() const override { return m_values.ShouldExtendLifetime(); }
    const void* GetIdentity() const override { return m_values.Get().GetAccessor(); }
    nonstd::optional<ListAccessorEnumeratorPtr> CreateListAccessorEnumerator() const override
    {
        const IListItemAccessor* accessor = m_values.Get().GetAccessor();
        if (!accessor)
            return {};
        return ListAccessorEnumeratorPtr(Enumerator(m_values.Get().GetAccessor()->CreateEnumerator()));
    }
    GenericList CreateGenericList() const override
    {
        // return m_values.Get();
        return GenericList([list = m_values]() -> const IListItemAccessor* { return list.Get().GetAccessor(); });
    }

private:
    Holder<GenericList> m_values;
};

template<template<typename> class Holder>
class ValuesListAdapter : public IndexedListAccessorImpl<ValuesListAdapter<Holder>>
{
public:
    template<typename U>
    ValuesListAdapter(U&& values)
        : m_values(std::forward<U>(values))
    {
    }

    size_t GetItemsCountImpl() const { return m_values.Get().size(); }
    nonstd::optional<InternalValue> GetItem(int64_t idx) const override
    {
        const auto& val = m_values.Get()[static_cast<size_t>(idx)];
        return visit(visitors::InputValueConvertor(false, true), val.data()).get();
    }
    bool ShouldExtendLifetime() const override { return m_values.ShouldExtendLifetime(); }
    GenericList CreateGenericList() const override
    {
        // return m_values.Get();
        return GenericList([list = *this]() -> const IListItemAccessor* { return &list; });
    }

private:
    Holder<ValuesList> m_values;
};

ListAdapter ListAdapter::CreateAdapter(InternalValueList&& values)
{
    class Adapter : public IndexedListAccessorImpl<Adapter>
    {
    public:
        explicit Adapter(InternalValueList&& values)
            : m_values(std::move(values))
        {
        }

        size_t GetItemsCountImpl() const { return m_values.size(); }
        nonstd::optional<InternalValue> GetItem(int64_t idx) const override { return m_values[static_cast<size_t>(idx)]; }
        bool ShouldExtendLifetime() const override { return false; }
        GenericList CreateGenericList() const override
        {
            return GenericList([adapter = *this]() -> const IListItemAccessor* { return &adapter; });
        }

    private:
        InternalValueList m_values;
    };

    return ListAdapter([accessor = Adapter(std::move(values))]() { return &accessor; });
}

ListAdapter ListAdapter::CreateAdapter(const GenericList& values)
{
    return ListAdapter([accessor = GenericListAdapter<ByRef>(values)]() { return &accessor; });
}

ListAdapter ListAdapter::CreateAdapter(const ValuesList& values)
{
    return ListAdapter([accessor = ValuesListAdapter<ByRef>(values)]() { return &accessor; });
}

ListAdapter ListAdapter::CreateAdapter(GenericList&& values)
{
    return ListAdapter([accessor = GenericListAdapter<BySharedVal>(std::move(values))]() { return &accessor; });
}

ListAdapter ListAdapter::CreateAdapter(ValuesList&& values)
{
    return ListAdapter([accessor = ValuesListAdapter<BySharedVal>(std::move(values))]() { return &accessor; });
}

ListAdapter ListAdapter::CreateAdapter(std::function<nonstd::optional<InternalValue>()> fn)
{
    using GenFn = std::function<nonstd::optional<InternalValue>()>;

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
                    throw std::runtime_error("List enumerator couldn't be created without element accessor function!");
            }

            Enumerator(const Enumerator& other)
                : m_fn(other.m_fn)
                , m_current(other.m_current)
                , m_isFinished(other.m_isFinished)
            {}

            Enumerator(Enumerator&& other) noexcept
                : m_fn(std::move(other.m_fn))
                , m_current(std::move(other.m_current))
                , m_isFinished(std::move(other.m_isFinished))
            {}

            void Reset() override {}

            bool MoveNext() override
            {
                if (m_isFinished)
                    return false;

                auto res = (*m_fn)();
                if (!res)
                    return false;

                m_current = *res;

                return true;
            }

            InternalValue GetCurrent() const override { return m_current; }

            nonstd::optional<ListAccessorEnumeratorPtr> Clone() const override
            {
                return nonstd::make_optional<ListAccessorEnumeratorPtr>(types::in_place_type_t<Enumerator>{}, *this);
            }

            nonstd::optional<ListAccessorEnumeratorPtr> Transfer() override
            {
                return nonstd::make_optional<ListAccessorEnumeratorPtr>(types::in_place_type_t<Enumerator>{}, std::move(*this));
            }

            bool IsEqual(const IComparable& other) const override
            {
                auto* val = dynamic_cast<const Enumerator*>(&other);
                if (!val)
                    return false;
                if (m_isFinished != val->m_isFinished)
                    return false;
                if (m_current != val->m_current)
                    return false;
                // TODO: compare fn?
                if (m_fn != val->m_fn)
                    return false;
                return true;
            }

        protected:
            const GenFn* m_fn{};
            InternalValue m_current;
            bool m_isFinished = false;
        };

        explicit Adapter(std::function<nonstd::optional<InternalValue>()>&& fn)
            : m_fn(std::move(fn))
        {
        }

        nonstd::optional<size_t> GetSize() const override { return nonstd::optional<size_t>(); }
        nonstd::optional<InternalValue> GetItem(int64_t /*idx*/) const override { return nonstd::optional<InternalValue>(); }
        bool ShouldExtendLifetime() const override { return false; }
        nonstd::optional<ListAccessorEnumeratorPtr> CreateListAccessorEnumerator() const override { return ListAccessorEnumeratorPtr(types::in_place_type_t<Enumerator>{}, Enumerator(&m_fn)); }

        GenericList CreateGenericList() const override
        {
            return GenericList(); //  return GenericList([adapter = *this]() -> const ListItemAccessor* {return &adapter; });
        }

    private:
        std::function<nonstd::optional<InternalValue>()> m_fn;
    };

    return ListAdapter([accessor = Adapter(std::move(fn))]() { return &accessor; });
}

ListAdapter ListAdapter::CreateAdapter(size_t listSize, std::function<InternalValue(size_t idx)> fn)
{
    using GenFn = std::function<InternalValue(size_t idx)>;

    class Adapter : public IndexedListAccessorImpl<Adapter>
    {
    public:
        explicit Adapter(size_t listSize, GenFn&& fn)
            : m_listSize(listSize)
            , m_fn(std::move(fn))
        {
        }

        size_t GetItemsCountImpl() const { return m_listSize; }
        nonstd::optional<InternalValue> GetItem(int64_t idx) const override { return m_fn(static_cast<size_t>(idx)); }
        bool ShouldExtendLifetime() const override { return false; }
        GenericList CreateGenericList() const override
        {
            return GenericList([adapter = *this]() -> const IListItemAccessor* { return &adapter; });
        }

    private:
        size_t m_listSize;
        GenFn m_fn;
    };

    return ListAdapter([accessor = Adapter(listSize, std::move(fn))]() { return &accessor; });
}

ListAdapter ListAdapter::CreateRange(int64_t start, int64_t stop, int64_t step)
{
    class Adapter : public IndexedListAccessorImpl<Adapter>
    {
    public:
        explicit Adapter(RangeInfo info)
            : m_info(info)
        {
            // Unsigned arithmetic: stop - start overflows int64_t for the widest ranges
            auto distance = [](int64_t from, int64_t to) { return static_cast<uint64_t>(to) - static_cast<uint64_t>(from); };
            if (info.step > 0 && info.start < info.stop)
                m_size = (distance(info.start, info.stop) - 1) / static_cast<uint64_t>(info.step) + 1;
            else if (info.step < 0 && info.start > info.stop)
                m_size = (distance(info.stop, info.start) - 1) / (0 - static_cast<uint64_t>(info.step)) + 1;
            // Python raises OverflowError for len() of such a range; lengths here are int64_t
            if (m_size > static_cast<uint64_t>(std::numeric_limits<int64_t>::max()))
                throw std::runtime_error("range() has more items than fit in a 64-bit integer");
        }

        size_t GetItemsCountImpl() const { return static_cast<size_t>(m_size); }
        nonstd::optional<InternalValue> GetItem(int64_t idx) const override
        {
            auto value = static_cast<uint64_t>(m_info.start) + static_cast<uint64_t>(m_info.step) * static_cast<uint64_t>(idx);
            return InternalValue(static_cast<int64_t>(value));
        }
        bool ShouldExtendLifetime() const override { return false; }
        const RangeInfo* GetRangeInfo() const override { return &m_info; }
        GenericList CreateGenericList() const override
        {
            return GenericList([adapter = *this]() -> const IListItemAccessor* { return &adapter; });
        }

    private:
        RangeInfo m_info;
        uint64_t m_size = 0;
    };

    return ListAdapter([accessor = Adapter(RangeInfo{ start, stop, step })]() { return &accessor; });
}

template<typename Holder>
auto CreateIndexedSubscribedList(Holder&& holder, const InternalValue& subscript, size_t size)
{
    return ListAdapter::CreateAdapter(
        size, [h = std::forward<Holder>(holder), subscript](size_t idx) -> InternalValue { return Subscript(h.Get().GetValueByIndex(idx), subscript, nullptr); });
}

template<typename Holder>
auto CreateGenericSubscribedList(Holder&& holder, const InternalValue& subscript)
{
    return ListAdapter::CreateAdapter([h = std::forward<Holder>(holder), e = nonstd::optional<ListAccessorEnumeratorPtr>(), isFirst = true, isLast = false, subscript]() mutable {
        using ResultType = nonstd::optional<InternalValue>;
        if (isFirst)
        {
            e = h.Get().GetEnumerator();
            isLast = !(*e)->MoveNext();
            isFirst = false;
        }
        if (isLast)
            return ResultType();

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
    else
    {
        ListAdapter tmp(*this);
        BySharedVal<ListAdapter> holder(std::move(tmp));
        return listSize ? CreateIndexedSubscribedList(std::move(holder), subscript, *listSize) : CreateGenericSubscribedList(std::move(holder), subscript);
    }
}

InternalValueList ListAdapter::ToValueList() const
{
    InternalValueList result;
    std::copy(begin(), end(), std::back_inserter(result));
    return result;
}

template<template<typename> class Holder, bool CanModify, typename Map = InternalValueMap>
class InternalValueMapAdapter : public MapAccessorImpl<InternalValueMapAdapter<Holder, CanModify, Map>>
{
public:
    template<typename U>
    InternalValueMapAdapter(U&& values)
        : m_values(std::forward<U>(values))
    {
    }

    size_t GetSize() const override { return m_values.Get().size(); }
    bool HasValue(const std::string& name) const override { return m_values.Get().count(name) != 0; }
    InternalValue GetItem(const std::string& name) const override
    {
        auto& vals = m_values.Get();
        auto p = vals.find(name);
        if (p == vals.end())
            return InternalValue();

        return p->second;
    }
    std::vector<std::string> GetKeys() const override
    {
        std::vector<std::string> result;

        for (auto& i : m_values.Get())
            result.push_back(i.first);

        return result;
    }

    bool SetValue(std::string name, const InternalValue& val) override
    {
        if (CanModify)
        {
            m_values.Get()[name] = val;
            return true;
        }
        return false;
    }
    bool ShouldExtendLifetime() const override { return m_values.ShouldExtendLifetime(); }
    GenericMap CreateGenericMap() const override
    {
        return GenericMap([accessor = *this]() -> const IMapItemAccessor* { return &accessor; });
    }
    bool IsEqual(const IComparable& other) const override
    {
        auto* val = dynamic_cast<const InternalValueMapAdapter*>(&other);
        if (!val)
            return false;
        return m_values == val->m_values;
    }
private:
    Holder<Map> m_values;
};

InternalValue Value2IntValue(const Value& val)
{
    auto result = nonstd::visit(visitors::InputValueConvertor(false, true), val.data());
    if (result)
        return result.get();

    return InternalValue(ValueRef(val));
}

InternalValue Value2IntValue(Value&& val)
{
    auto result = nonstd::visit(visitors::InputValueConvertor(true, false), val.data());
    if (result)
        return result.get();

    return InternalValue(ValueRef(val));
}

template<template<typename> class Holder>
class GenericMapAdapter : public MapAccessorImpl<GenericMapAdapter<Holder>>
{
public:
    template<typename U>
    GenericMapAdapter(U&& values)
        : m_values(std::forward<U>(values))
    {
    }

    size_t GetSize() const override { return m_values.Get().GetSize(); }
    bool HasValue(const std::string& name) const override { return m_values.Get().HasValue(name); }
    InternalValue GetItem(const std::string& name) const override
    {
        auto val = m_values.Get().GetValueByName(name);
        if (val.isEmpty())
            return InternalValue();

        return Value2IntValue(std::move(val));
    }
    std::vector<std::string> GetKeys() const override { return m_values.Get().GetKeys(); }
    bool ShouldExtendLifetime() const override { return m_values.ShouldExtendLifetime(); }
    const void* GetIdentity() const override { return m_values.Get().GetAccessor(); }
    GenericMap CreateGenericMap() const override
    {
        return GenericMap([accessor = *this]() -> const IMapItemAccessor* { return accessor.m_values.Get().GetAccessor(); });
    }
    bool IsEqual(const IComparable& other) const override
    {
        auto* val = dynamic_cast<const GenericMapAdapter*>(&other);
        if (!val)
            return false;
        return m_values == val->m_values;
    }
private:
    Holder<GenericMap> m_values;
};

template<template<typename> class Holder>
class ValuesMapAdapter : public MapAccessorImpl<ValuesMapAdapter<Holder>>
{
public:
    template<typename U>
    ValuesMapAdapter(U&& values)
        : m_values(std::forward<U>(values))
    {
    }

    size_t GetSize() const override { return m_values.Get().size(); }
    bool HasValue(const std::string& name) const override { return m_values.Get().count(name) != 0; }
    InternalValue GetItem(const std::string& name) const override
    {
        auto& vals = m_values.Get();
        auto p = vals.find(name);
        if (p == vals.end())
            return InternalValue();

        return Value2IntValue(p->second);
    }
    std::vector<std::string> GetKeys() const override
    {
        std::vector<std::string> result;

        for (auto& i : m_values.Get())
            result.push_back(i.first);

        return result;
    }
    bool ShouldExtendLifetime() const override { return m_values.ShouldExtendLifetime(); }
    GenericMap CreateGenericMap() const override
    {
        return GenericMap([accessor = *this]() -> const IMapItemAccessor* { return &accessor; });
    }
    bool IsEqual(const IComparable& other) const override
    {
        auto* val = dynamic_cast<const ValuesMapAdapter*>(&other);
        if (!val)
            return false;
        return m_values == val->m_values;
    }
private:
    Holder<ValuesMap> m_values;
};

MapAdapter CreateMapAdapter(InternalValueMap&& values)
{
    return MapAdapter([accessor = InternalValueMapAdapter<ByVal, true>(std::move(values))]() mutable { return &accessor; });
}

MapAdapter CreateMapAdapter(InternalDict&& values)
{
    return MapAdapter([accessor = InternalValueMapAdapter<ByVal, true, InternalDict>(std::move(values))]() mutable { return &accessor; });
}

MapAdapter CreateMapAdapter(const InternalValueMap* values)
{
    return MapAdapter([accessor = InternalValueMapAdapter<ByRef, false>(*values)]() mutable { return &accessor; });
}

MapAdapter CreateMapAdapter(const GenericMap& values)
{
    return MapAdapter([accessor = GenericMapAdapter<ByRef>(values)]() mutable { return &accessor; });
}

MapAdapter CreateMapAdapter(GenericMap&& values)
{
    return MapAdapter([accessor = GenericMapAdapter<BySharedVal>(std::move(values))]() mutable { return &accessor; });
}

MapAdapter CreateMapAdapter(const ValuesMap& values)
{
    return MapAdapter([accessor = ValuesMapAdapter<ByRef>(values)]() mutable { return &accessor; });
}

MapAdapter CreateMapAdapter(ValuesMap&& values)
{
    return MapAdapter([accessor = ValuesMapAdapter<BySharedVal>(std::move(values))]() mutable { return &accessor; });
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
            return nonstd::get<std::string>(str);
        default:
            return nonstd::get<std::wstring>(str);
        }
    }
    result_t operator()(const TargetStringView& str) const
    {
        switch (str.index())
        {
        case 0:
            return nonstd::get<nonstd::string_view>(str);
        default:
            return nonstd::get<nonstd::wstring_view>(str);
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

Value OptIntValue2Value(nonstd::optional<InternalValue> val)
{
    if (val)
        return Apply<OutputValueConvertor>(val.value());

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

    size_t GetSize() const override { return std::numeric_limits<size_t>::max(); }
    bool HasValue(const std::string& name) const override
    {
        bool found = false;
        m_context->FindValue(name, found);
        return found;
    }
    Value GetValueByName(const std::string& name) const override
    {
        bool found = false;
        auto p = m_context->FindValue(name, found);
        return found ? IntValue2Value(p->second) : Value();
    }
    std::vector<std::string> GetKeys() const override { return std::vector<std::string>(); }

    bool IsEqual(const IComparable& other) const override
    {
        auto* val = dynamic_cast<const ContextMapper*>(&other);
        if (!val)
            return false;
        if (m_context && val->m_context && !m_context->IsEqual(*val->m_context))
        {
            return false;
        }
        if ((m_context && !val->m_context) || (!m_context && val->m_context))
            return false;
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
        return result;

    for (auto& argInfo : argsInfo)
    {
        if (argInfo.name.size() > 1 && argInfo.name[0] == '*')
            continue;

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
    for (auto& p : args.extraKwArgs)
        extraKwArgs[p.first] = IntValue2Value(p.second);
    result.extraKwArgs = Value(std::move(extraKwArgs));

    ValuesList extraPosArgs;
    for (auto& p : args.extraPosArgs)
        extraPosArgs.push_back(IntValue2Value(p));
    result.extraPosArgs = Value(std::move(extraPosArgs));
    result.context = GenericMap([accessor = ContextMapper(&context)]() -> const IMapItemAccessor* { return &accessor; });

    return result;
}

namespace visitors
{

InputValueConvertor::result_t InputValueConvertor::ConvertUserCallable(const UserCallable& val)
{
    std::vector<ArgumentInfo> args;
    for (auto& pi : val.argsInfo)
    {
        args.emplace_back(pi.paramName, pi.isMandatory, Value2IntValue(pi.defValue));
    }

    return InternalValue(Callable(Callable::UserCallable, [val, argsInfo = std::move(args)](const CallParams& params, RenderContext& context) -> InternalValue {
        auto ucParams = PrepareUserCallableParams(params, context, argsInfo);
        return Value2IntValue(val.callable(ucParams));
    }));
}

} // namespace visitors

} // namespace jinja2
