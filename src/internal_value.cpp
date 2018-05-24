#include "internal_value.h"
#include "value_visitors.h"

namespace jinja2
{

struct SubscriptionVisitor : public visitors::BaseVisitor<>
{
    using BaseVisitor::operator ();

    InternalValue operator() (const MapAdapter& values, const std::string& field) const
    {
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
        if (index < 0 || static_cast<size_t>(index) >= values.GetSize())
            return InternalValue();

        return values.GetValueByIndex(index);
    }

    template<typename CharT>
    InternalValue operator() (const std::basic_string<CharT>& str, int64_t index) const
    {
        if (index < 0 || static_cast<size_t>(index) >= str.size())
            return InternalValue();

        std::basic_string<CharT> result(1, str[static_cast<size_t>(index)]);
        return TargetString(std::move(result));
    }

    InternalValue operator() (const KeyValuePair& values, const std::string& field)
    {
        if (field == "key")
            return InternalValue(values.key);
        else if (field == "value")
            return values.value;

        return InternalValue();
    }
};

InternalValue Subscript(const InternalValue& val, const InternalValue& subscript)
{
    return Apply2<SubscriptionVisitor>(val, subscript);
}

InternalValue Subscript(const InternalValue& val, const std::string& subscript)
{
    return Apply2<SubscriptionVisitor>(val, InternalValue(subscript));
}

std::string AsString(const InternalValue& val)
{
    auto* str = boost::get<std::string>(&val);
    auto* tstr = boost::get<TargetString>(&val);
    if (str != nullptr)
        return *str;
    else
    {
        str = boost::get<std::string>(tstr);
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
        return result.get();

    return result.get().ToSubscriptedList(subscipt, false);
}

template<typename T>
class ByRef
{
public:
    ByRef(const T& val)
        : m_val(&val)
    {}

    const T& Get() const {return *m_val;}
private:
    const T* m_val;
};

template<typename T>
class ByVal
{
public:
    ByVal(T&& val)
        : m_val(std::move(val))
    {}

    const T& Get() const {return m_val;}
private:
    T m_val;
};

template<template<typename> class Holder>
class GenericListAdapter : public IListAccessor
{
public:
    template<typename U>
    GenericListAdapter(U&& values) : m_values(std::forward<U>(values)) {}

    size_t GetSize() const override {return m_values.Get().GetSize();}
    InternalValue GetValueByIndex(int64_t idx) const override
    {
        auto val = m_values.Get().GetValueByIndex(idx);
        return boost::apply_visitor(visitors::InputValueConvertor(true), val.data()).get();
    }
private:
    Holder<GenericList> m_values;
};

template<template<typename> class Holder>
class ValuesListAdapter : public IListAccessor
{
public:
    template<typename U>
    ValuesListAdapter(U&& values) : m_values(std::forward<U>(values)) {}

    size_t GetSize() const override {return m_values.Get().size();}
    InternalValue GetValueByIndex(int64_t idx) const override
    {
        auto val = m_values.Get()[idx];
        return boost::apply_visitor(visitors::InputValueConvertor(false), val.data()).get();
    }
private:
    Holder<ValuesList> m_values;
};


ListAdapter ListAdapter::CreateAdapter(InternalValueList&& values)
{
    class Adapter : public IListAccessor
    {
    public:
        explicit Adapter(InternalValueList&& values) : m_values(std::move(values)) {}

        size_t GetSize() const override {return m_values.size();}
        InternalValue GetValueByIndex(int64_t idx) const override {return m_values[static_cast<size_t>(idx)];}
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
    return ListAdapter([accessor = GenericListAdapter<ByVal>(std::move(values))]() {return &accessor;});
}

ListAdapter ListAdapter::CreateAdapter(ValuesList&& values)
{
    return ListAdapter([accessor = ValuesListAdapter<ByVal>(std::move(values))]() {return &accessor;});
}

template<template<typename> class Holder>
class SubscriptedListAdapter : public IListAccessor
{
public:
    template<typename U>
    SubscriptedListAdapter(U&& values, const InternalValue& subscript) : m_values(std::forward<U>(values)), m_subscript(subscript) {}

    size_t GetSize() const override {return m_values.Get().GetSize();}
    InternalValue GetValueByIndex(int64_t idx) const override
    {
        return Subscript(m_values.Get().GetValueByIndex(idx), m_subscript);
    }
private:
    Holder<ListAdapter> m_values;
    InternalValue m_subscript;
};

ListAdapter ListAdapter::ToSubscriptedList(const InternalValue& subscript, bool asRef) const
{
    if (asRef)
        return ListAdapter([accessor = SubscriptedListAdapter<ByRef>(*this, subscript)]() {return &accessor;});

    ListAdapter tmp(*this);
    return ListAdapter([accessor = SubscriptedListAdapter<ByVal>(std::move(tmp), subscript)]() {return &accessor;});
}

InternalValueList ListAdapter::ToValueList() const
{
    InternalValueList result;
    std::copy(begin(), end(), std::back_inserter(result));
    return result;
}

template<template<typename> class Holder>
class InternalValueMapAdapter : public IMapAccessor
{
public:
    template<typename U>
    InternalValueMapAdapter(U&& values) : m_values(std::forward<U>(values)) {}

    size_t GetSize() const override {return m_values.Get().size();}
    InternalValue GetValueByIndex(int64_t idx) const override
    {
        KeyValuePair result;
        auto p = m_values.Get().begin();
        std::advance(p, idx);

        result.key = p->first;
        result.value = p->second;

        return InternalValue(std::move(result));
    }
    bool HasValue(const std::string& name) const override
    {
        return m_values.Get().count(name) != 0;
    }
    InternalValue GetValueByName(const std::string& name) const override
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
private:
    Holder<InternalValueMap> m_values;
};

InternalValue Value2IntValue(const Value& val)
{
    auto result = boost::apply_visitor(visitors::InputValueConvertor(false), val.data());
    if (result)
        return result.get();

    return InternalValue(ValueRef(val));
}

InternalValue Value2IntValue(Value&& val)
{
    auto result = boost::apply_visitor(visitors::InputValueConvertor(true), val.data());
    if (result)
        return result.get();

    return InternalValue(ValueRef(val));
}

template<template<typename> class Holder>
class GenericMapAdapter : public IMapAccessor
{
public:
    template<typename U>
    GenericMapAdapter(U&& values) : m_values(std::forward<U>(values)) {}

    size_t GetSize() const override {return m_values.Get().GetSize();}
    InternalValue GetValueByIndex(int64_t idx) const override
    {
        auto val = m_values.Get().GetValueByIndex(idx);
        KeyValuePair result;
        auto& map = val.asMap();
        result.key = map["key"].asString();
        result.value = Value2IntValue(std::move(map["value"]));
        return result;
    }
    bool HasValue(const std::string& name) const override
    {
        return m_values.Get().HasValue(name);
    }
    InternalValue GetValueByName(const std::string& name) const override
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

private:
    Holder<GenericMap> m_values;
};


template<template<typename> class Holder>
class ValuesMapAdapter : public IMapAccessor
{
public:
    template<typename U>
    ValuesMapAdapter(U&& values) : m_values(std::forward<U>(values)) {}

    size_t GetSize() const override {return m_values.Get().size();}
    InternalValue GetValueByIndex(int64_t idx) const override
    {
        KeyValuePair result;
        auto p = m_values.Get().begin();
        std::advance(p, idx);

        result.key = p->first;
        result.value = Value2IntValue(p->second);

        return result;
    }
    bool HasValue(const std::string& name) const override
    {
        return m_values.Get().count(name) != 0;
    }
    InternalValue GetValueByName(const std::string& name) const override
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
private:
    Holder<ValuesMap> m_values;
};


MapAdapter MapAdapter::CreateAdapter(InternalValueMap&& values)
{
    return MapAdapter([accessor = InternalValueMapAdapter<ByVal>(std::move(values))]() {return &accessor;});
}

MapAdapter MapAdapter::CreateAdapter(const InternalValueMap* values)
{
    return MapAdapter([accessor = InternalValueMapAdapter<ByRef>(*values)]() {return &accessor;});
}

MapAdapter MapAdapter::CreateAdapter(const GenericMap& values)
{
    return MapAdapter([accessor = GenericMapAdapter<ByRef>(values)]() {return &accessor;});
}

MapAdapter MapAdapter::CreateAdapter(GenericMap&& values)
{
    return MapAdapter([accessor = GenericMapAdapter<ByVal>(std::move(values))]() {return &accessor;});
}

MapAdapter MapAdapter::CreateAdapter(const ValuesMap& values)
{
    return MapAdapter([accessor = ValuesMapAdapter<ByRef>(values)]() {return &accessor;});
}

MapAdapter MapAdapter::CreateAdapter(ValuesMap&& values)
{
    return MapAdapter([accessor = ValuesMapAdapter<ByVal>(std::move(values))]() {return &accessor;});
}

} // jinja2
