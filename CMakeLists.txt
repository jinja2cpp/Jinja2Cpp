cmake_minimum_required(VERSION 3.0.2)
project(Jinja2Cpp VERSION 0.9.1)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

if (${CMAKE_CXX_COMPILER_ID} MATCHES "Clang" OR ${CMAKE_CXX_COMPILER_ID} MATCHES "GNU")
    if (NOT UNIX)
        set (CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-Wa,-mbig-obj")
    endif ()
else ()
    set (COMMON_MSVC_OPTS "/wd4503 /DBOOST_ALL_NO_LIB")

    # MSVC
    if (NOT DEFINED MSVC_RUNTIME_TYPE)
        set (MSVC_RUNTIME_TYPE "/MD")
        set (gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    endif ()
    if (CMAKE_BUILD_TYPE MATCHES "Debug")
        set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${MSVC_RUNTIME_TYPE}d ${COMMON_MSVC_OPTS}")
        set (Boost_USE_DEBUG_RUNTIME ON)
    else ()
        set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${MSVC_RUNTIME_TYPE}  ${COMMON_MSVC_OPTS}")
        set (CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} ${MSVC_RUNTIME_TYPE} ${COMMON_MSVC_OPTS}")
        set (CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "/PROFILE")
        set (Boost_USE_DEBUG_RUNTIME OFF)
    endif ()

endif()


if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")
    set(JINJA2CPP_IS_MAIN_PROEJCT TRUE)
else()
    set(JINJA2CPP_IS_MAIN_PROEJCT FALSE)
endif()

option(JINJA2CPP_BUILD_TESTS "Build Jinja2Cpp unit tests" ${JINJA2CPP_IS_MAIN_PROEJCT})

include(collect_sources)

set (LIB_TARGET_NAME jinja2cpp)

CollectSources(Sources Headers ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/src)
CollectSources(PublicSources PublicHeaders ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(${LIB_TARGET_NAME} STATIC
    ${Sources}
    ${Headers}
    ${PublicHeaders}
    )

add_subdirectory(thirdparty)

target_link_libraries(${LIB_TARGET_NAME} PUBLIC expected-lite variant-lite value-ptr-lite optional-lite boost_variant boost_filesystem boost_algorithm)

target_include_directories(${LIB_TARGET_NAME}
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)


if(NOT MSVC)
    target_compile_options(${LIB_TARGET_NAME} PRIVATE -Wall -Werror)
else ()
    target_compile_options(${LIB_TARGET_NAME} PRIVATE /W4)
endif()

target_compile_definitions(${LIB_TARGET_NAME} PUBLIC variant_CONFIG_SELECT_VARIANT=variant_VARIANT_NONSTD)
set_target_properties(${LIB_TARGET_NAME} PROPERTIES
    CXX_STANDARD 14
    CXX_STANDARD_REQUIRED ON)

if (JINJA2CPP_BUILD_TESTS)
    enable_testing()

    CollectSources(TestSources TestHeaders ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/test)
    add_executable(jinja2cpp_tests ${TestSources} ${TestHeaders})
    target_link_libraries(jinja2cpp_tests gtest gtest_main ${LIB_TARGET_NAME} ${EXTRA_TEST_LIBS})

    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/test_data/simple_template1.j2tpl
        COMMAND ${CMAKE_COMMAND} ARGS -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/test/test_data ${CMAKE_CURRENT_BINARY_DIR}/test_data
        MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/test/test_data/simple_template1.j2tpl
        COMMENT "Copy test data to the destination dir"
        )

    add_custom_target(CopyTestData ALL
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/test_data/simple_template1.j2tpl
        )

    add_test(NAME jinja2cpp_tests COMMAND jinja2cpp_tests)
endif ()

install(TARGETS ${LIB_TARGET_NAME}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib/static)

install (DIRECTORY include/ DESTINATION include)
install (DIRECTORY thirdparty/nonstd/expected-light/include/ DESTINATION include)
install (DIRECTORY thirdparty/nonstd/variant-light/include/ DESTINATION include)
install (DIRECTORY thirdparty/nonstd/value-ptr-lite/include/ DESTINATION include)
install (FILES cmake/public/FindJinja2Cpp.cmake DESTINATION cmake)
