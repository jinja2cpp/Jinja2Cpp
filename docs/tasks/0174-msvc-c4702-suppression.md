---
status: open
priority: low
area: build
touches: [CMakeLists.txt]
---
# `/wd4702` hides unreachable code in project code on MSVC

**Problem.** PR #478 (task 0082) turned on `/W4 /WX` for MSVC. Windows CI then reported
C4702 (unreachable code) in fmt's `fmt/base.h` (C++20 and C++23 Debug jobs). `/external:W0`
does not silence it: the code generator reports C4702 after the external-header filter no
longer applies. `CMakeLists.txt` therefore passes `/wd4702` to the whole library and test
target, which also hides a real C4702 in our own code, the one warning the task's
motivation named.

**Proposal.** Scope the suppression to fmt instead of the target, or drop it:
- wrap the `#include <fmt/...>` lines in `#pragma warning(push)` / `disable: 4702` /
  `pop` (ten include sites in `src/`; a single `src/fmt_include.h` wrapper would hold it); or
- drop `/wd4702` on the next fmt bump if the new fmt no longer triggers it, and check
  by building the three Windows jobs without it.

Either way, confirm on Windows CI that `/W4 /WX` still passes. The cloud cannot build with
MSVC, so this goes through CI.

**Done when** `/wd4702` is gone from `CMakeLists.txt` and Windows CI is green under `/W4 /WX`.
