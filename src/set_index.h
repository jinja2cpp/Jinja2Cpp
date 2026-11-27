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
#include <vector>

namespace jinja2
{
inline namespace v2
{
// A cache beside the items of a set (IListAccessor::GetSetIndex): it holds no values, and a lookup checks what it
// finds, so that a path that changed the items without the set's methods costs a rebuild, never a wrong answer.
// A live size that differs from the index's makes the next lookup rebuild it.
// Positions are slots of the items, in insertion order. An erase leaves its slot empty and counted here as erased
// (docs/tasks/0587), so that remove, discard and pop cost O(1); the slots close up, and the positions follow, once the
// erased ones pass half of them or anything else reads the items (the set's adapter, Compact). Its entries count
// against maxRenderMemory; what was counted is given back once, when the entry goes or the index is dropped
class SetIndex final : public ValueObject
{
public:
    static constexpr size_t NotFound = std::numeric_limits<size_t>::max();

    SetIndex() = default;
    SetIndex(const SetIndex&) = delete;
    SetIndex& operator=(const SetIndex&) = delete;
    ~SetIndex() override { ReleaseRenderMemory(m_charged); }

    // The position in `slots` of the item `token` stands for, for which `same(item)` holds, or NotFound
    template<typename Same>
    size_t Find(const InternalValueList& slots, const std::string& token, const Same& same)
    {
        for (int attempt = 0; attempt != 2; ++attempt)
        {
            Sync(slots);
            const auto found = m_positions.find(token);
            if (found == m_positions.end())
            {
                return NotFound;
            }
            if (found->second < slots.size() && !IsErased(found->second))
            {
                if (same(slots[found->second]))
                {
                    return found->second;
                }
                // An equal token that `same` tells apart is a NaN or holds one: the set holds it once, as Python's holds
                // a NaN object once and the engine's dict a NaN key (docs/tasks/0587)
                if (const auto held = MakeDictKey(slots[found->second]); held && *held == token)
                {
                    return found->second;
                }
            }
            // A hit that is not the item: the cache is stale, as only a path that bypassed it can leave it
            m_stale = true;
        }
        return NotFound;
    }

    // The number of items: the slots less the erased ones
    [[nodiscard]] size_t LiveCount(const InternalValueList& slots) const noexcept { return InStep(slots) ? slots.size() - m_erasedCount : slots.size(); }

    // The first item's slot, or NotFound for an empty set
    size_t First(const InternalValueList& slots)
    {
        if (!InStep(slots))
        {
            DropErased();
            m_stale = true;
        }
        while (m_head < slots.size() && IsErased(m_head))
        {
            ++m_head;
        }
        return m_head < slots.size() ? m_head : NotFound;
    }

    // Counts the entry for `token` and makes room for the erase mark of one more slot before the item is added to
    // `slots`, so that a failure leaves the set as it was
    void Reserve(const InternalValueList& slots, const std::string& token)
    {
        // Geometric growth, as push_back's: a reserve of one more each time would copy the marks at every add
        if (!m_erased.empty() && InStep(slots) && m_erased.capacity() < slots.size() + 1)
        {
            m_erased.reserve(std::max(slots.size() + 1, m_erased.capacity() * 2));
        }
        Charge(EntryBytes(token));
    }
    // The item `token` stands for was appended to the slots, whose last one it is; Reserve counted it
    void Appended(const InternalValueList& slots, std::string token, size_t position)
    {
        if (!m_erased.empty())
        {
            // Not erased; Reserve made the room
            m_erased.push_back(false);
        }
        if (m_stale)
        {
            Release(EntryBytes(token));
            return;
        }
        const auto bytes = EntryBytes(token);
        try
        {
            if (!m_positions.emplace(std::move(token), position).second)
            {
                // An item equal by its token to another that `same` told apart: it is in the slots, not here
                ++m_shadowed;
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
        m_stale = !Matches(slots);
    }
    // Takes the item at `position`, found by its `token`, out of `slots`: its slot is left empty and counted erased
    InternalValue Erase(InternalValueList& slots, const std::string& token, size_t position)
    {
        if (!InStep(slots))
        {
            DropErased();
            m_stale = true;
        }
        // The erased slots never outnumber the items for long, so that the slots stay within about twice the size.
        // The close-up comes first, so that a failure to allocate for it leaves the set as it was
        if ((m_erasedCount + 1) * 2 > slots.size())
        {
            Compact(slots, &position);
        }
        if (m_erased.empty())
        {
            // The marks, made at the first erase; a failure leaves the set as it was
            m_erased.assign(slots.size(), false);
        }
        auto result = std::exchange(slots[position], InternalValue());
        m_erased[position] = true;
        ++m_erasedCount;
        if (!m_stale)
        {
            const auto found = m_positions.find(token);
            if (found != m_positions.end() && found->second == position)
            {
                m_positions.erase(found);
                Release(EntryBytes(token));
                // A shadowed item of the same token has no entry now: the next lookup finds it by a rebuild
                m_stale = m_shadowed != 0;
            }
            else
            {
                m_stale = true;
            }
        }
        return result;
    }
    // Closes up the erased slots, keeping the order of the items, and moves the positions with them, and `tracked`'s
    void Compact(InternalValueList& slots, size_t* tracked = nullptr)
    {
        if (m_erasedCount == 0)
        {
            return;
        }
        if (!InStep(slots))
        {
            DropErased();
            m_stale = true;
            return;
        }
        // The new slot of each old one, made before anything moves, so that a failure leaves the set as it was
        std::vector<size_t> moved(slots.size(), NotFound);
        size_t kept = 0;
        for (size_t pos = 0; pos != slots.size(); ++pos)
        {
            if (!m_erased[pos])
            {
                moved[pos] = kept++;
            }
        }
        for (size_t pos = 0; pos != slots.size(); ++pos)
        {
            if (moved[pos] != NotFound && moved[pos] != pos)
            {
                slots[moved[pos]] = std::move(slots[pos]);
            }
        }
        slots.erase(slots.begin() + static_cast<std::ptrdiff_t>(kept), slots.end());
        for (auto& entry : m_positions)
        {
            entry.second = entry.second < moved.size() ? moved[entry.second] : NotFound;
        }
        if (tracked && *tracked < moved.size())
        {
            *tracked = moved[*tracked];
        }
        DropErased();
    }
    // The items were replaced or emptied: the next lookup builds the index again
    void Reset() noexcept
    {
        m_positions = Positions();
        ReleaseRenderMemory(std::exchange(m_charged, 0));
        m_stale = false;
        m_shadowed = 0;
        DropErased();
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

    [[nodiscard]] bool IsErased(size_t position) const noexcept { return position < m_erased.size() && m_erased[position]; }
    // Whether the erase marks are those of `slots`: a path that changed the slots otherwise (the cycle collector's
    // Clear()) leaves them out of step, and then they no longer count
    [[nodiscard]] bool InStep(const InternalValueList& slots) const noexcept { return m_erased.empty() || m_erased.size() == slots.size(); }
    void DropErased() noexcept
    {
        m_erased = std::vector<bool>();
        m_erasedCount = 0;
        m_head = 0;
    }
    // Whether the entries, with the items they shadow, are the items
    [[nodiscard]] bool Matches(const InternalValueList& slots) const noexcept { return m_positions.size() + m_shadowed == LiveCount(slots); }

    void Sync(const InternalValueList& slots)
    {
        if (!InStep(slots))
        {
            DropErased();
            m_stale = true;
        }
        if (m_stale || !Matches(slots))
        {
            Rebuild(slots);
        }
    }
    // Drops the lookup table and what it counted; the erase marks stay, as the slots they mark do
    void ClearTable() noexcept
    {
        m_positions = Positions();
        ReleaseRenderMemory(std::exchange(m_charged, 0));
        m_shadowed = 0;
    }
    void Rebuild(const InternalValueList& slots)
    {
        ClearTable();
        m_stale = false;
        try
        {
            m_positions.reserve(LiveCount(slots));
            for (size_t pos = 0; pos != slots.size(); ++pos)
            {
                if (IsErased(pos))
                {
                    continue;
                }
                // The items of a set are hashable; one that is not has no entry, which no lookup finds
                auto token = MakeDictKey(slots[pos]);
                if (!token)
                {
                    ++m_shadowed;
                    continue;
                }
                const auto bytes = EntryBytes(*token);
                Charge(bytes);
                if (!m_positions.emplace(std::move(*token), pos).second)
                {
                    ++m_shadowed;
                    Release(bytes);
                }
            }
        }
        catch (...)
        {
            // The erased slots are still there: only the table goes, and the next lookup builds it again
            ClearTable();
            m_stale = true;
            throw;
        }
    }

    Positions m_positions;
    std::uint64_t m_charged = 0;
    // Items in the slots without an entry: equal by token to an earlier one (or unhashable), so that the index
    // matches the items without one
    size_t m_shadowed = 0;
    // The erased slots, empty while none is (and after a compaction), and their count
    std::vector<bool> m_erased;
    size_t m_erasedCount = 0;
    // No slot before this one holds an item: pop() looks from here
    size_t m_head = 0;
    bool m_stale = false;
};
} // namespace v2
} // namespace jinja2

#endif // JINJA2CPP_SRC_SET_INDEX_H
