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

**Proposal.** A new class (additive; a flag on `RealFileSystem` would change its layout):
resolve the name with `weakly_canonical`, refuse any result outside the canonical root (symlinks,
Windows junctions and reparse points included), regular files only, no host path in error text.
Decide on Windows short names (`PROGRA~1`) and case folding. One extra system call per open;
compiled templates are cached, so this is a Load cost only.

**Tests.** Temp-dir table: a symlink inside the root pointing outside is not found; a symlink
pointing inside renders; a junction on Windows is not found; a FIFO is not opened.

**Done when.** The class ships in 2.0, is exported, listed in fwd.h/jinja2cpp.h (0076), and
docs/security.md (0224) recommends it for tier 1.
