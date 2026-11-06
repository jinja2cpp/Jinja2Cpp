#ifndef JINJA2CPP_SRC_PROMPT_ESCAPE_H
#define JINJA2CPP_SRC_PROMPT_ESCAPE_H

// The matcher behind PromptEscaper and the neutralize filter (docs/tasks/0358, docs/cpp-only-filters.md)

#include <jinja2cpp/prompt.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace jinja2
{
inline namespace v2
{
namespace detail
{
// The longest special token or tag name, in bytes: the edge checks cost the square of it per call
constexpr std::size_t MaxPatternSize = 256;


// The patterns of one character width as tries over code units. Specials and tags (`<t`, `</t`) are in the forward
// trie, found at a position by walking it; the specials reversed, without their first unit, are in the reverse trie,
// whose paths are the proper suffixes of the specials. Letters are folded to lower case in both, so a match ignores
// the case of ASCII letters
template<typename CharT>
class PromptMatcher
{
public:
    using Unit = std::make_unsigned_t<CharT>;

    PromptMatcher() = default;
    PromptMatcher(const std::vector<std::basic_string<CharT>>& specials, const std::vector<std::basic_string<CharT>>& tags);

    [[nodiscard]] bool Empty() const noexcept { return m_forward.size() == 1; }

    // Calls found(position) for each position where a pattern starts, in order, skipping those inside the first code
    // point of the previous one (only text that is not valid has them). A tag pattern needs a boundary after it
    template<typename Found>
    void ForEachMatch(std::basic_string_view<CharT> text, const Found& found) const;
    // Whether text starts with a non-empty proper suffix of a special
    [[nodiscard]] bool StartsWithSuffix(std::basic_string_view<CharT> text) const;
    // Whether text ends with a non-empty proper prefix of a special or a tag pattern
    [[nodiscard]] bool EndsWithPrefix(std::basic_string_view<CharT> text) const;

    // The length in code units of the code point at text[0]; a sequence that is not valid counts what is there
    static std::size_t CodePointLength(std::basic_string_view<CharT> text) noexcept;

private:
    static constexpr std::uint32_t None = 0xFFFFFFFFU;
    static constexpr std::uint8_t SpecialEnd = 1; // a special token ends here
    static constexpr std::uint8_t TagEnd = 2;     // `<t` or `</t` ends here: a match when a tag boundary follows

    struct Node
    {
        std::vector<std::pair<Unit, std::uint32_t>> next; // sorted by unit
        std::uint8_t flags = 0;
    };
    using Trie = std::vector<Node>;

    static Unit Fold(Unit unit) noexcept { return unit >= 'A' && unit <= 'Z' ? static_cast<Unit>(unit + ('a' - 'A')) : unit; }
    static bool IsBoundary(Unit unit) noexcept { return unit == '>' || unit == '/' || unit == ' ' || unit == '\t' || unit == '\r' || unit == '\n'; }
    static std::uint32_t Child(const Trie& trie, std::uint32_t node, Unit unit) noexcept;
    static void Insert(Trie& trie, const std::vector<Unit>& units, std::uint8_t flags);
    // Whether a pattern starts at text; with tagsFollowInsertions, a tag name followed by the separator a match
    // inside it gets counts as followed by a boundary
    [[nodiscard]] bool MatchesAt(const CharT* text, std::size_t remaining, bool tagsFollowInsertions) const noexcept;
    [[nodiscard]] bool InsertsAfterLastCodePoint(const CharT* text, std::size_t tagLength, std::size_t remaining) const noexcept;

    Trie m_forward = Trie(1);
    Trie m_reverse = Trie(1);
    // By the low 8 bits of a unit, both cases of a letter: whether a pattern can start with it, and whether a special
    // can end with it. A text without one is a single scan
    bool m_first[256] = {};
    bool m_last[256] = {};
    std::size_t m_maxPrefix = 0; // the longest pattern
    std::size_t m_maxSuffix = 0; // the longest special, less its first unit
};

// What a PromptEscaper shares between copies: the options, and the matchers and separators of both widths
class PromptEscaperImpl
{
public:
    // The options must be valid (PromptEscaper::IsValid)
    explicit PromptEscaperImpl(PromptEscapeOptions options);

    [[nodiscard]] const PromptEscapeOptions& Options() const noexcept { return m_options; }
    // Whether there is no pattern: the text is never changed
    [[nodiscard]] bool Empty() const noexcept { return m_narrow.Empty(); }

    // Writes the neutralized text into out and returns true, or returns false when the text is unchanged (out is not
    // written). checkLimit checks the size against the render's maxSequenceSize first (inside a render)
    bool Neutralize(std::string_view text, std::string& out, bool checkLimit) const;
    bool Neutralize(std::wstring_view text, std::wstring& out, bool checkLimit) const;

private:
    template<typename CharT>
    bool Run(const PromptMatcher<CharT>& matcher, const std::basic_string<CharT>& separator, std::basic_string_view<CharT> text, std::basic_string<CharT>& out, bool checkLimit) const;

    PromptEscapeOptions m_options;
    PromptMatcher<char> m_narrow;
    PromptMatcher<wchar_t> m_wide;
    std::string m_narrowSeparator;
    std::wstring m_wideSeparator;
};

struct PromptEscaperAccess
{
    // Null for an escaper that was not made from options
    static const PromptEscaperImpl* Impl(const PromptEscaper& escaper) noexcept { return escaper.m_impl.get(); }
};

} // namespace detail
} // namespace v2
} // namespace jinja2

#endif // JINJA2CPP_SRC_PROMPT_ESCAPE_H
