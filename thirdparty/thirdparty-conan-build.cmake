message(STATUS "'conan-build' dependencies mode selected for Jinja2Cpp. All dependencies are taken as a conan packages")

find_package(expected-lite REQUIRED)

find_package(fmt REQUIRED)

# The JSON libraries are for the tests only: the library reads {% meta %} itself (0230 D6)
if (JINJA2CPP_BUILD_TESTS)
    find_package(nlohmann_json REQUIRED)
endif ()

# The library links no Boost (docs/tasks/0230). conanfile.txt asks for a header-only Boost:
# Boost::headers carries Multiprecision for -DJINJA2CPP_WITH_BIGINT=boost and the tests'
# adapter check. Only the Boost.JSON adapter test needs a compiled library.
if (JINJA2CPP_BUILD_TESTS OR "${JINJA2CPP_WITH_BIGINT}" STREQUAL "boost")
    find_package(Boost REQUIRED)
    if (JINJA2CPP_BUILD_TESTS AND "${JINJA2CPP_WITH_JSON_BINDINGS}" STREQUAL "boost" AND NOT TARGET Boost::json)
        message(FATAL_ERROR "The Boost.JSON adapter test (JINJA2CPP_WITH_JSON_BINDINGS=boost) needs a compiled "
            "Boost.JSON, and conanfile.txt asks for a header-only Boost: install with "
            "`-o boost/*:header_only=False`, or configure with -DJINJA2CPP_WITH_JSON_BINDINGS=nlohmann")
    endif ()
endif ()

# Only expected-lite is public: no public header includes fmt, and the tests link their
# JSON library themselves.
set(JINJA2_PRIVATE_LIBS_INT fmt::fmt)
set(JINJA2_PUBLIC_LIBS_INT nonstd::expected-lite)
