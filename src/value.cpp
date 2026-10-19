#include "internal_value.h"
#include "value_visitors.h"

#include <jinja2cpp/big_int.h>
#include <jinja2cpp/generic_list.h>
#include <jinja2cpp/value.h>

#include <algorithm>
#include <charconv>
#include <cstddef>
#include <string>
#include <system_error>
#include <utility>
#include <variant>

namespace jinja2
{
namespace
{
// Every printing form of Value meets here, so none can print differently from {{ value }}:
// the value is lent to the template renderer, not copied
template<typename CharT>
void AppendText(const Value& val, std::basic_string<CharT>& out)
{
    // A template cannot hold a big integer yet (docs/tasks/0270), but its text is all {{ value }} prints
    if (const auto* big = val.GetIf<BigInt>())
    {
        const auto& digits = big->ToString();
        out.append(digits.begin(), digits.end());
        return;
    }
    const InternalValue internal = std::visit(visitors::InputValueConvertor(false, true), val.GetData());
    visitors::RenderValue(internal, out, false);
}
} // namespace

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
    catch (...)
    {
        return { last, std::errc::not_enough_memory };
    }
}

} // namespace jinja2
