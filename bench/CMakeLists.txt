# Google Benchmark suite, see bench/README.md and docs/tasks/0011
add_executable(jinja2cpp_bench jinja2cpp_bench.cpp)
target_link_libraries(jinja2cpp_bench
    benchmark::benchmark nlohmann_json::nlohmann_json ${LIB_TARGET_NAME})
set_target_properties(jinja2cpp_bench PROPERTIES
    CXX_STANDARD ${JINJA2CPP_CXX_STANDARD}
    CXX_STANDARD_REQUIRED ON)
target_compile_definitions(jinja2cpp_bench
    PRIVATE JINJA2CPP_BENCH_CASES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/cases")

# gperftools CPU and heap profiles of one benchmark (--count with --cpu-profile/--heap-profile)
option(JINJA2CPP_BENCH_WITH_GPERFTOOLS "Link jinja2cpp_bench with gperftools (libprofiler, tcmalloc)" OFF)
if (JINJA2CPP_BENCH_WITH_GPERFTOOLS)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GPERFTOOLS REQUIRED IMPORTED_TARGET libprofiler libtcmalloc)
    target_link_libraries(jinja2cpp_bench PkgConfig::GPERFTOOLS)
    target_compile_definitions(jinja2cpp_bench PRIVATE JINJA2CPP_BENCH_GPERFTOOLS)
endif ()

# Other C++ engines on the same cases (docs/tasks/0120): fetched only when enabled
option(JINJA2CPP_BENCH_WITH_OTHER_ENGINES "Build engines_bench: inja and minja on the bench cases (fetches both)" OFF)
if (JINJA2CPP_BENCH_WITH_OTHER_ENGINES)
    include(FetchContent)
    # Header-only; SOURCE_SUBDIR points nowhere so their own CMake projects (tests, installs)
    # stay out of this build
    FetchContent_Declare(inja
        GIT_REPOSITORY https://github.com/pantor/inja.git
        GIT_TAG 7d1b4600b68595085a949743331c2e5673f511ea # v3.5.0
        SOURCE_SUBDIR do-not-configure)
    # ochafik/minja: the maintained fork of google/minja, by its author
    FetchContent_Declare(minja
        GIT_REPOSITORY https://github.com/ochafik/minja.git
        GIT_TAG 143465ab2f924f7729a8ca5313a12fb83a106d6d # 2026-05-09
        SOURCE_SUBDIR do-not-configure)
    FetchContent_MakeAvailable(inja minja)

    add_executable(engines_bench engines_bench.cpp)
    target_include_directories(engines_bench SYSTEM PRIVATE ${inja_SOURCE_DIR}/include ${minja_SOURCE_DIR}/include)
    target_link_libraries(engines_bench benchmark::benchmark nlohmann_json::nlohmann_json)
    set_target_properties(engines_bench PROPERTIES
        CXX_STANDARD ${JINJA2CPP_CXX_STANDARD}
        CXX_STANDARD_REQUIRED ON)
    # inja stays on nlohmann::json (sorted keys): with INJA_DATA_TYPE=nlohmann::ordered_json
    # v3.5.0 crashes in `for i in range(n)`
    target_compile_definitions(engines_bench
        PRIVATE JINJA2CPP_BENCH_CASES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/cases")
endif ()

# Rust engines on the same cases (docs/tasks/0135): MiniJinja and Tera, built by cargo from
# bench/rust (versions pinned in its Cargo.lock), only when enabled
option(JINJA2CPP_BENCH_WITH_RUST_ENGINES "Build rust_engines_bench: MiniJinja and Tera on the bench cases (needs cargo)" OFF)
if (JINJA2CPP_BENCH_WITH_RUST_ENGINES)
    find_program(CARGO_EXECUTABLE cargo REQUIRED)
    # cargo tracks its own sources, so the target always runs it and it rebuilds what changed.
    # The binary: <build>/bench/rust/release/rust_engines_bench
    add_custom_target(rust_engines_bench ALL
        COMMAND ${CARGO_EXECUTABLE} build --release --locked
            --manifest-path ${CMAKE_CURRENT_SOURCE_DIR}/rust/Cargo.toml
            --target-dir ${CMAKE_CURRENT_BINARY_DIR}/rust
        USES_TERMINAL
        COMMENT "Building rust_engines_bench with cargo")
endif ()
