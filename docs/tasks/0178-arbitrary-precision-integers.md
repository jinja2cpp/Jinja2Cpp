---
status: done
pr: [https://github.com/jinja2cpp/Jinja2Cpp/pull/592, https://github.com/jinja2cpp/Jinja2Cpp/pull/613, https://github.com/jinja2cpp/Jinja2Cpp/pull/620, https://github.com/jinja2cpp/Jinja2Cpp/pull/630]
priority: medium
area: parity
depends: [0015]
touches: [src/internal_value.h, src/value_visitors.h#BinaryMathOperation, src/filters.cpp, src/python_format.cpp, src/serialize_filters.cpp, include/jinja2cpp/value.h]
shares: [src/internal_value.cpp, thirdparty/thirdparty-internal.cmake]
---
# Integers of arbitrary size, as Python's `int`

**Problem.** Integers are int64; 0015 made overflow raise and a literal beyond int64 a float,
recorded as deliberate divergences. Python Jinja2 computes with unbounded `int`, so these
differ: `literals.int_big`, `operators.int_overflow_mul`, `operators.int_big_pow`,
`operators.int_min_floordiv_overflow`, `filters.int_out_of_range`,
`filters.round_int_overflow`, and the unsigned values above `INT64_MAX` of 0172.

**Proposal** (Ruslan asked 2026-10-12, 0048 thread). Use Boost.Multiprecision's `cpp_int`
(header-only, in the Boost tree we already fetch; add `multiprecision` to
`BOOST_INCLUDE_LIBRARIES`) as CPython does with small ints: int64 stays the fast
alternative of `InternalValue` (32 B, 0140), and an int64 overflow, which 0015 already
detects, promotes the result to a boxed big integer instead of raising; a big result that
fits int64 drops back. Templates that never overflow pay nothing.

**Needs an architect plan first** (value model and public API), covering: the
`InternalValue` alternative and its box; every numeric path (arithmetic, `//`/`%` floor
rules, `**`, comparisons and sort, hashing for dict keys and `unique`, int/float mixing
and its OverflowError, `str`/repr, `int`/`float`/`round`/`abs`/`sum` filters, `%d`/`%x`
formatting, `tojson`, `range`, subscripts and slices with big bounds); how `Value` shows
a big integer to users (a new alternative, a string, or an error on conversion; ties to
0072/0172); compile-time cost; and a perf gate check (no change for int64 templates).

**Done when** (revised 2026-10-18, plan Q5). The six cases match in builds with a backend; in the
default build `literals.int_big` and `filters.int_out_of_range` match and the other four are
`@nobigint` divergences (0273); `docs/parity.md` calls integers a build option. 0269-0273 done.

**Decided by Ruslan 2026-10-18 (0221 thread), replacing the single-library proposal above.**
Big integers go behind a backend interface, so the core stays Boost-free (0230):
- Default build: no big-integer backend; int64 behaviour of 0015 (overflow raises).
- `-DJINJA2CPP_BIGINT=boost` (name to be settled by the plan): an optional compile-time
  backend over Boost.Multiprecision `cpp_int`, which also serves as the sample
  implementation.
- Users can supply their own backend.
The architect plan must decide how a user backend is supplied (a compile-time policy, or
an interface registered at run time, whose layout then becomes part of the ABI policy of
docs/api-2.0.md 5.4), how `Value` exposes a big integer when no backend is configured, and
keep the perf gate (no change for int64 templates). 0269 is a G1 condition; 0270-0273 are in 2.0
by D2 and only add to the ABI (docs/release-2.0.md D2/D3).

**Plan approved by Ruslan 2026-10-18 14:31** (every question as recommended):
[docs/design/0178-big-integer-plan.md](../design/0178-big-integer-plan.md). 0178 is now the umbrella:
- A runtime, versioned `BigIntBackend` function table set per `TemplateEnv`, called only after an
  int64 overflow; `-DJINJA2CPP_WITH_BIGINT=none|boost` picks only the built-in default. One layout
  for every build; the Boost adapter is a public header-only file and the sample backend.
- `jinja2::BigInt` (canonical decimal text) is a new `Value` alternative, appended last. Without a
  backend, big values print, compare, serialise and convert exactly; integer arithmetic raises.
  `Value(ULLONG_MAX)` stores a BigInt instead of 0172's throw.
- Limits: 2^20-bit results (`ResourceLimitExceeded`, per 0250) and CPython's 4300-digit limit.
- Phases: [0269](0269-public-big-int-value.md) (blocks G1), [0270](0270-big-int-core-text-form.md),
  [0271](0271-big-int-backend-interface.md), [0272](0272-big-int-boost-backend.md),
  [0273](0273-build-conditional-parity-divergences.md).

**Closed (2026-10-24).** The revised done-when holds: phases 0269-0273 are done (plan #592; #613, #620, #630), the default build keeps the four arithmetic cases as `@nobigint` lines and `docs/parity.md` calls integers a build option.
