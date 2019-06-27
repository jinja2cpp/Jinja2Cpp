update_submodule(nonstd/expected-lite)
add_subdirectory(thirdparty/nonstd/expected-lite EXCLUDE_FROM_ALL)

update_submodule(nonstd/variant-lite)
add_subdirectory(thirdparty/nonstd/variant-lite EXCLUDE_FROM_ALL)
add_library(variant-lite ALIAS variant-lite)

update_submodule(nonstd/optional-lite)
add_subdirectory(thirdparty/nonstd/optional-lite EXCLUDE_FROM_ALL)
add_library(optional-lite ALIAS optional-lite)

update_submodule(nonstd/string-view-lite)
add_subdirectory(thirdparty/nonstd/string-view-lite EXCLUDE_FROM_ALL)
add_library(string-view-lite ALIAS string-view-lite)

update_submodule(fmtlib)
set(FMT_INSTALL ON)
add_subdirectory(thirdparty/fmtlib EXCLUDE_FROM_ALL)
add_library(fmt ALIAS fmt-header-only)

install (FILES
        thirdparty/nonstd/expected-lite/include/nonstd/expected.hpp
        thirdparty/nonstd/variant-lite/include/nonstd/variant.hpp
        thirdparty/nonstd/optional-lite/include/nonstd/optional.hpp
        thirdparty/nonstd/string-view-lite/include/nonstd/string_view.hpp
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/nonstd)
    