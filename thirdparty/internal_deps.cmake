include(FetchContent)

FetchContent_Declare(
    expected-lite
    URL https://github.com/nonstd-lite/expected-lite/archive/refs/tags/v0.10.0.tar.gz
    URL_HASH SHA256=cfe082e4ffedeeedac47763504102646a39c080599c7c1fe99299d6a1f99af92
)
FetchContent_MakeAvailable(expected-lite)

FetchContent_Declare(
    variant-lite
    URL https://github.com/nonstd-lite/variant-lite/archive/refs/tags/v3.0.0.tar.gz
    URL_HASH SHA256=bd596550369f33ef9455566822f5a4d52852a63a33d3d70ac1fbb529b78abc7b
)
FetchContent_MakeAvailable(variant-lite)

FetchContent_Declare(
    optional-lite
    URL https://github.com/nonstd-lite/optional-lite/archive/5f924cbfc130484d4820bb105d6ad1a42df930e0.tar.gz
    URL_HASH SHA256=4f270ebbd0d7a6011befb1191acf66f2be4bb6a6d141490bed1363fa7f543123
)
FetchContent_MakeAvailable(optional-lite)

FetchContent_Declare(
    string-view-lite
    URL https://github.com/nonstd-lite/string-view-lite/archive/52dced64cd054c5deea282168a50f39ad77475dd.tar.gz
    URL_HASH SHA256=1151763852c4e2912950c53f2c9645c456d3deada02ff6979cf480db4e630c4f
)
FetchContent_MakeAvailable(string-view-lite)

set (FMT_INSTALL ON CACHE BOOL "" FORCE)
FetchContent_Declare(
    fmt
    URL https://github.com/fmtlib/fmt/archive/refs/tags/12.1.0.tar.gz
    URL_HASH SHA256=ea7de4299689e12b6dddd392f9896f08fb0777ac7168897a244a6d6085043fea
)
FetchContent_MakeAvailable(fmt)

if("${JINJA2CPP_WITH_JSON_BINDINGS}" STREQUAL "rapid")

set (RAPIDJSON_BUILD_DOC OFF CACHE BOOL "" FORCE)
set (RAPIDJSON_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set (RAPIDJSON_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set (RAPIDJSON_BUILD_THIRDPARTY_GTEST OFF CACHE BOOL "" FORCE)
set (RAPIDJSON_ENABLE_INSTRUMENTATION_OPT OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    rapidjson
    URL https://github.com/Tencent/rapidjson/archive/24b5e7a8b27f42fa16b96fc70aade9106cf7102f.tar.gz
    URL_HASH SHA256=2d2601a82d2d3b7e143a3c8d43ef616671391034bc46891a9816b79cf2d3e7a8
    PATCH_COMMAND echo "!!!" &&
        patch -p1 < ${CMAKE_CURRENT_SOURCE_DIR}/cmake/patches/rapid_json_improvements.patch &&
        patch -p1 < ${CMAKE_CURRENT_SOURCE_DIR}/cmake/patches/0001-fix-custom_command-error.patch
)
FetchContent_MakeAvailable(rapidjson)

add_library(RapidJson INTERFACE)
target_link_libraries(RapidJson INTERFACE RapidJSON)
target_include_directories(RapidJson
    INTERFACE
        $<BUILD_INTERFACE:${RapidJSON_INCLUDE_DIR}>
        $<INSTALL_INTERFACE:include>
)

endif()

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
endif()

install (FILES
        thirdparty/nonstd/expected-lite/include/nonstd/expected.hpp
        thirdparty/nonstd/variant-lite/include/nonstd/variant.hpp
        thirdparty/nonstd/optional-lite/include/nonstd/optional.hpp
        thirdparty/nonstd/string-view-lite/include/nonstd/string_view.hpp
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/nonstd)

if("${JINJA2CPP_WITH_JSON_BINDINGS}" STREQUAL "rapid")

install (TARGETS RapidJson
    EXPORT InstallTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/static
)

endif()
