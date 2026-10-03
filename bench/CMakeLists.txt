# Google Benchmark suite, see bench/README.md and docs/tasks/0011
add_executable(jinja2cpp_bench jinja2cpp_bench.cpp)
target_link_libraries(jinja2cpp_bench
    benchmark::benchmark nlohmann_json::nlohmann_json ${LIB_TARGET_NAME})
set_target_properties(jinja2cpp_bench PROPERTIES
    CXX_STANDARD ${JINJA2CPP_CXX_STANDARD}
    CXX_STANDARD_REQUIRED ON)
target_compile_definitions(jinja2cpp_bench
    PRIVATE JINJA2CPP_BENCH_CASES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/cases")
