---
status: open
priority: low
area: robustness
depends: [0566]
touches: [include/jinja2cpp/big_int.h, src/big_int.cpp, src/value_visitors.h]
---
# A words form in the public BigInt

**Problem.** Task 0566 (option A of the architect's plan) keeps a render's last four huge host
BigInts converted, so reading the same item of a host container again shares one object. That
bounds the CPU multiplier a template gets from a loop, but it leaves three costs, all from the
public `jinja2::BigInt` being one `std::string` of decimal text:

- every render converts a huge host BigInt from decimal to binary words once (118 ms for 262144
  bits in an ASan build), and every result returned to the host converts words back to decimal;
- more than four distinct huge host integers read in turn by one render replace each other and
  convert again;
- the Python module's boundary pays the conversion both ways for every huge integer.

**Proposal.** Pick one, with a recorded decision, as it changes the ABI (`sizeof(BigInt)` is
asserted equal to `sizeof(std::string)`): `BigInt::FromWords`/`Words` with the words kept next
to the text, or instead of it for a huge integer. Conversion in both directions is then a copy.

**Done when.** A render that reads 100 distinct huge host integers converts none from decimal
text, and the Python module returns a huge result without a decimal round trip.
