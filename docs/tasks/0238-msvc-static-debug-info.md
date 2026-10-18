---
status: open
priority: low
area: build
issues: ["https://github.com/jinja2cpp/Jinja2Cpp/issues/224"]
touches: [CMakeLists.txt#msvc-debug-info]
---
# Installed MSVC static Debug library has no usable debug information

**Problem.** Only CI sets `CMAKE_MSVC_DEBUG_INFORMATION_FORMAT=Embedded`
(`.github/workflows/windows-build.yml`). `CMakeLists.txt` sets no default and installs no
PDB, so a consumer of an installed static Debug library gets LNK4099 and cannot step into
the library (#224). 0079 and 0082 do not cover it.

**Proposal.** When the user has not set it, default the static library to `Embedded`
(`/Z7`), or install the compiler PDB next to the library; pick the one that works for
both the multi-config and single-config generators.

**Done when.** A consumer of an installed static Debug build links without LNK4099, checked
on Windows by the 0225 consumer check.
