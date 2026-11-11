# Build hardening of the library (docs/tasks/0265): flags that trap only on actual memory
# corruption, where stopping is the only safe outcome. They go to the library target alone
# (PRIVATE), so a consumer's own build is not changed. _GLIBCXX_ASSERTIONS and
# _LIBCPP_HARDENING_MODE are left out on purpose: they abort on a precondition failure, which
# src/contracts.h reports as an error Result instead.
include(CheckCXXCompilerFlag)
include(CheckCXXSourceCompiles)

function (jinja2cpp_apply_hardening target)
    set(applied "")
    if (MSVC)
        # /GS is the default of cl, named so that a CMAKE_CXX_FLAGS without it is covered
        target_compile_options(${target} PRIVATE /GS /guard:cf)
        get_target_property(_type ${target} TYPE)
        if (_type STREQUAL "SHARED_LIBRARY")
            target_link_options(${target} PRIVATE /GUARD:CF)
        endif ()
        list(APPEND applied "/GS" "/guard:cf")
    else ()
        # A flag the compiler does not know (Apple Clang, an old GCC) is skipped
        function (_jinja2cpp_try_flag flag var)
            string(MAKE_C_IDENTIFIER "JINJA2CPP_HAS${flag}" cache_var)
            check_cxx_compiler_flag("${flag}" ${cache_var})
            set(${var} ${${cache_var}} PARENT_SCOPE)
        endfunction ()

        _jinja2cpp_try_flag("-fstack-protector-strong" has_ssp)
        if (has_ssp)
            target_compile_options(${target} PRIVATE -fstack-protector-strong)
            list(APPEND applied "-fstack-protector-strong")
        endif ()

        _jinja2cpp_try_flag("-fstack-clash-protection" has_clash)
        if (has_clash)
            target_compile_options(${target} PRIVATE -fstack-clash-protection)
            list(APPEND applied "-fstack-clash-protection")
        endif ()

        # Control-flow protection (CET) exists for x86 only
        if (CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64|amd64|i[3-6]86|x86)$")
            _jinja2cpp_try_flag("-fcf-protection" has_cf)
            if (has_cf)
                target_compile_options(${target} PRIVATE -fcf-protection)
                list(APPEND applied "-fcf-protection")
            endif ()
        endif ()

        # Fortified libc calls need optimisation; the sanitizers and the fuzzers intercept the
        # same calls, so they keep their own checks. Level 3 needs GCC 12 or Clang 9 and glibc 2.34;
        # glibc's own headers fall back to 2 for a compiler without it, and a check with -Werror
        # catches a toolchain that rejects 3
        if (JINJA2CPP_WITH_SANITIZERS STREQUAL "none" AND NOT JINJA2CPP_BUILD_FUZZERS)
            set(_saved_flags "${CMAKE_REQUIRED_FLAGS}")
            set(CMAKE_REQUIRED_FLAGS "-O2 -Werror -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=3")
            check_cxx_source_compiles("#include <cstring>\nint main(int argc, char** argv) { char b[8]; std::memcpy(b, argv[0], 4); return b[argc]; }" JINJA2CPP_HAS_FORTIFY_3)
            set(CMAKE_REQUIRED_FLAGS "${_saved_flags}")
            if (JINJA2CPP_HAS_FORTIFY_3)
                set(_fortify 3)
            else ()
                set(_fortify 2)
            endif ()
            target_compile_options(${target} PRIVATE
                "$<$<CONFIG:Release,RelWithDebInfo,MinSizeRel>:-U_FORTIFY_SOURCE>"
                "$<$<CONFIG:Release,RelWithDebInfo,MinSizeRel>:-D_FORTIFY_SOURCE=${_fortify}>")
            list(APPEND applied "-D_FORTIFY_SOURCE=${_fortify} (optimised configs)")
        endif ()
    endif ()
    list(JOIN applied " " applied_text)
    message(STATUS "Jinja2C++ hardening: ${applied_text}")
endfunction ()
