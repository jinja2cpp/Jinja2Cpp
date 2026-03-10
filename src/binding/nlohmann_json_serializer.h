#ifndef JINJA2CPP_SRC_NLOHMANN_JSON_SERIALIZER_H
#define JINJA2CPP_SRC_NLOHMANN_JSON_SERIALIZER_H

#include "../internal_value.h"

#include <nlohmann/json.hpp>

#include <memory>

namespace jinja2
{

std::string ToJson(const InternalValue& value, uint8_t indent);

} // namespace jinja2

#endif // JINJA2CPP_SRC_NLOHMANN_JSON_SERIALIZER_H
