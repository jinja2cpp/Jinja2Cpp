#ifndef HELPERS_H
#define HELPERS_H

#include <string>
#include <type_traits>
#include <cwchar>

namespace jinja2
{
struct MultiStringLiteral
{
    const char* charValue;
    const wchar_t* wcharValue;

    template<typename CharT>
    auto GetValue() const
    {
        auto memPtr = SelectMemberPtr<CharT, &MultiStringLiteral::charValue, &MultiStringLiteral::wcharValue>::GetPtr();
        return std::basic_string<CharT>(this->*memPtr);
    }

    template<typename CharT, const char* MultiStringLiteral::*, const wchar_t* MultiStringLiteral::*>
    struct SelectMemberPtr;

    template<const char* (MultiStringLiteral::*charMemPtr), const wchar_t* (MultiStringLiteral::*wcharMemPtr)>
    struct SelectMemberPtr<char, charMemPtr, wcharMemPtr>
    {
        static auto GetPtr() {return charMemPtr;}
    };

    template<const char* (MultiStringLiteral::*charMemPtr), const wchar_t* (MultiStringLiteral::*wcharMemPtr)>
    struct SelectMemberPtr<wchar_t, charMemPtr, wcharMemPtr>
    {
        static auto GetPtr() {return wcharMemPtr;}
    };

    template<typename CharT>
    friend std::basic_ostream<CharT>& operator << (std::basic_ostream<CharT>& os, const MultiStringLiteral& lit)
    {
        os << lit.GetValue<CharT>();
        return os;
    }
};

#define UNIVERSAL_STR(Str) MultiStringLiteral{Str, L##Str}

namespace detail
{
template<typename Src, typename Dst>
struct StringConverter;

template<typename Src>
struct StringConverter<Src, Src>
{
    static Src DoConvert(const Src& from)
    {
        return from;
    }
};

template<>
struct StringConverter<std::wstring, std::string>
{
    static std::string DoConvert(const std::wstring& from)
    {
        std::mbstate_t state = std::mbstate_t();
        auto src = from.data();
        std::size_t len = 1 + std::wcsrtombs(nullptr, &src, 0, &state);
        std::string result;
        result.resize(len);
        src = from.data();
        std::wcsrtombs(&result[0], &src, from.size(), &state);
        return result;
    }
};

template<>
struct StringConverter<std::string, std::wstring>
{
    static std::wstring DoConvert(const std::string& from)
    {
        std::mbstate_t state = std::mbstate_t();
        auto src = from.data();
        std::size_t len = 1 + std::mbsrtowcs(NULL, &src, 0, &state);
        std::wstring result;
        result.resize(len);
        src = from.data();
        std::mbsrtowcs(&result[0], &src, result.size(), &state);
        return result;
    }
};

} // detail

template<typename Dst, typename Src>
Dst ConvertString(Src&& from)
{
    return detail::StringConverter<std::decay_t<Src>, std::decay_t<Dst>>::DoConvert(std::forward<Src>(from));
}

} // jinja2

#endif // HELPERS_H
