---
status: in-progress
priority: medium
area: build
depends: []
touches: [src/out_stream.cpp]
---
# The GCC address+undefined build fails in `OutStream::WriteInt`

**Problem.** Found by the verifier of the 0159 PR. A GCC build with
`-DJINJA2CPP_WITH_SANITIZERS=address+undefined -DCMAKE_BUILD_TYPE=RelWithDebInfo` stops in
`src/out_stream.cpp:138` (`WriteInt`, the `put` lambda's `memcpy`) with
`-Werror=stringop-overflow`. The same code builds with clang-18 and with GCC in Debug.
The file is untouched by 0159, so the failure is on master. GCC does not tie the digits
written from the end of the buffer to the `length` counted above, and the sanitizer
instrumentation makes it prove a bound it cannot. CLAUDE.md asks to keep the library
warning-free rather than turn warnings off, so the fix belongs in the code.

**Proposal.** Reproduce with the compiler of the CI sanitizer job (gcc 13/14) first; CI's
sanitizer matrix runs clang-18, so this is not red there today but any GCC user of the
sanitizer configuration is blocked. Restructure the loop so the bound is visible (write
into a local `char[20]` and `memcpy` the finished digits, or assert `length <= 21` for
the optimiser), and measure `Render/strings` with `preflight --perf`, since `WriteInt` is
on the output path (0113). Do not add a pragma or `-Wno-stringop-overflow`.

**Done when** the GCC address+undefined RelWithDebInfo build is warning-free and the
instruction gate shows no regression; consider adding a GCC job to the sanitizer matrix.
