#ifndef JINJA2CPP_SRC_BOOST_JSON_PARSER_H
#define JINJA2CPP_SRC_BOOST_JSON_PARSER_H

#include "../make_unexpected.h"

#include <jinja2cpp/binding/boost_json.h>
#include <jinja2cpp/value.h>

#include <memory>
#include <string>
#include <string_view>

namespace jinja2
{
inline namespace v2
{

template<typename CharT>
nonstd::expected<Value, std::string> Parse(std::basic_string_view<CharT> json, std::shared_ptr<void>& metadataJson)
{
    //intentionally ignore metadataJson
    (void)metadataJson;
    boost::system::error_code ec;
    auto value = boost::json::parse({ json.data(), json.size() }, ec);
    if (ec)
    {
        return MakeUnexpected(ec.what());
    }
    return Reflect(value);
}

} // namespace v2
} // namespace jinja2

#endif // JINJA2CPP_SRC_BOOST_JSON_PARSER_H
