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

find_package(expected-lite QUIET)
if(expected-lite_FOUND)
    imported_target_alias(expected-lite ALIAS expected-lite::expected-lite)
else()
    message(STATUS "expected-lite not found, using submodule")
    update_submodule(nonstd/expected-lite)
    add_subdirectory(nonstd/expected-lite EXCLUDE_FROM_ALL)
endif()

find_package(variant-lite QUIET)
if(variant-lite_FOUND)
    imported_target_alias(variant-lite ALIAS variant-lite::variant-lite)
else()
    message(STATUS "variant-lite not found, using submodule")
    update_submodule(nonstd/variant-lite)
    add_subdirectory(nonstd/variant-lite EXCLUDE_FROM_ALL)

    add_library(variant-lite ALIAS variant-lite)
endif()

find_package(optional-lite QUIET)
if(optional-lite_FOUND)
    imported_target_alias(optional-lite ALIAS optional-lite::optional-lite)
else()
    message(STATUS "optional-lite not found, using submodule")
    update_submodule(nonstd/optional-lite)
    add_subdirectory(nonstd/optional-lite EXCLUDE_FROM_ALL)

    add_library(optional-lite ALIAS optional-lite)
endif()

find_package(value-ptr-lite QUIET)
if(value-ptr-lite_FOUND)
    imported_target_alias(value-ptr-lite ALIAS value-ptr-lite::value-ptr-lite)
else()
    message(STATUS "value-ptr-lite not found, using submodule")
    update_submodule(nonstd/value-ptr-lite)
    add_subdirectory(nonstd/value-ptr-lite EXCLUDE_FROM_ALL)

    add_library(value-ptr-lite ALIAS value_ptr-lite)
endif()

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


find_package(boost_filesystem QUIET)
find_package(boost_algorithm  QUIET)
find_package(boost_variant    QUIET)
find_package(boost_optional   QUIET)

if(boost_filesystem_FOUND AND
   boost_algorithm_FOUND  AND
   boost_variant_FOUND    AND
   boost_optional_FOUND)
   imported_target_alias(boost_filesystem ALIAS boost_filesystem::boost_filesystem)
   imported_target_alias(boost_algorithm  ALIAS boost_algorithm::boost_algorithm)
   imported_target_alias(boost_variant    ALIAS boost_variant::boost_variant)
   imported_target_alias(boost_optional   ALIAS boost_optional::boost_optional)
else()
        if (MSVC)
                if (NOT DEFINED Boost_USE_STATIC_LIBS)
                        if (THIRDPARTY_RUNTIME_TYPE STREQUAL "/MD" OR THIRDPARTY_RUNTIME_TYPE STREQUAL "/MDd")
                                set (Boost_USE_STATIC_LIBS OFF)
                                set (Boost_USE_STATIC_RUNTIME OFF)
                        else ()
                                set (Boost_USE_STATIC_LIBS ON)
                                set (Boost_USE_STATIC_RUNTIME ON)
                        endif ()
                endif ()
        endif ()

    find_package(Boost COMPONENTS system filesystem QUIET)

    if (Boost_FOUND)
        imported_target_alias(boost_filesystem ALIAS Boost::filesystem)
        imported_target_alias(boost_algorithm  ALIAS Boost::boost)
        imported_target_alias(boost_variant    ALIAS Boost::boost)
        imported_target_alias(boost_optional   ALIAS Boost::boost)
    else()
        message(STATUS "One or more boost modules not found, using submodule")
        update_submodule(boost)
        list(APPEND BOOST_CMAKE_LIBRARIES filesystem algorithm variant optional)
        set(BOOST_CMAKE_LIBRARIES ${BOOST_CMAKE_LIBRARIES} CACHE INTERNAL "")
        add_subdirectory(boost EXCLUDE_FROM_ALL)

        if(NOT MSVC)
            # Enable -Werror and -Wall on jinja2cpp target, ignoring warning errors from thirdparty libs
            include(CheckCXXCompilerFlag)
            check_cxx_compiler_flag(-Wno-error=parentheses COMPILER_HAS_WNO_ERROR_PARENTHESES_FLAG)
            check_cxx_compiler_flag(-Wno-error=deprecated-declarations COMPILER_HAS_WNO_ERROR_DEPRECATED_DECLARATIONS_FLAG)
            check_cxx_compiler_flag(-Wno-error=maybe-uninitialized COMPILER_HAS_WNO_ERROR_MAYBE_UNINITIALIZED_FLAG)

            if(COMPILER_HAS_WNO_ERROR_PARENTHESES_FLAG)
                target_compile_options(boost_assert INTERFACE -Wno-error=parentheses)
            endif()
            if(COMPILER_HAS_WNO_ERROR_DEPRECATED_DECLARATIONS_FLAG)
                target_compile_options(boost_filesystem PRIVATE -Wno-error=deprecated-declarations)
            endif()
            if(COMPILER_HAS_WNO_ERROR_MAYBE_UNINITIALIZED_FLAG)
                target_compile_options(boost_variant INTERFACE -Wno-error=maybe-uninitialized)
            endif()
                else ()
        endif()
    endif()
endif()

if(JINJA2CPP_BUILD_TESTS)
    find_package(gtest QUIET)

    if(gtest_FOUND)
        imported_target_alias(gtest ALIAS gtest::gtest)
    else()
        message(STATUS "expected-lite not found, using submodule")
        update_submodule(gtest)

        if(MSVC)
                        if (THIRDPARTY_RUNTIME_TYPE STREQUAL "/MD" OR THIRDPARTY_RUNTIME_TYPE STREQUAL "/MDd")
                                set (gtest_force_shared_crt ON CACHE BOOL "" FORCE)
                        else ()
                                set (gtest_force_shared_crt OFF CACHE BOOL "" FORCE)
                        endif ()
        endif ()

        add_subdirectory(gtest EXCLUDE_FROM_ALL)
    endif()
endif()
