---
status: open
priority: low
area: parity
touches: [src/serialize_filters.cpp#Serialize, src/expression_parser.cpp]
---
# tojson accepts a non-int indent and calls accept a repeated keyword

**Problem.** Found by the 0124 verifier against Python Jinja2 3.1.6; neither is in the
corpus yet.
- `[1]|tojson(indent=1.5)` and `tojson(indent=[1])` render; Python's `json.dumps` raises
  TypeError. Only an int, a str or None is a valid indent.
- `x|tojson(indent=1, indent=2)` and `x|trim(chars='a', chars='b')` render with one of the
  values; Python rejects a repeated keyword argument at parse time (SyntaxError).
`trim(1)` and `undefined|tojson` are already tracked in 0107 and 0052.

**Proposal.** Raise InvalidValueType for an indent that is not int, str or None; reject a
repeated keyword in call/filter argument parsing. Add corpus cases for both.

**Done when.** The four templates above error as in Python, with corpus cases pinning them.
