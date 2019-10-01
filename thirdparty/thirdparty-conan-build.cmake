message(STATUS "'conan-build' dependencies mode selected for Jinja2Cpp. All dependencies are taken as a conan packages")

find_package(expected-lite)
find_package(variant-lite)
find_package(optional-lite)
find_package(string-view-lite)
find_package(boost)
find_package(fmt)

set(JINJA2_PRIVATE_LIBS_INT boost::boost fmt::fmt)
set (JINJA2_PUBLIC_LIBS_INT expected-lite::expected-lite variant-lite::variant-lite optional-lite::optional-lite string-view-lite::string-view-lite)
