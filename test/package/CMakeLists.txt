# Consumer of the installed package (docs/tasks/0068), driven by check_package.cmake
cmake_minimum_required(VERSION 3.23)
project(jinja2cpp_package_consumer CXX)

find_package(jinja2cpp ${JINJA2CPP_EXPECT_VERSION} REQUIRED)

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/templates/hello.j2 "Hello {{ name }}!")

foreach (_target consumer consumer_compat)
    add_executable(${_target} main.cpp)
    target_compile_definitions(${_target} PRIVATE
        $<$<BOOL:${JINJA2CPP_EXPECT_SHARED}>:EXPECT_SHARED>)
endforeach ()
target_link_libraries(consumer PRIVATE jinja2cpp::jinja2cpp)
target_link_libraries(consumer_compat PRIVATE jinja2cpp)

enable_testing()
add_test(NAME consumer COMMAND consumer ${CMAKE_CURRENT_BINARY_DIR}/templates)
add_test(NAME consumer_compat COMMAND consumer_compat ${CMAKE_CURRENT_BINARY_DIR}/templates)

# check_package.cmake points PKG_CONFIG_PATH at the install and turns this on. pkg-config
# lists the private dependencies of a static build only in the internal deps mode, where
# it knows them.
option(JINJA2CPP_CHECK_PKGCONFIG "Also build a consumer from jinja2cpp.pc" OFF)
find_package(PkgConfig QUIET)
if (JINJA2CPP_CHECK_PKGCONFIG AND PKG_CONFIG_FOUND AND NOT WIN32 AND (JINJA2CPP_EXPECT_SHARED OR jinja2cpp_DEPS_MODE STREQUAL "internal"))
    pkg_check_modules(J2C REQUIRED jinja2cpp)
    if (NOT J2C_VERSION STREQUAL jinja2cpp_VERSION)
        message(FATAL_ERROR "pkg-config version ${J2C_VERSION} != CMake package version ${jinja2cpp_VERSION}")
    endif ()
    add_executable(consumer_pc main.cpp)
    target_compile_definitions(consumer_pc PRIVATE
        $<$<BOOL:${JINJA2CPP_EXPECT_SHARED}>:EXPECT_SHARED>)
    if (JINJA2CPP_EXPECT_SHARED)
        target_compile_options(consumer_pc PRIVATE ${J2C_CFLAGS})
        target_link_libraries(consumer_pc PRIVATE ${J2C_LDFLAGS})
        set_target_properties(consumer_pc PROPERTIES BUILD_RPATH "${J2C_LIBDIR}")
    else ()
        target_compile_options(consumer_pc PRIVATE ${J2C_STATIC_CFLAGS})
        target_link_libraries(consumer_pc PRIVATE ${J2C_STATIC_LDFLAGS})
    endif ()
    add_test(NAME consumer_pc COMMAND consumer_pc ${CMAKE_CURRENT_BINARY_DIR}/templates)
endif ()
