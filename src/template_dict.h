#ifndef JINJA2CPP_SRC_TEMPLATE_DICT_H
#define JINJA2CPP_SRC_TEMPLATE_DICT_H

#include "internal_value.h"
#include "recursion_guard.h"

#include <cstddef>
#include <cstdint>
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
    // A dict of string keys, such as the names of keyword arguments, which never start with the mark
    explicit TemplateDict(InternalDict&& items);
    TemplateDict(const TemplateDict& other);
    // OrderedMap's moves allocate on MSVC (see ordered_map.h), so these are noexcept only where its are. The entries
    // move with their count against the render's memory budget: OrderedMap's moves leave the map moved from empty
    TemplateDict(TemplateDict&& other) noexcept(std::is_nothrow_move_constructible_v<InternalDict>) = default;
    TemplateDict& operator=(const TemplateDict& other);
    TemplateDict& operator=(TemplateDict&& other) noexcept(std::is_nothrow_move_assignable_v<InternalDict>);
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
    [[nodiscard]] bool HasKeyValues() const { return m_keys && !m_keys->empty(); }

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
            for (const auto& entry : *m_keys)
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

    // The render's memory budget (docs/tasks/0459) counts each entry from when it is added until it is erased or
    // the dict is freed, by its stored key alone, so that adding and freeing count the same
    static std::uint64_t EntryBytes(const std::string& stored) noexcept;
    void ChargeEntries() const
    {
        if (RenderMemoryCounted())
        {
            ChargeEntriesSlow();
        }
    }
    void ChargeEntriesSlow() const;
    void ReleaseEntries() const noexcept
    {
        if (m_counted && RenderMemoryCounted())
        {
            ReleaseEntriesSlow();
        }
    }
    void ReleaseEntriesSlow() const noexcept;

    InternalDict m_items;
    // Whether a budget was active when the dict was made: only then are its entries given back, those a render
    // adds to a dict made without one stay counted (docs/tasks/0483). Moves take the entries and the flag together
    bool m_counted = RenderMemoryCounted();
    // Token to first spelling, for the entries that are not stored under their own string
    std::unique_ptr<KeyTable> m_keys;
};

} // namespace v2
} // namespace jinja2

#endif // JINJA2CPP_SRC_TEMPLATE_DICT_H
