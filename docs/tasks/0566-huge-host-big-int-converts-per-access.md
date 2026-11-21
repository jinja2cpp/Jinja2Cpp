---
status: open
priority: low
area: robustness
depends: [0543]
touches: [src/value_visitors.h, src/big_int.cpp, include/jinja2cpp/big_int.h]
---
# Stop converting a huge host BigInt on every access

**Problem.** Found by the verifier on the 0543 P1b PR (plan `/mnt/project-files/0543/plan.md`,
section 8 "Host regression", accepted as Q8). A public `jinja2::BigInt` is decimal text, and since
P1b a big integer past `HugeBigIntBits` (14285 bits) keeps binary words, so turning a host
`BigInt` of more than 4300 digits into a template value converts its decimal form to words
(`MakeBigInt` in `src/big_int.cpp`, reached from `value_visitors.h` `MakeBigIntValue(val.ToString())`).
A top-level parameter converts once per render, but an item of a host container converts on
every access: `{% for i in range(20) %}{{ l[0] > 0 }}{% endfor %}` with
`l = [BigInt(2**262143 + 12345)]` (78914 digits) took 118 ms for one access and 2.5 s for 20
(ASan build); before P1b an access copied the text, about 30 µs. Template text chooses the
loop count, so a host that passes huge integers in lists or maps gives templates a CPU
multiplier. P2 does not remove it; neither does P4 (backend words), as the host side stays decimal.

**Proposal.** Pick one, with a recorded decision if it touches the public API:
- a binary form in public `BigInt` (an ABI change: `FromWords`/`Words`, or words kept alongside
  the text), so conversion in both directions is a copy;
- or keep the converted value with the host adapter's item (a per-adapter cache, which needs the
  thread-safety and memory-accounting analysis of plan section 4).

**Done when.** Twenty accesses to a 262144-bit host `BigInt` in a list take about the time of one
conversion or less (bench case or test), and the behaviour of host big integers is unchanged.
