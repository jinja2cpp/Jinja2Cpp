#ifndef JINJA2CPP_SRC_NLOHMANN_JSON_PARSER_H
#define JINJA2CPP_SRC_NLOHMANN_JSON_PARSER_H

#include <jinja2cpp/binding/nlohmann_json.h>

#include <boost/any.hpp>
#include <boost/any/unique_any.hpp>

namespace jinja2
{

template<typename CharT>
nonstd::expected<Value, std::string> Parse(nonstd::basic_string_view<CharT> json, boost::anys::unique_any& metadataJson)
{
    //intentionally ignore metadataJson
    (void)metadataJson;
    try
    {
        auto value = nlohmann::json::parse(json.data(), json.data() + json.size());
        return Reflect(value);
    }
    catch(std::exception& ex)
    {
        return nonstd::make_unexpected(ex.what());
    }
}

} // namespace jinja2

#endif // JINJA2CPP_SRC_NLOHMANN_JSON_PARSER_H
