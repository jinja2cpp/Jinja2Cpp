#include "decimal_number.h"

#include <fast_float/fast_float.h> // IWYU pragma: keep (declares from_chars with its default template arguments)
#include <fast_float/float_common.h>
#include <fast_float/parse_number.h>

#include <fmt/format.h>

#include <cmath>
#include <cstddef>
#include <optional>
#include <string_view>
#include <system_error>

namespace jinja2
{
inline namespace v2
{

std::optional<double> ParseDecimal(std::string_view text)
{
    const char* const begin = text.data();
    const char* const end = begin + text.size();
    double result = 0;
    // fast_float rounds correctly and sets inf or 0 when it reports the value out of range
    // The analyzer reports these inside fast_float: its flag enum combined with operator|
    // and its power-of-five table indexed by a range-checked exponent
    // NOLINTBEGIN(clang-analyzer-security.ArrayBound,clang-analyzer-optin.core.EnumCastOutOfRange)
    const auto parsed = fast_float::from_chars(begin, end, result,
                                               fast_float::chars_format::general | fast_float::chars_format::allow_leading_plus | fast_float::chars_format::no_infnan);
    // NOLINTEND(clang-analyzer-security.ArrayBound,clang-analyzer-optin.core.EnumCastOutOfRange)
    if (parsed.ptr != end || (parsed.ec != std::errc() && parsed.ec != std::errc::result_out_of_range))
    {
        return std::nullopt;
    }
    return result;
}

double FloatAsDecimalDouble(float val)
{
    const auto widened = static_cast<double>(val);
    if (!std::isfinite(val))
    {
        return widened;
    }
    // A float's longest decimal, "-1.17549435e-38", is 15 characters
    char buffer[32];
    auto nearest = [&buffer, val](const char* end) -> std::optional<double> {
        // The analyzer follows ParseDecimal into fast_float and reports its flag enum, as there
        // NOLINTBEGIN(clang-analyzer-security.ArrayBound,clang-analyzer-optin.core.EnumCastOutOfRange)
        const auto decimal = ParseDecimal(std::string_view(buffer, static_cast<size_t>(end - buffer)));
        // NOLINTEND(clang-analyzer-security.ArrayBound,clang-analyzer-optin.core.EnumCastOutOfRange)
        return decimal && static_cast<float>(*decimal) == val ? decimal : std::nullopt;
    };
    if (auto decimal = nearest(fmt::format_to(buffer, "{}", val)))
    {
        return *decimal;
    }
    // The double nearest the shortest decimal can round to the neighbouring float: 2 of all
    // 2^32, +-7.038531e-26, checked exhaustively. One more digit, 7.0385307e-26, round-trips
    for (int precision = 7; precision <= 8; ++precision)
    {
        if (auto decimal = nearest(fmt::format_to(buffer, "{:.{}e}", val, precision)))
        {
            return *decimal;
        }
    }
    return widened;
}

} // namespace v2
} // namespace jinja2
