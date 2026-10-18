message(STATUS "'internal' dependencies mode selected for Jinja2Cpp. All dependencies will be built from source pulled from github")

include (./thirdparty/internal_deps.cmake)

set(BOOST_ENABLE_CMAKE ON)
list(APPEND BOOST_INCLUDE_LIBRARIES
    algorithm
    any
    assert
    atomic
    container
    filesystem
    json
)

set(JINJA2CPP_BOOST_VERSION 1.92.0)
include(FetchContent)
FetchContent_Declare(
    Boost
    URL
    https://github.com/boostorg/boost/releases/download/boost-1.92.0/boost-1.92.0-cmake.tar.xz
    URL_HASH SHA256=9bed76128d4e46755dbe818487788c6fceb6f72b378f4daa49b7e1e600d9088d
)

set(BOOST_SKIP_INSTALL_RULES OFF)
FetchContent_MakeAvailable(Boost)
jinja2cpp_mark_system("${boost_SOURCE_DIR}")

if(NOT MSVC)
    # Enable -Werror and -Wall on jinja2cpp target, ignoring warning errors from thirdparty libs
    include(CheckCXXCompilerFlag)
    check_cxx_compiler_flag(-Wno-error=parentheses COMPILER_HAS_WNO_ERROR_PARENTHESES_FLAG)
    check_cxx_compiler_flag(-Wno-error=deprecated-declarations COMPILER_HAS_WNO_ERROR_DEPRECATED_DECLARATIONS_FLAG)

    if(COMPILER_HAS_WNO_ERROR_PARENTHESES_FLAG)
        target_compile_options(boost_assert INTERFACE -Wno-error=parentheses)
    endif()
    if(COMPILER_HAS_WNO_ERROR_DEPRECATED_DECLARATIONS_FLAG)
        if(TARGET boost_unordered)
            target_compile_options(boost_unordered INTERFACE -Wno-error=deprecated-declarations)
        endif()
        target_compile_options(boost_filesystem PRIVATE -Wno-error=deprecated-declarations)
    endif()
endif()

# install(TARGETS boost_filesystem boost_algorithm
#        EXPORT InstallTargets
#        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
#        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
#        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/static
#        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/boost
#        )
