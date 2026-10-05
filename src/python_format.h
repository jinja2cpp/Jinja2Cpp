#ifndef JINJA2CPP_SRC_PYTHON_FORMAT_H
#define JINJA2CPP_SRC_PYTHON_FORMAT_H

#include "internal_value.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace jinja2
{

// Python's printf-style formatting, `format % values`, for a UTF-8 format string. `values` is
// what Python's right operand is: a tuple (a list marked as tuple) of positional arguments, a
// mapping for "%(name)s", or any other single value. Throws std::runtime_error with Python's
// message where Python raises.
std::string PythonPercentFormat(std::string_view format, const InternalValue& values);

// The same with positional arguments given as an array, as if they were a tuple: format()
// passes its evaluated arguments here without building a list for them
std::string PythonPercentFormat(std::string_view format, const InternalValue* args, size_t count);

// One piece of a parsed format: literal text (a range of the format string) followed by a
// conversion, if any. A piece that ends the format in an error carries the error, which is
// raised only when formatting reaches it, after the arguments before it were read, so the
// message is the one the unparsed format gives
struct PercentDirective
{
    enum class Error : uint8_t
    {
        None,
        Incomplete,          // "%" at the end
        IncompleteKey,       // "%(" without its ")"
        IncompleteAfterSpec, // flags, width or precision at the end
    };

    size_t literalStart = 0;
    size_t literalSize = 0;
    // False for literal text alone
    bool hasConversion = false;
    char conversion = '\0';
    Error error = Error::None;
    bool hasKey = false;
    bool left = false;
    bool plus = false;
    bool space = false;
    bool zero = false;
    bool alternate = false;
    bool widthStar = false;
    bool precisionStar = false;
    size_t keyStart = 0;
    size_t keySize = 0;
    int64_t width = -1;
    int64_t precision = -1;
    // Where the conversion character is, for "unsupported format character ... at index N"
    size_t index = 0;
};

// A format string parsed once, for a format that is a template literal. It never changes
// after construction, so concurrent renders of one template share it without locks
class CompiledPercentFormat
{
public:
    explicit CompiledPercentFormat(std::string format);

    // The same as PythonPercentFormat(format, ...)
    [[nodiscard]] std::string Format(const InternalValue& values) const;
    [[nodiscard]] std::string Format(const InternalValue* args, size_t count) const;

private:
    std::string m_format;
    std::vector<PercentDirective> m_pieces;
};

// The text of a narrow string value without copying it; empty for any other value
std::optional<std::string_view> NarrowStringView(const InternalValue& val);

} // namespace jinja2

#endif // JINJA2CPP_SRC_PYTHON_FORMAT_H
