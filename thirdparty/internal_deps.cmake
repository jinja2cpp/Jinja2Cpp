function(update_submodule submodule)
    find_package(Git REQUIRED)
    execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init thirdparty/${submodule}
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
endfunction()

function(imported_target_alias ALIAS)
    # For some unknown reason CMake does not support creating alias
    # libraries from IMPORTED libraries. This function is an ugly workaround
    # to get the same

    cmake_parse_arguments("__ALIAS"
        ""
        "ALIAS"
        ""
        ${ARGN}
    )

    if(NOT __ALIAS_ALIAS)
        message(FATAL_ERROR "imported_target_alias invoked with wrong arguments, missing ALIAS")
    endif()

    add_library(${ALIAS} INTERFACE)
    target_link_libraries(${ALIAS} INTERFACE ${__ALIAS_ALIAS})
endfunction()

message(STATUS "'internal' dependencies mode selected for Jinja2Cpp. All dependencies are used as submodules")


update_submodule(nonstd/expected-lite)
add_subdirectory(thirdparty/nonstd/expected-lite EXCLUDE_FROM_ALL)

update_submodule(nonstd/variant-lite)
add_subdirectory(thirdparty/nonstd/variant-lite EXCLUDE_FROM_ALL)
add_library(variant-lite ALIAS variant-lite)

update_submodule(nonstd/optional-lite)
add_subdirectory(thirdparty/nonstd/optional-lite EXCLUDE_FROM_ALL)
add_library(optional-lite ALIAS optional-lite)

update_submodule(nonstd/value-ptr-lite)
add_subdirectory(thirdparty/nonstd/value-ptr-lite EXCLUDE_FROM_ALL)
add_library(value-ptr-lite ALIAS value_ptr-lite)