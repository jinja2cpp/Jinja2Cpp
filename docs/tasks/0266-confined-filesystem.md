---
status: open
priority: medium
area: api
touches: [include/jinja2cpp/filesystem_handler.h, src/filesystem_handler.cpp, test/filesystem_handler_test.cpp]
---
# `ConfinedFileSystem`: a loader that never leaves its root

**Decided (Ruslan 2026-10-18, untrusted-input review S4: "both").** `RealFileSystem` follows
symlinks, as Python's `FileSystemLoader` does, and documents it. 2.0 also ships a separate strict
loader for hosts whose template root may hold links they do not control.

**Proposal.** A new class (additive; a flag on `RealFileSystem` would change its layout).
Checking a resolved path and then opening it by name is a check/use race: a link swapped
between the two opens a file outside the root (Codex review on #584). So the check must apply to
what is actually opened:
- POSIX: walk the name one component at a time from a directory descriptor of the root with
  `openat(..., O_NOFOLLOW | O_CLOEXEC)` (or `openat2` with `RESOLVE_BENEATH` on Linux 5.6+),
  refuse `..` and absolute names before the walk, `fstat` the final descriptor (regular file
  only), and read from that descriptor.
- Windows: open with `CreateFileW` and `FILE_FLAG_OPEN_REPARSE_POINT` per component, or open
  normally and check `GetFinalPathNameByHandleW` of the opened handle against the root's final
  path, then read from that handle; refuse junctions and reparse points outside the root.
- Reading from a descriptor or handle needs our own `std::streambuf` over it; the standard
  streams cannot be opened from one portably.
- No host path in error text. Decide on Windows short names (`PROGRA~1`) and case folding.
  Compiled templates are cached, so the extra system calls are a Load cost only.
Until this lands, docs must not promise confinement against links an attacker can change.

**Tests.** Temp-dir table (plus a stress row: a thread swapping a link between an inside and an
outside target while another loads through it; the outside canary must never be read): a symlink inside the root pointing outside is not found; a symlink
pointing inside renders; a junction on Windows is not found; a FIFO is not opened.

**Done when.** The class ships in 2.0, is exported, listed in fwd.h/jinja2cpp.h (0076), and
docs/security.md (0224) recommends it for tier 1.
