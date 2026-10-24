# Boost from the system (external and external-boost modes, docs/tasks/0163). A Boost built
# with CMake (1.82+) defines a target per header-only library; one built with b2 only offers
# Boost::headers for them, so ask for the compiled libraries first and fall back to it.
if (JINJA2CPP_VERBOSE)
    set (FIND_BOOST_PACKAGE_QUIET)
else ()
    set (FIND_BOOST_PACKAGE_QUIET QUIET)
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

set(_jinja2cpp_boost_compiled)
if ("${JINJA2CPP_WITH_JSON_BINDINGS}" STREQUAL "boost")
    list(APPEND _jinja2cpp_boost_compiled json)
endif ()
set(_jinja2cpp_boost_header_only algorithm any)

find_package(Boost 1.75 CONFIG ${FIND_BOOST_PACKAGE_QUIET}
    COMPONENTS ${_jinja2cpp_boost_compiled} ${_jinja2cpp_boost_header_only})
# A b2-built config may report the header-only components found without defining targets
set(_jinja2cpp_boost_per_library ${Boost_FOUND})
foreach (_lib IN LISTS _jinja2cpp_boost_header_only)
    if (NOT TARGET Boost::${_lib})
        set(_jinja2cpp_boost_per_library FALSE)
    endif ()
endforeach ()
if (_jinja2cpp_boost_per_library)
    set(JINJA2CPP_BOOST_COMPONENTS ${_jinja2cpp_boost_compiled} ${_jinja2cpp_boost_header_only})
    set(JINJA2CPP_BOOST_LIBS)
    foreach (_lib IN LISTS _jinja2cpp_boost_header_only)
        list(APPEND JINJA2CPP_BOOST_LIBS Boost::${_lib})
    endforeach ()
else ()
    find_package(Boost 1.75 CONFIG REQUIRED COMPONENTS ${_jinja2cpp_boost_compiled})
    # Quoted: an empty list must stay defined, or the installed config asks for algorithm and any
    set(JINJA2CPP_BOOST_COMPONENTS "${_jinja2cpp_boost_compiled}")
    set(JINJA2CPP_BOOST_LIBS Boost::headers)
endif ()
set(JINJA2CPP_BOOST_VERSION ${Boost_VERSION})
