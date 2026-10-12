#ifndef JINJA2CPP_SRC_HELPERS_H
#define JINJA2CPP_SRC_HELPERS_H


#include <cwchar>
#include <ostream>
#include <string>
#include <type_traits>

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

namespace detail
{
// Writes one code point to out in the encoding of CharT: UTF-8, UTF-16 or UTF-32 by its size.
// A lone surrogate is written as the unit(s) it would have (Python's unicode-escape yields it too)
template<typename CharT, typename Out>
Out PutCodePoint(Out out, char32_t cp)
{
    if constexpr (sizeof(CharT) == 1)
    {
        if (cp < 0x80)
        {
            *out++ = static_cast<CharT>(cp);
        }
        else if (cp < 0x800)
        {
            *out++ = static_cast<CharT>(0xC0 | (cp >> 6));
            *out++ = static_cast<CharT>(0x80 | (cp & 0x3F));
        }
        else if (cp < 0x10000)
        {
            *out++ = static_cast<CharT>(0xE0 | (cp >> 12));
            *out++ = static_cast<CharT>(0x80 | ((cp >> 6) & 0x3F));
            *out++ = static_cast<CharT>(0x80 | (cp & 0x3F));
        }
        else
        {
            *out++ = static_cast<CharT>(0xF0 | (cp >> 18));
            *out++ = static_cast<CharT>(0x80 | ((cp >> 12) & 0x3F));
            *out++ = static_cast<CharT>(0x80 | ((cp >> 6) & 0x3F));
            *out++ = static_cast<CharT>(0x80 | (cp & 0x3F));
        }
    }
    else if constexpr (sizeof(CharT) == 2)
    {
        if (cp >= 0x10000)
        {
            cp -= 0x10000;
            *out++ = static_cast<CharT>(0xD800 | (cp >> 10));
            *out++ = static_cast<CharT>(0xDC00 | (cp & 0x3FF));
        }
        else
        {
            *out++ = static_cast<CharT>(cp);
        }
    }
    else
    {
        *out++ = static_cast<CharT>(cp);
    }
    return out;
}

constexpr int HexDigitValue(char32_t ch)
{
    if (ch >= '0' && ch <= '9')
    {
        return static_cast<int>(ch - '0');
    }
    if (ch >= 'a' && ch <= 'f')
    {
        return static_cast<int>(ch - 'a') + 10;
    }
    if (ch >= 'A' && ch <= 'F')
    {
        return static_cast<int>(ch - 'A') + 10;
    }
    return -1;
}

// The character of a single-letter escape, or -1 for another letter
constexpr int SimpleEscape(char32_t ch)
{
    switch (ch)
    {
    case '\\': return '\\';
    case '\'': return '\'';
    case '"': return '"';
    case 'a': return '\a';
    case 'b': return '\b';
    case 'f': return '\f';
    case 'n': return '\n';
    case 'r': return '\r';
    case 't': return '\t';
    case 'v': return '\v';
    default: return -1;
    }
}

// The code point of \x, \u or \U escapes: count hex digits at pos, which moves past them
template<typename It>
bool ParseHexEscape(It& pos, It last, int count, char32_t& cp)
{
    cp = 0;
    for (int i = 0; i < count; ++i, ++pos)
    {
        int digit = pos == last ? -1 : HexDigitValue(static_cast<char32_t>(*pos));
        if (digit < 0)
        {
            return false;
        }
        cp = (cp << 4) | static_cast<char32_t>(digit);
    }
    return cp <= 0x10FFFF;
}

// The escape after a backslash that first points at, written to out. False if it is malformed
template<typename CharT, typename It>
bool DecodeEscape(It& first, It last, It& out)
{
    auto ch = static_cast<char32_t>(*first++);
    if (ch == '\n')
    {
        return true;
    }
    if (int simple = SimpleEscape(ch); simple >= 0)
    {
        *out++ = static_cast<CharT>(simple);
        return true;
    }
    char32_t cp = 0;
    if (ch >= '0' && ch <= '7')
    {
        cp = ch - '0';
        for (int i = 0; i < 2 && first != last && *first >= '0' && *first <= '7'; ++i, ++first)
        {
            cp = (cp << 3) | static_cast<char32_t>(*first - '0');
        }
    }
    else if (ch == 'x' || ch == 'u' || ch == 'U')
    {
        if (!ParseHexEscape(first, last, ch == 'x' ? 2 : ch == 'u' ? 4
                                                                   : 8,
                            cp))
        {
            return false;
        }
    }
    else if (ch == 'N')
    {
        return false;
    }
    else
    {
        // Python keeps the backslash of an unknown escape
        *out++ = static_cast<CharT>('\\');
        *out++ = static_cast<CharT>(ch);
        return true;
    }
    out = PutCodePoint<CharT>(out, cp);
    return true;
}
} // namespace detail

//! CompileEscapes replaces escape sequences of a string literal by their meanings, in place.
/**
 * Decodes what Python's unicode-escape does (Jinja2 applies it to string literals): the
 * single-character escapes, backslash-newline (removed), 1-3 digit octal, \xHH, \uHHHH and \UHHHHHHHH,
 * written as UTF-8, UTF-16 or UTF-32 by the size of the character type. Any other escape keeps its
 * backslash. A decoded sequence is never longer than its source, so the output trails the input.
 * \N{name} is not supported and counts as malformed, like a truncated \x, \u or \U or a code point
 * above U+10FFFF.
 *
 * @param[in] first, last Characters with zero or more escape sequences.
 * @param[out] ok Set to false when a sequence is malformed; decoding stops there.
 * @return The end of the characters left, as std::remove returns it.
 */
template<typename It>
It CompileEscapes(It first, It last, bool* ok = nullptr)
{
    using CharT = std::decay_t<decltype(*first)>;
    auto out = first;
    while (first != last)
    {
        if ('\\' != *first)
        {
            *out++ = *first++;
        }
        else if (++first == last)
        {
            *out++ = static_cast<CharT>('\\');
        }
        else if (!detail::DecodeEscape<CharT>(first, last, out))
        {
            if (ok)
            {
                *ok = false;
            }
            break;
        }
    }
    return out;
}

//! The same on a copy of a whole sequence
template<typename Sequence>
Sequence CompileEscapes(Sequence s, bool* ok = nullptr)
{
    s.erase(CompileEscapes(s.begin(), s.end(), ok), s.end());
    return s;
}

} // namespace jinja2

#endif // JINJA2CPP_SRC_HELPERS_H
