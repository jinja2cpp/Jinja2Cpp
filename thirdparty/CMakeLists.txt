if(MSVC)
    set (THIRDPARTY_RUNTIME_TYPE ${MSVC_RUNTIME_TYPE})
    if ("${THIRDPARTY_RUNTIME_TYPE}" STREQUAL "")
        string (FIND "${CURRENT_CXX_FLAGS}" "MT" THIRDPARTY_MT_POS REVERSE)
        string (FIND "${CURRENT_CXX_FLAGS}" "MD" THIRDPARTY_MD_POS REVERSE)
        if (NOT THIRDPARTY_MT_POS EQUAL -1)
            set (THIRDPARTY_RUNTIME_TYPE "/MT")
        elseif (NOT THIRDPARTY_MD_POS EQUAL -1)
            set (THIRDPARTY_RUNTIME_TYPE "/MD")
        else ()
            message (STATUS "Dynamic C runtime assumed. Use 'MSVC_RUNTIME_TYPE' variable for override")
            set (THIRDPARTY_RUNTIME_TYPE "/MD")
        endif()
    endif ()
endif ()

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

include (./thirdparty/thirdparty-${JINJA2CPP_DEPS_MODE}.cmake)

if(JINJA2CPP_BUILD_TESTS)
    find_package(gtest QUIET)

    if(gtest_FOUND)
        imported_target_alias(gtest ALIAS gtest::gtest)
    else()
        message(STATUS "gtest not found, using submodule")
        update_submodule(gtest)

        if(MSVC)
            if (THIRDPARTY_RUNTIME_TYPE STREQUAL "/MD" OR THIRDPARTY_RUNTIME_TYPE STREQUAL "/MDd")
                set (gtest_force_shared_crt ON CACHE BOOL "" FORCE)
            else ()
                set (gtest_force_shared_crt OFF CACHE BOOL "" FORCE)
            endif ()
        endif ()

        add_subdirectory(thirdparty/gtest EXCLUDE_FROM_ALL)
    endif()
endif()

if (NOT DEFINED JINJA2_PRIVATE_LIBS_INT)
    set (JINJA2CPP_PRIVATE_LIBS ${JINJA2CPP_PRIVATE_LIBS} boost_variant boost_filesystem boost_algorithm fmt rh_lib)
else ()
    set (JINJA2CPP_PRIVATE_LIBS ${JINJA2_PRIVATE_LIBS_INT})
endif ()


if (NOT DEFINED JINJA2_PUBLIC_LIBS_INT)
    set (JINJA2CPP_PUBLIC_LIBS ${JINJA2CPP_PUBLIC_LIBS} expected-lite variant-lite optional-lite string-view-lite)
else ()
    set (JINJA2CPP_PUBLIC_LIBS ${JINJA2_PUBLIC_LIBS_INT})
endif ()
