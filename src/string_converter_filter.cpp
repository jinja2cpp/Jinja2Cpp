#include "filters.h"
#include "markup.h"
#include "testers.h"
#include "value_visitors.h"
#include "value_helpers.h"
#include "unicode_tables.h"

#include <cctype>
#include <cmath>
#include <cstring>
#include <algorithm>
#include <numeric>
#include <regex>
#include <sstream>

#include <boost/algorithm/string/trim_all.hpp>
#include <boost/algorithm/string/replace.hpp>

namespace ba = boost::algorithm;

namespace jinja2::filters
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
    TargetString operator()(const std::basic_string_view<CharT>& str) const
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
    explicit GenericStringEncoder(Fn fn)
        : m_fn(std::move(fn)) {}

    template<typename CharT, typename AppendFn>
    void EncodeChar(CharT ch, AppendFn&& fn) const
    {
        m_fn(ch, std::forward<AppendFn>(fn));
    }

    mutable Fn m_fn;
};

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

// Port of Python's textwrap.wrap as Jinja2's wordwrap calls it (expand_tabs, replace_whitespace
// off; drop_whitespace on). Lengths are counted in code points. Each paragraph of the input is
// wrapped separately and all lines are joined with wrapString.
template<typename CharT>
std::basic_string<CharT> WordWrap(std::basic_string_view<CharT> text, int64_t width, bool breakLongWords, const std::basic_string<CharT>& wrapString, bool breakOnHyphens)
{
    using View = std::basic_string_view<CharT>;
    using Range = std::pair<size_t, size_t>;

    auto asciiOf = [](View ch) -> int {
        auto unit = CodeUnit(ch[0]);
        return ch.size() == 1 && unit < 0x80 ? static_cast<int>(unit) : -1;
    };
    // textwrap chunks on ASCII whitespace only (its _whitespace), not on str.isspace()
    auto isSpace = [&asciiOf](View ch) {
        auto c = asciiOf(ch);
        return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
    };
    // The line boundaries of str.splitlines()
    auto isLineBreak = [](View ch) {
        auto c = CodePointValue(ch);
        return (c >= 0x0A && c <= 0x0D) || (c >= 0x1C && c <= 0x1E) || c == 0x85 || c == 0x2028 || c == 0x2029;
    };
    auto isHyphen = [&asciiOf](View ch) { return asciiOf(ch) == '-'; };

    std::vector<std::vector<View>> paragraphs;
    std::vector<View> current;
    auto chars = SplitCodePoints(text);
    for (size_t n = 0; n < chars.size(); ++n)
    {
        if (isLineBreak(chars[n]))
        {
            if (asciiOf(chars[n]) == '\r' && n + 1 < chars.size() && asciiOf(chars[n + 1]) == '\n')
            {
                ++n;
            }
            paragraphs.push_back(std::move(current));
            current.clear();
        }
        else
        {
            current.push_back(chars[n]);
        }
    }
    if (!current.empty())
    {
        paragraphs.push_back(std::move(current));
    }

    std::vector<std::basic_string<CharT>> lines;
    for (auto& line : paragraphs)
    {
        // textwrap's letter is [^\d\W]: a word character that is not a decimal digit
        auto isLetter = [&line](size_t idx) {
            if (idx >= line.size())
            {
                return false;
            }
            auto cp = CodePointValue(line[idx]);
            return unicode::IsWordChar(cp) && !unicode::IsDecimal(cp);
        };
        auto isHyphenAt = [&line, &isHyphen](size_t idx) { return idx < line.size() && isHyphen(line[idx]); };
        // \w, and textwrap's word punctuation [\w!"'&.,?]
        auto isWordChar = [&line](size_t idx) { return idx < line.size() && unicode::IsWordChar(CodePointValue(line[idx])); };
        auto isWordPunct = [&](size_t idx) {
            auto c = idx < line.size() ? asciiOf(line[idx]) : -1;
            return isWordChar(idx) || (c >= 0 && std::strchr("!\"'&.,?", c) != nullptr && c != 0);
        };
        // Length of an em-dash ("--" or longer, followed by a word character) starting at idx
        auto emDashAt = [&](size_t idx) -> size_t {
            auto end = idx;
            while (isHyphenAt(end))
            {
                ++end;
            }
            return end - idx >= 2 && isWordChar(end) ? end - idx : 0;
        };
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
                {
                    ++end;
                }
            }
            else if (breakOnHyphens && pos > 0 && isWordPunct(pos - 1) && emDashAt(pos) != 0)
            {
                // An em-dash between words is a chunk of its own: "hello--world" -> "hello", "--", "world"
                end = pos + emDashAt(pos);
            }
            else
            {
                end = pos;
                while (end < line.size() && !isSpace(line[end]))
                {
                    if (breakOnHyphens && end > pos && isWordPunct(end - 1) && emDashAt(end) != 0)
                    {
                        break;
                    }
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
        // Chunks split on ASCII whitespace, but drop_whitespace tests chunk.strip() == '',
        // which is Unicode-aware: a chunk of NBSPs is dropped too
        auto isSpaceChunk = [&](const Range& r) {
            for (auto n = r.first; n != r.second; ++n)
            {
                if (!unicode::IsSpace(CodePointValue(line[n])))
                {
                    return false;
                }
            }
            return true;
        };

        std::vector<std::basic_string<CharT>> wrapped;
        size_t next = 0;
        while (next < chunks.size())
        {
            std::vector<Range> curLine;
            int64_t curLen = 0;
            if (!wrapped.empty() && isSpaceChunk(chunks[next]))
            {
                ++next;
            }

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
                            {
                                continue;
                            }
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
            {
                curLine.pop_back();
            }
            if (curLine.empty())
            {
                continue;
            }

            std::basic_string<CharT> out;
            for (auto& r : curLine)
            {
                for (auto n = r.first; n != r.second; ++n)
                {
                    out.append(line[n].begin(), line[n].end());
                }
            }
            wrapped.push_back(std::move(out));
        }

        std::basic_string<CharT> paragraph;
        for (size_t n = 0; n != wrapped.size(); ++n)
        {
            paragraph += (n == 0 ? std::basic_string<CharT>() : wrapString) + wrapped[n];
        }
        lines.push_back(std::move(paragraph));
    }

    std::basic_string<CharT> result;
    for (size_t n = 0; n != lines.size(); ++n)
    {
        result += (n == 0 ? std::basic_string<CharT>() : wrapString) + lines[n];
    }
    return result;
}

template<typename CharT>
std::basic_string<CharT> AsciiString(const char* str)
{
    return std::basic_string<CharT>(str, str + std::strlen(str));
}

// The line boundaries of str.splitlines()
inline bool IsLineBreak(uint32_t cp)
{
    return (cp >= 0x0A && cp <= 0x0D) || (cp >= 0x1C && cp <= 0x1E) || cp == 0x85 || cp == 0x2028 || cp == 0x2029;
}

// Port of Jinja2's do_indent
template<typename CharT>
std::basic_string<CharT> Indent(std::basic_string_view<CharT> text, const std::basic_string<CharT>& indention, bool first, bool blank)
{
    // Jinja2 appends a newline before splitting, so a trailing newline is dropped
    std::basic_string<CharT> str(text.begin(), text.end());
    str.push_back('\n');
    std::vector<std::basic_string<CharT>> lines;
    std::basic_string<CharT> current;
    auto chars = SplitCodePoints(std::basic_string_view<CharT>(str));
    for (size_t n = 0; n < chars.size(); ++n)
    {
        auto cp = CodePointValue(chars[n]);
        if (!IsLineBreak(cp))
        {
            current.append(chars[n].begin(), chars[n].end());
            continue;
        }
        if (cp == '\r' && n + 1 < chars.size() && CodePointValue(chars[n + 1]) == '\n')
        {
            ++n;
        }
        lines.push_back(std::move(current));
        current.clear();
    }

    std::basic_string<CharT> result;
    for (size_t n = 0; n != lines.size(); ++n)
    {
        if (n != 0)
        {
            result.push_back('\n');
        }
        if (n != 0 && (blank || !lines[n].empty()))
        {
            result += indention;
        }
        result += lines[n];
    }
    return first ? indention + result : result;
}

// Port of Jinja2's urlize (jinja2/utils.py). Python's regular expressions are matched by hand,
// so that a long word cannot exhaust the stack of std::regex.
template<typename CharT>
class Urlizer
{
public:
    using String = std::basic_string<CharT>;
    using View = std::basic_string_view<CharT>;

    Urlizer(std::optional<int64_t> trimUrlLimit, const String& rel, const String& target, std::vector<String> extraSchemes)
        : m_trimUrlLimit(trimUrlLimit)
        , m_extraSchemes(std::move(extraSchemes))
    {
        if (!rel.empty())
        {
            m_relAttr = Ascii(" rel=\"") + EscapeHtml(View(rel)) + Ascii("\"");
        }
        if (!target.empty())
        {
            m_targetAttr = Ascii(" target=\"") + EscapeHtml(View(target)) + Ascii("\"");
        }
    }

    String operator()(View text) const
    {
        auto escaped = EscapeHtml(text);
        auto chars = SplitCodePoints(View(escaped));
        String result;
        for (size_t pos = 0; pos < chars.size();)
        {
            bool isSpace = unicode::IsSpace(CodePointValue(chars[pos]));
            String word;
            for (; pos < chars.size() && unicode::IsSpace(CodePointValue(chars[pos])) == isSpace; ++pos)
            {
                word.append(chars[pos].begin(), chars[pos].end());
            }
            // Whitespace runs come out unchanged
            result += isSpace ? word : ProcessWord(std::move(word));
        }
        return result;
    }

    // filters._uri_scheme_re: ^([\w.+-]{2,}:(/){0,2})$
    static bool IsValidScheme(View scheme)
    {
        auto chars = SplitCodePoints(scheme);
        size_t colon = 0;
        for (; colon < chars.size(); ++colon)
        {
            auto cp = CodePointValue(chars[colon]);
            if (cp == ':')
            {
                break;
            }
            if (!unicode::IsWordChar(cp) && cp != '.' && cp != '+' && cp != '-')
            {
                return false;
            }
        }
        if (colon < 2 || colon == chars.size() || chars.size() - colon > 3)
        {
            return false;
        }
        for (size_t n = colon + 1; n < chars.size(); ++n)
        {
            if (CodePointValue(chars[n]) != '/')
            {
                return false;
            }
        }
        return true;
    }

private:
    using Chars = std::vector<View>;

    static String Ascii(const char* str) { return AsciiString<CharT>(str); }
    static bool StartsWith(const String& str, const char* prefix)
    {
        auto p = Ascii(prefix);
        return str.compare(0, p.size(), p) == 0;
    }
    static bool EndsWith(const String& str, const char* suffix)
    {
        auto s = Ascii(suffix);
        return str.size() >= s.size() && str.compare(str.size() - s.size(), s.size(), s) == 0;
    }
    static size_t Count(const String& str, const String& sub)
    {
        size_t result = 0;
        for (auto pos = str.find(sub); pos != String::npos; pos = str.find(sub, pos + sub.size()))
        {
            ++result;
        }
        return result;
    }

    static int AsciiLower(View ch)
    {
        auto cp = CodePointValue(ch);
        if (cp >= 0x80)
        {
            return -1;
        }
        return cp >= 'A' && cp <= 'Z' ? static_cast<int>(cp - 'A' + 'a') : static_cast<int>(cp);
    }
    static bool StartsWithNoCase(const Chars& chars, size_t pos, const char* prefix)
    {
        for (; *prefix != 0; ++prefix, ++pos)
        {
            if (pos >= chars.size() || AsciiLower(chars[pos]) != *prefix)
            {
                return false;
            }
        }
        return true;
    }
    template<typename Pred>
    static bool All(const Chars& chars, size_t from, size_t to, const Pred& pred)
    {
        for (; from != to; ++from)
        {
            if (!pred(CodePointValue(chars[from])))
            {
                return false;
            }
        }
        return true;
    }
    static bool IsLabelChar(uint32_t cp) { return unicode::IsWordChar(cp) || cp == '%' || cp == '-'; }
    // Splits [from, to) on '.'
    static std::vector<std::pair<size_t, size_t>> SplitLabels(const Chars& chars, size_t from, size_t to)
    {
        std::vector<std::pair<size_t, size_t>> labels;
        size_t start = from;
        for (size_t n = from; n != to; ++n)
        {
            if (CodePointValue(chars[n]) == '.')
            {
                labels.emplace_back(start, n);
                start = n + 1;
            }
        }
        labels.emplace_back(start, to);
        return labels;
    }

    // utils._http_re
    static bool IsHttpUrl(const String& str)
    {
        auto chars = SplitCodePoints(View(str));
        size_t scheme = 0;
        if (StartsWithNoCase(chars, 0, "https://"))
        {
            scheme = 8;
        }
        else if (StartsWithNoCase(chars, 0, "http://"))
        {
            scheme = 7;
        }
        // The path, query and fragment ([/?#]\S*) is whatever follows the host and port
        size_t end = scheme;
        for (; end < chars.size(); ++end)
        {
            auto ch = AsciiLower(chars[end]);
            if (ch == '/' || ch == '?' || ch == '#')
            {
                break;
            }
        }
        // The port (:[\d]{1,5}); no host form ends in a colon and digits
        for (size_t colon = end; colon-- > scheme;)
        {
            if (CodePointValue(chars[colon]) != ':')
            {
                continue;
            }
            if (end - colon >= 2 && end - colon <= 6 && All(chars, colon + 1, end, unicode::IsDecimal))
            {
                end = colon;
            }
            break;
        }

        // (https?://|www\.) (([\w%-]+\.)+)? ([a-z]{2,63} | xn--[\w%]{2,59})
        size_t prefix = scheme;
        if (prefix == 0 && StartsWithNoCase(chars, 0, "www."))
        {
            prefix = 4;
        }
        if (prefix != 0)
        {
            auto labels = SplitLabels(chars, prefix, end);
            auto& tld = labels.back();
            auto tldLen = tld.second - tld.first;
            bool basicTld = tldLen >= 2 && tldLen <= 63 && All(chars, tld.first, tld.second, [](uint32_t cp) { return cp < 0x80 && std::isalpha(static_cast<int>(cp)); });
            bool idnaTld = StartsWithNoCase(chars, tld.first, "xn--") && tldLen >= 6 && tldLen <= 63 && All(chars, tld.first + 4, tld.second, [](uint32_t cp) { return unicode::IsWordChar(cp) || cp == '%'; });
            bool tldOk = basicTld || idnaTld;
            bool labelsOk = std::all_of(labels.begin(), labels.end() - 1, [&chars](auto& l) { return l.second != l.first && All(chars, l.first, l.second, IsLabelChar); });
            if (tldOk && labelsOk)
            {
                return true;
            }
        }

        // ([\w%-]{2,63}\.)+ (com|net|int|edu|gov|org|info|mil)
        {
            auto labels = SplitLabels(chars, 0, end);
            auto& tld = labels.back();
            static const char* const tlds[] = { "com", "net", "int", "edu", "gov", "org", "info", "mil" };
            bool tldOk = std::any_of(std::begin(tlds), std::end(tlds), [&](const char* t) { return tld.second - tld.first == std::strlen(t) && StartsWithNoCase(chars, tld.first, t); });
            bool labelsOk = std::all_of(labels.begin(), labels.end() - 1, [&chars](auto& l) {
                auto len = l.second - l.first;
                return len >= 2 && len <= 63 && All(chars, l.first, l.second, IsLabelChar);
            });
            if (labels.size() >= 2 && tldOk && labelsOk)
            {
                return true;
            }
        }

        if (scheme == 0)
        {
            return false;
        }

        // (https?://) ((\d{1,3})(\.\d{1,3}){3})
        auto labels = SplitLabels(chars, scheme, end);
        if (labels.size() == 4 && std::all_of(labels.begin(), labels.end(), [&chars](auto& l) {
                auto len = l.second - l.first;
                return len >= 1 && len <= 3 && All(chars, l.first, l.second, unicode::IsDecimal);
            }))
        {
            return true;
        }

        // (https?://) (\[([\da-f]{0,4}:){2}([\da-f]{0,4}:?){1,6}])
        if (end - scheme < 2 || end - scheme > 42 || CodePointValue(chars[scheme]) != '[' || CodePointValue(chars[end - 1]) != ']')
        {
            return false;
        }
        std::string inner;
        for (size_t n = scheme + 1; n != end - 1; ++n)
        {
            auto cp = CodePointValue(chars[n]);
            if (cp >= 0x80)
            {
                return false;
            }
            inner.push_back(static_cast<char>(cp));
        }
        static const std::regex ipv6("([0-9a-fA-F]{0,4}:){2}([0-9a-fA-F]{0,4}:?){1,6}");
        return std::regex_match(inner, ipv6);
    }

    // utils._email_re: ^\S+@\w[\w.-]*\.\w+$
    static bool IsEmail(const String& str, size_t from)
    {
        auto chars = SplitCodePoints(View(str).substr(from));
        // The last '@', or npos
        size_t at = chars.size() - 1;
        while (at != static_cast<size_t>(-1) && CodePointValue(chars[at]) != '@')
        {
            --at;
        }
        if (at == static_cast<size_t>(-1) || at == 0 || at + 1 == chars.size() || !unicode::IsWordChar(CodePointValue(chars[at + 1])))
        {
            return false;
        }
        // The last '.' after the character that follows '@', or at + 1
        size_t dot = chars.size() - 1;
        while (dot > at + 1 && CodePointValue(chars[dot]) != '.')
        {
            --dot;
        }
        if (dot == at + 1 || dot + 1 == chars.size() || !All(chars, dot + 1, chars.size(), unicode::IsWordChar))
        {
            return false;
        }
        return All(chars, at + 1, dot, [](uint32_t cp) { return unicode::IsWordChar(cp) || cp == '.' || cp == '-'; });
    }

    [[nodiscard]] String TrimUrl(const String& url) const
    {
        if (!m_trimUrlLimit)
        {
            return url;
        }
        auto chars = SplitCodePoints(View(url));
        auto size = static_cast<int64_t>(chars.size());
        auto limit = *m_trimUrlLimit;
        if (size <= limit)
        {
            return url;
        }
        // x[:limit] with Python's slice semantics for a negative limit
        auto keep = static_cast<size_t>(limit >= 0 ? limit : std::max<int64_t>(0, size + limit));
        String result;
        for (size_t n = 0; n != keep; ++n)
        {
            result.append(chars[n].begin(), chars[n].end());
        }
        return result + Ascii("...");
    }

    [[nodiscard]] String ProcessWord(String middle) const
    {
        String head;
        String tail;
        for (;;)
        {
            const char* lead = nullptr;
            for (const auto* l : { "(", "<", "&lt;" })
            {
                if (StartsWith(middle, l))
                {
                    lead = l;
                }
            }
            if (!lead)
            {
                break;
            }
            auto len = std::strlen(lead);
            head += middle.substr(0, len);
            middle.erase(0, len);
        }
        for (;;)
        {
            const char* trail = nullptr;
            for (const auto* t : { ")", ">", ".", ",", "\n", "&gt;" })
            {
                if (EndsWith(middle, t))
                {
                    trail = t;
                }
            }
            if (!trail)
            {
                break;
            }
            auto len = std::strlen(trail);
            tail.insert(0, middle.substr(middle.size() - len));
            middle.erase(middle.size() - len);
        }

        // Prefer balancing parentheses in URLs instead of ignoring a trailing character
        static const char* const pairs[][2] = { { "(", ")" }, { "<", ">" }, { "&lt;", "&gt;" } };
        for (const auto& pair : pairs)
        {
            auto startChar = Ascii(pair[0]);
            auto endChar = Ascii(pair[1]);
            auto startCount = Count(middle, startChar);
            if (startCount <= Count(middle, endChar))
            {
                continue;
            }
            for (auto n = std::min(startCount, Count(tail, endChar)); n != 0; --n)
            {
                auto endIndex = tail.find(endChar) + endChar.size();
                middle += tail.substr(0, endIndex);
                tail.erase(0, endIndex);
            }
        }

        if (IsHttpUrl(middle))
        {
            auto href = StartsWith(middle, "https://") || StartsWith(middle, "http://") ? middle : Ascii("https://") + middle;
            middle = Ascii("<a href=\"") + href + Ascii("\"") + m_relAttr + m_targetAttr + Ascii(">") + TrimUrl(middle) + Ascii("</a>");
        }
        else if (StartsWith(middle, "mailto:") && IsEmail(middle, 7))
        {
            middle = Ascii("<a href=\"") + middle + Ascii("\">") + middle.substr(7) + Ascii("</a>");
        }
        else if (middle.find('@') != String::npos && !StartsWith(middle, "www.") && !StartsWith(middle, "@") && middle.find(':') == String::npos && IsEmail(middle, 0))
        {
            middle = Ascii("<a href=\"mailto:") + middle + Ascii("\">") + middle + Ascii("</a>");
        }
        else
        {
            for (auto& scheme : m_extraSchemes)
            {
                if (middle != scheme && middle.compare(0, scheme.size(), scheme) == 0)
                {
                    middle = Ascii("<a href=\"") + middle + Ascii("\"") + m_relAttr + m_targetAttr + Ascii(">") + middle + Ascii("</a>");
                }
            }
        }

        return head + middle + tail;
    }

    std::optional<int64_t> m_trimUrlLimit;
    std::vector<String> m_extraSchemes;
    String m_relAttr;
    String m_targetAttr;
};

// One character (code point) as an upper- or lowercase character. Only single-unit
// characters change: ASCII in UTF-8 strings, the BMP in wide strings.
template<typename CharT>
void AppendWithCase(std::basic_string<CharT>& out, std::basic_string_view<CharT> ch, bool upper)
{
    if (ch.size() == 1 && (sizeof(CharT) != 1 || static_cast<unsigned char>(ch[0]) < 0x80))
    {
        out.push_back(upper ? std::toupper(ch[0], std::locale()) : std::tolower(ch[0], std::locale()));
    }
    else
    {
        out.append(ch.begin(), ch.end());
    }
}

// Port of Jinja2's do_title: each word (split on runs of -, whitespace, (, {, [ and <) gets
// an uppercase first character and lowercase others
template<typename CharT>
std::basic_string<CharT> TitleCase(std::basic_string_view<CharT> str)
{
    std::basic_string<CharT> result;
    bool wordStart = true;
    for (auto ch : SplitCodePoints(str))
    {
        auto cp = CodePointValue(ch);
        bool isDelim = cp == '-' || cp == '(' || cp == '{' || cp == '[' || cp == '<' || unicode::IsSpace(cp);
        AppendWithCase(result, ch, wordStart);
        wordStart = isDelim;
    }
    return result;
}

// Python's str.strip(chars): without chars, Unicode whitespace
template<typename CharT>
std::basic_string<CharT> PythonStrip(std::basic_string_view<CharT> str, const std::optional<std::basic_string<CharT>>& chars)
{
    std::vector<std::basic_string_view<CharT>> stripSet;
    if (chars)
    {
        stripSet = SplitCodePoints(std::basic_string_view<CharT>(*chars));
    }
    auto isStripped = [&](std::basic_string_view<CharT> ch) {
        if (!chars)
        {
            return unicode::IsSpace(CodePointValue(ch));
        }
        return std::find(stripSet.begin(), stripSet.end(), ch) != stripSet.end();
    };
    auto parts = SplitCodePoints(str);
    size_t first = 0;
    size_t last = parts.size();
    while (first != last && isStripped(parts[first]))
    {
        ++first;
    }
    while (last != first && isStripped(parts[last - 1]))
    {
        --last;
    }
    std::basic_string<CharT> result;
    for (auto n = first; n != last; ++n)
    {
        result.append(parts[n].begin(), parts[n].end());
    }
    return result;
}

// html.unescape for the character references markupsafe's striptags leaves: numeric ones
// and the common named ones (the full HTML5 table is task 0048)
template<typename CharT>
std::basic_string<CharT> HtmlUnescape(const std::basic_string<CharT>& str)
{
    static const std::pair<const char*, uint32_t> named[] = {
        { "amp", '&' },
        { "lt", '<' },
        { "gt", '>' },
        { "quot", '"' },
        { "apos", '\'' },
        { "nbsp", 0xA0 },
        { "copy", 0xA9 },
        { "reg", 0xAE },
        { "raquo", 0xBB },
        { "laquo", 0xAB },
        { "hellip", 0x2026 },
        { "mdash", 0x2014 },
        { "ndash", 0x2013 },
        { "euro", 0x20AC },
        { "trade", 0x2122 },
        { "middot", 0xB7 },
        { "times", 0xD7 },
        { "deg", 0xB0 },
    };
    // html._invalid_charrefs: C1 controls are read as Windows-1252
    static const uint16_t cp1252[32] = { 0x20AC, 0x81, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021, 0x02C6, 0x2030, 0x0160,
                                         0x2039, 0x0152, 0x8D, 0x017D, 0x8F, 0x90, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022,
                                         0x2013, 0x2014, 0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0x9D, 0x017E, 0x0178 };
    auto appendCodePoint = [](std::basic_string<CharT>& out, uint32_t cp) {
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
    };

    std::basic_string<CharT> result;
    for (size_t pos = 0; pos < str.size();)
    {
        if (str[pos] != '&')
        {
            result.push_back(str[pos++]);
            continue;
        }
        auto next = pos + 1;
        if (next < str.size() && str[next] == '#')
        {
            ++next;
            bool hex = next < str.size() && (str[next] == 'x' || str[next] == 'X');
            if (hex)
            {
                ++next;
            }
            auto digitsStart = next;
            uint64_t value = 0;
            for (; next < str.size(); ++next)
            {
                auto ch = CodeUnit(str[next]);
                int digit = -1;
                if (ch >= '0' && ch <= '9')
                {
                    digit = static_cast<int>(ch - '0');
                }
                else if (hex && ch >= 'a' && ch <= 'f')
                {
                    digit = static_cast<int>(ch - 'a' + 10);
                }
                else if (hex && ch >= 'A' && ch <= 'F')
                {
                    digit = static_cast<int>(ch - 'A' + 10);
                }
                if (digit < 0)
                {
                    break;
                }
                value = std::min<uint64_t>((value * (hex ? 16 : 10)) + static_cast<uint64_t>(digit), 0x110000);
            }
            if (next != digitsStart)
            {
                if (next < str.size() && str[next] == ';')
                {
                    ++next;
                }
                uint32_t cp = static_cast<uint32_t>(value);
                if (cp >= 0x80 && cp <= 0x9F)
                {
                    cp = cp1252[cp - 0x80];
                }
                else if (cp == 0 || cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF))
                {
                    cp = 0xFFFD;
                }
                appendCodePoint(result, cp);
                pos = next;
                continue;
            }
        }
        else
        {
            auto semicolon = str.find(';', next);
            bool found = false;
            if (semicolon != std::basic_string<CharT>::npos && semicolon - next <= 32)
            {
                std::string name;
                for (auto n = next; n != semicolon; ++n)
                {
                    name.push_back(static_cast<unsigned>(str[n]) < 0x80 ? static_cast<char>(str[n]) : '?');
                }
                for (const auto& entity : named)
                {
                    if (name == entity.first)
                    {
                        appendCodePoint(result, entity.second);
                        pos = semicolon + 1;
                        found = true;
                        break;
                    }
                }
            }
            if (found)
            {
                continue;
            }
        }
        result.push_back(str[pos++]);
    }
    return result;
}

// Port of markupsafe's Markup.striptags: drop comments, then tags, collapse whitespace, unescape
template<typename CharT>
std::basic_string<CharT> StripTags(std::basic_string_view<CharT> text)
{
    using String = std::basic_string<CharT>;
    String value(text.begin(), text.end());
    auto removeBlocks = [&value](const String& open, const String& close) {
        for (auto start = value.find(open); start != String::npos; start = value.find(open))
        {
            auto end = value.find(close, start);
            if (end == String::npos)
            {
                break;
            }
            value.erase(start, end + close.size() - start);
        }
    };
    removeBlocks(AsciiString<CharT>("<!--"), AsciiString<CharT>("-->"));
    removeBlocks(AsciiString<CharT>("<"), AsciiString<CharT>(">"));

    // " ".join(value.split())
    String collapsed;
    bool pendingSpace = false;
    for (auto ch : SplitCodePoints(std::basic_string_view<CharT>(value)))
    {
        if (unicode::IsSpace(CodePointValue(ch)))
        {
            pendingSpace = !collapsed.empty();
            continue;
        }
        if (pendingSpace)
        {
            collapsed.push_back(' ');
        }
        pendingSpace = false;
        collapsed.append(ch.begin(), ch.end());
    }
    return HtmlUnescape(collapsed);
}

// Jinja2's url_quote: urllib's quote of the UTF-8 bytes, keeping "/" unless quoting for a
// query string, where a space becomes "+"
inline std::string UrlQuote(const std::string& str, bool forQuery)
{
    static const char hexDigits[] = "0123456789ABCDEF";
    std::string result;
    for (auto ch : str)
    {
        auto byte = static_cast<unsigned char>(ch);
        if ((std::isalnum(byte) && byte < 0x80) || ch == '_' || ch == '.' || ch == '-' || ch == '~' || (ch == '/' && !forQuery))
        {
            result.push_back(ch);
        }
        else if (ch == ' ' && forQuery)
        {
            result.push_back('+');
        }
        else
        {
            result.push_back('%');
            result.push_back(hexDigits[byte >> 4]);
            result.push_back(hexDigits[byte & 0x0F]);
        }
    }
    return result;
}

// What an argument Jinja2 hands to Python code must be: any number, a number with an
// integral value (textwrap slices with it) or an int (str.center)
enum class NumberKind
{
    Any,
    Whole,
    Int
};

namespace
{

// A string or a list fails in that Python code with a TypeError
int64_t NumericArgument(const InternalValue& val, const char* filter, const char* arg, NumberKind kind)
{
    const auto* asDouble = GetIf<double>(&val);
    bool isAcceptedDouble = asDouble != nullptr && (kind == NumberKind::Any || (kind == NumberKind::Whole && std::floor(*asDouble) == *asDouble));
    bool isNumber = GetIf<int64_t>(&val) != nullptr || GetIf<bool>(&val) != nullptr || isAcceptedDouble;
    if (!isNumber)
    {
        throw std::runtime_error(std::string(filter) + "(): '" + arg + "' must be " + (kind == NumberKind::Any ? "a number" : "an integer"));
    }
    return ConvertToInt(val);
}

// Python's len(): code points, not UTF-8 bytes
template<typename CharT>
int64_t CodePointCount(const std::basic_string<CharT>& str)
{
    if (sizeof(CharT) != 1)
    {
        return static_cast<int64_t>(str.size());
    }
    return std::count_if(str.begin(), str.end(), [](CharT ch) { return (static_cast<unsigned char>(ch) & 0xC0) != 0x80; });
}

} // namespace

StringConverter::StringConverter(const FilterParams& params, StringConverter::Mode mode)
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
    case IndentMode:
        ParseParams({ { "width", false, static_cast<int64_t>(4) }, { "first", false, false }, { "blank", false, false } }, params);
        break;
    case UrlizeMode:
        ParseParams({ { "trim_url_limit", false }, { "nofollow", false, false }, { "target", false }, { "rel", false }, { "extra_schemes", false } }, params);
        break;
    case TrimMode:
        ParseParams({ { "chars", false } }, params);
        break;
    default:
        ParseParams({}, params);
        break;
    }
}

InternalValue StringConverter::Filter(const InternalValue& baseVal, RenderContext& context)
{
    TargetString result;

    switch (m_mode)
    {
    case SafeMode:
    {
        if (!IsStringValue(baseVal))
        {
            return MakeMarkup(baseVal, context.GetRendererCallback());
        }
        InternalValue result = baseVal;
        result.SetMarkup();
        return result;
    }
    case ToStringMode:
        if (baseVal.IsMarkup())
        {
            return baseVal;
        }
        return context.GetRendererCallback()->GetAsTargetString(baseVal);
    case EscapeHtmlMode:
        return MarkupEscape(baseVal, context.GetRendererCallback());
    case ForceEscapeMode:
    {
        InternalValue val = baseVal;
        val.SetMarkup(false);
        return MarkupEscape(val, context.GetRendererCallback());
    }
    case UrlEncodeMode:
    {
        // A mapping or a sequence of pairs is a query string, anything else is quoted as str()
        auto* callback = context.GetRendererCallback();
        auto asText = [callback](const InternalValue& val) { return IsStringValue(val) ? AsString(val) : AsString(InternalValue(callback->GetAsTargetString(val))); };
        if (IsStringValue(baseVal) || (!GetIf<MapAdapter>(&baseVal) && !GetIf<ListAdapter>(&baseVal)))
        {
            return InternalValue(UrlQuote(asText(baseVal), false));
        }
        std::string query;
        auto appendPair = [&](const InternalValue& key, const InternalValue& value) {
            query += (query.empty() ? "" : "&") + UrlQuote(asText(key), true) + "=" + UrlQuote(asText(value), true);
        };
        if (const auto* map = GetIf<MapAdapter>(&baseVal))
        {
            for (auto& key : map->GetKeys())
            {
                appendPair(InternalValue(key), map->GetValueByName(key));
            }
        }
        else
        {
            for (const auto& item : *GetIf<ListAdapter>(&baseVal))
            {
                bool isList = false;
                auto pair = ConvertToList(item, isList);
                if (!isList || pair.GetSize().value_or(0) != 2)
                {
                    context.GetRendererCallback()->ThrowRuntimeError(ErrorCode::InvalidValueType, ValuesList{});
                }
                appendPair(pair.GetValueByIndex(0), pair.GetValueByIndex(1));
            }
        }
        return InternalValue(query);
    }
    case IndentMode:
    case UrlizeMode:
    case LowerMode:
    case UpperMode:
    case CapitalMode:
    case TitleMode:
    case CenterMode:
    case TrimMode:
    case ReplaceMode:
    case WordCountMode:
    case StriptagsMode:
        // These filters convert any value with str() first
        if (!IsStringValue(baseVal))
        {
            return Filter(InternalValue(context.GetRendererCallback()->GetAsTargetString(baseVal)), context);
        }
        break;
    default:
        break;
    }

    auto isAlpha = ba::is_alpha();
    auto isAlNum = ba::is_alnum();

    switch (m_mode)
    {
    case TrimMode:
        result = ApplyStringConverter(baseVal, [this, &context](auto srcStr) -> TargetString {
            auto chars = GetAsSameString(srcStr, this->GetArgumentValue("chars", context));
            return PythonStrip(srcStr, chars);
        });
        break;
    case TitleMode:
        result = ApplyStringConverter(baseVal, [](auto srcStr) -> TargetString { return TitleCase(srcStr); });
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
            {
                fn(std::toupper(ch, std::locale()));
            }
            else
            {
                fn(ch);
            }
        });
        break;
    case LowerMode:
        result = ApplyStringConverter<GenericStringEncoder>(baseVal, [&isAlpha](auto ch, auto&& fn) mutable {
            if (isAlpha(ch))
            {
                fn(std::tolower(ch, std::locale()));
            }
            else
            {
                fn(ch);
            }
        });
        break;
    case ReplaceMode:
    {
        // Jinja2's do_replace: under autoescape a Markup string, or a Markup old or new, makes it a Markup replace
        auto oldVal = GetArgumentValue("old", context);
        auto newVal = GetArgumentValue("new", context);
        InternalValue srcVal = baseVal;
        bool isMarkup = false;
        if (context.IsAutoescape())
        {
            auto* callback = context.GetRendererCallback();
            if (oldVal.IsMarkup() || (newVal.IsMarkup() && !baseVal.IsMarkup()))
            {
                srcVal = MarkupEscape(baseVal, callback);
            }
            isMarkup = srcVal.IsMarkup();
            // MarkupSafe 3 escapes only `new`
            if (isMarkup)
            {
                newVal = MarkupEscape(newVal, callback);
            }
        }
        result = ApplyStringConverter(srcVal, [this, &context, &oldVal, &newVal](auto srcStr) -> TargetString {
            std::decay_t<decltype(srcStr)> emptyStrView;
            using CharT = typename decltype(emptyStrView)::value_type;
            std::basic_string<CharT> emptyStr;
            auto oldStr = GetAsSameString(srcStr, oldVal).value_or(emptyStr);
            auto newStr = GetAsSameString(srcStr, newVal).value_or(emptyStr);
            auto count = ConvertToInt(this->GetArgumentValue("count", context));
            auto str = sv_to_string(srcStr);
            if (count == 0)
            {
                ba::replace_all(str, oldStr, newStr);
            }
            else
            {
                for (int64_t n = 0; n < count; ++n)
                {
                    ba::replace_first(str, oldStr, newStr);
                }
            }
            return str;
        });
        InternalValue replaced(std::move(result));
        replaced.SetMarkup(isMarkup);
        return replaced;
    }
    case TruncateMode:
        result = ApplyStringConverter(baseVal, [this, &context](auto srcStr) -> TargetString {
            using CharT = typename decltype(srcStr)::value_type;
            using String = std::basic_string<CharT>;
            auto leewayVal = this->GetArgumentValue("leeway", context);
            auto length = NumericArgument(this->GetArgumentValue("length", context), "truncate", "length", NumberKind::Any);
            auto killWords = ConvertToBool(this->GetArgumentValue("killwords", context));
            auto end = GetAsSameString(srcStr, this->GetArgumentValue("end", context)).value_or(String());
            auto leeway = IsEmpty(leewayVal) ? 5 : NumericArgument(leewayVal, "truncate", "leeway", NumberKind::Any);
            auto endLength = CodePointCount(end);
            // Jinja2 asserts both
            if (length < endLength || leeway < 0)
            {
                throw std::runtime_error("truncate(): expected length >= len(end) and leeway >= 0");
            }

            // Port of Jinja2's do_truncate, counting code points
            auto chars = SplitCodePoints(srcStr);
            // length + leeway can overflow; length >= 0 here, so the subtraction cannot
            if (static_cast<int64_t>(chars.size()) - length <= leeway)
            {
                return sv_to_string(srcStr);
            }

            String truncated;
            for (size_t n = 0; n != static_cast<size_t>(length - endLength); ++n)
            {
                truncated.append(chars[n].begin(), chars[n].end());
            }
            // Without killwords the last partial word goes: rsplit(" ", 1)[0]
            if (!killWords)
            {
                auto space = truncated.rfind(CharT(' '));
                if (space != String::npos)
                {
                    truncated.erase(space);
                }
            }
            return truncated + end;
        });
        break;
    case CapitalMode:
        result = ApplyStringConverter<GenericStringEncoder>(baseVal, [isFirstChar = true, &isAlpha](auto ch, auto&& fn) mutable {
            if (isAlpha(ch))
            {
                if (isFirstChar)
                {
                    fn(std::toupper(ch, std::locale()));
                }
                else
                {
                    fn(std::tolower(ch, std::locale()));
                }
            }
            else
            {
                fn(ch);
            }

            isFirstChar = false;
        });
        break;
    case IndentMode:
        result = ApplyStringConverter(baseVal, [this, &context](auto srcStr) -> TargetString {
            using CharT = typename decltype(srcStr)::value_type;
            auto width = this->GetArgumentValue("width", context);
            // A string width is the indentation itself, a number counts spaces
            auto indention = GetAsSameString(srcStr, width);
            if (!indention)
            {
                indention = std::basic_string<CharT>(static_cast<size_t>(std::max<int64_t>(0, ConvertToInt(width))), ' ');
            }
            auto first = ConvertToBool(this->GetArgumentValue("first", context));
            auto blank = ConvertToBool(this->GetArgumentValue("blank", context));
            return Indent(srcStr, *indention, first, blank);
        });
        break;
    case UrlizeMode:
        result = ApplyStringConverter(baseVal, [this, &context](auto srcStr) -> TargetString {
            using CharT = typename decltype(srcStr)::value_type;
            using String = std::basic_string<CharT>;
            auto limitVal = this->GetArgumentValue("trim_url_limit", context);
            std::optional<int64_t> limit;
            if (!IsEmpty(limitVal))
            {
                limit = ConvertToInt(limitVal);
            }

            // The rel words, plus nofollow and the default policy's noopener, sorted and unique
            std::vector<String> relParts;
            std::basic_istringstream<CharT> relWords(GetAsSameString(srcStr, this->GetArgumentValue("rel", context)).value_or(String()));
            for (String word; relWords >> word;)
            {
                relParts.push_back(word);
            }
            if (ConvertToBool(this->GetArgumentValue("nofollow", context)))
            {
                relParts.push_back(AsciiString<CharT>("nofollow"));
            }
            relParts.push_back(AsciiString<CharT>("noopener"));
            std::sort(relParts.begin(), relParts.end());
            relParts.erase(std::unique(relParts.begin(), relParts.end()), relParts.end());
            String rel;
            for (auto& part : relParts)
            {
                rel += (rel.empty() ? String() : String(1, ' ')) + part;
            }

            auto target = GetAsSameString(srcStr, this->GetArgumentValue("target", context)).value_or(String());

            std::vector<String> extraSchemes;
            auto schemesVal = this->GetArgumentValue("extra_schemes", context);
            bool isList = false;
            auto schemes = ConvertToList(schemesVal, isList);
            for (const InternalValue& scheme : isList ? schemes : ListAdapter::CreateAdapter(InternalValueList()))
            {
                auto str = GetAsSameString(srcStr, scheme);
                if (!str || !Urlizer<CharT>::IsValidScheme(*str))
                {
                    context.GetRendererCallback()->ThrowRuntimeError(ErrorCode::InvalidValueType, ValuesList{});
                }
                extraSchemes.push_back(*str);
            }

            return Urlizer<CharT>(limit, rel, target, std::move(extraSchemes))(srcStr);
        });
        break;
    case StriptagsMode:
        result = ApplyStringConverter(baseVal, [](auto srcStr) -> TargetString { return StripTags(srcStr); });
        break;
    case CenterMode:
        result = ApplyStringConverter(baseVal, [this, &context](auto srcStr) -> TargetString {
            auto width = NumericArgument(this->GetArgumentValue("width", context), "center", "width", NumberKind::Int);
            auto str = sv_to_string(srcStr);
            auto length = CodePointCount(str);
            if (length >= width)
            {
                return str;
            }
            // CPython's str.center puts the odd space on the left only when width is odd too
            auto margin = width - length;
            auto left = (margin / 2) + (margin & width & 1);
            str.insert(0, static_cast<size_t>(left), ' ');
            str.append(static_cast<size_t>(margin - left), ' ');
            return TargetString(std::move(str));
        });
        break;
    case WordWrapMode:
        result = ApplyStringConverter(baseVal, [this, &context](auto srcStr) -> TargetString {
            using CharT = typename decltype(srcStr)::value_type;
            auto width = NumericArgument(this->GetArgumentValue("width", context), "wordwrap", "width", NumberKind::Whole);
            auto breakLongWords = ConvertToBool(this->GetArgumentValue("break_long_words", context));
            auto breakOnHyphens = ConvertToBool(this->GetArgumentValue("break_on_hyphens", context));
            // Jinja2 wraps with the environment's newline_sequence unless wrapstring is given
            auto* callback = context.GetRendererCallback();
            const std::string newline = callback ? callback->GetSettings().newlineSequence : std::string("\n");
            auto wrapString =
                GetAsSameString(srcStr, this->GetArgumentValue("wrapstring", context)).value_or(std::basic_string<CharT>(newline.begin(), newline.end()));
            // Python raises "invalid width" here
            if (width <= 0)
            {
                return sv_to_string(srcStr);
            }
            return WordWrap(srcStr, width, breakLongWords, wrapString, breakOnHyphens);
        });
        break;
    default:
        break;
    }

    // Markup's own string methods return Markup; urlize returns Markup under autoescape
    bool isMarkup = false;
    switch (m_mode)
    {
    case UpperMode:
    case LowerMode:
    case CapitalMode:
    case TrimMode:
    case CenterMode:
    case IndentMode:
        isMarkup = baseVal.IsMarkup();
        break;
    case UrlizeMode:
        isMarkup = context.IsAutoescape();
        break;
    default:
        break;
    }
    InternalValue resultVal(std::move(result));
    resultVal.SetMarkup(isMarkup);
    return resultVal;
}

} // namespace jinja2::filters
