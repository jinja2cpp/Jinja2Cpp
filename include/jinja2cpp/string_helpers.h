#ifndef JINJA2CPP_STRING_HELPERS_H
#define JINJA2CPP_STRING_HELPERS_H

#include "value.h"

#include <cwchar>
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

// The C conversion functions stop at a NUL and need a NUL-terminated source. Convert the
// source one NUL-free segment at a time, from a terminated copy, and keep each NUL.
template<typename DstChar, typename SrcChar, typename SegmentConverter>
std::basic_string<DstChar> ConvertBySegments(std::basic_string_view<SrcChar> from, SegmentConverter convertSegment)
{
    std::basic_string<DstChar> result;
    std::basic_string<SrcChar> segment;
    for (;;)
    {
        const auto nul = from.find(SrcChar());
        segment.assign(from.substr(0, nul));
        if (!segment.empty() && !convertSegment(segment.c_str(), segment.size(), result))
        {
            return {};
        }
        if (nul == std::basic_string_view<SrcChar>::npos)
        {
            return result;
        }
        result.push_back(DstChar());
        from.remove_prefix(nul + 1);
    }
}

template<>
struct StringConverter<std::wstring, std::string>
{
    static std::string DoConvert(const std::wstring_view& from)
    {
        return ConvertBySegments<char>(from, [](const wchar_t* src, std::size_t srcSize, std::string& out) {
            std::mbstate_t state = std::mbstate_t();
            const auto* srcPtr = src;
            std::size_t destBytes = 0;
            const auto pos = out.size();
#ifndef _MSC_VER
            (void)srcSize;
            destBytes = std::wcsrtombs(nullptr, &srcPtr, 0, &state);
            if (destBytes == static_cast<std::size_t>(-1))
            {
                return false;
            }
            out.resize(pos + destBytes + 1);
            srcPtr = src;
            state = std::mbstate_t();
            std::wcsrtombs(&out[pos], &srcPtr, destBytes + 1, &state);
            out.resize(pos + destBytes);
#else
            if (wcsrtombs_s(&destBytes, nullptr, 0, &srcPtr, srcSize, &state) != 0)
            {
                return false;
            }
            out.resize(pos + destBytes);
            srcPtr = src;
            state = std::mbstate_t();
            wcsrtombs_s(&destBytes, &out[pos], destBytes, &srcPtr, destBytes, &state);
            out.resize(pos + destBytes - 1);
#endif
            return true;
        });
    }
};

template<>
struct StringConverter<std::string, std::wstring>
{
    static std::wstring DoConvert(const std::string_view& from)
    {
        return ConvertBySegments<wchar_t>(from, [](const char* src, std::size_t srcSize, std::wstring& out) {
            std::mbstate_t state = std::mbstate_t();
            const auto* srcPtr = src;
            std::size_t destChars = 0;
            const auto pos = out.size();
#ifndef _MSC_VER
            (void)srcSize;
            destChars = std::mbsrtowcs(nullptr, &srcPtr, 0, &state);
            if (destChars == static_cast<std::size_t>(-1))
            {
                return false;
            }
            out.resize(pos + destChars + 1);
            srcPtr = src;
            state = std::mbstate_t();
            std::mbsrtowcs(&out[pos], &srcPtr, destChars + 1, &state);
            out.resize(pos + destChars);
#else
            if (mbsrtowcs_s(&destChars, nullptr, 0, &srcPtr, srcSize, &state) != 0)
            {
                return false;
            }
            out.resize(pos + destChars);
            srcPtr = src;
            state = std::mbstate_t();
            mbsrtowcs_s(&destChars, &out[pos], destChars, &srcPtr, destChars, &state);
            out.resize(pos + destChars - 1);
#endif
            return true;
        });
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
 * A wide string is converted with the C library's current locale (`wcsrtombs`); the result is empty when it cannot
 * be converted.
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
 * A narrow string is converted with the C library's current locale (`mbsrtowcs`); the result is empty when it
 * cannot be converted.
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
