cmake_minimum_required(VERSION 3.0.2)
project(Jinja2Cpp VERSION 0.5.0)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
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

set(BOOST_CMAKE_LIBRARIES filesystem algorithm variant optional CACHE INTERNAL "")
add_subdirectory(thirdparty/boost EXCLUDE_FROM_ALL)
add_subdirectory(thirdparty/nonstd EXCLUDE_FROM_ALL)

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

target_link_libraries(${LIB_TARGET_NAME} PUBLIC ThirdParty::nonstd boost_variant boost_filesystem boost_algorithm)

target_include_directories(${LIB_TARGET_NAME}
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)


if(NOT MSVC)
    # Enable -Werror and -Wall on jinja2cpp target, ignoring warning errors from thirdparty libs
    include(CheckCXXCompilerFlag)
    check_cxx_compiler_flag(-Wno-error=parentheses COMPILER_HAS_WNO_ERROR_PARENTHESES_FLAG)
    check_cxx_compiler_flag(-Wno-error=deprecated-declarations COMPILER_HAS_WNO_ERROR_DEPRECATED_DECLARATIONS_FLAG)
    check_cxx_compiler_flag(-Wno-error=maybe-uninitialized COMPILER_HAS_WNO_ERROR_MAYBE_UNINITIALIZED_FLAG)

    if(COMPILER_HAS_WNO_ERROR_PARENTHESES_FLAG)
        target_compile_options(boost_assert INTERFACE -Wno-error=parentheses)
    endif()
    if(COMPILER_HAS_WNO_ERROR_DEPRECATED_DECLARATIONS_FLAG)
        target_compile_options(boost_filesystem PRIVATE -Wno-error=deprecated-declarations)
    endif()
    if(COMPILER_HAS_WNO_ERROR_MAYBE_UNINITIALIZED_FLAG)
        target_compile_options(boost_variant INTERFACE -Wno-error=maybe-uninitialized)
    endif()

    target_compile_options(${LIB_TARGET_NAME} PRIVATE -Wall -Werror)
endif()

if (JINJA2CPP_BUILD_TESTS)
    enable_testing()
    add_subdirectory(thirdparty/gtest)

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
