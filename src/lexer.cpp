#include "lexer.h"

#include "big_int.h"
#include "internal_value.h"

#include <algorithm>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <string>
#include <utility>

namespace jinja2
{
inline namespace v2
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
        // Wider than int64_t: a big integer (docs/tasks/0270)
        return IntegerFromDigits(digits, radix, false);
    }

    const auto value = std::strtoll(digits, &end, 10);
    if (errno != ERANGE && *end == '\0')
    {
        return InternalValue(static_cast<int64_t>(value));
    }
    if (number.find_first_of(".eE") == std::string::npos)
    {
        // An integer literal beyond int64: a big integer, as Python's int() makes it
        return IntegerFromDigits(number, 10, false);
    }

    return InternalValue(std::strtod(digits, nullptr));
}

} // namespace v2
} // namespace jinja2
