---
status: done
priority: medium
area: build
depends: [0086]
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/373
touches: [CMakeLists.txt, thirdparty/, conanfile.py, cmake/, README.md, .github/workflows/*.yml]
---
# Boost.Regex and `JINJA2CPP_USE_REGEX` are dead weight after 0086

**Problem.** 0086 replaced the statement keyword regex in `src/template_parser.h` with a
table lookup; it was the only user of `JINJA2CPP_USE_REGEX_BOOST`. The build still offers
the `JINJA2CPP_USE_REGEX` option (`CMakeLists.txt`, "boost works faster"), defines
`JINJA2CPP_USE_REGEX_BOOST`, and finds, links and installs `Boost::regex` in every
dependency mode (`thirdparty/external_boost_deps.cmake`, `thirdparty-internal.cmake`,
`thirdparty-conan-build.cmake`, `thirdparty/CMakeLists.txt`). Users of external or Conan
Boost therefore still need a compiled Boost library for nothing. The only regex left in
`src/` is a `std::regex` in `string_converter_filter.cpp` (urlize's IPv6 check).

**Proposal.** Deprecate `JINJA2CPP_USE_REGEX` (warn when set, ignore the value), drop the
define and `Boost::regex` from every dependency mode, the install/export config and the
Conan recipe. Check that nothing in `include/` or the exported targets still names it.

**Done when.** All CI configurations (internal, external, Conan, shared) build without
Boost.Regex, and `grep -rn regex thirdparty CMakeLists.txt` finds only the deprecation.

**Next.** Whether urlize's `std::regex` is worth a hand-written check too (it runs only
for bracketed hosts).
