---
status: open
priority: high
area: docs
depends: [0240]
touches: ["site:index.md", "site:docs/getting_started.md", "site:docs/j2_compatibility.md", "site:docs/usage/"]
---
# Website: 2.0 content refresh and compiled snippets

Paths prefixed `site:` are in jinja2cpp/jinja2cpp.github.io.

**Problem.** Beyond the build guide (0077) and the example inventory (0227), the 0221
scout (2026-10-18) found, on the site's master dccc3f0:
- `index.md`: "C++14/17/20", links to jinja.pocoo.org 2.10 docs, a dead Bintray Conan
  button (Bintray closed in 2021), `jinja2cpp/1.2.1`, and `value_ptr.hpp`, gone from master.
- `getting_started.md`: `#include <jinja2/template.h>` (should be `jinja2cpp/`), a link to
  `examples-build/conan/1.1.0`; its sample ignores the `[[nodiscard]]` `Load` result and
  fails under `-Werror` against master.
- `j2_compatibility.md` says i18n, break/continue, autoescape, slicing and several filters
  are unsupported; `docs/parity.md` says they are done.
- `usage/reflection.md`: `obj.name` on a struct without `name`, and
  `std::string result = tpl.RenderAsString(params);`, which does not compile
  (`Result<std::string>`).
- `error_reporting`, `include_import`, `main_statements` are TODO stubs.

**Proposal.** Every C++ snippet on the 2.x pages comes from a file that the site CI (or the
library's `examples/`) compiles and runs against the tag. The compatibility page renders or
links `docs/parity.md` instead of a hand-kept table. Fill the stubs, drop dead links.
Build guide and migration text stay with 0077, external example repositories with 0227.

**Done when.** Every 2.x snippet compiles in CI against the RC, no page contradicts
`docs/parity.md`, and no TODO stub or dead link is left in the 2.x tree.
