---
status: in-progress
priority: low
area: parity
depends: [0186]
touches: [test/parity/cases/literals.py, test/parity/cases/string_filters.py, test/parity/cases/methods.py, test/parity/expected/literals.json, test/parity/expected/string_filters.json, test/parity/expected/methods.json]
---
# Corpus cases on code points whose Unicode properties changed after 15.0

**Problem.** Jinja2C++'s generated tables (`src/unicode_printable.h`, `unicode_tables.h`,
`unicode_case.h`, `unicode_names.h`) follow the pinned Python 3.12, which ships Unicode 15.0.
Python 3.13 ships 15.1 and 3.14 ships 16.0. The 0186 oracle runs (`JINJA2CPP_TEST_ORACLE`,
test/parity/README.md "Other oracles") show the Python axis only where the corpus has a case,
and the corpus has none on a code point that 15.1 or 16.0 assigned or changed: under Python
3.13 the measured deltas were stdlib code (`textwrap`, `\N{}` name rules), never table data
(0186 plan, section 10). A table change between Python versions is therefore invisible to the
oracle rows of phase 2.

For example, U+2EBF0 (first ideograph of CJK Extension I, new in 15.1): Python 3.12 gives
`repr` `'\U0002ebf0'`, `isalpha()` False, no name; Python 3.13 gives `'𮯰'`, True and
`CJK UNIFIED IDEOGRAPH-2EBF0`.

**Proposal.** Add a few cases, in the areas that own each behaviour, on code points new or
changed in 15.1 and 16.0: `repr` of a string holding one (printable table), `upper`/`lower`/
`title` on a cased letter added in 16.0 (case tables), a `\N{...}` escape naming one (names
table), and `int()`/`wordcount` style uses of `\d`/`\w` if 16.0 changed a digit or word
property. They match under the pinned 3.12 (no allow-list line), and the 3.13/3.14 oracle rows
then report them as `tracks-bundled`, which is the record that the Python axis is measured.

**Done when.** The cases are in the corpus, the bundled suite matches them, and an oracle run
under Python 3.13 lists them in `report.md`.

**Result.** Thirteen cases: `literals` (`repr` of 15.1/16.0 code points, `\N{}` computed,
stored and any-case names), `methods` (`isnumeric` on U+4E24, whose numeric value 15.1 added;
digit/alpha/upper/identifier classes and `upper`/`casefold`/`swapcase` on Garay, U+019B, U+A7CB,
U+1C89), `string_filters` (`upper`/`lower`/`title`/`capitalize`, `int`/`float` on Garay,
Sunuwar and Myanmar Pao digits, `wordcount`). All match the pinned 3.12 without an allow-list
line. Oracle runs (`JINJA2CPP_TEST_ORACLE_DIR` on a `generate.py --out --any-version`
directory): Python 3.13.18 (Unicode 15.1) reports the five 15.1 cases `tracks-bundled`;
Python 3.14.6 (Unicode 16.0) reports all thirteen `tracks-bundled`, none `tracks-oracle`,
`neither` or `not-run`.
