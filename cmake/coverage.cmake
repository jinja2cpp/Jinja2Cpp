if ((NOT ${CMAKE_CXX_COMPILER_ID} MATCHES "Clang") AND (NOT ${CMAKE_CXX_COMPILER_ID} MATCHES "GNU"))
    message(WARNING "coverage build is not supported on such compiler ${CMAKE_CXX_COMPILER_ID}")
    set(JINJA2CPP_WITH_COVERAGE OFF)
    return()
endif()

function(add_coverage_target _TARGET)
    if (NOT TARGET ${_TARGET})
        add_library(${_TARGET} INTERFACE)
    endif()
    target_compile_options(
            ${_TARGET}
        INTERFACE
            -fprofile-arcs -ftest-coverage
    )
    target_link_libraries(${_TARGET} INTERFACE gcov)

    install(
        TARGETS
            ${_TARGET}
        EXPORT
            InstallTargets
    )
endfunction()
