#ifndef JINJA2CPP_SRC_CASE_MAPPING_H
#define JINJA2CPP_SRC_CASE_MAPPING_H

#include "internal_value.h"
#include "unicode_case.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

// Python's full Unicode case mappings of str.upper(), lower(), capitalize(), swapcase() and
// title(), shared by the case filters and the str methods. They work on code points; ASCII
// fast paths stay with their callers.

namespace jinja2
{
inline namespace v2
{

// Code point of one character produced by SplitCodePoints
inline uint32_t CodePointValue(std::string_view ch)
{
    auto lead = static_cast<unsigned char>(ch[0]);
    if (lead < 0x80 || ch.size() == 1)
    {
        return lead;
    }
    uint32_t value = lead & (lead >= 0xF0 ? 0x07 : lead >= 0xE0 ? 0x0F
                                                                : 0x1F);
    for (size_t n = 1; n < ch.size(); ++n)
    {
        value = (value << 6) | (static_cast<unsigned char>(ch[n]) & 0x3F);
    }
    return value;
}

inline uint32_t CodePointValue(std::wstring_view ch)
{
    auto unit = CodeUnit(ch[0]);
    if (ch.size() == 2 && unit >= 0xD800 && unit <= 0xDBFF)
    {
        return 0x10000 + ((unit - 0xD800) << 10) + (CodeUnit(ch[1]) - 0xDC00);
    }
    return unit;
}

template<typename CharT>
void AppendCodePoint(std::basic_string<CharT>& out, uint32_t cp)
{
    if (sizeof(CharT) == 1)
    {
        if (cp < 0x80)
        {
            out.push_back(static_cast<CharT>(cp));
        }
        else if (cp < 0x800)
        {
            out.append({ static_cast<CharT>(0xC0 | (cp >> 6)), static_cast<CharT>(0x80 | (cp & 0x3F)) });
        }
        else if (cp < 0x10000)
        {
            out.append({ static_cast<CharT>(0xE0 | (cp >> 12)), static_cast<CharT>(0x80 | ((cp >> 6) & 0x3F)), static_cast<CharT>(0x80 | (cp & 0x3F)) });
        }
        else
        {
            out.append({ static_cast<CharT>(0xF0 | (cp >> 18)),
                         static_cast<CharT>(0x80 | ((cp >> 12) & 0x3F)),
                         static_cast<CharT>(0x80 | ((cp >> 6) & 0x3F)),
                         static_cast<CharT>(0x80 | (cp & 0x3F)) });
        }
    }
    else if (sizeof(CharT) == 2 && cp >= 0x10000)
    {
        out.append({ static_cast<CharT>(0xD800 + ((cp - 0x10000) >> 10)), static_cast<CharT>(0xDC00 + ((cp - 0x10000) & 0x3FF)) });
    }
    else
    {
        out.push_back(static_cast<CharT>(cp));
    }
}

// How many code units AppendCodePoint writes for cp
template<typename CharT>
size_t EncodedUnits(uint32_t cp)
{
    if (sizeof(CharT) == 1)
    {
        return cp < 0x80 ? 1 : cp < 0x800 ? 2 : cp < 0x10000 ? 3 : 4;
    }
    return sizeof(CharT) == 2 && cp >= 0x10000 ? 2 : 1;
}

// The code points of a string with the characters they came from, so that characters a case
// mapping leaves alone are copied as they are
template<typename CharT>
struct CaseChars
{
    explicit CaseChars(std::basic_string_view<CharT> str)
        : views(SplitCodePoints(str))
    {
        cps.reserve(views.size());
        for (auto view : views)
        {
            cps.push_back(CodePointValue(view));
        }
    }

    std::vector<std::basic_string_view<CharT>> views;
    std::vector<uint32_t> cps;
};

using CaseFn = size_t (*)(uint32_t, uint32_t (&)[3]);

template<typename CharT>
void AppendMapped(std::basic_string<CharT>& out, const CaseChars<CharT>& chars, size_t idx, CaseFn map)
{
    uint32_t mapped[3];
    auto count = map(chars.cps[idx], mapped);
    if (count == 1 && mapped[0] == chars.cps[idx])
    {
        out.append(chars.views[idx].begin(), chars.views[idx].end());
        return;
    }
    // A mapping can lengthen the string ('\u0390'.upper() is three code points), so the result
    // is held to the sequence limit before it grows
    size_t units = 0;
    for (size_t n = 0; n != count; ++n)
    {
        units += EncodedUnits<CharT>(mapped[n]);
    }
    CheckSequenceSize(static_cast<uint64_t>(out.size()) + units, "a case-mapped string");
    for (size_t n = 0; n != count; ++n)
    {
        AppendCodePoint(out, mapped[n]);
    }
}

// CPython's handle_capital_sigma: the sigma at idx ends a word when a cased character precedes
// it and none follows it, skipping case-ignorable ones; the string is cps[begin, end)
inline bool IsFinalSigma(const std::vector<uint32_t>& cps, size_t begin, size_t end, size_t idx)
{
    auto before = idx;
    while (before > begin && unicode::IsCaseIgnorable(cps[before - 1]))
    {
        --before;
    }
    if (before == begin || !unicode::IsCased(cps[before - 1]))
    {
        return false;
    }
    auto after = idx + 1;
    while (after < end && unicode::IsCaseIgnorable(cps[after]))
    {
        ++after;
    }
    return after == end || !unicode::IsCased(cps[after]);
}

// CPython's lower_ucs4: str.lower() of the character idx of the string cps[begin, end)
template<typename CharT>
void AppendLowerChar(std::basic_string<CharT>& out, const CaseChars<CharT>& chars, size_t begin, size_t end, size_t idx)
{
    if (chars.cps[idx] == 0x3A3)
    {
        AppendCodePoint(out, IsFinalSigma(chars.cps, begin, end, idx) ? 0x3C2U : 0x3C3U);
    }
    else
    {
        AppendMapped(out, chars, idx, unicode::ToLower);
    }
}

// str.lower() of the characters [from, to) of the string cps[begin, to)
template<typename CharT>
void AppendLower(std::basic_string<CharT>& out, const CaseChars<CharT>& chars, size_t begin, size_t from, size_t to)
{
    for (auto idx = from; idx != to; ++idx)
    {
        AppendLowerChar(out, chars, begin, to, idx);
    }
}

template<typename CharT>
std::basic_string<CharT> UpperUnicode(std::basic_string_view<CharT> str)
{
    std::basic_string<CharT> result;
    CaseChars<CharT> chars(str);
    for (size_t idx = 0; idx != chars.cps.size(); ++idx)
    {
        AppendMapped(result, chars, idx, unicode::ToUpper);
    }
    return result;
}

template<typename CharT>
std::basic_string<CharT> LowerUnicode(std::basic_string_view<CharT> str)
{
    std::basic_string<CharT> result;
    CaseChars<CharT> chars(str);
    AppendLower(result, chars, 0, 0, chars.cps.size());
    return result;
}

// str.capitalize(): the first character titlecase, the others lowercase
template<typename CharT>
std::basic_string<CharT> CapitalizeUnicode(std::basic_string_view<CharT> str)
{
    std::basic_string<CharT> result;
    CaseChars<CharT> chars(str);
    if (!chars.cps.empty())
    {
        AppendMapped(result, chars, 0, unicode::ToTitle);
        AppendLower(result, chars, 0, 1, chars.cps.size());
    }
    return result;
}

// str.swapcase(): uppercase characters lowered, lowercase ones uppercased; titlecase and
// uncased characters stay
template<typename CharT>
std::basic_string<CharT> SwapcaseUnicode(std::basic_string_view<CharT> str)
{
    std::basic_string<CharT> result;
    CaseChars<CharT> chars(str);
    for (size_t idx = 0; idx != chars.cps.size(); ++idx)
    {
        if (unicode::IsUppercase(chars.cps[idx]))
        {
            AppendLowerChar(result, chars, 0, chars.cps.size(), idx);
        }
        else if (unicode::IsLowercase(chars.cps[idx]))
        {
            AppendMapped(result, chars, idx, unicode::ToUpper);
        }
        else
        {
            result.append(chars.views[idx].begin(), chars.views[idx].end());
        }
    }
    return result;
}

// str.title(): a character that follows a cased one is lowered, any other is titlecased
template<typename CharT>
std::basic_string<CharT> PythonTitleUnicode(std::basic_string_view<CharT> str)
{
    std::basic_string<CharT> result;
    CaseChars<CharT> chars(str);
    bool prevCased = false;
    for (size_t idx = 0; idx != chars.cps.size(); ++idx)
    {
        if (prevCased)
        {
            AppendLowerChar(result, chars, 0, chars.cps.size(), idx);
        }
        else
        {
            AppendMapped(result, chars, idx, unicode::ToTitle);
        }
        prevCased = unicode::IsCasedChar(chars.cps[idx]);
    }
    return result;
}

} // namespace v2
} // namespace jinja2

#endif // JINJA2CPP_SRC_CASE_MAPPING_H
