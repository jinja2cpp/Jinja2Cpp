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

// The double a float is in a template: the one nearest the float's shortest round-trip decimal,
// so 12.123f is 12.123, as Python and numpy print it (docs/tasks/0039). Converting it back to
// float gives `val` exactly; inf, nan and -0.0 stay as they are
double FloatAsDecimalDouble(float val);

} // namespace v2
} // namespace jinja2

#endif // JINJA2CPP_SRC_DECIMAL_NUMBER_H
