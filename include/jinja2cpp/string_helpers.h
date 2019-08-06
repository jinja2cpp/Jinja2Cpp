#ifndef JINJA2_STRING_HELPERS_H
#define JINJA2_STRING_HELPERS_H

#include <nonstd/string_view.hpp>
#include "value.h"

#include <string>
#include <type_traits>
#include <cwchar>
#include <iostream>

namespace jinja2
{
namespace detail
{
    template<typename Src, typename Dst>
    struct StringConverter;

    template<typename Src>
    struct StringConverter<Src, Src>
    {
        static Src DoConvert(const nonstd::basic_string_view<typename Src::value_type>& from)
        {
            return Src(from.begin(), from.end());
        }
    };

    template<>
    struct StringConverter<std::wstring, std::string>
    {
        static std::string DoConvert(const nonstd::wstring_view& from)
        {
            std::mbstate_t state = std::mbstate_t();
            auto srcPtr = from.data();
            std::size_t srcSize = from.size();
            std::size_t destBytes = 0;

#ifndef _MSC_VER
            destBytes = std::wcsrtombs(nullptr, &srcPtr, srcSize, &state);
            if (destBytes == static_cast<std::size_t>(-1))
                return std::string();
#else
            auto err = wcsrtombs_s(&destBytes, nullptr, 0, &srcPtr, srcSize, &state);
            if (err != 0)
                return std::string();
#endif
            std::string result;
#ifndef _MSC_VER
            result.resize(destBytes + 1);
            auto converted = std::wcsrtombs(&result[0], &srcPtr, srcSize, &state);
            if (converted == static_cast<std::size_t>(-1))
                return std::string();
            result.resize(converted);
            // std::wcout << L"Convert '" << from << L"' of " << srcSize << L" chars len to '";
            // std::cout << result << "' of " << result.size() << " chars len\n";
#else
            result.resize(destBytes);
            wcsrtombs_s(&destBytes, &result[0], destBytes, &srcPtr, srcSize, &state);
            result.resize(destBytes - 1);
#endif
            return result;
        }
    };

    template<>
    struct StringConverter<std::string, std::wstring>
    {
        static std::wstring DoConvert(const nonstd::string_view& from)
        {
            std::mbstate_t state = std::mbstate_t();
            auto srcPtr = from.data();
            std::size_t srcSize = from.size();
            std::size_t destBytes = 0;

#ifndef _MSC_VER
            destBytes = std::mbsrtowcs(nullptr, &srcPtr, srcSize, &state);
            if (destBytes == static_cast<std::size_t>(-1))
                return std::wstring();
#else
            auto err = mbsrtowcs_s(&destBytes, nullptr, 0, &srcPtr, srcSize, &state);
            if (err != 0)
                return std::wstring();
#endif
            std::wstring result;
#ifndef _MSC_VER
            result.resize(destBytes + 1);
            srcPtr = from.data();
            auto converted = std::mbsrtowcs(&result[0], &srcPtr, srcSize, &state);
            if (converted == static_cast<std::size_t>(-1))
                return std::wstring();
            result.resize(converted);
            // std::cout << "Convert '" << from << "' of " << srcSize << " chars len to '";
            // std::wcout << result << L"' of " << result.size() << L" chars len\n";
#else
            result.resize(destBytes);
            mbsrtowcs_s(&destBytes, &result[0], destBytes, &srcPtr, srcSize, &state);
            result.resize(destBytes - 1);
#endif
            return result;
        }
    };

    template<typename CharT, typename T>
    struct StringConverter<nonstd::basic_string_view<CharT>, T> : public StringConverter<std::basic_string<CharT>, T> {};

} // detail

template<typename Dst, typename Src>
Dst ConvertString(Src&& from)
{
    using src_t = std::decay_t<Src>;
    return detail::StringConverter<src_t, std::decay_t<Dst>>::DoConvert(nonstd::basic_string_view<typename src_t::value_type>(from));
}

inline const std::string AsString(const std::string& str)
{
    return str;
}

inline std::string AsString(const std::wstring& str)
{
    return ConvertString<std::string>(str);
}

inline std::string AsString(const nonstd::string_view& str)
{
    return std::string(str.begin(), str.end());
}

inline std::string AsString(const nonstd::wstring_view& str)
{
    return ConvertString<std::string>(str);
}

inline const std::wstring AsWString(const std::wstring& str)
{
    return str;
}

inline std::wstring AsWString(const std::string& str)
{
    return ConvertString<std::wstring>(str);
}

inline std::wstring AsWString(const nonstd::wstring_view& str)
{
    return std::wstring(str.begin(), str.end());
}

inline std::wstring AsWString(const nonstd::string_view& str)
{
    return ConvertString<std::wstring>(str);
}

namespace detail
{
struct StringGetter
{
    template<typename CharT>
    std::string operator()(const std::basic_string<CharT>& str) const
    {
        return AsString(str);
    }
    template<typename CharT>
    std::string operator()(const nonstd::basic_string_view<CharT>& str) const
    {
        return AsString(str);
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
        return AsWString(str);
    }
    template<typename CharT>
    std::wstring operator()(const nonstd::basic_string_view<CharT>& str) const
    {
        return AsWString(str);
    }

    template<typename T>
    std::wstring operator()(T&&) const
    {
        return std::wstring();
    }
};
}

inline std::string AsString(const Value& val)
{
    return nonstd::visit(detail::StringGetter(), val.data());
}

inline std::wstring AsWString(const Value& val)
{
    return nonstd::visit(detail::WStringGetter(), val.data());
}
}

#endif // JINJA2_STRING_HELPERS_H