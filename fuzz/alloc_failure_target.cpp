// The allocation-failure target (docs/tasks/0267, docs/api-2.0.md 2.1): the unit test of task 0253
// (test/allocation_failure_test.cpp) fails every allocation of a few fixed templates; this fails one allocation of
// the template the input chooses. The global operator new is replaced, so this file goes only into its own
// executables (jinja2cpp_fuzz_alloc_failure, jinja2cpp_fuzz_alloc_replay): the replacement would hide the
// sanitizers' own operator new from the other targets.
//
// Input: two bytes, little-endian, the number of allocations to let through before one fails; a settings byte
// (fuzz_common.h, narrow only); the template. The template first runs without a failure. Then a fresh template of the
// same environment loads and renders it with the failure armed, and each call must return or throw std::bad_alloc,
// never anything else. When the failure fired and no std::bad_alloc came out, the outcome must be the one of the
// first runs (compared only when three runs agree and nothing in it may vary: `random`, an address, a time limit): an engine that swallows the failure into another result breaks the contract. Afterwards the environment
// and the template must render as in the first run. With a Windows DLL build the library keeps its CRT's operator new,
// so no failure is injected there.

#include "fuzz_common.h"

#include <jinja2cpp/filesystem_handler.h>
#include <jinja2cpp/template.h>
#include <jinja2cpp/template_env.h>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <new>
#include <optional>
#include <string>

// NOLINTBEGIN(cppcoreguidelines-no-malloc): the replaced global allocation functions sit on malloc and free
namespace
{
// The number of allocations still to succeed before one fails; negative when no failure is armed
std::atomic<long> g_countdown{ -1 };

// MSVC's checked iterators give every container a heap proxy, made in noexcept constructors, so failing one ends
// in std::terminate: a debugging aid of the standard library, never failed (as in test/allocation_failure_test.cpp)
bool Injectable([[maybe_unused]] std::size_t size)
{
#if defined(_MSC_VER) && defined(_ITERATOR_DEBUG_LEVEL) && _ITERATOR_DEBUG_LEVEL != 0
    return size != sizeof(std::_Container_proxy);
#else
    return true;
#endif
}

void* Allocate(std::size_t size)
{
    if (g_countdown.load(std::memory_order_relaxed) >= 0 && Injectable(size) && g_countdown.fetch_sub(1, std::memory_order_relaxed) == 0)
    {
        throw std::bad_alloc();
    }
    if (void* p = std::malloc(size == 0 ? 1 : size))
    {
        return p;
    }
    throw std::bad_alloc();
}
} // namespace

void* operator new(std::size_t size)
{
    return Allocate(size);
}
void* operator new[](std::size_t size)
{
    return Allocate(size);
}
void* operator new(std::size_t size, const std::nothrow_t& /*tag*/) noexcept
{
    try
    {
        return Allocate(size);
    }
    catch (const std::bad_alloc&)
    {
        return nullptr;
    }
}
void* operator new[](std::size_t size, const std::nothrow_t& tag) noexcept
{
    return operator new(size, tag);
}
void operator delete(void* p) noexcept
{
    std::free(p);
}
void operator delete[](void* p) noexcept
{
    std::free(p);
}
void operator delete(void* p, std::size_t /*size*/) noexcept
{
    std::free(p);
}
void operator delete[](void* p, std::size_t /*size*/) noexcept
{
    std::free(p);
}
void operator delete(void* p, const std::nothrow_t& /*tag*/) noexcept
{
    std::free(p);
}
void operator delete[](void* p, const std::nothrow_t& /*tag*/) noexcept
{
    std::free(p);
}
// NOLINTEND(cppcoreguidelines-no-malloc)

namespace jinja2_fuzz
{
namespace
{
// Arms the failure for its scope
class Injection
{
public:
    explicit Injection(long succeed) { g_countdown.store(succeed, std::memory_order_relaxed); }
    ~Injection() { g_countdown.store(-1, std::memory_order_relaxed); }
    Injection(const Injection&) = delete;
    Injection& operator=(const Injection&) = delete;
    Injection(Injection&&) = delete;
    Injection& operator=(Injection&&) = delete;

    static bool Fired() { return g_countdown.load(std::memory_order_relaxed) < 0; }
};

// How a Load and Render ended: not loaded (a Load error), a Render error, or the output
struct Run
{
    bool loaded = false;
    std::optional<std::string> output;
    jinja2::ErrorCode code = jinja2::ErrorCode::Unspecified;

    bool operator==(const Run& other) const { return loaded == other.loaded && output == other.output && code == other.code; }
    bool operator!=(const Run& other) const { return !(*this == other); }
};

Run LoadAndRender(jinja2::Template& tpl, const std::string& source, bool load)
{
    Run run;
    if (load)
    {
        auto loaded = tpl.Load(source, "self");
        if (!loaded)
        {
            run.code = loaded.error().GetCode();
            return run;
        }
    }
    run.loaded = true;
    auto rendered = tpl.RenderAsString(MakeContext());
    if (rendered)
    {
        run.output = std::move(rendered.value());
    }
    else
    {
        run.code = rendered.error().GetCode();
    }
    return run;
}

// A render whose result may change between runs: the `random` filter, an address in the output (a bound method's
// repr), a time limit hit
bool Reproducible(const std::string& source, const Run& run)
{
    return source.find("random") == std::string::npos && run.code != jinja2::ErrorCode::ResourceLimitExceeded && (!run.output || run.output->find("0x") == std::string::npos);
}

// Equal, unless either hit a limit: maxRenderTime makes a render near it end either way
bool Same(const Run& a, const Run& b)
{
    return a == b || a.code == jinja2::ErrorCode::ResourceLimitExceeded || b.code == jinja2::ErrorCode::ResourceLimitExceeded;
}

[[noreturn]] void Fail(const char* what, long succeed)
{
    std::fprintf(stderr, "allocation failure target: %s (allocation %ld failing)\n", what, succeed);
    std::abort();
}
} // namespace

void FuzzAllocFailure(const std::uint8_t* data, std::size_t size)
{
    if (size < 3 || size > MaxInputSize)
    {
        return;
    }
    const long succeed = static_cast<long>(data[0]) | (static_cast<long>(data[1]) << 8);
    const auto settingsByte = static_cast<std::uint8_t>(data[2] & ~settings_byte::Wide);
    const std::string source(reinterpret_cast<const char*>(data) + 3, size - 3);

    jinja2::TemplateEnv env;
    ConfigureUntrustedEnv(env, settingsByte);
    auto fs = std::make_shared<jinja2::MemoryFileSystem>();
    AddSupportTemplates(*fs, source, DelimitersOf(settingsByte));
    env.AddFilesystemHandler(std::string(), fs);

    // Only a render that depends on its source alone can be compared; any other still runs with the failure armed
    jinja2::Template first(&env);
    const Run expected = LoadAndRender(first, source, true);
    bool comparable = Reproducible(source, expected);
    for (int n = 0; comparable && n < 2; ++n)
    {
        jinja2::Template other(&env);
        comparable = Same(LoadAndRender(other, source, true), expected);
    }

    jinja2::Template tpl(&env);
    Run injected;
    bool threw = false;
    bool fired = false;
    {
        Injection injection(succeed);
        try
        {
            injected = LoadAndRender(tpl, source, true);
        }
        catch (const std::bad_alloc&)
        {
            threw = true;
        }
        fired = Injection::Fired();
    }
    if (!comparable)
    {
        return;
    }
    if (fired && !threw && !Same(injected, expected))
    {
        Fail("the failure was turned into another outcome", succeed);
    }
    // The template and the environment stay usable
    if (injected.loaded && !Same(LoadAndRender(tpl, source, false), expected))
    {
        Fail("the template renders differently after the failure", succeed);
    }
    jinja2::Template again(&env);
    if (!Same(LoadAndRender(again, source, true), expected))
    {
        Fail("the environment renders differently after the failure", succeed);
    }
}

} // namespace jinja2_fuzz
