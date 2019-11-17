message(STATUS "'conan-build' dependencies mode selected for Jinja2Cpp. All dependencies are taken as a conan packages")

find_package(expected-lite)
find_package(variant-lite)
find_package(optional-lite)
find_package(string-view-lite)

# Temporary workaround for conan.io issue with boost package naming
if (WIN32 OR WIN64)
    find_package(boost)
    set(CONAN_BOOST_PACKAGE_NAME boost::boost)
else ()
    find_package(Boost)
    set(CONAN_BOOST_PACKAGE_NAME Boost::Boost)
endif ()

find_package(fmt)
find_package(rapidjson)

set(JINJA2_PRIVATE_LIBS_INT CONAN_BOOST_PACKAGE_NAME fmt::fmt rapidjson::rapidjson)
set(JINJA2_PUBLIC_LIBS_INT expected-lite::expected-lite variant-lite::variant-lite optional-lite::optional-lite string-view-lite::string-view-lite)
