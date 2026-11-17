#include "hash_map.h"
#include "recursion_guard.h"

#include <cstddef>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <exception>
#include <random>

namespace jinja2
{
inline namespace v2
{
namespace detail
{
namespace
{
namespace hash_impl = ankerl::unordered_dense::detail::hash_impl;

std::uint64_t MakeNameHashKey() noexcept
{
#ifdef JINJA2CPP_NAME_HASH_KEY
    // A fixed key, for a build that must hash the same in every run (a fuzz reproducer)
    return static_cast<std::uint64_t>(JINJA2CPP_NAME_HASH_KEY);
#else
    std::uint64_t key = 0;
    try
    {
        std::random_device device;
        key = (static_cast<std::uint64_t>(device()) << 32) ^ device();
    }
    catch (const std::exception&) // NOLINT(bugprone-empty-catch): the clock and the address below still vary the key
    {
    }
    // Where the library is loaded and when the key is drawn, in case the device is
    // missing or deterministic
    const auto clock = static_cast<std::uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count());
    const auto here = static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(&key)); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
    const std::uint64_t parts[] = { key, clock, here };                                   // NOLINT(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
    return ankerl::unordered_dense::detail::hash_bytes(parts, sizeof(parts));
#endif
}

// The secrets of the name hash, drawn once per process. hash_bytes xors fixed secrets into its
// lanes, and a lane whose input equals its secret multiplies by zero and drops the bytes it
// covers: every key of 8 to 16 bytes that starts with the eight bytes of secret[1] hashes alike,
// and the block and chained lanes past 16 bytes have the same weakness. No key applied after the
// hash separates names whose unkeyed hashes are equal, and host keys (render params) may hold any
// bytes, so the secrets themselves are keyed: a lane drops its bytes only for an input nobody
// outside the process knows (docs/tasks/0507). The shape and the reads stay hash_bytes's
struct NameHashSecrets
{
    // seed: the second word of the short path. lane: one per 8 byte word of a 16 byte block,
    // the blocks of the middle lengths in turn. length: the finalizer's. chain: links the 144
    // byte pieces of a longer name
    std::uint64_t seed;
    std::array<std::uint64_t, 18> lane;
    std::uint64_t length;
    std::uint64_t chain;
};

// Filled once by FillSecrets, before Drawn is set; read only once Drawn is seen set
NameHashSecrets Secrets{};        // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
std::atomic<bool> Drawn{ false }; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

// splitmix64: distinct, well mixed words from one key, none of which tells the others or the key
std::uint64_t NextSecret(std::uint64_t& state) noexcept
{
    state += UINT64_C(0x9e3779b97f4a7c15);
    auto z = state;
    z = (z ^ (z >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    z = (z ^ (z >> 27)) * UINT64_C(0x94d049bb133111eb);
    return z ^ (z >> 31);
}

bool FillSecrets() noexcept
{
    auto state = MakeNameHashKey();
    Secrets.seed = NextSecret(state);
    for (auto& lane : Secrets.lane)
    {
        lane = NextSecret(state);
    }
    Secrets.length = NextSecret(state);
    Secrets.chain = NextSecret(state);
    return true;
}

// The first hash of the process. Apart, so that HashName saves no registers for the call. The
// function-local static fills the secrets exactly once, whichever threads hash first
JINJA2CPP_NOINLINE std::size_t HashNameDrawingSecrets(const void* data, std::size_t size) noexcept
{
    static const bool filled = FillSecrets();
    static_cast<void>(filled);
    Drawn.store(true, std::memory_order_release);
    return HashName(data, size);
}

// hash_bytes with the process's secrets, over 17 to 144 bytes: one block of 16 bytes and the
// last 16, then whole blocks from the front, all independent. `nameSize` is the length of the
// whole name, which the finalizer takes
std::uint64_t HashBlocks(const std::uint8_t* p, std::size_t len, std::size_t nameSize, const NameHashSecrets& keys) noexcept
{
    using hash_impl::mix;
    using hash_impl::r8;
    const auto& lane = keys.lane;
    auto x = mix(r8(p) ^ lane[0], r8(p + 8) ^ lane[1]) ^ mix(r8(p + len - 16) ^ lane[2], r8(p + len - 8) ^ lane[3]); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    // Blocks 16 to 127 of a longer name, each with its own pair of lanes
    for (std::size_t offset = 16, idx = 4; offset + 16 < len; offset += 16, idx += 2)
    {
        x ^= mix(r8(p + offset) ^ lane[idx], r8(p + offset + 8) ^ lane[idx + 1]); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic,cppcoreguidelines-pro-bounds-constant-array-index)
    }
    return mix(keys.length ^ nameSize, x);
}

// The longest piece HashBlocks takes
constexpr std::size_t MaxBlockedBytes = 144;

// Names past 16 bytes. Apart, so that HashName, which the short names take, saves no registers.
// Past 144 bytes in pieces of 144, each linked to the hash of those before it
JINJA2CPP_NOINLINE std::size_t HashLongName(const void* data, std::size_t size, const NameHashSecrets& keys) noexcept
{
    const auto* bytes = static_cast<const std::uint8_t*>(data);
    const auto nameSize = size;
    std::uint64_t state = 0;
    while (size > MaxBlockedBytes)
    {
        state = hash_impl::mix(state ^ keys.chain, HashBlocks(bytes, MaxBlockedBytes, MaxBlockedBytes, keys) ^ keys.seed);
        bytes += MaxBlockedBytes; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        size -= MaxBlockedBytes;
    }
    if (size <= 16)
    {
        // A last piece of 1 to 16 bytes is read with the 16 before it, which the piece before
        // hashed too: the length of the whole name keeps it apart from a name 16 bytes longer
        bytes -= 16; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        size += 16;
    }
    return static_cast<std::size_t>(HashBlocks(bytes, size, nameSize, keys) ^ state);
}
} // namespace

std::size_t HashName(const void* data, std::size_t size) noexcept
{
    if (!Drawn.load(std::memory_order_acquire))
    {
        return HashNameDrawingSecrets(data, size);
    }
    const auto& keys = Secrets;
    if (size > 16)
    {
        return HashLongName(data, size, keys);
    }
    // hash_bytes's short path: two words that cover the name, each xored with a secret, then
    // the finalizer. The product is zero only for a word equal to its secret
    const auto* p = static_cast<const std::uint8_t*>(data);
    std::uint64_t a = 0;
    std::uint64_t b = 0;
    if (size >= 8)
    {
        a = hash_impl::r8(p);
        b = hash_impl::r8(p + size - 8); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    }
    else if (size >= 4)
    {
        a = hash_impl::r4(p);
        b = hash_impl::r4(p + size - 4); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    }
    else if (size > 0)
    {
        a = hash_impl::r3(p, size);
    }
    return static_cast<std::size_t>(hash_impl::mix(keys.length ^ size, hash_impl::mix(a ^ keys.lane[0], b ^ keys.seed)));
}
} // namespace detail
} // namespace v2
} // namespace jinja2
