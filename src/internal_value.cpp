#include "internal_value.h"
#include "helpers.h"
#include "value_visitors.h"
#include "expression_evaluator.h"
#include "generic_adapters.h"

namespace jinja2
{

InternalValue Value2IntValue(const Value& val);
InternalValue Value2IntValue(Value&& val);

struct SubscriptionVisitor : public visitors::BaseVisitor<>
{
    using BaseVisitor<>::operator ();

    template<typename CharT>
    InternalValue operator() (const MapAdapter& values, const std::basic_string<CharT>& fieldName) const
    {
        auto field = ConvertString<std::string>(fieldName);
        if (!values.HasValue(field))
            return InternalValue();

        return values.GetValueByName(field);
    }

    template<typename CharT>
    InternalValue operator() (const MapAdapter& values, const nonstd::basic_string_view<CharT>& fieldName) const
    {
        auto field = ConvertString<std::string>(fieldName);
        if (!values.HasValue(field))
            return InternalValue();

        return values.GetValueByName(field);
    }

    InternalValue operator() (const ListAdapter& values, int64_t index) const
    {
        if (index < 0 || static_cast<size_t>(index) >= values.GetSize())
            return InternalValue();

        return values.GetValueByIndex(index);
    }

    InternalValue operator() (const MapAdapter& values, int64_t index) const
    {
        return InternalValue();
    }

    template<typename CharT>
    InternalValue operator() (const std::basic_string<CharT>& str, int64_t index) const
    {
        if (index < 0 || static_cast<size_t>(index) >= str.size())
            return InternalValue();

        std::basic_string<CharT> resultStr(1, str[static_cast<size_t>(index)]);
        return TargetString(std::move(resultStr));
    }

    template<typename CharT>
    InternalValue operator() (const nonstd::basic_string_view<CharT>& str, int64_t index) const
    {
        // std::cout << "operator() (const std::basic_string<CharT>& str, int64_t index)" << ": index = " << index << std::endl;
        if (index < 0 || static_cast<size_t>(index) >= str.size())
            return InternalValue();

        std::basic_string<CharT> result(1, str[static_cast<size_t>(index)]);
        return TargetString(std::move(result));
    }

    template<typename CharT>
    InternalValue operator() (const KeyValuePair& values, const std::basic_string<CharT>& fieldName) const
    {
        return SubscriptKvPair(values, ConvertString<std::string>(fieldName));
    }

    template<typename CharT>
    InternalValue operator() (const KeyValuePair& values, const nonstd::basic_string_view<CharT>& fieldName) const
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

std::string AsString(const InternalValue& val)
{
    auto* str = GetIf<std::string>(&val);
    auto* tstr = GetIf<TargetString>(&val);
    if (str != nullptr)
        return *str;
    else /* if (tstr != nullptr) */
    {
        str = GetIf<std::string>(tstr);
        if (str != nullptr)
            return *str;
    }

    return std::string();
}

struct ListConverter : public visitors::BaseVisitor<boost::optional<ListAdapter>>
{
    using BaseVisitor::operator ();

    using result_t = boost::optional<ListAdapter>;


    result_t operator() (const ListAdapter& list) const
    {
        return list;
    }

//    template<typename CharT>
//    result_t operator() (const std::basic_string<CharT>& str) const
//    {
//        return result_t(ListAdapter([adaptor = StringAdapter<CharT>(&str)]() {return &adaptor;}));
//    }

};

ListAdapter ConvertToList(const InternalValue& val, bool& isConverted)
{
    auto result = Apply<ListConverter>(val);
    if (!result)
    {
        isConverted = false;
        return ListAdapter();
    }
    isConverted = true;
    return result.get();
}

ListAdapter ConvertToList(const InternalValue& val, InternalValue subscipt, bool& isConverted)
{
    auto result = Apply<ListConverter>(val);
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
    {}

    const T& Get() const {return *m_val;}
    T& Get() {return *const_cast<T*>(m_val);}
    bool ShouldExtendLifetime() const {return false;}
private:
    const T* m_val;
};

template<typename T>
class ByVal
{
public:
    explicit ByVal(T&& val)
        : m_val(std::move(val))
    {}
    ~ByVal() = default;

    const T& Get() const {return m_val;}
    T& Get() {return m_val;}
    bool ShouldExtendLifetime() const {return false;}
private:
    T m_val;
};

template<typename T>
class BySharedVal
{
public:
    explicit BySharedVal(T&& val)
        : m_val(std::make_shared<T>(std::move(val)))
    {}
    ~BySharedVal() = default;

    const T& Get() const {return *m_val;}
    T& Get() {return *m_val;}
    bool ShouldExtendLifetime() const {return true;}
private:
    std::shared_ptr<T> m_val;
};

template<template<typename> class Holder>
class GenericListAdapter : public IListAccessor
{
public:
    struct Enumerator : public IListAccessorEnumerator
    {
        ListEnumeratorPtr m_enum;

        explicit Enumerator(ListEnumeratorPtr e)
            : m_enum(std::move(e))
        {}

        // Inherited via IListAccessorEnumerator
        void Reset() override
        {
            if (m_enum)
                m_enum->Reset();
        }
        bool MoveNext() override
        {
            return !m_enum ? false : m_enum->MoveNext();
        }
        InternalValue GetCurrent() const override
        {
            return !m_enum ? InternalValue() : Value2IntValue(m_enum->GetCurrent());
        }
        IListAccessorEnumerator *Clone() const override
        {
            return !m_enum ? new Enumerator(MakeEmptyListEnumeratorPtr()) : new Enumerator(m_enum->Clone());
        }
        IListAccessorEnumerator *Transfer() override
        {
            return new Enumerator(std::move(m_enum));
        }
    };

    template<typename U>
    GenericListAdapter(U&& values) : m_values(std::forward<U>(values)) {}

    nonstd::optional<size_t> GetSize() const override {return m_values.Get().GetSize();}
    nonstd::optional<InternalValue> GetItem(int64_t idx) const override
    {
        const ListItemAccessor* accessor = m_values.Get().GetAccessor();
        auto indexer = accessor->GetIndexer();
        if (!indexer)
            return nonstd::optional<InternalValue>();

        const auto& val = indexer->GetItemByIndex(idx);
        return visit(visitors::InputValueConvertor(true, true), val.data()).get();
    }
    bool ShouldExtendLifetime() const override {return m_values.ShouldExtendLifetime();}
    ListAccessorEnumeratorPtr CreateListAccessorEnumerator() const override
    {
        const ListItemAccessor* accessor = m_values.Get().GetAccessor();
        if (!accessor)
            return ListAccessorEnumeratorPtr(new Enumerator(MakeEmptyListEnumeratorPtr()));
        return ListAccessorEnumeratorPtr(new Enumerator(m_values.Get().GetAccessor()->CreateEnumerator()));
    }
    GenericList CreateGenericList() const override
    {
        // return m_values.Get();
        return GenericList([list = m_values]() -> const ListItemAccessor* {return list.Get().GetAccessor();});
    }
private:
    Holder<GenericList> m_values;
};

template<template<typename> class Holder>
class ValuesListAdapter : public IndexedListAccessorImpl<ValuesListAdapter<Holder>>
{
public:
    template<typename U>
    ValuesListAdapter(U&& values) : m_values(std::forward<U>(values)) {}

    size_t GetItemsCountImpl() const {return m_values.Get().size();}
    nonstd::optional<InternalValue> GetItem(int64_t idx) const override
    {
        const auto& val = m_values.Get()[idx];
        return visit(visitors::InputValueConvertor(false, true), val.data()).get();
    }
    bool ShouldExtendLifetime() const override {return m_values.ShouldExtendLifetime();}
    GenericList CreateGenericList() const override
    {
        // return m_values.Get();
        return GenericList([list = *this]() -> const ListItemAccessor* {return &list;});
    }
private:
    Holder<ValuesList> m_values;
};


ListAdapter ListAdapter::CreateAdapter(InternalValueList&& values)
{
    class Adapter : public IndexedListAccessorImpl<Adapter>
    {
    public:
        explicit Adapter(InternalValueList&& values) : m_values(std::move(values)) {}

        size_t GetItemsCountImpl() const {return m_values.size();}
        nonstd::optional<InternalValue> GetItem(int64_t idx) const override {return m_values[static_cast<size_t>(idx)];}
        bool ShouldExtendLifetime() const override {return false;}
        GenericList CreateGenericList() const override
        {
            return GenericList([adapter = *this]() -> const ListItemAccessor* {return &adapter;});
        }
    private:
        InternalValueList m_values;
    };

    return ListAdapter([accessor = Adapter(std::move(values))]() {return &accessor;});
}

ListAdapter ListAdapter::CreateAdapter(const GenericList& values)
{
    return ListAdapter([accessor = GenericListAdapter<ByRef>(values)]() {return &accessor;});
}

ListAdapter ListAdapter::CreateAdapter(const ValuesList& values)
{
    return ListAdapter([accessor = ValuesListAdapter<ByRef>(values)]() {return &accessor;});
}

ListAdapter ListAdapter::CreateAdapter(GenericList&& values)
{
    return ListAdapter([accessor = GenericListAdapter<BySharedVal>(std::move(values))]() {return &accessor;});
}

ListAdapter ListAdapter::CreateAdapter(ValuesList&& values)
{
    return ListAdapter([accessor = ValuesListAdapter<BySharedVal>(std::move(values))]() {return &accessor;});
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
            { }

            void Reset() override
            {
            }

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

            IListAccessorEnumerator* Clone() const override
            {
                auto result = new Enumerator(*this);
                return result;
            }

            IListAccessorEnumerator* Transfer() override
            {
                auto result = new Enumerator(std::move(*this));
                return result;
            }

        protected:
            const GenFn* m_fn;
            InternalValue m_current;
            bool m_isFinished = false;
        };

        explicit Adapter(std::function<nonstd::optional<InternalValue>()>&& fn) : m_fn(std::move(fn)) {}

        nonstd::optional<size_t> GetSize() const override
        {
            return nonstd::optional<size_t>();
        }
        nonstd::optional<InternalValue> GetItem(int64_t idx) const override
        {
            return nonstd::optional<InternalValue>();
        }
        bool ShouldExtendLifetime() const override { return false; }
        ListAccessorEnumeratorPtr CreateListAccessorEnumerator() const override
        {
            return ListAccessorEnumeratorPtr(new Enumerator(&m_fn));
        }

        GenericList CreateGenericList() const override
        {
            return GenericList(); //  return GenericList([adapter = *this]() -> const ListItemAccessor* {return &adapter; });
        }
    private:
        std::function<nonstd::optional<InternalValue>()> m_fn;
    };

    return ListAdapter([accessor = Adapter(std::move(fn))]() {return &accessor;});
}

ListAdapter ListAdapter::CreateAdapter(size_t listSize, std::function<InternalValue (size_t idx)> fn)
{
    using GenFn = std::function<InternalValue(size_t idx)>;

    class Adapter : public IndexedListAccessorImpl<Adapter>
    {
    public:
        explicit Adapter(size_t listSize, GenFn&& fn) : m_listSize(listSize), m_fn(std::move(fn)) {}

        size_t GetItemsCountImpl() const { return m_listSize; }
        nonstd::optional<InternalValue> GetItem(int64_t idx) const override { return m_fn(static_cast<size_t>(idx)); }
        bool ShouldExtendLifetime() const override { return false; }
        GenericList CreateGenericList() const override
        {
            return GenericList([adapter = *this]() -> const ListItemAccessor* {return &adapter; });
        }
    private:
        size_t m_listSize;
        GenFn m_fn;
    };

    return ListAdapter([accessor = Adapter(listSize, std::move(fn))]() {return &accessor; });
}

template<typename Holder>
auto CreateIndexedSubscribedList(Holder&& holder, const InternalValue& subscript, size_t size)
{
    return ListAdapter::CreateAdapter(size, [h = std::forward<Holder>(holder), subscript](size_t idx)->InternalValue {
        return Subscript(h.Get().GetValueByIndex(idx), subscript, nullptr);
    });
}

template<typename Holder>
auto CreateGenericSubscribedList(Holder&& holder, const InternalValue& subscript)
{
    return ListAdapter::CreateAdapter([h = std::forward<Holder>(holder), e = ListAccessorEnumeratorPtr(), isFirst = true, isLast = false, subscript]() mutable {
        using ResultType = nonstd::optional<InternalValue>;
        if (isFirst)
        {
            e = h.Get().GetEnumerator();
            isLast = !e->MoveNext();
            isFirst = false;
        }
        if (isLast)
            return ResultType();

        return ResultType(Subscript(e->GetCurrent(), subscript, nullptr));
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

template<template<typename> class Holder, bool CanModify>
class InternalValueMapAdapter : public MapAccessorImpl<InternalValueMapAdapter<Holder, CanModify>>
{
public:
    template<typename U>
    InternalValueMapAdapter(U&& values) : m_values(std::forward<U>(values)) {}

    size_t GetSize() const override {return m_values.Get().size();}
    bool HasValue(const std::string& name) const override
    {
        return m_values.Get().count(name) != 0;
    }
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
    bool ShouldExtendLifetime() const override {return m_values.ShouldExtendLifetime();}
    GenericMap CreateGenericMap() const override
    {
        return GenericMap([accessor = *this]() -> const MapItemAccessor* {return &accessor;});
    }
private:
    Holder<InternalValueMap> m_values;
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
    GenericMapAdapter(U&& values) : m_values(std::forward<U>(values)) {}

    size_t GetSize() const override {return m_values.Get().GetSize();}
    bool HasValue(const std::string& name) const override
    {
        return m_values.Get().HasValue(name);
    }
    InternalValue GetItem(const std::string& name) const override
    {
        auto val = m_values.Get().GetValueByName(name);
        if (val.isEmpty())
            return InternalValue();

        return Value2IntValue(std::move(val));
    }
    std::vector<std::string> GetKeys() const override
    {
        return m_values.Get().GetKeys();
    }
    bool ShouldExtendLifetime() const override {return m_values.ShouldExtendLifetime();}
    GenericMap CreateGenericMap() const override
    {
        return GenericMap([accessor = *this]() -> const MapItemAccessor* {return accessor.m_values.Get().GetAccessor();});
    }

private:
    Holder<GenericMap> m_values;
};


template<template<typename> class Holder>
class ValuesMapAdapter : public MapAccessorImpl<ValuesMapAdapter<Holder>>
{
public:
    template<typename U>
    ValuesMapAdapter(U&& values) : m_values(std::forward<U>(values)) {}

    size_t GetSize() const override {return m_values.Get().size();}
    bool HasValue(const std::string& name) const override
    {
        return m_values.Get().count(name) != 0;
    }
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
    bool ShouldExtendLifetime() const override {return m_values.ShouldExtendLifetime();}
    GenericMap CreateGenericMap() const override
    {
        return GenericMap([accessor = *this]() -> const MapItemAccessor* {return &accessor;});
    }
private:
    Holder<ValuesMap> m_values;
};

MapAdapter CreateMapAdapter(InternalValueMap&& values)
{
    return MapAdapter([accessor = InternalValueMapAdapter<ByVal, true>(std::move(values))]() mutable {return &accessor;});
}


MapAdapter CreateMapAdapter(const InternalValueMap* values)
{
    return MapAdapter([accessor = InternalValueMapAdapter<ByRef, false>(*values)]() mutable {return &accessor;});
}


MapAdapter CreateMapAdapter(const GenericMap& values)
{
    return MapAdapter([accessor = GenericMapAdapter<ByRef>(values)]() mutable {return &accessor;});
}


MapAdapter CreateMapAdapter(GenericMap&& values)
{
    return MapAdapter([accessor = GenericMapAdapter<BySharedVal>(std::move(values))]() mutable {return &accessor;});
}


MapAdapter CreateMapAdapter(const ValuesMap& values)
{
    return MapAdapter([accessor = ValuesMapAdapter<ByRef>(values)]() mutable {return &accessor;});
}


MapAdapter CreateMapAdapter(ValuesMap&& values)
{
    return MapAdapter([accessor = ValuesMapAdapter<BySharedVal>(std::move(values))]() mutable {return &accessor;});
}

struct OutputValueConvertor
{
    using result_t = Value;

    result_t operator()(const EmptyValue&) const {return result_t();}
    result_t operator()(const MapAdapter& adapter) const {return result_t(adapter.CreateGenericMap());}
    result_t operator()(const ListAdapter& adapter) const {return result_t(adapter.CreateGenericList());}
    result_t operator()(const ValueRef& ref) const
    {
        return ref.get();
    }
    result_t operator()(const TargetString& str) const
    {
        switch (str.index())
        {
        case 0:
            return str.get<std::string>();
        default:
            return str.get<std::wstring>();
        }
    }
    result_t operator()(const TargetStringView& str) const
    {
        switch (str.index())
        {
        case 0:
            return str.get<nonstd::string_view>();
        default:
            return str.get<nonstd::wstring_view>();
        }
    }
    result_t operator()(const KeyValuePair& pair) const
    {
        return ValuesMap{{"key", Value(pair.key)}, {"value", IntValue2Value(pair.value)}};
    }
    result_t operator()(const Callable&) const {return result_t();}
    result_t operator()(const UserCallable&) const {return result_t();}
    result_t operator()(const std::shared_ptr<RendererBase>&) const {return result_t();}

    template<typename T>
    result_t operator()(const RecWrapper<T>& val) const
    {
        return this->operator()(const_cast<const T&>(*val.get()));
    }

    template<typename T>
    result_t operator()(RecWrapper<T>& val) const
    {
        return this->operator()(*val.get());
    }

    template<typename T>
    result_t operator() (T&& val) const
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

UserCallableParams PrepareUserCallableParams(const CallParams& params, RenderContext& context, const std::vector<ArgumentInfo>& argsInfo)
{
    UserCallableParams result;

    ParsedArguments args = helpers::ParseCallParams(argsInfo, params, result.paramsParsed);
    if (!result.paramsParsed)
        return result;

    for (auto& argInfo : argsInfo)
    {
        if (argInfo.name == "*args" || argInfo.name == "**kwargs")
            continue;

        auto p = args.args.find(argInfo.name);
        if (p == args.args.end())
        {
            result.args[argInfo.name] = IntValue2Value(argInfo.defaultVal);
            continue;
        }

        const auto& v = p->second->Evaluate(context);
        result.args[argInfo.name] = IntValue2Value(v);
    }

    ValuesMap extraKwArgs;
    for (auto p : args.extraKwArgs)
        extraKwArgs[p.first] = IntValue2Value(p.second->Evaluate(context));
    result.extraKwArgs = Value(std::move(extraKwArgs));

    ValuesList extraPosArgs;
    for (auto p : args.extraPosArgs)
        extraPosArgs.push_back(IntValue2Value(p->Evaluate(context)));
    result.extraPosArgs = Value(std::move(extraPosArgs));

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

} // visitors

} // jinja2
