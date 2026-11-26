#ifndef JINJA2CPP_SRC_SET_INDEX_H
#define JINJA2CPP_SRC_SET_INDEX_H

// The hash index of the engine's set (docs/tasks/0561): the position of each item of the set's list, by the
// item's hash token (MakeDictKey), so that add, remove, discard and `in` do not scan the items

#include "hash_map.h"
#include "internal_value.h"
#include "recursion_guard.h"
#include "template_dict.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <utility>

namespace jinja2
{
inline namespace v2
{
// A cache beside the items of a set (IListAccessor::GetSetIndex): it holds no values, and a lookup checks what it
// finds, so that a path that changed the items without the set's methods costs a rebuild, never a wrong answer.
// A size that differs from the items' (the cycle collector's Clear() among them) makes the next lookup rebuild it.
// Positions stay those of the items, in insertion order: an erase renumbers the later ones. Its entries count
// against maxRenderMemory; what was counted is given back once, when the entry goes or the index is dropped
class SetIndex final : public ValueObject
{
public:
    static constexpr size_t NotFound = std::numeric_limits<size_t>::max();

    SetIndex() = default;
    SetIndex(const SetIndex&) = delete;
    SetIndex& operator=(const SetIndex&) = delete;
    ~SetIndex() override { ReleaseRenderMemory(m_charged); }

    // The position in `items` of the item `token` stands for, for which `same(item)` holds, or NotFound
    template<typename Same>
    size_t Find(const InternalValueList& items, const std::string& token, const Same& same)
    {
        for (int attempt = 0; attempt != 2; ++attempt)
        {
            Sync(items);
            const auto found = m_positions.find(token);
            if (found == m_positions.end())
            {
                return NotFound;
            }
            if (found->second < items.size() && same(items[found->second]))
            {
                return found->second;
            }
            // A hit that is not the item: the cache is stale, as only a path that bypassed it can leave it
            m_stale = true;
        }
        return NotFound;
    }

    // Counts the entry for `token` before the item is added, so that a failure leaves the set as it was
    void Reserve(const std::string& token) { Charge(EntryBytes(token)); }
    // The item `token` stands for was appended to the items, whose last position it is; Reserve counted it
    void Appended(const InternalValueList& items, std::string token, size_t position)
    {
        if (m_stale)
        {
            return;
        }
        const auto bytes = EntryBytes(token);
        try
        {
            if (!m_positions.emplace(std::move(token), position).second)
            {
                Release(bytes);
            }
        }
        catch (...)
        {
            m_stale = true;
            Release(bytes);
            throw;
        }
        // An index that does not match the items is built again by the next lookup
        m_stale = m_positions.size() != items.size();
    }
    // The item at `position` was erased from `items`, whose later items moved down by one
    void Erased(const InternalValueList& items, const std::string& token, size_t position)
    {
        if (m_stale)
        {
            return;
        }
        if (m_positions.erase(token) != 0)
        {
            Release(EntryBytes(token));
        }
        for (auto& entry : m_positions)
        {
            if (entry.second > position)
            {
                --entry.second;
            }
        }
        m_stale = m_positions.size() != items.size();
    }
    // The items were replaced or emptied: the next lookup builds the index again
    void Reset() noexcept
    {
        m_positions = Positions();
        ReleaseRenderMemory(std::exchange(m_charged, 0));
        m_stale = false;
    }

private:
    using Positions = detail::FlatMap<std::string, size_t, NameHash>;

    // What an entry counts: its slot, the bytes of its token and the table's own overhead
    static std::uint64_t EntryBytes(const std::string& token) noexcept
    {
        constexpr std::uint64_t slot = sizeof(Positions::value_type);
        constexpr std::uint64_t overhead = std::uint64_t{ 2 } * sizeof(void*);
        return std::uint64_t{ token.size() } + slot + overhead;
    }

    void Charge(std::uint64_t bytes)
    {
        const bool counted = RenderMemoryCounted();
        ChargeRenderMemory(bytes);
        if (counted)
        {
            m_charged += bytes;
        }
    }
    void Release(std::uint64_t bytes) noexcept
    {
        // Never more than was counted
        bytes = std::min(bytes, m_charged);
        m_charged -= bytes;
        ReleaseRenderMemory(bytes);
    }

    void Sync(const InternalValueList& items)
    {
        if (m_stale || m_positions.size() != items.size())
        {
            Rebuild(items);
        }
    }
    void Rebuild(const InternalValueList& items)
    {
        Reset();
        try
        {
            m_positions.reserve(items.size());
            for (size_t pos = 0; pos != items.size(); ++pos)
            {
                // The items of a set are hashable; one that is not has no entry, which no lookup finds
                auto token = MakeDictKey(items[pos]);
                if (!token)
                {
                    continue;
                }
                const auto bytes = EntryBytes(*token);
                Charge(bytes);
                if (!m_positions.emplace(std::move(*token), pos).second)
                {
                    Release(bytes);
                }
            }
        }
        catch (...)
        {
            Reset();
            throw;
        }
    }

    Positions m_positions;
    std::uint64_t m_charged = 0;
    bool m_stale = false;
};
} // namespace v2
} // namespace jinja2

#endif // JINJA2CPP_SRC_SET_INDEX_H
