# find_package(expected-lite)
# find_package(variant-lite)
# find_package(optional-lite)
# find_package(value-ptr-lite)
# find_package(boost)

set (JINJA2_PRIVATE_LIBS_INT CONAN_PKG::boost PARENT_SCOPE)
set (JINJA2_PUBLIC_LIBS_INT CONAN_PKG::expected-lite CONAN_PKG::variant-lite CONAN_PKG::value-ptr-lite CONAN_PKG::optional-lite PARENT_SCOPE)
