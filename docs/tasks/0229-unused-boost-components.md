---
status: open
priority: low
area: build
touches: [thirdparty/CMakeLists.txt, thirdparty/thirdparty-internal.cmake, thirdparty/thirdparty-conan-build.cmake, thirdparty/external_boost_deps.cmake, CMakeLists.txt#JINJA2CPP_INSTALL, src/internal_value.h, src/template_parser.cpp, src/statements.cpp, src/string_converter_filter.cpp, src/expression_evaluator.cpp, sbom/components.json]
---
# Drop the Boost libraries and includes nothing uses

**Problem.** The 0187 audit (docs/dependencies.md) found Boost pieces that are requested
but unused. `lexical_cast`, `numeric_conversion`, `variant` and `optional` are in
`BOOST_INCLUDE_LIBRARIES` (thirdparty-internal.cmake), `JINJA2CPP_PRIVATE_LIBS`
(thirdparty/CMakeLists.txt), the installed config's default `JINJA2CPP_BOOST_COMPONENTS`
(CMakeLists.txt) and the external/conan `find_package(Boost ...)` lists, though no source
includes them since 0071; `-Wno-error` is still set on `boost_variant`. Five includes are
dead: `boost/unordered_map.hpp` (internal_value.h), `boost/cast.hpp`
(template_parser.cpp), `boost/core/null_deleter.hpp` (statements.cpp),
`algorithm/string/trim_all.hpp` (string_converter_filter.cpp) and
`algorithm/string/join.hpp` (expression_evaluator.cpp). External-mode users must have
those components installed for nothing.

**Proposal.** Remove them from every list and drop the includes; remove the matching
`boost-*` redundant entries from sbom/components.json and regenerate. Check each deps
mode (internal, external with apt Boost, conan-build) still configures, since a removed
component may have been the one pulling a transitive library that is used (for example
`container` for `small_vector`); name such libraries explicitly instead.

**Done when.** No Boost component is requested that the code does not include, all CI
deps modes are green, and `scripts/sbom.py --check` passes with the entries removed.
