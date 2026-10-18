---
status: open
priority: high
area: release
depends: [0072, 0073, 0074, 0075, 0043]
touches: [include/, CMakeLists.txt, cmake/public/]
---
# 2.0: inline ABI namespace, header layout, version

**Problem.** Nothing stops 1.x headers from linking against a 2.x library or the reverse;
users include five headers; macros and vendored code leak (`docs/api-2.0.md`, 3.8 and 5.3).

**Proposal.** `namespace jinja2 { inline namespace v2 { ... } }` in every public header;
`jinja2cpp/fwd.h` and `jinja2cpp/jinja2cpp.h`; vendored `polymorphic` into
`jinja2::detail`; `JINJA2CPP_DECLSPEC`; remove `error_handler.h`; `project(VERSION 2.0.0)`,
SOVERSION 2 and the SOVERSION policy of section 5.4 in the README. Touches every header's
namespace lines, so it runs alone after the four API tasks.

**Done when.** `nm` shows `jinja2::v2::` symbols, a test forward-declares through `fwd.h`,
and the library reports 2.0.0 / SOVERSION 2.

**Release gate (0221).** SOVERSION 2 and project version 2.0.0 are already set;
they do not replace the remaining inline namespace/header work. Add a negative
link check showing that a consumer built against 1.x headers cannot silently link
to the 2.x library (the inline namespace's job). SOVERSION tracks the ABI, not the
major (docs/api-2.0.md 5.4), so it guards ABI breaks within 2.x. Exercise both static
and shared artifacts where supported. Publish the agreed source-compatibility and
SOVERSION policy, including Settings layout changes, rather than implying every
future 2.x build is binary-compatible. Settle the API-shaping decisions listed in
0221 before the final namespace/header freeze.
