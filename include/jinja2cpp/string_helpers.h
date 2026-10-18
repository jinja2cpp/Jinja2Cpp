#ifndef JINJA2CPP_STRING_HELPERS_H
#define JINJA2CPP_STRING_HELPERS_H

#include "fwd.h" // IWYU pragma: export
#include "config.h"
#include "value.h"

#include <cstddef>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>

namespace jinja2
{
namespace detail
{
template<typename Src, typename Dst>
struct StringConverter;

template<typename Src>
struct StringConverter<Src, Src>
{
    static Src DoConvert(const std::basic_string_view<typename Src::value_type>& from)
    {
        return Src(from.begin(), from.end());
    }
};

// std::string holds UTF-8 and std::wstring UTF-32, or UTF-16 where wchar_t is 16 bits wide.
// The codec below does not depend on the C locale. Each malformed sequence becomes U+FFFD:
// in UTF-8 one per maximal subpart of an ill-formed sequence, as Python's
// bytes.decode('utf-8', 'replace') does; in wide text each lone surrogate or value outside
// the Unicode range.
constexpr char32_t ReplacementChar = 0xFFFD;

inline void AppendUtf8(std::string& out, char32_t cp)
{
    if (cp < 0x80)
    {
        out.push_back(static_cast<char>(cp));
    }
    else if (cp < 0x800)
    {
        const char bytes[] = { static_cast<char>(0xC0 | (cp >> 6)), static_cast<char>(0x80 | (cp & 0x3F)) };
        out.append(bytes, sizeof(bytes));
    }
    else if (cp < 0x10000)
    {
        const char bytes[] = { static_cast<char>(0xE0 | (cp >> 12)), static_cast<char>(0x80 | ((cp >> 6) & 0x3F)), static_cast<char>(0x80 | (cp & 0x3F)) };
        out.append(bytes, sizeof(bytes));
    }
    else
    {
        const char bytes[] = { static_cast<char>(0xF0 | (cp >> 18)),
                               static_cast<char>(0x80 | ((cp >> 12) & 0x3F)),
                               static_cast<char>(0x80 | ((cp >> 6) & 0x3F)),
                               static_cast<char>(0x80 | (cp & 0x3F)) };
        out.append(bytes, sizeof(bytes));
    }
}

inline void AppendWide(std::wstring& out, char32_t cp)
{
    if constexpr (sizeof(wchar_t) == 2)
    {
        if (cp >= 0x10000)
        {
            cp -= 0x10000;
            out.push_back(static_cast<wchar_t>(0xD800 + (cp >> 10)));
            out.push_back(static_cast<wchar_t>(0xDC00 + (cp & 0x3FF)));
            return;
        }
    }
    out.push_back(static_cast<wchar_t>(cp));
}

// Decodes the code point at the start of a non-empty str and returns the bytes it takes;
// an ill-formed sequence yields U+FFFD and the length of its maximal subpart (at least 1)
inline std::size_t DecodeUtf8(std::string_view str, char32_t& cp)
{
    const auto lead = static_cast<unsigned char>(str[0]);
    if (lead < 0x80)
    {
        cp = lead;
        return 1;
    }
    std::size_t len = 0;
    unsigned char low = 0x80; // the range of the second byte (Unicode table 3-7)
    unsigned char high = 0xBF;
    if (lead >= 0xC2 && lead <= 0xDF)
    {
        len = 2;
    }
    else if (lead >= 0xE0 && lead <= 0xEF)
    {
        len = 3;
        low = static_cast<unsigned char>(lead == 0xE0 ? 0xA0 : 0x80);
        high = static_cast<unsigned char>(lead == 0xED ? 0x9F : 0xBF);
    }
    else if (lead >= 0xF0 && lead <= 0xF4)
    {
        len = 4;
        low = static_cast<unsigned char>(lead == 0xF0 ? 0x90 : 0x80);
        high = static_cast<unsigned char>(lead == 0xF4 ? 0x8F : 0xBF);
    }
    else
    {
        cp = ReplacementChar;
        return 1;
    }
    char32_t result = lead & (0x7F >> len);
    for (std::size_t n = 1; n != len; ++n)
    {
        if (n == str.size())
        {
            cp = ReplacementChar;
            return n;
        }
        const auto next = static_cast<unsigned char>(str[n]);
        if (next < low || next > high)
        {
            cp = ReplacementChar;
            return n;
        }
        low = 0x80;
        high = 0xBF;
        result = (result << 6) | (next & 0x3F);
    }
    cp = result;
    return len;
}

// Decodes the code point at the start of a non-empty str and returns the units it takes
inline std::size_t DecodeWide(std::wstring_view str, char32_t& cp)
{
    const auto unit = static_cast<char32_t>(static_cast<std::make_unsigned_t<wchar_t>>(str[0]));
    if constexpr (sizeof(wchar_t) == 2)
    {
        if (unit >= 0xD800 && unit <= 0xDBFF && str.size() > 1)
        {
            const auto next = static_cast<char32_t>(static_cast<std::make_unsigned_t<wchar_t>>(str[1]));
            if (next >= 0xDC00 && next <= 0xDFFF)
            {
                cp = 0x10000 + ((unit - 0xD800) << 10) + (next - 0xDC00);
                return 2;
            }
        }
    }
    cp = unit > 0x10FFFF || (unit >= 0xD800 && unit <= 0xDFFF) ? ReplacementChar : unit;
    return 1;
}

template<>
struct StringConverter<std::wstring, std::string>
{
    static std::string DoConvert(std::wstring_view from)
    {
        std::string result;
        result.reserve(from.size());
        while (!from.empty())
        {
            char32_t cp = 0;
            from.remove_prefix(DecodeWide(from, cp));
            AppendUtf8(result, cp);
        }
        return result;
    }
};

template<>
struct StringConverter<std::string, std::wstring>
{
    static std::wstring DoConvert(std::string_view from)
    {
        std::wstring result;
        result.reserve(from.size());
        while (!from.empty())
        {
            const auto lead = static_cast<unsigned char>(from[0]);
            if (lead < 0x80)
            {
                result.push_back(static_cast<wchar_t>(lead));
                from.remove_prefix(1);
                continue;
            }
            char32_t cp = 0;
            from.remove_prefix(DecodeUtf8(from, cp));
            AppendWide(result, cp);
        }
        return result;
    }
};

template<typename CharT, typename T>
struct StringConverter<std::basic_string_view<CharT>, T> : public StringConverter<std::basic_string<CharT>, T>
{
};

} // namespace detail

/*!
 * \brief Convert string objects from one representation or another
 *
 * Converts string or string views to string objects with possible char conversion (char -> wchar_t or wchar_t -> char).
 * This function should be used when exact type of string is needed.
 *
 * Narrow strings are UTF-8; wide strings are UTF-32, or UTF-16 where `wchar_t` is 16 bits wide (Windows). The
 * conversion does not depend on the C locale. A malformed sequence becomes U+FFFD (the replacement character), as
 * Python's `decode('utf-8', 'replace')` does; the rest of the string converts as usual.
 *
 * @tparam Dst Destination string type. Mandatory. Can be std::string or std::wstring
 * @tparam Src Source string type. Auto detected. Can be either std::basic_string<CharT> or std::string_view<CharT>
 *
 * @param from Source string object which should be converted
 * @return Destination string object of the specified type
 */
template<typename Dst, typename Src>
Dst ConvertString(const Src& from)
{
    using src_t = std::decay_t<Src>;
    return detail::StringConverter<src_t, std::decay_t<Dst>>::DoConvert(std::basic_string_view<typename src_t::value_type>(from));
}

/*!
 * \brief Narrow copy of a string of either character type
 *
 * A wide string is converted to UTF-8 as \ref ConvertString does.
 */
inline std::string ToString(std::string_view str)
{
    return std::string(str);
}
//! \copydoc ToString(std::string_view)
inline std::string ToString(std::wstring_view str)
{
    return ConvertString<std::string>(str);
}
//! \copydoc ToString(std::string_view)
inline std::string ToString(const std::string& str)
{
    return str;
}
//! \copydoc ToString(std::string_view)
inline std::string ToString(const std::wstring& str)
{
    return ConvertString<std::string>(str);
}
//! \copydoc ToString(std::string_view)
inline std::string ToString(const char* str)
{
    return std::string(str);
}
//! \copydoc ToString(std::string_view)
inline std::string ToString(const wchar_t* str)
{
    return ConvertString<std::string>(std::wstring_view(str));
}

/*!
 * \brief Wide copy of a string of either character type
 *
 * A narrow string is read as UTF-8 as \ref ConvertString does.
 */
inline std::wstring ToWString(std::wstring_view str)
{
    return std::wstring(str);
}
//! \copydoc ToWString(std::wstring_view)
inline std::wstring ToWString(std::string_view str)
{
    return ConvertString<std::wstring>(str);
}
//! \copydoc ToWString(std::wstring_view)
inline std::wstring ToWString(const std::wstring& str)
{
    return str;
}
//! \copydoc ToWString(std::wstring_view)
inline std::wstring ToWString(const std::string& str)
{
    return ConvertString<std::wstring>(str);
}
//! \copydoc ToWString(std::wstring_view)
inline std::wstring ToWString(const wchar_t* str)
{
    return std::wstring(str);
}
//! \copydoc ToWString(std::wstring_view)
inline std::wstring ToWString(const char* str)
{
    return ConvertString<std::wstring>(std::string_view(str));
}

namespace detail
{
struct StringGetter
{
    template<typename CharT>
    std::string operator()(const std::basic_string<CharT>& str) const
    {
        return ToString(str);
    }
    template<typename CharT>
    std::string operator()(const std::basic_string_view<CharT>& str) const
    {
        return ToString(str);
    }

    template<typename T>
    std::string operator()(T&&) const
    {
        return std::string();
    }
};

struct WStringGetter
{
    template<typename CharT>
    std::wstring operator()(const std::basic_string<CharT>& str) const
    {
        return ToWString(str);
    }
    template<typename CharT>
    std::wstring operator()(const std::basic_string_view<CharT>& str) const
    {
        return ToWString(str);
    }

    template<typename T>
    std::wstring operator()(T&&) const
    {
        return std::wstring();
    }
};
} // namespace detail

// 1.x names, kept as one-line forwards until 3.0 (docs/api-2.0.md 5.1)

JINJA2CPP_DEPRECATED("jinja2cpp-2: ToString")
inline std::string AsString(const std::string& str) { return ToString(str); }
JINJA2CPP_DEPRECATED("jinja2cpp-2: ToString")
inline std::string AsString(const std::wstring& str) { return ToString(str); }
JINJA2CPP_DEPRECATED("jinja2cpp-2: ToString")
inline std::string AsString(const std::string_view& str) { return ToString(str); }
JINJA2CPP_DEPRECATED("jinja2cpp-2: ToString")
inline std::string AsString(const std::wstring_view& str) { return ToString(str); }
JINJA2CPP_DEPRECATED("jinja2cpp-2: ToWString")
inline std::wstring AsWString(const std::wstring& str) { return ToWString(str); }
JINJA2CPP_DEPRECATED("jinja2cpp-2: ToWString")
inline std::wstring AsWString(const std::string& str) { return ToWString(str); }
JINJA2CPP_DEPRECATED("jinja2cpp-2: ToWString")
inline std::wstring AsWString(const std::wstring_view& str) { return ToWString(str); }
JINJA2CPP_DEPRECATED("jinja2cpp-2: ToWString")
inline std::wstring AsWString(const std::string_view& str) { return ToWString(str); }
//! The stored string, converted if needed, or an empty string for any other value, as in 1.x; \ref ToString prints every value
JINJA2CPP_DEPRECATED("jinja2cpp-2: ToString")
inline std::string AsString(const Value& val) { return std::visit(detail::StringGetter(), val.GetData()); }
//! The stored string, converted if needed, or an empty string for any other value, as in 1.x; \ref ToWString prints every value
JINJA2CPP_DEPRECATED("jinja2cpp-2: ToWString")
inline std::wstring AsWString(const Value& val) { return std::visit(detail::WStringGetter(), val.GetData()); }
} // namespace jinja2

#endif // JINJA2CPP_STRING_HELPERS_H
