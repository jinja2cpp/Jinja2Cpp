#include "filters.h"
#include "testers.h"
#include "value_visitors.h"
#include "value_helpers.h"

#include <algorithm>
#include <numeric>
#include <sstream>

#include <boost/algorithm/string/trim_all.hpp>
#include <boost/algorithm/string/replace.hpp>

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

StringConverter::StringConverter(FilterParams params, StringConverter::Mode mode)
    : m_mode(mode)
{
    switch (m_mode)
    {
    case ReplaceMode:
        ParseParams({{"old", true}, {"new", true}, {"count", false, static_cast<int64_t>(0)}}, params);
        break;
    case TruncateMode:
        ParseParams({{"length", false, static_cast<int64_t>(255)}, {"killwords", false, false}, {"end", false, std::string("...")}, {"leeway", false}}, params);
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
        result = ApplyStringConverter(baseVal, [](auto str) -> InternalValue {
            ba::trim_all(str);
            return str;
        });
        break;
    case TitleMode:
        result = ApplyStringConverter<GenericStringEncoder>(baseVal, [isDelim = true, &isAlpha, &isAlNum](auto ch, auto&& fn) mutable {
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
        ApplyStringConverter<GenericStringEncoder>(baseVal, [isDelim = true, &wc, &isAlpha, &isAlNum](auto ch, auto&& fn) mutable {
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
        result = ApplyStringConverter<GenericStringEncoder>(baseVal, [&isAlpha](auto ch, auto&& fn) mutable {
            if (isAlpha(ch))
                fn(std::toupper(ch, std::locale()));
            else
                fn(ch);
        });
        break;
    case LowerMode:
        result = ApplyStringConverter<GenericStringEncoder>(baseVal, [&isAlpha](auto ch, auto&& fn) mutable {
            if (isAlpha(ch))
                fn(std::tolower(ch, std::locale()));
            else
                fn(ch);
        });
        break;
    case ReplaceMode:
        result = ApplyStringConverter(baseVal, [this, &context](auto str) -> InternalValue {
            auto oldStr = GetAsSameString(str, this->GetArgumentValue("old", context));
            auto newStr = GetAsSameString(str, this->GetArgumentValue("new", context));
            auto count = ConvertToInt(this->GetArgumentValue("count", context));
            if (count == 0)
                ba::replace_all(str, oldStr, newStr);
            else
            {
                for (int64_t n = 0; n < count; ++ n)
                    ba::replace_first(str, oldStr, newStr);
            }
            return str;
        });
        break;
    case TruncateMode:
        result = ApplyStringConverter(baseVal, [this, &context, &isAlNum](auto str) -> InternalValue {
            auto length = ConvertToInt(this->GetArgumentValue("length", context));
            auto killWords = ConvertToBool(this->GetArgumentValue("killwords", context));
            auto end = GetAsSameString(str, this->GetArgumentValue("end", context));
            auto leeway = ConvertToInt(this->GetArgumentValue("leeway", context), 5);
            if (str.size() <= length)
                return str;

            if (killWords)
            {
                if (str.size() > (length + leeway))
                {
                    str.erase(str.begin() + length, str.end());
                    str += end;
                }
                return str;
            }

            auto p = str.begin() + length;
            if (leeway != 0)
            {
                for (; leeway != 0 && p != str.end() && isAlNum(*p); -- leeway, ++ p);
                if (p == str.end())
                    return str;
            }

            if (isAlNum(*p))
            {
                for (; p != str.begin() && isAlNum(*p); -- p);
            }
            str.erase(p, str.end());
            ba::trim_right(str);
            str += end;

            return str;
        });
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
