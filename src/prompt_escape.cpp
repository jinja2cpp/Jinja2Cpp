#include "prompt_escape.h"

#include "internal_value.h"

#include <jinja2cpp/prompt.h>
#include <jinja2cpp/string_helpers.h>
#include <jinja2cpp/template_env.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace jinja2
{
inline namespace v2
{
namespace detail
{
namespace
{
// Not std::tolower: the process locale must not change which letters match
constexpr char AsciiLower(char ch) noexcept
{
    return ch >= 'A' && ch <= 'Z' ? static_cast<char>(ch - 'A' + 'a') : ch;
}

// The character `at(idx)` of the text prefix + name, without building it
struct Joined
{
    std::string_view prefix;
    std::string_view name;
    [[nodiscard]] std::size_t size() const noexcept { return prefix.size() + name.size(); }
    [[nodiscard]] char at(std::size_t idx) const noexcept { return idx < prefix.size() ? prefix[idx] : name[idx - prefix.size()]; }
};

// Whether needle occurs in the pattern, ignoring the case of ASCII letters (the matching ignores it, so a separator
// that differs from a pattern's text only in case could build a match)
bool Occurs(std::string_view needle, const Joined& pattern) noexcept
{
    if (needle.size() > pattern.size())
    {
        return false;
    }
    for (std::size_t start = 0; start + needle.size() <= pattern.size(); ++start)
    {
        std::size_t idx = 0;
        while (idx != needle.size() && AsciiLower(pattern.at(start + idx)) == AsciiLower(needle[idx]))
        {
            ++idx;
        }
        if (idx == needle.size())
        {
            return true;
        }
    }
    return false;
}

template<typename CharT>
std::basic_string<CharT> ToWidth(const std::string& text)
{
    if constexpr (std::is_same_v<CharT, char>)
    {
        return text;
    }
    else
    {
        return ConvertString<std::basic_string<CharT>>(std::string_view(text));
    }
}

template<typename CharT>
std::vector<std::basic_string<CharT>> ToWidth(const std::vector<std::string>& texts)
{
    std::vector<std::basic_string<CharT>> result;
    result.reserve(texts.size());
    for (const auto& text : texts)
    {
        result.push_back(ToWidth<CharT>(text));
    }
    return result;
}
} // namespace

template<typename CharT>
PromptMatcher<CharT>::PromptMatcher(const std::vector<std::basic_string<CharT>>& specials, const std::vector<std::basic_string<CharT>>& tags)
{
    auto unitsOf = [](std::basic_string_view<CharT> text) {
        std::vector<Unit> units;
        units.reserve(text.size());
        for (auto ch : text)
        {
            units.push_back(Fold(static_cast<Unit>(ch)));
        }
        return units;
    };
    auto markFirst = [this](Unit unit) {
        // Both cases of a letter, by the low 8 bits: a unit of a wide text only has to pass the filter, the trie decides
        const auto low = static_cast<unsigned>(unit & 0xFFU);
        m_first[low] = true;
        if (low >= 'a' && low <= 'z')
        {
            m_first[low - ('a' - 'A')] = true;
        }
    };
    auto markLast = [this](Unit unit) {
        const auto low = static_cast<unsigned>(unit & 0xFFU);
        m_last[low] = true;
        if (low >= 'a' && low <= 'z')
        {
            m_last[low - ('a' - 'A')] = true;
        }
    };

    for (const auto& special : specials)
    {
        if (special.empty())
        {
            continue;
        }
        auto units = unitsOf(special);
        Insert(m_forward, units, SpecialEnd);
        markFirst(units.front());
        m_maxPrefix = std::max(m_maxPrefix, units.size());
        if (units.size() > 1)
        {
            // The paths of the reverse trie are the proper suffixes: the last unit first, the first unit left out
            markLast(units.back());
            std::vector<Unit> reversed(units.rbegin(), units.rend() - 1);
            Insert(m_reverse, reversed, 0);
            m_maxSuffix = std::max(m_maxSuffix, reversed.size());
        }
    }
    for (const auto& tag : tags)
    {
        if (tag.empty())
        {
            continue;
        }
        for (int closing = 0; closing != 2; ++closing)
        {
            std::vector<Unit> units;
            units.push_back(static_cast<Unit>('<'));
            if (closing)
            {
                units.push_back(static_cast<Unit>('/'));
            }
            for (auto unit : unitsOf(tag))
            {
                units.push_back(unit);
            }
            Insert(m_forward, units, TagEnd);
            markFirst(units.front());
            m_maxPrefix = std::max(m_maxPrefix, units.size());
        }
    }
}

template<typename CharT>
std::uint32_t PromptMatcher<CharT>::Child(const Trie& trie, std::uint32_t node, Unit unit) noexcept
{
    const auto& next = trie[node].next;
    const auto found = std::lower_bound(next.begin(), next.end(), unit, [](const auto& edge, Unit value) { return edge.first < value; });
    return found != next.end() && found->first == unit ? found->second : None;
}

template<typename CharT>
void PromptMatcher<CharT>::Insert(Trie& trie, const std::vector<Unit>& units, std::uint8_t flags)
{
    std::uint32_t node = 0;
    for (auto unit : units)
    {
        auto child = Child(trie, node, unit);
        if (child == None)
        {
            child = static_cast<std::uint32_t>(trie.size());
            trie.emplace_back();
            auto& next = trie[node].next;
            const auto at = std::lower_bound(next.begin(), next.end(), unit, [](const auto& edge, Unit value) { return edge.first < value; });
            next.insert(at, { unit, child });
        }
        node = child;
    }
    trie[node].flags = static_cast<std::uint8_t>(trie[node].flags | flags);
}

template<typename CharT>
bool PromptMatcher<CharT>::MatchesAt(const CharT* text, std::size_t remaining, bool tagsFollowInsertions) const noexcept
{
    std::uint32_t node = 0;
    for (std::size_t depth = 0;; ++depth)
    {
        const auto flags = m_forward[node].flags;
        if ((flags & SpecialEnd) != 0)
        {
            return true;
        }
        if ((flags & TagEnd) != 0 && (depth == remaining || IsBoundary(static_cast<Unit>(text[depth])) || (tagsFollowInsertions && InsertsAfterLastCodePoint(text, depth, remaining))))
        {
            return true;
        }
        if (depth == remaining)
        {
            return false;
        }
        node = Child(m_forward, node, Fold(static_cast<Unit>(text[depth])));
        if (node == None)
        {
            return false;
        }
    }
}

template<typename CharT>
bool PromptMatcher<CharT>::InsertsAfterLastCodePoint(const CharT* text, std::size_t tagLength, std::size_t remaining) const noexcept
{
    // A separator goes after the first code point of every match, so a tag name whose last code point starts a
    // special is followed by the separator in the output; its first unit may be a tag boundary (a space)
    std::size_t last = tagLength - 1;
    if constexpr (sizeof(CharT) == 2)
    {
        const auto unit = static_cast<std::uint32_t>(static_cast<Unit>(text[last]));
        if (last != 0 && unit >= 0xDC00 && unit <= 0xDFFF)
        {
            --last;
        }
    }
    else if constexpr (sizeof(CharT) == 1)
    {
        while (last != 0 && (static_cast<unsigned char>(text[last]) & 0xC0U) == 0x80U)
        {
            --last;
        }
    }
    return m_first[static_cast<unsigned>(static_cast<Unit>(text[last]) & 0xFFU)] && MatchesAt(text + last, remaining - last, false);
}

template<typename CharT>
std::size_t PromptMatcher<CharT>::CodePointLength(std::basic_string_view<CharT> text) noexcept
{
    if (text.empty())
    {
        return 0;
    }
    if constexpr (sizeof(CharT) == 1)
    {
        const auto lead = static_cast<unsigned char>(text[0]);
        std::size_t expected = 1;
        if (lead >= 0xF8)
        {
            expected = 1;
        }
        else if (lead >= 0xF0)
        {
            expected = 4;
        }
        else if (lead >= 0xE0)
        {
            expected = 3;
        }
        else if (lead >= 0xC0)
        {
            expected = 2;
        }
        std::size_t length = 1;
        while (length < expected && length < text.size() && (static_cast<unsigned char>(text[length]) & 0xC0U) == 0x80U)
        {
            ++length;
        }
        return length;
    }
    else if constexpr (sizeof(CharT) == 2)
    {
        const auto lead = static_cast<std::uint32_t>(static_cast<Unit>(text[0]));
        const bool isLead = lead >= 0xD800 && lead <= 0xDBFF;
        if (!isLead || text.size() < 2)
        {
            return 1;
        }
        const auto trail = static_cast<std::uint32_t>(static_cast<Unit>(text[1]));
        return trail >= 0xDC00 && trail <= 0xDFFF ? 2 : 1;
    }
    else
    {
        return 1;
    }
}

template<typename CharT>
template<typename Found>
void PromptMatcher<CharT>::ForEachMatch(std::basic_string_view<CharT> text, const Found& found) const
{
    const auto size = text.size();
    std::size_t consumed = 0;
    for (std::size_t idx = 0; idx < size; ++idx)
    {
        // A text without a first unit of any pattern is this loop alone
        if (!m_first[static_cast<unsigned>(static_cast<Unit>(text[idx]) & 0xFFU)] || idx < consumed)
        {
            continue;
        }
        if (MatchesAt(text.data() + idx, size - idx, true))
        {
            found(idx);
            consumed = idx + CodePointLength(text.substr(idx));
        }
    }
}

template<typename CharT>
bool PromptMatcher<CharT>::StartsWithSuffix(std::basic_string_view<CharT> text) const
{
    // The prefix text[0, length) is read backwards along the reverse trie
    const auto longest = std::min(m_maxSuffix, text.size());
    for (std::size_t length = 1; length <= longest; ++length)
    {
        if (!m_last[static_cast<unsigned>(static_cast<Unit>(text[length - 1]) & 0xFFU)])
        {
            continue;
        }
        std::uint32_t node = 0;
        std::size_t idx = length;
        while (idx != 0 && node != None)
        {
            node = Child(m_reverse, node, Fold(static_cast<Unit>(text[--idx])));
        }
        if (node != None)
        {
            return true;
        }
    }
    return false;
}

template<typename CharT>
bool PromptMatcher<CharT>::EndsWithPrefix(std::basic_string_view<CharT> text) const
{
    if (m_maxPrefix < 2)
    {
        return false;
    }
    const auto size = text.size();
    for (std::size_t start = size > m_maxPrefix - 1 ? size - (m_maxPrefix - 1) : 0; start < size; ++start)
    {
        if (!m_first[static_cast<unsigned>(static_cast<Unit>(text[start]) & 0xFFU)])
        {
            continue;
        }
        std::uint32_t node = 0;
        for (std::size_t idx = start; idx != size && node != None; ++idx)
        {
            node = Child(m_forward, node, Fold(static_cast<Unit>(text[idx])));
        }
        // A node with children lies on a longer pattern: the text ends with a proper prefix of it
        if (node != None && !m_forward[node].next.empty())
        {
            return true;
        }
    }
    return false;
}

template class PromptMatcher<char>;
template class PromptMatcher<wchar_t>;

PromptEscaperImpl::PromptEscaperImpl(PromptEscapeOptions options)
    : m_options(std::move(options))
    , m_narrow(ToWidth<char>(m_options.specialTokens), ToWidth<char>(m_options.tags))
    , m_wide(ToWidth<wchar_t>(m_options.specialTokens), ToWidth<wchar_t>(m_options.tags))
    , m_narrowSeparator(m_options.separator)
    , m_wideSeparator(ToWidth<wchar_t>(m_options.separator))
{
}

template<typename CharT>
bool PromptEscaperImpl::Run(const PromptMatcher<CharT>& matcher, const std::basic_string<CharT>& separator, std::basic_string_view<CharT> text, std::basic_string<CharT>& out, bool checkLimit) const
{
    if (matcher.Empty() || text.empty())
    {
        return false;
    }
    std::uint64_t insertions = 0;
    matcher.ForEachMatch(text, [&insertions](std::size_t) { ++insertions; });
    const bool prepend = matcher.StartsWithSuffix(text);
    const bool append = matcher.EndsWithPrefix(text);
    insertions += (prepend ? 1U : 0U) + (append ? 1U : 0U);
    if (insertions == 0)
    {
        return false;
    }
    // One separator per match and edge: the size is known before the result is built
    const std::uint64_t unitsAdded = insertions * separator.size();
    const std::uint64_t total = text.size() + unitsAdded;
    if (checkLimit)
    {
        CheckSequenceSize(total, "a neutralized string");
    }
    else if (unitsAdded / separator.size() != insertions || total < unitsAdded || total > out.max_size())
    {
        throw std::length_error("the neutralized text is too long");
    }
    out.clear();
    out.reserve(static_cast<std::size_t>(total));
    if (prepend)
    {
        out.append(separator);
    }
    std::size_t copied = 0;
    matcher.ForEachMatch(text, [&](std::size_t idx) {
        const auto end = idx + PromptMatcher<CharT>::CodePointLength(text.substr(idx));
        out.append(text.data() + copied, end - copied);
        out.append(separator);
        copied = end;
    });
    out.append(text.data() + copied, text.size() - copied);
    if (append)
    {
        out.append(separator);
    }
    return true;
}

bool PromptEscaperImpl::Neutralize(std::string_view text, std::string& out, bool checkLimit) const
{
    return Run<char>(m_narrow, m_narrowSeparator, text, out, checkLimit);
}

bool PromptEscaperImpl::Neutralize(std::wstring_view text, std::wstring& out, bool checkLimit) const
{
    return Run<wchar_t>(m_wide, m_wideSeparator, text, out, checkLimit);
}

const PromptEscaperImpl* AutoescapePromptEscaper(const Settings& settings)
{
    if (settings.autoescapeMode != AutoescapeMode::Prompt)
    {
        return nullptr;
    }
    if (const auto* impl = PromptEscaperAccess::Impl(settings.promptEscaper))
    {
        return impl;
    }
    // Prompt mode without patterns still must not write HTML entities: it changes nothing
    static const PromptEscaperImpl empty{ PromptEscapeOptions{} };
    return &empty;
}

std::string NeutralizeString(const PromptEscaperImpl& escaper, std::string_view text)
{
    std::string out;
    if (!escaper.Neutralize(text, out, true))
    {
        return std::string(text);
    }
    return out;
}

std::wstring NeutralizeString(const PromptEscaperImpl& escaper, std::wstring_view text)
{
    std::wstring out;
    if (!escaper.Neutralize(text, out, true))
    {
        return std::wstring(text);
    }
    return out;
}

} // namespace detail

namespace
{
const PromptEscapeOptions& DefaultOptions()
{
    static const PromptEscapeOptions options;
    return options;
}
} // namespace

PromptEscaper::PromptEscaper(PromptEscapeOptions options)
{
    if (!IsValid(options))
    {
        throw std::invalid_argument("PromptEscapeOptions: the separator must be non-empty and share no character with a special token or tag pattern, no special may be a single character, all must be well-formed UTF-8, and no pattern may be longer than 256 bytes");
    }
    m_impl = std::make_shared<const detail::PromptEscaperImpl>(std::move(options));
}

namespace
{
// Whether text is well-formed UTF-8 (no overlong forms, surrogates or code points past U+10FFFF): the wide
// patterns and separator are converted from it, and a malformed sequence would become U+FFFD there only
bool IsUtf8(std::string_view text) noexcept
{
    for (std::size_t idx = 0; idx < text.size();)
    {
        const auto lead = static_cast<unsigned char>(text[idx]);
        std::size_t length = 0;
        std::uint32_t codePoint = 0;
        if (lead < 0x80U)
        {
            ++idx;
            continue;
        }
        if (lead >= 0xC2U && lead <= 0xDFU)
        {
            length = 2;
            codePoint = lead & 0x1FU;
        }
        else if (lead >= 0xE0U && lead <= 0xEFU)
        {
            length = 3;
            codePoint = lead & 0x0FU;
        }
        else if (lead >= 0xF0U && lead <= 0xF4U)
        {
            length = 4;
            codePoint = lead & 0x07U;
        }
        else
        {
            return false;
        }
        if (text.size() - idx < length)
        {
            return false;
        }
        for (std::size_t next = 1; next < length; ++next)
        {
            const auto unit = static_cast<unsigned char>(text[idx + next]);
            if ((unit & 0xC0U) != 0x80U)
            {
                return false;
            }
            codePoint = (codePoint << 6U) | (unit & 0x3FU);
        }
        const bool overlong = (length == 3 && codePoint < 0x800U) || (length == 4 && codePoint < 0x10000U);
        if (overlong || (codePoint >= 0xD800U && codePoint <= 0xDFFFU) || codePoint > 0x10FFFFU)
        {
            return false;
        }
        idx += length;
    }
    return true;
}

} // namespace

bool PromptEscaper::IsValid(const PromptEscapeOptions& options) noexcept
{
    const std::string_view separator = options.separator;
    if (separator.empty())
    {
        return false;
    }
    if (!IsUtf8(separator) || !std::all_of(options.specialTokens.begin(), options.specialTokens.end(), [](const std::string& p) { return IsUtf8(p); }) || !std::all_of(options.tags.begin(), options.tags.end(), [](const std::string& p) { return IsUtf8(p); }))
    {
        return false;
    }
    const auto tooLong = [](const std::string& pattern) { return pattern.size() > detail::MaxPatternSize; };
    if (std::any_of(options.specialTokens.begin(), options.specialTokens.end(), tooLong) || std::any_of(options.tags.begin(), options.tags.end(), tooLong))
    {
        return false;
    }
    // A separator after the first code point cannot break a special of one code point: nothing would remove it
    const auto oneCodePoint = [](const std::string& special) {
        return !special.empty() && detail::PromptMatcher<char>::CodePointLength(special) == special.size();
    };
    if (std::any_of(options.specialTokens.begin(), options.specialTokens.end(), oneCodePoint))
    {
        return false;
    }
    // Each code point of the separator on its own: a separator that shares one with a pattern could
    // complete that pattern with the text around an insertion ("ab" inside "xyz" next to the special "by")
    for (std::size_t start = 0; start < separator.size();)
    {
        const auto length = detail::PromptMatcher<char>::CodePointLength(separator.substr(start));
        const auto codePoint = separator.substr(start, length);
        start += length;
        const bool inSpecial = std::any_of(options.specialTokens.begin(), options.specialTokens.end(),
                                           [codePoint](const std::string& special) { return detail::Occurs(codePoint, { {}, special }); });
        const bool inTag = std::any_of(options.tags.begin(), options.tags.end(), [codePoint](const std::string& tag) {
            return !tag.empty() && (detail::Occurs(codePoint, { "<", tag }) || detail::Occurs(codePoint, { "</", tag }));
        });
        if (inSpecial || inTag)
        {
            return false;
        }
    }
    return true;
}

const PromptEscapeOptions& PromptEscaper::Options() const noexcept
{
    return m_impl ? m_impl->Options() : DefaultOptions();
}

std::string PromptEscaper::Neutralize(std::string_view text) const
{
    std::string out;
    if (!m_impl || !m_impl->Neutralize(text, out, false))
    {
        return std::string(text);
    }
    return out;
}

std::wstring PromptEscaper::Neutralize(std::wstring_view text) const
{
    std::wstring out;
    if (!m_impl || !m_impl->Neutralize(text, out, false))
    {
        return std::wstring(text);
    }
    return out;
}

} // namespace v2
} // namespace jinja2
