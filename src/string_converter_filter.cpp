#include "filters.h"
#include "testers.h"
#include "value_visitors.h"
#include "value_helpers.h"

#include <cctype>
#include <algorithm>
#include <numeric>
#include <regex>
#include <sstream>

#include <boost/algorithm/string/trim_all.hpp>
#include <boost/algorithm/string/replace.hpp>

namespace ba = boost::algorithm;

namespace jinja2
{

namespace filters
{

template<typename D>
struct StringEncoder : public visitors::BaseVisitor<TargetString>
{
    using BaseVisitor::operator();

    template<typename CharT>
    TargetString operator()(const std::basic_string<CharT>& str) const
    {
        std::basic_string<CharT> result;

        for (auto& ch : str)
        {
            static_cast<const D*>(this)->EncodeChar(ch, [&result](auto... chs) { AppendChar(result, chs...); });
        }

        return TargetString(std::move(result));
    }

    template<typename CharT>
    TargetString operator()(const nonstd::basic_string_view<CharT>& str) const
    {
        std::basic_string<CharT> result;

        for (auto& ch : str)
        {
            static_cast<const D*>(this)->EncodeChar(ch, [&result](auto... chs) { AppendChar(result, chs...); });
        }

        return TargetString(std::move(result));
    }

    template<typename Str, typename CharT>
    static void AppendChar(Str& str, CharT ch)
    {
        str.push_back(static_cast<typename Str::value_type>(ch));
    }
    template<typename Str, typename CharT, typename... Args>
    static void AppendChar(Str& str, CharT ch, Args... chs)
    {
        str.push_back(static_cast<typename Str::value_type>(ch));
        AppendChar(str, chs...);
    }
};

template<typename Fn>
struct GenericStringEncoder : public StringEncoder<GenericStringEncoder<Fn>>
{
    GenericStringEncoder(Fn fn)
        : m_fn(std::move(fn)) {}

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
        enum EncodeStyle
        {
            None,
            Percent
        };

        EncodeStyle encStyle = None;
        switch (ch)
        {
        case ' ':
            fn('+');
            return;
        case '+':
        case '\"':
        case '%':
        case '-':
        case '!':
        case '#':
        case '$':
        case '&':
        case '\'':
        case '(':
        case ')':
        case '*':
        case ',':
        case '/':
        case ':':
        case ';':
        case '=':
        case '?':
        case '@':
        case '[':
        case ']':
            encStyle = Percent;
            break;
        default:
            if (AsUnsigned(ch) > 0x7f)
                encStyle = Percent;
            break;
        }

        if (encStyle == None)
        {
            fn(ch);
            return;
        }
        union
        {
            uint32_t intCh;
            uint8_t chars[4];
        };
        intCh = AsUnsigned(ch);
        if (intCh > 0xffffff)
            DoPercentEncoding(chars[3], fn);
        if (intCh > 0xffff)
            DoPercentEncoding(chars[2], fn);
        if (intCh > 0xff)
            DoPercentEncoding(chars[1], fn);
        DoPercentEncoding(chars[0], fn);
    }

    template<typename Fn>
    void DoPercentEncoding(uint8_t ch, Fn&& fn) const
    {
        char chars[] = "0123456789ABCDEF";
        int ch1 = static_cast<int>(chars[(ch & 0xf0) >> 4]);
        int ch2 = static_cast<int>(chars[ch & 0x0f]);
        fn('%', ch1, ch2);
    }

    template<typename Ch, size_t SZ>
    struct ToUnsigned;

    template<typename Ch>
    struct ToUnsigned<Ch, 1>
    {
        static auto Cast(Ch ch) { return static_cast<uint8_t>(ch); }
    };

    template<typename Ch>
    struct ToUnsigned<Ch, 2>
    {
        static auto Cast(Ch ch) { return static_cast<uint16_t>(ch); }
    };

    template<typename Ch>
    struct ToUnsigned<Ch, 4>
    {
        static auto Cast(Ch ch) { return static_cast<uint32_t>(ch); }
    };

    template<typename Ch>
    auto AsUnsigned(Ch ch) const
    {
        return static_cast<uint32_t>(ToUnsigned<Ch, sizeof(Ch)>::Cast(ch));
    }
};

// Port of Python's textwrap.wrap as Jinja2's wordwrap calls it (expand_tabs, replace_whitespace
// off; drop_whitespace on). Lengths are counted in code points. Each paragraph of the input is
// wrapped separately and all lines are joined with wrapString.
template<typename CharT>
std::basic_string<CharT> WordWrap(nonstd::basic_string_view<CharT> text, int64_t width, bool breakLongWords, const std::basic_string<CharT>& wrapString, bool breakOnHyphens)
{
    using View = nonstd::basic_string_view<CharT>;
    using Range = std::pair<size_t, size_t>;

    auto asciiOf = [](View ch) -> int {
        auto unit = static_cast<uint32_t>(ch[0]);
        return ch.size() == 1 && unit < 0x80 ? static_cast<int>(unit) : -1;
    };
    auto isSpace = [&asciiOf](View ch) {
        auto c = asciiOf(ch);
        return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
    };
    auto isHyphen = [&asciiOf](View ch) { return asciiOf(ch) == '-'; };

    std::vector<std::vector<View>> paragraphs;
    std::vector<View> current;
    auto chars = SplitCodePoints(text);
    for (size_t n = 0; n < chars.size(); ++n)
    {
        auto c = asciiOf(chars[n]);
        if (c == '\n' || c == '\r' || c == '\v' || c == '\f')
        {
            if (c == '\r' && n + 1 < chars.size() && asciiOf(chars[n + 1]) == '\n')
                ++n;
            paragraphs.push_back(std::move(current));
            current.clear();
        }
        else
            current.push_back(chars[n]);
    }
    if (!current.empty())
        paragraphs.push_back(std::move(current));

    std::vector<std::basic_string<CharT>> lines;
    for (auto& line : paragraphs)
    {
        // textwrap's letter is [^\d\W]; treat every non-ASCII code point as one
        auto isLetter = [&line, &asciiOf](size_t idx) {
            if (idx >= line.size())
                return false;
            auto c = asciiOf(line[idx]);
            return c < 0 || std::isalpha(c) || c == '_';
        };
        auto isHyphenAt = [&line, &isHyphen](size_t idx) { return idx < line.size() && isHyphen(line[idx]); };
        // A word is split after a hyphen between letters, like "long-word" -> "long-", "word"
        auto splitsAfter = [&](size_t h) {
            bool before = h >= 2 && isLetter(h - 1) && (isLetter(h - 2) || (h >= 3 && isHyphenAt(h - 2) && isLetter(h - 3)));
            bool after = isLetter(h + 1) && (isLetter(h + 2) || (isHyphenAt(h + 2) && isLetter(h + 3)));
            return before && after;
        };

        std::vector<Range> chunks;
        for (size_t pos = 0; pos < line.size();)
        {
            size_t end = pos + 1;
            if (isSpace(line[pos]))
            {
                while (end < line.size() && isSpace(line[end]))
                    ++end;
            }
            else
            {
                end = pos;
                while (end < line.size() && !isSpace(line[end]))
                {
                    if (breakOnHyphens && isHyphen(line[end]) && end > pos && splitsAfter(end))
                    {
                        ++end;
                        break;
                    }
                    ++end;
                }
            }
            chunks.emplace_back(pos, end);
            pos = end;
        }

        auto chunkLen = [](const Range& r) { return static_cast<int64_t>(r.second - r.first); };
        auto isSpaceChunk = [&](const Range& r) {
            for (auto n = r.first; n != r.second; ++n)
                if (!isSpace(line[n]))
                    return false;
            return true;
        };

        std::vector<std::basic_string<CharT>> wrapped;
        size_t next = 0;
        while (next < chunks.size())
        {
            std::vector<Range> curLine;
            int64_t curLen = 0;
            if (!wrapped.empty() && isSpaceChunk(chunks[next]))
                ++next;

            for (; next < chunks.size() && curLen + chunkLen(chunks[next]) <= width; ++next)
            {
                curLine.push_back(chunks[next]);
                curLen += chunkLen(chunks[next]);
            }

            if (next < chunks.size() && chunkLen(chunks[next]) > width)
            {
                auto spaceLeft = width < 1 ? 1 : width - curLen;
                auto& chunk = chunks[next];
                if (breakLongWords)
                {
                    auto end = static_cast<size_t>(spaceLeft);
                    if (breakOnHyphens && chunkLen(chunk) > spaceLeft)
                    {
                        for (size_t h = end; h-- > 1;)
                        {
                            if (!isHyphen(line[chunk.first + h]))
                                continue;
                            for (size_t n = 0; n != h; ++n)
                            {
                                if (!isHyphen(line[chunk.first + n]))
                                {
                                    end = h + 1;
                                    break;
                                }
                            }
                            break;
                        }
                    }
                    curLine.emplace_back(chunk.first, chunk.first + end);
                    chunk.first += end;
                }
                else if (curLine.empty())
                {
                    curLine.push_back(chunk);
                    ++next;
                }
            }

            if (!curLine.empty() && isSpaceChunk(curLine.back()))
                curLine.pop_back();
            if (curLine.empty())
                continue;

            std::basic_string<CharT> out;
            for (auto& r : curLine)
                for (auto n = r.first; n != r.second; ++n)
                    out.append(line[n].begin(), line[n].end());
            wrapped.push_back(std::move(out));
        }

        std::basic_string<CharT> paragraph;
        for (size_t n = 0; n != wrapped.size(); ++n)
            paragraph += (n == 0 ? std::basic_string<CharT>() : wrapString) + wrapped[n];
        lines.push_back(std::move(paragraph));
    }

    std::basic_string<CharT> result;
    for (size_t n = 0; n != lines.size(); ++n)
        result += (n == 0 ? std::basic_string<CharT>() : wrapString) + lines[n];
    return result;
}

StringConverter::StringConverter(FilterParams params, StringConverter::Mode mode)
    : m_mode(mode)
{
    switch (m_mode)
    {
    case ReplaceMode:
        ParseParams({ { "old", true }, { "new", true }, { "count", false, static_cast<int64_t>(0) } }, params);
        break;
    case TruncateMode:
        ParseParams({ { "length", false, static_cast<int64_t>(255) }, { "killwords", false, false }, { "end", false, std::string("...") }, { "leeway", false } }, params);
        break;
    case CenterMode:
        ParseParams({ { "width", false, static_cast<int64_t>(80) } }, params);
        break;
    case WordWrapMode:
        ParseParams({ { "width", false, static_cast<int64_t>(79) }, { "break_long_words", false, true }, { "wrapstring", false }, { "break_on_hyphens", false, true } }, params);
        break;
    default: break;
    }
}

InternalValue StringConverter::Filter(const InternalValue& baseVal, RenderContext& context)
{
    TargetString result;

    auto isAlpha = ba::is_alpha();
    auto isAlNum = ba::is_alnum();

    switch (m_mode)
    {
    case TrimMode:
        result = ApplyStringConverter(baseVal, [](auto strView) -> TargetString {
            auto str = sv_to_string(strView);
            ba::trim_all(str);
            return TargetString(str);
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
        ApplyStringConverter<GenericStringEncoder>(baseVal, [isDelim = true, &wc, &isAlNum](auto ch, auto&&) mutable {
            if (isDelim && isAlNum(ch))
            {
                isDelim = false;
                wc++;
                return;
            }
            isDelim = !isAlNum(ch);
        });
        return InternalValue(wc);
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
        result = ApplyStringConverter(baseVal, [this, &context](auto srcStr) -> TargetString {
            std::decay_t<decltype(srcStr)> emptyStrView;
            using CharT = typename decltype(emptyStrView)::value_type;
            std::basic_string<CharT> emptyStr;
            auto oldStr = GetAsSameString(srcStr, this->GetArgumentValue("old", context)).value_or(emptyStr);
            auto newStr = GetAsSameString(srcStr, this->GetArgumentValue("new", context)).value_or(emptyStr);
            auto count = ConvertToInt(this->GetArgumentValue("count", context));
            auto str = sv_to_string(srcStr);
            if (count == 0)
                ba::replace_all(str, oldStr, newStr);
            else
            {
                for (int64_t n = 0; n < count; ++n)
                    ba::replace_first(str, oldStr, newStr);
            }
            return str;
        });
        break;
    case TruncateMode:
        result = ApplyStringConverter(baseVal, [this, &context, &isAlNum](auto srcStr) -> TargetString {
            std::decay_t<decltype(srcStr)> emptyStrView;
            using CharT = typename decltype(emptyStrView)::value_type;
            std::basic_string<CharT> emptyStr;
            auto length = ConvertToInt(this->GetArgumentValue("length", context));
            auto killWords = ConvertToBool(this->GetArgumentValue("killwords", context));
            auto end = GetAsSameString(srcStr, this->GetArgumentValue("end", context));
            auto leeway = ConvertToInt(this->GetArgumentValue("leeway", context), 5);
            if (static_cast<long long int>(srcStr.size()) <= length)
                return sv_to_string(srcStr);

            auto str = sv_to_string(srcStr);

            if (killWords)
            {
                if (static_cast<long long int>(str.size()) > (length + leeway))
                {
                    str.erase(str.begin() + static_cast<std::ptrdiff_t>(length), str.end());
                    str += end.value_or(emptyStr);
                }
                return str;
            }

            auto p = str.begin() + static_cast<std::ptrdiff_t>(length);
            if (leeway != 0)
            {
                for (; leeway != 0 && p != str.end() && isAlNum(*p); --leeway, ++p)
                    ;
                if (p == str.end())
                    return TargetString(str);
            }

            if (isAlNum(*p))
            {
                for (; p != str.begin() && isAlNum(*p); --p)
                    ;
            }
            str.erase(p, str.end());
            ba::trim_right(str);
            str += end.value_or(emptyStr);

            return TargetString(std::move(str));
        });
        break;
    case UrlEncodeMode:
        result = Apply<UrlStringEncoder>(baseVal);
        break;
    case CapitalMode:
        result = ApplyStringConverter<GenericStringEncoder>(baseVal, [isFirstChar = true, &isAlpha](auto ch, auto&& fn) mutable {
            if (isAlpha(ch))
            {
                if (isFirstChar)
                    fn(std::toupper(ch, std::locale()));
                else
                    fn(std::tolower(ch, std::locale()));
            }
            else
                fn(ch);

            isFirstChar = false;
        });
        break;
    case EscapeHtmlMode:
        result = ApplyStringConverter<GenericStringEncoder>(baseVal, [](auto ch, auto&& fn) mutable {
            switch (ch)
            {
            case '<':
                fn('&', 'l', 't', ';');
                break;
            case '>':
                fn('&', 'g', 't', ';');
                break;
            case '&':
                fn('&', 'a', 'm', 'p', ';');
                break;
            case '\'':
                fn('&', '#', '3', '9', ';');
                break;
            case '\"':
                fn('&', '#', '3', '4', ';');
                break;
            default:
                fn(ch);
                break;
            }
        });
        break;
    case StriptagsMode:
        result = ApplyStringConverter(baseVal, [](auto srcStr) -> TargetString {
            auto str = sv_to_string(srcStr);
            using StringT = decltype(str);
            using CharT = typename StringT::value_type;
            static const std::basic_regex<CharT> STRIPTAGS_RE(UNIVERSAL_STR("(<!--.*?-->|<[^>]*>)").GetValueStr<CharT>());
            str = std::regex_replace(str, STRIPTAGS_RE, UNIVERSAL_STR("").GetValueStr<CharT>());
            ba::trim_all(str);
            static const StringT html_entities[]{
                UNIVERSAL_STR("&amp;").GetValueStr<CharT>(),
                UNIVERSAL_STR("&").GetValueStr<CharT>(),
                UNIVERSAL_STR("&apos;").GetValueStr<CharT>(),
                UNIVERSAL_STR("\'").GetValueStr<CharT>(),
                UNIVERSAL_STR("&gt;").GetValueStr<CharT>(),
                UNIVERSAL_STR(">").GetValueStr<CharT>(),
                UNIVERSAL_STR("&lt;").GetValueStr<CharT>(),
                UNIVERSAL_STR("<").GetValueStr<CharT>(),
                UNIVERSAL_STR("&quot;").GetValueStr<CharT>(),
                UNIVERSAL_STR("\"").GetValueStr<CharT>(),
                UNIVERSAL_STR("&#39;").GetValueStr<CharT>(),
                UNIVERSAL_STR("\'").GetValueStr<CharT>(),
                UNIVERSAL_STR("&#34;").GetValueStr<CharT>(),
                UNIVERSAL_STR("\"").GetValueStr<CharT>(),
            };
            for (auto it = std::begin(html_entities), end = std::end(html_entities); it < end; it += 2)
            {
                ba::replace_all(str, *it, *(it + 1));
            }
            return str;
        });
        break;
    case CenterMode:
        result = ApplyStringConverter(baseVal, [this, &context](auto srcStr) -> TargetString {
            auto width = ConvertToInt(this->GetArgumentValue("width", context));
            auto str = sv_to_string(srcStr);
            auto string_length = static_cast<long long int>(str.size());
            if (string_length >= width)
                return str;
            auto whitespaces = width - string_length;
            str.insert(0, static_cast<std::string::size_type>(whitespaces + 1) / 2, ' ');
            str.append(static_cast<std::string::size_type>(whitespaces / 2), ' ');
            return TargetString(std::move(str));
        });
        break;
    case WordWrapMode:
        result = ApplyStringConverter(baseVal, [this, &context](auto srcStr) -> TargetString {
            using CharT = typename decltype(srcStr)::value_type;
            auto width = ConvertToInt(this->GetArgumentValue("width", context));
            auto breakLongWords = ConvertToBool(this->GetArgumentValue("break_long_words", context));
            auto breakOnHyphens = ConvertToBool(this->GetArgumentValue("break_on_hyphens", context));
            auto wrapString = GetAsSameString(srcStr, this->GetArgumentValue("wrapstring", context)).value_or(std::basic_string<CharT>(1, '\n'));
            // Python raises "invalid width" here
            if (width <= 0)
                return sv_to_string(srcStr);
            return WordWrap(srcStr, width, breakLongWords, wrapString, breakOnHyphens);
        });
        break;
    default:
        break;
    }

    return std::move(result);
}

} // namespace filters
} // namespace jinja2
