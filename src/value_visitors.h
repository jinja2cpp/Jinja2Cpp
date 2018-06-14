#ifndef VALUE_VISITORS_H
#define VALUE_VISITORS_H

#include "expression_evaluator.h"
#include "jinja2cpp/value.h"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/optional.hpp>

#include <iostream>
#include <cmath>
#include <limits>

namespace jinja2
{

namespace detail
{

template<typename Fn>
auto ApplyUnwrapped(const InternalValue& val, Fn&& fn)
{
    auto valueRef = boost::get<ValueRef>(&val);
    auto targetString = boost::get<TargetString>(&val);
    // auto internalValueRef = boost::get<InternalValueRef>(&val);

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
        return boost::apply_visitor(V(args...), val);
    });
}

template<typename V, typename ... Args>
auto Apply2(const InternalValue& val1, const InternalValue& val2, Args&& ... args)
{
    return detail::ApplyUnwrapped(val1, [&val2, &args...](auto& uwVal1) {
        return detail::ApplyUnwrapped(val2, [&uwVal1, &args...](auto& uwVal2) {
            return boost::apply_visitor(V(args...), uwVal1, uwVal2);
        });
    });
}

bool ConvertToBool(const InternalValue& val);

namespace visitors
{
template<typename R = InternalValue>
struct BaseVisitor : public boost::static_visitor<R>
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
#if 0
    template<typename U>
    R operator() (const GenericMap&, U&&) const
    {
        assert(false);
        return R();
    }

    template<typename U>
    R operator() (const GenericList&, U&&) const
    {
        assert(false);
        return R();
    }

    template<typename U>
    R operator() (const ValueRef&, U&&) const
    {
        assert(false);
        return R();
    }

    template<typename U>
    R operator() (const TargetString&, U&&) const
    {
        assert(false);
        return R();
    }

    template<typename T>
    R operator() (T&&, const GenericMap&) const
    {
        assert(false);
        return R();
    }

    template<typename T>
    R operator() (T&&, const GenericList&) const
    {
        assert(false);
        return R();
    }

    template<typename T>
    R operator() (T&&, const ValueRef&) const
    {
        assert(false);
        return R();
    }

    template<typename T>
    R operator() (T&&, const TargetString&) const
    {
        assert(false);
        return R();
    }
#endif

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
    void operator()(const MapAdapter&) const {}
    void operator()(const ListAdapter&) const {}
    void operator()(const ValueRef&) const {}
    void operator()(const TargetString&) const {}
    void operator()(const KeyValuePair&) const {}

    std::basic_ostream<CharT>* m_os;
};

struct InputValueConvertor : public boost::static_visitor<boost::optional<InternalValue>>
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

struct IntegerEvaluator : public boost::static_visitor<int64_t>
{
    IntegerEvaluator(int64_t def = 0) : m_def(def)
    {}

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
        return m_def;
    }

    int64_t m_def;
};

#if 0
struct ValueListEvaluator : boost::static_visitor<ValuesList>
{
    ValueListEvaluator(InternalValue attr = InternalValue())
        : m_attr(std::move(attr))
    {}

    ValuesList operator() (const ValuesList& values) const
    {
        if (IsEmpty(m_attr))
            return values;

        ValuesList result;
        std::transform(values.begin(), values.end(), std::back_inserter(result), [this](const InternalValue& val) {return Subscript(val, m_attr);});
        return result;
    }

    ValuesList operator() (const GenericList& values) const
    {
        int64_t size = values.GetSize();

        ValuesList result;
        for (int64_t idx = 0; idx < size; ++ idx)
        {
            auto val = values.GetValueByIndex(idx);
            if (!IsEmpty(m_attr))
                result.push_back(Subscript(val, m_attr));
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

    InternalValue m_attr;
};


struct GenericListEvaluator : public boost::static_visitor<GenericList>
{
    struct ValueListAdaptor : public ListItemAccessor
    {
        ValueListAdaptor(const ValuesList* list)
            : m_list(list)
        {
        }

        size_t GetSize() const override {return m_list->size();}
        InternalValue GetValueByIndex(int64_t idx) const override {return (*m_list)[static_cast<size_t>(idx)];};

        const ValuesList* m_list;
    };

    template<typename CharT>
    struct StringAdaptor : public ListItemAccessor
    {
        using string = std::basic_string<CharT>;
        StringAdaptor(const string* str)
            : m_str(str)
        {
        }

        size_t GetSize() const override {return m_str->size();}
        InternalValue GetValueByIndex(int64_t idx) const override {return m_str->substr(static_cast<size_t>(idx), 1);};

        const string* m_str;
    };

    GenericListEvaluator(bool unrollStrings)
        : m_unrollStrings(unrollStrings)
    {
    }

    GenericList operator() (const ValuesList& values) const
    {
        return GenericList([adaptor = ValueListAdaptor(&values)]() {return &adaptor;});
    }

    GenericList operator() (const GenericList& values) const
    {
        return values;
    }

    template<typename CharT>
    GenericList operator() (const std::basic_string<CharT>& str) const
    {
        return GenericList([adaptor = StringAdaptor<CharT>(&str)]() {return &adaptor;});
    }

    template<typename U>
    GenericList operator() (U&&) const
    {
        return GenericList();
    }

    bool m_unrollStrings;
};
#endif

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

} // visitors

inline bool ConvertToBool(const InternalValue& val)
{
    return Apply<visitors::BooleanEvaluator>(val);
}

inline int64_t ConvertToInt(const InternalValue& val, int64_t def = 0)
{
    return Apply<visitors::IntegerEvaluator>(val, def);
}

#if 0
inline auto AsValueList(const InternalValue& val, InternalValue subAttr = InternalValue())
{
    auto list = boost::get<ListAdapter>(&val);
    if (!list)
        return ListAdapter();

    if (IsEmpty(subAttr))
        return *list;

    return list->ToSubscriptedList(subAttr);
}

inline auto AsGenericList(const InternalValue& val, bool isStrict = false)
{
    return ListAdapter::CreateAdapter(val, isStrict); // Apply<visitors::GenericListEvaluator>(val, unrollStrings);
}
#endif
} // jinja2

#endif // VALUE_VISITORS_H
