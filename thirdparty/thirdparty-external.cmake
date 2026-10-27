message(STATUS "'external' dependencies mode selected for Jinja2Cpp. All dependencies are treated as external")

# Each dependency is found through the package config it installs (docs/tasks/0163)
find_package(expected-lite CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(FastFloat CONFIG REQUIRED)

# The JSON libraries are for the tests only: the library reads {% meta %} itself (0230 D6)
if (JINJA2CPP_BUILD_TESTS)
    find_package(nlohmann_json 3 CONFIG REQUIRED)
endif ()

include (./thirdparty/external_boost_deps.cmake)

set(JINJA2_PRIVATE_LIBS_INT fmt::fmt)
set(JINJA2_PUBLIC_LIBS_INT nonstd::expected-lite)
