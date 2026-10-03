---
status: open
priority: low
area: build
touches: [CMakeLists.txt]
---
# MSVC never gets the strict warning flags

**Problem.** `JINJA2CPP_STRICT_WARNINGS` sets `MSVC_CXX_FLAGS` to `/W4` inside
`if (UNIX)` (`CMakeLists.txt`, the block after `include(thirdparty/CMakeLists.txt)`), so
an MSVC build never sees it, and nothing passes `/WX`. Warnings that only MSVC reports
(C4702 unreachable code, C4244 narrowing) land unnoticed, while GCC and Clang fail on
`-Wall -Werror`.

**Proposal.** Move the MSVC line out of `if (UNIX)`, measure what `/W4` reports on the
Windows CI jobs, fix or suppress with a reason, then add `/WX` so the three compilers are
held to the same bar.

**Done when** Windows CI builds the library with `/W4 /WX`.
