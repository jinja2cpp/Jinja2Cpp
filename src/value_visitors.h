#ifndef VALUE_VISITORS_H
#define VALUE_VISITORS_H

#include "expression_evaluator.h"
#include "jinja2cpp/value.h"

#include <boost/algorithm/string/predicate.hpp>

#include <iostream>
#include <cmath>
#include <limits>

namespace jinja2
{
namespace visitors
{
template<typename R = Value>
struct BaseVisitor : public boost::static_visitor<R>
{
    template<typename T, typename U>
    Value operator() (T&&, U&&) const
    {
        return Value();
    }
};


template<typename CharT>
struct ValueRendererBase : public boost::static_visitor<>
{
    ValueRendererBase(std::basic_ostream<CharT>& os)
        : m_os(&os)
    {
    }

    template<typename T>
    void operator()(const T& val) const
    {
        (*m_os) << val;
    }

    void operator()(const EmptyValue&) const {}
    void operator()(const ValuesList&) const {}
    void operator()(const ValuesMap&) const {}
    void operator()(const GenericMap&) const {}
    void operator()(const GenericList&) const {}

    std::basic_ostream<CharT>* m_os;
};

template<typename CharT>
struct ValueRenderer;

template<>
struct ValueRenderer<char> : ValueRendererBase<char>
{
    ValueRenderer(std::ostream& os)
        : ValueRendererBase<char>::ValueRendererBase<char>(os)
    {
    }

    using ValueRendererBase<char>::operator ();
    void operator()(const std::wstring&) const {}
    void operator() (bool val) const
    {
        (*m_os) << (val ? "true" : "false");
    }
};

template<>
struct ValueRenderer<wchar_t> : ValueRendererBase<wchar_t>
{
    ValueRenderer(std::wostream& os)
        : ValueRendererBase<wchar_t>::ValueRendererBase<wchar_t>(os)
    {
    }

    using ValueRendererBase<wchar_t>::operator ();
    void operator()(const std::string&) const
    {
    }
    void operator() (bool val) const
    {
        (*m_os) << (val ? L"true" : L"false");
    }
};

struct UnaryOperation : boost::static_visitor<Value>
{
    UnaryOperation(UnaryExpression::Operation oper)
        : m_oper(oper)
    {
    }

    Value operator() (int64_t val) const
    {
        Value result;
        switch (m_oper)
        {
        case jinja2::UnaryExpression::LogicalNot:
            result = val ? false : true;
            break;
        case jinja2::UnaryExpression::UnaryPlus:
            result = +val;
            break;
        case jinja2::UnaryExpression::UnaryMinus:
            result = -val;
            break;
        }

        return result;
    }

    Value operator() (double val) const
    {
        Value result;
        switch (m_oper)
        {
        case jinja2::UnaryExpression::LogicalNot:
            result = val ? false : true;
            break;
        case jinja2::UnaryExpression::UnaryPlus:
            result = +val;
            break;
        case jinja2::UnaryExpression::UnaryMinus:
            result = -val;
            break;
        }

        return result;
    }

    Value operator() (bool val) const
    {
        Value result;
        switch (m_oper)
        {
        case jinja2::UnaryExpression::LogicalNot:
            result = !val;
            break;
        default:
            break;
        }

        return result;
    }

    Value operator() (const GenericMap&) const
    {
        Value result;
        switch (m_oper)
        {
        case jinja2::UnaryExpression::LogicalNot:
            result = true;
            break;
        default:
            break;
        }

        return result;
    }

    Value operator() (const GenericList&) const
    {
        Value result;
        switch (m_oper)
        {
        case jinja2::UnaryExpression::LogicalNot:
            result = true;
            break;
        default:
            break;
        }

        return result;
    }

    Value operator() (const EmptyValue&) const
    {
        Value result;
        switch (m_oper)
        {
        case jinja2::UnaryExpression::LogicalNot:
            result = true;
            break;
        default:
            break;
        }

        return result;
    }

    template<typename T>
    Value operator() (const T& val) const
    {
        Value result;
        switch (m_oper)
        {
        case jinja2::UnaryExpression::LogicalNot:
            result = val.empty();
            break;
        default:
            break;
        }

        return result;
    }

    UnaryExpression::Operation m_oper;
};

struct BinaryMathOperation : BaseVisitor<>
{
    using BaseVisitor::operator ();

    bool AlmostEqual(double x, double y) const
    {
        return std::abs(x - y) <= std::numeric_limits<double>::epsilon() * std::abs(x + y) * 6
               || std::abs(x - y) < std::numeric_limits<double>::min();
    }

    BinaryMathOperation(BinaryExpression::Operation oper, BinaryExpression::CompareType compType = BinaryExpression::CaseSensitive)
        : m_oper(oper)
        , m_compType(compType)
    {
    }

    Value operator() (double left, double right) const
    {
        Value result = 0;
        switch (m_oper)
        {
        case jinja2::BinaryExpression::Plus:
            result = left + right;
            break;
        case jinja2::BinaryExpression::Minus:
            result = left - right;
            break;
        case jinja2::BinaryExpression::Mul:
            result = left * right;
            break;
        case jinja2::BinaryExpression::Div:
            result = left / right;
            break;
        case jinja2::BinaryExpression::DivReminder:
            result = std::remainder(left, right);
            break;
        case jinja2::BinaryExpression::DivInteger:
        {
            double val = left / right;
            result = val < 0 ? ceil(val) : floor(val);
            break;
        }
        case jinja2::BinaryExpression::Pow:
            result = pow(left, right);
            break;
        case jinja2::BinaryExpression::LogicalEq:
            result = AlmostEqual(left, right);
            break;
        case jinja2::BinaryExpression::LogicalNe:
            result = !AlmostEqual(left, right);
            break;
        case jinja2::BinaryExpression::LogicalGt:
            result = left > right;
            break;
        case jinja2::BinaryExpression::LogicalLt:
            result = left < right;
            break;
        case jinja2::BinaryExpression::LogicalGe:
            result = left > right || AlmostEqual(left, right);
            break;
        case jinja2::BinaryExpression::LogicalLe:
            result = left < right || AlmostEqual(left, right);
            break;
        default:
            break;
        }

        return result;
    }

    Value operator() (int64_t left, int64_t right) const
    {
        Value result;
        switch (m_oper)
        {
        case jinja2::BinaryExpression::Plus:
            result = left + right;
            break;
        case jinja2::BinaryExpression::Minus:
            result = left - right;
            break;
        case jinja2::BinaryExpression::Mul:
            result = left * right;
            break;
        case jinja2::BinaryExpression::DivInteger:
            result = left / right;
            break;
        case jinja2::BinaryExpression::Div:
        case jinja2::BinaryExpression::DivReminder:
        case jinja2::BinaryExpression::Pow:
            result = this->operator ()(static_cast<double>(left), static_cast<double>(right));
            break;
        case jinja2::BinaryExpression::LogicalEq:
            result = left == right;
            break;
        case jinja2::BinaryExpression::LogicalNe:
            result = left != right;
            break;
        case jinja2::BinaryExpression::LogicalGt:
            result = left > right;
            break;
        case jinja2::BinaryExpression::LogicalLt:
            result = left < right;
            break;
        case jinja2::BinaryExpression::LogicalGe:
            result = left >= right;
            break;
        case jinja2::BinaryExpression::LogicalLe:
            result = left <= right;
            break;
        default:
            break;
        }

        return result;
    }

    Value operator() (double left, int64_t right) const
    {
        return this->operator ()(static_cast<double>(left), static_cast<double>(right));
    }

    Value operator() (int64_t left, double right) const
    {
        return this->operator ()(static_cast<double>(left), static_cast<double>(right));
    }

    template<typename CharT>
    Value operator() (const std::basic_string<CharT>& left, const std::basic_string<CharT>& right) const
    {
        Value result;
        switch (m_oper)
        {
        case jinja2::BinaryExpression::Plus:
            result = left + right;
            break;
        case jinja2::BinaryExpression::LogicalEq:
            result = m_compType == BinaryExpression::CaseSensitive ? left == right : boost::iequals(left, right);
            break;
        case jinja2::BinaryExpression::LogicalNe:
            result = m_compType == BinaryExpression::CaseSensitive ? left != right : !boost::iequals(left, right);
            break;
        case jinja2::BinaryExpression::LogicalGt:
            result = m_compType == BinaryExpression::CaseSensitive ? left > right : boost::lexicographical_compare(right, left, boost::algorithm::is_iless());
            break;
        case jinja2::BinaryExpression::LogicalLt:
            result = m_compType == BinaryExpression::CaseSensitive ? left < right : boost::lexicographical_compare(left, right, boost::algorithm::is_iless());
            break;
        case jinja2::BinaryExpression::LogicalGe:
            if (m_compType == BinaryExpression::CaseSensitive)
            {
                result = left >= right;
            }
            else
            {
                result = boost::iequals(left, right) ? true : boost::lexicographical_compare(right, left, boost::algorithm::is_iless());
            }
            break;
        case jinja2::BinaryExpression::LogicalLe:
            if (m_compType == BinaryExpression::CaseSensitive)
            {
                result = left <= right;
            }
            else
            {
                result = boost::iequals(left, right) ? true : boost::lexicographical_compare(left, right, boost::algorithm::is_iless());
            }
            break;
        default:
            break;
        }

        return result;
    }

    Value operator() (bool left, bool right)
    {
        Value result;
        switch (m_oper)
        {
        case jinja2::BinaryExpression::LogicalEq:
            result = left == right;
            break;
        case jinja2::BinaryExpression::LogicalNe:
            result = left != right;
            break;
        default:
            break;
        }

        return result;
    }

    Value operator() (EmptyValue, EmptyValue) const
    {
        Value result;
        switch (m_oper)
        {
        case jinja2::BinaryExpression::LogicalEq:
            result = true;
            break;
        case jinja2::BinaryExpression::LogicalNe:
            result = false;
            break;
        default:
            break;
        }

        return result;
    }

    template<typename T>
    Value operator() (EmptyValue, T&&) const
    {
        Value result;
        switch (m_oper)
        {
        case jinja2::BinaryExpression::LogicalEq:
            result = false;
            break;
        case jinja2::BinaryExpression::LogicalNe:
            result = true;
            break;
        default:
            break;
        }

        return result;
    }

    template<typename T>
    Value operator() (T&&, EmptyValue) const
    {
        Value result;
        switch (m_oper)
        {
        case jinja2::BinaryExpression::LogicalEq:
            result = false;
            break;
        case jinja2::BinaryExpression::LogicalNe:
            result = true;
            break;
        default:
            break;
        }

        return result;
    }

    BinaryExpression::Operation m_oper;
    BinaryExpression::CompareType m_compType;
};

struct BooleanEvaluator : boost::static_visitor<bool>
{
    bool operator() (int64_t val) const
    {
        return val != 0;
    }

    bool operator() (double val) const
    {
        return fabs(val) < std::numeric_limits<double>::epsilon();
    }

    bool operator() (bool val) const
    {
        return val;
    }

    bool operator() (const GenericMap&) const
    {
        return true;
    }

    bool operator() (const GenericList&) const
    {
        return true;
    }

    bool operator() (const EmptyValue&) const
    {
        return false;
    }

    template<typename T>
    bool operator() (const T& val) const
    {
        return !val.empty();
    }
};

struct IntegerEvaluator : public boost::static_visitor<int64_t>
{
    int64_t operator ()(int64_t val) const
    {
        return val;
    }
    int64_t operator ()(double val) const
    {
        return static_cast<int64_t>(val);
    }
    int64_t operator ()(bool val) const
    {
        return static_cast<int64_t>(val);
    }
    template<typename U>
    int64_t operator()(U&&) const
    {
        return 0;
    }
};


struct ListEvaluator : boost::static_visitor<ValuesList>
{
    ListEvaluator(Value attr = Value())
        : m_attr(std::move(attr))
    {}

    ValuesList operator() (const ValuesList& values) const
    {
        if (m_attr.isEmpty())
            return values;

        ValuesList result;
        std::transform(values.begin(), values.end(), std::back_inserter(result), [this](const Value& val) {return val.subscript(m_attr);});
        return result;
    }

    ValuesList operator() (const GenericList& values) const
    {
        int64_t size = values.GetSize();

        ValuesList result;
        for (int64_t idx = 0; idx < size; ++ idx)
        {
            auto val = values.GetValueByIndex(idx);
            if (!m_attr.isEmpty())
                result.push_back(val.subscript(m_attr));
            else
                result.push_back(val);
        }

        return result;
    }

    template<typename U>
    ValuesList operator() (U&&) const
    {
        return ValuesList();
    }

    Value m_attr;
};

struct StringJoiner : BaseVisitor<>
{
    using BaseVisitor::operator ();

    Value operator() (EmptyValue, const std::string& str) const
    {
        return str;
    }

    Value operator() (const std::string& left, const std::string& right) const
    {
        return left + right;
    }
};

} // visitors

template<typename V, typename ValType, typename ... Args>
auto Apply(ValType&& val, Args&& ... args)
{
    return boost::apply_visitor(V(std::forward<Args>(args)...), std::forward<ValType>(val).data());
}

template<typename V, typename ValType, typename ... Args>
auto Apply(ValType&& val1, ValType&& val2, Args&& ... args)
{
    return boost::apply_visitor(V(std::forward<Args>(args)...), std::forward<ValType>(val1).data(), std::forward<ValType>(val2).data());
}

} // jinja2

#endif // VALUE_VISITORS_H
