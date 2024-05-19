message(STATUS "'internal' dependencies mode selected for Jinja2Cpp. All dependencies will be built from source pulled from github")

include (./thirdparty/internal_deps.cmake)

set(BOOST_ENABLE_CMAKE ON)
list(APPEND BOOST_INCLUDE_LIBRARIES
    algorithm
    assert
    atomic
    filesystem
    numeric/conversion
    lexical_cast
    optional
    variant
    json
    regex
)

include(FetchContent)
FetchContent_Declare(
    Boost
    URL https://github.com/boostorg/boost/releases/download/boost-1.85.0/boost-1.85.0-cmake.tar.gz
    URL_HASH SHA256=ab9c9c4797384b0949dd676cf86b4f99553f8c148d767485aaac412af25183e6
)

set(BOOST_SKIP_INSTALL_RULES OFF)
FetchContent_MakeAvailable(Boost)

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
        target_compile_options(boost_unordered INTERFACE -Wno-error=deprecated-declarations)
        target_compile_options(boost_filesystem PRIVATE -Wno-error=deprecated-declarations)
    endif()
    if(COMPILER_HAS_WNO_ERROR_MAYBE_UNINITIALIZED_FLAG)
        target_compile_options(boost_variant INTERFACE -Wno-error=maybe-uninitialized)
    endif()
endif()

# install(TARGETS boost_filesystem boost_algorithm boost_variant boost_optional
#        EXPORT InstallTargets
#        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
#        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
#        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/static
#        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/boost
#        )
