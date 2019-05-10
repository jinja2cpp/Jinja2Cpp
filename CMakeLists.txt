cmake_minimum_required(VERSION 3.5)
project(Jinja2Cpp VERSION 0.9.1)

if (${CMAKE_VERSION} VERSION_GREATER "3.12")
    cmake_policy(SET CMP0074 OLD)
endif ()

if (NOT DEFINED JINJA2CPP_DEPS_MODE)
    set (JINJA2CPP_DEPS_MODE "internal")
endif ()

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

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
option(JINJA2CPP_USE_CONANPKG_PREFIX "Use 'CONAN_PKG' prefix for conan targets" ${JINJA2CPP_USE_CONANPKG_PREFIX})
# option(JINJA2CPP_DEPS_MODE "Jinja2Cpp dependencies handling mode" "internal")

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

include(thirdparty/CMakeLists.txt)

message (STATUS ">>>>>>> JINJA2CPP_PUBLIC_LIBS = ${JINJA2CPP_PUBLIC_LIBS}")
message (STATUS ">>>>>>> JINJA2CPP_PRIVATE_LIBS = ${JINJA2CPP_PRIVATE_LIBS}")

target_link_libraries(${LIB_TARGET_NAME} PUBLIC ${JINJA2CPP_PUBLIC_LIBS} PRIVATE ${JINJA2CPP_PRIVATE_LIBS})

target_include_directories(${LIB_TARGET_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>  
        $<INSTALL_INTERFACE:include>)

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
    CXX_STANDARD_REQUIRED ON
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    )
    
set_property(TARGET ${LIB_TARGET_NAME} PROPERTY PUBLIC_HEADER ${PublicHeaders} ${JINJA2CPP_EXTRA_PUBLIC_HEADERS})
    
configure_file(jinja2cpp.pc.in jinja2cpp.pc @ONLY)

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

set (JINJA2CPP_INSTALL_CONFIG_DIR ${CMAKE_INSTALL_LIBDIR}/${LIB_TARGET_NAME})
set (JINJA2CPP_TMP_CONFIG_PATH cmake/config)

  
get_target_property(JINJA2CPP_EXPECTED-LITE_INCLUDE_DIRECTORIES expected-lite INTERFACE_INCLUDE_DIRECTORIES)
get_target_property(JINJA2CPP_VARIANT-LITE_INCLUDE_DIRECTORIES variant-lite INTERFACE_INCLUDE_DIRECTORIES)
get_target_property(JINJA2CPP_OPTIONAL-LITE_INCLUDE_DIRECTORIES optional-lite INTERFACE_INCLUDE_DIRECTORIES)
get_target_property(JINJA2CPP_VALUE-PTR-LITE_INCLUDE_DIRECTORIES value-ptr-lite INTERFACE_INCLUDE_DIRECTORIES)

message(STATUS "expected-lite includes: ${JINJA2CPP_EXPECTED-LITE_INCLUDE_DIRECTORIES}")
message(STATUS "variant-lite includes: ${JINJA2CPP_VARIANT-LITE_INCLUDE_DIRECTORIES}")
message(STATUS "optional-lite includes: ${JINJA2CPP_OPTIONAL-LITE_INCLUDE_DIRECTORIES}")
message(STATUS "value-ptr-lite includes: ${JINJA2CPP_VALUE-PTR-LITE_INCLUDE_DIRECTORIES}")

install(TARGETS ${LIB_TARGET_NAME}
        EXPORT InstallTargets
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/static
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jinja2cpp
        )
        
install (FILES ${CMAKE_BINARY_DIR}/jinja2cpp.pc
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)

install(EXPORT InstallTargets
    FILE jinja2cpp-cfg.cmake
    DESTINATION ${JINJA2CPP_INSTALL_CONFIG_DIR})
    
configure_package_config_file(
    cmake/public/jinja2cpp-config.cmake.in
    ${JINJA2CPP_TMP_CONFIG_PATH}/jinja2cpp-config.cmake
    INSTALL_DESTINATION ${JINJA2CPP_TMP_CONFIG_PATH}
    NO_CHECK_REQUIRED_COMPONENTS_MACRO
  )   

configure_package_config_file(
    cmake/public/jinja2cpp-config-deps-${JINJA2CPP_DEPS_MODE}.cmake.in
    ${JINJA2CPP_TMP_CONFIG_PATH}/jinja2cpp-config-deps.cmake
    INSTALL_DESTINATION ${JINJA2CPP_TMP_CONFIG_PATH}
    NO_CHECK_REQUIRED_COMPONENTS_MACRO
  )

install (FILES
        ${CMAKE_CURRENT_BINARY_DIR}/${JINJA2CPP_TMP_CONFIG_PATH}/${LIB_TARGET_NAME}-config.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/${JINJA2CPP_TMP_CONFIG_PATH}/${LIB_TARGET_NAME}-config-deps.cmake
    DESTINATION ${JINJA2CPP_INSTALL_CONFIG_DIR})
