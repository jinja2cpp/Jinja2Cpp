message(STATUS "'conan-build' dependencies mode selected for Jinja2Cpp. All dependencies are taken as a conan packages")

find_package(expected-lite REQUIRED)

find_package(fmt REQUIRED)

if (JINJA2CPP_BUILD_TESTS)
    find_package(nlohmann_json REQUIRED)
endif()

set(_bindings_json)
set(_bindings_find_package_boost)
if ("${JINJA2CPP_WITH_JSON_BINDINGS}" STREQUAL "boost")
    set(_bindings_find_package_boost "json")
    set(_bindings_json "Boost::json")
endif()

find_package(Boost COMPONENTS algorithm filesystem numeric_conversion ${_bindings_find_package_boost} optional variant REQUIRED)

if("${JINJA2CPP_WITH_JSON_BINDINGS}" STREQUAL "rapid")
    find_package(RapidJSON REQUIRED)
    # header-only: keep it out of the exported link interface
    set(_bindings_json $<BUILD_INTERFACE:rapidjson>)
endif()

# numeric_conversion is header-only; Conan's Boost exposes it through Boost::headers.
# Only expected-lite is public: no public header includes fmt or the JSON libraries, and
# the tests link nlohmann_json themselves.
set(JINJA2_PRIVATE_LIBS_INT Boost::headers Boost::filesystem fmt::fmt ${_bindings_json})
set(JINJA2_PUBLIC_LIBS_INT nonstd::expected-lite)

