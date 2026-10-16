#ifndef JINJA2CPP_SRC_VALUE_VISITORS_H
#define JINJA2CPP_SRC_VALUE_VISITORS_H

#include "expression_evaluator.h"
#include "internal_value.h"
#include "make_unexpected.h"
#include "undefined.h"
#include "unicode_printable.h"

#include <jinja2cpp/generic_list.h>
#include <jinja2cpp/string_helpers.h>
#include <jinja2cpp/template_env.h>
#include <jinja2cpp/value.h>

#include <boost/algorithm/string/predicate.hpp>
#include <fmt/format.h>
#include <fmt/xchar.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace jinja2
{

namespace detail
{

// Python's a.lower() <=> b.lower(), comparing code points: -1, 0 or 1. The full Unicode
// mapping (string_converter_filter.cpp); CompareIgnoreCase calls it only for non-ASCII text
int CompareLowerUnicode(std::string_view left, std::string_view right);
int CompareLowerUnicode(std::wstring_view left, std::wstring_view right);
// str.lower() of UTF-8 text with a non-ASCII byte (string_converter_filter.cpp)
void LowerUtf8Unicode(std::string_view str, std::string& out);

// Sets out to str.lower() of UTF-8 text, whose bytes then order as the code points of the
// lowered string. ASCII text is lowered inline, into a sized buffer: push_back with the
// non-ASCII check costs dictsort a third more per key (Render/dict_ops)
inline void LowerUtf8(std::string_view str, std::string& out)
{
    out.resize(str.size());
    uint32_t bits = 0;
    for (size_t idx = 0; idx != str.size(); ++idx)
    {
        const auto ch = str[idx];
        bits |= CodeUnit(ch);
        out[idx] = ch >= 'A' && ch <= 'Z' ? static_cast<char>(ch - 'A' + 'a') : ch;
    }
    if (bits >= 0x80)
    {
        LowerUtf8Unicode(str, out);
    }
}

// Case-insensitive three-way comparison as Jinja2's ignore_case does it (str.lower()). An
// ASCII prefix is compared inline: lowering a code point never looks at the ones after it,
// so the first ASCII difference decides, and only a non-ASCII unit takes the Unicode path
template<typename CharT>
int CompareIgnoreCase(std::basic_string_view<CharT> left, std::basic_string_view<CharT> right)
{
    const auto size = std::min(left.size(), right.size());
    for (size_t idx = 0; idx != size; ++idx)
    {
        auto l = CodeUnit(left[idx]);
        auto r = CodeUnit(right[idx]);
        if ((l | r) >= 0x80)
        {
            return CompareLowerUnicode(left, right);
        }
        l += (l >= 'A' && l <= 'Z') ? 'a' - 'A' : 0;
        r += (r >= 'A' && r <= 'Z') ? 'a' - 'A' : 0;
        if (l != r)
        {
            return l < r ? -1 : 1;
        }
    }
    if (left.size() == right.size())
    {
        return 0;
    }
    // Every code point lowers to at least one, so the longer string is greater, unless its
    // next unit is a stray continuation that belongs to the last character (malformed text)
    const auto next = CodeUnit(left.size() < right.size() ? right[size] : left[size]);
    if (next >= 0x80)
    {
        return CompareLowerUnicode(left, right);
    }
    return left.size() < right.size() ? -1 : 1;
}

template<typename V, size_t N>
struct RecursiveUnwrapper
{
    V* m_visitor{};
    // The objects of the visited values, which the list and map views a visitor receives
    // name as their owners (ListRef::ToAdapter)
    std::array<const ValueObject*, N> m_owners{};

    RecursiveUnwrapper(V* v, std::array<const ValueObject*, N> owners)
        : m_visitor(v)
        , m_owners(owners)
    {}

    template<typename T>
    static const T& UnwrapRecursive(const T& arg, const ValueObject* /*owner*/)
    {
        // An alternative std::visit holds for the call
        return arg; // NOLINT(bugprone-return-const-ref-from-parameter)
    }

    // What a value holds in place is visited as what it stands for: a string, a string
    // view, a list or map view, or the held object (docs/tasks/0140). A visitor receives a
    // short string as a std::basic_string, copied out of the value within the string's own
    // SSO buffer (SmallString::Capacity), so no visitor can keep a view into a value's bytes.
    // Any other text is a view: borrowed text, or the characters of the value's object
    // (docs/tasks/0151), which live as long as the value. A visitor that makes a value of a
    // view must copy it (InternalValue::FromText) unless the text is known to be borrowed
    template<typename CharT>
    static std::basic_string<CharT> UnwrapRecursive(const SmallString<CharT>& arg, const ValueObject* /*owner*/)
    {
        return std::basic_string<CharT>(arg.View());
    }
    template<typename CharT>
    static std::basic_string_view<CharT> UnwrapRecursive(const HeapString<CharT>& arg, const ValueObject* /*owner*/) noexcept
    {
        return arg.object->View();
    }
    template<typename CharT>
    static std::basic_string_view<CharT> UnwrapRecursive(const StringView<CharT>& arg, const ValueObject* /*owner*/) noexcept
    {
        return arg.View();
    }
    static ListRef UnwrapRecursive(const ListHandle& arg, const ValueObject* owner) noexcept { return ListRef(arg.accessor, owner, arg.isTuple); }
    static MapRef UnwrapRecursive(const MapHandle& arg, const ValueObject* owner) noexcept { return MapRef(arg.accessor, owner); }
    template<typename T>
    static const T& UnwrapRecursive(const BoxHandle<T>& arg, const ValueObject* /*owner*/) noexcept
    {
        return *arg.value;
    }

    // The visitor gets every argument as a const lvalue, views included, so that an
    // overload taking `const ListRef&` beats a forwarding-reference template
    template<typename... Args>
    auto operator()(const Args&... args) const
    {
        static_assert(sizeof...(Args) == N);
        return Call(std::index_sequence_for<Args...>{}, args...);
    }

private:
    template<size_t... indices, typename... Args>
    [[nodiscard]] auto Call(std::index_sequence<indices...> /*sequence*/, const Args&... args) const
    {
        assert(m_visitor != nullptr);
        return [this](const auto&... unwrapped) { return (*m_visitor)(unwrapped...); }(UnwrapRecursive(args, m_owners[indices])...);
    }
};

// Calls fn with what val holds and the object that owns it; a host value is visited as what
// it holds
template<typename Fn>
auto ApplyUnwrapped(const InternalValue& val, Fn&& fn)
{
    const auto& data = InternalValueAccess::Data(val);
    if (const auto* valueRef = std::get_if<ValueRef>(&data))
    {
        return std::forward<Fn>(fn)(valueRef->get().data(), nullptr);
    }
    return std::forward<Fn>(fn)(data, InternalValueAccess::Object(val));
}
} // namespace detail

template<typename V, typename... Args>
// NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward): args construct a visitor per alternative
auto Apply(const InternalValue& val, Args&&... args)
{
    return detail::ApplyUnwrapped(val, [&args...](auto& data, const ValueObject* owner) {
        auto v = V(args...);
        return std::visit(detail::RecursiveUnwrapper<V, 1>(&v, { owner }), data);
    });
}

template<typename V, typename... Args>
// NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward): args construct a visitor per alternative
auto Apply2(const InternalValue& val1, const InternalValue& val2, Args&&... args)
{
    return detail::ApplyUnwrapped(val1, [&val2, &args...](auto& uwVal1, const ValueObject* owner1) {
        return detail::ApplyUnwrapped(val2, [&uwVal1, owner1, &args...](auto& uwVal2, const ValueObject* owner2) {
            auto v = V(args...);
            return std::visit(detail::RecursiveUnwrapper<V, 2>(&v, { owner1, owner2 }), uwVal1, uwVal2);
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
// exponent form outside [1e-4, 1e16), and ".0" on whole numbers. The text lives in the
// object, so writing a float allocates nothing
class PythonFloatText
{
public:
    explicit PythonFloatText(double val)
    {
        if (std::isnan(val))
        {
            m_text = "nan";
            return;
        }
        if (std::isinf(val))
        {
            m_text = val < 0 ? "-inf" : "inf";
            return;
        }
        // The longest shortest form, "-2.2250738585072014e-308", is 24 characters
        char* end = m_buffer;
        end = fmt::format_to(end, "{}", val);
        std::string_view digits(m_buffer, static_cast<size_t>(end - m_buffer));
        if (digits.find_first_of(".e") == std::string_view::npos)
        {
            *end++ = '.';
            *end++ = '0';
        }
        m_text = std::string_view(m_buffer, static_cast<size_t>(end - m_buffer));
    }
    // The view points into the object itself
    PythonFloatText(const PythonFloatText&) = delete;
    PythonFloatText& operator=(const PythonFloatText&) = delete;
    ~PythonFloatText() = default;

    [[nodiscard]] std::string_view View() const { return m_text; }

private:
    char m_buffer[32]{};
    std::string_view m_text;
};

inline std::string FormatPythonFloat(double val)
{
    return std::string(PythonFloatText(val).View());
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
    void operator()(double val) const { AppendAscii(PythonFloatText(val).View()); }
    // Integers are the most common output of a table: format_int skips fmt's format string
    void operator()(int64_t val) const
    {
        const fmt::format_int text(val);
        AppendAscii({ text.data(), text.size() });
    }
    void operator()(bool val) const { AppendAscii(val ? "True" : "False"); }
    void operator()(const std::basic_string_view<CharT>& val) const { AppendString(val); }
    void operator()(const std::basic_string<CharT>& val) const { AppendString(val); }

    void operator()(const EmptyValue&) const { AppendAscii("None"); }
    // Undefined prints as empty; inside a container, as Python's repr does, as Undefined
    void operator()(const UndefinedValue& val) const
    {
        if (m_asRepr)
        {
            AppendAscii("Undefined");
            return;
        }
        CheckStrictUndefined(val);
        if (val.info && val.info->policy == UndefinedPolicy::Debug)
        {
            AppendString(ConvertString<std::basic_string<CharT>>(DebugUndefinedText(*val.info)));
        }
    }
    void operator()(const ListRef& list) const;
    void operator()(const MapRef& map) const;
    void operator()(const KeyValuePair& pair) const;
    void operator()(const ValuesList& list) const { RenderConverted(list); }
    void operator()(const ValuesMap& map) const { RenderConverted(map); }
    void operator()(const GenericList& list) const { RenderConverted(list); }
    void operator()(const GenericMap& map) const { RenderConverted(map); }
    void operator()(const ValueRef&) const {}
    void operator()(const Callable&) const {}
    void operator()(const UserCallable&) const {}
    void operator()(const std::shared_ptr<IRendererBase>&) const {}
    template<typename T>
    void operator()(const RecWrapper<T>& val) const
    {
        (*this)(*val);
    }

    [[nodiscard]] auto GetOs() const { return std::back_inserter(*m_os); }

    void AppendAscii(std::string_view str) const
    {
        if constexpr (std::is_same_v<CharT, char>)
        {
            m_os->append(str.data(), str.size());
        }
        else
        {
            m_os->append(str.begin(), str.end());
        }
    }
    void AppendString(std::basic_string_view<CharT> str) const;
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
    {
        AppendAscii(fmt::format("\\x{:02x}", cp));
    }
    else if (cp < 0x10000)
    {
        AppendAscii(fmt::format("\\u{:04x}", cp));
    }
    else
    {
        AppendAscii(fmt::format("\\U{:08x}", cp));
    }
}

namespace detail
{
// Decodes one code point from UTF-8 (char) or UTF-16/UTF-32 (wchar_t) starting at pos and
// returns the number of code units it takes. Malformed input yields one unit as is.
inline size_t DecodeCodePoint(std::string_view str, size_t pos, uint32_t& cp)
{
    auto unit = [&str](size_t idx) { return static_cast<uint32_t>(static_cast<unsigned char>(str[idx])); };
    uint32_t lead = unit(pos);
    size_t len = lead < 0x80 ? 1 : (lead >> 5) == 0x6 ? 2
                               : (lead >> 4) == 0xe   ? 3
                               : (lead >> 3) == 0x1e  ? 4
                                                      : 0;
    cp = lead;
    if (len <= 1 || pos + len > str.size())
    {
        return 1;
    }

    uint32_t result = lead & (0xff >> (len + 1));
    for (size_t idx = 1; idx != len; ++idx)
    {
        uint32_t next = unit(pos + idx);
        if ((next & 0xc0) != 0x80)
        {
            return 1;
        }
        result = (result << 6) | (next & 0x3f);
    }
    static const uint32_t minValue[] = { 0, 0, 0x80, 0x800, 0x10000 };
    if (result < minValue[len] || result > 0x10ffff || (result >= 0xd800 && result <= 0xdfff))
    {
        return 1;
    }
    cp = result;
    return len;
}

inline size_t DecodeCodePoint(std::wstring_view str, size_t pos, uint32_t& cp)
{
    cp = CodeUnit(str[pos]);
    if (sizeof(wchar_t) == 2 && cp >= 0xd800 && cp <= 0xdbff && pos + 1 < str.size())
    {
        auto low = CodeUnit(str[pos + 1]);
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
void ValueRendererBase<CharT>::AppendString(std::basic_string_view<CharT> str) const
{
    if (!m_asRepr)
    {
        m_os->append(str.data(), str.size());
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
        {
            AppendAscii("\\t");
        }
        else if (cp == '\n')
        {
            AppendAscii("\\n");
        }
        else if (cp == '\r')
        {
            AppendAscii("\\r");
        }
        else if (!jinja2::detail::IsPythonPrintable(cp))
        {
            AppendCodePointEscape(cp);
        }
        else
        {
            m_os->append(str.data() + pos, len);
        }
        pos += len;
    }
    m_os->push_back(quote);
}

struct InputValueConvertor
{
    // Every alternative of Value converts, so the result is never empty: a caller has
    // no fallback that could borrow from a value about to die (docs/tasks/0055).
    using result_t = InternalValue;

    InputValueConvertor(bool byValue, bool allowStringRef)
        : m_byValue(byValue)
        , m_allowStringRef(allowStringRef)
    {
    }

    template<typename ChT>
    result_t operator()(const std::basic_string<ChT>& val) const
    {
        if (m_allowStringRef)
        {
            return result_t(TargetStringView(std::basic_string_view<ChT>(val)));
        }

        return InternalValue::FromText(std::basic_string_view<ChT>(val));
    }

    template<typename ChT>
    result_t operator()(std::basic_string<ChT>& val) const
    {
        return InternalValue::FromText(std::basic_string_view<ChT>(val));
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

    result_t operator()(UserCallable& val) const { return ConvertUserCallable(val); }

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
    void operator()(const std::wstring_view& str) const { AppendString(ConvertString<std::string>(str)); }
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
    void operator()(const std::string_view& str) const { AppendString(ConvertString<std::wstring>(str)); }
};

// Writes str(val), or repr(val) when asRepr is set: Markup's repr is Markup('...')
template<typename CharT>
void RenderValue(const InternalValue& val, std::basic_string<CharT>& os, bool asRepr, typename ValueRendererBase<CharT>::ContainerStack* containers = nullptr)
{
    const bool asMarkup = asRepr && val.IsMarkup() && IsStringValue(val);
    if (asMarkup)
    {
        os.append({ CharT('M'), CharT('a'), CharT('r'), CharT('k'), CharT('u'), CharT('p'), CharT('(') });
    }
    Apply<ValueRenderer<CharT>>(val, os, asRepr, containers);
    if (asMarkup)
    {
        os.push_back(CharT(')'));
    }
}

template<typename CharT>
void ValueRendererBase<CharT>::RenderRepr(const InternalValue& val, ContainerStack* containers) const
{
    RenderValue(val, *m_os, true, containers);
}

template<typename CharT>
template<typename T>
void ValueRendererBase<CharT>::RenderConverted(const T& val) const
{
    Apply<ValueRenderer<CharT>>(InputValueConvertor(false, true)(val), *m_os, m_asRepr, m_containers);
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
void ValueRendererBase<CharT>::operator()(const ListRef& list) const
{
    if (const auto* range = list.GetRangeInfo())
    {
        // Python prints a range by its arguments, the step only when it is not 1
        AppendAscii("range(" + std::to_string(range->start) + ", " + std::to_string(range->stop));
        if (range->step != 1)
        {
            AppendAscii(", " + std::to_string(range->step));
        }
        AppendAscii(")");
        return;
    }

    bool isTuple = list.IsTuple();
    ContainerStack ownContainers;
    auto& containers = m_containers ? *m_containers : ownContainers;
    if (!EnterContainer(list.GetIdentity(), containers, isTuple ? "(...)" : "[...]"))
    {
        return;
    }

    AppendAscii(isTuple ? "(" : "[");
    size_t count = 0;
    for (const auto& item : list)
    {
        if (count++ != 0)
        {
            AppendAscii(", ");
        }
        RenderRepr(item, &containers);
    }
    if (isTuple && count == 1)
    {
        AppendAscii(",");
    }
    AppendAscii(isTuple ? ")" : "]");
    containers.pop_back();
}

template<typename CharT>
void ValueRendererBase<CharT>::operator()(const MapRef& map) const
{
    // An object prints as Python's repr of it: <Namespace {...}> wraps its attributes, the
    // rest have the default <module.Class object at 0x...>
    const char* objectClass = map.GetObjectClass();
    if (objectClass && !map.IsNamespace())
    {
        AppendAscii("<jinja2.utils.");
        AppendAscii(objectClass);
        AppendAscii(" object at ");
        AppendAscii(fmt::format("{}", map.GetIdentity()).c_str());
        AppendAscii(">");
        return;
    }
    if (objectClass)
    {
        AppendAscii("<Namespace ");
    }

    ContainerStack ownContainers;
    auto& containers = m_containers ? *m_containers : ownContainers;
    if (!EnterContainer(map.GetIdentity(), containers, "{...}"))
    {
        if (objectClass)
        {
            AppendAscii(">");
        }
        return;
    }

    // Python prints dicts in their own order: ValuesMap and InternalDict keep insertion order, reflected
    // structs report sorted keys
    auto keys = map.GetKeys();

    ValueRenderer<CharT> keyRenderer(*m_os, true);
    AppendAscii("{");
    bool isFirst = true;
    for (auto& key : keys)
    {
        if (!isFirst)
        {
            AppendAscii(", ");
        }
        isFirst = false;
        keyRenderer(key);
        AppendAscii(": ");
        RenderRepr(map.GetValueByName(key), &containers);
    }
    AppendAscii("}");
    if (objectClass)
    {
        AppendAscii(">");
    }
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

// Python's name for the type of a value, for error messages
inline const char* PythonTypeName(const EmptyValue&)
{
    return "NoneType";
}
inline const char* PythonTypeName(const UndefinedValue&)
{
    return "Undefined";
}

inline const char* PythonTypeName(bool)
{
    return "bool";
}
inline const char* PythonTypeName(int64_t)
{
    return "int";
}
inline const char* PythonTypeName(double)
{
    return "float";
}
template<typename CharT>
const char* PythonTypeName(const std::basic_string<CharT>&)
{
    return "str";
}
template<typename CharT>
const char* PythonTypeName(const std::basic_string_view<CharT>&)
{
    return "str";
}
inline const char* PythonTypeName(const ListRef& list)
{
    return list.IsTuple() ? "tuple" : "list";
}
inline const char* PythonTypeName(const MapRef& map)
{
    const char* objectClass = map.GetObjectClass();
    return objectClass ? objectClass : "dict";
}
inline const char* PythonTypeName(const KeyValuePair&)
{
    return "tuple";
}

// A dictsort pair as the 2-tuple (key, value) it stands for in Python
ListAdapter PairAsTuple(const KeyValuePair& pair);
inline const char* PythonTypeName(const Callable&)
{
    return "function";
}
template<typename T>
const char* PythonTypeName(const T&)
{
    return "object";
}

struct PythonTypeNameGetter
{
    template<typename T>
    const char* operator()(const T& val) const
    {
        return PythonTypeName(val);
    }
};

template<typename T>
struct IsStringType : std::false_type
{
};
template<typename CharT>
struct IsStringType<std::basic_string<CharT>> : std::true_type
{
};
template<typename CharT>
struct IsStringType<std::basic_string_view<CharT>> : std::true_type
{
};
template<typename T>
inline constexpr bool IsStringType_v = IsStringType<T>::value;

// Overflow-checked int64 arithmetic. Python integers are unbounded; Jinja2C++ raises
// instead of switching to big integers (a deliberate divergence, docs/parity.md).
inline bool AddOverflows(int64_t a, int64_t b, int64_t& result)
{
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_add_overflow(a, b, &result);
#else
    if ((b > 0 && a > std::numeric_limits<int64_t>::max() - b) || (b < 0 && a < std::numeric_limits<int64_t>::min() - b))
    {
        return true;
    }
    result = a + b;
    return false;
#endif
}

inline bool SubOverflows(int64_t a, int64_t b, int64_t& result)
{
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_sub_overflow(a, b, &result);
#else
    if ((b < 0 && a > std::numeric_limits<int64_t>::max() + b) || (b > 0 && a < std::numeric_limits<int64_t>::min() + b))
    {
        return true;
    }
    result = a - b;
    return false;
#endif
}

inline bool MulOverflows(int64_t a, int64_t b, int64_t& result)
{
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_mul_overflow(a, b, &result);
#else
    const auto maxVal = std::numeric_limits<int64_t>::max();
    const auto minVal = std::numeric_limits<int64_t>::min();
    bool overflows = false;
    if (a > 0)
    {
        overflows = b > 0 ? a > maxVal / b : b < minVal / a;
    }
    else
    {
        overflows = b > 0 ? a < minVal / b : (a != 0 && b < maxVal / a);
    }
    if (overflows)
    {
        return true;
    }
    result = a * b;
    return false;
#endif
}

[[noreturn]] inline void ThrowIntegerOverflow()
{
    throw std::runtime_error("integer result does not fit in 64 bits");
}

struct UnaryOperation : BaseVisitor<InternalValue>
{
    explicit UnaryOperation(UnaryExpression::Operation oper)
        : m_oper(oper)
    {
    }

    InternalValue operator()(int64_t val) const
    {
        switch (m_oper)
        {
        case jinja2::UnaryExpression::LogicalNot:
            return val == 0;
        case jinja2::UnaryExpression::UnaryPlus:
            return val;
        case jinja2::UnaryExpression::UnaryMinus:
            if (val == std::numeric_limits<int64_t>::min())
            {
                ThrowIntegerOverflow();
            }
            return -val;
        }
        return InternalValue();
    }

    InternalValue operator()(double val) const
    {
        switch (m_oper)
        {
        case jinja2::UnaryExpression::LogicalNot:
            return val == 0.0;
        case jinja2::UnaryExpression::UnaryPlus:
            return val;
        case jinja2::UnaryExpression::UnaryMinus:
            return -val;
        }
        return InternalValue();
    }

    // bool is an int subclass: -True is -1
    InternalValue operator()(bool val) const
    {
        if (m_oper == jinja2::UnaryExpression::LogicalNot)
        {
            return !val;
        }
        return this->operator()(static_cast<int64_t>(val));
    }

    template<typename T>
    InternalValue operator()(const T& val) const
    {
        const char* oper = m_oper == jinja2::UnaryExpression::UnaryMinus ? "-" : "+";
        using namespace std::string_literals;
        throw std::runtime_error("bad operand type for unary "s + oper + ": '" + PythonTypeName(val) + "'");
    }

    UnaryExpression::Operation m_oper;
};

// Binary arithmetic and comparison with Python semantics: bool is an int, int op int stays
// an int (overflow raises), `/` is true division, `//` and `%` floor, operands of unrelated
// types compare unequal and raise TypeError for everything else.
struct BinaryMathOperation : BaseVisitor<>
{
    using ResultType = InternalValue;

    explicit BinaryMathOperation(BinaryExpression::Operation oper, BinaryExpression::CompareType compType = BinaryExpression::CaseSensitive)
        : m_oper(oper)
        , m_compType(compType)
    {
    }

    [[nodiscard]] bool IsComparison() const
    {
        return m_oper >= BinaryExpression::LogicalEq && m_oper <= BinaryExpression::LogicalLe;
    }

    [[nodiscard]] bool IsOrdering() const
    {
        return m_oper == BinaryExpression::LogicalLt || m_oper == BinaryExpression::LogicalLe || m_oper == BinaryExpression::LogicalGt || m_oper == BinaryExpression::LogicalGe;
    }

    [[nodiscard]] const char* OperatorName() const
    {
        switch (m_oper)
        {
        case BinaryExpression::Plus:
            return "+";
        case BinaryExpression::Minus:
            return "-";
        case BinaryExpression::Mul:
            return "*";
        case BinaryExpression::Div:
            return "/";
        case BinaryExpression::DivInteger:
            return "//";
        case BinaryExpression::DivRemainder:
            return "%";
        case BinaryExpression::Pow:
            return "** or pow()";
        case BinaryExpression::LogicalLt:
            return "<";
        case BinaryExpression::LogicalLe:
            return "<=";
        case BinaryExpression::LogicalGt:
            return ">";
        case BinaryExpression::LogicalGe:
            return ">=";
        default:
            return "?";
        }
    }

    template<typename L, typename R>
    [[noreturn]] void ThrowUnsupported(const L& left, const R& right) const
    {
        const std::string leftType = PythonTypeName(left);
        const std::string rightType = PythonTypeName(right);
        using namespace std::string_literals;
        if (IsOrdering())
        {
            throw std::runtime_error("'"s + OperatorName() + "' not supported between instances of '" + leftType + "' and '" + rightType + "'");
        }
        if (m_oper == BinaryExpression::Plus && leftType == "str")
        {
            throw std::runtime_error("can only concatenate str (not \"" + rightType + "\") to str");
        }
        throw std::runtime_error("unsupported operand type(s) for "s + OperatorName() + ": '" + leftType + "' and '" + rightType + "'");
    }

    // Operands that have no operation in common: unequal, anything else is a TypeError
    template<typename L, typename R>
    [[nodiscard]] ResultType Mismatch(const L& left, const R& right) const
    {
        if (m_oper == BinaryExpression::DivRemainder && IsStringType_v<L>)
        {
            return PercentFormat(left, right);
        }
        if (m_oper == BinaryExpression::LogicalEq)
        {
            return false;
        }
        if (m_oper == BinaryExpression::LogicalNe)
        {
            return true;
        }
        ThrowUnsupported(left, right);
    }

    // printf-style `str % args` is task 0020's; until it lands the result is empty
    template<typename L, typename R>
    [[nodiscard]] ResultType PercentFormat(const L& /*format*/, const R& /*args*/) const
    {
        return InternalValue();
    }

    // Maps a three-way comparison result (-1, 0, 1; 2 for unordered NaN) to the operator
    [[nodiscard]] ResultType FromCompare(int cmp) const
    {
        switch (m_oper)
        {
        case BinaryExpression::LogicalEq:
            return cmp == 0;
        case BinaryExpression::LogicalNe:
            return cmp != 0;
        case BinaryExpression::LogicalLt:
            return cmp == -1;
        case BinaryExpression::LogicalLe:
            return cmp == -1 || cmp == 0;
        case BinaryExpression::LogicalGt:
            return cmp == 1;
        case BinaryExpression::LogicalGe:
            return cmp == 1 || cmp == 0;
        default:
            return InternalValue();
        }
    }

    // Python compares int and float exactly, not by converting the int to a double
    static int CompareIntDouble(int64_t left, double right)
    {
        if (std::isnan(right))
        {
            return 2;
        }
        if (right >= 9223372036854775808.0)
        {
            return -1;
        }
        if (right < -9223372036854775808.0)
        {
            return 1;
        }
        double whole = 0;
        const double frac = std::modf(right, &whole);
        const auto rightWhole = static_cast<int64_t>(whole);
        if (left != rightWhole)
        {
            return left < rightWhole ? -1 : 1;
        }
        return frac > 0 ? -1 : (frac < 0 ? 1 : 0);
    }

    ResultType operator()(double left, double right) const
    {
        switch (m_oper)
        {
        case jinja2::BinaryExpression::Plus:
            return left + right;
        case jinja2::BinaryExpression::Minus:
            return left - right;
        case jinja2::BinaryExpression::Mul:
            return left * right;
        case jinja2::BinaryExpression::Div:
            if (right == 0.0)
            {
                throw std::runtime_error("float division by zero");
            }
            return left / right;
        case jinja2::BinaryExpression::DivRemainder:
        {
            if (right == 0.0)
            {
                throw std::runtime_error("float modulo by zero");
            }
            double mod = 0;
            FloatDivMod(left, right, mod);
            return mod;
        }
        case jinja2::BinaryExpression::DivInteger:
        {
            if (right == 0.0)
            {
                throw std::runtime_error("float floor division by zero");
            }
            double mod = 0;
            return FloatDivMod(left, right, mod);
        }
        case jinja2::BinaryExpression::Pow:
        {
            if (left == 0.0 && right < 0)
            {
                throw std::runtime_error("0.0 cannot be raised to a negative power");
            }
            const double result = std::pow(left, right);
            if (std::isinf(result) && std::isfinite(left) && std::isfinite(right))
            {
                throw std::runtime_error("(34, 'Numerical result out of range')");
            }
            return result;
        }
        case jinja2::BinaryExpression::LogicalEq:
            return left == right;
        case jinja2::BinaryExpression::LogicalNe:
            return left != right;
        case jinja2::BinaryExpression::LogicalGt:
            return left > right;
        case jinja2::BinaryExpression::LogicalLt:
            return left < right;
        case jinja2::BinaryExpression::LogicalGe:
            return left >= right;
        case jinja2::BinaryExpression::LogicalLe:
            return left <= right;
        default:
            return InternalValue();
        }
    }

    // CPython's _float_div_mod: returns the floored quotient, stores the modulo (sign of the divisor)
    static double FloatDivMod(double left, double right, double& mod)
    {
        mod = std::fmod(left, right);
        double div = (left - mod) / right;
        if (mod != 0.0)
        {
            if ((right < 0) != (mod < 0))
            {
                mod += right;
                div -= 1.0;
            }
        }
        else
        {
            mod = std::copysign(0.0, right);
        }

        if (div == 0.0)
        {
            return std::copysign(0.0, left / right);
        }
        double floorDiv = std::floor(div);
        if (div - floorDiv > 0.5)
        {
            floorDiv += 1.0;
        }
        return floorDiv;
    }

    // int ** int is an int unless the exponent is negative
    static ResultType IntegerPow(int64_t base, int64_t exp)
    {
        if (exp < 0)
        {
            if (base == 0)
            {
                throw std::runtime_error("0.0 cannot be raised to a negative power");
            }
            return std::pow(static_cast<double>(base), static_cast<double>(exp));
        }

        int64_t result = 1;
        while (exp != 0)
        {
            if ((exp & 1) != 0 && MulOverflows(result, base, result))
            {
                ThrowIntegerOverflow();
            }
            exp >>= 1;
            // Squaring only matters while bits remain; once it overflows, so would the result
            if (exp != 0 && MulOverflows(base, base, base))
            {
                ThrowIntegerOverflow();
            }
        }
        return result;
    }

    // Python's // on ints: the floored quotient
    static int64_t IntegerFloorDiv(int64_t left, int64_t right)
    {
        if (right == 0)
        {
            throw std::runtime_error("integer division or modulo by zero");
        }
        if (right == -1 && left == std::numeric_limits<int64_t>::min())
        {
            ThrowIntegerOverflow();
        }
        int64_t quot = left / right;
        if (left % right != 0 && (left < 0) != (right < 0))
        {
            --quot;
        }
        return quot;
    }

    // Python's % on ints
    static int64_t IntegerRemainder(int64_t left, int64_t right)
    {
        if (right == 0)
        {
            throw std::runtime_error("integer modulo by zero");
        }
        if (right == -1)
        {
            return 0;
        }
        // Python's % takes the sign of the divisor
        int64_t rem = left % right;
        if (rem != 0 && (rem < 0) != (right < 0))
        {
            rem += right;
        }
        return rem;
    }

    ResultType operator()(int64_t left, int64_t right) const
    {
        int64_t result = 0;
        switch (m_oper)
        {
        case jinja2::BinaryExpression::Plus:
            if (AddOverflows(left, right, result))
            {
                ThrowIntegerOverflow();
            }
            return result;
        case jinja2::BinaryExpression::Minus:
            if (SubOverflows(left, right, result))
            {
                ThrowIntegerOverflow();
            }
            return result;
        case jinja2::BinaryExpression::Mul:
            if (MulOverflows(left, right, result))
            {
                ThrowIntegerOverflow();
            }
            return result;
        case jinja2::BinaryExpression::DivInteger:
            return IntegerFloorDiv(left, right);
        case jinja2::BinaryExpression::DivRemainder:
            return IntegerRemainder(left, right);
        case jinja2::BinaryExpression::Pow:
            return IntegerPow(left, right);
        case jinja2::BinaryExpression::Div:
            if (right == 0)
            {
                throw std::runtime_error("division by zero");
            }
            return static_cast<double>(left) / static_cast<double>(right);
        case jinja2::BinaryExpression::LogicalEq:
        case jinja2::BinaryExpression::LogicalNe:
        case jinja2::BinaryExpression::LogicalGt:
        case jinja2::BinaryExpression::LogicalLt:
        case jinja2::BinaryExpression::LogicalGe:
        case jinja2::BinaryExpression::LogicalLe:
            return FromCompare(left < right ? -1 : (left > right ? 1 : 0));
        default:
            return InternalValue();
        }
    }

    ResultType operator()(int64_t left, double right) const
    {
        if (IsComparison())
        {
            return FromCompare(CompareIntDouble(left, right));
        }
        return this->operator()(static_cast<double>(left), right);
    }

    ResultType operator()(double left, int64_t right) const
    {
        if (IsComparison())
        {
            // CompareIntDouble takes the integer first: swap, then negate the order
            const int cmp = CompareIntDouble(right, left); // NOLINT(readability-suspicious-call-argument)
            return FromCompare(cmp == 2 ? 2 : -cmp);
        }
        return this->operator()(left, static_cast<double>(right));
    }

    // bool is an int subclass in Python: True + 1 == 2, True == 1
    ResultType operator()(bool left, bool right) const { return this->operator()(static_cast<int64_t>(left), static_cast<int64_t>(right)); }
    ResultType operator()(bool left, int64_t right) const { return this->operator()(static_cast<int64_t>(left), right); }
    ResultType operator()(int64_t left, bool right) const { return this->operator()(left, static_cast<int64_t>(right)); }
    ResultType operator()(bool left, double right) const { return this->operator()(static_cast<int64_t>(left), right); }
    ResultType operator()(double left, bool right) const { return this->operator()(left, static_cast<int64_t>(right)); }

    template<typename CharT>
    ResultType operator()(const std::basic_string<CharT>& left, const std::basic_string<CharT>& right) const
    {
        return ProcessStrings(std::basic_string_view<CharT>(left), std::basic_string_view<CharT>(right));
    }

    template<typename CharT1, typename CharT2>
    std::enable_if_t<!std::is_same_v<CharT1, CharT2>, ResultType> operator()(const std::basic_string<CharT1>& left, const std::basic_string<CharT2>& right) const
    {
        auto rightStr = ConvertString<std::basic_string<CharT1>>(right);
        return ProcessStrings(std::basic_string_view<CharT1>(left), std::basic_string_view<CharT1>(rightStr));
    }

    template<typename CharT>
    ResultType operator()(const std::basic_string_view<CharT>& left, const std::basic_string<CharT>& right) const
    {
        return ProcessStrings(left, std::basic_string_view<CharT>(right));
    }

    template<typename CharT1, typename CharT2>
    std::enable_if_t<!std::is_same_v<CharT1, CharT2>, ResultType> operator()(const std::basic_string_view<CharT1>& left, const std::basic_string<CharT2>& right) const
    {
        auto rightStr = ConvertString<std::basic_string<CharT1>>(right);
        return ProcessStrings(left, std::basic_string_view<CharT1>(rightStr));
    }

    template<typename CharT>
    ResultType operator()(const std::basic_string<CharT>& left, const std::basic_string_view<CharT>& right) const
    {
        return ProcessStrings(std::basic_string_view<CharT>(left), right);
    }

    template<typename CharT1, typename CharT2>
    std::enable_if_t<!std::is_same_v<CharT1, CharT2>, ResultType> operator()(const std::basic_string<CharT1>& left, const std::basic_string_view<CharT2>& right) const
    {
        auto rightStr = ConvertString<std::basic_string<CharT1>>(right);
        return ProcessStrings(std::basic_string_view<CharT1>(left), std::basic_string_view<CharT1>(rightStr));
    }

    template<typename CharT>
    ResultType operator()(const std::basic_string_view<CharT>& left, const std::basic_string_view<CharT>& right) const
    {
        return ProcessStrings(left, right);
    }

    template<typename CharT1, typename CharT2>
    std::enable_if_t<!std::is_same_v<CharT1, CharT2>, ResultType> operator()(const std::basic_string_view<CharT1>& left, const std::basic_string_view<CharT2>& right) const
    {
        auto rightStr = ConvertString<std::basic_string<CharT1>>(right);
        return ProcessStrings(left, std::basic_string_view<CharT1>(rightStr));
    }

    // str * int and int * str repeat the string
    template<typename S>
    std::enable_if_t<IsStringType_v<S>, ResultType> operator()(const S& left, int64_t right) const
    {
        return RepeatString(left, right, left, right);
    }
    template<typename S>
    std::enable_if_t<IsStringType_v<S>, ResultType> operator()(int64_t left, const S& right) const
    {
        return RepeatString(right, left, left, right);
    }
    template<typename S>
    std::enable_if_t<IsStringType_v<S>, ResultType> operator()(const S& left, bool right) const
    {
        return RepeatString(left, static_cast<int64_t>(right), left, right);
    }
    template<typename S>
    std::enable_if_t<IsStringType_v<S>, ResultType> operator()(bool left, const S& right) const
    {
        return RepeatString(right, static_cast<int64_t>(left), left, right);
    }

    template<typename S, typename L, typename R>
    [[nodiscard]] ResultType RepeatString(const S& str, const int64_t count, const L& left, const R& right) const
    {
        if (m_oper != jinja2::BinaryExpression::Mul)
        {
            return Mismatch(left, right);
        }

        using CharT = typename S::value_type;
        const std::basic_string_view<CharT> text(str);
        if (count <= 0 || text.empty())
        {
            return InternalValue::FromText(std::basic_string_view<CharT>());
        }
        if (static_cast<uint64_t>(count) > MaxSequenceSize / text.size())
        {
            throw std::runtime_error("repeated string is too long");
        }
        return InternalValue::MakeText<CharT>(text.size() * static_cast<size_t>(count), [text, count](boost::span<CharT> chars) {
            auto out = chars.begin();
            for (int64_t i = 0; i < count; ++i)
            {
                out = std::copy(text.begin(), text.end(), out);
            }
        });
    }

    template<typename CharT>
    [[nodiscard]] ResultType ProcessStrings(const std::basic_string_view<CharT>& left, const std::basic_string_view<CharT>& right) const
    {
        ResultType result;

        switch (m_oper)
        {
        case jinja2::BinaryExpression::Plus:
            result = InternalValue::FromText<CharT>({ left, right });
            break;
        case jinja2::BinaryExpression::LogicalEq:
            result = m_compType == BinaryExpression::CaseSensitive ? left == right : jinja2::detail::CompareIgnoreCase(left, right) == 0;
            break;
        case jinja2::BinaryExpression::LogicalNe:
            result = m_compType == BinaryExpression::CaseSensitive ? left != right : jinja2::detail::CompareIgnoreCase(left, right) != 0;
            break;
        case jinja2::BinaryExpression::LogicalGt:
            result = m_compType == BinaryExpression::CaseSensitive ? left > right : jinja2::detail::CompareIgnoreCase(left, right) > 0;
            break;
        case jinja2::BinaryExpression::LogicalLt:
            result = m_compType == BinaryExpression::CaseSensitive ? left < right : jinja2::detail::CompareIgnoreCase(left, right) < 0;
            break;
        case jinja2::BinaryExpression::LogicalGe:
            result = m_compType == BinaryExpression::CaseSensitive ? left >= right : jinja2::detail::CompareIgnoreCase(left, right) >= 0;
            break;
        case jinja2::BinaryExpression::LogicalLe:
            result = m_compType == BinaryExpression::CaseSensitive ? left <= right : jinja2::detail::CompareIgnoreCase(left, right) <= 0;
            break;
        case jinja2::BinaryExpression::DivRemainder:
            result = PercentFormat(left, right);
            break;
        default:
            ThrowUnsupported(left, right);
        }

        return result;
    }

    // A dictsort pair compares, concatenates and repeats as the 2-tuple (key, value). Only the
    // operands a tuple has operations with get an overload: a template over every alternative
    // grows this visitor enough that GCC stops inlining int op int into the evaluator
    ResultType operator()(const KeyValuePair& left, const KeyValuePair& right) const
    {
        const auto leftTuple = PairAsTuple(left);
        const auto rightTuple = PairAsTuple(right);
        return this->operator()(static_cast<const ListRef&>(leftTuple.Ref()), static_cast<const ListRef&>(rightTuple.Ref()));
    }
    ResultType operator()(const KeyValuePair& left, const ListRef& right) const
    {
        const auto tuple = PairAsTuple(left);
        return this->operator()(static_cast<const ListRef&>(tuple.Ref()), right);
    }
    ResultType operator()(const ListRef& left, const KeyValuePair& right) const
    {
        const auto tuple = PairAsTuple(right);
        return this->operator()(left, static_cast<const ListRef&>(tuple.Ref()));
    }
    ResultType operator()(const KeyValuePair& left, int64_t right) const { return RepeatList(PairAsTuple(left).Ref(), right, left, right); }
    ResultType operator()(int64_t left, const KeyValuePair& right) const { return RepeatList(PairAsTuple(right).Ref(), left, left, right); }
    ResultType operator()(const KeyValuePair& left, bool right) const { return RepeatList(PairAsTuple(left).Ref(), static_cast<int64_t>(right), left, right); }
    ResultType operator()(bool left, const KeyValuePair& right) const { return RepeatList(PairAsTuple(right).Ref(), static_cast<int64_t>(left), left, right); }

    ResultType operator()(const ListRef& left, const ListRef& right) const
    {
        // A list and a tuple are never equal and do not combine
        if (left.IsTuple() != right.IsTuple())
        {
            if (m_oper == jinja2::BinaryExpression::Plus)
            {
                using namespace std::string_literals;
                throw std::runtime_error("can only concatenate "s + PythonTypeName(left) + " (not \"" + PythonTypeName(right) + "\") to " + PythonTypeName(left));
            }
            return Mismatch(left, right);
        }

        if (m_oper == jinja2::BinaryExpression::Plus)
        {
            InternalValueList values;
            ReserveHint(values, left.GetSize().value_or(0) + right.GetSize().value_or(0));
            for (const auto& v : left)
            {
                values.push_back(v);
            }
            for (const auto& v : right)
            {
                values.push_back(v);
            }
            auto result = ListAdapter::CreateAdapter(std::move(values));
            if (left.IsTuple())
            {
                result.MarkAsTuple();
            }
            return result;
        }

        if (!IsComparison())
        {
            ThrowUnsupported(left, right);
        }

        // Lexicographic, as Python: the first differing item decides, else the length.
        // Case folding (the filters' case_sensitive=false) applies to a top-level string
        // only, as Jinja2's ignore_case: items inside lists and tuples compare as they are
        auto l = left.begin();
        auto r = right.begin();
        for (; l != left.end() && r != right.end(); ++l, ++r)
        {
            if (ConvertToBool(Apply2<BinaryMathOperation>(*l, *r, BinaryExpression::LogicalEq)))
            {
                continue;
            }
            if (m_oper == BinaryExpression::LogicalEq)
            {
                return false;
            }
            if (m_oper == BinaryExpression::LogicalNe)
            {
                return true;
            }
            return Apply2<BinaryMathOperation>(*l, *r, m_oper);
        }
        const bool leftDone = l == left.end();
        const bool rightDone = r == right.end();
        return FromCompare(leftDone && rightDone ? 0 : (leftDone ? -1 : 1));
    }

    // list * int repeats the list
    ResultType operator()(const ListRef& left, int64_t right) const { return RepeatList(left, right, left, right); }
    ResultType operator()(int64_t left, const ListRef& right) const { return RepeatList(right, left, left, right); }
    ResultType operator()(const ListRef& left, bool right) const { return RepeatList(left, static_cast<int64_t>(right), left, right); }
    ResultType operator()(bool left, const ListRef& right) const { return RepeatList(right, static_cast<int64_t>(left), left, right); }

    template<typename L, typename R>
    [[nodiscard]] ResultType RepeatList(const ListRef& list, int64_t count, const L& left, const R& right) const
    {
        if (m_oper != jinja2::BinaryExpression::Mul)
        {
            return Mismatch(left, right);
        }

        InternalValueList values;
        if (count > 0)
        {
            ReserveHint(values, list.GetSize().value_or(0));
            for (const auto& v : list)
            {
                values.push_back(v);
            }
        }
        const auto size = values.size();
        if (size != 0 && count > 0 && static_cast<uint64_t>(count) > static_cast<uint64_t>(std::numeric_limits<int64_t>::max()) / size)
        {
            throw std::runtime_error("repeated list is too long");
        }
        const auto listSize = size * static_cast<size_t>(count > 0 ? count : 0);
        auto result = ListAdapter::CreateAdapter(listSize, [size, values = std::move(values)](size_t idx) { return values[idx % size]; });
        if (list.IsTuple())
        {
            result.MarkAsTuple();
        }
        return result;
    }

    // Dicts are equal when they hold the same keys with equal values, whatever adapter backs them
    ResultType operator()(const MapRef& left, const MapRef& right) const
    {
        if (m_oper != BinaryExpression::LogicalEq && m_oper != BinaryExpression::LogicalNe)
        {
            return Mismatch(left, right);
        }

        bool equal = left.GetSize() == right.GetSize();
        if (equal)
        {
            for (auto& key : left.GetKeys())
            {
                if (!right.HasValue(key) || !ConvertToBool(Apply2<BinaryMathOperation>(left.GetValueByName(key), right.GetValueByName(key), BinaryExpression::LogicalEq)))
                {
                    equal = false;
                    break;
                }
            }
        }
        return m_oper == BinaryExpression::LogicalEq ? equal : !equal;
    }

    // None and undefined equal only themselves (undefined == undefined, as in Jinja2); mixed pairs go to Mismatch
    ResultType operator()(EmptyValue left, EmptyValue right) const
    {
        switch (m_oper)
        {
        case jinja2::BinaryExpression::LogicalEq:
            return true;
        case jinja2::BinaryExpression::LogicalNe:
            return false;
        default:
            ThrowUnsupported(left, right);
        }
    }

    ResultType operator()(const UndefinedValue& left, const UndefinedValue& right) const
    {
        switch (m_oper)
        {
        case jinja2::BinaryExpression::LogicalEq:
            return true;
        case jinja2::BinaryExpression::LogicalNe:
            return false;
        default:
            ThrowUnsupported(left, right);
        }
    }

    template<typename L, typename R>
    ResultType operator()(const L& left, const R& right) const
    {
        return Mismatch(left, right);
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
        return val != 0.0;
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
    bool operator()(const std::basic_string_view<CharT>& str) const
    {
        return !str.empty();
    }

    bool operator()(const MapRef& val) const
    {
        return val.GetSize() != 0ULL;
    }

    bool operator()(const ListRef& val) const
    {
        return val.GetSize() != 0ULL;
    }

    bool operator()(const EmptyValue&) const
    {
        return false;
    }

    bool operator()(const UndefinedValue& val) const
    {
        CheckStrictUndefined(val);
        return false;
    }

    // A dictsort pair is a non-empty tuple
    bool operator()(const KeyValuePair&) const
    {
        return true;
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
    explicit NumberEvaluator(TargetType def = 0)
        : m_def(def)
    {}

    TargetType operator()(int64_t val) const
    {
        return static_cast<TargetType>(val);
    }
    TargetType operator()(double val) const
    {
        return FromDouble(val, std::is_integral<TargetType>());
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

    // An out-of-range double to integer cast is UB: saturate, and NaN gives the default
    [[nodiscard]] TargetType FromDouble(double val, std::true_type) const
    {
        if (std::isnan(val))
        {
            return m_def;
        }
        if (val >= static_cast<double>(std::numeric_limits<TargetType>::max()))
        {
            return std::numeric_limits<TargetType>::max();
        }
        if (val <= static_cast<double>(std::numeric_limits<TargetType>::min()))
        {
            return std::numeric_limits<TargetType>::min();
        }
        return static_cast<TargetType>(val);
    }
    [[nodiscard]] TargetType FromDouble(double val, std::false_type) const
    {
        return static_cast<TargetType>(val);
    }

    TargetType m_def;
};

using IntegerEvaluator = NumberEvaluator<int64_t>;
using DoubleEvaluator = NumberEvaluator<double>;


template<typename Fn>
struct StringConverterImpl : public BaseVisitor<decltype(std::declval<Fn>()(std::declval<std::string_view>()))>
{
    using R = decltype(std::declval<Fn>()(std::string_view()));
    using BaseVisitor<R>::operator();

    explicit StringConverterImpl(const Fn& fn)
        : m_fn(fn) {}

    template<typename CharT>
    R operator()(const std::basic_string<CharT>& str) const
    {
        return m_fn(std::basic_string_view<CharT>(str));
    }

    template<typename CharT>
    R operator()(const std::basic_string_view<CharT>& str) const
    {
        return m_fn(str);
    }

    const Fn& m_fn;
};

template<typename CharT>
struct SameStringGetter : public visitors::BaseVisitor<nonstd::expected<void, std::basic_string<CharT>>>
{
    using ResultString = std::basic_string<CharT>;
    using OtherString = std::conditional_t<std::is_same_v<CharT, char>, std::wstring, std::string>;
    using ResultStringView = std::basic_string_view<CharT>;
    using OtherStringView = std::conditional_t<std::is_same_v<CharT, char>, std::wstring_view, std::string_view>;
    using Result = nonstd::expected<void, ResultString>;
    using BaseVisitor<Result>::operator();

    Result operator()(const ResultString& str) const
    {
        return MakeUnexpected(str);
    }

    Result operator()(const ResultStringView& str) const
    {
        return MakeUnexpected(ResultString(str.begin(), str.end()));
    }

    Result operator()(const OtherString& str) const
    {
        return MakeUnexpected(ConvertString<ResultString>(str));
    }

    Result operator()(const OtherStringView& str) const
    {
        return MakeUnexpected(ConvertString<ResultString>(str));
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
    using Result = std::optional<std::basic_string<CharT>>;
    auto result = Apply<visitors::SameStringGetter<CharT>>(val);
    if (!result)
    {
        return Result(result.error());
    }

    return Result();
}

template<typename CharT>
auto GetAsSameString(const std::basic_string_view<CharT>&, const InternalValue& val)
{
    using Result = std::optional<std::basic_string<CharT>>;
    auto result = Apply<visitors::SameStringGetter<CharT>>(val);
    if (!result)
    {
        return Result(result.error());
    }

    return Result();
}

namespace visitors
{
// Whether a value holds a number: an int, a float or a bool
inline bool IsNumber(const InternalValue& value)
{
    return GetIf<int64_t>(&value) != nullptr || GetIf<double>(&value) != nullptr || GetIf<bool>(&value) != nullptr;
}

// Calls fn with the number a value holds; the value must hold one (IsNumber)
template<typename Fn>
inline auto VisitNumber(const InternalValue& value, Fn&& fn)
{
    if (const auto* intVal = GetIf<int64_t>(&value))
    {
        return std::forward<Fn>(fn)(*intVal);
    }
    if (const auto* doubleVal = GetIf<double>(&value))
    {
        return std::forward<Fn>(fn)(*doubleVal);
    }
    return std::forward<Fn>(fn)(*GetIf<bool>(&value));
}

// An arithmetic operator or comparison on two numbers (IsNumber): the overload of
// BinaryMathOperation that Apply2 would pick, called without its two variant dispatches
inline InternalValue ApplyToNumbers(const InternalValue& left, const InternalValue& right, BinaryExpression::Operation oper)
{
    return VisitNumber(left, [&](auto leftNum) {
        return VisitNumber(right, [&](auto rightNum) { return visitors::BinaryMathOperation(oper)(leftNum, rightNum); });
    });
}

} // namespace visitors

} // namespace jinja2

#endif // JINJA2CPP_SRC_VALUE_VISITORS_H
