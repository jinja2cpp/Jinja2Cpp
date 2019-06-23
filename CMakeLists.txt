cmake_minimum_required(VERSION 3.0.1)
project(Jinja2Cpp VERSION 0.9.3)

if (${CMAKE_VERSION} VERSION_GREATER "3.12")
    cmake_policy(SET CMP0074 OLD)
endif ()

if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")
    set(JINJA2CPP_IS_MAIN_PROJECT TRUE)
else()
    set(JINJA2CPP_IS_MAIN_PROJECT FALSE)
endif()

set (JINJA2CPP_DEPS_MODE "internal" CACHE STRING "Jinja2Cpp dependency management mode (internal | external | external-boost | conan-build). See documentation for details. 'interal' is default.")
if (NOT JINJA2CPP_DEPS_MODE)
    set (JINJA2CPP_DEPS_MODE "internal")
endif ()

if (JINJA2CPP_IS_MAIN_PROJECT)
    set (JINJA2CPP_CXX_STANDARD 14 CACHE STRING "Jinja2Cpp C++ standard to build with. C++14 is default")
    if (NOT JINJA2CPP_CXX_STANDARD)
        set (JINJA2CPP_CXX_STANDARD 14)
    endif ()
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

include(thirdparty/CMakeLists.txt)

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
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    )
    
if (JINJA2CPP_IS_MAIN_PROJECT)
    set_target_properties(${LIB_TARGET_NAME} PROPERTIES
        CXX_STANDARD ${JINJA2CPP_CXX_STANDARD}
        CXX_STANDARD_REQUIRED ON
        )    
endif ()
    
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

    set_target_properties(jinja2cpp_tests PROPERTIES
        CXX_STANDARD ${JINJA2CPP_CXX_STANDARD}
        CXX_STANDARD_REQUIRED ON)

	if (MSVC)
	    target_compile_options(jinja2cpp_tests PRIVATE /bigobj)
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


macro (Jinja2CppGetTargetIncludeDir infix target)
    message (STATUS "infix: ${infix} target: ${target}")

    if (TARGET ${target})
        set (_J2CPP_VAR_NAME JINJA2CPP_${infix}_INCLUDE_DIRECTORIES)
        get_target_property(${_J2CPP_VAR_NAME} ${target} INTERFACE_INCLUDE_DIRECTORIES)
    endif ()
endmacro ()
  
Jinja2CppGetTargetIncludeDir(EXPECTED-LITE expected-lite)
Jinja2CppGetTargetIncludeDir(VARIANT-LITE variant-lite)
Jinja2CppGetTargetIncludeDir(OPTIONAL-LITE optional-lite)

# Workaround for #14444 bug of CMake (https://gitlab.kitware.com/cmake/cmake/issues/14444)
# We can't use EXPORT feature of 'install' as is due to limitation of subproject's targets installation
# So jinja2cpp-config.cmake should be written manually

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
