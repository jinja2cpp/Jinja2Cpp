#if ((NOT ${CMAKE_CXX_COMPILER_ID} MATCHES "Clang") AND (NOT ${CMAKE_CXX_COMPILER_ID} MATCHES "GNU"))
#   message(WARNING "sanitized build is not supported using this compiler ${CMAKE_CXX_COMPILER_ID}")
#   set(JINJA2CPP_WITH_SANITIZERS OFF)
#   return()
#endif()

set(_BASE_SANITIZER_FLAGS "-fno-omit-frame-pointer -fno-optimize-sibling-calls")
separate_arguments(_BASE_SANITIZER_FLAGS)
set(_BASE_ENABLE_SANITIZER_FLAGS)
if(JINJA2CPP_WITH_SANITIZERS STREQUAL address+undefined)
    set(_BASE_ENABLE_SANITIZER_FLAGS "-fsanitize=address,undefined")
endif()

if(JINJA2CPP_WITH_SANITIZERS STREQUAL memory)
    set(_BASE_ENABLE_SANITIZER_FLAGS "-fsanitize=memory")
endif()

function(add_sanitizer_target _TARGET)
    if (NOT TARGET ${_TARGET})
        add_library(${_TARGET} INTERFACE)
    endif()
    target_compile_options(
            ${_TARGET}
        INTERFACE
            ${_BASE_SANITIZER_FLAGS} ${_BASE_ENABLE_SANITIZER_FLAGS}
    )
    target_link_libraries(
            ${_TARGET}
        INTERFACE
            ${_BASE_ENABLE_SANITIZER_FLAGS}
    )

    install(
        TARGETS
            ${_TARGET}
        EXPORT
            InstallTargets
    )
endfunction()


