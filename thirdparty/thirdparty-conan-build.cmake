# find_package(expected-lite)
# find_package(variant-lite)
# find_package(optional-lite)
# find_package(value-ptr-lite)
# find_package(boost)


set (CONAN_PKG_PREFIX )

if (JINJA2CPP_USE_CONANPKG_PREFIX)
    message (STATUS "########--------##########!!!!!!!!!!!")
    set (CONAN_PKG_PREFIX CONAN_PKG::)
else ()    
    message (STATUS "########--------##########???????????")
endif ()


message (STATUS "JINJA2CPP_USE_CONANPKG_PREFIX = ${JINJA2CPP_USE_CONANPKG_PREFIX}, CONAN_PKG_PREFIX = ${CONAN_PKG_PREFIX}")

set (JINJA2_PRIVATE_LIBS_INT ${CONAN_PKG_PREFIX}boost)
set (JINJA2_PUBLIC_LIBS_INT ${CONAN_PKG_PREFIX}expected-lite ${CONAN_PKG_PREFIX}variant-lite ${CONAN_PKG_PREFIX}value-ptr-lite ${CONAN_PKG_PREFIX}optional-lite)
