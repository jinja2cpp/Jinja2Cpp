cmake_minimum_required(VERSION 3.23.0..4.0)

if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

project(Jinja2Cpp VERSION 2.0.0)

if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")
    set(JINJA2CPP_IS_MAIN_PROJECT TRUE)
else()
    set(JINJA2CPP_IS_MAIN_PROJECT FALSE)
endif()

# Options
set(JINJA2CPP_SANITIZERS address+undefined memory thread)
set(JINJA2CPP_WITH_SANITIZERS none CACHE STRING "Build with sanitizer")
set_property(CACHE JINJA2CPP_WITH_SANITIZERS PROPERTY STRINGS ${JINJA2CPP_SANITIZERS})
if (DEFINED JINJA2CPP_USE_REGEX)
    # The lexer no longer uses a regex (docs/tasks/0086), so Boost.Regex is not needed (0090).
    message(DEPRECATION "JINJA2CPP_USE_REGEX is deprecated and ignored: Jinja2Cpp no longer uses Boost.Regex")
    unset(JINJA2CPP_USE_REGEX CACHE)
endif ()
set(JINJA2CPP_WITH_JSON_SUPPORTED_BINDINGS boost nlohmann rapid)
set(JINJA2CPP_WITH_JSON_BINDINGS boost CACHE STRING "Build with json support(boost|rapid|nlohmann) for serialization operations, like tojson filter. 'boost' is default.")
set_property(CACHE JINJA2CPP_WITH_JSON_BINDINGS PROPERTY STRINGS ${JINJA2CPP_WITH_JSON_SUPPORTED_BINDINGS})
set(JINJA2CPP_WITH_BIGINT_SUPPORTED_BACKENDS none boost)
set(JINJA2CPP_WITH_BIGINT none CACHE STRING "Built-in big-integer backend (none|boost): with 'boost', integer arithmetic beyond int64 is exact by default (docs/tasks/0272). 'none' is default.")
set_property(CACHE JINJA2CPP_WITH_BIGINT PROPERTY STRINGS ${JINJA2CPP_WITH_BIGINT_SUPPORTED_BACKENDS})
if (NOT JINJA2CPP_WITH_BIGINT IN_LIST JINJA2CPP_WITH_BIGINT_SUPPORTED_BACKENDS)
    message(FATAL_ERROR "JINJA2CPP_WITH_BIGINT must be one of ${JINJA2CPP_WITH_BIGINT_SUPPORTED_BACKENDS}, not '${JINJA2CPP_WITH_BIGINT}'")
endif ()
set (JINJA2CPP_DEPS_MODE "internal" CACHE STRING "Jinja2Cpp dependency management mode (internal | external | external-boost | conan-build). See documentation for details. 'interal' is default.")
option(JINJA2CPP_BUILD_TESTS "Build Jinja2Cpp unit tests" ${JINJA2CPP_IS_MAIN_PROJECT})
option(JINJA2CPP_BUILD_BENCHMARKS "Build the Google Benchmark suite in bench/ (needs JINJA2CPP_BUILD_TESTS)" OFF)
option(JINJA2CPP_BUILD_FUZZERS "Build the libFuzzer targets in fuzz/ (clang only)" OFF)
option(JINJA2CPP_STRICT_WARNINGS "Enable additional warnings and treat them as errors" ON)
set(JINJA2CPP_NODEREF_CHECKS "" CACHE STRING "Checks of the handles that link a parse tree: OFF, ON (Seal and cross-template links), FULL (every access); empty picks FULL for Debug, sanitizer and fuzz builds, else ON")
set_property(CACHE JINJA2CPP_NODEREF_CHECKS PROPERTY STRINGS "" ON FULL OFF)
option(JINJA2CPP_CHECK_SLOTS "Check every name read from a slot against its lookup by name (always on without NDEBUG)" OFF)
option(JINJA2CPP_BUILD_SHARED "Build shared linkage version of Jinja2Cpp" OFF)
option(JINJA2CPP_PIC "Control -fPIC option for library build" OFF)
option(JINJA2CPP_VERBOSE "Add extra debug output to the build scripts" OFF)
option(JINJA2CPP_INSTALL "Add installation rules for JinjaCpp targets" ${JINJA2CPP_IS_MAIN_PROJECT})

if (DEFINED BUILD_SHARED_LIBS)
    set(JINJA2CPP_BUILD_SHARED ${BUILD_SHARED_LIBS})
endif ()

if (JINJA2CPP_BUILD_SHARED)
    set(JINJA2CPP_PIC ON)
    set(JINJA2CPP_MSVC_RUNTIME_TYPE "/MD")
endif ()

if (NOT JINJA2CPP_DEPS_MODE)
    set(JINJA2CPP_DEPS_MODE "internal")
endif ()

if (JINJA2CPP_IS_MAIN_PROJECT OR NOT CMAKE_CXX_STANDARD)
    set(JINJA2CPP_CXX_STANDARD 17 CACHE STRING "Jinja2Cpp C++ standard to build with: 17 (default), 20, 23 or 26")
    set(CMAKE_CXX_STANDARD ${JINJA2CPP_CXX_STANDARD})
endif ()

if (NOT JINJA2CPP_CXX_STANDARD)
    set (JINJA2CPP_CXX_STANDARD ${CMAKE_CXX_STANDARD})
endif ()

if (JINJA2CPP_CXX_STANDARD LESS 17)
    message(FATAL_ERROR "Jinja2Cpp requires C++17 or newer. Currently selected standard: ${JINJA2CPP_CXX_STANDARD}")
else ()
    message(STATUS "Jinja2Cpp C++ standard: ${JINJA2CPP_CXX_STANDARD}")
endif ()

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set(CLANG_CXX_FLAGS)
set(GCC_CXX_FLAGS)
set(MSVC_CXX_FLAGS)

if (JINJA2CPP_WITH_COVERAGE)
    message(STATUS "This is DEBUG build with enabled Code Coverage")
    set(CMAKE_BUILD_TYPE Debug)
    set(JINJA2CPP_COVERAGE_TARGET "jinja2cpp_build_coverage")
    include(coverage)
    add_coverage_target("${JINJA2CPP_COVERAGE_TARGET}")
endif ()

if(NOT ${JINJA2CPP_WITH_SANITIZERS} STREQUAL "none")
    message (STATUS "Build with sanitizers enabled: ${JINJA2CPP_WITH_SANITIZERS}")
    set(_chosen_san)
    list(FIND JINJA2CPP_SANITIZERS ${JINJA2CPP_WITH_SANITIZERS} _chosen_san)
    if (${_chosen_san} EQUAL -1)
        message(FATAL_ERROR "Wrong sanitizer type has been chosen, must be one of ${JINJA2CPP_SANITIZERS}")
    endif()

    include("sanitizer.${JINJA2CPP_WITH_SANITIZERS}")
    set (JINJA2CPP_SANITIZE_TARGET "jinja2cpp_build_sanitizers")
    add_sanitizer_target(${JINJA2CPP_SANITIZE_TARGET})
endif()

if (UNIX)
    if (JINJA2CPP_PIC OR CONAN_CMAKE_POSITION_INDEPENDENT_CODE)
        add_compile_options(-fPIC)
    endif ()

    if (DEFINED CONAN_SHARED_LINKER_FLAGS)
        set(GCC_CXX_FLAGS ${GCC_CXX_FLAGS} -Wl,${CONAN_SHARED_LINKER_FLAGS})
        set(CLANG_CXX_FLAGS ${CLANG_CXX_FLAGS} -Wl,${CONAN_SHARED_LINKER_FLAGS})
    endif ()
else ()
    set(GCC_CXX_FLAGS ${GCC_CXX_FLAGS} "-Wa,-mbig-obj" -O1)
    if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    endif ()
    add_definitions(-DBOOST_ALL_NO_LIB)
    set(MSVC_CXX_FLAGS ${MSVC_CXX_FLAGS} /wd4503 /bigobj /Zc:__cplusplus)

    if (CMAKE_BUILD_TYPE MATCHES "Debug" AND JINJA2CPP_MSVC_RUNTIME_TYPE)
        set(JINJA2CPP_MSVC_RUNTIME_TYPE "${JINJA2CPP_MSVC_RUNTIME_TYPE}d")
    endif ()
    if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
        if (JINJA2CPP_DEPS_MODE MATCHES "conan-build" OR CMAKE_BUILD_TYPE STREQUAL "")
            if (NOT JINJA2CPP_MSVC_RUNTIME_TYPE STREQUAL "")
                set(MSVC_RUNTIME_DEBUG ${JINJA2CPP_MSVC_RUNTIME_TYPE})
                set(MSVC_RUNTIME_RELEASE ${JINJA2CPP_MSVC_RUNTIME_TYPE})
            endif ()
            set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${MSVC_RUNTIME_DEBUG}")
            set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${MSVC_RUNTIME_RELEASE}")
            set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} ${MSVC_RUNTIME_RELEASE}")
            set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "/PROFILE")
            set(Boost_USE_DEBUG_RUNTIME OFF)
        elseif (CMAKE_BUILD_TYPE MATCHES "Debug")
            set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${JINJA2CPP_MSVC_RUNTIME_TYPE}")
            set(Boost_USE_DEBUG_RUNTIME ON)
        else ()
            set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${JINJA2CPP_MSVC_RUNTIME_TYPE}")
            set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} ${JINJA2CPP_MSVC_RUNTIME_TYPE}")
            set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "/PROFILE")
            set(Boost_USE_DEBUG_RUNTIME OFF)
        endif ()
        message(STATUS "Selected MSVC runtime type for Jinja2C++ library: '${JINJA2CPP_MSVC_RUNTIME_TYPE}'")
    endif ()
endif()


if (JINJA2CPP_BUILD_SHARED)
    set(LIB_LINK_TYPE SHARED)
    message(STATUS "Jinja2C++ library type: SHARED")
else()
    set(LIB_LINK_TYPE STATIC)
    message(STATUS "Jinja2C++ library type: STATIC")
endif()

include(collect_sources)

set (LIB_TARGET_NAME jinja2cpp)

set(JINJA2CPP_SOURCES
    src/confined_filesystem.cpp
    src/cycle_collector.cpp
    src/error_info.cpp
    src/big_int.cpp
    src/big_int_builtin.cpp
    src/expression_evaluator.cpp
    src/expression_parser.cpp
    src/filesystem_handler.cpp
    src/filter_factories.cpp
    src/filters.cpp
    src/generic_list.cpp
    src/global_functions.cpp
    src/internal_value.cpp
    src/lexer.cpp
    src/name_resolver.cpp
    src/node_arena.cpp
    src/out_stream.cpp
    src/python_format.cpp
    src/recursion_guard.cpp
    src/render_failure.cpp
    src/render_workspace.cpp
    src/serialize_filters.cpp
    src/statements.cpp
    src/string_converter_filter.cpp
    src/template.cpp
    src/template_dict.cpp
    src/template_env.cpp
    src/template_module.cpp
    src/template_parser.cpp
    src/tester_factories.cpp
    src/testers.cpp
    src/undefined.cpp
    src/value.cpp
    src/value_methods.cpp
)

CollectSources(PublicSources PublicHeaders ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(${LIB_TARGET_NAME} ${LIB_LINK_TYPE}
    ${JINJA2CPP_SOURCES}
)

add_library(jinja2cpp::jinja2cpp ALIAS ${LIB_TARGET_NAME})

# jinja2cpp/version.h carries project(VERSION) into JINJA2CPP_VERSION (docs/tasks/0069)
set(JINJA2CPP_GENERATED_INCLUDE_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated/include)
configure_file(cmake/version.h.in ${JINJA2CPP_GENERATED_INCLUDE_DIR}/jinja2cpp/version.h @ONLY)

target_sources(${LIB_TARGET_NAME}
    PUBLIC FILE_SET HEADERS
    FILES ${PublicHeaders} ${JINJA2CPP_GENERATED_INCLUDE_DIR}/jinja2cpp/version.h
    BASE_DIRS include ${JINJA2CPP_GENERATED_INCLUDE_DIR}
)

string(TOUPPER "${CMAKE_BUILD_TYPE}" BUILD_CFG_NAME)
set(CURRENT_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_${BUILD_CFG_NAME}}")

set(JINJA2CPP_EXTRA_LIBS "" CACHE STRING "You can pass some libs that could used during link stage")
set(JINJA2CPP_PUBLIC_LIBS "${JINJA2CPP_EXTRA_LIBS}")
separate_arguments(JINJA2CPP_PUBLIC_LIBS)
if (JINJA2CPP_WITH_COVERAGE)
    target_compile_options(
        ${JINJA2CPP_COVERAGE_TARGET}
        INTERFACE
        -g -O0
    )
    list(APPEND JINJA2CPP_PUBLIC_LIBS ${JINJA2CPP_COVERAGE_TARGET})
endif()
if (NOT JINJA2CPP_WITH_SANITIZERS STREQUAL "none")
    target_compile_options(
        ${JINJA2CPP_SANITIZE_TARGET}
        INTERFACE
        -g -O2
    )
    list(APPEND JINJA2CPP_PUBLIC_LIBS ${JINJA2CPP_SANITIZE_TARGET})
endif()

set(JINJA2CPP_PRIVATE_LIBS "${JINJA2CPP_PRIVATE_LIBS}")
include(thirdparty/CMakeLists.txt)


if (JINJA2CPP_STRICT_WARNINGS)
    if (UNIX)
        # Shared by GCC and Clang; dependencies come in as system headers (thirdparty/CMakeLists.txt)
        set(_strict_warnings -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion
            -Wnon-virtual-dtor -Woverloaded-virtual -Wold-style-cast -Wimplicit-fallthrough -Werror)
        # A profile-guided build (bench/README.md) changes GCC's inlining enough for a false
        # -Warray-bounds in the vendored robin_hood.h; keep it a warning. GCC 13 Release also
        # reports a false -Wmaybe-uninitialized in expected-lite's move assignment (ParseResult);
        # boost_variant used to hand this flag to the library as an INTERFACE option (0229)
        set(GCC_CXX_FLAGS ${GCC_CXX_FLAGS} ${_strict_warnings} -Wno-error=array-bounds
            -Wno-error=maybe-uninitialized)
        set(CLANG_CXX_FLAGS ${CLANG_CXX_FLAGS} ${_strict_warnings} -Wno-unused-command-line-argument)
    elseif (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
        # Dependencies come in as system headers (thirdparty/CMakeLists.txt), which /W4 leaves alone
        # C4702 is reported by the code generator, after /external:W0 no longer applies, so
        # fmt's unreachable `return` after a throwing branch (fmt/base.h) would fail /WX
        set(MSVC_CXX_FLAGS ${MSVC_CXX_FLAGS} /W4 /WX /external:W0 /wd4702)
    endif ()
endif ()
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    set(JINJA2CPP_TARGET_CXX_FLAGS ${GCC_CXX_FLAGS})
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(JINJA2CPP_TARGET_CXX_FLAGS ${CLANG_CXX_FLAGS})
elseif (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    set(JINJA2CPP_TARGET_CXX_FLAGS ${MSVC_CXX_FLAGS})
endif ()
target_compile_options(${LIB_TARGET_NAME} PRIVATE ${JINJA2CPP_TARGET_CXX_FLAGS})


if ("${JINJA2CPP_WITH_JSON_BINDINGS}" STREQUAL "boost")
    set(_bindings_define "-DJINJA2CPP_WITH_JSON_BINDINGS_BOOST")
    list(APPEND JINJA2CPP_PRIVATE_LIBS Boost::json)
elseif("${JINJA2CPP_WITH_JSON_BINDINGS}" STREQUAL "rapid")
    set(_bindings_define "-DJINJA2CPP_WITH_JSON_BINDINGS_RAPID")
    list(APPEND JINJA2CPP_PRIVATE_LIBS $<BUILD_INTERFACE:RapidJson>)
else()
    set(_bindings_define "-DJINJA2CPP_WITH_JSON_BINDINGS_NLOHMANN")
    # header-only and private: keep it out of the exported link interface
    list(APPEND JINJA2CPP_PRIVATE_LIBS $<BUILD_INTERFACE:nlohmann_json::nlohmann_json>)
endif()

# The built-in big-integer backend: header-only and private, so consumers need nothing for it
if ("${JINJA2CPP_WITH_BIGINT}" STREQUAL "boost")
    list(APPEND JINJA2CPP_PRIVATE_LIBS $<BUILD_INTERFACE:jinja2cpp_boost_multiprecision>)
    target_compile_definitions(${LIB_TARGET_NAME} PRIVATE JINJA2CPP_WITH_BIGINT_BOOST)
endif()
message(STATUS "Jinja2C++ built-in big-integer backend: ${JINJA2CPP_WITH_BIGINT}")

target_link_libraries(
        ${LIB_TARGET_NAME}
    PUBLIC
        ${JINJA2CPP_PUBLIC_LIBS}
    PRIVATE
        ${JINJA2CPP_PRIVATE_LIBS}
)

target_include_directories(
        ${LIB_TARGET_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_definitions(
        ${LIB_TARGET_NAME}
    PUBLIC
        -DBOOST_SYSTEM_NO_DEPRECATED
        -DBOOST_ERROR_CODE_HEADER_ONLY
        ${_bindings_define}
)
# Result<T> is expected-lite's own nonstd::expected at every standard (docs/api-2.0.md,
# decision 4): one type for library and consumers, and Clang 18 with libstdc++ at C++23
# has an <expected> header but no std::expected in it.
target_compile_definitions(
        ${LIB_TARGET_NAME}
    PUBLIC
        nsel_CONFIG_SELECT_EXPECTED=nsel_EXPECTED_NONSTD
)
if (JINJA2CPP_CHECK_SLOTS)
    target_compile_definitions(${LIB_TARGET_NAME} PUBLIC JINJA2CPP_CHECK_SLOTS)
endif ()
# The level is part of the node layout's checks, so every target that includes src/ takes it
# from the library; installed consumers never include src/ and never see it
string(TOUPPER "${JINJA2CPP_NODEREF_CHECKS}" _noderef_checks)
if (_noderef_checks STREQUAL "OFF")
    set(_noderef_level 0)
elseif (_noderef_checks STREQUAL "ON")
    set(_noderef_level 1)
elseif (_noderef_checks STREQUAL "FULL")
    set(_noderef_level 2)
elseif (_noderef_checks STREQUAL "")
    if (NOT JINJA2CPP_WITH_SANITIZERS STREQUAL "none" OR JINJA2CPP_BUILD_FUZZERS)
        set(_noderef_level 2)
        set(_noderef_checks "FULL")
    else ()
        set(_noderef_level "$<IF:$<CONFIG:Debug>,2,1>")
        set(_noderef_checks "FULL in Debug, else ON")
    endif ()
else ()
    message(FATAL_ERROR "JINJA2CPP_NODEREF_CHECKS is OFF, ON, FULL or empty, not '${JINJA2CPP_NODEREF_CHECKS}'")
endif ()
message(STATUS "Jinja2C++ node reference checks: ${_noderef_checks}")
target_compile_definitions(${LIB_TARGET_NAME} PUBLIC "$<BUILD_INTERFACE:JINJA2CPP_NODEREF_CHECKS_LEVEL=${_noderef_level}>")
if (JINJA2CPP_BUILD_SHARED)
    target_compile_definitions(${LIB_TARGET_NAME} PRIVATE -DJINJA2CPP_BUILD_AS_SHARED PUBLIC -DJINJA2CPP_LINK_AS_SHARED)
endif ()

set_target_properties(${LIB_TARGET_NAME} PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 2
    # Only what JINJA2CPP_EXPORT marks leaves the shared library, as on Windows
    # (config.h), so internal changes do not show as ABI changes (docs/tasks/0076)
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)
# Nor do the static dependencies linked into it (fmt, Boost in the internal mode)
if (JINJA2CPP_BUILD_SHARED AND UNIX AND NOT APPLE)
    target_link_options(${LIB_TARGET_NAME} PRIVATE "LINKER:--exclude-libs,ALL")
endif ()
# An installed shared library finds the shared dependencies installed next to it
if (APPLE)
    set_target_properties(${LIB_TARGET_NAME} PROPERTIES INSTALL_RPATH "@loader_path")
elseif (UNIX)
    set_target_properties(${LIB_TARGET_NAME} PROPERTIES INSTALL_RPATH "$ORIGIN")
endif ()

set_target_properties(${LIB_TARGET_NAME} PROPERTIES
    CXX_STANDARD ${JINJA2CPP_CXX_STANDARD}
    CXX_STANDARD_REQUIRED ON
)
# Consumers get at least the floor the public headers need.
target_compile_features(${LIB_TARGET_NAME} PUBLIC cxx_std_17)

# One ".." per component of the relative pkgconfig directory (file(RELATIVE_PATH) rejects
# "/" as a full path on Windows)
string(REGEX REPLACE "[^/]+" ".." JINJA2CPP_PC_PREFIX_REL "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
set(JINJA2CPP_PC_CFLAGS "-Dnsel_CONFIG_SELECT_EXPECTED=nsel_EXPECTED_NONSTD")
if (JINJA2CPP_BUILD_SHARED)
    string(APPEND JINJA2CPP_PC_CFLAGS " -DJINJA2CPP_LINK_AS_SHARED")
endif ()
set(JINJA2CPP_PC_REQUIRES_PRIVATE "")
set(JINJA2CPP_PC_LIBS_PRIVATE "")
if (JINJA2CPP_DEPS_MODE STREQUAL "internal")
    set(JINJA2CPP_PC_REQUIRES_PRIVATE "fmt")
    set(JINJA2CPP_PC_LIBS_PRIVATE "-lboost_filesystem -lboost_atomic")
    if ("${JINJA2CPP_WITH_JSON_BINDINGS}" STREQUAL "boost")
        string(APPEND JINJA2CPP_PC_LIBS_PRIVATE " -lboost_json -lboost_container")
    endif ()
endif ()
configure_file(jinja2cpp.pc.in ${CMAKE_BINARY_DIR}/jinja2cpp.pc @ONLY)

if (JINJA2CPP_BUILD_TESTS)
    enable_testing()

    CollectSources(TestSources TestHeaders ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/test)
    set(JINJA2CPP_TEST_SOURCES
        test/autoescape_test.cpp
        test/basic_tests.cpp
        test/big_int_test.cpp
        test/containers_api_test.cpp
        test/errors_test.cpp
        test/expressions_test.cpp
        test/extends_test.cpp
        test/filesystem_handler_test.cpp
        test/filters_test.cpp
        test/forloop_test.cpp
        test/fwd_test.cpp
        test/global_functions_test.cpp
        test/helpers_tests.cpp
        test/i18n_test.cpp
        test/limits_test.cpp
        test/if_test.cpp
        test/import_test.cpp
        test/literals_test.cpp
        test/includes_test.cpp
        test/internal_value_test.cpp
        test/macro_test.cpp
        test/output_test.cpp
        test/metadata_test.cpp
        test/name_map_test.cpp
        test/name_resolver_test.cpp
        test/node_arena_test.cpp
        test/arena_switch_test.cpp
        test/ordered_map_test.cpp
        test/recursion_limits_test.cpp
        test/slot_frame_test.cpp
        test/statements_tets.cpp
        test/template_api_test.cpp
        test/template_env_test.cpp
        test/template_module_test.cpp
        test/test_data
        test/test_tools.h
        test/testers_test.cpp
        test/tojson_filter_test.cpp
        test/undeclared_variables_test.cpp
        test/undefined_test.cpp
        test/user_callable_test.cpp
        test/v1_names_test.cpp
        test/value_ptr_test.cpp
        test/parity/parity_test.cpp
    )
    add_executable(jinja2cpp_tests ${JINJA2CPP_TEST_SOURCES})
    target_compile_options(jinja2cpp_tests PRIVATE ${JINJA2CPP_TARGET_CXX_FLAGS})
    target_link_libraries(jinja2cpp_tests gtest gmock gtest_main
        nlohmann_json::nlohmann_json ${LIB_TARGET_NAME} ${EXTRA_TEST_LIBS} ${JINJA2CPP_PRIVATE_LIBS})
    # The Boost big-integer adapter is checked in every build; the built-in one only in boost builds
    target_link_libraries(jinja2cpp_tests jinja2cpp_boost_multiprecision)
    if ("${JINJA2CPP_WITH_BIGINT}" STREQUAL "boost")
        target_compile_definitions(jinja2cpp_tests PRIVATE JINJA2CPP_WITH_BIGINT_BOOST)
    endif ()

    set_target_properties(jinja2cpp_tests PROPERTIES
            CXX_STANDARD ${JINJA2CPP_CXX_STANDARD}
            CXX_STANDARD_REQUIRED ON)

    # fwd.h must repeat each type's class-key (MSVC mangles it, C4099)
    if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        set_source_files_properties(test/fwd_test.cpp PROPERTIES COMPILE_OPTIONS "-Wmismatched-tags")
    endif ()

    if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
        target_compile_options(jinja2cpp_tests PRIVATE /bigobj)
        # RecursionLimitsTest renders 200 nested macro calls, which Python Jinja2 accepts;
        # Debug builds need more than the default 1 MiB stack for that (docs/tasks/0003)
        target_link_options(jinja2cpp_tests PRIVATE /STACK:8388608)
    endif ()

    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/test_data/simple_template1.j2tpl
        COMMAND ${CMAKE_COMMAND} ARGS -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/test/test_data ${CMAKE_CURRENT_BINARY_DIR}/test_data
        MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/test/test_data/simple_template1.j2tpl
        COMMENT "Copy test data to the destination dir"
    )

    add_custom_target(CopyTestData ALL
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/test_data/simple_template1.j2tpl
    )

    add_dependencies(jinja2cpp_tests CopyTestData)

    add_test(NAME jinja2cpp_tests COMMAND jinja2cpp_tests --gtest_filter=-Parity*)

    # Differential corpus against Python Jinja2, see test/parity/README.md
    add_test(NAME jinja2cpp_parity COMMAND jinja2cpp_tests --gtest_filter=Parity*)
    # Every jinja2 symbol the library defines is in the inline ABI namespace v2 (docs/tasks/0076)
    if (CMAKE_NM AND NOT MSVC)
        add_test(NAME jinja2cpp_abi_namespace
            COMMAND ${CMAKE_COMMAND} -DNM=${CMAKE_NM} -DLIBRARY=$<TARGET_FILE:jinja2cpp>
                -P ${CMAKE_CURRENT_SOURCE_DIR}/test/abi_symbols.cmake)
    endif ()

    # Replaces the global operator new, so it cannot share the main test binary (docs/tasks/0253).
    # The TSan and MSan runtimes define operator new themselves, so those builds go without it.
    if (NOT JINJA2CPP_WITH_SANITIZERS MATCHES "thread|memory")
        add_executable(jinja2cpp_alloc_tests test/allocation_failure_test.cpp)
        target_compile_options(jinja2cpp_alloc_tests PRIVATE ${JINJA2CPP_TARGET_CXX_FLAGS})
        target_link_libraries(jinja2cpp_alloc_tests gtest gtest_main ${LIB_TARGET_NAME} ${EXTRA_TEST_LIBS} ${JINJA2CPP_PRIVATE_LIBS})
        set_target_properties(jinja2cpp_alloc_tests PROPERTIES
                CXX_STANDARD ${JINJA2CPP_CXX_STANDARD}
                CXX_STANDARD_REQUIRED ON)
        target_compile_definitions(jinja2cpp_alloc_tests PRIVATE ${_bindings_define})
        # After the parity run, whose output would push this one out of the CI log tail
        add_test(NAME jinja2cpp_alloc_tests COMMAND jinja2cpp_alloc_tests)
    endif()

    if ("${JINJA2CPP_WITH_JSON_BINDINGS}" STREQUAL "boost")
        set(_bindings_define "-DJINJA2CPP_WITH_JSON_BINDINGS_BOOST")
        target_sources(jinja2cpp_tests
            PRIVATE
                test/binding/boost_json_binding_test.cpp
        )
    endif()
    if("${JINJA2CPP_WITH_JSON_BINDINGS}" STREQUAL "rapid")
        set(_bindings_define "-DJINJA2CPP_WITH_JSON_BINDINGS_RAPID")
        target_sources(jinja2cpp_tests
            PRIVATE
                test/binding/rapid_json_binding_test.cpp
        )
    endif()
    if("${JINJA2CPP_WITH_JSON_BINDINGS}" STREQUAL "nlohmann")
        set(_bindings_define "-DJINJA2CPP_WITH_JSON_BINDINGS_NLOHMANN")
        target_sources(jinja2cpp_tests
            PRIVATE
                test/binding/nlohmann_json_binding_test.cpp
        )
    endif()
    target_compile_definitions(jinja2cpp_tests
        PUBLIC
            -DJINJA2CPP_PARITY_DIR="${CMAKE_CURRENT_SOURCE_DIR}/test/parity"
            -DBOOST_SYSTEM_NO_DEPRECATED
            -DBOOST_ERROR_CODE_HEADER_ONLY
            ${_bindings_define}
    )

endif ()

if (JINJA2CPP_BUILD_BENCHMARKS)
    add_subdirectory(bench)
endif ()

if (JINJA2CPP_BUILD_TESTS OR JINJA2CPP_BUILD_FUZZERS)
    add_subdirectory(fuzz)
endif ()

# The package is generated by install(EXPORT) (docs/tasks/0068); cmake/public/ only adds
# the find_dependency calls for what the exported targets refer to.
if(JINJA2CPP_INSTALL)
    set(JINJA2CPP_INSTALL_CONFIG_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/jinja2cpp")
    install(
        TARGETS
            ${LIB_TARGET_NAME}
        EXPORT
            InstallTargets
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        FILE_SET HEADERS
    )

    install(
        FILES
            ${CMAKE_BINARY_DIR}/jinja2cpp.pc
        DESTINATION
            ${CMAKE_INSTALL_LIBDIR}/pkgconfig
    )

    # The library SBOM, generated from sbom/components.json by scripts/sbom.py (docs/tasks/0187)
    install(
        FILES
            sbom/jinja2cpp.cdx.json
            sbom/jinja2cpp.spdx.json
        DESTINATION
            ${CMAKE_INSTALL_DOCDIR}/sbom
    )

    install(
        EXPORT
            InstallTargets
        FILE
            jinja2cpp-targets.cmake
        NAMESPACE
            jinja2cpp::
        DESTINATION
            ${JINJA2CPP_INSTALL_CONFIG_DIR}
    )

    if (JINJA2CPP_BUILD_SHARED)
        set(JINJA2CPP_BUILD_SHARED_BOOL TRUE)
    else ()
        set(JINJA2CPP_BUILD_SHARED_BOOL FALSE)
    endif ()
    if (NOT JINJA2CPP_BUILD_SHARED OR BUILD_SHARED_LIBS OR NOT JINJA2CPP_DEPS_MODE STREQUAL "internal")
        set(JINJA2CPP_EXPORT_PRIVATE_DEPS TRUE)
    else ()
        set(JINJA2CPP_EXPORT_PRIVATE_DEPS FALSE)
    endif ()
    # The external modes record the components their Boost provides
    if (NOT DEFINED JINJA2CPP_BOOST_COMPONENTS)
        set(JINJA2CPP_BOOST_COMPONENTS algorithm any filesystem)
        if ("${JINJA2CPP_WITH_JSON_BINDINGS}" STREQUAL "boost")
            list(APPEND JINJA2CPP_BOOST_COMPONENTS json)
        endif ()
    endif ()
    string(REPLACE ";" " " JINJA2CPP_BOOST_COMPONENTS "${JINJA2CPP_BOOST_COMPONENTS}")

    configure_package_config_file(
        cmake/public/jinja2cpp-config.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/cmake/config/jinja2cpp-config.cmake
        INSTALL_DESTINATION ${JINJA2CPP_INSTALL_CONFIG_DIR}
    )
    configure_file(
        cmake/public/jinja2cpp-config-deps-${JINJA2CPP_DEPS_MODE}.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/cmake/config/jinja2cpp-config-deps.cmake
        @ONLY
    )
    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/cmake/config/jinja2cpp-config-version.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )

    install(
        FILES
            ${CMAKE_CURRENT_BINARY_DIR}/cmake/config/jinja2cpp-config.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/cmake/config/jinja2cpp-config-deps.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/cmake/config/jinja2cpp-config-version.cmake
        DESTINATION
            ${JINJA2CPP_INSTALL_CONFIG_DIR}
    )
endif()
