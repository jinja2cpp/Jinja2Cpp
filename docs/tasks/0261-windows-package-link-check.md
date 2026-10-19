---
status: open
priority: low
area: ci
depends: [0076]
touches: [.github/workflows/windows-build.yml, test/package/]
---
# Run the installed-package test and the ABI link probe on Windows

**Problem.** test/package (the installed-package build, check_package.cmake) and, since
0076, its negative link probe (a consumer declaring `jinja2::operator==` outside the `v2`
inline namespace must fail to link) run only in Linux rows and the Conan job. MSVC mangles
the inline namespace too, so the probe should fail there as well, and the DLL import
library path (`JINJA2CPP_LINK_AS_SHARED`, dllimport) is the one most likely to differ, but
nothing checks it. Ruslan decided on 2026-10-18 (0076 plan, Q3) that this is a follow-up,
not a 2.0 release gate.

**Proposal.** Add a package step to one static and one shared row of windows-build.yml:
install into a temporary prefix, build test/package against it with `CONSUMER_STD`, run it,
and run the probe test. Keep runner minutes in mind (docs/tasks/0081 trimmed Windows to
three jobs): reuse an existing row rather than adding one.

**Done when.** A Windows static and a Windows shared row build and run test/package, and
the v1 probe fails to link there.
