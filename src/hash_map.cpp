#include "hash_map.h"

#include <chrono>
#include <cstdint>
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
    const std::uint64_t parts[] = { key, clock, here }; // NOLINT(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
    return ankerl::unordered_dense::detail::hash_bytes(parts, sizeof(parts));
#endif
}
} // namespace

std::uint64_t NameHashKey() noexcept
{
    static const std::uint64_t key = MakeNameHashKey();
    return key;
}
} // namespace detail
} // namespace v2
} // namespace jinja2
