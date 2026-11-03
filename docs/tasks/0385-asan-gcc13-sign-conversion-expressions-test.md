---
status: done
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/753
priority: low
area: build
touches: [test/expressions_test.cpp]
---
# Sanitizer build with GCC 13 fails on a "\x80" literal in expressions_test

**Problem.** The verifier of task 0384 configured an address+undefined sanitizer build with GCC 13
(`-DJINJA2CPP_WITH_SANITIZERS=address+undefined`, Debug and RelWithDebInfo, strict warnings on).
On master c9398f9 that build stops at `test/expressions_test.cpp` around line 756 with
`-Werror=sign-conversion`. The cause is the malformed-UTF-8 block, where the `"\x80"` literals
are converted to an unsigned code-point type. The plain Debug build compiles the same lines
without the error, so the warning appears only when the sanitizer instrumentation changes
inlining. The verifier got past it with `-Wno-error=sign-conversion` and did not trace it
further. It is not known whether CI's sanitizer row hits it, since that row may use another compiler.

**Proposal.** Reproduce with the GCC 13 sanitizer configuration and find the conversion (it may
be inside a helper reached from that block), then make it explicit, for example
`static_cast<unsigned char>`, where the value is converted. Do not relax the warning.

**Done when.** The GCC 13 address+undefined build compiles with strict warnings and no extra
flags, and `jinja2cpp_tests` passes in it.

**Resolution.** The conversion was in the test's `Utf8ToWide` helper, inlined at the `"\x80"`
line: `lead & (0xFF >> (len + 1))` is an `int` that the ternary assigns to `uint32_t`. The masks
are now unsigned (`0xFFU`, `0x3FU`), so the expression is unsigned throughout. Reproduced and
checked with GCC 13.3 `-DJINJA2CPP_WITH_SANITIZERS=address+undefined`, RelWithDebInfo, strict
warnings.
