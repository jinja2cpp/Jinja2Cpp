# Consumer of the installed package (docs/tasks/0068), driven by check_package.cmake
cmake_minimum_required(VERSION 3.23)
project(jinja2cpp_package_consumer CXX)

find_package(jinja2cpp ${JINJA2CPP_EXPECT_VERSION} REQUIRED)
# The built-in big-integer backend the library was configured with (docs/tasks/0272)
if (NOT jinja2cpp_BIGINT STREQUAL JINJA2CPP_EXPECT_BIGINT)
    message(FATAL_ERROR "jinja2cpp_BIGINT is '${jinja2cpp_BIGINT}', the build used '${JINJA2CPP_EXPECT_BIGINT}'")
endif ()

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/templates/hello.j2 "Hello {{ name }}!")

foreach (_target consumer consumer_compat)
    add_executable(${_target} main.cpp)
    target_compile_definitions(${_target} PRIVATE
        $<$<BOOL:${JINJA2CPP_EXPECT_SHARED}>:EXPECT_SHARED>
        $<$<STREQUAL:${jinja2cpp_BIGINT},boost>:EXPECT_BIGINT_BOOST>)
endforeach ()
target_link_libraries(consumer PRIVATE jinja2cpp::jinja2cpp)
target_link_libraries(consumer_compat PRIVATE jinja2cpp)

enable_testing()
add_test(NAME consumer COMMAND consumer ${CMAKE_CURRENT_BINARY_DIR}/templates)
add_test(NAME consumer_compat COMMAND consumer_compat ${CMAKE_CURRENT_BINARY_DIR}/templates)

# A program built against 1.x headers must fail to link with this library, not misbehave:
# the inline namespace v2 is in every mangled name (docs/tasks/0076). abi_probe_v2 is the
# positive control; abi_probe_v1 is built only by the test, which expects the link error.
add_executable(abi_probe_v2 abi_probe.cpp)
target_compile_definitions(abi_probe_v2 PRIVATE PROBE_V2)
target_link_libraries(abi_probe_v2 PRIVATE jinja2cpp::jinja2cpp)
add_test(NAME abi_probe_v2 COMMAND abi_probe_v2)
add_executable(abi_probe_v1 EXCLUDE_FROM_ALL abi_probe.cpp)
target_link_libraries(abi_probe_v1 PRIVATE jinja2cpp::jinja2cpp)
add_test(NAME abi_probe_v1_does_not_link
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target abi_probe_v1 --config $<CONFIG>)
# GNU ld, lld, Apple ld and MSVC link; a compile error or a successful link does not match
set_tests_properties(abi_probe_v1_does_not_link PROPERTIES PASS_REGULAR_EXPRESSION
    "undefined reference to [`']?jinja2::operator==|undefined symbol: jinja2::operator==|Undefined symbols for architecture|unresolved external symbol")

# check_package.cmake points PKG_CONFIG_PATH at the install and turns this on. pkg-config
# lists the private dependencies of a static build only in the internal deps mode, where
# it knows them (external-boost installs the internal mode's pkg-config file).
option(JINJA2CPP_CHECK_PKGCONFIG "Also build a consumer from jinja2cpp.pc" OFF)
find_package(PkgConfig QUIET)
if (JINJA2CPP_CHECK_PKGCONFIG AND PKG_CONFIG_FOUND AND NOT WIN32 AND (JINJA2CPP_EXPECT_SHARED OR jinja2cpp_DEPS_MODE MATCHES "^(internal|external-boost)$"))
    pkg_check_modules(J2C REQUIRED jinja2cpp)
    if (NOT J2C_VERSION STREQUAL jinja2cpp_VERSION)
        message(FATAL_ERROR "pkg-config version ${J2C_VERSION} != CMake package version ${jinja2cpp_VERSION}")
    endif ()
    pkg_get_variable(J2C_BIGINT jinja2cpp bigint)
    if (NOT J2C_BIGINT STREQUAL jinja2cpp_BIGINT)
        message(FATAL_ERROR "pkg-config bigint '${J2C_BIGINT}' != CMake package '${jinja2cpp_BIGINT}'")
    endif ()
    add_executable(consumer_pc main.cpp)
    target_compile_definitions(consumer_pc PRIVATE
        $<$<BOOL:${JINJA2CPP_EXPECT_SHARED}>:EXPECT_SHARED>
        $<$<STREQUAL:${J2C_BIGINT},boost>:EXPECT_BIGINT_BOOST>)
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
