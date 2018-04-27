cmake_minimum_required(VERSION 3.0)
project(Jinja2Cpp VERSION 0.1.0)

list (APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

message (STATUS ">>> Build type: ${CMAKE_BUILD_TYPE}")

set(GTEST_ROOT $ENV{GTEST_DIR} CACHE PATH "Path to GTest/GMock library root")
if (NOT UNIX)
    set(BOOST_ROOT $ENV{BOOST_DIR} CACHE PATH "Path to boost library root")
    set(LIBRARY_TYPE STATIC CACHE PATH "Library link type")
else ()
    set(BOOST_ROOT "/usr" CACHE PATH "Path to boost library root")
    set(LIBRARY_TYPE SHARED CACHE PATH "Library link type")
endif ()

if (NOT DEFINED WITH_TESTS)
    set (WITH_TESTS TRUE)
endif ()

set (EXTRA_TEST_LIBS "")
if (${CMAKE_CXX_COMPILER_ID} MATCHES "Clang" OR ${CMAKE_CXX_COMPILER_ID} MATCHES "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

    if (NOT UNIX)
        set (EXTRA_TEST_LIBS "stdc++")
    else ()
        set (EXTRA_TEST_LIBS "pthread")
    endif ()
endif()

if("${GTEST_ROOT}" STREQUAL "" AND WITH_TESTS)
    set (THIRDPARTY_TARGETS ${THIRDPARTY_TARGETS} gtest)
endif()

if(NOT "${BOOST_ROOT}" STREQUAL "")
    list (APPEND CMAKE_PREFIX_PATH ${BOOST_ROOT})
    set (Boost_DIR ${BOOST_ROOT})
    message(STATUS "BOOST library search path: ${Boost_DIR}")
endif()

message (STATUS "Thirdparty to build: ${THIRDPARTY_TARGETS}")
message (STATUS ">>> Thirdparty build flags: ${CMAKE_CXX_FLAGS}")
make_directory (${CMAKE_CURRENT_BINARY_DIR}/thirdparty)
execute_process (
    COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER} -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER} -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS} ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/thirdparty
    RESULT_VARIABLE THIRDPARTY_BUILD_RESULT
    )

if (THIRDPARTY_BUILD_RESULT EQUAL 0 AND THIRDPARTY_TARGETS)
    execute_process (
        COMMAND ${CMAKE_COMMAND} --build . --target ${THIRDPARTY_TARGETS}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/thirdparty
        RESULT_VARIABLE THIRDPARTY_BUILD_RESULT
        )
endif ()

if (NOT THIRDPARTY_BUILD_RESULT EQUAL 0)
    message (FATAL_ERROR "Can't build thirdparty libraries")
endif ()

if("${GTEST_ROOT}" STREQUAL "" AND WITH_TESTS)
    set (GTEST_ROOT ${CMAKE_CURRENT_BINARY_DIR}/thirdparty/gtest/install)
endif ()

message (STATUS "### GTEST_ROOT = ${GTEST_ROOT}")

if(NOT "${GTEST_ROOT}" STREQUAL "" AND WITH_TESTS)
    list (APPEND CMAKE_PREFIX_PATH ${GTEST_ROOT})
    set (Gtest_DIR ${GTEST_ROOT})
    message(STATUS "GTest library search path: ${Gtest_DIR}")
    find_package(GTest)
    if (MSVC AND NOT GTEST_INCLUDE_DIRS)
        set (GTEST_MSVC_SEARCH "MT")
        find_package(GTest)
    endif ()
endif()
message (STATUS "### GTEST_DIR = ${Gtest_DIR}")
message (STATUS "### GTEST_INCLUDE_DIRS = ${GTEST_INCLUDE_DIRS}")
message (STATUS "### GTEST_BOTH_LIBRARIES = ${GTEST_BOTH_LIBRARIES}")
message (STATUS "### GTEST_LIBRARIES = ${GTEST_LIBRARIES}")
message (STATUS "### GTEST_MAIN_LIBRARIES = ${GTEST_MAIN_LIBRARIES}")


find_package(Boost)

include(collect_sources)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${Boost_INCLUDE_DIRS}
    ${GTEST_INCLUDE_DIRS}
    )

set (LIB_TARGET_NAME jinja2cpp)

CollectSources(Sources Headers ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/src)
CollectSources(PublicSources PublicHeaders ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(${LIB_TARGET_NAME} ${LIBRARY_TYPE}
    ${Sources}
    ${Headers}
    ${PublicHeaders}
)

target_include_directories(${LIB_TARGET_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

if (WITH_TESTS)
    CollectSources(TestSources TestHeaders ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/test)
    add_executable(jinja2cpp_tests ${TestSources} ${TestHeaders})
    target_link_libraries(jinja2cpp_tests ${GTEST_BOTH_LIBRARIES} ${LIB_TARGET_NAME} ${EXTRA_TEST_LIBS})
endif ()

install(TARGETS ${LIB_TARGET_NAME}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib/static)

install (DIRECTORY include/ DESTINATION include)
install (DIRECTORY cmake/public DESTINATION cmake)

add_test(NAME jinja2cpp_tests COMMAND jinja2cpp_tests)
