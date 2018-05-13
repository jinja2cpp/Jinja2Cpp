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

ListAdapter ConvertToList(const InternalValue& val, bool& isConverted)
{
    return ListAdapter();
}

ListAdapter ConvertToList(const InternalValue& val, InternalValue subscipt, bool& isConverted)
{
    return ListAdapter();
}

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
    class Adapter : public IListAccessor
    {
    public:
        explicit Adapter(const GenericList& values) : m_values(&values) {}

        size_t GetSize() const override {return m_values->GetSize();}
        InternalValue GetValueByIndex(int64_t idx) const override
        {
            auto val = m_values->GetValueByIndex(idx);
            return boost::apply_visitor(visitors::InputValueConvertor(true), val.data()).get();
        }
    private:
        const GenericList* m_values;
    };

    return ListAdapter([accessor = Adapter(std::move(values))]() {return &accessor;});
}

ListAdapter ListAdapter::CreateAdapter(const ValuesList& values)
{
    return ListAdapter();
}

ListAdapter ListAdapter::ToSubscriptedList(const InternalValue& Subscript) const
{
    return ListAdapter();
}

InternalValueList ListAdapter::ToValueList() const
{
    return InternalValueList();
}

MapAdapter MapAdapter::CreateAdapter(const InternalValue& from)
{
    return MapAdapter();
}

MapAdapter MapAdapter::CreateAdapter(InternalValue&& from)
{
    return MapAdapter();
}

MapAdapter MapAdapter::CreateAdapter(InternalValueMap&& values)
{
    return MapAdapter();
}

MapAdapter MapAdapter::CreateAdapter(const GenericMap& values)
{
    return MapAdapter();
}

MapAdapter MapAdapter::CreateAdapter(const ValuesMap& values)
{
    return MapAdapter();
}

} // jinja2
