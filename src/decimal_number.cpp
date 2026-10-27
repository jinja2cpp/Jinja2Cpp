#include "decimal_number.h"

#include <fast_float/fast_float.h> // IWYU pragma: keep (declares from_chars with its default template arguments)
#include <fast_float/float_common.h>
#include <fast_float/parse_number.h>

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

} // namespace v2
} // namespace jinja2
