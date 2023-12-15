message(STATUS "'conan-build' dependencies mode selected for Jinja2Cpp. All dependencies are taken as a conan packages")

find_package(expected-lite REQUIRED)
find_package(variant-lite REQUIRED)
find_package(optional-lite REQUIRED)
find_package(string-view-lite REQUIRED)

find_package(fmt REQUIRED)

set(_test_dependencies)
if (JINJA2CPP_BUILD_TESTS)
    find_package(nlohmann_json REQUIRED)
    set(_test_dependencies "nlohmann_json::nlohmann_json")
endif()

set(_bindings_json)
set(_bindings_find_package_boost)
if ("${JINJA2CPP_WITH_JSON_BINDINGS}" STREQUAL "boost")
    set(_bindings_find_package_boost "json")
    set(_bindings_json "Boost::json")
endif()

find_package(Boost COMPONENTS algorithm filesystem numeric_conversion ${_bindings_boost} optional variant regex REQUIRED)

if("${JINJA2CPP_WITH_JSON_BINDINGS}" STREQUAL "rapid")
    find_package(RapidJSON REQUIRED)
    set(_bindings_json "rapidjson")
endif()

set(JINJA2_PRIVATE_LIBS_INT Boost::headers Boost::filesystem Boost::numeric_conversion)
set(JINJA2_PUBLIC_LIBS_INT
    ${_bindings_json}
    Boost::regex
    fmt::fmt
    ${_test_dependencies}
    nonstd::expected-lite
    nonstd::optional-lite
    nonstd::string-view-lite
    nonstd::variant-lite
)

