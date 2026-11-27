---
status: in-progress
priority: low
area: parity
depends: []
touches: [src/value_methods.cpp#FormatValue, src/value_methods.cpp#PadFormatted, src/internal_value.cpp#SubscriptCallable]
shares: [test/parity/cases/filters.py, test/parity/divergences/filters.txt]
---
# Integer format specs: `#` with zero padding, grouped zero padding, `,` with a binary base

**Problem.** Found by the 0571/0577 verifier (batch 13 A); all identical on master 1b69673, Jinja2 3.1.6:
- `'{:#010x}'.format(255)` renders `0000000xff`; Python `0x000000ff`. `'{:=#20x}'` pads before `0x`; Python
  `0x                ff`. The prefix belongs to the sign part, before `=` or `0` padding.
- A zero-padded width with grouping does not group the padding: `'{:012_x}'.format(4294967295)` renders
  `000ffff_ffff` (Python `00_ffff_ffff`), `'{:012,}'.format(1234567)` should be `0,001,234,567`.
- `'{:,x}'`, `'{:,b}'` and `'{:,o}'` are accepted; Python raises "Cannot specify ',' with 'x'.".
- `loop.cycle.__name__` and other methods bound through a receiver (`IBoundReceiver`) are undefined; Python gives
  `cycle`. 0571 named only the built-in methods of str, list, dict and tuple.

**Proposal.** Move the `0x`/`0o`/`0b` prefix out of the digits into the sign string `PadFormatted` places, give `0`
padding with grouping Python's grouped fill (CPython's `_PyUnicode_InsertThousandsGrouping` with a minimum width),
reject `,` with b/o/x/X, and answer `__name__`/`__qualname__` for receiver-bound methods from `GetBoundMethod`.

**Done when** each form above matches Python in narrow and wide runs, with corpus cases under `filters.format_*`.

**Resolution (batch 14 D).** The prefix goes with the sign (`Formatted::prefix`); grouped zero padding follows CPython's
`_PyUnicode_InsertThousandsGrouping` with a minimum width (`GroupDigits`), for floats' integer part too; `,` and `_`
are checked against the type as CPython's parser does (`CheckGrouping`, also for strings: "Cannot specify ',' with
's'."). Found on the way and fixed: a `0` before the width fills with zeros under an explicit align and for strings
(`'{:<010}'`, `'{:010}'.format('ab')`), a bool formats as its int under any non-empty spec (`'{:>5}'.format(true)`),
and string specs raise Python's messages. `__name__`/`__qualname__` of receiver-bound methods (`loop.cycle`,
`loop.changed`, `cycler.next`) come from the receiver's class. Corpus: `filters.format_spec_*`.
