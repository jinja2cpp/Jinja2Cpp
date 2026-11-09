# The Python module jinja2cpp._jinja2cpp (docs/python.md, docs/tasks/0353): nanobind, CPython 3.12
# stable ABI (abi3), linking the library built static and position-independent in this build
find_package(Python 3.12 REQUIRED COMPONENTS Interpreter Development.Module OPTIONAL_COMPONENTS Development.SABIModule)

# nanobind: exactly the version thirdparty/internal_deps.cmake pins (JINJA2CPP_NANOBIND_VERSION).
# A Python that has it installed (scikit-build-core installs pyproject.toml's build requirement)
# gives it through `python -m nanobind --cmake_dir`; otherwise the pinned sdist is fetched
execute_process(
    COMMAND "${Python_EXECUTABLE}" -m nanobind --cmake_dir
    OUTPUT_VARIABLE _nanobind_dir OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE _nanobind_found
    ERROR_QUIET)
if (DEFINED JINJA2CPP_NANOBIND_VERSION)
    # The build requirement must name the pin, so a wheel build and a plain CMake build use one nanobind
    file(STRINGS "${PROJECT_SOURCE_DIR}/pyproject.toml" _nanobind_requirement REGEX "\"nanobind==")
    if (NOT _nanobind_requirement MATCHES "\"nanobind==${JINJA2CPP_NANOBIND_VERSION}\"")
        message(FATAL_ERROR "pyproject.toml must require nanobind==${JINJA2CPP_NANOBIND_VERSION}, the pin of thirdparty/internal_deps.cmake")
    endif ()
    if (_nanobind_found EQUAL 0)
        find_package(nanobind ${JINJA2CPP_NANOBIND_VERSION} EXACT CONFIG QUIET PATHS "${_nanobind_dir}" NO_DEFAULT_PATH)
    endif ()
    if (NOT nanobind_FOUND)
        FetchContent_MakeAvailable(nanobind)
        find_package(nanobind CONFIG REQUIRED PATHS "${nanobind_SOURCE_DIR}/cmake" NO_DEFAULT_PATH)
    endif ()
elseif (_nanobind_found EQUAL 0)
    # The external and Conan modes pin no nanobind: the build's Python provides it
    find_package(nanobind CONFIG REQUIRED PATHS "${_nanobind_dir}" NO_DEFAULT_PATH)
else ()
    message(FATAL_ERROR "JINJA2CPP_BUILD_PYTHON with JINJA2CPP_DEPS_MODE=${JINJA2CPP_DEPS_MODE} needs nanobind installed in ${Python_EXECUTABLE}")
endif ()
message(STATUS "Jinja2C++ Python module: nanobind from ${nanobind_DIR}")

# NB_STATIC: nanobind's own runtime in the module; NB_SUPPRESS_WARNINGS: its headers are system
# headers, so the library's strict warnings apply to the module's own code only;
# NOMINSIZE: the module's code is optimized for speed, as the library's, not for size (-Os)
nanobind_add_module(_jinja2cpp STABLE_ABI NB_STATIC NB_SUPPRESS_WARNINGS NOMINSIZE
    src/module.cpp
    src/convert.cpp
    src/adapters.cpp
    src/context.cpp
    src/environment.cpp
    src/errors.cpp
    src/loaders.cpp
    src/template_module.cpp
)
# The library's private dependencies too (fmt): the private headers include them
target_link_libraries(_jinja2cpp PRIVATE ${LIB_TARGET_NAME} ${JINJA2CPP_PRIVATE_LIBS})
# The module reads the library's private headers (src/host_object.h, src/template_impl.h)
target_include_directories(_jinja2cpp PRIVATE "${PROJECT_SOURCE_DIR}/src" "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_compile_options(_jinja2cpp PRIVATE ${JINJA2CPP_TARGET_CXX_FLAGS})
set_target_properties(_jinja2cpp PROPERTIES
    CXX_STANDARD ${JINJA2CPP_CXX_STANDARD}
    CXX_STANDARD_REQUIRED ON
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)
# Nor do the static libraries in it (the library, fmt) export their symbols, so another extension
# carrying Jinja2C++ or fmt cannot clash with this one
if (UNIX AND NOT APPLE)
    target_link_options(_jinja2cpp PRIVATE "LINKER:--exclude-libs,ALL")
endif ()

# A build tree runs the tests from python/: the module goes next to the package's sources
add_custom_command(TARGET _jinja2cpp POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:_jinja2cpp>" "${CMAKE_CURRENT_BINARY_DIR}/jinja2cpp/$<TARGET_FILE_NAME:_jinja2cpp>"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/jinja2cpp" "${CMAKE_CURRENT_BINARY_DIR}/jinja2cpp")

# scikit-build-core installs into the wheel; the package's Python sources come from
# pyproject.toml's wheel.packages
if (DEFINED SKBUILD)
    install(TARGETS _jinja2cpp LIBRARY DESTINATION jinja2cpp)
endif ()
