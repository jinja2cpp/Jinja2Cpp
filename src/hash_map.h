#ifndef JINJA2CPP_SRC_HASH_MAP_H
#define JINJA2CPP_SRC_HASH_MAP_H

// The hash tables of the library: the vendored ankerl::unordered_dense 5.3.3 (src/ankerl/, MIT,
// docs/tasks/0231), moved into jinja2::v2::detail (0259), behind names of its own so that a
// later change of map touches only this file

#include "ankerl/unordered_dense.h"

#include <jinja2cpp/config.h>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>

#ifndef JINJA2CPP_STABLE_MAP_SEGMENT_BYTES
#define JINJA2CPP_STABLE_MAP_SEGMENT_BYTES 512
#endif

namespace jinja2
{
inline namespace v2
{
namespace detail
{
// The scope maps compare names inline (NameEqual): the probe past the home group stays inline
// too. Out of line it takes the key by reference, which gives every function that looks up a
// name a stack protector (docs/tasks/0231)
template<>
struct ankerl::unordered_dense::detail::key_compare_is_call<std::string> : std::false_type
{
};

// The key of the name hash, drawn once per process. The hash of unordered_dense is unseeded
// and its index never rehashes a crowded group, so without a key names found offline to share
// one group cost each set or lookup a scan of all of them, in every process (security review
// of #891, docs/tasks/0231). Zero until drawn: the first hash draws it (hash_map.cpp), whichever
// thread and static initializer that is. Exported, so that a test linked to the shared library
// hashes as the library does
extern JINJA2CPP_EXPORT std::atomic<std::uint64_t> NameHashKeyValue; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
JINJA2CPP_EXPORT std::uint64_t DrawNameHashKey() noexcept;

inline std::uint64_t NameHashKey() noexcept
{
    const auto key = NameHashKeyValue.load(std::memory_order_relaxed);
    return key != 0 ? key : DrawNameHashKey();
}

// The library's hash of a name: 64 bits, avalanching, keyed per process. Multiplying the keyed
// hash by an odd constant is a bijection, and folding the high half down makes the low byte
// (the group's fingerprint) depend on all of it
inline std::size_t HashName(const void* data, std::size_t size) noexcept
{
    auto x = (ankerl::unordered_dense::detail::hash_bytes(data, size) ^ NameHashKey()) * UINT64_C(0xd6e8feb86659fd93);
    x ^= x >> 32;
    return static_cast<std::size_t>(x);
}

// The index of the lowest set bit of a nonzero `bits`
inline unsigned CountTrailingZeros(std::uint32_t bits) noexcept
{
    return ankerl::unordered_dense::detail::countr_zero(bits);
}

// A map whose values stay where they are while others are added: they live in segments
// of JINJA2CPP_STABLE_MAP_SEGMENT_BYTES that never move. An erase moves the last value
// into the gap, so a caller that keeps pointers into the map forgets that one too
template<typename Key, typename T, typename Hash, typename KeyEqual>
using StableMap = ankerl::unordered_dense::segmented_map<Key,
                                                         T,
                                                         Hash,
                                                         KeyEqual,
                                                         std::allocator<std::pair<Key, T>>,
                                                         ankerl::unordered_dense::bucket_type::group,
                                                         JINJA2CPP_STABLE_MAP_SEGMENT_BYTES>;

// Whether a map keeps its values in place while others are added (StableMap)
template<typename Map>
struct KeepsValuesInPlace : std::false_type
{
};
template<typename Key, typename T, typename Hash, typename KeyEqual, typename Values, typename Bucket>
struct KeepsValuesInPlace<ankerl::unordered_dense::detail::table<Key, T, Hash, KeyEqual, Values, Bucket, true>> : std::true_type
{
};

// Whether `map` holds memory: it had values, and keeps their storage while it is not moved
// from. Cheaper than bucket_count(), for the checks on every render and scope
template<typename Map>
inline bool HasStorage(const Map& map) noexcept
{
    return map.values().capacity() != 0;
}

// A map that may move its values on any insert or erase
template<typename Key, typename T, typename Hash = ankerl::unordered_dense::hash<Key>>
using FlatMap = ankerl::unordered_dense::map<Key, T, Hash>;

template<typename Key, typename Hash = ankerl::unordered_dense::hash<Key>>
using FlatSet = ankerl::unordered_dense::set<Key, Hash>;
} // namespace detail
} // namespace v2
} // namespace jinja2

#endif // JINJA2CPP_SRC_HASH_MAP_H
