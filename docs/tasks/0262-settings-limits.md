---
status: done
priority: high
area: api
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/601
depends: [0250]
touches: [include/jinja2cpp/template_env.h, src/template_env.cpp, src/recursion_guard.h, src/recursion_guard.cpp, src/renderer.h, src/global_functions.cpp, src/out_stream.cpp, src/template_impl.h, test/limits_test.cpp]
shares: [src/internal_value.h, src/internal_value.cpp]
---
# `Settings::Limits`: one place for every limit a template can hit

**Release impact (0221).** Freeze-critical: `Settings` is allocated by the user, so any field
added after 2.0.0 costs a SOVERSION bump. Lands in the API wave next to 0250 P1.

**Problem.** A host rendering templates written by its users cannot bound a render. The only
limits are compile-time constants (0098) and a fixed sequence cap far above what such a host
wants (0097). There is no step budget, deadline, output bound, template-size bound or bound on
how many templates one render may load.

**Decided (Ruslan 2026-10-18, untrusted-input review S1, S2, S5, S6, S7).**
- One nested struct, fixed and enforced at 2.0 (S1 a), no reserved slots (S2 a):
  ```cpp
  struct Settings {
      struct Limits {                      // 0 = no limit beyond the built-in caps
          std::uint64_t maxRange = 0;                   // range() length
          std::uint64_t maxSequenceSize = 0;            // items/chars one operation builds; only lowers the 2^26 cap
          std::uint64_t maxOutputSize = 0;              // characters one Render writes
          std::uint64_t maxTemplateSize = 0;            // source characters at Load, loader-sourced included
          std::uint32_t maxTemplateLoads = 0;           // templates one Render may load (include/import/extends)
          std::uint64_t maxRenderSteps = 0;             // statement bodies + calls per Render (approximate unit)
          std::chrono::milliseconds maxRenderTime{ 0 }; // polled every ~1024 steps
          std::uint32_t maxRenderDepth = 256, maxExpressionDepth = 64,
                        maxExpressionOperators = 400, maxBlockNesting = 128; // 0098, defaults as today
      } limits;
  };
  ```
  Every field must be enforced in 2.0: no inert fields.
- Every limit fails the render with `ErrorCode::ResourceLimitExceeded` (0250), `extraParams[0]`
  the message and `extraParams[1]` the field name (`"maxRenderTime"`); depth limits keep
  `RecursionLimitExceeded` (S5 a). A host-initiated cancellation (the later hook in 0268) gets an
  appended code of its own (Ruslan has nothing against a new code).
- Presets, both header-inline (values are behaviour, not ABI) (S6: both):
  `Settings::Limits UntrustedLimits()` (limits only) and `Settings UntrustedSettings()` (the
  limits plus `autoescape = true`). Values anchored on Python's `SandboxedEnvironment` (S7 a):
  `maxRange` 100000, `maxSequenceSize` 2^20, `maxOutputSize` 16 MiB, `maxRenderSteps` 10^7, no
  time limit; tune so the chat-template corpus renders under them.

**Design notes (untrusted-input review §4).**
- At Render start one thread-local pointer to the template's `Limits` and one countdown, restored
  by RAII so a render started from a host callable keeps its own limits.
- Steps: `if (--left == 0) Slow();` beside the existing `CheckStack()` in `ComposedRenderer::Render`
  (src/renderer.h), in `RenderDepthGuard` and in `UserCallableBody`. Off means `left = UINT64_MAX`.
  `Slow()` refills and polls the clock. Fallback if the bench gate fails: tick per loop iteration
  and per call only.
- `CheckSequenceSize` reads `min(maxSequenceSize, 2^26)`; `maxRange` in `CallRange`;
  `maxOutputSize` at the output stream flush; template size and loads on the Load path.
- `Settings::operator==` compares `limits`.
- 0097 keeps the per-operation checks (0263) and fuzzing; 0098's API part is this task.

**Tests.** New `test/limits_test.cpp` table {template, field = value, code, field name}:
`range(100001)|list` with `maxRange` 100000 fails, `range(100000)|list|length` renders;
`'x'*2000` with `maxSequenceSize` 1000; a 2000-iteration loop with `maxOutputSize` 1000; a 2 KB
source with `maxTemplateSize` 1000 (Load error); three includes with `maxTemplateLoads` 2; a
doubling macro `f(40)` with `maxRenderSteps` 10^5; `range(10**9)` loop with `maxRenderTime` 200 ms
fails within 2x in Release; a host callable sleeping past the deadline fails only after it
returns. Every row renders unchanged with default `Limits`. Existing `RecursionLimitsTest` rows
plus `maxRenderDepth = 10`.

**Perf gate.** `bench/count.py --cache-sim`: Render Ir within ±0.1% with limits off,
`Render/plain_text` fixed cost recorded.

**Done when.** The struct and presets ship in 2.0, every field is enforced and tested, and
docs/api-2.0.md and docs/security.md (0224) describe them.

**Done in #601.** All eleven fields, both presets and error code 18 as specified. Measured with limits
off, in instructions: Render from −0.02% to +0.5% (layout), with Render/plain_text +8 as its fixed cost;
Load/plain_text about +47 (the limits compared word by word, the size check, the parse-limit scope).
Leftovers are in 0282; memory bounds stay in 0263.
