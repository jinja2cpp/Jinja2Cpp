function (_Jinja2Cpp_Find_Library varName)
    find_library(${varName}
        NAMES ${ARGN}
        HINTS
            ENV JINJA2CPP_ROOT
            ${JINJA2CPP_INSTALL_DIR}
        PATH_SUFFIXES lib/static lib64/static
    )
    mark_as_advanced(${varName})
endfunction ()

find_path(JINJA2CPP_INCLUDE_DIR jinja2cpp/template.h
    HINTS
        $ENV{JINJA2CPP_ROOT}/include
        ${JINJA2CPP_INSTALL_DIR}/include
)
mark_as_advanced(${JINJA2CPP_INCLUDE_DIR})

_Jinja2Cpp_Find_Library(JINJA2CPP_LIBRARY jinja2cpp)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(JINJA2CPP DEFAULT_MSG JINJA2CPP_LIBRARY JINJA2CPP_INCLUDE_DIR)

if (JINJA2CPP_FOUND)
    if (NOT TARGET Jinja2Cpp)
        add_library(Jinja2Cpp UNKNOWN IMPORTED)
        set_target_properties(Jinja2Cpp PROPERTIES
                        INTERFACE_INCLUDE_DIRECTORIES "${JINJA2CPP_INCLUDE_DIR}")
        set_target_properties(Jinja2Cpp PROPERTIES
            IMPORTED_LINK_INTERFACE_LANGUAGES "CXX"
            IMPORTED_LOCATION "${JINJA2CPP_LIBRARY}")
    endif ()
endif ()
