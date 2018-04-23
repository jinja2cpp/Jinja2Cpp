project(Jinja2CppLight)

include(collect_sources)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${Boost_INCLUDE_DIRS}
    ${GTEST_INCLUDE_DIRS}
    )

CollectSources(Sources Headers ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/src)
CollectSources(PublicSources PublicHeaders ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/include)
CollectSources(TestSources TestHeaders ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/test)

add_library(Jinja2Cpp STATIC
    ${Sources}
    ${Headers}
    ${PublicHeaders}
)

target_include_directories(Jinja2Cpp INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(jinja2cpp_unittests ${TestSources} ${TestHeaders})
target_link_libraries(jinja2cpp_unittests ${GTEST_BOTH_LIBRARIES})
target_link_libraries(jinja2cpp_unittests Jinja2Cpp)
