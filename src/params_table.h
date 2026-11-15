#ifndef JINJA2CPP_SRC_PARAMS_TABLE_H
#define JINJA2CPP_SRC_PARAMS_TABLE_H

#include "contracts.h"
#include "internal_value.h"

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <utility>
#include <vector>

namespace jinja2
{
inline namespace v2
{
// The params of a render: one level of names, filled once when the render starts and never
// changed in size after, so that a lookup result pointing into it stays valid to the end
// (docs/tasks/0513). The names are views of the keys the host passed (a ValuesMap's nodes, or
// the names of the values a GenericMap gave), which outlive the render as the values do.
// Up to LinearMax params a lookup compares the names in turn, with no hash at all; a bigger
// table also keeps an index by the keyed name hash (HashName), so that a host's names cannot
// make its lookups quadratic (docs/tasks/0231).
class ParamsTable
{
public:
    struct Entry
    {
        std::string_view name;
        InternalValue value;
        // HashedName::Hash(name), kept only with an index
        size_t hash = 0;
    };
    // Every benchmark row has 1 to 7 params
    static constexpr size_t LinearMax = 8;

    // Room for `count` params, before the first Add. Throws std::bad_alloc
    void Reserve(size_t count)
    {
        JINJA2CPP_ASSERT(m_entries.empty() && m_index.empty());
        if (count > m_entries.capacity())
        {
            m_entries.reserve(count);
        }
        if (count > LinearMax)
        {
            MakeIndex(count);
        }
#if JINJA2CPP_FULL_CONTRACT_CHECKS
        m_reserved = count;
#endif
    }
    // Adds the param `name` with `value`, of the at most Reserve'd count. `mayRepeat` when the
    // host may give a name twice (a GenericMap's keys): the last value wins, in the first place.
    // Throws std::bad_alloc
    void Add(std::string_view name, InternalValue value, bool mayRepeat)
    {
        if (!m_index.empty())
        {
            AddIndexed(name, std::move(value));
            return;
        }
        if (mayRepeat)
        {
            if (auto* found = FindLinearMutable(name))
            {
                *found = std::move(value);
                return;
            }
        }
#if JINJA2CPP_FULL_CONTRACT_CHECKS
        JINJA2CPP_ASSERT(m_entries.size() < m_reserved);
#endif
        m_entries.push_back({ name, std::move(value), 0 });
    }

    // The value of the param `name` (a HashedName or a std::string); null when there is none
    [[nodiscard]] const InternalValue* Find(const HashedName& name) const
    {
        if (!m_index.empty())
        {
            return CheckedIndexed(name.name, name.hash);
        }
        return FindLinear(name.name);
    }
    [[nodiscard]] const InternalValue* Find(std::string_view name) const
    {
        if (!m_index.empty())
        {
            return CheckedIndexed(name, HashedName::Hash(name));
        }
        return FindLinear(name);
    }
    // Where the param `name` is stored, so that a value changed in place is stored back
    // (RenderContext::FindForWrite) or marked (prompt-safe params)
    [[nodiscard]] InternalValue* FindMutable(std::string_view name) { return const_cast<InternalValue*>(std::as_const(*this).Find(name)); }

    [[nodiscard]] bool empty() const noexcept { return m_entries.empty(); }
    [[nodiscard]] size_t size() const noexcept { return m_entries.size(); }
    [[nodiscard]] auto begin() const noexcept { return m_entries.begin(); }
    [[nodiscard]] auto end() const noexcept { return m_entries.end(); }
    // Whether the table holds memory, and so has something to empty when the render ends
    [[nodiscard]] bool HasStorage() const noexcept { return m_entries.capacity() != 0; }
    [[nodiscard]] size_t capacity() const noexcept { return m_entries.capacity(); }

    // Empties the table; with `keepStorage`, keeps its memory for the next render unless it
    // holds more than `maxKept` entries
    void Clear(bool keepStorage, size_t maxKept) noexcept
    {
        if (!keepStorage || m_entries.capacity() > maxKept)
        {
            // Moved out first, so that the table is empty before a value's destructor runs
            const auto entries = std::move(m_entries);
            const auto index = std::move(m_index);
            m_entries = {};
            m_index = {};
        }
        else
        {
            m_entries.clear();
            m_index.clear();
        }
        m_mask = 0;
#if JINJA2CPP_FULL_CONTRACT_CHECKS
        m_reserved = 0;
#endif
    }

    friend bool operator==(const ParamsTable& lhs, const ParamsTable& rhs)
    {
        if (lhs.size() != rhs.size())
        {
            return false;
        }
        for (const auto& entry : lhs)
        {
            const auto* other = rhs.Find(entry.name);
            if (!other || !(*other == entry.value))
            {
                return false;
            }
        }
        return true;
    }

private:
    [[nodiscard]] const InternalValue* FindLinear(std::string_view name) const
    {
        for (const auto& entry : m_entries)
        {
            if (NameEqual::Equal(entry.name, name))
            {
                return &entry.value;
            }
        }
        return nullptr;
    }
    [[nodiscard]] InternalValue* FindLinearMutable(std::string_view name) { return const_cast<InternalValue*>(std::as_const(*this).FindLinear(name)); }

    // FindIndexed, checked against a scan of every entry at FULL contract checks
    [[nodiscard]] const InternalValue* CheckedIndexed(std::string_view name, size_t hash) const
    {
        const auto* found = FindIndexed(name, hash);
#if JINJA2CPP_FULL_CONTRACT_CHECKS
        JINJA2CPP_ASSERT(found == FindLinear(name));
#endif
        return found;
    }
    // An index of at least twice `count` positions: a probe ends at an empty one soon
    void MakeIndex(size_t count)
    {
        size_t positions = 16;
        while (positions < count * 2)
        {
            positions *= 2;
        }
        m_index.assign(positions, 0);
        m_mask = positions - 1;
    }
    [[nodiscard]] const InternalValue* FindIndexed(std::string_view name, size_t hash) const
    {
        for (size_t pos = hash & m_mask;; pos = (pos + 1) & m_mask)
        {
            const auto slot = m_index[pos];
            if (slot == 0)
            {
                return nullptr;
            }
            const auto& entry = m_entries[slot - 1];
            if (entry.hash == hash && NameEqual::Equal(entry.name, name))
            {
                return &entry.value;
            }
        }
    }
    void AddIndexed(std::string_view name, InternalValue value)
    {
        const size_t hash = HashedName::Hash(name);
        for (size_t pos = hash & m_mask;; pos = (pos + 1) & m_mask)
        {
            auto& slot = m_index[pos];
            if (slot == 0)
            {
#if JINJA2CPP_FULL_CONTRACT_CHECKS
                JINJA2CPP_ASSERT(m_entries.size() < m_reserved);
#endif
                m_entries.push_back({ name, std::move(value), hash });
                slot = static_cast<std::uint32_t>(m_entries.size());
                return;
            }
            auto& entry = m_entries[slot - 1];
            if (entry.hash == hash && NameEqual::Equal(entry.name, name))
            {
                entry.value = std::move(value);
                return;
            }
        }
    }

    std::vector<Entry> m_entries;
    // 0 for an empty position, else an entry's index plus one; empty up to LinearMax params
    std::vector<std::uint32_t> m_index;
    size_t m_mask = 0;
#if JINJA2CPP_FULL_CONTRACT_CHECKS
    size_t m_reserved = 0;
#endif
};
} // namespace v2
} // namespace jinja2

#endif // JINJA2CPP_SRC_PARAMS_TABLE_H
