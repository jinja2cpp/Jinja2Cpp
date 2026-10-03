---
status: open
priority: low
area: parity
depends: []
touches: [src/python_format.cpp]
---
# `%`-format divergences from Python

**Problem.** While verifying the 0061 split of `Directive` (`src/python_format.cpp`), a
113-template comparison found seven cases where Jinja2C++ and Python Jinja2 differ.
The split did not cause them; master behaves the same:

- `"%d" % 1e30` throws "cannot convert float infinity to integer"; Python prints the
  integer. The `int64_t` range check treats every out-of-range float as infinity.
- A non-ASCII string under `%r`/`%a` differs. Python keeps `'héllo'` for `%r` and escapes
  it to `'h\xe9llo'` for `%a`; the comparison flagged one of the two on the C++ side.
- `"%s"` of a dict prints keys in alphabetical order instead of insertion order.
- `"%(a)*d" % d` and `"%(a)s %s" % d` render; Python raises "not enough arguments"
  because a mapping argument cannot also supply positional ones.
- `"%c" % 55296` (a lone surrogate): Python yields the surrogate code point (bytes
  `ed a0 80` with `surrogatepass`); Jinja2C++ output differs.

**Proposal.** Add each as a parity case under `test/parity/cases/` and fix the ones that
are cheap (the mapping-plus-positional error, the float range message). List any that stay
in `test/parity/divergences/`.

**Done when** each case is in the corpus and either matches Python or is listed as a
deliberate divergence.
