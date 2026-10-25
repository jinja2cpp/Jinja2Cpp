message(STATUS "'internal' dependencies mode selected for Jinja2Cpp. All dependencies will be built from source pulled from github")

include (./thirdparty/internal_deps.cmake)

# The library links no Boost (docs/tasks/0230). The Boost release is fetched only for the
# Boost.JSON adapter test, and never installed: the package names no Boost.
if (JINJA2CPP_BUILD_TESTS AND "${JINJA2CPP_WITH_JSON_BINDINGS}" STREQUAL "boost")
    set(BOOST_ENABLE_CMAKE ON)
    list(APPEND BOOST_INCLUDE_LIBRARIES json)

    include(FetchContent)
    FetchContent_Declare(
        Boost
        URL
        https://github.com/boostorg/boost/releases/download/boost-1.92.0/boost-1.92.0-cmake.tar.xz
        URL_HASH SHA256=9bed76128d4e46755dbe818487788c6fceb6f72b378f4daa49b7e1e600d9088d
    )

    set(BOOST_SKIP_INSTALL_RULES ON)
    FetchContent_MakeAvailable(Boost)
    jinja2cpp_mark_system("${boost_SOURCE_DIR}")

    if(NOT MSVC)
        # Ignore warning errors from the Boost headers the test includes
        include(CheckCXXCompilerFlag)
        check_cxx_compiler_flag(-Wno-error=parentheses COMPILER_HAS_WNO_ERROR_PARENTHESES_FLAG)
        check_cxx_compiler_flag(-Wno-error=deprecated-declarations COMPILER_HAS_WNO_ERROR_DEPRECATED_DECLARATIONS_FLAG)

        if(COMPILER_HAS_WNO_ERROR_PARENTHESES_FLAG AND TARGET boost_assert)
            target_compile_options(boost_assert INTERFACE -Wno-error=parentheses)
        endif()
        if(COMPILER_HAS_WNO_ERROR_DEPRECATED_DECLARATIONS_FLAG AND TARGET boost_unordered)
            target_compile_options(boost_unordered INTERFACE -Wno-error=deprecated-declarations)
        endif()
    endif()
endif ()
