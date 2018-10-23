
find_package(expected-lite QUIET)
if(expected-lite_FOUND)
    add_library(expected-lite ALIAS expected-lite::expected-lite)
else()
    message(STATUS "expected-lite not found, using submodule")
    add_subdirectory(nonstd/expected-light EXCLUDE_FROM_ALL)
endif()

find_package(variant-lite QUIET)
if(variant-lite_FOUND)
    add_library(variant-lite ALIAS variant-lite::variant-lite)
else()
    message(STATUS "variant-lite not found, using submodule")
    add_subdirectory(nonstd/variant-light EXCLUDE_FROM_ALL)
    # There's a bug in the lib, the target does not include the header include dirs.
    # See https://github.com/martinmoene/variant-lite/issues/25
    target_include_directories(variant-lite INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/nonstd/variant-light/include")
endif()

find_package(value-ptr-lite QUIET)
if(value-ptr-lite_FOUND)
    add_library(value-ptr-lite ALIAS value-ptr-lite::value-ptr-lite)
else()
    message(STATUS "value-ptr-lite not found, using submodule")
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
   add_library(boost_filesystem ALIAS boost_filesystem::boost_filesystem)
   add_library(boost_algorithm  ALIAS boost_algorithm::boost_algorithm)
   add_library(boost_variant    ALIAS boost_variant::boost_variant)
   add_library(boost_optional   ALIAS boost_optional::boost_optional)
else()
    message(STATUS "One or more boost modules not found, using submodule")
    set(BOOST_CMAKE_LIBRARIES filesystem algorithm variant optional CACHE INTERNAL "")
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

if(JINJA2CPP_BUILD_TESTS)
    find_package(gtest QUIET)

    if(gtest_FOUND)
        add_library(gtest ALIAS gtest::gtest)
    else()
        message(STATUS "expected-lite not found, using submodule")

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
