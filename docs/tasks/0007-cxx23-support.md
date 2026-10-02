---
status: open
priority: medium
area: standards
depends: [5]
---
# C++23 support

**Problem.** The library builds as C++14/17/20. C++23 changes things the code relies on
(e.g. `std::expected` arrives in the standard while the API uses `nonstd::expected`;
deprecations and stricter rules around `char8_t`, `u8` literals and implicit moves).
Nobody has tried it, so users on C++23 find the breakage first.

**Proposal.** Add `23` to `JINJA2CPP_CXX_STANDARD`, add C++23 rows to the CI matrix (0005)
on the newest GCC and Clang, fix what breaks. Decide whether nonstd shims should map to
`std::` types under C++23, as they already do for C++17 where available.

**Done when.** C++23 rows in CI are green on GCC and Clang (MSVC with `/std:c++latest`).

**Superseded (2026-10-02).** C++23 joins C++17 and C++20 as a supported standard in 2.0; the work is task 0070.
