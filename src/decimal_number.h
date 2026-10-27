#ifndef JINJA2CPP_SRC_DECIMAL_NUMBER_H
#define JINJA2CPP_SRC_DECIMAL_NUMBER_H

#include <optional>
#include <string_view>

namespace jinja2
{
inline namespace v2
{

// The double nearest the decimal number that is all of `text`: an optional sign, digits with
// an optional '.' ("1.5", ".5", "5.") and an optional exponent, '.' being the decimal point
// whatever the C locale (docs/tasks/0333). An exponent out of range gives inf or 0, as
// Python's float() does. nullopt for anything else: whitespace, inf/nan, hex, a bare sign
std::optional<double> ParseDecimal(std::string_view text);

} // namespace v2
} // namespace jinja2

#endif // JINJA2CPP_SRC_DECIMAL_NUMBER_H
