#include "engine_errors.h"
#include "internal_value.h"
#include "value_visitors.h"

#include <jinja2cpp/generic_list.h>
#include <jinja2cpp/user_callable.h>
#include <jinja2cpp/value.h>

#include <fmt/format.h>

#include <algorithm>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <new>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
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

// Whether `value` holds a ValuesList or a ValuesMap (not a moved-from one): what Value's copy
// and destructor recurse into
bool IsNestable(const Value& value) noexcept
{
    if (value.GetIf<ValuesList>())
    {
        return true;
    }
    return static_cast<bool>(value.GetIf<ValuesMap>());
}

// Whether `value` holds a list or map with items: one whose destruction may recurse
bool HoldsItems(const Value& value) noexcept
{
    if (const auto* list = value.GetIf<ValuesList>())
    {
        return !list->empty();
    }
    if (const auto* map = value.GetIf<ValuesMap>())
    {
        return !map->empty();
    }
    return false;
}

// Moves the items of `container` that hold items of their own to the back of `pending`, leaving
// moved-from items behind, whose destruction does not recurse
void TakeNested(Value& container, std::vector<Value>& pending)
{
    const auto take = [&pending](Value& item) {
        if (HoldsItems(item))
        {
            pending.push_back(std::move(item));
        }
    };
    if (auto* list = container.GetIf<ValuesList>())
    {
        for (auto& item : *list)
        {
            take(item);
        }
    }
    else if (auto* map = container.GetIf<ValuesMap>())
    {
        for (auto& entry : *map)
        {
            take(entry.second);
        }
    }
}

// Copies a Value top-down from a worklist, so that no copy recurses: each container is copied
// with empty items in place of its nested containers, which are filled in later. The items of
// a finished container never move again, so the worklist may refer to them
class DeepCopier
{
public:
    explicit DeepCopier(std::size_t maxDepth)
        : m_maxDepth(maxDepth)
    {
    }

    // Copies the list or map `source` into `root`; false when it nests deeper than maxDepth
    bool Copy(const Value& source, Value& root)
    {
        m_pending.push_back(Step{ source, root, 1 });
        while (!m_pending.empty())
        {
            const Step step = m_pending.back();
            m_pending.pop_back();
            const Value& from = step.source.get();
            const auto* list = from.GetIf<ValuesList>();
            if (!(list ? CopyList(*list, step) : CopyMap(*from.GetIf<ValuesMap>(), step)))
            {
                return false;
            }
        }
        return true;
    }

private:
    // One container to copy: its source, the item of the copy that receives it, and its depth
    struct Step
    {
        std::reference_wrapper<const Value> source;
        std::reference_wrapper<Value> target;
        std::size_t depth;
    };

    // The item itself, or an empty one that a later step fills in
    static Value Shallow(const Value& item)
    {
        if (IsNestable(item))
        {
            return {};
        }
        return item;
    }

    // Queues `source`, an item of a container `depth` deep, for `target` when it is a container
    bool Queue(const Value& source, Value& target, std::size_t depth)
    {
        if (!IsNestable(source))
        {
            return true;
        }
        if (depth >= m_maxDepth)
        {
            return false;
        }
        m_pending.push_back(Step{ source, target, depth + 1 });
        return true;
    }

    bool CopyList(const ValuesList& list, const Step& step)
    {
        ValuesList copy;
        copy.reserve(list.size());
        for (const auto& item : list)
        {
            copy.push_back(Shallow(item));
        }
        step.target.get() = Value(std::move(copy));
        auto& items = *step.target.get().GetIf<ValuesList>();
        for (std::size_t idx = 0; idx < list.size(); ++idx)
        {
            if (!Queue(list[idx], items[idx], step.depth))
            {
                return false;
            }
        }
        return true;
    }

    bool CopyMap(const ValuesMap& map, const Step& step)
    {
        ValuesMap copy;
        copy.reserve(map.size());
        for (const auto& [key, item] : map)
        {
            copy.emplace(key, Shallow(item));
        }
        step.target.get() = Value(std::move(copy));
        // The copy keeps the source's order: its keys are unique, so each one was inserted
        auto& items = *step.target.get().GetIf<ValuesMap>();
        auto target = items.begin();
        for (auto from = map.begin(); from != map.end(); ++from, ++target)
        {
            if (!Queue(from->second, target->second, step.depth))
            {
                return false;
            }
        }
        return true;
    }

    std::size_t m_maxDepth;
    std::vector<Step> m_pending;
};
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

void DestroyDeep(Value&& value) noexcept
{
    Value root = std::move(value);
    value = Value();
    if (!HoldsItems(root))
    {
        return;
    }
    // Depth-first on the heap: each container is destroyed after its nested containers have been
    // moved out of it, so no destructor recurses more than one level. If the heap runs out, the
    // rest is destroyed by Value's destructor while unwinding, as it would have been without this
    try
    {
        std::vector<Value> pending;
        pending.push_back(std::move(root));
        while (!pending.empty())
        {
            Value current = std::move(pending.back());
            pending.pop_back();
            TakeNested(current, pending);
        }
    }
    catch (...) // NOLINT(bugprone-empty-catch): only std::bad_alloc, and the rest is already released
    {
    }
}

std::optional<Value> DeepCopy(const Value& value, std::size_t maxDepth)
{
    if (!IsNestable(value))
    {
        return value;
    }
    if (maxDepth == 0)
    {
        return std::nullopt;
    }
    Value root;
    try
    {
        if (!DeepCopier(maxDepth).Copy(value, root))
        {
            DestroyDeep(std::move(root));
            return std::nullopt;
        }
    }
    catch (...)
    {
        DestroyDeep(std::move(root));
        throw;
    }
    return std::optional<Value>(std::in_place, std::move(root));
}

namespace detail
{
namespace
{
template<typename T>
[[noreturn]] void ThrowOutOfRange(std::string_view paramName, T value, int bits, bool isSigned)
{
    const char* kind = isSigned ? "signed" : "unsigned";
    if (paramName.empty())
    {
        throw IntegerOutOfRangeError(fmt::format("callable argument {} is out of range for its {}-bit {} integer parameter", value, bits, kind));
    }
    throw IntegerOutOfRangeError(fmt::format("callable argument '{}' = {} is out of range for its {}-bit {} integer parameter", paramName, value, bits, kind));
}
} // namespace

void ThrowArgumentOutOfRange(std::string_view paramName, int64_t value, int bits, bool isSigned)
{
    ThrowOutOfRange(paramName, value, bits, isSigned);
}

void ThrowArgumentOutOfRange(std::string_view paramName, double value, int bits, bool isSigned)
{
    ThrowOutOfRange(paramName, value, bits, isSigned);
}
} // namespace detail
} // namespace v2
} // namespace jinja2
