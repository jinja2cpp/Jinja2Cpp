---
status: open
priority: low
area: release
depends: [0076]
touches: [include/jinja2cpp/template.h, include/jinja2cpp/error_info.h, include/jinja2cpp/filesystem_handler.h]
---
# Public headers include `<iostream>` where `<iosfwd>` would do

**Problem.** template.h:20, error_info.h:8 and filesystem_handler.h:10 include `<iostream>`,
which every user translation unit then parses (and which, before C++ library changes,
also put a static `ios_base::Init` object in each of them). The headers only name
`std::basic_ostream`/`std::ostream` in declarations (`Render(std::basic_ostream<CharT>&,
...)`, `operator<<` for `ErrorInfo`); filesystem_handler.h may need `<istream>` for its
stream types. Found by the 0076 plan.

**Proposal.** Switch to `<iosfwd>` (plus `<istream>`/`<ostream>` where a definition needs
it), and include `<iostream>`/`<sstream>` in the sources and tests that relied on the
transitive include. A release note says user code that used `std::cout` through a
Jinja2C++ header must include `<iostream>` itself. Land before the 2.0 RC: removing a
transitive include after the tag breaks source compatibility.

**Done when.** No public header includes `<iostream>`, and the installed-package test
(test/package/main.cpp) still builds.
