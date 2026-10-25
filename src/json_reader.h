#ifndef JINJA2CPP_SRC_JSON_READER_H
#define JINJA2CPP_SRC_JSON_READER_H

#include <jinja2cpp/config.h>
#include <jinja2cpp/value.h>

#include <nonstd/expected.hpp>

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace jinja2
{
inline namespace v2
{

// The bounds a read applies to text that may be untrusted (`{% meta %}` blocks)
struct JsonReadLimits
{
    // Arrays and objects open at once
    std::size_t maxDepth = 128;
    // Items of one array or object, UTF-8 bytes of one string and characters of one number;
    // 0 keeps MaxSequenceSize
    std::uint64_t maxSequenceSize = 0;
};

// Reads an RFC 8259 JSON document whose top level is an object, into Values built directly:
// strings as std::string (UTF-8, checked), integers that fit as int64_t, other numbers as
// double, a repeated key keeping its first position and its last value, as Python's json
// does. An error message reads as Python's json.JSONDecodeError's: what was expected, then
// the line, column and character offset in `text`. Throws only std::bad_alloc.
JINJA2CPP_EXPORT nonstd::expected<ValuesMap, std::string> ReadJsonObject(std::string_view text, const JsonReadLimits& limits);

// Reads a JSON document of any value, as ReadJsonObject does
JINJA2CPP_EXPORT nonstd::expected<Value, std::string> ReadJsonValue(std::string_view text, const JsonReadLimits& limits);

} // namespace v2
} // namespace jinja2

#endif // JINJA2CPP_SRC_JSON_READER_H
