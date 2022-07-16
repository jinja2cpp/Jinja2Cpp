message(STATUS "'external-boost' dependencies mode selected for Jinja2Cpp. All dependencies are used as submodules except of boost")

include (./thirdparty/internal_deps.cmake)
include (./thirdparty/external_boost_deps.cmake)
