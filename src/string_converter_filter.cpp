#include "filters.h"
#include "function_base.h"
#include "internal_value.h"
#include "markup.h"
#include "render_context.h"
#include "unicode_tables.h"
#include "value_visitors.h"

#include <jinja2cpp/error_info.h>

#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/trim_all.hpp>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <locale>
#include <optional>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace ba = boost::algorithm;

using namespace std::string_literals;

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
class TextWrapper
{
public:
    using View = std::basic_string_view<CharT>;
    using String = std::basic_string<CharT>;
    using Range = std::pair<size_t, size_t>;

    TextWrapper(int64_t width, bool breakLongWords, bool breakOnHyphens)
        : m_width(width)
        , m_breakLongWords(breakLongWords)
        , m_breakOnHyphens(breakOnHyphens)
    {
    }

    String Wrap(View text, const String& wrapString)
    {
        std::vector<String> lines;
        for (auto& paragraph : SplitParagraphs(text))
        {
            m_line = &paragraph;
            lines.push_back(Join(WrapChunks(SplitChunks()), wrapString));
        }
        m_line = nullptr;
        return Join(lines, wrapString);
    }

private:
    static int AsciiOf(View ch)
    {
        auto unit = CodeUnit(ch[0]);
        return ch.size() == 1 && unit < 0x80 ? static_cast<int>(unit) : -1;
    }

    // textwrap chunks on ASCII whitespace only (its _whitespace), not on str.isspace()
    static bool IsSpace(View ch)
    {
        auto c = AsciiOf(ch);
        return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
    }

    // The line boundaries of str.splitlines()
    static bool IsLineBreak(View ch)
    {
        auto c = CodePointValue(ch);
        return (c >= 0x0A && c <= 0x0D) || (c >= 0x1C && c <= 0x1E) || c == 0x85 || c == 0x2028 || c == 0x2029;
    }

    static bool IsHyphen(View ch) { return AsciiOf(ch) == '-'; }

    static String Join(const std::vector<String>& parts, const String& separator)
    {
        String result;
        for (size_t n = 0; n != parts.size(); ++n)
        {
            result += (n == 0 ? String() : separator) + parts[n];
        }
        return result;
    }

    static std::vector<std::vector<View>> SplitParagraphs(View text)
    {
        std::vector<std::vector<View>> paragraphs;
        std::vector<View> current;
        auto chars = SplitCodePoints(text);
        for (size_t n = 0; n < chars.size(); ++n)
        {
            if (!IsLineBreak(chars[n]))
            {
                current.push_back(chars[n]);
                continue;
            }
            if (AsciiOf(chars[n]) == '\r' && n + 1 < chars.size() && AsciiOf(chars[n + 1]) == '\n')
            {
                ++n;
            }
            paragraphs.push_back(std::move(current));
            current.clear();
        }
        if (!current.empty())
        {
            paragraphs.push_back(std::move(current));
        }
        return paragraphs;
    }

    [[nodiscard]] const std::vector<View>& Line() const { return *m_line; }

    // textwrap's letter is [^\d\W]: a word character that is not a decimal digit
    [[nodiscard]] bool IsLetter(size_t idx) const
    {
        if (idx >= Line().size())
        {
            return false;
        }
        auto cp = CodePointValue(Line()[idx]);
        return unicode::IsWordChar(cp) && !unicode::IsDecimal(cp);
    }

    [[nodiscard]] bool IsHyphenAt(size_t idx) const { return idx < Line().size() && IsHyphen(Line()[idx]); }

    // \w, and textwrap's word punctuation [\w!"'&.,?]
    [[nodiscard]] bool IsWordChar(size_t idx) const { return idx < Line().size() && unicode::IsWordChar(CodePointValue(Line()[idx])); }

    [[nodiscard]] bool IsWordPunct(size_t idx) const
    {
        auto c = idx < Line().size() ? AsciiOf(Line()[idx]) : -1;
        return IsWordChar(idx) || (c >= 0 && std::strchr("!\"'&.,?", c) != nullptr && c != 0);
    }

    // Length of an em-dash ("--" or longer, followed by a word character) starting at idx
    [[nodiscard]] size_t EmDashAt(size_t idx) const
    {
        auto end = idx;
        while (IsHyphenAt(end))
        {
            ++end;
        }
        return end - idx >= 2 && IsWordChar(end) ? end - idx : 0;
    }

    // An em-dash right after word punctuation starts a chunk of its own
    [[nodiscard]] bool EmDashAfterWord(size_t idx) const { return m_breakOnHyphens && idx > 0 && IsWordPunct(idx - 1) && EmDashAt(idx) != 0; }

    // A word is split after a hyphen between letters, like "long-word" -> "long-", "word"
    [[nodiscard]] bool SplitsAfter(size_t h) const
    {
        bool before = h >= 2 && IsLetter(h - 1) && (IsLetter(h - 2) || (h >= 3 && IsHyphenAt(h - 2) && IsLetter(h - 3)));
        bool after = IsLetter(h + 1) && (IsLetter(h + 2) || (IsHyphenAt(h + 2) && IsLetter(h + 3)));
        return before && after;
    }

    // End of the word chunk that starts at pos
    [[nodiscard]] size_t WordEnd(size_t pos) const
    {
        auto end = pos;
        while (end < Line().size() && !IsSpace(Line()[end]))
        {
            if (end > pos && EmDashAfterWord(end))
            {
                break;
            }
            if (m_breakOnHyphens && IsHyphen(Line()[end]) && end > pos && SplitsAfter(end))
            {
                return end + 1;
            }
            ++end;
        }
        return end;
    }

    [[nodiscard]] size_t ChunkEnd(size_t pos) const
    {
        if (IsSpace(Line()[pos]))
        {
            auto end = pos + 1;
            while (end < Line().size() && IsSpace(Line()[end]))
            {
                ++end;
            }
            return end;
        }
        // An em-dash between words is a chunk of its own: "hello--world" -> "hello", "--", "world"
        if (EmDashAfterWord(pos))
        {
            return pos + EmDashAt(pos);
        }
        return WordEnd(pos);
    }

    [[nodiscard]] std::vector<Range> SplitChunks() const
    {
        std::vector<Range> chunks;
        for (size_t pos = 0; pos < Line().size();)
        {
            auto end = ChunkEnd(pos);
            chunks.emplace_back(pos, end);
            pos = end;
        }
        return chunks;
    }

    static int64_t ChunkLen(const Range& r) { return static_cast<int64_t>(r.second - r.first); }

    // Chunks split on ASCII whitespace, but drop_whitespace tests chunk.strip() == '',
    // which is Unicode-aware: a chunk of NBSPs is dropped too
    [[nodiscard]] bool IsSpaceChunk(const Range& r) const
    {
        for (auto n = r.first; n != r.second; ++n)
        {
            if (!unicode::IsSpace(CodePointValue(Line()[n])))
            {
                return false;
            }
        }
        return true;
    }

    // How much of a too-long chunk fits in spaceLeft; with break_on_hyphens, the cut moves back
    // to just after the last hyphen that has a non-hyphen before it
    [[nodiscard]] size_t LongWordCut(const Range& chunk, int64_t spaceLeft) const
    {
        auto end = static_cast<size_t>(spaceLeft);
        if (!m_breakOnHyphens || ChunkLen(chunk) <= spaceLeft)
        {
            return end;
        }
        for (size_t h = end; h-- > 1;)
        {
            if (!IsHyphen(Line()[chunk.first + h]))
            {
                continue;
            }
            for (size_t n = 0; n != h; ++n)
            {
                if (!IsHyphen(Line()[chunk.first + n]))
                {
                    return h + 1;
                }
            }
            return end;
        }
        return end;
    }

    // textwrap's _handle_long_word: next points at a chunk longer than the width
    void HandleLongWord(std::vector<Range>& chunks, size_t& next, std::vector<Range>& curLine, int64_t curLen) const
    {
        auto spaceLeft = m_width < 1 ? 1 : m_width - curLen;
        auto& chunk = chunks[next];
        if (m_breakLongWords)
        {
            auto end = LongWordCut(chunk, spaceLeft);
            curLine.emplace_back(chunk.first, chunk.first + end);
            chunk.first += end;
        }
        else if (curLine.empty())
        {
            curLine.push_back(chunk);
            ++next;
        }
    }

    [[nodiscard]] String LineText(const std::vector<Range>& curLine) const
    {
        String out;
        for (const auto& [first, last] : curLine)
        {
            for (auto n = first; n != last; ++n)
            {
                out.append(Line()[n].begin(), Line()[n].end());
            }
        }
        return out;
    }

    [[nodiscard]] std::vector<String> WrapChunks(std::vector<Range> chunks) const
    {
        std::vector<String> wrapped;
        size_t next = 0;
        while (next < chunks.size())
        {
            std::vector<Range> curLine;
            int64_t curLen = 0;
            if (!wrapped.empty() && IsSpaceChunk(chunks[next]))
            {
                ++next;
            }

            for (; next < chunks.size() && curLen + ChunkLen(chunks[next]) <= m_width; ++next)
            {
                curLine.push_back(chunks[next]);
                curLen += ChunkLen(chunks[next]);
            }

            if (next < chunks.size() && ChunkLen(chunks[next]) > m_width)
            {
                HandleLongWord(chunks, next, curLine, curLen);
            }

            if (!curLine.empty() && IsSpaceChunk(curLine.back()))
            {
                curLine.pop_back();
            }
            if (!curLine.empty())
            {
                wrapped.push_back(LineText(curLine));
            }
        }
        return wrapped;
    }

    int64_t m_width;
    bool m_breakLongWords;
    bool m_breakOnHyphens;
    const std::vector<View>* m_line = nullptr;
};

template<typename CharT>
std::basic_string<CharT> WordWrap(std::basic_string_view<CharT> text, int64_t width, bool breakLongWords, const std::basic_string<CharT>& wrapString, bool breakOnHyphens)
{
    return TextWrapper<CharT>(width, breakLongWords, breakOnHyphens).Wrap(text, wrapString);
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

    // The end of the host, before the port, path, query and fragment
    static size_t FindHostEnd(const Chars& chars, size_t scheme)
    {
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
        return end;
    }

    // (([\w%-]+\.)+)? ([a-z]{2,63} | xn--[\w%]{2,59}) on [prefix, end)
    static bool IsDomainHost(const Chars& chars, size_t prefix, size_t end)
    {
        auto labels = SplitLabels(chars, prefix, end);
        auto& tld = labels.back();
        auto tldLen = tld.second - tld.first;
        bool basicTld = tldLen >= 2 && tldLen <= 63 && All(chars, tld.first, tld.second, [](uint32_t cp) { return cp < 0x80 && std::isalpha(static_cast<int>(cp)); });
        bool idnaTld = StartsWithNoCase(chars, tld.first, "xn--") && tldLen >= 6 && tldLen <= 63 && All(chars, tld.first + 4, tld.second, [](uint32_t cp) { return unicode::IsWordChar(cp) || cp == '%'; });
        bool tldOk = basicTld || idnaTld;
        bool labelsOk = std::all_of(labels.begin(), labels.end() - 1, [&chars](auto& l) { return l.second != l.first && All(chars, l.first, l.second, IsLabelChar); });
        return tldOk && labelsOk;
    }

    // ([\w%-]{2,63}\.)+ (com|net|int|edu|gov|org|info|mil) on [0, end)
    static bool IsCommonTldHost(const Chars& chars, size_t end)
    {
        auto labels = SplitLabels(chars, 0, end);
        auto& tld = labels.back();
        static const char* const tlds[] = { "com", "net", "int", "edu", "gov", "org", "info", "mil" };
        bool tldOk = std::any_of(std::begin(tlds), std::end(tlds), [&](const char* t) { return tld.second - tld.first == std::strlen(t) && StartsWithNoCase(chars, tld.first, t); });
        bool labelsOk = std::all_of(labels.begin(), labels.end() - 1, [&chars](auto& l) {
            auto len = l.second - l.first;
            return len >= 2 && len <= 63 && All(chars, l.first, l.second, IsLabelChar);
        });
        return labels.size() >= 2 && tldOk && labelsOk;
    }

    // ((\d{1,3})(\.\d{1,3}){3}) on [scheme, end)
    static bool IsIpv4Host(const Chars& chars, size_t scheme, size_t end)
    {
        auto labels = SplitLabels(chars, scheme, end);
        return labels.size() == 4 && std::all_of(labels.begin(), labels.end(), [&chars](auto& l) {
                   auto len = l.second - l.first;
                   return len >= 1 && len <= 3 && All(chars, l.first, l.second, unicode::IsDecimal);
               });
    }

    // (\[([\da-f]{0,4}:){2}([\da-f]{0,4}:?){1,6}]) on [scheme, end)
    static bool IsIpv6Host(const Chars& chars, size_t scheme, size_t end)
    {
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
        size_t end = FindHostEnd(chars, scheme);

        // (https?://|www\.) (([\w%-]+\.)+)? ([a-z]{2,63} | xn--[\w%]{2,59})
        size_t prefix = scheme;
        if (prefix == 0 && StartsWithNoCase(chars, 0, "www."))
        {
            prefix = 4;
        }
        if (prefix != 0 && IsDomainHost(chars, prefix, end))
        {
            return true;
        }

        // ([\w%-]{2,63}\.)+ (com|net|int|edu|gov|org|info|mil)
        if (IsCommonTldHost(chars, end))
        {
            return true;
        }

        if (scheme == 0)
        {
            return false;
        }

        // (https?://) ((\d{1,3})(\.\d{1,3}){3})
        if (IsIpv4Host(chars, scheme, end))
        {
            return true;
        }

        // (https?://) (\[([\da-f]{0,4}:){2}([\da-f]{0,4}:?){1,6}])
        return IsIpv6Host(chars, scheme, end);
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

    // Moves the leading punctuation of middle to the end of head
    static void StripLeadChars(String& middle, String& head)
    {
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
    }

    // Moves the trailing punctuation of middle to the start of tail
    static void StripTrailChars(String& middle, String& tail)
    {
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
    }

    // Prefer balancing parentheses in URLs instead of ignoring a trailing character
    static void BalanceBrackets(String& middle, String& tail)
    {
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
    }

    // The word without its surrounding punctuation as a link, or unchanged
    [[nodiscard]] String MakeLink(String middle) const
    {
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
            middle = Ascii("<a href=\"mailto:").append(middle).append(Ascii("\">")).append(middle).append(Ascii("</a>"));
        }
        else
        {
            for (auto& scheme : m_extraSchemes)
            {
                if (middle != scheme && middle.compare(0, scheme.size(), scheme) == 0)
                {
                    middle = Ascii("<a href=\"").append(middle).append(Ascii("\"")).append(m_relAttr).append(m_targetAttr).append(Ascii(">")).append(middle).append(Ascii("</a>"));
                }
            }
        }
        return middle;
    }

    [[nodiscard]] String ProcessWord(String middle) const
    {
        String head;
        String tail;
        StripLeadChars(middle, head);
        StripTrailChars(middle, tail);
        BalanceBrackets(middle, tail);
        return head + MakeLink(std::move(middle)) + tail;
    }

    std::optional<int64_t> m_trimUrlLimit;
    std::vector<String> m_extraSchemes;
    String m_relAttr;
    String m_targetAttr;
};

// An ASCII code unit in upper- or lowercase; any other unit unchanged. ASCII maps the same
// in every locale, as in Python's str methods. In a UTF-8 string this covers every byte:
// the others are parts of multi-byte characters.
template<bool upper, typename CharT>
CharT AsciiWithCase(CharT ch)
{
    const CharT from = upper ? 'a' : 'A';
    return ch >= from && ch <= from + 25 ? static_cast<CharT>(ch ^ 0x20) : ch;
}

template<typename CharT>
bool IsAscii(std::basic_string_view<CharT> str)
{
    // A reduction rather than an early exit, so that the compiler vectorises it
    uint32_t bits = 0;
    for (auto ch : str)
    {
        bits |= CodeUnit(ch);
    }
    return bits < 0x80;
}

// One character (code point) as an upper- or lowercase character. Only single-unit
// characters change: ASCII in UTF-8 strings, the BMP in wide strings.
template<typename CharT>
void AppendWithCase(std::basic_string<CharT>& out, std::basic_string_view<CharT> ch, bool upper)
{
    if (ch.size() != 1)
    {
        out.append(ch.begin(), ch.end());
    }
    else if (CodeUnit(ch[0]) < 0x80)
    {
        out.push_back(upper ? AsciiWithCase<true>(ch[0]) : AsciiWithCase<false>(ch[0]));
    }
    else if (sizeof(CharT) != 1)
    {
        out.push_back(upper ? std::toupper(ch[0], std::locale()) : std::tolower(ch[0], std::locale()));
    }
    else
    {
        out.push_back(ch[0]);
    }
}

// Port of Jinja2's do_title: each word (split on runs of -, whitespace, (, {, [ and <) gets
// an uppercase first character and lowercase others
template<typename CharT>
std::basic_string<CharT> TitleCase(std::basic_string_view<CharT> str)
{
    std::basic_string<CharT> result;
    if (IsAscii(str))
    {
        result.assign(str.begin(), str.end());
        bool wordStart = true;
        for (auto& ch : result)
        {
            ch = wordStart ? AsciiWithCase<true>(ch) : AsciiWithCase<false>(ch);
            wordStart = ch == '-' || ch == '(' || ch == '{' || ch == '[' || ch == '<' || unicode::IsSpace(CodeUnit(ch));
        }
        return result;
    }
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

// Python's str.strip(chars): without chars, Unicode whitespace. Only the characters at the two
// ends are looked at; they split as SplitCodePoints splits them.
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
    size_t first = 0;
    while (first != str.size())
    {
        auto end = first + 1;
        while (end != str.size() && IsCodePointTail(str[end]))
        {
            ++end;
        }
        if (!isStripped(str.substr(first, end - first)))
        {
            break;
        }
        first = end;
    }
    size_t last = str.size();
    while (last != first)
    {
        auto start = last - 1;
        while (start != first && IsCodePointTail(str[start]))
        {
            --start;
        }
        if (!isStripped(str.substr(start, last - start)))
        {
            break;
        }
        last = start;
    }
    return std::basic_string<CharT>(str.substr(first, last - first));
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

// The code point a numeric character reference stands for, as html.unescape maps it
inline uint32_t CharRefCodePoint(uint64_t value)
{
    // html._invalid_charrefs: C1 controls are read as Windows-1252
    static const uint16_t cp1252[32] = { 0x20AC, 0x81, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021, 0x02C6, 0x2030, 0x0160,
                                         0x2039, 0x0152, 0x8D, 0x017D, 0x8F, 0x90, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022,
                                         0x2013, 0x2014, 0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0x9D, 0x017E, 0x0178 };
    auto cp = static_cast<uint32_t>(value);
    if (cp >= 0x80 && cp <= 0x9F)
    {
        return cp1252[cp - 0x80];
    }
    if (cp == 0 || cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF))
    {
        return 0xFFFD;
    }
    return cp;
}

// &#123; or &#x1F; (the ';' is optional) at pos; false when no digits follow
template<typename CharT>
bool UnescapeNumeric(const std::basic_string<CharT>& str, size_t& pos, std::basic_string<CharT>& result)
{
    auto next = pos + 2;
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
    if (next == digitsStart)
    {
        return false;
    }
    if (next < str.size() && str[next] == ';')
    {
        ++next;
    }
    AppendCodePoint(result, CharRefCodePoint(value));
    pos = next;
    return true;
}

// &name; at pos for the common named references; false when the name is not one of them
template<typename CharT>
bool UnescapeNamed(const std::basic_string<CharT>& str, size_t& pos, std::basic_string<CharT>& result)
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
    auto next = pos + 1;
    auto semicolon = str.find(';', next);
    if (semicolon == std::basic_string<CharT>::npos || semicolon - next > 32)
    {
        return false;
    }
    std::string name;
    for (auto n = next; n != semicolon; ++n)
    {
        name.push_back(static_cast<unsigned>(str[n]) < 0x80 ? static_cast<char>(str[n]) : '?');
    }
    for (const auto& [entityName, codePoint] : named)
    {
        if (name == entityName)
        {
            AppendCodePoint(result, codePoint);
            pos = semicolon + 1;
            return true;
        }
    }
    return false;
}

// html.unescape for the character references markupsafe's striptags leaves: numeric ones
// and the common named ones (the full HTML5 table is task 0048)
template<typename CharT>
std::basic_string<CharT> HtmlUnescape(const std::basic_string<CharT>& str)
{
    std::basic_string<CharT> result;
    for (size_t pos = 0; pos < str.size();)
    {
        if (str[pos] == '&')
        {
            bool numeric = pos + 1 < str.size() && str[pos + 1] == '#';
            if (numeric ? UnescapeNumeric(str, pos, result) : UnescapeNamed(str, pos, result))
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
    {
        static const auto args = MakeArgumentsTable({ { "old", true }, { "new", true }, { "count", false, static_cast<int64_t>(0) } });
        ParseParams(args, params);
        break;
    }
    case TruncateMode:
    {
        static const auto args = MakeArgumentsTable({ { "length", false, static_cast<int64_t>(255) }, { "killwords", false, false }, { "end", false, "..."s }, { "leeway", false } });
        ParseParams(args, params);
        break;
    }
    case CenterMode:
    {
        static const auto args = MakeArgumentsTable({ { "width", false, static_cast<int64_t>(80) } });
        ParseParams(args, params);
        break;
    }
    case WordWrapMode:
    {
        static const auto args = MakeArgumentsTable({ { "width", false, static_cast<int64_t>(79) }, { "break_long_words", false, true }, { "wrapstring", false }, { "break_on_hyphens", false, true } });
        ParseParams(args, params);
        break;
    }
    case IndentMode:
    {
        static const auto args = MakeArgumentsTable({ { "width", false, static_cast<int64_t>(4) }, { "first", false, false }, { "blank", false, false } });
        ParseParams(args, params);
        break;
    }
    case UrlizeMode:
    {
        static const auto args = MakeArgumentsTable({ { "trim_url_limit", false }, { "nofollow", false, false }, { "target", false }, { "rel", false }, { "extra_schemes", false } });
        ParseParams(args, params);
        break;
    }
    case TrimMode:
    {
        static const auto args = MakeArgumentsTable({ { "chars", false } });
        ParseParams(args, params);
        break;
    }
    default:
        ParseParams({}, params);
        break;
    }
}

InternalValue StringConverter::ApplyUrlEncode(const InternalValue& baseVal, RenderContext& context)
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

InternalValue StringConverter::ApplyReplace(const InternalValue& baseVal, RenderContext& context)
{
    TargetString result;
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
        auto str = std::basic_string(srcStr);
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

TargetString StringConverter::ApplyTruncate(const InternalValue& baseVal, RenderContext& context)
{
    return ApplyStringConverter(baseVal, [this, &context](auto srcStr) -> TargetString {
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
            return std::basic_string(srcStr);
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
}

TargetString StringConverter::ApplyIndent(const InternalValue& baseVal, RenderContext& context)
{
    return ApplyStringConverter(baseVal, [this, &context](auto srcStr) -> TargetString {
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
}

TargetString StringConverter::ApplyUrlize(const InternalValue& baseVal, RenderContext& context)
{
    return ApplyStringConverter(baseVal, [this, &context](auto srcStr) -> TargetString {
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
}

TargetString StringConverter::ApplyCenter(const InternalValue& baseVal, RenderContext& context)
{
    return ApplyStringConverter(baseVal, [this, &context](auto srcStr) -> TargetString {
        auto width = NumericArgument(this->GetArgumentValue("width", context), "center", "width", NumberKind::Int);
        auto str = std::basic_string(srcStr);
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
}

TargetString StringConverter::ApplyWordWrap(const InternalValue& baseVal, RenderContext& context)
{
    return ApplyStringConverter(baseVal, [this, &context](auto srcStr) -> TargetString {
        using CharT = typename decltype(srcStr)::value_type;
        auto width = NumericArgument(this->GetArgumentValue("width", context), "wordwrap", "width", NumberKind::Whole);
        auto breakLongWords = ConvertToBool(this->GetArgumentValue("break_long_words", context));
        auto breakOnHyphens = ConvertToBool(this->GetArgumentValue("break_on_hyphens", context));
        // Jinja2 wraps with the environment's newline_sequence unless wrapstring is given
        auto* callback = context.GetRendererCallback();
        const std::string newline = callback ? callback->GetSettings().newlineSequence : "\n"s;
        auto wrapString =
            GetAsSameString(srcStr, this->GetArgumentValue("wrapstring", context)).value_or(std::basic_string<CharT>(newline.begin(), newline.end()));
        // Python raises "invalid width" here
        if (width <= 0)
        {
            return std::basic_string(srcStr);
        }
        return WordWrap(srcStr, width, breakLongWords, wrapString, breakOnHyphens);
    });
}

// One code unit in upper- or lowercase as upper, lower and capitalize map it: ASCII by itself,
// other wide units through the global locale (letters only)
template<bool upper, typename CharT>
CharT UnitWithCase(CharT ch)
{
    if constexpr (sizeof(CharT) != 1)
    {
        if (CodeUnit(ch) >= 0x80)
        {
            std::locale loc;
            if (!std::isalpha(ch, loc))
            {
                return ch;
            }
            return upper ? std::toupper(ch, loc) : std::tolower(ch, loc);
        }
    }
    return AsciiWithCase<upper>(ch);
}

// Upper, lower or capitalize (first unit upper, the rest lower) one code unit at a time
template<bool upper, bool capitalize, typename CharT>
std::basic_string<CharT> MapCase(std::basic_string_view<CharT> str)
{
    std::basic_string<CharT> result(str);
    auto it = result.begin();
    if (capitalize && it != result.end())
    {
        *it = UnitWithCase<true>(*it);
        ++it;
    }
    std::transform(it, result.end(), it, UnitWithCase<upper, CharT>);
    return result;
}

TargetString StringConverter::MapChars(const InternalValue& baseVal) const
{
    switch (m_mode)
    {
    case UpperMode:
        return ApplyStringConverter(baseVal, [](auto str) -> TargetString { return MapCase<true, false>(str); });
    case LowerMode:
        return ApplyStringConverter(baseVal, [](auto str) -> TargetString { return MapCase<false, false>(str); });
    default:
        return ApplyStringConverter(baseVal, [](auto str) -> TargetString { return MapCase<false, true>(str); });
    }
}

int64_t StringConverter::WordCount(const InternalValue& baseVal)
{
    auto isAlNum = ba::is_alnum();
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
    return wc;
}

// Markup's own string methods return Markup; urlize returns Markup under autoescape
bool StringConverter::ReturnsMarkup(const InternalValue& baseVal, RenderContext& context) const
{
    switch (m_mode)
    {
    case UpperMode:
    case LowerMode:
    case CapitalMode:
    case TrimMode:
    case CenterMode:
    case IndentMode:
        return baseVal.IsMarkup();
    case UrlizeMode:
        return context.IsAutoescape();
    default:
        return false;
    }
}

// The modes that return a string, Markup or not as ReturnsMarkup says
TargetString StringConverter::Convert(const InternalValue& baseVal, RenderContext& context)
{
    switch (m_mode)
    {
    case TrimMode:
        return ApplyStringConverter(baseVal, [this, &context](auto srcStr) -> TargetString {
            auto chars = GetAsSameString(srcStr, this->GetArgumentValue("chars", context));
            return PythonStrip(srcStr, chars);
        });
    case TitleMode:
        return ApplyStringConverter(baseVal, [](auto srcStr) -> TargetString { return TitleCase(srcStr); });
    case UpperMode:
    case LowerMode:
    case CapitalMode:
        return MapChars(baseVal);
    case TruncateMode:
        return ApplyTruncate(baseVal, context);
    case IndentMode:
        return ApplyIndent(baseVal, context);
    case UrlizeMode:
        return ApplyUrlize(baseVal, context);
    case StriptagsMode:
        return ApplyStringConverter(baseVal, [](auto srcStr) -> TargetString { return StripTags(srcStr); });
    case CenterMode:
        return ApplyCenter(baseVal, context);
    case WordWrapMode:
        return ApplyWordWrap(baseVal, context);
    default:
        return {};
    }
}

InternalValue StringConverter::Filter(const InternalValue& baseVal, RenderContext& context)
{
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
        return ApplyUrlEncode(baseVal, context);
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

    if (m_mode == WordCountMode)
    {
        return InternalValue(WordCount(baseVal));
    }
    if (m_mode == ReplaceMode)
    {
        return ApplyReplace(baseVal, context);
    }
    InternalValue resultVal(Convert(baseVal, context));
    resultVal.SetMarkup(ReturnsMarkup(baseVal, context));
    return resultVal;
}

} // namespace jinja2::filters
