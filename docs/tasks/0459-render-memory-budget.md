---
status: open
priority: medium
area: robustness
depends: [0262, 0263]
touches: [include/jinja2cpp/template_env.h, src/render_context.h]
shares: [docs/security.md, fuzz/fuzz_common.h]
---
# A memory budget for one render

**Problem.** `Settings::Limits` bounds one operation (`maxSequenceSize`), the output, the steps and
the time, but not what a render keeps. A template that appends a sequence-cap list on every step
keeps `maxRenderSteps * maxSequenceSize` items: under `UntrustedLimits()` (10^7 steps, 2^20
items) that is far past any host's memory, so `UntrustedLimits()` says it "does not bound a
render's memory". Found by 0267: the limits-on fuzz targets treat out-of-memory as a finding, so
`FuzzLimits()` (fuzz/fuzz_common.h) lowers both to 20000 steps and 2^16 items. Measured in a
RelWithDebInfo ASan build: `{% for i in range(100000) %}{% do ns.l.append(range(65536)|list) %}`
peaks at about 1 GB before the step limit stops it.

**Proposal.** A `maxRenderMemory` limit (bytes, 0 none) charged where the engine builds a list,
dict or string a template can keep: the growth operations 0263 checks already call
`CheckSequenceSize` and could charge the same budget with an item-size estimate. Released
values need not be refunded (a cumulative allocation budget, as Jinja2's sandbox has none to
copy). `UntrustedLimits()` then sets one and drops its memory caveat, and `FuzzLimits()` can use
`UntrustedLimits()`' step and sequence values.

**Tests.** Rows in `HugeSequencesFailFast` with a small budget: the append loop above, dict
`update` in a loop, string `~` kept in a namespace; each fails with `ResourceLimitExceeded`.

**Done when.** The rows pass and the untrusted fuzz target runs with `UntrustedLimits()` steps
and sequence cap under the nightly RSS limit.

**Trust model.** When this lands, update its entry under Known gaps in [docs/security.md](../security.md) (0224).
