---
status: open
priority: medium
area: style
depends: [0600, 0601]
touches: [.clang-tidy, src/node_arena.h, src/filesystem_handler.cpp, src/recursion_guard.h]
shares: [src/, include/jinja2cpp/]
---
# clang-tidy does not check the memory-safety style the project asks for

**Problem.** The code style is modern memory-safe C++ with no raw pointers unless truly necessary, and
REVIEW.md ranks lifetime defects as the worst class. `.clang-tidy` turns off most of the checks that
encode that style (0054 disabled them for noise at the time): `cppcoreguidelines-owning-memory`,
`pro-bounds-*`, `pro-type-reinterpret-cast`, `pro-type-const-cast`, `pro-type-static-cast-downcast`,
`pro-type-union-access` and `avoid-c-arrays`. So the rule is kept by review alone. Meanwhile 95 of the
378 NOLINTs in `src/`, `include/` and `test/` name checks that are off (pointer arithmetic,
reinterpret_cast, macro-usage, owning-memory, C arrays, hicpp-*): they suppress nothing today, nothing
reports them stale, and no one can tell which still hold.

A run of only those checks on `src/*.cpp` and the headers they reach (master 4a076e9, existing NOLINTs
honoured; list in project files `clang-tidy/2026-11-27-safety-checks-src-4a076e9.txt`) finds 918 hits:

| hits | check | where |
|---:|---|---|
| 306 | pro-bounds-pointer-arithmetic | `node_arena.h` 54, `template_parser.h` 47, `helpers.h` 24, `unicode_names.cpp` 19 |
| 174 | pro-bounds-constant-array-index | tables and lexer |
| 151 | avoid-c-arrays | |
| 109 | owning-memory | `node_arena.h` 88 (the arena, by design); the other 21 in `filesystem_handler.cpp` 11, `render_context.h` 3, `public_host_object.cpp`, `out_stream_pieces.cpp` |
| 80 | pro-bounds-array-to-pointer-decay | `out_stream.cpp` 15, `value_visitors.h` 10 |
| 48 | pro-type-reinterpret-cast | `node_arena.h` 22, `recursion_guard.h` 9 |
| 21 | pro-type-union-access | |
| 17 | pro-type-static-cast-downcast | `filesystem_handler.cpp` 5, `internal_value.cpp` 4 |
| 12 | pro-type-const-cast | `render_context.h` 6 |

The bounds and C-array checks (711) fire on the lexer's and arena's deliberate hot-path idioms; C++17
has no `std::span` to replace them with. The ownership and cast checks (186, 110 of them in the arena
and recursion guard) are the ones that match the style rule and are cheap to keep.

**Proposal.**
1. Enable `owning-memory`, `pro-type-reinterpret-cast`, `pro-type-const-cast`,
   `pro-type-static-cast-downcast` and `pro-type-union-access`. Fix the hits outside the arena (the
   `new std::ifstream` streams with lambda deleters in `filesystem_handler.cpp`, the downcasts) and wrap
   the arena and `recursion_guard.h` in `NOLINTBEGIN(<check>)`/`NOLINTEND` with one reason each.
2. Keep the bounds and C-array checks off; record why in `.clang-tidy`.
3. Delete the NOLINTs that name checks still off, or turn them into the reason comment where they say
   something; add a small script (or the `misc-*` stale-suppression check if clang-tidy gains one) to
   the weekly job that lists NOLINTs naming a disabled check.
4. Then evaluate `bugprone-unchecked-optional-access` (no hits today, already on) and the Clang
   lifetime work (0343 annotations, 0345's 381 lifetimebound suggestions, 0349) as the next step for
   lifetimes, which clang-tidy itself does not check.

**Done when.** The five checks are on and error on new hits, the whole-tree job is green, and no NOLINT
names a check that `.clang-tidy` disables.
