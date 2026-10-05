#ifndef JINJA2CPP_SRC_PYTHON_FORMAT_H
#define JINJA2CPP_SRC_PYTHON_FORMAT_H

#include "internal_value.h"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

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

// The text of a narrow string value without copying it; empty for any other value
std::optional<std::string_view> NarrowStringView(const InternalValue& val);

} // namespace jinja2

#endif // JINJA2CPP_SRC_PYTHON_FORMAT_H
