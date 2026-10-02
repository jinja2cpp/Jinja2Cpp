#ifndef JINJA2CPP_SRC_PYTHON_FORMAT_H
#define JINJA2CPP_SRC_PYTHON_FORMAT_H

#include "internal_value.h"

#include <string>

namespace jinja2
{

// Python's printf-style formatting, `format % values`, for a UTF-8 format string. `values` is
// what Python's right operand is: a tuple (a list marked as tuple) of positional arguments, a
// mapping for "%(name)s", or any other single value. Throws std::runtime_error with Python's
// message where Python raises.
std::string PythonPercentFormat(const std::string& format, const InternalValue& values);

} // namespace jinja2

#endif // JINJA2CPP_SRC_PYTHON_FORMAT_H
