project (nonstd)

add_library(nonstd INTERFACE)
add_library(ThirdParty::nonstd ALIAS nonstd)
target_include_directories(nonstd SYSTEM
    INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/expected-light/include
    )

target_sources(nonstd
    INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/expected-light/include/nonstd/expected.hpp
    )




