// libFuzzer entry point; the work is in untrusted_targets.cpp (see fuzz/README.md).
#include "fuzz_common.h"

#include <cstddef>
#include <cstdint>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size)
{
    jinja2_fuzz::FuzzUntrusted(data, size);
    return 0;
}
