---
status: done
priority: medium
area: perf
depends: [0131]
touches: [src/template_env.cpp, include/jinja2cpp/template_env.h, src/load_settings.h]
---
# Load compares the whole Settings on every template

**Problem.** `TemplateEnvImpl::GetLoadSettings` (template_env.cpp) compares the
environment's `Settings` with the cached `LoadSettings` field by field, under a mutex,
on every Load, because `TemplateEnv::GetSettings()` returns a mutable reference and a
change made through it cannot be seen any other way. Measured on master 3cc8e7d plus
0117 P2 (callgrind, inclusive share of one Load): plain_text 13.3%, substitute 5.4%,
for_range 2.9%, filters 0.6%. Split out of the 0118 P5 plan (perf track review,
2026-10-08), where it was item D.

**Options.**
1. A generation counter bumped by `SetSettings` (and any other mutator), with the mutable
   `GetSettings()` deprecated for 2.0 the way 0070-0075 deprecated old names. The Load
   check becomes one integer compare. Public API: Ruslan decides (card in the wave 2
   thread, 2026-10-08).
2. No API change: a cheaper compare (scalars first, the short delimiter strings inline),
   about half the saving. 0118 P5 keeps this as its fallback if plain_text misses its
   gate.

**Progress.**
- Option 2 (strings compared in place after the flags and numbers): this PR. Load against
  3cc8e7d: plain_text -5.75%, substitute -2.36%, for_range -1.25%, inheritance -0.71%,
  other cases -0.0..-0.6%; Render unchanged. Option 1 waits on Ruslan, and the perf track
  asks for it only if plain_text is still above its 0892811 count (4,121) after 0118 P5.
- Option 2 merged in #435 (6039634). The perf track no longer needs option 1: 0118 P5b-2
  (#437) brought Load/plain_text to 4,077, under its 0892811 count. Option 1 (the
  generation counter and the 2.0 deprecation) stays an API choice for Ruslan, and gets its
  own task if he takes it.
