#ifndef JINJA2CPP_PROMPT_H
#define JINJA2CPP_PROMPT_H

#include "config.h"

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

JINJA2CPP_SUPPRESS_DLL_INTERFACE_WARNINGS_BEGIN

namespace jinja2
{
inline namespace v2
{
namespace detail
{
class PromptEscaperImpl;
struct PromptEscaperAccess;
class TokenizerImpl;
struct TokenizerAccess;
} // namespace detail

//! What autoescape escapes for (\ref Settings::autoescapeMode). Jinja2 has Html only
enum class AutoescapeMode : unsigned char
{
    //! HTML entities, as Jinja2 and MarkupSafe write them
    Html,
    //! The special tokens and tags of \ref Settings::promptEscaper are broken, as the `neutralize` filter does; Markup
    //! means prompt-safe, and the template's string literals are prompt-safe (docs/cpp-only-filters.md, "Prompt
    //! escaping mode")
    Prompt
};

/*!
 * \brief The strings \ref PromptEscaper breaks (docs/cpp-only-filters.md, "Prompt filters")
 *
 * Templates that build prompts for a language model put untrusted text into the channel that carries the
 * instructions. What breaks out is a string the tokenizer or the model trusts: a special token such as
 * `<|im_start|>` or `[INST]`, or a closing tag. The library keeps no list of a model's tokens; the host reads them from
 * its tokenizer and registers them here.
 */
struct PromptEscapeOptions
{
    //! Special tokens in UTF-8, matched exactly and ASCII case-insensitively. Empty ones are ignored
    std::vector<std::string> specialTokens;
    //! Tag names: `<name` and `</name` followed by `>`, `/`, a space, a tab, CR, LF or the end of the text are broken,
    //! ASCII case-insensitively. Empty ones are ignored
    std::vector<std::string> tags;
    //! Inserted after the first code point of every match to break it. Must be non-empty and occur in no pattern (case
    //! ignored for ASCII letters). The default, a space, survives every text normalizer; an invisible character such as
    //! U+2060 does not, and puts the token back together where a pipeline deletes it
    std::string separator = " ";
};

/*!
 * \brief The compiled patterns of \ref PromptEscapeOptions, for narrow (UTF-8) and wide text. Immutable
 *
 * Copies share the compiled patterns and compare equal; escapers made apart compare unequal, whatever they match.
 * The default escaper has no patterns and changes nothing. Set one in \ref Settings::promptEscaper for the `neutralize`
 * and `fence` filters, or call \ref Neutralize from host code:
 *
 * ```c++
 *  env.GetSettings().promptEscaper = jinja2::PromptEscaper({ { "<|im_start|>", "<|im_end|>" }, { "document" } });
 * ```
 *
 * The output of \ref Neutralize is specified in docs/cpp-only-filters.md. It adds one separator per match start plus at
 * most two for the edges of the text.
 */
class JINJA2CPP_EXPORT PromptEscaper
{
public:
    //! No patterns
    PromptEscaper() = default;
    //! Throws std::invalid_argument when \ref IsValid is false
    explicit PromptEscaper(PromptEscapeOptions options);

    //! Whether options can make an escaper: a non-empty separator that occurs in no pattern
    [[nodiscard]] static bool IsValid(const PromptEscapeOptions& options) noexcept;
    //! What the escaper was made with (empty options with the default separator for a default escaper)
    [[nodiscard]] const PromptEscapeOptions& Options() const noexcept;

    //! The `neutralize` filter for host code: the text with every match broken by the separator
    [[nodiscard]] std::string Neutralize(std::string_view text) const;
    [[nodiscard]] std::wstring Neutralize(std::wstring_view text) const;

    //! Whether both share their compiled patterns
    friend bool operator==(const PromptEscaper& lhs, const PromptEscaper& rhs) noexcept { return lhs.m_impl == rhs.m_impl; }
    friend bool operator!=(const PromptEscaper& lhs, const PromptEscaper& rhs) noexcept { return lhs.m_impl != rhs.m_impl; }

private:
    friend struct detail::PromptEscaperAccess;

    std::shared_ptr<const detail::PromptEscaperImpl> m_impl;
};

/*!
 * \brief The host's token counter for the `count_tokens`, `truncate_tokens` and `pack` filters (docs/cpp-only-filters.md,
 * "Prompt filters"). Immutable
 *
 * A tokenizer is model specific and large, so the library bundles none: the host wraps its own. Copies share the
 * callbacks and compare equal; tokenizers made apart compare unequal. The default tokenizer has no callback and
 * estimates `ceil(utf8_bytes / 4)` tokens, which can undercount CJK text, digits and code: a hard context limit needs a
 * real one.
 *
 * ```c++
 *  env.GetSettings().tokenizer = jinja2::Tokenizer([&tok](std::string_view text) { return tok.Count(text); });
 * ```
 *
 * Callbacks take UTF-8 (a wide template converts its text first) and may be called from renders on several threads at
 * once: making them thread-safe is the host's job. An exception from a callback fails the render with
 * ErrorCode::UserCallableException and is kept in ErrorInfo::GetException. Results are not trusted: a prefix length past
 * the end of the text is clamped, and one inside a UTF-8 sequence is rounded down to its start.
 */
class JINJA2CPP_EXPORT Tokenizer
{
public:
    //! The tokens of UTF-8 `text`, without the special tokens a tokenizer adds around a sequence
    using CountFunction = std::function<std::size_t(std::string_view text)>;
    //! The bytes of the longest prefix of UTF-8 `text` that is at most `maxTokens` tokens and ends on a token boundary
    using PrefixFunction = std::function<std::size_t(std::string_view text, std::size_t maxTokens)>;

    //! The byte estimate
    Tokenizer() = default;
    /*!
     * \param count Required. An empty one makes the byte estimate, as the default tokenizer
     * \param prefix Optional: what `truncate_tokens` cuts with. Without it the filter searches the length with `count`.
     * Throws std::invalid_argument when given without `count`
     */
    explicit Tokenizer(CountFunction count, PrefixFunction prefix = {});

    //! Whether a count function is set
    explicit operator bool() const noexcept { return static_cast<bool>(m_impl); }
    //! The tokens of UTF-8 `text`: the estimate when unset, 0 for an empty text without a call. A callback's exception
    //! is thrown as it is
    [[nodiscard]] std::size_t Count(std::string_view text) const;

    //! Whether both share their callbacks
    friend bool operator==(const Tokenizer& lhs, const Tokenizer& rhs) noexcept { return lhs.m_impl == rhs.m_impl; }
    friend bool operator!=(const Tokenizer& lhs, const Tokenizer& rhs) noexcept { return lhs.m_impl != rhs.m_impl; }

private:
    friend struct detail::TokenizerAccess;

    std::shared_ptr<const detail::TokenizerImpl> m_impl;
};

} // namespace v2
} // namespace jinja2

JINJA2CPP_SUPPRESS_DLL_INTERFACE_WARNINGS_END

#endif // JINJA2CPP_PROMPT_H
