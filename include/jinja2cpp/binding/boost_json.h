#ifndef JINJA2CPP_BINDING_BOOST_JSON_H
#define JINJA2CPP_BINDING_BOOST_JSON_H

#include <boost/json.hpp>

#include <jinja2cpp/json_adapter.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

namespace jinja2
{
inline namespace v2
{

/*!
 * \brief JsonTraits of boost::json::value
 *
 * Objects keep the document order, as a Python dict from `json.loads` does. Boost.JSON's parser keeps the last value
 * of a repeated key.
 */
template<>
struct JsonTraits<boost::json::value> : DomJsonTraits<boost::json::value>
{
    static JsonKind Kind(Node node)
    {
        switch (node.get().kind())
        {
        case boost::json::kind::bool_:
            return JsonKind::Bool;
        case boost::json::kind::int64:
            return JsonKind::Int;
        case boost::json::kind::uint64:
            return JsonKind::UInt;
        case boost::json::kind::double_:
            return JsonKind::Double;
        case boost::json::kind::string:
            return JsonKind::String;
        case boost::json::kind::array:
            return JsonKind::Array;
        case boost::json::kind::object:
            return JsonKind::Object;
        default:
            return JsonKind::Null;
        }
    }
    static bool AsBool(Node node) { return node.get().get_bool(); }
    static std::int64_t AsInt(Node node) { return node.get().get_int64(); }
    static std::uint64_t AsUInt(Node node) { return node.get().get_uint64(); }
    static double AsDouble(Node node) { return node.get().get_double(); }
    static std::string_view AsString(Node node)
    {
        const auto& str = node.get().get_string();
        return { str.data(), str.size() };
    }
    static std::size_t Size(Node node)
    {
        const auto& val = node.get();
        if (const auto* arr = val.if_array())
        {
            return arr->size();
        }
        if (const auto* obj = val.if_object())
        {
            return obj->size();
        }
        return 0;
    }
    static Node At(Node node, std::size_t idx) { return std::cref(node.get().get_array()[idx]); }
    static std::optional<Node> Find(Node node, std::string_view name)
    {
        const auto* obj = node.get().if_object();
        const auto* item = obj ? obj->if_contains(boost::json::string_view(name.data(), name.size())) : nullptr;
        if (!item)
        {
            return std::nullopt;
        }
        return std::cref(*item);
    }
    template<typename F>
    static void ForEachMember(Node node, F&& onMember)
    {
        for (const auto& member : node.get().get_object())
        {
            const auto key = member.key();
            if (!onMember(std::string_view(key.data(), key.size()), std::cref(member.value())))
            {
                break;
            }
        }
    }
};

} // namespace v2
} // namespace jinja2

#endif // JINJA2CPP_BINDING_BOOST_JSON_H
