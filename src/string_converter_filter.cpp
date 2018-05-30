#include "filters.h"
#include "testers.h"
#include "value_visitors.h"
#include "value_helpers.h"

#include <algorithm>
#include <numeric>
#include <sstream>

#include <boost/algorithm/string/trim_all.hpp>

namespace ba = boost::algorithm;

namespace jinja2
{

namespace filters
{

template<typename D>
struct StringEncoder : public visitors::BaseVisitor<>
{
    using BaseVisitor::operator();

    template<typename CharT>
    InternalValue operator() (const std::basic_string<CharT>& str) const
    {
        std::basic_string<CharT> result;

        for (auto& ch : str)
        {
            static_cast<const D*>(this)->EncodeChar(ch, [&result](auto ... chs) {AppendChar(result, chs...);});
        }

        return result;
    }

    template<typename Str, typename CharT>
    static void AppendChar(Str& str, CharT ch)
    {
        str.push_back(static_cast<typename Str::value_type>(ch));
    }
    template<typename Str, typename CharT, typename ... Args>
    static void AppendChar(Str& str, CharT ch, Args ... chs)
    {
        str.push_back(static_cast<typename Str::value_type>(ch));
        AppendChar(str, chs...);
    }
};

template<typename Fn>
struct GenericStringEncoder : public StringEncoder<GenericStringEncoder<Fn>>
{
    GenericStringEncoder(Fn fn) : m_fn(std::move(fn)) {}
    
    template<typename CharT, typename AppendFn>
    void EncodeChar(CharT ch, AppendFn&& fn) const
    {
        m_fn(ch, std::forward<AppendFn>(fn));
    }    
    
    mutable Fn m_fn;
};

struct UrlStringEncoder : public StringEncoder<UrlStringEncoder>
{
    template<typename CharT, typename Fn>
    void EncodeChar(CharT ch, Fn&& fn) const
    {
        switch (ch)
        {
        case ' ':
            fn('+');
            break;
        case '+':
            fn('%', '2', 'B');
            break;
        case '\"':
            fn('%', '2', '2');
            break;
        case '%':
            fn('%', '2', '5');
            break;
        case '-':
            fn('%', '2', 'D');
            break;
        case '!':
            fn('%', '2', '1');
            break;
        case '#':
            fn('%', '2', '3');
            break;
        case '$':
            fn('%', '2', '4');
            break;
        case '&':
            fn('%', '2', '6');
            break;
        case '\'':
            fn('%', '2', '7');
            break;
        case '(':
            fn('%', '2', '8');
            break;
        case ')':
            fn('%', '2', '9');
            break;
        case '*':
            fn('%', '2', 'A');
            break;
        case ',':
            fn('%', '2', 'C');
            break;
        case '/':
            fn('%', '2', 'F');
            break;
        case ':':
            fn('%', '3', 'A');
            break;
        case ';':
            fn('%', '3', 'B');
            break;
        case '=':
            fn('%', '3', 'D');
            break;
        case '?':
            fn('%', '3', 'F');
            break;
        case '@':
            fn('%', '4', '0');
            break;
        case '[':
            fn('%', '5', 'B');
            break;
        case ']':
            fn('%', '5', 'D');
            break;
        default:
            fn(ch);
            break;
        }
    }
};

template<typename Fn>
struct StringConverterImpl : public visitors::BaseVisitor<>
{
    using BaseVisitor::operator ();

    StringConverterImpl(const Fn& fn) : m_fn(fn) {}

    template<typename CharT>
    InternalValue operator()(const std::basic_string<CharT>& str) const
    {
        return m_fn(str);
    }

    const Fn& m_fn;
};

template<template<typename> class Cvt = StringConverterImpl, typename Fn>
auto ApplyConverter(const InternalValue& str, Fn&& fn)
{
    return Apply<Cvt<Fn>>(str, std::forward<Fn>(fn));
}


StringConverter::StringConverter(FilterParams params, StringConverter::Mode mode)
    : m_mode(mode)
{
    switch (m_mode)
    {
    case ReplaceMode:
        ParseParams({{"old", true}, {"new", true}, {"count", false}}, params);
        break;
    }
}

InternalValue StringConverter::Filter(const InternalValue& baseVal, RenderContext& context)
{
    InternalValue result;
    
    auto isAlpha = ba::is_alpha();
    auto isAlNum = ba::is_alnum();

    switch (m_mode)
    {
    case TrimMode:
        result = ApplyConverter(baseVal, [](auto str) {
            ba::trim_all(str);
            return str;
        });
        break;
    case TitleMode:
        result = ApplyConverter<GenericStringEncoder>(baseVal, [isDelim = true, &isAlpha, &isAlNum](auto ch, auto&& fn) mutable {
            if (isDelim && isAlpha(ch))
            {
                isDelim = false;
                fn(std::toupper(ch, std::locale()));
                return;
            }
            
            isDelim = !isAlNum(ch);
            fn(ch);
        });
        break;
    case WordCountMode:
    {
        int64_t wc = 0;
        ApplyConverter<GenericStringEncoder>(baseVal, [isDelim = true, &wc, &isAlpha, &isAlNum](auto ch, auto&& fn) mutable {
            if (isDelim && isAlNum(ch))
            {
                isDelim = false;
                wc ++;
                return;
            }
            isDelim = !isAlNum(ch);
        });
        result = wc;
        break;        
    }
    case UpperMode:
        result = ApplyConverter<GenericStringEncoder>(baseVal, [&isAlpha](auto ch, auto&& fn) mutable {
            if (isAlpha(ch))
                fn(std::toupper(ch, std::locale()));
            else
                fn(ch);
        });
        break;
    case LowerMode:
        result = ApplyConverter<GenericStringEncoder>(baseVal, [&isAlpha](auto ch, auto&& fn) mutable {
            if (isAlpha(ch))
                fn(std::tolower(ch, std::locale()));
            else
                fn(ch);
        });
        break;
    case ReplaceMode:
        break;
    case UrlEncodeMode:
        result = Apply<UrlStringEncoder>(baseVal);
        break;
    default:
        break;
    }
    return result;
}

}
}
