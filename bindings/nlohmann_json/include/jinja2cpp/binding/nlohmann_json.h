#ifndef JINJA2CPP_BINDING_NLOHMANN_JSON_H
#define JINJA2CPP_BINDING_NLOHMANN_JSON_H

#include <nlohmann/json.hpp>

#include <jinja2cpp/json_adapter.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string> // IWYU pragma: keep (Key() before nlohmann_json 3.11)
#include <string_view>
#include <type_traits>

namespace jinja2
{
inline namespace v2
{

/*!
 * \brief JsonTraits of nlohmann::json and nlohmann::ordered_json
 *
 * `nlohmann::json` iterates objects in key order; `nlohmann::ordered_json` keeps the document order, as a Python dict
 * from `json.loads` does (docs/tasks/0043). Binary values and discarded values read as null.
 *
 * Out of memory: `nlohmann::json`'s destructor allocates (v3.12 flattens a non-empty array or object into a
 * `std::vector`) and is `noexcept`, so an allocation failure while a document is destroyed would end the process.
 * The view therefore empties a document it owns with `Release`, which does not allocate, before destroying it, and
 * copies a non-const lvalue with `Copy`, whose partial result `Release` empties when the copy runs out of memory
 * (docs/tasks/0487). A document the host destroys itself is the host's concern.
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

    /*!
     * \brief Empties `doc` without allocating, so that its destructor does not allocate either
     *
     * Destroying a scalar or an empty container frees memory and allocates none, so the drain removes the last child
     * of the deepest container on the current path until the root is empty. It keeps the last `MaxTrail` ancestors
     * on the stack and starts again from the root when a deeper path runs out of them. The work is capped at a
     * constant times the values removed: a document much deeper than `MaxTrail` (a chain of more than about 1500
     * levels) is emptied only partly, and its destructor, which allocates, frees the rest.
     */
    static void Release(Json& doc) noexcept
    {
        std::array<Json*, MaxTrail> trail{};
        std::size_t top = 0;   // the slot of the next ancestor in the ring
        std::size_t depth = 0; // the ancestors the ring holds
        std::size_t steps = 0;
        std::size_t removed = 0;
        Json* cur = &doc;
        for (; steps <= ReleaseWorkSlack + (ReleaseWorkPerValue * removed); ++steps)
        {
            Json* child = LastChild(*cur);
            if (!child)
            {
                if (depth != 0)
                {
                    top = (top + MaxTrail - 1) % MaxTrail;
                    --depth;
                    cur = trail[top];
                }
                else if (cur != &doc)
                {
                    cur = &doc;
                }
                else
                {
                    return;
                }
            }
            else if (LastChild(*child))
            {
                trail[top] = cur;
                top = (top + 1) % MaxTrail;
                depth = std::min(depth + 1, MaxTrail);
                cur = child;
            }
            else
            {
                PopLast(*cur);
                ++removed;
            }
        }
    }

    /*!
     * \brief Copies `from` into the default-constructed `to`, building it in place
     *
     * When an allocation fails, `to` holds the part copied so far and every container in it is whole, so `Release`
     * can empty it. nlohmann's own copy would destroy its partial copy on the way out, which allocates. Below
     * `MaxCopyDepth` levels the copy is nlohmann's own, whose recursion takes less stack.
     */
    static void Copy(const Json& from, Json& to) { CopyAt(from, to, 0); }

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

private:
    static constexpr std::size_t MaxTrail = 256;
    static constexpr std::size_t ReleaseWorkPerValue = 8;
    static constexpr std::size_t ReleaseWorkSlack = 4096;
    static constexpr std::size_t MaxCopyDepth = 512;

    template<typename C, typename = void>
    struct HasContainer : std::false_type
    {
    };
    template<typename C>
    struct HasContainer<C, std::void_t<typename C::Container>> : std::true_type
    {
    };

    static void CopyAt(const Json& from, Json& to, std::size_t level)
    {
        if (level >= MaxCopyDepth || !(from.is_array() || from.is_object()))
        {
            to = from;
            return;
        }
        if (from.is_array())
        {
            to = Json(Json::value_t::array);
            auto& items = to.template get_ref<typename Json::array_t&>();
            items.reserve(from.size());
            for (const Json& item : from)
            {
                if (item.is_array() || item.is_object())
                {
                    items.emplace_back();
                    CopyAt(item, items.back(), level + 1);
                }
                else
                {
                    items.push_back(item);
                }
            }
            return;
        }
        to = Json(Json::value_t::object);
        auto& members = to.template get_ref<typename Json::object_t&>();
        using Members = typename Json::object_t;
        if constexpr (HasContainer<Members>::value)
        {
            // ordered_json keeps its members in a vector of pairs with const keys, which a reallocation would copy
            members.reserve(from.size());
        }
        for (auto item = from.begin(); item != from.end(); ++item)
        {
            // The keys are unique and come in the container's order: append, without the lookup of emplace
            Json* value = nullptr;
            if constexpr (HasContainer<Members>::value)
            {
                auto& vec = static_cast<typename Members::Container&>(members);
                vec.emplace_back(item.key(), Json());
                value = &vec.back().second;
            }
            else
            {
                value = &members.emplace_hint(members.end(), item.key(), Json())->second;
            }
            if (item->is_array() || item->is_object())
            {
                CopyAt(*item, *value, level + 1);
            }
            else
            {
                *value = *item;
            }
        }
    }

    // The last child of a non-empty array or object, null for a scalar or an empty container; get_ptr does not throw
    static Json* LastChild(Json& node) noexcept
    {
        if (auto* items = node.template get_ptr<typename Json::array_t*>())
        {
            return items->empty() ? nullptr : &items->back();
        }
        if (auto* members = node.template get_ptr<typename Json::object_t*>())
        {
            return members->empty() ? nullptr : &std::prev(members->end())->second;
        }
        return nullptr;
    }

    // Removes the last child of `node`, a non-empty container whose last child is a scalar or an empty container
    static void PopLast(Json& node) noexcept
    {
        if (auto* items = node.template get_ptr<typename Json::array_t*>())
        {
            items->pop_back();
        }
        else if (auto* members = node.template get_ptr<typename Json::object_t*>())
        {
            members->erase(std::prev(members->end()));
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
