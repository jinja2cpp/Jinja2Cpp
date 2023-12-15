#ifndef JINJA2CPP_SRC_BOOST_JSON_SERIALIZER_H
#define JINJA2CPP_SRC_BOOST_JSON_SERIALIZER_H

#include "../internal_value.h"

#include <boost/json.hpp>

#include <memory>

namespace jinja2
{
namespace boost_json_serializer
{

class ValueWrapper
{
    friend class DocumentWrapper;

public:
    ValueWrapper(ValueWrapper&&) = default;
    ValueWrapper& operator=(ValueWrapper&&) = default;

    std::string AsString(uint8_t indent = 0) const;

private:
    ValueWrapper(boost::json::value&& value);

    boost::json::value m_value;
};

class DocumentWrapper
{
public:
    DocumentWrapper();

    DocumentWrapper(DocumentWrapper&&) = default;
    DocumentWrapper& operator=(DocumentWrapper&&) = default;

    ValueWrapper CreateValue(const InternalValue& value) const;

private:
};

using DocumentWrapper = jinja2::boost_json_serializer::DocumentWrapper;

} // namespace boost_json_serializer
} // namespace jinja2

#endif // JINJA2CPP_SRC_BOOST_JSON_SERIALIZER_H
