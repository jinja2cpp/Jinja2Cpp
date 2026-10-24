---
status: open
priority: low
area: api
depends: [0262]
touches: []
---
# Untrusted mode: additive follow-ups after the freeze

Items from the untrusted-input review that need no ABI change at 2.0, kept together until each
gets a design:
- **Cancellation hook.** `TemplateEnv::SetInterruptHandler` or a size-prefixed `RenderOptions`
  overload (0252) with a deadline and a `const std::atomic<bool>*`; a cancelled render reports a
  new appended `ErrorCode` (Ruslan 2026-10-18, S5).
- **Stack bounds for fibers and coroutines.** `SetThreadStackBounds(lo, hi)`: the stack check
  reads the OS thread stack once per thread (src/recursion_guard.cpp), so renders on custom stacks
  get spurious errors or no check.
- **Error redaction.** An `ErrorInfo::ToString` mode that omits host exception text and source
  quotes, for hosts that show errors to template authors.
- **One UTF-8 decoder.** The strict `DecodeUtf8` and the lenient `SplitCodePoints` (about 32 call
  sites) differ on invalid input; `|upper` turns byte 0xFF into "Ÿ". Pick one policy (U+FFFD,
  as Python's decode with `errors='replace'`).
- **`maxIntegerDigits`** if the 0178 big-integer backend lands; it then has to go into
  `Settings::Limits` before the freeze instead (CPython caps int-to-str at 4300 digits).

**Trust model.** When this lands, update its entry under Known gaps in [docs/security.md](../security.md) (0224).
