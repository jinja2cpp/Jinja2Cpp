cmake_minimum_required(VERSION 3.0)
project(Jinja2Cpp VERSION 0.1.0)

list (APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

if (NOT UNIX)
    set(GTEST_ROOT $ENV{GTEST_DIR} CACHE PATH "Path to GTest/GMock library root")
    set(BOOST_ROOT $ENV{BOOST_DIR} CACHE PATH "Path to boost library root")
    set(LIBRARY_TYPE STATIC CACHE PATH "Library link type")
else ()
    set(GTEST_ROOT "/usr" CACHE PATH "Path to GTest/GMock library root")
    set(BOOST_ROOT "/usr" CACHE PATH "Path to boost library root")
    set(LIBRARY_TYPE SHARED CACHE PATH "Library link type")
endif ()

if(NOT "${GTEST_ROOT}" STREQUAL "")
    list (APPEND CMAKE_PREFIX_PATH ${GTEST_ROOT})
    set (Gtest_DIR ${GTEST_ROOT})
    message(STATUS "GTest library search path: ${Gtest_DIR}")
endif()


if(NOT "${BOOST_ROOT}" STREQUAL "")
    list (APPEND CMAKE_PREFIX_PATH ${BOOST_ROOT})
    set (Boost_DIR ${BOOST_ROOT})
    message(STATUS "BOOST library search path: ${Boost_DIR}")
endif()

find_package(GTest)
find_package(Boost)

include(collect_sources)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${Boost_INCLUDE_DIRS}
    ${GTEST_INCLUDE_DIRS}
    )

set (EXTRA_TEST_LIBS "")
set (LIB_TARGET_NAME jinja2cpp)

if (${CMAKE_CXX_COMPILER_ID} MATCHES "Clang" OR ${CMAKE_CXX_COMPILER_ID} MATCHES "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

    if (NOT UNIX)
        set (EXTRA_TEST_LIBS "stdc++")
    endif ()
endif()

CollectSources(Sources Headers ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/src)
CollectSources(PublicSources PublicHeaders ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/include)
CollectSources(TestSources TestHeaders ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/test)

add_library(${LIB_TARGET_NAME} ${LIBRARY_TYPE}
    ${Sources}
    ${Headers}
    ${PublicHeaders}
)

target_include_directories(${LIB_TARGET_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(jinja2cpp_tests ${TestSources} ${TestHeaders})
target_link_libraries(jinja2cpp_tests ${GTEST_BOTH_LIBRARIES} ${LIB_TARGET_NAME} stdc++)

install(TARGETS ${LIB_TARGET_NAME}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib/static)

install (DIRECTORY include/ DESTINATION include)
install (DIRECTORY cmake/public DESTINATION cmake)

add_test(NAME jinja2cpp_tests COMMAND jinja2cpp_tests)
