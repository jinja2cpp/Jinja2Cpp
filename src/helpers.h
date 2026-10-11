#ifndef JINJA2CPP_SRC_HELPERS_H
#define JINJA2CPP_SRC_HELPERS_H


#include <cwchar>
#include <ostream>
#include <string>

namespace jinja2
{
struct MultiStringLiteral
{
    const char* charValue;
    const wchar_t* wcharValue;

    constexpr MultiStringLiteral(const char* val, const wchar_t* wval)
        : charValue(val)
        , wcharValue(wval)
    {
    }

    template<typename CharT>
    [[nodiscard]] constexpr auto GetValue() const
    {
#if __cplusplus < 202002L
        return GetValueStr<CharT>();
#else
        constexpr auto memPtr = SelectMemberPtr<CharT, &MultiStringLiteral::charValue, &MultiStringLiteral::wcharValue>::GetPtr();
        return std::basic_string_view<CharT>(this->*memPtr);
#endif
    }

    template<typename CharT>
    [[nodiscard]] constexpr auto GetValueStr() const
    {
        constexpr auto memPtr = SelectMemberPtr<CharT, &MultiStringLiteral::charValue, &MultiStringLiteral::wcharValue>::GetPtr();
        return std::basic_string<CharT>(this->*memPtr);
    }

    // The literal itself, with static storage duration, unlike GetValue, which returns a
    // std::basic_string before C++20.
    template<typename CharT>
    [[nodiscard]] constexpr const CharT* GetCStr() const
    {
        constexpr auto memPtr = SelectMemberPtr<CharT, &MultiStringLiteral::charValue, &MultiStringLiteral::wcharValue>::GetPtr();
        return this->*memPtr;
    }

    template<typename CharT, const char* MultiStringLiteral::*, const wchar_t* MultiStringLiteral::*>
    struct SelectMemberPtr;

    template<const char*(MultiStringLiteral::*charMemPtr), const wchar_t*(MultiStringLiteral::*wcharMemPtr)>
    struct SelectMemberPtr<char, charMemPtr, wcharMemPtr>
    {
        static constexpr auto GetPtr() { return charMemPtr; }
    };

    template<const char*(MultiStringLiteral::*charMemPtr), const wchar_t*(MultiStringLiteral::*wcharMemPtr)>
    struct SelectMemberPtr<wchar_t, charMemPtr, wcharMemPtr>
    {
        static constexpr auto GetPtr() { return wcharMemPtr; }
    };

    template<typename CharT>
    friend std::basic_ostream<CharT>& operator<<(std::basic_ostream<CharT>& os, const MultiStringLiteral& lit)
    {
        os << lit.GetValue<CharT>();
        return os;
    }
};

#define UNIVERSAL_STR(Str) \
    ::jinja2::MultiStringLiteral { Str, L##Str }

//! CompileEscapes replaces escape characters by their meanings, in place.
/**
 * @param[in] first, last Characters with zero or more escape characters.
 * @return The end of the characters left, as std::remove returns it.
 */
template<typename It>
It CompileEscapes(It first, It last)
{
    auto out = first;
    while (first != last)
    {
        if ('\\' == *first)
        {
            if (++first == last)
            {
                break;
            }
            switch (*first)
            {
            case 'n': *first = '\n'; break;
            case 'r': *first = '\r'; break;
            case 't': *first = '\t'; break;
            default: break;
            }
        }
        *out++ = *first++;
    }
    return out;
}

//! The same on a copy of a whole sequence
template<typename Sequence>
Sequence CompileEscapes(Sequence s)
{
    s.erase(CompileEscapes(s.begin(), s.end()), s.end());
    return s;
}

} // namespace jinja2

#endif // JINJA2CPP_SRC_HELPERS_H
