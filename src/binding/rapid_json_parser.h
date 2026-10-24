#ifndef JINJA2CPP_SRC_RAPID_JSON_PARSER_H
#define JINJA2CPP_SRC_RAPID_JSON_PARSER_H

#include <rapidjson/document.h>
#include "../make_unexpected.h"
#include <rapidjson/encodings.h>
#include <rapidjson/error/en.h>

#include <jinja2cpp/binding/rapid_json.h>

#include <memory>
#include <utility>

namespace jinja2
{
inline namespace v2
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

#if defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__ // MSVC targets are little-endian
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

template<typename CharT>
nonstd::expected<Value, std::string> Parse(std::basic_string_view<CharT> json, std::shared_ptr<void>& metadataJson)
{
    using JsonDocumentType = rapidjson::GenericDocument<typename detail::RapidJsonEncodingType<sizeof(CharT)>::type>;
    auto doc = std::make_shared<JsonDocumentType>();
    auto& jsonDoc = *doc;
    metadataJson = std::move(doc); // persist parse result
    rapidjson::ParseResult res = jsonDoc.Parse(json.data(), json.size());
    if (!res)
    {
        std::string jsonError = rapidjson::GetParseError_En(res.Code());
        return MakeUnexpected(jsonError);
    }
    return Reflect(jsonDoc);
}

} // namespace v2
} // namespace jinja2

#endif // JINJA2CPP_SRC_RAPID_JSON_PARSER_H
