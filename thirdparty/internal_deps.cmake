include(FetchContent)

FetchContent_Declare(
    expected-lite
    URL https://github.com/martinmoene/expected-lite/archive/f17940fabae07063cabb67abf2c8d164d3146044.tar.gz
    URL_HASH SHA256=b7fdfda5603a77eb1b3808d0df91e8e232160e33b3e2fe7cfdfcba644b7d602d
)
FetchContent_MakeAvailable(expected-lite)

FetchContent_Declare(
    variant-lite
    URL https://github.com/martinmoene/variant-lite/archive/7e3bce818adb69bab27ccb982ea1b6779c2e379e.tar.gz
    URL_HASH SHA256=5e4f3dfa9d24baf31457aa12540ebdec31b4cfc377e35d0a1b10554b17839a37
)
FetchContent_MakeAvailable(variant-lite)

FetchContent_Declare(
    optional-lite
    URL https://github.com/martinmoene/optional-lite/archive/44ae889d969117c05d84c96f34e20f9e1b5a1511.tar.gz
    URL_HASH SHA256=cccf7e559e6469054184391157d41c556458d3144b318bed3bded5dd54e032b8
)
FetchContent_MakeAvailable(optional-lite)

FetchContent_Declare(
    string-view-lite
    URL https://github.com/martinmoene/string-view-lite/archive/d46fe4d59214779d433b30bba2911119b43fa711.tar.gz
    URL_HASH SHA256=454d949acdd9fe75cf55c1078ba9ce1fd2fcf0b85f45993cc28c7d097da1ec85
)
FetchContent_MakeAvailable(string-view-lite)

set (FMT_INSTALL ON CACHE BOOL "" FORCE)
FetchContent_Declare(
    fmt
    URL https://github.com/fmtlib/fmt/archive/refs/tags/11.2.0.tar.gz
    URL_HASH SHA256=bc23066d87ab3168f27cef3e97d545fa63314f5c79df5ea444d41d56f962c6af
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
