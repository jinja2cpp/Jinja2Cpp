#include "jinja2cpp/value.h"
#include <sstream>

namespace jinja2
{
template<typename T>
std::string toString(T val)
{
    std::ostringstream os;
    os << val;
    return os.str();
}

namespace
{
struct ValueRenderer : boost::static_visitor<std::string>
{
    std::string operator() (bool val) const
    {
        return val ? "True" : "False";
    }
    std::string operator() (const EmptyValue&) const
    {
        return std::string();
    }
    std::string operator() (const std::wstring&) const
    {
        return std::string();
    }

    std::string operator() (const ValuesList& vals) const
    {
        std::string result = "{";
        bool isFirst = true;
        for (auto& val : vals)
        {
            if (isFirst)
                isFirst = false;
            else
                result += ", ";

            result += boost::apply_visitor(ValueRenderer(), val.data());
        }
        result += "}";
        return result;
    }

    std::string operator() (const ValuesMap& vals) const
    {
        std::string result = "{";
        bool isFirst = true;
        for (auto& val : vals)
        {
            if (isFirst)
                isFirst = false;
            else
                result += ", ";

            result += "{\"" + val.first + "\",";
            result += boost::apply_visitor(ValueRenderer(), val.second.data());
            result += "}";
        }
        result += "}";
        return result;
    }

    std::string operator() (const GenericMap& /*val*/) const
    {
        return "";
    }

    std::string operator() (const GenericList& /*val*/) const
    {
        return "";
    }

    template<typename T>
    std::string operator() (const T& val) const
    {
        return toString(val);
    }
};

struct SubscriptionVisitor : public boost::static_visitor<Value>
{
    Value operator() (const ValuesMap& values, const std::string& field) const
    {
        auto p = values.find(field);
        if (p == values.end())
            return Value();

        return p->second;
    }

    Value operator() (const GenericMap& values, const std::string& field) const
    {
        if (!values.HasValue(field))
            return Value();

        return values.GetValueByName(field);
    }

    Value operator() (const GenericMap& values, const int64_t index) const
    {
        if (index < 0 || static_cast<size_t>(index) >= values.GetSize())
            return Value();

        return values.GetValueByIndex(index);
    }

    Value operator() (const ValuesList& values, int64_t index) const
    {
        if (index < 0 || static_cast<size_t>(index) >= values.size())
            return Value();

        return values[index];
    }

    Value operator() (const GenericList& values, const int64_t index) const
    {
        if (index < 0 || static_cast<size_t>(index) >= values.GetSize())
            return Value();

        return values.GetValueByIndex(index);
    }

    template<typename T, typename U>
    Value operator() (T&& /*first*/, U&& /*second*/) const
    {
        return Value();
    }
};

} //

Value Value::subscript(const Value& index) const
{
    return boost::apply_visitor(SubscriptionVisitor(), m_data, index.m_data);
}

} // jinja2
