cmake_minimum_required(VERSION 3.0.2)
project(Jinja2Cpp VERSION 0.9.1)

if (${CMAKE_VERSION} VERSION_GREATER "3.12")
    cmake_policy(SET CMP0074 OLD)
endif ()

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

if(CMAKE_COMPILER_IS_GNUCXX AND COVERAGE_ENABLED)
    message (STATUS "This is DEBUG build with enabled Code Coverage")
    set (CMAKE_BUILD_TYPE Debug)
    include(code_coverage)
    setup_target_for_coverage(build_coverage jinja2cpp_tests coverage)
endif()

if (${CMAKE_CXX_COMPILER_ID} MATCHES "Clang" OR ${CMAKE_CXX_COMPILER_ID} MATCHES "GNU")
    if (NOT UNIX)
        set (CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-Wa,-mbig-obj")
    endif ()
else ()
    set (COMMON_MSVC_OPTS "/wd4503 /bigobj")
    add_definitions(/DBOOST_ALL_NO_LIB)

    # MSVC
    if (CMAKE_BUILD_TYPE MATCHES "Debug" AND MSVC_RUNTIME_TYPE)
        set (MSVC_RUNTIME_TYPE "${MSVC_RUNTIME_TYPE}d")
    endif ()
    if (CMAKE_BUILD_TYPE MATCHES "Debug")
        set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${MSVC_RUNTIME_TYPE} ${COMMON_MSVC_OPTS}")
        set (Boost_USE_DEBUG_RUNTIME ON)
    else ()
        set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${MSVC_RUNTIME_TYPE}  ${COMMON_MSVC_OPTS}")
        set (CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} ${MSVC_RUNTIME_TYPE} ${COMMON_MSVC_OPTS}")
        set (CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "/PROFILE")
        set (Boost_USE_DEBUG_RUNTIME OFF)
    endif ()

endif()


if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")
    set(JINJA2CPP_IS_MAIN_PROJECT TRUE)
else()
    set(JINJA2CPP_IS_MAIN_PROJECT FALSE)
endif()

option(JINJA2CPP_BUILD_TESTS "Build Jinja2Cpp unit tests" ${JINJA2CPP_IS_MAIN_PROJECT})
option(JINJA2CPP_STRICT_WARNINGS "Enable additional warnings and treat them as errors" ON)
option(JINJA2CPP_BUILD_SHARED "Build shared linkage version of Jinja2Cpp" OFF)
if (JINJA2CPP_BUILD_SHARED)
    set(LIB_LINK_TYPE SHARED)
else()
    set(LIB_LINK_TYPE STATIC)
endif()

include(collect_sources)

set (LIB_TARGET_NAME jinja2cpp)

CollectSources(Sources Headers ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/src)
CollectSources(PublicSources PublicHeaders ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(${LIB_TARGET_NAME} ${LIB_LINK_TYPE}
    ${Sources}
    ${Headers}
    ${PublicHeaders}
)

string(TOUPPER "${CMAKE_BUILD_TYPE}" BUILD_CFG_NAME)
set(CURRENT_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_${BUILD_CFG_NAME}}")

add_subdirectory(thirdparty)

target_link_libraries(${LIB_TARGET_NAME} PUBLIC expected-lite variant-lite value_ptr-lite optional-lite boost_filesystem)

target_include_directories(${LIB_TARGET_NAME}
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(JINJA2CPP_STRICT_WARNINGS)
if(NOT MSVC)
    target_compile_options(${LIB_TARGET_NAME} PRIVATE -Wall -Werror)
else ()
    target_compile_options(${LIB_TARGET_NAME} PRIVATE /W4)
endif()
endif()
if (COVERAGE_ENABLED AND NOT MSVC)
    target_compile_options(${LIB_TARGET_NAME} PRIVATE -g PUBLIC -O0 --coverage -fprofile-arcs -ftest-coverage)
endif ()

target_compile_definitions(${LIB_TARGET_NAME} PUBLIC variant_CONFIG_SELECT_VARIANT=variant_VARIANT_NONSTD)
set_target_properties(${LIB_TARGET_NAME} PROPERTIES
    CXX_STANDARD 14
    CXX_STANDARD_REQUIRED ON)

if (JINJA2CPP_BUILD_TESTS)
    enable_testing()

    CollectSources(TestSources TestHeaders ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/test)
    add_executable(jinja2cpp_tests ${TestSources} ${TestHeaders})
    target_link_libraries(jinja2cpp_tests gtest gtest_main ${LIB_TARGET_NAME} ${EXTRA_TEST_LIBS})
    if (COVERAGE_ENABLED)
        target_link_libraries(jinja2cpp_tests gcov)
    endif ()

    get_target_property(TEST_CXX_STD jinja2cpp_tests CXX_STANDARD)

    string (FIND "${CURRENT_CXX_FLAGS}" "-std" TEST_FLAGS_STD_POS)
    string (FIND "${TEST_CXX_STD}" "NOTFOUND" TEST_CXX_STD_NOTFOUND_POS)

    if (NOT MSVC AND TEST_FLAGS_STD_POS EQUAL -1 AND NOT (TEST_CXX_STD_NOTFOUND_POS EQUAL -1))
        set_target_properties(jinja2cpp_tests PROPERTIES
            CXX_STANDARD 14
            CXX_STANDARD_REQUIRED ON)
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

    add_test(NAME jinja2cpp_tests COMMAND jinja2cpp_tests)
endif ()

install(TARGETS ${LIB_TARGET_NAME}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib/static)

install (DIRECTORY include/ DESTINATION include)
install (DIRECTORY thirdparty/nonstd/expected-lite/include/ DESTINATION include)
install (DIRECTORY thirdparty/nonstd/variant-lite/include/ DESTINATION include)
install (DIRECTORY thirdparty/nonstd/value-ptr-lite/include/ DESTINATION include)
install (DIRECTORY thirdparty/nonstd/optional-lite/include/ DESTINATION include)
install (FILES cmake/public/FindJinja2Cpp.cmake DESTINATION cmake)
