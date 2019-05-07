find_package(expected-lite)
find_package(variant-lite)
find_package(optional-lite)
find_package(value-ptr-lite)
find_package(boost)


set (CONAN_PKG_PREFIX )

if (JINJA2CPP_USE_CONANPKG_PREFIX)
    message (STATUS "########--------##########!!!!!!!!!!!")
    set (CONAN_PKG_PREFIX CONAN_PKG::)
else ()    
    message (STATUS "########--------##########???????????")
endif ()


message (STATUS "JINJA2CPP_USE_CONANPKG_PREFIX = ${JINJA2CPP_USE_CONANPKG_PREFIX}, CONAN_PKG_PREFIX = ${CONAN_PKG_PREFIX}")

set (JINJA2_PRIVATE_LIBS_INT boost::boost)
set (JINJA2_PUBLIC_LIBS_INT expected-lite::expected-lite variant-lite::variant-lite value-ptr-lite::value-ptr-lite optional-lite::optional-lite)
