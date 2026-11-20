---
status: in-progress
priority: medium
area: robustness
depends: [0249]
touches: [include/jinja2cpp/user_callable.h]
---
# A user callable's narrow integer parameter silently truncates its argument

**Problem.** Found by 0249's boundary-cast audit. `ArgPromoter<T>` for a fundamental `T`
(include/jinja2cpp/user_callable.h) converts the template's `int` (int64) or `float` to the parameter type the host
callable declares with a plain `static_cast<U>`. A value outside the parameter's range wraps or truncates and the
callable runs with a different number, with no error. With `MakeCallable` over these lambdas:

| Callable | Template | Callable receives |
|---|---|---|
| `[](int x)` | `{{ f(2**40 + 5) }}` | `5` |
| `[](unsigned x)` | `{{ u(-1) }}` | `4294967295` |
| `[](char x)` | `{{ c(300) }}` | `44` |
| `[](int x)` | `{{ f(3.9) }}` | `3` |

Jinja2 passes the exact Python `int` (or `float`) on, so the host sees the template's value or fails itself. Only
an argument beyond int64 (`2**70`) is rejected today, by the big-int cap. `-fsanitize=implicit-conversion` cannot
see this: the cast is explicit.

**Proposal.** For integral `U` narrower than or differently signed from the stored `int64_t`, check the range (as
`detail::ToStoredInt` does in the other direction) and fail the call with a TypeError-like error naming the
parameter when it does not fit. Decide whether a `float` argument to an integral parameter is an error (Python
would not convert) or keeps C++'s truncation; the second keeps today's tests, the first matches the oracle more
closely. The conversion runs per argument on the callable path, so keep it a compile-time branch with no cost for
`int64_t`, `double` and `bool` parameters.

**Done when.** The four rows above report an error (or the documented float rule), in-range values of every
integer parameter type still pass, there are rows in test/user_callable_test.cpp for both character types, and
`bench/count.py` shows no change on `callable`-heavy workloads.

**Decision (Ruslan, 2026-11-20).** An integer argument outside the range of an integer parameter other than
`bool` (narrower, or unsigned for a negative value) fails the call with `ErrorCode::IntegerOutOfRange`, as Python's
OverflowError for a C integer. A float argument to an integer parameter keeps C++'s truncation toward zero (Python
never converts here, the host type is a C++ choice, so this breaks no host code); only a NaN, an infinity or a float
whose truncation is outside the range fails, which was undefined behaviour before. The rejected alternative was a
TypeError for every float, as CPython's C `int` parameters raise.

**Implementation.** `detail::NeedsRangeCheck<T, U>` / `ArgFits<U>` in user_callable.h pick the check at compile
time: nothing changes for `int64_t`, `double` and `bool` parameters given an int. The typed path checks in
`TypedParamUnwrapper`, where the descriptor's name is known ("callable argument 'x' = 1099511627781 is out of range
for its 32-bit signed integer parameter"); the joint visit of a generic callable checks in `ArgPromoter` without a
name. The message is formatted out of line by the exported `detail::ThrowArgumentOutOfRange` (src/value.cpp, an
additive ABI change), which throws the engine's `IntegerOutOfRangeError`.
