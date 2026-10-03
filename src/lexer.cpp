#include "lexer.h"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <limits>

namespace jinja2
{

bool Lexer::Preprocess()
{
    bool result = true;
    while (true)
    {
        lexertk::token token = m_tokenizer();
        if (token.is_error())
        {
            result = false;
            break;
        }

        Token newToken;
        newToken.range.startOffset = token.position;
        newToken.range.endOffset = newToken.range.startOffset + token.length;

        if (token.type == lexertk::token::e_eof)
        {
            newToken.type = Token::Eof;
            m_tokens.push_back(std::move(newToken));
            break;
        }

        switch (token.type)
        {
        case lexertk::token::e_number:
            result = ProcessNumber(token, newToken);
            break;
        case lexertk::token::e_symbol:
            result = ProcessSymbolOrKeyword(token, newToken);
            break;
        case lexertk::token::e_string:
            result = ProcessString(token, newToken);
            break;
        case lexertk::token::e_lte:
            newToken.type = Token::LessEqual;
            break;
        case lexertk::token::e_ne:
            newToken.type = Token::NotEqual;
            break;
        case lexertk::token::e_gte:
            newToken.type = Token::GreaterEqual;
            break;
        case lexertk::token::e_eq:
            newToken.type = Token::Equal;
            break;
        case lexertk::token::e_mulmul:
            newToken.type = Token::MulMul;
            break;
        case lexertk::token::e_divdiv:
            newToken.type = Token::DivDiv;
            break;
        default:
            newToken.type = static_cast<Token::Type>(token.type);
            break;
        }

        if (result)
        {
            m_tokens.push_back(std::move(newToken));
        }
        else
        {
            break;
        }
    }

    return result;
}

namespace
{
int GetRadix(const std::string& number)
{
    if (number.size() < 2 || number[0] != '0')
    {
        return 10;
    }

    switch (number[1])
    {
    case 'x':
    case 'X':
        return 16;
    case 'o':
    case 'O':
        return 8;
    case 'b':
    case 'B':
        return 2;
    default:
        return 10;
    }
}

InternalValue ParseNumber(std::string number)
{
    // The tokenizer has checked the syntax; only the digit separators have to go
    number.erase(std::remove(number.begin(), number.end(), '_'), number.end());

    const int radix = GetRadix(number);
    const char* digits = number.c_str() + (radix == 10 ? 0 : 2);
    char* end = nullptr;

    errno = 0;
    if (radix != 10)
    {
        const auto value = std::strtoull(digits, &end, radix);
        if (errno != ERANGE && value <= static_cast<unsigned long long>(std::numeric_limits<int64_t>::max()))
        {
            return InternalValue(static_cast<int64_t>(value));
        }

        // Wider than int64_t: degrade to double like a decimal literal does
        double result = 0;
        for (const char* ch = digits; *ch; ++ch)
        {
            result = (result * radix) + (std::isdigit(static_cast<unsigned char>(*ch)) ? *ch - '0' : std::tolower(static_cast<unsigned char>(*ch)) - 'a' + 10);
        }
        return InternalValue(result);
    }

    const auto value = std::strtoll(digits, &end, 10);
    if (errno != ERANGE && *end == '\0')
    {
        return InternalValue(static_cast<int64_t>(value));
    }

    return InternalValue(std::strtod(digits, nullptr));
}
} // namespace

bool Lexer::ProcessNumber(const lexertk::token&, Token& newToken)
{
    newToken.type = Token::FloatNum;
    newToken.value = ParseNumber(m_helper->GetAsString(newToken.range));
    return true;
}

bool Lexer::ProcessSymbolOrKeyword(const lexertk::token&, Token& newToken)
{
    Keyword kwType = m_helper->GetKeyword(newToken.range);
    Token::Type tokType = Token::Unknown;

    switch (kwType)
    {
    case Keyword::None:
        tokType = Token::None;
        break;
    case Keyword::True:
        tokType = Token::True;
        break;
    case Keyword::False:
        tokType = Token::False;
        break;
    default:
        tokType = Token::Unknown;
        break;
    }

    if (tokType == Token::Unknown)
    {
        newToken.type = Token::Identifier;
        auto id = m_helper->GetAsString(newToken.range);
        newToken.value = InternalValue(id);
    }
    else
    {
        newToken.type = tokType;
    }
    return true;
}

bool Lexer::ProcessString(const lexertk::token&, Token& newToken)
{
    newToken.type = Token::String;
    newToken.value = m_helper->GetAsValue(newToken.range, newToken.type);
    return true;
}

} // namespace jinja2
