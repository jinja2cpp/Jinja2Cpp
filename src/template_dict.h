#ifndef JINJA2CPP_SRC_TEMPLATE_DICT_H
#define JINJA2CPP_SRC_TEMPLATE_DICT_H

#include "internal_value.h"
#include "recursion_guard.h"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>

namespace jinja2
{
inline namespace v2
{
// The dict a template owns (dict literals, dict(), namespace(), the result of the dict methods):
// Python keys, which are any hashable value, over the string keys of InternalDict (docs/tasks/0036).
//
// A string key is stored as itself. Any other hashable key is stored under a canonical token
// (MakeDictKey): one binary string that is equal for keys Python finds equal (1, 1.0 and True;
// (1, 2.0) and (1.0, 2)), so the dict needs no custom hash or comparison and a lookup is one
// find. A token cannot be turned back into a key (a float would have to be parsed), so the
// first spelling of each key is kept in a side table, allocated only when the dict has a
// key that is not a string. Iteration reads the keys from it.
//
// Every key goes in through Set or SetString; Items() is const so that no code can store a raw
// string under a token's name.

// The first byte of every token, a byte that is not valid in UTF-8. A string that starts with
// it (host data that is not valid UTF-8) is escaped, so a token is never a plain string
inline constexpr char DictKeyMark = static_cast<char>(0xFF);

// The type Python's hash() rejects in a value, if any: a list or a dict, also inside a tuple.
// A range hashes, and so does an object stored as a map (namespace(), cycler()) or a host object
// (a reflected struct, which Python would hold as an object, or a JSON object)
const char* UnhashableType(const InternalValue& value);

// The token of a key, which is equal for the keys Python treats as one. Nullopt when the key
// cannot be hashed, with the type to report in `unhashableType`: Python's TypeError
// `unhashable type: 'list'`. Objects are keyed by identity, as Python's default hash has it: an
// object of a host language (host_object.h) by IEngineHostObject::Identity (docs/tasks/0352), the
// engine's objects (namespace(), cycler(), joiner(), reflected structs, functions, macros and
// bound methods) by the identity their copies share (docs/tasks/0212). Every undefined value is
// one key, and a StrictUndefined one throws UndefinedError
std::optional<std::string> MakeDictKey(const InternalValue& key, const char** unhashableType = nullptr);

// Whether two receivers of a method (BoundMethodCall::self) make it the same method, as Python
// compares builtin methods: a container by identity, anything else by value (Callable::IsEqual)
bool SameMethodReceiver(const InternalValue& left, const InternalValue& right);

// Throws Python's TypeError for a key MakeDictKey rejects
[[noreturn]] void ThrowUnhashable(const char* type);

// str(key): how a mapping that holds only string keys (ValuesMap) spells a key it cannot keep
std::string DictKeyText(const InternalValue& key);

class TemplateDict
{
public:
    using iterator = InternalDict::iterator;
    using const_iterator = InternalDict::const_iterator;

    TemplateDict() = default;
    // A dict of string keys, such as the names of keyword arguments; a name that starts with the mark is escaped
    explicit TemplateDict(InternalDict&& items);
    TemplateDict(const TemplateDict& other);
    // OrderedMap's moves allocate on MSVC (see ordered_map.h), so these are noexcept only where its are. The entries
    // move with their count against the render's memory budget: OrderedMap's moves leave the map moved from empty
    TemplateDict(TemplateDict&& other) noexcept(std::is_nothrow_move_constructible_v<InternalDict>) = default;
    // No assignment: entries a dict took over would carry no epoch, and a walk of the dict could take one of them for
    // the entry it stopped at (docs/tasks/0553). A dict is made, changed through its methods and freed
    TemplateDict& operator=(const TemplateDict& other) = delete;
    TemplateDict& operator=(TemplateDict&& other) = delete;
    ~TemplateDict() { ReleaseEntries(); }

    // The entries under their tokens: for the count and the values. Read the keys through KeyOf
    [[nodiscard]] const InternalDict& Items() const { return m_items; }
    [[nodiscard]] size_t size() const { return m_items.size(); }
    [[nodiscard]] bool empty() const { return m_items.empty(); }
    [[nodiscard]] iterator begin() { return m_items.begin(); }
    [[nodiscard]] iterator end() { return m_items.end(); }
    [[nodiscard]] const_iterator begin() const { return m_items.begin(); }
    [[nodiscard]] const_iterator end() const { return m_items.end(); }
    // Whether some key is not a string, which the public (string keyed) view has to spell
    [[nodiscard]] bool HasKeyValues() const { return m_keys && !m_keys->keys.empty(); }

    // How a walk tells an entry it stopped at from one added again under the same key (docs/tasks/0553): the erase
    // epoch counts every Erase and Clear, and an entry added while it is past 0 is noted with it. An
    // address a walk kept can be the address of an entry added since (the allocator reuses it); the entry is the same
    // one only if it was not added after the epoch the walk saw
    [[nodiscard]] std::uint64_t EraseEpoch() const noexcept { return m_keys.Epoch(); }
    // The epoch at which the entry was added: 0 for an entry added before the first erase
    [[nodiscard]] std::uint64_t AddedEpoch(const_iterator pos) const noexcept;

    // A string key; end() when absent
    [[nodiscard]] const_iterator FindString(const std::string& name) const;
    [[nodiscard]] iterator FindString(const std::string& name);
    // Any key; end() when absent or unhashable (the caller checks UnhashableType where Python raises)
    [[nodiscard]] const_iterator Find(const InternalValue& key) const;
    [[nodiscard]] iterator Find(const InternalValue& key);

    // Python's d[key] = value: a new key is appended with this spelling, an existing key (the
    // same token) keeps its position and its first spelling and takes the value. Throws
    // TypeError for an unhashable key
    iterator Set(const InternalValue& key, InternalValue value);
    iterator SetString(std::string name, InternalValue value);
    // Appends the key if it is new and keeps the value of one that exists (setdefault)
    std::pair<iterator, bool> TryEmplace(const InternalValue& key, InternalValue value);

    iterator Erase(const_iterator pos);
    void Clear();

    // The key of a stored entry as a template value: the string, or the first spelling of a
    // non-string key. An entry that starts with the mark and has no spelling reads as a plain string
    [[nodiscard]] InternalValue KeyOf(const std::string& stored) const;
    // Passes fn the first spelling of each key that is not a string (the cycle collector's walk)
    template<typename Fn>
    void ForEachKeyValue(const Fn& fn) const
    {
        if (m_keys)
        {
            for (const auto& entry : m_keys->keys)
            {
                fn(entry.second);
            }
        }
    }

    // Dicts are equal when they hold the same entries, in any order. Tokens are canonical, so
    // comparing the entries compares the keys
    friend bool operator==(const TemplateDict& lhs, const TemplateDict& rhs) { return lhs.m_items == rhs.m_items; }
    friend bool operator!=(const TemplateDict& lhs, const TemplateDict& rhs) { return !(lhs == rhs); }

private:
    using KeyTable = std::unordered_map<std::string, InternalValue>;

    // What a dict keeps beside its entries, allocated only when needed: the first spellings of the keys that are not
    // strings, the dict's erase epoch, and the epoch at which each entry added after an erase was added
    // (docs/tasks/0553)
    struct SideTable
    {
        KeyTable keys;
        std::uint64_t epoch = 0;
        // Entry address (a number, never used to reach the entry) to the epoch at which it was added
        std::unordered_map<std::uintptr_t, std::uint64_t> born;
    };

    // Owns the side table, as std::unique_ptr<SideTable> does, and keeps the dict's "counted" flag in the low bit of
    // the pointer, so that the flag costs no byte (docs/tasks/0483): set when the dict's entries were all counted
    // against a render's memory budget, and only then does it give them back. Without a table the other bits hold the
    // erase epoch, so that erasing and clearing count without allocating (docs/tasks/0553)
    class SideHolder
    {
    public:
        SideHolder() noexcept = default;
        SideHolder(std::unique_ptr<SideTable> table) noexcept           // NOLINT(google-explicit-constructor): as a unique_ptr
            : m_bits(reinterpret_cast<std::uintptr_t>(table.release())) // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
        {
        }
        SideHolder(const SideHolder&) = delete;
        SideHolder& operator=(const SideHolder&) = delete;
        SideHolder(SideHolder&& other) noexcept
            : m_bits(std::exchange(other.m_bits, 0))
        {
        }
        SideHolder& operator=(SideHolder&& other) noexcept
        {
            if (this != &other)
            {
                Destroy();
                m_bits = std::exchange(other.m_bits, 0);
            }
            return *this;
        }
        ~SideHolder() { Destroy(); }

        void swap(SideHolder& other) noexcept { std::swap(m_bits, other.m_bits); }
        [[nodiscard]] SideTable* get() const noexcept
        {
            return (m_bits & InlineBit) != 0 ? nullptr : reinterpret_cast<SideTable*>(m_bits & ~CountedBit); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast,performance-no-int-to-ptr)
        }
        explicit operator bool() const noexcept { return get() != nullptr; }
        SideTable& operator*() const noexcept { return *get(); }
        SideTable* operator->() const noexcept { return get(); }
        // Takes a table in place of the current one, keeping the flag and the epoch
        void Assign(std::unique_ptr<SideTable> table) noexcept
        {
            table->epoch = Epoch();
            const auto counted = Counted();
            Destroy();
            m_bits = reinterpret_cast<std::uintptr_t>(table.release()) | (counted ? CountedBit : 0); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
        }
        // Drops the table, the flag and the epoch
        void Reset() noexcept
        {
            Destroy();
            m_bits = 0;
        }
        [[nodiscard]] bool Counted() const noexcept { return (m_bits & CountedBit) != 0; }
        void SetCounted(bool counted) noexcept { m_bits = counted ? (m_bits | CountedBit) : (m_bits & ~CountedBit); }
        [[nodiscard]] std::uint64_t Epoch() const noexcept
        {
            if (const auto* table = get())
            {
                return table->epoch;
            }
            return static_cast<std::uint64_t>(m_bits >> EpochShift);
        }
        void SetEpoch(std::uint64_t epoch) noexcept
        {
            if (auto* table = get())
            {
                table->epoch = epoch;
            }
            else
            {
                // An epoch past what the bits hold (2^62 erases on 64 bits, 2^30 on 32) stays at the largest one
                constexpr std::uint64_t maxInline = std::numeric_limits<std::uintptr_t>::max() >> EpochShift;
                const auto value = static_cast<std::uintptr_t>(epoch < maxInline ? epoch : maxInline);
                m_bits = (value << EpochShift) | (value != 0 ? InlineBit : 0) | (m_bits & CountedBit);
            }
        }

    private:
        static constexpr std::uintptr_t CountedBit = 1;
        static constexpr std::uintptr_t InlineBit = 2;
        static constexpr unsigned EpochShift = 2;
        static_assert(alignof(SideTable) >= 4, "the two low bits of a table's address are free");
        void Destroy() const noexcept { delete get(); } // NOLINT(cppcoreguidelines-owning-memory)

        std::uintptr_t m_bits = 0;
    };

    // The render's memory budget (docs/tasks/0459) counts each entry from when it is added until it is erased or
    // the dict is freed, by its stored key alone, so that adding and freeing count the same
    static std::uint64_t EntryBytes(const std::string& stored) noexcept;
    // Counts the entries of a dict just made (every one of them) and, when a budget is active, marks the dict counted
    void ChargeEntries()
    {
        if (RenderMemoryCounted())
        {
            ChargeEntriesSlow();
            m_keys.SetCounted(true);
        }
    }
    void ChargeEntriesSlow() const;
    // Gives the entries back if every one of them was counted: those of a dict that held entries when a render first
    // counted it, or that a render added while no budget was active, stay uncounted and are never credited
    void ReleaseEntries() const noexcept
    {
        if (m_keys.Counted() && RenderMemoryCounted())
        {
            ReleaseEntriesSlow();
        }
    }
    void ReleaseEntriesSlow() const noexcept;

    // Notes that the entry was added at the current epoch, when it is past 0 (docs/tasks/0553). Throws when the
    // note cannot be allocated: the caller takes the entry out again
    void StampAdded(iterator pos)
    {
        if (m_keys.Epoch() != 0)
        {
            StampAddedSlow(pos);
        }
    }
    void StampAddedSlow(iterator pos);
    // What the render's memory budget counts for one note: a node of the table and its bucket
    static constexpr std::uint64_t NoteNode = sizeof(std::pair<const std::uintptr_t, std::uint64_t>);
    static constexpr std::uint64_t NoteBucket = 3 * sizeof(void*);
    static constexpr std::uint64_t NoteBytes = NoteNode + NoteBucket;

    InternalDict m_items;
    // Token to first spelling, for the entries that are not stored under their own string, and the epochs; also the
    // dict's counted flag (docs/tasks/0483), which a move takes along with the entries
    SideHolder m_keys;
};
// The flag adds no member: the dict is its entries and one pointer, as before docs/tasks/0483
namespace detail
{
struct TemplateDictLayout
{
    InternalDict items;
    void* keys;
};
} // namespace detail
static_assert(sizeof(TemplateDict) == sizeof(detail::TemplateDictLayout), "TemplateDict grew");

} // namespace v2
} // namespace jinja2

#endif // JINJA2CPP_SRC_TEMPLATE_DICT_H
