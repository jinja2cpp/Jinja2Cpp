if(MSVC)
    set (THIRDPARTY_RUNTIME_TYPE ${JINJA2CPP_MSVC_RUNTIME_TYPE})
    if ("${THIRDPARTY_RUNTIME_TYPE}" STREQUAL "")
        string (FIND "${CURRENT_CXX_FLAGS}" "MT" THIRDPARTY_MT_POS REVERSE)
        string (FIND "${CURRENT_CXX_FLAGS}" "MD" THIRDPARTY_MD_POS REVERSE)
        if (NOT THIRDPARTY_MT_POS EQUAL -1)
            set (THIRDPARTY_RUNTIME_TYPE "/MT")
        elseif (NOT THIRDPARTY_MD_POS EQUAL -1)
            set (THIRDPARTY_RUNTIME_TYPE "/MD")
        else ()
            message (STATUS "Dynamic C runtime assumed. Use 'JINJA2CPP_MSVC_RUNTIME_TYPE' variable for override")
            set (THIRDPARTY_RUNTIME_TYPE "/MD")
        endif()
    endif ()
    message(STATUS "Selected Jinja2C++ thirdparty libraries runtime type: '${THIRDPARTY_RUNTIME_TYPE}'")
endif ()

function(update_submodule submodule)
    find_package(Git REQUIRED)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive thirdparty/${submodule}
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    )
endfunction()

function(imported_target_alias ALIAS)
    # For some unknown reason CMake does not support creating alias
    # libraries from IMPORTED libraries. This function is an ugly workaround
    # to get the same

    cmake_parse_arguments("__ALIAS"
        ""
        "ALIAS"
        ""
        ${ARGN}
    )

    if(NOT __ALIAS_ALIAS)
        message(FATAL_ERROR "imported_target_alias invoked with wrong arguments, missing ALIAS")
    endif()

    add_library(${ALIAS} INTERFACE)
    target_link_libraries(${ALIAS} INTERFACE ${__ALIAS_ALIAS})
endfunction()

# Treats the headers of every target defined under `dir` (a FetchContent source directory)
# as system headers, so our warning flags (and -Werror) do not apply to them. Imported
# targets (find_package, Conan) are system already. CMake 3.25 has FetchContent SYSTEM for
# this; the floor is 3.23.
function(jinja2cpp_mark_system dir)
    get_property(targets DIRECTORY "${dir}" PROPERTY BUILDSYSTEM_TARGETS)
    foreach(target IN LISTS targets)
        get_target_property(type ${target} TYPE)
        get_target_property(aliased ${target} ALIASED_TARGET)
        if(aliased OR type STREQUAL "UTILITY")
            continue()
        endif()
        get_target_property(includes ${target} INTERFACE_INCLUDE_DIRECTORIES)
        if(NOT includes)
            continue()
        endif()
        # Build tree only, so the dependencies' installed exports stay as they were
        foreach(include IN LISTS includes)
            set_property(TARGET ${target} APPEND PROPERTY INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "$<BUILD_INTERFACE:${include}>")
        endforeach()
    endforeach()
    get_property(subdirs DIRECTORY "${dir}" PROPERTY SUBDIRECTORIES)
    foreach(subdir IN LISTS subdirs)
        jinja2cpp_mark_system("${subdir}")
    endforeach()
endfunction()

include (./thirdparty/thirdparty-${JINJA2CPP_DEPS_MODE}.cmake)

# Boost.Multiprecision in standalone mode, which needs only Boost.Config besides itself
# (docs/tasks/0272). The library compiles it in with -DJINJA2CPP_WITH_BIGINT=boost; the tests
# check the header-only adapter (jinja2cpp/big_int/boost_cpp_int.h) in every build. Nothing
# else needs it, so the default build fetches no Boost (docs/tasks/0230).
if (JINJA2CPP_BUILD_TESTS OR "${JINJA2CPP_WITH_BIGINT}" STREQUAL "boost")
    add_library(jinja2cpp_boost_multiprecision INTERFACE)
    if (DEFINED boost_SOURCE_DIR)
        # The internal mode's Boost release, fetched for the Boost.JSON adapter test, carries
        # every library's headers; standalone mode needs only Multiprecision's and Config's
        target_compile_definitions(jinja2cpp_boost_multiprecision INTERFACE BOOST_MP_STANDALONE=1)
        target_include_directories(jinja2cpp_boost_multiprecision SYSTEM INTERFACE
            "${boost_SOURCE_DIR}/libs/multiprecision/include" "${boost_SOURCE_DIR}/libs/config/include")
    elseif (TARGET Boost::headers)
        # A Boost found by the external or Conan modes is used whole, so any release
        # find_package accepts works, also one older than standalone Multiprecision
        target_link_libraries(jinja2cpp_boost_multiprecision INTERFACE Boost::headers)
    else ()
        # The two libraries on their own. Their CMakeLists want other Boost targets, so
        # SOURCE_SUBDIR names a directory they lack and FetchContent only unpacks them.
        include(FetchContent)
        FetchContent_Declare(
            boost_multiprecision
            URL https://github.com/boostorg/multiprecision/archive/refs/tags/boost-1.92.0.tar.gz
            URL_HASH SHA256=9da997843edd802f9d5e25d53872a7676330f31a52a5e47a418d1c94ba3b01d0
            SOURCE_SUBDIR jinja2cpp-headers-only
        )
        FetchContent_Declare(
            boost_config
            URL https://github.com/boostorg/config/archive/refs/tags/boost-1.92.0.tar.gz
            URL_HASH SHA256=b4171037f13373203ba79cbc141d612982052283e696a315185ab5bea46102a0
            SOURCE_SUBDIR jinja2cpp-headers-only
        )
        FetchContent_MakeAvailable(boost_multiprecision boost_config)
        target_compile_definitions(jinja2cpp_boost_multiprecision INTERFACE BOOST_MP_STANDALONE=1)
        target_include_directories(jinja2cpp_boost_multiprecision SYSTEM INTERFACE
            "${boost_multiprecision_SOURCE_DIR}/include" "${boost_config_SOURCE_DIR}/include")
    endif ()
endif ()

if(JINJA2CPP_BUILD_TESTS)
    find_package(GTest QUIET)

    if(gtest_FOUND)
        imported_target_alias(gtest ALIAS gtest::gtest)
        imported_target_alias(gmock ALIAS gmock::gmock)
    else()
        message(STATUS "gtest not found, building own one")
        if(MSVC)
            if (THIRDPARTY_RUNTIME_TYPE STREQUAL "/MD" OR THIRDPARTY_RUNTIME_TYPE STREQUAL "/MDd")
                set (gtest_force_shared_crt ON CACHE BOOL "" FORCE)
            else ()
                set (gtest_force_shared_crt OFF CACHE BOOL "" FORCE)
            endif ()
        endif ()
        # The test framework is not part of the package (sbom/components.json)
        set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
        set(JINJA2CPP_GTEST_TAG "v1.18.0")
        # only the internal deps mode includes FetchContent itself
        include(FetchContent)
        FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG ${JINJA2CPP_GTEST_TAG}
        )
        FetchContent_MakeAvailable(googletest)
        jinja2cpp_mark_system("${googletest_SOURCE_DIR}")
    endif()
endif()

if(JINJA2CPP_BUILD_BENCHMARKS)
    if(NOT JINJA2CPP_BUILD_TESTS)
        # The suite reads its case data with nlohmann_json, a test dependency
        message(FATAL_ERROR "JINJA2CPP_BUILD_BENCHMARKS needs JINJA2CPP_BUILD_TESTS")
    endif()
    find_package(benchmark QUIET)
    if(NOT benchmark_FOUND)
        message(STATUS "google benchmark not found, building own one")
        set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
        set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
        set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
        set(BENCHMARK_ENABLE_WERROR OFF CACHE BOOL "" FORCE)
        include(FetchContent)
        FetchContent_Declare(
            benchmark
            GIT_REPOSITORY https://github.com/google/benchmark.git
            GIT_TAG v1.9.5
        )
        FetchContent_MakeAvailable(benchmark)
    endif()
endif()

if (NOT DEFINED JINJA2_PRIVATE_LIBS_INT)
    set(JINJA2CPP_PRIVATE_LIBS ${JINJA2CPP_PRIVATE_LIBS}
        fmt
    )
else ()
    set (JINJA2CPP_PRIVATE_LIBS ${JINJA2_PRIVATE_LIBS_INT})
endif ()

if (NOT DEFINED JINJA2_PUBLIC_LIBS_INT)
    set (JINJA2CPP_PUBLIC_LIBS ${JINJA2CPP_PUBLIC_LIBS}
        expected-lite)
else ()
    set (JINJA2CPP_PUBLIC_LIBS ${JINJA2_PUBLIC_LIBS_INT})
endif ()
