#include "lexer.h"

#include "internal_value.h"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <string>
#include <utility>

namespace jinja2
{

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

} // namespace

InternalValue ParseNumberLiteral(std::string number)
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

} // namespace jinja2
