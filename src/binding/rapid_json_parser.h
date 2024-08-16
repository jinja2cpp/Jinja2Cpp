#ifndef JINJA2CPP_SRC_RAPID_JSON_PARSER_H
#define JINJA2CPP_SRC_RAPID_JSON_PARSER_H

#include <rapidjson/document.h>
#include <rapidjson/encodings.h>
#include <rapidjson/error/en.h>

#include <jinja2cpp/binding/rapid_json.h>

#include <boost/any.hpp>
#include <boost/any/unique_any.hpp>


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

template<typename CharT>
nonstd::expected<Value, std::string> Parse(nonstd::basic_string_view<CharT> json, boost::anys::unique_any& metadataJson)
{
    using JsonDocumentType = rapidjson::GenericDocument<typename detail::RapidJsonEncodingType<sizeof(CharT)>::type>;
    metadataJson.emplace<JsonDocumentType>();
//    metadataJson = boost::anys::unique_any(boost::anys::in_place_type_t<typename JsonDocumentType>);
//    metadataJson = {std::make_unique<JsonDocumentType>()};
   // metadataJson = boost::any(std::move(rapidJson));
    auto& jsonDoc = boost::any_cast<JsonDocumentType&>(metadataJson);
    rapidjson::ParseResult res = jsonDoc.Parse(json.data(), json.size());
//__assert("a", __FILE__, __LINE__);
//    if (res.IsError())
//        __assert("a", __FILE__, __LINE__);
    if (!res)
    {
        std::string jsonError = rapidjson::GetParseError_En(res.Code());
        return nonstd::make_unexpected(jsonError);
    }
//    metadataJson = json;
    std::cout<< "!!!!" << jsonDoc.MemberCount() << std::endl;
    return Reflect(jsonDoc);
}

} // namespace jinja2

#endif // JINJA2CPP_SRC_RAPID_JSON_PARSER_H
