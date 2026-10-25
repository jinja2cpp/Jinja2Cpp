#ifndef JINJA2CPP_BINDING_NLOHMANN_JSON_H
#define JINJA2CPP_BINDING_NLOHMANN_JSON_H

#include <nlohmann/json.hpp>

#include <jinja2cpp/json_adapter.h>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string> // IWYU pragma: keep (Key() before nlohmann_json 3.11)
#include <string_view>

namespace jinja2
{
inline namespace v2
{

/*!
 * \brief JsonTraits of nlohmann::json and nlohmann::ordered_json
 *
 * `nlohmann::json` iterates objects in key order; `nlohmann::ordered_json` keeps the document order, as a Python dict
 * from `json.loads` does (docs/tasks/0043). Binary values and discarded values read as null.
 */
template<typename Json>
struct NlohmannJsonTraits : DomJsonTraits<Json>
{
    using Node = typename DomJsonTraits<Json>::Node;

    static JsonKind Kind(Node node)
    {
        switch (node.get().type())
        {
        case nlohmann::detail::value_t::boolean:
            return JsonKind::Bool;
        case nlohmann::detail::value_t::number_integer:
            return JsonKind::Int;
        case nlohmann::detail::value_t::number_unsigned:
            return JsonKind::UInt;
        case nlohmann::detail::value_t::number_float:
            return JsonKind::Double;
        case nlohmann::detail::value_t::string:
            return JsonKind::String;
        case nlohmann::detail::value_t::array:
            return JsonKind::Array;
        case nlohmann::detail::value_t::object:
            return JsonKind::Object;
        default:
            return JsonKind::Null;
        }
    }
    static bool AsBool(Node node) { return node.get().template get<bool>(); }
    static std::int64_t AsInt(Node node) { return node.get().template get<std::int64_t>(); }
    static std::uint64_t AsUInt(Node node) { return node.get().template get<std::uint64_t>(); }
    static double AsDouble(Node node) { return node.get().template get<double>(); }
    static std::string_view AsString(Node node) { return node.get().template get_ref<const typename Json::string_t&>(); }
    static std::size_t Size(Node node) { return node.get().size(); }
    static Node At(Node node, std::size_t idx) { return std::cref(node.get()[idx]); }

    // nlohmann::json looks a key up by string_view from 3.11 on
#if NLOHMANN_JSON_VERSION_MAJOR > 3 || (NLOHMANN_JSON_VERSION_MAJOR == 3 && NLOHMANN_JSON_VERSION_MINOR >= 11)
    static std::string_view Key(std::string_view name) { return name; }
#else
    static std::string Key(std::string_view name) { return std::string(name); }
#endif

    static std::optional<Node> Find(Node node, std::string_view name)
    {
        const Json& obj = node.get();
        auto item = obj.find(Key(name));
        if (item == obj.end())
        {
            return std::nullopt;
        }
        return std::cref(*item);
    }

    template<typename F>
    static void ForEachMember(Node node, F onMember)
    {
        const Json& obj = node.get();
        for (auto item = obj.begin(); item != obj.end(); ++item)
        {
            if (!onMember(std::string_view(item.key()), std::cref(*item)))
            {
                break;
            }
        }
    }
};

template<>
struct JsonTraits<nlohmann::json> : NlohmannJsonTraits<nlohmann::json>
{
};

// ordered_json exists from nlohmann 3.9 on
#if NLOHMANN_JSON_VERSION_MAJOR > 3 || (NLOHMANN_JSON_VERSION_MAJOR == 3 && NLOHMANN_JSON_VERSION_MINOR >= 9)
template<>
struct JsonTraits<nlohmann::ordered_json> : NlohmannJsonTraits<nlohmann::ordered_json>
{
};
#endif

} // namespace v2
} // namespace jinja2

#endif // JINJA2CPP_BINDING_NLOHMANN_JSON_H
