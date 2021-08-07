message(STATUS "'internal' dependencies mode selected for Jinja2Cpp. All dependencies are used as submodules")

include (./thirdparty/internal_deps.cmake)

update_submodule(boost)
set(BOOST_ENABLE_CMAKE ON)
list(APPEND BOOST_INCLUDE_LIBRARIES
    algorithm
    assert
    atomic
    filesystem
    lexical_cast
    optional
    variant
)
set(BOOST_INCLUDE_LIBRARIES ${BOOST_INCLUDE_LIBRARIES} CACHE INTERNAL "")
add_subdirectory(thirdparty/boost)

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

# install(TARGETS boost_filesystem boost::algorithm boost::variant boost::optional
#        EXPORT InstallTargets
#        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
#        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
#        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/static
#        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/boost
#        )
