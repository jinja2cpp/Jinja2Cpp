include(FetchContent)

FetchContent_Declare(
    expected-lite
    URL https://github.com/nonstd-lite/expected-lite/archive/refs/tags/v0.10.0.tar.gz
    URL_HASH SHA256=cfe082e4ffedeeedac47763504102646a39c080599c7c1fe99299d6a1f99af92
)
FetchContent_MakeAvailable(expected-lite)
jinja2cpp_mark_system("${expected-lite_SOURCE_DIR}")

set (FMT_INSTALL ON CACHE BOOL "" FORCE)
# fmt >= 12.2 builds a C++20 module library by default with Ninja and clang >= 16 /
# gcc >= 15, and no longer opts its plain target out of module scanning, so the build
# would need clang-scan-deps. Nothing here consumes modules.
set (FMT_MODULE OFF CACHE BOOL "" FORCE)
set (CMAKE_CXX_SCAN_FOR_MODULES OFF)
FetchContent_Declare(
    fmt
    URL https://github.com/fmtlib/fmt/archive/refs/tags/12.2.0.tar.gz
    URL_HASH SHA256=8b852bb5aa6e7d8564f9e81394055395dd1d1936d38dfd3a17792a02bebd7af0
)
FetchContent_MakeAvailable(fmt)
set(JINJA2CPP_FMT_VERSION 12.2.0)
jinja2cpp_mark_system("${fmt_SOURCE_DIR}")

# The JSON libraries are for the tests only: the library reads {% meta %} itself (0230 D6)
if (JINJA2CPP_BUILD_TESTS)
    set (JSON_BuildTests OFF CACHE BOOL "" FORCE)
    set (JSON_Install OFF CACHE BOOL "" FORCE)
    set (JSON_MultipleHeaders ON CACHE BOOL "" FORCE)

    FetchContent_Declare(
        nlohmann_json
        URL https://github.com/nlohmann/json/archive/refs/tags/v3.12.0.tar.gz
        URL_HASH SHA256=4b92eb0c06d10683f7447ce9406cb97cd4b453be18d7279320f7b2f025c10187
    )
    FetchContent_MakeAvailable(nlohmann_json)
jinja2cpp_mark_system("${nlohmann_json_SOURCE_DIR}")
endif()
