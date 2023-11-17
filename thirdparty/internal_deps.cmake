include(FetchContent)

FetchContent_Declare(
    expected-lite
    GIT_REPOSITORY https://github.com/martinmoene/expected-lite.git
    GIT_TAG master
)
FetchContent_MakeAvailable(expected-lite)

FetchContent_Declare(
    variant-lite
    GIT_REPOSITORY https://github.com/martinmoene/variant-lite.git
    GIT_TAG master
)
FetchContent_MakeAvailable(variant-lite)

FetchContent_Declare(
    optional-lite
    GIT_REPOSITORY https://github.com/martinmoene/optional-lite.git
    GIT_TAG master
)
FetchContent_MakeAvailable(optional-lite)

FetchContent_Declare(
    string-view-lite
    GIT_REPOSITORY https://github.com/martinmoene/string-view-lite.git
    GIT_TAG master
)
FetchContent_MakeAvailable(string-view-lite)

set (FMT_INSTALL ON CACHE BOOL "" FORCE)
FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG 10.1.1
)
FetchContent_MakeAvailable(fmt)

set (RAPIDJSON_BUILD_DOC OFF CACHE BOOL "" FORCE)
set (RAPIDJSON_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set (RAPIDJSON_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set (RAPIDJSON_BUILD_THIRDPARTY_GTEST OFF CACHE BOOL "" FORCE)
set (RAPIDJSON_ENABLE_INSTRUMENTATION_OPT OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    rapidjson
    GIT_REPOSITORY https://github.com/Tencent/rapidjson.git
    GIT_TAG 973dc9c06dcd3d035ebd039cfb9ea457721ec213
)
#    GIT_TAG f9d53419e912910fd8fa57d5705fa41425428c35 - latest but broken revision
FetchContent_MakeAvailable(rapidjson)
find_package(RapidJSON REQUIRED)
add_library(RapidJson INTERFACE)
target_include_directories(RapidJson
    INTERFACE
        $<BUILD_INTERFACE:${RapidJSON_INCLUDE_DIR}>
        $<INSTALL_INTERFACE:include>
)
if (JINJA2CPP_BUILD_TESTS)
    set (JSON_BuildTests OFF CACHE BOOL "" FORCE)
    set (JSON_Install OFF CACHE BOOL "" FORCE)
    set (JSON_MultipleHeaders ON CACHE BOOL "" FORCE)

    FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG develop
    )
    FetchContent_MakeAvailable(nlohmann_json)
endif()

install (FILES
        thirdparty/nonstd/expected-lite/include/nonstd/expected.hpp
        thirdparty/nonstd/variant-lite/include/nonstd/variant.hpp
        thirdparty/nonstd/optional-lite/include/nonstd/optional.hpp
        thirdparty/nonstd/string-view-lite/include/nonstd/string_view.hpp
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/nonstd)

install (TARGETS RapidJson
    EXPORT InstallTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/static
)
