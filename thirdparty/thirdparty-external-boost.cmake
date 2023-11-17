message(STATUS "'external-boost' dependencies mode selected for Jinja2Cpp. All
dependencies are built from source pulled from github except of boost")

include (./thirdparty/internal_deps.cmake)
include (./thirdparty/external_boost_deps.cmake)
