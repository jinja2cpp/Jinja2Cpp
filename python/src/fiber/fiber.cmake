# The fiber switch of the Python module's lazy Template.generate() (python/src/fiber, docs/tasks/0457), a
# static library without Python that jinja2cpp_tests links too. Included by the top-level CMakeLists.txt
# (tests) and by the module's build; defines jinja2cpp_fiber once.
if (TARGET jinja2cpp_fiber)
    return()
endif ()

set(_fiber_dir "${CMAKE_CURRENT_LIST_DIR}")
get_filename_component(_fiber_root "${_fiber_dir}/../../.." ABSOLUTE)
add_library(jinja2cpp_fiber STATIC "${_fiber_dir}/fiber.cpp" "${_fiber_dir}/fiber.h")
target_include_directories(jinja2cpp_fiber PUBLIC "${_fiber_dir}")
target_compile_features(jinja2cpp_fiber PUBLIC cxx_std_17)
set_target_properties(jinja2cpp_fiber PROPERTIES POSITION_INDEPENDENT_CODE ON)

# boostorg/context's fcontext assembly (thirdparty/boost_context) where it is vendored: x86-64 and AArch64,
# on Linux and the BSDs (ELF) and macOS (Mach-O). Windows uses its fiber API; elsewhere fibers are unavailable
set(_fiber_arch "${CMAKE_SYSTEM_PROCESSOR}")
if (APPLE AND CMAKE_OSX_ARCHITECTURES)
    list(LENGTH CMAKE_OSX_ARCHITECTURES _fiber_arch_count)
    if (_fiber_arch_count EQUAL 1)
        set(_fiber_arch "${CMAKE_OSX_ARCHITECTURES}")
    else ()
        # A universal build would need both switches in one object; not supported
        set(_fiber_arch "")
    endif ()
endif ()
string(TOLOWER "${_fiber_arch}" _fiber_arch)
if (NOT WIN32 AND _fiber_arch MATCHES "^(x86_64|amd64|aarch64|arm64)$" AND CMAKE_SIZEOF_VOID_P EQUAL 8)
    enable_language(ASM)
    target_sources(jinja2cpp_fiber PRIVATE "${_fiber_dir}/fiber_make.S" "${_fiber_dir}/fiber_jump.S")
    set_source_files_properties("${_fiber_dir}/fiber_make.S" "${_fiber_dir}/fiber_jump.S" PROPERTIES
        INCLUDE_DIRECTORIES "${_fiber_root}/thirdparty/boost_context")
    target_compile_definitions(jinja2cpp_fiber PRIVATE JINJA2CPP_FIBER_FCONTEXT)
endif ()
if (JINJA2CPP_TARGET_CXX_FLAGS)
    target_compile_options(jinja2cpp_fiber PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:${JINJA2CPP_TARGET_CXX_FLAGS}>")
endif ()
if (TARGET jinja2cpp_build_sanitizers)
    target_link_libraries(jinja2cpp_fiber PUBLIC jinja2cpp_build_sanitizers)
endif ()
