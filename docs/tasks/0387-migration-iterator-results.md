---
status: open
priority: low
area: release
depends: [0077, 0383]
touches: [MIGRATION.md]
---
# MIGRATION.md: lazy filter results are no longer sequences

**Problem.** 0383 (Ruslan's choice (a), 2026-11-03) made 2.0 refuse what 1.x answered: `|last`,
`|length`, `|count`, `|random` and slicing of a `map`, `select`, `reject`, `selectattr`,
`rejectattr`, `batch`, `slice`, `unique` or `items` result now raise Python's TypeError
("'generator' object is not reversible", "object of type 'generator' has no len()",
"'generator' object is not subscriptable"), as does `|last` or a slice of `reverse`'s result
("'list_reverseiterator' object is not reversible"). An index (`(x|select)[0]`) is undefined
and `is sequence` is False. A template that worked with 1.x can fail with 2.0. MIGRATION.md has
no section for template behaviour changes yet, and 0077 owns that file.

**Proposal.** Add a "Template behaviour" section to MIGRATION.md (with 0077's notes or before
them) with this change and its fix: insert `|list` (`x|select|list|last`,
`(x|map('string')|list)[0:2]`), which works in Python Jinja2 and every Jinja2C++ version.

**Done when** MIGRATION.md lists the change with the `|list` fix.
