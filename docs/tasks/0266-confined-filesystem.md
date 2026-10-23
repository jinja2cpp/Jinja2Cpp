---
status: done
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/615
priority: medium
area: api
touches: [include/jinja2cpp/filesystem_handler.h, include/jinja2cpp/fwd.h, src/filesystem_handler.cpp, src/confined_filesystem.cpp, src/template_path.h, CMakeLists.txt, test/filesystem_handler_test.cpp]
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

**Done (this PR).** `ConfinedFileSystem` (src/confined_filesystem.cpp, no Boost), declared in
fwd.h; `jinja2cpp.h` already includes filesystem_handler.h. The name rules moved to
`src/template_path.h` (`SplitTemplateName`), shared with `RealFileSystem`, whose behaviour is
unchanged.
- POSIX: a walk from a descriptor of the root, one `openat(O_NOFOLLOW)` per piece. A link met on
  the way is read with `readlinkat` and its target walked the same way, so a link that stays inside
  the root renders; an absolute or empty target, a `..` climbing above the root or more than 40
  expansions is not found. The last piece must be a regular file: on Linux it is pinned with `O_PATH`
  (which opens nothing), checked, and reopened for reading through `/proc/self/fd`, so a device
  or FIFO is never opened even if swapped in (Codex P2); elsewhere, or without `/proc`, `fstatat`
  before the open and an identity check (`st_dev`/`st_ino`) of the opened descriptor after it, so a
  swap in between can open a special file but never read it. The
  stream reads from that descriptor (own `streambuf`; the wide one decodes UTF-8 chunk by chunk, so
  `maxTemplateSize` still stops the read).
- Windows (compiled in CI only): the file is opened normally and `GetFinalPathNameByHandleW` of the
  opened handle must lie under the root's final path, compared exactly (normalized final paths spell each
  component as stored, so a case-sensitive sibling `root` of `Root` does not match, Codex P1); final
  paths are long names, so short names need nothing extra, and junctions or links leading outside
  fail the check. A non-disk handle or a directory is not found. `OpenStream`/`OpenWStream` read
  `\r\n` as `\n` like RealFileSystem's text-mode streams; `OpenByteStream` reads raw bytes.
- Tests: `ConfinedFsTest` rows in test/filesystem_handler_test.cpp (POSIX), the link-swapping stress
  row included. No junction test on Windows.
- Left: docs/security.md does not exist yet; 0224 must recommend this class for tier 1 when it writes
  that page. Modification time is `st_mtime` (one-second resolution), as RealFileSystem's.
