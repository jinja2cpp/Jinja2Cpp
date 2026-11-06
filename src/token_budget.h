#ifndef JINJA2CPP_SRC_TOKEN_BUDGET_H
#define JINJA2CPP_SRC_TOKEN_BUDGET_H

// The tokenizer behind count_tokens, truncate_tokens and pack (docs/tasks/0359, docs/cpp-only-filters.md)

#include <jinja2cpp/prompt.h>

#include <cstddef>
#include <string_view>
#include <utility>

namespace jinja2
{
inline namespace v2
{
namespace detail
{
class TokenizerImpl
{
public:
    TokenizerImpl(Tokenizer::CountFunction count, Tokenizer::PrefixFunction prefix)
        : m_count(std::move(count))
        , m_prefix(std::move(prefix))
    {
    }

    [[nodiscard]] const Tokenizer::CountFunction& CountFunction() const noexcept { return m_count; }
    [[nodiscard]] const Tokenizer::PrefixFunction& PrefixFunction() const noexcept { return m_prefix; }

private:
    Tokenizer::CountFunction m_count;
    Tokenizer::PrefixFunction m_prefix;
};

struct TokenizerAccess
{
    // Null for a tokenizer without a count function
    static const TokenizerImpl* Impl(const Tokenizer& tokenizer) noexcept { return tokenizer.m_impl.get(); }
};

// What a render asks a tokenizer. Text is in the template's character type; a wide text is converted to UTF-8 for the
// callbacks. A callback's exception leaves as a HostCallableFailure (ErrorCode::UserCallableException), and what a
// callback returns is clamped. Without a tokenizer the byte estimate answers
class TokenMeter
{
public:
    explicit TokenMeter(const Tokenizer& tokenizer) noexcept
        : m_impl(TokenizerAccess::Impl(tokenizer))
    {
    }

    // The tokens of text: 0 for an empty one, without a call
    template<typename CharT>
    [[nodiscard]] std::size_t Count(std::basic_string_view<CharT> text) const;

    // The code units of the longest prefix of text that is at most maxTokens tokens, on a code point boundary. text is
    // taken not to fit in maxTokens already (a search starts from there)
    template<typename CharT>
    [[nodiscard]] std::size_t PrefixSize(std::basic_string_view<CharT> text, std::size_t maxTokens) const;

private:
    [[nodiscard]] std::size_t CountUtf8(std::string_view text) const;
    [[nodiscard]] std::size_t PrefixUtf8(std::string_view text, std::size_t maxTokens) const;

    const TokenizerImpl* m_impl;
};

} // namespace detail
} // namespace v2
} // namespace jinja2

#endif // JINJA2CPP_SRC_TOKEN_BUDGET_H
