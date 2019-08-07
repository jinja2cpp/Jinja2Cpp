message(STATUS "'external' dependencies mode selected for Jinja2Cpp. All dependencies are treated as external")

include(FindPackageHandleStandardArgs)

macro (FindHeaderOnlyLib HDR_PATH TARGET_NAME)
    set (TARGET_NAME_INC_DIR ${TARGET_NAME}_INCLUDE_DIR)
    find_path(${TARGET_NAME_INC_DIR} NAMES ${HDR_PATH})
    mark_as_advanced(${TARGET_NAME_INC_DIR})

    include(FindPackageHandleStandardArgs)
    FIND_PACKAGE_HANDLE_STANDARD_ARGS(${TARGET_NAME} DEFAULT_MSG ${TARGET_NAME}_INCLUDE_DIR)

    if (${TARGET_NAME}_FOUND)
        if (NOT TARGET ${TARGET_NAME})
            add_library(${TARGET_NAME} INTERFACE)
            set_target_properties(${TARGET_NAME} PROPERTIES
                            INTERFACE_INCLUDE_DIRECTORIES "${${TARGET_NAME}_INCLUDE_DIR}")
        endif ()
    endif ()
endmacro ()

macro (find_hdr_package PKG_NAME HDR_PATH)
    find_package(${PKG_NAME})
    if(NOT ${PKG_NAME}_FOUND)
        FindHeaderOnlyLib(${HDR_PATH} ${PKG_NAME})
    endif ()

    if(${PKG_NAME}_FOUND)
        if (NOT TARGET ${PKG_NAME})
            imported_target_alias(${PKG_NAME} ALIAS "${PKG_NAME}::${PKG_NAME}")
        endif ()
    else()
        message(FATAL_ERROR "${PKG_NAME} not found!")
    endif()
endmacro ()

find_hdr_package(expected-lite nonstd/expected.hpp)
find_hdr_package(variant-lite nonstd/variant.hpp)
find_hdr_package(optional-lite nonstd/optional.hpp)
find_hdr_package(string-view-lite nonstd/string_view.hpp)
find_hdr_package(fmt-header-only fmt/format.h)
find_hdr_package(rh_lib robin_hood.h)

if (TARGET fmt-header-only)
    target_compile_definitions(fmt-header-only INTERFACE FMT_HEADER_ONLY=1)
    add_library(fmt ALIAS fmt-header-only)
endif ()

install(TARGETS expected-lite variant-lite optional-lite string-view-lite
        EXPORT InstallTargets
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/static
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/nonstd
        )

install(TARGETS fmt-header-only rh_lib
        EXPORT InstallTargets
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/static
        )

include (./thirdparty/external_boost_deps.cmake)
