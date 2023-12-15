#ifndef JINJA2CPP_SRC_RAPID_JSON_PARSER_H
#define JINJA2CPP_SRC_RAPID_JSON_PARSER_H

#include <rapidjson/error/en.h>


namespace jinja2
{

namespace detail
{
template<size_t Sz>
struct RapidJsonEncodingType;

template<>
struct RapidJsonEncodingType<1>
{
    using type = rapidjson::UTF8<char>;
};

#ifdef BOOST_ENDIAN_BIG_BYTE
template<>
struct RapidJsonEncodingType<2>
{
    using type = rapidjson::UTF16BE<wchar_t>;
};

template<>
struct RapidJsonEncodingType<4>
{
    using type = rapidjson::UTF32BE<wchar_t>;
};
#else
template<>
struct RapidJsonEncodingType<2>
{
    using type = rapidjson::UTF16LE<wchar_t>;
};

template<>
struct RapidJsonEncodingType<4>
{
    using type = rapidjson::UTF32LE<wchar_t>;
};
#endif
} // namespace detail

} // namespace jinja2

#endif // JINJA2CPP_SRC_RAPID_JSON_PARSER_H
