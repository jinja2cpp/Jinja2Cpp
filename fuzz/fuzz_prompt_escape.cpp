// libFuzzer entry point; the work is in fuzz_targets.cpp and prompt_escape_case.h (see fuzz/README.md).
#include "fuzz_common.h"

#include <cstddef>
#include <cstdint>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size)
{
    jinja2_fuzz::FuzzPromptEscape(data, size);
    return 0;
}
