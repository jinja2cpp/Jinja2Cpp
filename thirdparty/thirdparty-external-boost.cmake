# Deprecated alias, removed in 3.0 (docs/tasks/0230): the library no longer needs Boost, so
# this mode is the internal one, except that the opt-in Boost adapters take Boost from
# find_package. It installs the internal mode's package config.
message(DEPRECATION "JINJA2CPP_DEPS_MODE=external-boost is deprecated and will be removed in 3.0: "
    "Jinja2C++ no longer needs Boost. Use 'internal'; the Boost adapters' tests then fetch what they need.")
message(STATUS "'external-boost' dependencies mode selected for Jinja2Cpp. All
dependencies are built from source pulled from github; Boost, used only by the opt-in adapters, comes from the system")

include (./thirdparty/internal_deps.cmake)
include (./thirdparty/external_boost_deps.cmake)
