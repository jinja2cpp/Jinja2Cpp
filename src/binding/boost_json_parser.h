#ifndef JINJA2CPP_SRC_BOOST_JSON_PARSER_H
#define JINJA2CPP_SRC_BOOST_JSON_PARSER_H

#include "../make_unexpected.h"

#include <jinja2cpp/binding/boost_json.h>
#include <jinja2cpp/value.h>

#include <boost/any.hpp>
#include <boost/any/unique_any.hpp>

#include <string>
#include <string_view>

namespace jinja2
{

template<typename CharT>
nonstd::expected<Value, std::string> Parse(std::basic_string_view<CharT> json, boost::anys::unique_any& metadataJson)
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

} // namespace jinja2

#endif // JINJA2CPP_SRC_BOOST_JSON_PARSER_H
