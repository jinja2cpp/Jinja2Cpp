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
    update_submodule(nonstd/expected-light)
    add_subdirectory(nonstd/expected-light EXCLUDE_FROM_ALL)
endif()

find_package(variant-lite QUIET)
find_package(optional-lite QUIET)
if(variant-lite_FOUND AND optional-lite_FOUND)
    imported_target_alias(variant-lite ALIAS variant-lite::variant-lite)
    imported_target_alias(optional-lite ALIAS optional-lite::optional-lite)
else()
    message(STATUS "variant-lite or optional-lite not found, using submodule")
    update_submodule(nonstd/variant-light)
    add_subdirectory(nonstd/variant-light EXCLUDE_FROM_ALL)
    # There's a bug in the lib, the target does not include the header include dirs.
    # See https://github.com/martinmoene/variant-lite/issues/25
    target_include_directories(variant-lite INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/nonstd/variant-light/include")

    # Fake target until we use separated optional-lite as submodule
    # See https://github.com/martinmoene/variant-lite/issues/19
    add_library(optional-lite ALIAS variant-lite)
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
            if (MSVC_RUNTIME_TYPE STREQUAL "/MD" OR NOT MSVC_RUNTIME_TYPE)
                set (GTEST_EXTRA_OPTIONS "-Dgtest_force_shared_crt=TRUE" CACHE INTERNAL "")
            else ()
                set (GTEST_EXTRA_OPTIONS "-Dgtest_force_shared_crt=TRUE" CACHE INTERNAL "")
            endif ()
        endif ()

        add_subdirectory(gtest EXCLUDE_FROM_ALL)
    endif()
endif()
