#ifndef JINJA2CPP_SRC_RAPID_JSON_SERIALIZER_H
#define JINJA2CPP_SRC_RAPID_JSON_SERIALIZER_H

#include "../internal_value.h"

#include <rapidjson/document.h>
#include <rapidjson/rapidjson.h>

#include <memory>

namespace jinja2
{

namespace rapidjson_serializer
{

class ValueWrapper
{
    friend class DocumentWrapper;

public:
    ValueWrapper(const ValueWrapper&) = delete;
    ValueWrapper(ValueWrapper&&) = default;
    ValueWrapper& operator=(const ValueWrapper&) = delete;
    ValueWrapper& operator=(ValueWrapper&&) = default;
    ~ValueWrapper() = default;

    std::string AsString(uint8_t indent = 0) const;

private:
    ValueWrapper(rapidjson::Value&& value, std::shared_ptr<rapidjson::Document> document);

    rapidjson::Value m_value;
    std::shared_ptr<rapidjson::Document> m_document;
};

class DocumentWrapper
{
public:
    DocumentWrapper();

    DocumentWrapper(const DocumentWrapper&) = delete;
    DocumentWrapper(DocumentWrapper&&) = default;
    DocumentWrapper& operator=(const DocumentWrapper&) = delete;
    DocumentWrapper& operator=(DocumentWrapper&&) = default;
    ~DocumentWrapper() = default;

    ValueWrapper CreateValue(const InternalValue& value) const;

private:
    std::shared_ptr<rapidjson::Document> m_document;
};

using DocumentWrapper = jinja2::rapidjson_serializer::DocumentWrapper;


} // namespace rapidjson_serializer

std::string ToJson(const InternalValue& value, uint8_t indent);

} // namespace jinja2

#endif // JINJA2CPP_SRC_RAPID_JSON_SERIALIZER_H
