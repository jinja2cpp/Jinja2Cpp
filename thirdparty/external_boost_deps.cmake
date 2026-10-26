# Boost from the system (external and external-boost modes, docs/tasks/0163). The library
# links no Boost (docs/tasks/0230): only the opt-in Boost adapters use it. Boost.Multiprecision
# (with -DJINJA2CPP_WITH_BIGINT=boost, and for the tests' adapter check) takes Boost::headers
# when a Boost is found, else its standalone pins (thirdparty/CMakeLists.txt); the Boost.JSON
# binding test (boost_json in JINJA2CPP_TEST_BINDINGS) needs the compiled Boost.JSON.
if (NOT JINJA2CPP_BUILD_TESTS AND NOT "${JINJA2CPP_WITH_BIGINT}" STREQUAL "boost")
    return()
endif ()

if (MSVC)
    if (NOT DEFINED Boost_USE_STATIC_LIBS)
        if (THIRDPARTY_RUNTIME_TYPE STREQUAL "/MD" OR THIRDPARTY_RUNTIME_TYPE STREQUAL "/MDd")
            set (Boost_USE_STATIC_LIBS OFF)
            set (Boost_USE_STATIC_RUNTIME OFF)
        else ()
            set (Boost_USE_STATIC_LIBS ON)
            set (Boost_USE_STATIC_RUNTIME ON)
        endif ()
    endif ()
endif ()

if (JINJA2CPP_BUILD_TESTS AND "boost_json" IN_LIST JINJA2CPP_TEST_BINDINGS)
    find_package(Boost 1.75 CONFIG REQUIRED COMPONENTS json)
elseif ("${JINJA2CPP_WITH_BIGINT}" STREQUAL "boost")
    find_package(Boost 1.75 CONFIG REQUIRED)
elseif (JINJA2CPP_VERBOSE)
    find_package(Boost 1.75 CONFIG)
else ()
    find_package(Boost 1.75 CONFIG QUIET)
endif ()
