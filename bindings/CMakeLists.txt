# The JSON bindings and the Boost big-integer adapter are components of the package
# (docs/tasks/0311): each one is a header-only INTERFACE target jinja2cpp::<name> with its
# own headers, export set, deps file and pkg-config file. The library itself never includes
# them. Every component is installed whatever the build tested; a consumer's
# find_package(jinja2cpp COMPONENTS <name>) finds the component's dependency only then
# (cmake/public/jinja2cpp-config.cmake.in).

set(JINJA2CPP_COMPONENTS "" CACHE INTERNAL "")

# jinja2cpp_add_binding(NAME <name> DESCRIPTION <text> FIND <find_package arguments>
#                       TARGETS <installed link targets> [BUILD_TARGETS <build-tree link targets>]
#                       [PC_REQUIRES <pkg-config modules>] [PC_LIBS <flags>])
# BUILD_TARGETS defaults to TARGETS. A component whose dependency this build does not have is
# still installed; only linking it in the build tree needs the dependency's target.
function(jinja2cpp_add_binding)
    cmake_parse_arguments(ARG "" "NAME;DESCRIPTION;PC_LIBS" "FIND;TARGETS;BUILD_TARGETS;PC_REQUIRES" ${ARGN})
    if (NOT ARG_BUILD_TARGETS)
        set(ARG_BUILD_TARGETS ${ARG_TARGETS})
    endif ()
    set(_target jinja2cpp_${ARG_NAME})
    set(_dir ${CMAKE_CURRENT_SOURCE_DIR}/${ARG_NAME}/include)
    file(GLOB_RECURSE _headers CONFIGURE_DEPENDS ${_dir}/*.h)

    add_library(${_target} INTERFACE)
    add_library(jinja2cpp::${ARG_NAME} ALIAS ${_target})
    set_target_properties(${_target} PROPERTIES EXPORT_NAME ${ARG_NAME})
    target_sources(${_target} INTERFACE FILE_SET HEADERS BASE_DIRS ${_dir} FILES ${_headers})
    target_link_libraries(${_target} INTERFACE ${LIB_TARGET_NAME})
    foreach (_dep IN LISTS ARG_BUILD_TARGETS)
        target_link_libraries(${_target} INTERFACE "$<BUILD_INTERFACE:${_dep}>")
    endforeach ()
    # The installed target gets its dependency's targets from jinja2cpp-config.cmake, which
    # finds them first: an exported link to a target of this build, such as the fetched
    # nlohmann_json, would have to be exported too

    set(JINJA2CPP_COMPONENTS ${JINJA2CPP_COMPONENTS} ${ARG_NAME} CACHE INTERNAL "")
    if (NOT JINJA2CPP_INSTALL)
        return()
    endif ()

    install(TARGETS ${_target} EXPORT jinja2cpp-${ARG_NAME}-targets FILE_SET HEADERS)
    install(EXPORT jinja2cpp-${ARG_NAME}-targets NAMESPACE jinja2cpp:: DESTINATION ${JINJA2CPP_INSTALL_CONFIG_DIR})

    list(JOIN ARG_FIND " " JINJA2CPP_BINDING_FIND)
    list(JOIN ARG_TARGETS " " JINJA2CPP_BINDING_TARGETS)
    set(JINJA2CPP_BINDING_NAME ${ARG_NAME})
    configure_file(${PROJECT_SOURCE_DIR}/cmake/public/jinja2cpp-component-deps.cmake.in
        ${PROJECT_BINARY_DIR}/cmake/config/jinja2cpp-${ARG_NAME}-deps.cmake @ONLY)

    set(JINJA2CPP_BINDING_DESCRIPTION "${ARG_DESCRIPTION}")
    set(JINJA2CPP_BINDING_PC_REQUIRES "jinja2cpp = ${PROJECT_VERSION}")
    foreach (_module IN LISTS ARG_PC_REQUIRES)
        string(APPEND JINJA2CPP_BINDING_PC_REQUIRES ", ${_module}")
    endforeach ()
    set(JINJA2CPP_BINDING_PC_LIBS "${ARG_PC_LIBS}")
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/jinja2cpp-binding.pc.in
        ${PROJECT_BINARY_DIR}/jinja2cpp-${ARG_NAME}.pc @ONLY)

    install(FILES ${PROJECT_BINARY_DIR}/cmake/config/jinja2cpp-${ARG_NAME}-deps.cmake
        DESTINATION ${JINJA2CPP_INSTALL_CONFIG_DIR})
    install(FILES ${PROJECT_BINARY_DIR}/jinja2cpp-${ARG_NAME}.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
endfunction()

jinja2cpp_add_binding(NAME nlohmann_json
    DESCRIPTION "nlohmann::json"
    FIND nlohmann_json 3 CONFIG
    TARGETS nlohmann_json::nlohmann_json
    PC_REQUIRES nlohmann_json)

# Boost ships no pkg-config files, so the pkg-config file names the libraries
jinja2cpp_add_binding(NAME boost_json
    DESCRIPTION "boost::json"
    FIND Boost 1.75 CONFIG COMPONENTS json
    TARGETS Boost::json
    PC_LIBS "-lboost_json -lboost_container")

# Header-only. In the build tree it takes the headers the build already uses for the
# built-in backend (thirdparty/CMakeLists.txt): standalone Multiprecision or Boost::headers.
jinja2cpp_add_binding(NAME boost_multiprecision
    DESCRIPTION "Boost.Multiprecision big integers"
    FIND Boost 1.75 CONFIG
    TARGETS Boost::headers
    BUILD_TARGETS jinja2cpp_boost_mp_headers)
