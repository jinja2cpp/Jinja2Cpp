message(STATUS "'conan-build' dependencies mode selected for Jinja2Cpp. All dependencies are taken as a conan packages")

find_package(expected-lite)
find_package(variant-lite)
find_package(optional-lite)
find_package(string-view-lite)
find_package(boost)
find_package(fmt)

update_submodule(robin-hood-hashing)
add_library(rh_lib INTERFACE)
target_include_directories(rh_lib
        INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/robin-hood-hashing/src/include>
        $<INSTALL_INTERFACE:include>)

set (JINJA2_PRIVATE_LIBS_INT boost::boost fmt::fmt rh_lib)
set (JINJA2_PUBLIC_LIBS_INT expected-lite::expected-lite variant-lite::variant-lite optional-lite::optional-lite string-view-lite::string-view-lite)

install(TARGETS rh_lib
        EXPORT InstallTargets
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/static
        )

