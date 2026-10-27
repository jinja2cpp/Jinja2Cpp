#include "decimal_number.h"

#include <fast_float/fast_float.h>

#include <optional>
#include <string_view>
#include <system_error>

namespace jinja2
{
inline namespace v2
{

std::optional<double> ParseDecimal(std::string_view text)
{
    const char* const end = text.data() + text.size();
    double result = 0;
    // fast_float rounds correctly and sets inf or 0 when it reports the value out of range
    const auto parsed = fast_float::from_chars(text.data(), end, result,
                                               fast_float::chars_format::general | fast_float::chars_format::allow_leading_plus | fast_float::chars_format::no_infnan);
    if (parsed.ptr != end || (parsed.ec != std::errc() && parsed.ec != std::errc::result_out_of_range))
    {
        return std::nullopt;
    }
    return result;
}

} // namespace v2
} // namespace jinja2
