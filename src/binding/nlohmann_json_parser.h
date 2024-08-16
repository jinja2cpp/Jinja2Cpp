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
    (void)json;
    auto value = nlohmann::json::parse({json.data(), json.size()});
    if (ec)
    {
        return nonstd::make_unexpected(ec.what());
    }
    return Reflect(value);
}

} // namespace jinja2

#endif // JINJA2CPP_SRC_NLOHMANN_JSON_PARSER_H
