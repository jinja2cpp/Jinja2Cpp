#ifndef JINJA2CPP_SRC_VALUE_VISITORS_H
#define JINJA2CPP_SRC_VALUE_VISITORS_H

#include "expression_evaluator.h"
#include "helpers.h"
#include "unicode_printable.h"
#include "jinja2cpp/value.h"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/optional.hpp>
#include <fmt/format.h>
#include <fmt/xchar.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>
#include <vector>
#include <typeinfo>

namespace jinja2
{

namespace detail
{

template<typename V>
struct RecursiveUnwrapper
{
    V* m_visitor{};

    RecursiveUnwrapper(V* v)
        : m_visitor(v)
    {}


    template<typename T>
    static const auto& UnwrapRecursive(const T& arg)
    {
        return arg; // std::forward<T>(arg);
    }

    template<typename T>
    static auto& UnwrapRecursive(const RecursiveWrapper<T>& arg)
    {
        return arg.GetValue();
    }

    //    template<typename T>
    //   static auto& UnwrapRecursive(RecursiveWrapper<T>& arg)
    //    {
    //        return arg.GetValue();
    //    }

    template<typename... Args>
    auto operator()(const Args&... args) const
    {
        assert(m_visitor != nullptr);
        return (*m_visitor)(UnwrapRecursive(args)...);
    }
};

template<typename Fn>
auto ApplyUnwrapped(const InternalValueData& val, Fn&& fn)
{
    auto valueRef = GetIf<ValueRef>(&val);
    auto targetString = GetIf<TargetString>(&val);
    auto targetSV = GetIf<TargetStringView>(&val);
    // auto internalValueRef = GetIf<InternalValueRef>(&val);

    if (valueRef != nullptr)
        return fn(valueRef->get().data());
    else if (targetString != nullptr)
        return fn(*targetString);
    else if (targetSV != nullptr)
        return fn(*targetSV);
    //    else if (internalValueRef != nullptr)
    //        return fn(internalValueRef->get());

    return fn(val);
}
} // namespace detail

template<typename V, typename... Args>
auto Apply(const InternalValue& val, Args&&... args)
{
    return detail::ApplyUnwrapped(val.GetData(), [&args...](auto& val) {
        auto v = V(args...);
        return nonstd::visit(detail::RecursiveUnwrapper<V>(&v), val);
    });
}

template<typename V, typename... Args>
auto Apply2(const InternalValue& val1, const InternalValue& val2, Args&&... args)
{
    return detail::ApplyUnwrapped(val1.GetData(), [&val2, &args...](auto& uwVal1) {
        return detail::ApplyUnwrapped(val2.GetData(), [&uwVal1, &args...](auto& uwVal2) {
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
    R operator()(const GenericMap&) const
    {
        assert(false);
        return R();
    }

    R operator()(const GenericList&) const
    {
        assert(false);
        return R();
    }

    R operator()(const ValueRef&) const
    {
        assert(false);
        return R();
    }

    R operator()(const TargetString&) const
    {
        assert(false);
        return R();
    }

    template<typename T>
    R operator()(T&&) const
    {
        return R();
    }

    template<typename T, typename U>
    R operator()(T&&, U&&) const
    {
        return R();
    }
};


// Formats a double the way Python repr() and str() do: shortest round-trip digits,
// exponent form outside [1e-4, 1e16), and ".0" on whole numbers.
inline std::string FormatPythonFloat(double val)
{
    if (std::isnan(val))
        return "nan";
    if (std::isinf(val))
        return val < 0 ? "-inf" : "inf";

    auto result = fmt::format("{}", val);
    if (result.find_first_of(".e") == std::string::npos)
        result += ".0";
    return result;
}

template<typename CharT>
struct ValueRenderer;

// Writes a value the way Python str() does, or repr() when asRepr is set. Containers
// always print their items with repr(), as Python does.
template<typename CharT>
struct ValueRendererBase
{
    // Containers being printed, outermost first: a container that contains itself prints
    // as [...] or {...}, as in Python, and nesting deeper than this prints as ... too
    using ContainerStack = std::vector<const void*>;
    static constexpr size_t MaxReprDepth = 256;

    ValueRendererBase(std::basic_string<CharT>& os, bool asRepr, ContainerStack* containers)
        : m_os(&os)
        , m_asRepr(asRepr)
        , m_containers(containers)
    {
    }

    template<typename T>
    void operator()(const T& val) const;
    void operator()(double val) const { AppendAscii(FormatPythonFloat(val)); }
    void operator()(bool val) const { AppendAscii(val ? "True" : "False"); }
    void operator()(const nonstd::basic_string_view<CharT>& val) const { AppendString(val); }
    void operator()(const std::basic_string<CharT>& val) const { AppendString(val); }

    void operator()(const EmptyValue&) const { AppendAscii("None"); }
    // Undefined prints as empty. Inside a container Python shows Undefined, but a JSON null
    // in a reflected object still reads as undefined (task 0045), so it stays None there
    void operator()(const UndefinedValue&) const
    {
        if (m_asRepr)
            AppendAscii("None");
    }
    void operator()(const ListAdapter& list) const;
    void operator()(const MapAdapter& map) const;
    void operator()(const KeyValuePair& pair) const;
    void operator()(const ValuesList& list) const { RenderConverted(list); }
    void operator()(const ValuesMap& map) const { RenderConverted(map); }
    void operator()(const GenericList& list) const { RenderConverted(list); }
    void operator()(const GenericMap& map) const { RenderConverted(map); }
    void operator()(const ValueRef&) const {}
    void operator()(const TargetString&) const {}
    void operator()(const TargetStringView&) const {}
    void operator()(const Callable&) const {}
    void operator()(const UserCallable&) const {}
    void operator()(const std::shared_ptr<IRendererBase>) const {}
    template<typename T>
    void operator()(const boost::recursive_wrapper<T>&) const
    {
    }
    template<typename T>
    void operator()(const RecWrapper<T>& val) const
    {
        (*this)(*val);
    }

    auto GetOs() const { return std::back_inserter(*m_os); }

    void AppendAscii(nonstd::string_view str) const { m_os->append(str.begin(), str.end()); }
    void AppendString(nonstd::basic_string_view<CharT> str) const;
    void AppendCodePointEscape(uint32_t cp) const;
    template<typename T>
    void RenderConverted(const T& val) const;
    void RenderRepr(const InternalValue& val, ContainerStack* containers) const;
    // Returns false (and prints the placeholder) when the container is already being printed
    bool EnterContainer(const void* id, ContainerStack& containers, const char* placeholder) const;

    std::basic_string<CharT>* m_os;
    bool m_asRepr = false;
    ContainerStack* m_containers = nullptr;
};

template<>
template<typename T>
void ValueRendererBase<char>::operator()(const T& val) const
{
    fmt::format_to(GetOs(), "{}", val);
}

template<>
template<typename T>
void ValueRendererBase<wchar_t>::operator()(const T& val) const
{
    fmt::format_to(GetOs(), L"{}", val);
}

template<typename CharT>
void ValueRendererBase<CharT>::AppendCodePointEscape(uint32_t cp) const
{
    if (cp < 0x100)
        AppendAscii(fmt::format("\\x{:02x}", cp));
    else if (cp < 0x10000)
        AppendAscii(fmt::format("\\u{:04x}", cp));
    else
        AppendAscii(fmt::format("\\U{:08x}", cp));
}

namespace detail
{
// Decodes one code point from UTF-8 (char) or UTF-16/UTF-32 (wchar_t) starting at pos and
// returns the number of code units it takes. Malformed input yields one unit as is.
inline size_t DecodeCodePoint(nonstd::string_view str, size_t pos, uint32_t& cp)
{
    auto unit = [&str](size_t idx) { return static_cast<uint32_t>(static_cast<unsigned char>(str[idx])); };
    uint32_t lead = unit(pos);
    size_t len = lead < 0x80 ? 1 : (lead >> 5) == 0x6 ? 2
                               : (lead >> 4) == 0xe   ? 3
                               : (lead >> 3) == 0x1e  ? 4
                                                      : 0;
    cp = lead;
    if (len <= 1 || pos + len > str.size())
        return 1;

    uint32_t result = lead & (0xff >> (len + 1));
    for (size_t idx = 1; idx != len; ++idx)
    {
        uint32_t next = unit(pos + idx);
        if ((next & 0xc0) != 0x80)
            return 1;
        result = (result << 6) | (next & 0x3f);
    }
    static const uint32_t minValue[] = { 0, 0, 0x80, 0x800, 0x10000 };
    if (result < minValue[len] || result > 0x10ffff || (result >= 0xd800 && result <= 0xdfff))
        return 1;
    cp = result;
    return len;
}

inline size_t DecodeCodePoint(nonstd::wstring_view str, size_t pos, uint32_t& cp)
{
    cp = static_cast<uint32_t>(str[pos]);
    if (sizeof(wchar_t) == 2 && cp >= 0xd800 && cp <= 0xdbff && pos + 1 < str.size())
    {
        auto low = static_cast<uint32_t>(str[pos + 1]);
        if (low >= 0xdc00 && low <= 0xdfff)
        {
            cp = 0x10000 + ((cp - 0xd800) << 10) + (low - 0xdc00);
            return 2;
        }
    }
    return 1;
}
} // namespace detail

template<typename CharT>
void ValueRendererBase<CharT>::AppendString(nonstd::basic_string_view<CharT> str) const
{
    if (!m_asRepr)
    {
        m_os->append(str.begin(), str.end());
        return;
    }

    // Python picks single quotes unless the string has a single quote and no double one
    bool hasSingle = str.find(CharT('\'')) != str.npos;
    bool hasDouble = str.find(CharT('"')) != str.npos;
    CharT quote = hasSingle && !hasDouble ? CharT('"') : CharT('\'');

    m_os->push_back(quote);
    for (size_t pos = 0; pos < str.size();)
    {
        uint32_t cp = 0;
        size_t len = detail::DecodeCodePoint(str, pos, cp);
        if (cp == static_cast<uint32_t>(quote) || cp == '\\')
        {
            m_os->push_back(CharT('\\'));
            m_os->push_back(str[pos]);
        }
        else if (cp == '\t')
            AppendAscii("\\t");
        else if (cp == '\n')
            AppendAscii("\\n");
        else if (cp == '\r')
            AppendAscii("\\r");
        else if (!jinja2::detail::IsPythonPrintable(cp))
            AppendCodePointEscape(cp);
        else
            m_os->append(str.data() + pos, len);
        pos += len;
    }
    m_os->push_back(quote);
}

struct InputValueConvertor
{
    using result_t = boost::optional<InternalValue>;

    InputValueConvertor(bool byValue, bool allowStringRef)
        : m_byValue(byValue)
        , m_allowStringRef(allowStringRef)
    {
    }

    template<typename ChT>
    result_t operator()(const std::basic_string<ChT>& val) const
    {
        if (m_allowStringRef)
            return result_t(TargetStringView(nonstd::basic_string_view<ChT>(val)));

        return result_t(TargetString(val));
    }

    template<typename ChT>
    result_t operator()(std::basic_string<ChT>& val) const
    {
        return result_t(TargetString(std::move(val)));
    }

    result_t operator()(const ValuesList& vals) const
    {
        if (m_byValue)
        {
            ValuesList newVals(vals);
            return result_t(InternalValue(ListAdapter::CreateAdapter(std::move(newVals))));
        }

        return result_t(InternalValue(ListAdapter::CreateAdapter(vals)));
    }

    result_t operator()(ValuesList& vals) const
    {
        return result_t(InternalValue(ListAdapter::CreateAdapter(std::move(vals))));
    }

    result_t operator()(const GenericList& vals) const
    {
        if (m_byValue)
        {
            GenericList newVals(vals);
            return result_t(InternalValue(ListAdapter::CreateAdapter(std::move(newVals))));
        }

        return result_t(InternalValue(ListAdapter::CreateAdapter(vals)));
    }

    result_t operator()(GenericList& vals) const
    {
        return result_t(InternalValue(ListAdapter::CreateAdapter(std::move(vals))));
    }

    result_t operator()(const ValuesMap& vals) const
    {
        if (m_byValue)
        {
            ValuesMap newVals(vals);
            return result_t(CreateMapAdapter(std::move(newVals)));
        }

        return result_t(CreateMapAdapter(vals));
    }

    result_t operator()(ValuesMap& vals) const { return result_t(CreateMapAdapter(std::move(vals))); }

    result_t operator()(const GenericMap& vals) const
    {
        if (m_byValue)
        {
            GenericMap newVals(vals);
            return result_t(CreateMapAdapter(std::move(newVals)));
        }

        return result_t(CreateMapAdapter(vals));
    }

    result_t operator()(GenericMap& vals) const { return result_t(CreateMapAdapter(std::move(vals))); }

    result_t operator()(const UserCallable& val) const { return ConvertUserCallable(val); }

    result_t operator()(UserCallable& val) const { return ConvertUserCallable(std::move(val)); }

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

    result_t operator()(bool val) const
    {
        return result_t(InternalValue(val));
    }

    result_t operator()(int64_t val) const
    {
        return result_t(InternalValue(val));
    }

    result_t operator()(double val) const
    {
        return result_t(InternalValue(val));
    }

    template<typename T>
    result_t operator()(const T& val) const
    {
        return result_t(InternalValue(val));
    }

    static result_t ConvertUserCallable(const UserCallable& val);

    bool m_byValue{};
    bool m_allowStringRef{};
};

template<>
struct ValueRenderer<char> : ValueRendererBase<char>
{
    explicit ValueRenderer(std::string& os, bool asRepr = false, ContainerStack* containers = nullptr)
        : ValueRendererBase<char>::ValueRendererBase<char>(os, asRepr, containers)
    {
    }

    using ValueRendererBase<char>::operator();
    void operator()(const std::wstring& str) const { AppendString(ConvertString<std::string>(str)); }
    void operator()(const nonstd::wstring_view& str) const { AppendString(ConvertString<std::string>(str)); }
};

template<>
struct ValueRenderer<wchar_t> : ValueRendererBase<wchar_t>
{
    explicit ValueRenderer(std::wstring& os, bool asRepr = false, ContainerStack* containers = nullptr)
        : ValueRendererBase<wchar_t>::ValueRendererBase<wchar_t>(os, asRepr, containers)
    {
    }

    using ValueRendererBase<wchar_t>::operator();
    void operator()(const std::string& str) const { AppendString(ConvertString<std::wstring>(str)); }
    void operator()(const nonstd::string_view& str) const { AppendString(ConvertString<std::wstring>(str)); }
};

template<typename CharT>
void ValueRendererBase<CharT>::RenderRepr(const InternalValue& val, ContainerStack* containers) const
{
    Apply<ValueRenderer<CharT>>(val, *m_os, true, containers);
}

template<typename CharT>
template<typename T>
void ValueRendererBase<CharT>::RenderConverted(const T& val) const
{
    auto converted = InputValueConvertor(false, true)(val);
    if (converted)
        Apply<ValueRenderer<CharT>>(*converted, *m_os, m_asRepr, m_containers);
}

template<typename CharT>
bool ValueRendererBase<CharT>::EnterContainer(const void* id, ContainerStack& containers, const char* placeholder) const
{
    if (containers.size() >= MaxReprDepth || std::find(containers.begin(), containers.end(), id) != containers.end())
    {
        AppendAscii(placeholder);
        return false;
    }
    containers.push_back(id);
    return true;
}

template<typename CharT>
void ValueRendererBase<CharT>::operator()(const ListAdapter& list) const
{
    if (auto range = list.GetRangeInfo())
    {
        // Python prints a range by its arguments, the step only when it is not 1
        AppendAscii("range(" + std::to_string(range->start) + ", " + std::to_string(range->stop));
        if (range->step != 1)
            AppendAscii(", " + std::to_string(range->step));
        AppendAscii(")");
        return;
    }

    bool isTuple = list.IsTuple();
    ContainerStack ownContainers;
    auto& containers = m_containers ? *m_containers : ownContainers;
    if (!EnterContainer(list.GetIdentity(), containers, isTuple ? "(...)" : "[...]"))
        return;

    AppendAscii(isTuple ? "(" : "[");
    size_t count = 0;
    for (auto& item : list)
    {
        if (count++ != 0)
            AppendAscii(", ");
        RenderRepr(item, &containers);
    }
    if (isTuple && count == 1)
        AppendAscii(",");
    AppendAscii(isTuple ? ")" : "]");
    containers.pop_back();
}

template<typename CharT>
void ValueRendererBase<CharT>::operator()(const MapAdapter& map) const
{
    ContainerStack ownContainers;
    auto& containers = m_containers ? *m_containers : ownContainers;
    if (!EnterContainer(map.GetIdentity(), containers, "{...}"))
        return;

    // Python prints dicts in insertion order; the maps behind MapAdapter are unordered,
    // so sort the keys to keep the output stable across standard libraries
    auto keys = map.GetKeys();
    std::sort(keys.begin(), keys.end());

    ValueRenderer<CharT> keyRenderer(*m_os, true);
    AppendAscii("{");
    bool isFirst = true;
    for (auto& key : keys)
    {
        if (!isFirst)
            AppendAscii(", ");
        isFirst = false;
        keyRenderer(key);
        AppendAscii(": ");
        RenderRepr(map.GetValueByName(key), &containers);
    }
    AppendAscii("}");
    containers.pop_back();
}

template<typename CharT>
void ValueRendererBase<CharT>::operator()(const KeyValuePair& pair) const
{
    // dict items are (key, value) tuples in Python
    AppendAscii("(");
    ValueRenderer<CharT>(*m_os, true)(pair.key);
    AppendAscii(", ");
    RenderRepr(pair.value, m_containers);
    AppendAscii(")");
}

struct UnaryOperation : BaseVisitor<InternalValue>
{
    using BaseVisitor::operator();

    UnaryOperation(UnaryExpression::Operation oper)
        : m_oper(oper)
    {
    }

    InternalValue operator()(int64_t val) const
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

    InternalValue operator()(double val) const
    {
        InternalValue result;
        switch (m_oper)
        {
        case jinja2::UnaryExpression::LogicalNot:
            result = fabs(val) > std::numeric_limits<double>::epsilon() ? false : true;
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

    InternalValue operator()(bool val) const
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

    InternalValue operator()(const MapAdapter&) const
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

    InternalValue operator()(const ListAdapter&) const
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
    InternalValue operator()(const std::basic_string<CharT>& val) const
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

    template<typename CharT>
    InternalValue operator()(const nonstd::basic_string_view<CharT>& val) const
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

    InternalValue operator()(const EmptyValue&) const { return NothingResult(); }
    InternalValue operator()(const UndefinedValue&) const { return NothingResult(); }

    InternalValue NothingResult() const
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
    using BaseVisitor::operator();
    using ResultType = InternalValue;
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

    ResultType operator()(double left, double right) const
    {
        ResultType result = 0.0;
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
        case jinja2::BinaryExpression::DivRemainder:
            result = std::fmod(left, right);
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

    // int ** int is an int in Python unless the exponent is negative. Results beyond the
    // exactly representable double range stay floats until big integers exist (task 0015).
    ResultType IntegerPow(int64_t base, int64_t exp) const
    {
        double approx = std::pow(static_cast<double>(base), static_cast<double>(exp));
        if (exp < 0 || !(std::abs(approx) < 9007199254740992.0))
            return approx;

        if (base == 0 || base == 1)
            return exp == 0 ? int64_t(1) : base;
        if (base == -1)
            return exp % 2 == 0 ? int64_t(1) : int64_t(-1);

        // |base| >= 2 and the result fits in 2^53, so exp <= 53
        int64_t result = 1;
        for (; exp != 0; --exp)
            result *= base;
        return result;
    }

    ResultType operator()(int64_t left, int64_t right) const
    {
        ResultType result;
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
            // integer division by zero and INT64_MIN / -1 trap, so leave those to the float path
            if (right == 0 || (right == -1 && left == std::numeric_limits<int64_t>::min()))
                result = this->operator()(static_cast<double>(left), static_cast<double>(right));
            else
                result = left / right;
            break;
        case jinja2::BinaryExpression::DivRemainder:
            if (right == 0)
                result = this->operator()(static_cast<double>(left), static_cast<double>(right));
            else if (right == -1)
                result = int64_t(0);
            else
            {
                // Python's % takes the sign of the divisor
                int64_t rem = left % right;
                if (rem != 0 && (rem < 0) != (right < 0))
                    rem += right;
                result = rem;
            }
            break;
        case jinja2::BinaryExpression::Pow:
            result = IntegerPow(left, right);
            break;
        case jinja2::BinaryExpression::Div:
            result = this->operator()(static_cast<double>(left), static_cast<double>(right));
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

    ResultType operator()(int64_t left, double right) const
    {
        return this->operator()(static_cast<double>(left), static_cast<double>(right));
    }

    ResultType operator()(double left, int64_t right) const
    {
        return this->operator()(static_cast<double>(left), static_cast<double>(right));
    }

    template<typename CharT>
    ResultType operator()(const std::basic_string<CharT>& left, const std::basic_string<CharT>& right) const
    {
        return ProcessStrings(nonstd::basic_string_view<CharT>(left), nonstd::basic_string_view<CharT>(right));
    }

    template<typename CharT1, typename CharT2>
    std::enable_if_t<!std::is_same<CharT1, CharT2>::value, ResultType> operator()(const std::basic_string<CharT1>& left, const std::basic_string<CharT2>& right) const
    {
        auto rightStr = ConvertString<std::basic_string<CharT1>>(right);
        return ProcessStrings(nonstd::basic_string_view<CharT1>(left), nonstd::basic_string_view<CharT1>(rightStr));
    }

    template<typename CharT>
    ResultType operator()(const nonstd::basic_string_view<CharT>& left, const std::basic_string<CharT>& right) const
    {
        return ProcessStrings(left, nonstd::basic_string_view<CharT>(right));
    }

    template<typename CharT1, typename CharT2>
    std::enable_if_t<!std::is_same<CharT1, CharT2>::value, ResultType> operator()(const nonstd::basic_string_view<CharT1>& left, const std::basic_string<CharT2>& right) const
    {
        auto rightStr = ConvertString<std::basic_string<CharT1>>(right);
        return ProcessStrings(left, nonstd::basic_string_view<CharT1>(rightStr));
    }

    template<typename CharT>
    ResultType operator()(const std::basic_string<CharT>& left, const nonstd::basic_string_view<CharT>& right) const
    {
        return ProcessStrings(nonstd::basic_string_view<CharT>(left), right);
    }

    template<typename CharT1, typename CharT2>
    std::enable_if_t<!std::is_same<CharT1, CharT2>::value, ResultType> operator()(const std::basic_string<CharT1>& left, const nonstd::basic_string_view<CharT2>& right) const
    {
        auto rightStr = ConvertString<std::basic_string<CharT1>>(right);
        return ProcessStrings(nonstd::basic_string_view<CharT1>(left), nonstd::basic_string_view<CharT1>(rightStr));
    }

    template<typename CharT>
    ResultType operator()(const nonstd::basic_string_view<CharT>& left, const nonstd::basic_string_view<CharT>& right) const
    {
        return ProcessStrings(left, right);
    }

    template<typename CharT1, typename CharT2>
    std::enable_if_t<!std::is_same<CharT1, CharT2>::value, ResultType> operator()(const nonstd::basic_string_view<CharT1>& left, const nonstd::basic_string_view<CharT2>& right) const
    {
        auto rightStr = ConvertString<std::basic_string<CharT1>>(right);
        return ProcessStrings(left, nonstd::basic_string_view<CharT1>(rightStr));
    }

    template<typename CharT>
    ResultType operator()(const std::basic_string<CharT>& left, int64_t right) const
    {
        return RepeatString(nonstd::basic_string_view<CharT>(left), right);
    }

    template<typename CharT>
    ResultType operator()(const nonstd::basic_string_view<CharT>& left, int64_t right) const
    {
        return RepeatString(left, right);
    }

    template<typename CharT>
    ResultType RepeatString(const nonstd::basic_string_view<CharT>& left, const int64_t right) const
    {
        using string = std::basic_string<CharT>;
        ResultType result;

        if (m_oper == jinja2::BinaryExpression::Mul)
        {
            string str;
            for (int i = 0; i < right; ++i)
                str.append(left.begin(), left.end());
            result = TargetString(std::move(str));
        }
        return result;
    }

    template<typename CharT>
    ResultType ProcessStrings(const nonstd::basic_string_view<CharT>& left, const nonstd::basic_string_view<CharT>& right) const
    {
        using string = std::basic_string<CharT>;
        ResultType result;

        switch (m_oper)
        {
        case jinja2::BinaryExpression::Plus:
        {
            auto str = string(left.begin(), left.end());
            str.append(right.begin(), right.end());
            result = TargetString(std::move(str));
            break;
        }
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

    ResultType operator()(const KeyValuePair& left, const KeyValuePair& right) const
    {
        ResultType result;
        switch (m_oper)
        {
        case jinja2::BinaryExpression::LogicalEq:
            result = ConvertToBool(this->operator()(left.key, right.key)) && ConvertToBool(Apply2<BinaryMathOperation>(left.value, right.value, BinaryExpression::LogicalEq, m_compType));
            break;
        case jinja2::BinaryExpression::LogicalNe:
            result = ConvertToBool(this->operator()(left.key, right.key)) || ConvertToBool(Apply2<BinaryMathOperation>(left.value, right.value, BinaryExpression::LogicalNe, m_compType));
            break;
        default:
            break;
        }

        return result;
    }

    ResultType operator()(const ListAdapter& left, const ListAdapter& right) const
    {
        ResultType result;
        if (m_oper == jinja2::BinaryExpression::Plus)
        {
            InternalValueList values;
            values.reserve(left.GetSize().value_or(0) + right.GetSize().value_or(0));
            for (auto& v : left)
                values.push_back(v);
            for (auto& v : right)
                values.push_back(v);
            result = ListAdapter::CreateAdapter(std::move(values));
        }

        return result;
    }

    ResultType operator()(const ListAdapter& left, int64_t right) const
    {
        ResultType result;
        if (right >= 0 && m_oper == jinja2::BinaryExpression::Mul)
        {
            InternalValueList values;
            values.reserve(left.GetSize().value_or(0));
            for (auto& v : left)
                values.push_back(v);
            auto listSize = values.size() * right;
            result = ListAdapter::CreateAdapter(static_cast<size_t>(listSize),
                                                [size = values.size(), values = std::move(values)](size_t idx) { return values[idx % size]; });
        }

        return result;
    }

    ResultType operator()(bool left, bool right) const
    {
        ResultType result;
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

    // None and undefined equal only themselves (undefined == undefined, as in Jinja2)
    ResultType operator()(EmptyValue, EmptyValue) const { return EqualityResult(true); }
    ResultType operator()(UndefinedValue, UndefinedValue) const { return EqualityResult(true); }
    ResultType operator()(EmptyValue, UndefinedValue) const { return EqualityResult(false); }
    ResultType operator()(UndefinedValue, EmptyValue) const { return EqualityResult(false); }

    template<typename T>
    ResultType operator()(EmptyValue, T&&) const
    {
        return EqualityResult(false);
    }

    template<typename T>
    ResultType operator()(T&&, EmptyValue) const
    {
        return EqualityResult(false);
    }

    template<typename T>
    ResultType operator()(UndefinedValue, T&&) const
    {
        return EqualityResult(false);
    }

    template<typename T>
    ResultType operator()(T&&, UndefinedValue) const
    {
        return EqualityResult(false);
    }

    ResultType EqualityResult(bool equal) const
    {
        ResultType result;
        switch (m_oper)
        {
        case jinja2::BinaryExpression::LogicalEq:
            result = equal;
            break;
        case jinja2::BinaryExpression::LogicalNe:
            result = !equal;
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
    using BaseVisitor::operator();

    bool operator()(int64_t val) const
    {
        return val != 0;
    }

    bool operator()(double val) const
    {
        return fabs(val) < std::numeric_limits<double>::epsilon();
    }

    bool operator()(bool val) const
    {
        return val;
    }

    template<typename CharT>
    bool operator()(const std::basic_string<CharT>& str) const
    {
        return !str.empty();
    }

    template<typename CharT>
    bool operator()(const nonstd::basic_string_view<CharT>& str) const
    {
        return !str.empty();
    }

    bool operator()(const MapAdapter& val) const
    {
        return val.GetSize() != 0ULL;
    }

    bool operator()(const ListAdapter& val) const
    {
        return val.GetSize() != 0ULL;
    }

    bool operator()(const EmptyValue&) const
    {
        return false;
    }

    bool operator()(const UndefinedValue&) const
    {
        return false;
    }

    // Functions and macros are truthy, as in Python
    bool operator()(const Callable&) const
    {
        return true;
    }
};

template<typename TargetType>
struct NumberEvaluator
{
    NumberEvaluator(TargetType def = 0)
        : m_def(def)
    {}

    TargetType operator()(int64_t val) const
    {
        return static_cast<TargetType>(val);
    }
    TargetType operator()(double val) const
    {
        return static_cast<TargetType>(val);
    }
    TargetType operator()(bool val) const
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


struct StringJoiner : BaseVisitor<TargetString>
{
    using BaseVisitor::operator();

    template<typename CharT>
    TargetString operator()(UndefinedValue, const std::basic_string<CharT>& str) const
    {
        return str;
    }

    template<typename CharT>
    TargetString operator()(UndefinedValue, const nonstd::basic_string_view<CharT>& str) const
    {
        return std::basic_string<CharT>(str.begin(), str.end());
    }

    template<typename CharT>
    TargetString operator()(const std::basic_string<CharT>& left, const std::basic_string<CharT>& right) const
    {
        return left + right;
    }

    template<typename CharT1, typename CharT2>
    std::enable_if_t<!std::is_same<CharT1, CharT2>::value, TargetString> operator()(const std::basic_string<CharT1>& left, const std::basic_string<CharT2>& right) const
    {
        return left + ConvertString<std::basic_string<CharT1>>(right);
    }

    template<typename CharT>
    TargetString operator()(std::basic_string<CharT> left, const nonstd::basic_string_view<CharT>& right) const
    {
        left.append(right.begin(), right.end());
        return std::move(left);
    }

    template<typename CharT1, typename CharT2>
    std::enable_if_t<!std::is_same<CharT1, CharT2>::value, TargetString> operator()(std::basic_string<CharT1> left, const nonstd::basic_string_view<CharT2>& right) const
    {
        auto r = ConvertString<std::basic_string<CharT1>>(right);
        left.append(r.begin(), r.end());
        return std::move(left);
    }
};

template<typename Fn>
struct StringConverterImpl : public BaseVisitor<decltype(std::declval<Fn>()(std::declval<nonstd::string_view>()))>
{
    using R = decltype(std::declval<Fn>()(nonstd::string_view()));
    using BaseVisitor<R>::operator();

    StringConverterImpl(const Fn& fn)
        : m_fn(fn) {}

    template<typename CharT>
    R operator()(const std::basic_string<CharT>& str) const
    {
        return m_fn(nonstd::basic_string_view<CharT>(str));
    }

    template<typename CharT>
    R operator()(const nonstd::basic_string_view<CharT>& str) const
    {
        return m_fn(str);
    }

    const Fn& m_fn;
};

template<typename CharT>
struct SameStringGetter : public visitors::BaseVisitor<nonstd::expected<void, std::basic_string<CharT>>>
{
    using ResultString = std::basic_string<CharT>;
    using OtherString = std::conditional_t<std::is_same<CharT, char>::value, std::wstring, std::string>;
    using ResultStringView = nonstd::basic_string_view<CharT>;
    using OtherStringView = std::conditional_t<std::is_same<CharT, char>::value, nonstd::wstring_view, nonstd::string_view>;
    using Result = nonstd::expected<void, ResultString>;
    using BaseVisitor<Result>::operator();

    Result operator()(const ResultString& str) const
    {
        return nonstd::make_unexpected(str);
    }

    Result operator()(const ResultStringView& str) const
    {
        return nonstd::make_unexpected(ResultString(str.begin(), str.end()));
    }

    Result operator()(const OtherString& str) const
    {
        return nonstd::make_unexpected(ConvertString<ResultString>(str));
    }

    Result operator()(const OtherStringView& str) const
    {
        return nonstd::make_unexpected(ConvertString<ResultString>(str));
    }
};

} // namespace visitors

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
auto GetAsSameString(const std::basic_string<CharT>&, const InternalValue& val)
{
    using Result = nonstd::optional<std::basic_string<CharT>>;
    auto result = Apply<visitors::SameStringGetter<CharT>>(val);
    if (!result)
        return Result(result.error());

    return Result();
}

template<typename CharT>
auto GetAsSameString(const nonstd::basic_string_view<CharT>&, const InternalValue& val)
{
    using Result = nonstd::optional<std::basic_string<CharT>>;
    auto result = Apply<visitors::SameStringGetter<CharT>>(val);
    if (!result)
        return Result(result.error());

    return Result();
}

inline bool operator==(const InternalValueData& lhs, const InternalValueData& rhs)
{
    InternalValue cmpRes;
    cmpRes = Apply2<visitors::BinaryMathOperation>(lhs, rhs, BinaryExpression::LogicalEq);
    return ConvertToBool(cmpRes);
}

inline bool operator!=(const InternalValueData& lhs, const InternalValueData& rhs)
{
    return !(lhs == rhs);
}

} // namespace jinja2

#endif // JINJA2CPP_SRC_VALUE_VISITORS_H
