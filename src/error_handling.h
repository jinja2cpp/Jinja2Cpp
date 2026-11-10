#ifndef JINJA2CPP_SRC_ERROR_HANDLING_H
#define JINJA2CPP_SRC_ERROR_HANDLING_H

#include "lexer.h"
#include "make_unexpected.h"
#include <jinja2cpp/error_info.h>
#include <nonstd/expected.hpp>

#include <initializer_list>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace jinja2
{
inline namespace v2
{

struct ParseError
{
    ParseError() = default;
    ParseError(ErrorCode code, Token tok)
        : errorCode(code)
        , errorToken(std::move(tok))
    {}

    ParseError(ErrorCode code, Token tok, std::initializer_list<Token> toks)
        : errorCode(code)
        , errorToken(std::move(tok))
        , relatedTokens(toks)
    {}

    ParseError(ErrorCode code, Token tok, std::string text)
        : errorCode(code)
        , errorToken(tok)
        , message(std::make_shared<const std::string>(std::move(text)))
    {}
    // Out of line (expression_parser.cpp), so that destroying or moving an expected<T, ParseError>
    // inlines to a test of its flag on the success path instead of a call (0152)
    ParseError(const ParseError&);
    ParseError(ParseError&& other) noexcept;
    ParseError& operator=(const ParseError&);
    ParseError& operator=(ParseError&& error) noexcept;
    ~ParseError();

    ErrorCode errorCode{ ErrorCode::Unspecified };
    Token errorToken;
    std::vector<Token> relatedTokens;
    // Jinja2's message of a construct it rejects with one of its own: the whole of a TemplateSyntaxError
    // and TemplateAssertionError, ExtraParams[1] of an UnexpectedStatement
    // Shared and behind a pointer, so that the result of every parser step stays small
    std::shared_ptr<const std::string> message;
};

inline auto MakeParseError(ErrorCode code, Token tok)
{
    return MakeUnexpected(ParseError{ code, std::move(tok) });
}

inline auto MakeParseErrorMsg(ErrorCode code, Token tok, std::string message)
{
    return MakeUnexpected(ParseError{ code, tok, std::move(message) });
}

inline auto MakeParseError(ErrorCode code, Token tok, std::initializer_list<Token> toks)
{
    return MakeUnexpected(ParseError{ code, std::move(tok), toks });
}

} // namespace v2
} // namespace jinja2
#endif // JINJA2CPP_SRC_ERROR_HANDLING_H
