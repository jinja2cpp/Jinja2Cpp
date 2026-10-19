---
status: open
priority: medium
area: release
depends: [0076]
touches: [src/robin_hood.h, src/lexertk.h, .clang-format-ignore, sbom/components.json]
---
# Vendored robin_hood and lexertk still live in global namespaces

**Problem.** 0076 moved the vendored value_types into `jinja2::detail::xyz`, but the two
internal vendored headers keep their upstream namespaces: `robin_hood` (src/robin_hood.h:84,
about 546 symbols in the library) and `lexertk` (src/lexertk.h:63). Since 0076 the shared
library hides them (`-fvisibility=hidden`), but the static library still carries them as
global symbols: a consumer that links its own robin_hood or lexertk of another version next
to `libjinja2cpp.a` gets an ODR violation the linker does not report, and the 0076 nm test
has to allow-list them.

**Proposal.** Wrap both in `jinja2::detail` (inside the `v2` inline namespace) with a
documented local change, as 0076 did for value_types, rename their include guards and
macros (`ROBIN_HOOD_*`) with a `JINJA2CPP_` prefix, and drop them from the nm test's
allow-list. If 0231 replaces robin_hood first, only lexertk and the successor remain.

**Done when.** `nm -C libjinja2cpp.a` shows no top-level `robin_hood::` or `lexertk::`
symbols and the abi_namespace test checks them without exceptions.
