# Fuzzing (docs/tasks/0003, fuzz/README.md).
#
# jinja2cpp_fuzz_replay is built with the tests under every compiler and runs the
# regression inputs in fuzz/regressions/ through all fuzz targets (ctest jinja2cpp_fuzz_regressions).
# With JINJA2CPP_BUILD_FUZZERS=ON (clang only) the libFuzzer binaries are built as well;
# combine it with JINJA2CPP_WITH_SANITIZERS=address+undefined.

set(_fuzz_dir ${CMAKE_CURRENT_SOURCE_DIR})

add_library(jinja2cpp_fuzz_targets STATIC ${_fuzz_dir}/fuzz_targets.cpp ${_fuzz_dir}/fuzz_common.h)
target_link_libraries(jinja2cpp_fuzz_targets PUBLIC ${LIB_TARGET_NAME})
set_target_properties(jinja2cpp_fuzz_targets PROPERTIES
    CXX_STANDARD ${JINJA2CPP_CXX_STANDARD}
    CXX_STANDARD_REQUIRED ON)

if (JINJA2CPP_BUILD_TESTS)
    add_executable(jinja2cpp_fuzz_replay ${_fuzz_dir}/replay_main.cpp)
    target_link_libraries(jinja2cpp_fuzz_replay jinja2cpp_fuzz_targets)
    # Next to jinja2cpp_tests and the shared library, which Windows looks up beside the exe
    set_target_properties(jinja2cpp_fuzz_replay PROPERTIES
        CXX_STANDARD ${JINJA2CPP_CXX_STANDARD}
        CXX_STANDARD_REQUIRED ON
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
    if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
        target_link_options(jinja2cpp_fuzz_replay PRIVATE /STACK:8388608)
    endif ()
    add_test(NAME jinja2cpp_fuzz_regressions COMMAND jinja2cpp_fuzz_replay ${_fuzz_dir}/regressions)
endif ()

if (JINJA2CPP_BUILD_FUZZERS)
    if (NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        message(FATAL_ERROR "JINJA2CPP_BUILD_FUZZERS needs clang (libFuzzer), not ${CMAKE_CXX_COMPILER_ID}")
    endif ()
    # Coverage instrumentation for everything the fuzzers reach; only the binaries link the
    # libFuzzer runtime (it brings its own main).
    target_compile_options(${LIB_TARGET_NAME} PRIVATE -fsanitize=fuzzer-no-link)
    target_compile_options(jinja2cpp_fuzz_targets PRIVATE -fsanitize=fuzzer-no-link)
    foreach (_target parse render render_wide)
        add_executable(jinja2cpp_fuzz_${_target} ${_fuzz_dir}/fuzz_${_target}.cpp)
        target_link_libraries(jinja2cpp_fuzz_${_target} jinja2cpp_fuzz_targets)
        target_compile_options(jinja2cpp_fuzz_${_target} PRIVATE -fsanitize=fuzzer)
        target_link_options(jinja2cpp_fuzz_${_target} PRIVATE -fsanitize=fuzzer)
        set_target_properties(jinja2cpp_fuzz_${_target} PROPERTIES
            CXX_STANDARD ${JINJA2CPP_CXX_STANDARD}
            CXX_STANDARD_REQUIRED ON)
    endforeach ()
endif ()
