#include "internal_value.h"
#include "value_visitors.h"

#include <jinja2cpp/big_int.h>
#include <jinja2cpp/generic_list.h>
#include <jinja2cpp/value.h>

#include <algorithm>
#include <charconv>
#include <cstddef>
#include <new>
#include <string>
#include <system_error>
#include <utility>
#include <type_traits>
#include <variant>
#include <vector>

namespace jinja2
{
inline namespace v2
{
namespace
{
// Every printing form of Value meets here, so none can print differently from {{ value }}:
// the value is lent to the template renderer, not copied
template<typename CharT>
void AppendText(const Value& val, std::basic_string<CharT>& out)
{
    const InternalValue internal = std::visit(visitors::InputValueConvertor(false, true), val.GetData());
    visitors::RenderValue(internal, out, false);
}

// The containers being destroyed on this thread, one inside the other (Value::DestroyContainer)
thread_local unsigned t_valueDestroyDepth = 0;
// Deeper, the rest of a value is destroyed one container at a time
constexpr unsigned MaxRecursiveDestroy = 64;
constexpr std::size_t ListIndex = 8;
constexpr std::size_t MapIndex = 9;
static_assert(std::is_same_v<std::variant_alternative_t<ListIndex, Value::ValueData>, RecWrapper<ValuesList>> && std::is_same_v<std::variant_alternative_t<MapIndex, Value::ValueData>, RecWrapper<ValuesMap>>);

// A ValuesList or ValuesMap that holds items
bool IsFilledContainer(const Value& value) noexcept
{
    const auto& data = value.GetData();
    if (const auto* list = std::get_if<ListIndex>(&data))
    {
        return !list->valueless_after_move() && !(*list)->empty();
    }
    if (const auto* map = std::get_if<MapIndex>(&data))
    {
        return !map->valueless_after_move() && !(*map)->empty();
    }
    return false;
}

// Moves the filled containers `value` holds to `pending`
void TakeFilledContainers(Value& value, std::vector<Value>& pending)
{
    const auto take = [&pending](Value& item) {
        if (IsFilledContainer(item))
        {
            pending.push_back(std::move(item));
        }
    };
    auto& data = value.GetData();
    if (auto* list = std::get_if<ListIndex>(&data))
    {
        if (!list->valueless_after_move())
        {
            std::for_each((*list)->begin(), (*list)->end(), take);
        }
    }
    else if (auto* map = std::get_if<MapIndex>(&data))
    {
        if (!map->valueless_after_move())
        {
            std::for_each((*map)->begin(), (*map)->end(), [&take](auto& entry) { take(entry.second); });
        }
    }
}
} // namespace

void Value::DestroyContainer() noexcept
{
    auto& depth = t_valueDestroyDepth;
    if (depth < MaxRecursiveDestroy)
    {
        ++depth;
        // Moved out and destroyed here, with the depth counted; what stays is valueless
        if (auto* list = std::get_if<ListIndex>(&m_data))
        {
            const RecWrapper<ValuesList> doomed(std::move(*list));
        }
        else if (auto* map = std::get_if<MapIndex>(&m_data))
        {
            const RecWrapper<ValuesMap> doomed(std::move(*map));
        }
        --depth;
        return;
    }
    // The nested containers move to a list and are destroyed one by one, each after its own
    // filled containers have moved: none of them recurses. A failure to grow the list leaves
    // the rest to the recursive destruction, as before
    std::vector<Value> pending;
    try
    {
        TakeFilledContainers(*this, pending);
        while (!pending.empty())
        {
            Value item = std::move(pending.back());
            pending.pop_back();
            TakeFilledContainers(item, pending);
        }
    }
    catch (...) // NOLINT(bugprone-empty-catch)
    {
    }
}

GenericList Value::ToGenericList() const&
{
    if (const auto* list = GetIf<GenericList>())
    {
        return *list;
    }
    if (const auto* list = GetIf<ValuesList>())
    {
        return ListAdapter::CreateAdapter(*list).CreateGenericList();
    }
    return {};
}

GenericMap Value::ToGenericMap() const&
{
    if (const auto* map = GetIf<GenericMap>())
    {
        return *map;
    }
    if (const auto* map = GetIf<ValuesMap>())
    {
        return CreateMapAdapter(*map).CreateGenericMap();
    }
    return {};
}

GenericList Value::ToGenericList() &&
{
    if (auto* list = GetIf<ValuesList>())
    {
        return ListAdapter::CreateAdapter(std::move(*list)).CreateGenericList();
    }
    return std::as_const(*this).ToGenericList();
}

GenericMap Value::ToGenericMap() &&
{
    if (auto* map = GetIf<ValuesMap>())
    {
        return CreateMapAdapter(std::move(*map)).CreateGenericMap();
    }
    return std::as_const(*this).ToGenericMap();
}

std::string Value::ToString() const
{
    std::string result;
    AppendText(*this, result);
    return result;
}

void Value::ToString(std::string& out) const
{
    AppendText(*this, out);
}

std::wstring Value::ToWString() const
{
    std::wstring result;
    AppendText(*this, result);
    return result;
}

void Value::ToWString(std::wstring& out) const
{
    AppendText(*this, out);
}

std::to_chars_result Value::ToChars(char* first, char* last) const noexcept
{
    try
    {
        // Short text (numbers, booleans, None) stays in the string's inline buffer
        std::string text;
        AppendText(*this, text);
        const auto room = static_cast<size_t>(last - first);
        if (text.size() > room)
        {
            std::copy_n(text.data(), room, first);
            return { last, std::errc::value_too_large };
        }
        return { std::copy_n(text.data(), text.size(), first), std::errc() };
    }
    catch (const std::bad_alloc&)
    {
        return { last, std::errc::not_enough_memory };
    }
    catch (...)
    {
        // A host accessor or reflector threw
        return { last, std::errc::invalid_argument };
    }
}

} // namespace v2
} // namespace jinja2
