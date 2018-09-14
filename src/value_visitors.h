#ifndef VALUE_VISITORS_H
#define VALUE_VISITORS_H

#include "expression_evaluator.h"
#include "jinja2cpp/value.h"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/optional.hpp>

#include <iostream>
#include <cmath>
#include <limits>
#include <utility>

namespace jinja2
{

namespace detail
{

template<typename V>
struct RecursiveUnwrapper
{
    V* m_visitor;

    RecursiveUnwrapper(V* v)
        : m_visitor(v)
    {}


    template<typename T>
    static auto& UnwrapRecursive(T&& arg)
    {
        return std::forward<T>(arg);
    }

    template<typename T>
    static auto& UnwrapRecursive(const RecursiveWrapper<T>& arg)
    {
        return arg.GetValue();
    }

    template<typename T>
    static auto& UnwrapRecursive(RecursiveWrapper<T>& arg)
    {
        return arg.GetValue();
    }

    template<typename ... Args>
    auto operator()(Args&& ... args) const
    {
        assert(m_visitor != nullptr);
        return (*m_visitor)(UnwrapRecursive(std::forward<Args>(args))...);
    }
};

template<typename Fn>
auto ApplyUnwrapped(const InternalValue& val, Fn&& fn)
{
    auto valueRef = GetIf<ValueRef>(&val);
    auto targetString = GetIf<TargetString>(&val);
    // auto internalValueRef = GetIf<InternalValueRef>(&val);

    if (valueRef != nullptr)
        return fn(valueRef->get().data());
    else if (targetString != nullptr)
        return fn(*targetString);
//    else if (internalValueRef != nullptr)
//        return fn(internalValueRef->get());

    return fn(val);
}
} // detail

template<typename V, typename ... Args>
auto Apply(const InternalValue& val, Args&& ... args)
{
    return detail::ApplyUnwrapped(val, [&args...](auto& val) {
        auto v = V(args...);
        return nonstd::visit(detail::RecursiveUnwrapper<V>(&v), val);
    });
}

template<typename V, typename ... Args>
auto Apply2(const InternalValue& val1, const InternalValue& val2, Args&& ... args)
{
    return detail::ApplyUnwrapped(val1, [&val2, &args...](auto& uwVal1) {
        return detail::ApplyUnwrapped(val2, [&uwVal1, &args...](auto& uwVal2) {
            auto v = V(args...);
            return nonstd::visit(detail::RecursiveUnwrapper<V>(&v), uwVal1, uwVal2);
        });
    });
}

bool ConvertToBool(const InternalValue& val);

namespace visitors
{
template<typename R = InternalValue>
struct BaseVisitor
{
    R operator() (const GenericMap&) const
    {
        assert(false);
        return R();
    }

    R operator() (const GenericList&) const
    {
        assert(false);
        return R();
    }

    R operator() (const ValueRef&) const
    {
        assert(false);
        return R();
    }

    R operator() (const TargetString&) const
    {
        assert(false);
        return R();
    }

    template<typename T>
    R operator() (T&&) const
    {
        return R();
    }

    template<typename T, typename U>
    R operator() (T&&, U&&) const
    {
        return R();
    }
};


template<typename CharT>
struct ValueRendererBase
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
    void operator()(const MapAdapter&) const {}
    void operator()(const ListAdapter&) const {}
    void operator()(const ValueRef&) const {}
    void operator()(const TargetString&) const {}
    void operator()(const KeyValuePair&) const {}
    void operator()(const Callable&) const {}
    void operator()(const UserFunction&) const {}
    void operator()(const RendererBase*) const {}
    template<typename T>
    void operator()(const boost::recursive_wrapper<T>&) const {}

    std::basic_ostream<CharT>* m_os;
};

struct InputValueConvertor
{
    using result_t = boost::optional<InternalValue>;

    InputValueConvertor(bool byValue = false)
        : m_byValue(byValue)
    {
    }

    template<typename ChT>
    result_t operator() (const std::basic_string<ChT>& val) const
    {
        // if (m_byValue)
        return result_t(InternalValue(TargetString(val)));

        // return result_t();
    }

    result_t operator() (const ValuesList& vals) const
    {
        if (m_byValue)
        {
            ValuesList newVals(vals);
            return result_t(InternalValue(ListAdapter::CreateAdapter(std::move(newVals))));
        }

        return result_t(InternalValue(ListAdapter::CreateAdapter(vals)));
    }

    result_t operator() (ValuesList& vals) const
    {
        return result_t(InternalValue(ListAdapter::CreateAdapter(std::move(vals))));
    }

    result_t operator() (const GenericList& vals) const
    {
        if (m_byValue)
        {
            GenericList newVals(vals);
            return result_t(InternalValue(ListAdapter::CreateAdapter(std::move(newVals))));
        }

        return result_t(InternalValue(ListAdapter::CreateAdapter(vals)));
    }

    result_t operator() (GenericList& vals) const
    {
        return result_t(InternalValue(ListAdapter::CreateAdapter(std::move(vals))));
    }

    result_t operator() (const ValuesMap& vals) const
    {
        if (m_byValue)
        {
            ValuesMap newVals(vals);
            return result_t(InternalValue(MapAdapter::CreateAdapter(std::move(newVals))));
        }

        return result_t(InternalValue(MapAdapter::CreateAdapter(vals)));
    }

    result_t operator() (ValuesMap& vals) const
    {
        return result_t(InternalValue(MapAdapter::CreateAdapter(std::move(vals))));
    }

    result_t operator() (const GenericMap& vals) const
    {
        if (m_byValue)
        {
            GenericMap newVals(vals);
            return result_t(InternalValue(MapAdapter::CreateAdapter(std::move(newVals))));
        }

        return result_t(InternalValue(MapAdapter::CreateAdapter(vals)));
    }

    result_t operator() (GenericMap& vals) const
    {
        return result_t(InternalValue(MapAdapter::CreateAdapter(std::move(vals))));
    }
    
    result_t operator() (const UserFunction& val) const
    {
        return result_t();
    }
    
    template<typename T>
    result_t operator()(const boost::recursive_wrapper<T>& val) const
    {
        return this->operator()(val.get());
    }
    
    template<typename T>
    result_t operator()(boost::recursive_wrapper<T>& val) const
    {
        return this->operator()(val.get());
    }

    template<typename T>
    result_t operator() (T&& val) const
    {
        return result_t(InternalValue(std::forward<T>(val)));
    }

    bool m_byValue;
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

struct UnaryOperation : BaseVisitor<InternalValue>
{
    using BaseVisitor::operator ();

    UnaryOperation(UnaryExpression::Operation oper)
        : m_oper(oper)
    {
    }

    InternalValue operator() (int64_t val) const
    {
        InternalValue result;
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

    InternalValue operator() (double val) const
    {
        InternalValue result;
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

    InternalValue operator() (bool val) const
    {
        InternalValue result;
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

    InternalValue operator() (const MapAdapter&) const
    {
        InternalValue result;
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

    InternalValue operator() (const ListAdapter&) const
    {
        InternalValue result;
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

    template<typename CharT>
    InternalValue operator() (const std::basic_string<CharT>& val) const
    {
        InternalValue result;
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

    InternalValue operator() (const EmptyValue&) const
    {
        InternalValue result;
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

    UnaryExpression::Operation m_oper;
};

struct BinaryMathOperation : BaseVisitor<>
{
    using BaseVisitor::operator ();
    // InternalValue operator() (int, int) const {return InternalValue();}

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

    InternalValue operator() (double left, double right) const
    {
        InternalValue result = 0.0;
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

    InternalValue operator() (int64_t left, int64_t right) const
    {
        InternalValue result;
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

    InternalValue operator() (double left, int64_t right) const
    {
        return this->operator ()(static_cast<double>(left), static_cast<double>(right));
    }

    InternalValue operator() (int64_t left, double right) const
    {
        return this->operator ()(static_cast<double>(left), static_cast<double>(right));
    }

    template<typename CharT>
    InternalValue operator() (const std::basic_string<CharT>& left, const std::basic_string<CharT>& right) const
    {
        InternalValue result;
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

    InternalValue operator() (const KeyValuePair& left, const KeyValuePair& right) const
    {
        InternalValue result;
        switch (m_oper)
        {
        case jinja2::BinaryExpression::LogicalEq:
            result = ConvertToBool(this->operator ()(left.key, right.key)) && ConvertToBool(Apply2<BinaryMathOperation>(left.value, right.value, BinaryExpression::LogicalEq, m_compType));
            break;
        case jinja2::BinaryExpression::LogicalNe:
            result = ConvertToBool(this->operator ()(left.key, right.key)) || ConvertToBool(Apply2<BinaryMathOperation>(left.value, right.value, BinaryExpression::LogicalNe, m_compType));
            break;
        default:
            break;
        }

        return result;
    }

    InternalValue operator() (bool left, bool right) const
    {
        InternalValue result;
        switch (m_oper)
        {
        case jinja2::BinaryExpression::LogicalEq:
            result = left == right;
            break;
        case jinja2::BinaryExpression::LogicalNe:
            result = left != right;
            break;
        case jinja2::BinaryExpression::LogicalLt:
            result = (left ? 1 : 0) < (right ? 1 : 0);
            break;
        default:
            break;
        }

        return result;
    }

    InternalValue operator() (EmptyValue, EmptyValue) const
    {
        InternalValue result;
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
    InternalValue operator() (EmptyValue, T&&) const
    {
        InternalValue result;
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
    InternalValue operator() (T&&, EmptyValue) const
    {
        InternalValue result;
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

struct BooleanEvaluator : BaseVisitor<bool>
{
    using BaseVisitor::operator ();

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

    template<typename CharT>
    bool operator()(const std::basic_string<CharT>& str) const
    {
        return !str.empty();
    }

    bool operator() (const MapAdapter& val) const
    {
        return val.GetSize() != 0;
    }

    bool operator() (const ListAdapter& val) const
    {
        return val.GetSize() != 0;
    }

    bool operator() (const EmptyValue&) const
    {
        return false;
    }
};

template<typename TargetType>
struct NumberEvaluator
{
    NumberEvaluator(TargetType def = 0) : m_def(def)
    {}

    TargetType operator ()(int64_t val) const
    {
        return static_cast<TargetType>(val);
    }
    TargetType operator ()(double val) const
    {
        return static_cast<TargetType>(val);
    }
    TargetType operator ()(bool val) const
    {
        return static_cast<TargetType>(val);
    }
    template<typename U>
    TargetType operator()(U&&) const
    {
        return m_def;
    }

    TargetType m_def;
};

using IntegerEvaluator = NumberEvaluator<int64_t>;
using DoubleEvaluator = NumberEvaluator<double>;


struct StringJoiner : BaseVisitor<>
{
    using BaseVisitor::operator ();

    InternalValue operator() (EmptyValue, const std::string& str) const
    {
        return str;
    }

    InternalValue operator() (const std::string& left, const std::string& right) const
    {
        return left + right;
    }
};

namespace
{
inline std::string GetSampleString();
}

template<typename Fn>
struct StringConverterImpl : public BaseVisitor<decltype(std::declval<Fn>()(GetSampleString()))>
{
    using R = decltype(std::declval<Fn>()(std::string()));
    using BaseVisitor<R>::operator ();

    StringConverterImpl(const Fn& fn) : m_fn(fn) {}

    template<typename CharT>
    R operator()(const std::basic_string<CharT>& str) const
    {
        return m_fn(str);
    }

    const Fn& m_fn;
};

template<typename CharT>
struct SameStringGetter : public visitors::BaseVisitor<std::basic_string<CharT>>
{
    using ResultString = std::basic_string<CharT>;
    using BaseVisitor<ResultString>::operator ();

    ResultString operator()(const ResultString& str) const
    {
        return str;
    }
};

} // visitors

inline bool ConvertToBool(const InternalValue& val)
{
    return Apply<visitors::BooleanEvaluator>(val);
}

inline int64_t ConvertToInt(const InternalValue& val, int64_t def = 0)
{
    return Apply<visitors::IntegerEvaluator>(val, def);
}

inline double ConvertToDouble(const InternalValue& val, double def = 0)
{
    return Apply<visitors::DoubleEvaluator>(val, def);
}

template<template<typename> class Cvt = visitors::StringConverterImpl, typename Fn>
auto ApplyStringConverter(const InternalValue& str, Fn&& fn)
{
    return Apply<Cvt<Fn>>(str, std::forward<Fn>(fn));
}

template<typename CharT>
auto GetAsSameString(const std::basic_string<CharT>& s, const InternalValue& val)
{
    return Apply<visitors::SameStringGetter<CharT>>(val);
}

} // jinja2

#endif // VALUE_VISITORS_H
