#ifndef JINJA2CPP_SRC_ERROR_HANDLING_H
#define JINJA2CPP_SRC_ERROR_HANDLING_H

#include "lexer.h"
#include "make_unexpected.h"
#include <jinja2cpp/error_info.h>
#include <nonstd/expected.hpp>

#include <initializer_list>
#include <utility>
#include <vector>

namespace jinja2
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
};

inline auto MakeParseError(ErrorCode code, Token tok)
{
    return MakeUnexpected(ParseError{ code, std::move(tok) });
}

inline auto MakeParseError(ErrorCode code, Token tok, std::initializer_list<Token> toks)
{
    return MakeUnexpected(ParseError{ code, std::move(tok), toks });
}

} // namespace jinja2
#endif // JINJA2CPP_SRC_ERROR_HANDLING_H
