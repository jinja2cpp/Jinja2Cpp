message(STATUS "'external' dependencies mode selected for Jinja2Cpp. All dependencies are treated as external")

# Each dependency is found through the package config it installs (docs/tasks/0163)
find_package(expected-lite CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)

if ("${JINJA2CPP_WITH_JSON_BINDINGS}" STREQUAL "rapid")
    # Jinja2C++ builds its own RapidJSON with patches (internal_deps.cmake); a system copy
    # needs the same fixes
    find_package(RapidJSON CONFIG REQUIRED)
    add_library(RapidJson INTERFACE)
    if (TARGET RapidJSON)
        target_link_libraries(RapidJson INTERFACE RapidJSON)
    else ()
        target_include_directories(RapidJson SYSTEM INTERFACE ${RapidJSON_INCLUDE_DIRS} ${RAPIDJSON_INCLUDE_DIRS})
    endif ()
endif ()

if (JINJA2CPP_BUILD_TESTS OR "${JINJA2CPP_WITH_JSON_BINDINGS}" STREQUAL "nlohmann")
    find_package(nlohmann_json 3 CONFIG REQUIRED)
endif ()

include (./thirdparty/external_boost_deps.cmake)

set(JINJA2_PRIVATE_LIBS_INT ${JINJA2CPP_BOOST_LIBS} fmt::fmt)
set(JINJA2_PUBLIC_LIBS_INT nonstd::expected-lite)
