#include "helpers.h"

#ifndef JINJA2CPP_NO_UNICODE_NAMES
#include "unicode_names.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <string>
#include <vector>
#endif

#include <optional>
#include <string_view>

namespace jinja2
{
inline namespace v2
{
namespace detail
{
#ifdef JINJA2CPP_NO_UNICODE_NAMES

std::optional<char32_t> LookupUnicodeName(std::string_view /*name*/)
{
    return std::nullopt;
}

#else

namespace
{
using unicode_names::Range;

bool InRanges(const Range* first, const Range* last, std::uint32_t cp)
{
    return std::any_of(first, last, [cp](const Range& range) { return cp >= range.first && cp <= range.last; });
}

// The value of 4 or 5 upper-case hex digits, as CPython reads the code point of a computed name
std::optional<std::uint32_t> ParseUpperHex(std::string_view digits)
{
    if (digits.size() != 4 && digits.size() != 5)
    {
        return std::nullopt;
    }
    std::uint32_t cp = 0;
    for (char ch : digits)
    {
        std::uint32_t digit = 0;
        if (ch >= '0' && ch <= '9')
        {
            digit = static_cast<std::uint32_t>(ch - '0');
        }
        else if (ch >= 'A' && ch <= 'F')
        {
            digit = static_cast<std::uint32_t>(ch - 'A') + 10;
        }
        else
        {
            return std::nullopt;
        }
        cp = (cp << 4) | digit;
    }
    return cp;
}

// The short jamo names of a Hangul syllable's lead consonant, vowel and trailing consonant
constexpr std::array<std::string_view, 19> HangulLeads = { "G", "GG", "N", "D", "DD", "R", "M", "B", "BB", "S", "SS", "", "J", "JJ", "C", "K", "T", "P", "H" };
constexpr std::array<std::string_view, 21> HangulVowels = { "A", "AE", "YA", "YAE", "EO", "E", "YEO", "YE", "O", "WA", "WAE", "OE", "YO", "U", "WEO", "WE", "WI", "YU", "EU", "YI", "I" };
constexpr std::array<std::string_view, 28> HangulTrails = { "", "G", "GG", "GS", "N", "NJ", "NH", "D", "L", "LG", "LM", "LB", "LS", "LT", "LP", "LH", "M", "B", "BS", "S", "SS", "NG", "J", "C", "K", "T", "P", "H" };

// CPython's find_syllable: the longest jamo name at the start of rest, which it skips
template<std::size_t N>
std::optional<std::size_t> FindJamo(std::string_view& rest, const std::array<std::string_view, N>& names)
{
    std::optional<std::size_t> found;
    std::size_t length = 0;
    for (std::size_t idx = 0; idx < N; ++idx)
    {
        const auto jamo = names[idx];
        if ((!found || jamo.size() > length) && rest.substr(0, jamo.size()) == jamo)
        {
            found = idx;
            length = jamo.size();
        }
    }
    rest.remove_prefix(length);
    return found;
}

std::optional<char32_t> LookupHangul(std::string_view rest)
{
    const auto lead = FindJamo(rest, HangulLeads);
    const auto vowel = FindJamo(rest, HangulVowels);
    const auto trail = FindJamo(rest, HangulTrails);
    if (!lead || !vowel || !trail || !rest.empty())
    {
        return std::nullopt;
    }
    return static_cast<char32_t>(0xAC00 + (((*lead * HangulVowels.size()) + *vowel) * HangulTrails.size()) + *trail);
}

std::optional<char32_t> LookupCjkUnified(std::string_view digits)
{
    const auto cp = ParseUpperHex(digits);
    if (!cp || !InRanges(std::begin(unicode_names::CjkUnifiedRanges), std::end(unicode_names::CjkUnifiedRanges), *cp))
    {
        return std::nullopt;
    }
    return static_cast<char32_t>(*cp);
}

// A stored "<prefix><code point>" name, spelled as the name is: 4 hex digits below U+10000, else 5
std::optional<char32_t> LookupHexNamed(std::string_view digits, const Range* first, const Range* last)
{
    const auto cp = ParseUpperHex(digits);
    if (!cp || !InRanges(first, last, *cp) || digits.size() != (*cp > 0xFFFF ? 5U : 4U))
    {
        return std::nullopt;
    }
    return static_cast<char32_t>(*cp);
}

// Words and Names hold entries in byte order with an index of every BlockSize-th one: the
// entry equal to key, found by a binary search of the index and a walk of one block
template<std::size_t indexSize, std::size_t dataSize, typename Compare>
const std::uint8_t* FindEntry(const std::uint32_t (&index)[indexSize], const std::uint8_t (&data)[dataSize], const Compare& compare)
{
    // The first block whose first entry is above key; the entry is in the block before it
    const auto* block = std::upper_bound(std::begin(index), std::end(index), 0, [&data, &compare](int, std::uint32_t offset) { return compare(data + offset) < 0; });
    if (block == std::begin(index))
    {
        return nullptr;
    }
    const auto* entry = data + *(block - 1);
    const auto* end = block == std::end(index) ? std::end(data) : data + *block;
    while (entry != end)
    {
        const int order = compare(entry);
        if (order == 0)
        {
            return entry;
        }
        if (order < 0)
        {
            return nullptr;
        }
        entry = compare.Next(entry);
    }
    return nullptr;
}

// Orders a word of Words against the word looked up
struct WordOrder
{
    std::string_view word;

    // Below zero when the entry comes after the word
    int operator()(const std::uint8_t* entry) const
    {
        const std::string_view text(reinterpret_cast<const char*>(entry + 1), *entry);
        return word.compare(text);
    }
    static const std::uint8_t* Next(const std::uint8_t* entry) { return entry + 1 + *entry + 2; }
};

// Orders a name of Names, by its codes, against the codes looked up
struct NameOrder
{
    const std::vector<std::uint8_t>& key;

    static const std::uint8_t* CodesEnd(const std::uint8_t* codes)
    {
        // A two-byte code's low byte can equal EndOfName
        while (*codes != unicode_names::EndOfName)
        {
            codes += *codes < unicode_names::OneByteWords ? 1 : 2;
        }
        return codes + 1;
    }
    int operator()(const std::uint8_t* entry) const
    {
        const auto* codes = entry + 3;
        const auto* end = CodesEnd(codes);
        if (std::lexicographical_compare(key.begin(), key.end(), codes, end))
        {
            return -1;
        }
        return std::equal(key.begin(), key.end(), codes, end) ? 0 : 1;
    }
    static const std::uint8_t* Next(const std::uint8_t* entry) { return CodesEnd(entry + 3); }
};

// The code of word, or nothing
std::optional<unsigned> FindWord(std::string_view word)
{
    const auto* entry = FindEntry(unicode_names::WordIndex, unicode_names::Words, WordOrder{ word });
    if (!entry)
    {
        return std::nullopt;
    }
    const auto* code = entry + 1 + *entry;
    return (static_cast<unsigned>(code[0]) << 8) | code[1];
}

// A stored name, by the codes of its words
std::optional<char32_t> LookupStored(std::string_view name)
{
    std::vector<std::uint8_t> key;
    while (true)
    {
        const auto space = name.find(' ');
        const auto code = FindWord(name.substr(0, space));
        if (!code)
        {
            return std::nullopt;
        }
        if (*code < unicode_names::OneByteWords)
        {
            key.push_back(static_cast<std::uint8_t>(*code));
        }
        else
        {
            key.push_back(static_cast<std::uint8_t>(unicode_names::OneByteWords + (*code >> 8)));
            key.push_back(static_cast<std::uint8_t>(*code & 0xFF));
        }
        if (space == std::string_view::npos)
        {
            break;
        }
        name.remove_prefix(space + 1);
    }
    key.push_back(unicode_names::EndOfName);

    const auto* entry = FindEntry(unicode_names::NameIndex, unicode_names::Names, NameOrder{ key });
    if (!entry)
    {
        return std::nullopt;
    }
    return static_cast<char32_t>((static_cast<std::uint32_t>(entry[0]) << 16) | (static_cast<std::uint32_t>(entry[1]) << 8) | entry[2]);
}
} // namespace

std::optional<char32_t> LookupUnicodeName(std::string_view name)
{
    // CPython computes these two families and matches them case sensitively
    constexpr std::string_view cjkUnified = "CJK UNIFIED IDEOGRAPH-";
    constexpr std::string_view hangul = "HANGUL SYLLABLE ";
    if (name.substr(0, hangul.size()) == hangul)
    {
        return LookupHangul(name.substr(hangul.size()));
    }
    if (name.substr(0, cjkUnified.size()) == cjkUnified)
    {
        return LookupCjkUnified(name.substr(cjkUnified.size()));
    }

    // Stored names and aliases ignore ASCII case
    if (name.size() > unicode_names::MaxNameLength)
    {
        return std::nullopt;
    }
    std::string upper(name);
    for (auto& ch : upper)
    {
        if (ch >= 'a' && ch <= 'z')
        {
            ch = static_cast<char>(ch - 'a' + 'A');
        }
        else if ((ch < 'A' || ch > 'Z') && (ch < '0' || ch > '9') && ch != ' ' && ch != '-')
        {
            return std::nullopt;
        }
    }
    const std::string_view key = upper;
    if (key.empty())
    {
        return std::nullopt;
    }

    struct HexFamily
    {
        std::string_view prefix;
        const Range* first;
        const Range* last;
    };
    const HexFamily hexFamilies[] = {
        { "CJK COMPATIBILITY IDEOGRAPH-", std::begin(unicode_names::CjkCompatibilityRanges), std::end(unicode_names::CjkCompatibilityRanges) },
        { "NUSHU CHARACTER-", std::begin(unicode_names::NushuRanges), std::end(unicode_names::NushuRanges) },
        { "KHITAN SMALL SCRIPT CHARACTER-", std::begin(unicode_names::KhitanRanges), std::end(unicode_names::KhitanRanges) },
    };
    for (const auto& family : hexFamilies)
    {
        if (key.substr(0, family.prefix.size()) == family.prefix)
        {
            return LookupHexNamed(key.substr(family.prefix.size()), family.first, family.last);
        }
    }

    for (const auto& alias : unicode_names::Aliases)
    {
        if (key == alias.name)
        {
            return static_cast<char32_t>(alias.cp);
        }
    }
    return LookupStored(key);
}

#endif
} // namespace detail
} // namespace v2
} // namespace jinja2
