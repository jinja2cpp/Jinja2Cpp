cmake_minimum_required(VERSION 3.0)

project (thirdparty)
list (APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../cmake)

include (build_thirdparty)

if (${CMAKE_CXX_COMPILER_ID} STREQUAL "MSVC")
    message (STATUS "######### <<<<<<<<< !!!!!!!!!!!!")
    if (MSVC_RUNTIME_TYPE STREQUAL "/MD" OR NOT MSVC_RUNTIME_TYPE)
        set (GTEST_EXTRA_OPTIONS "-Dgtest_force_shared_crt=TRUE")
    else ()
        set (GTEST_EXTRA_OPTIONS "-Dgtest_force_shared_crt=TRUE")
    endif ()
else ()
    message (STATUS "######### <<<<<<<<< ????????")
endif ()

message (STATUS "#### CMAKE_CXX_COMPILER_ID: '${CMAKE_CXX_COMPILER_ID}'")
message (STATUS "#### GTEST_EXTRA_OPTIONS: ${GTEST_EXTRA_OPTIONS}")

BuildThirdparty(gtest ${CMAKE_CURRENT_SOURCE_DIR}/gtest include/gmock/gmock.h "${GTEST_EXTRA_OPTIONS}")
