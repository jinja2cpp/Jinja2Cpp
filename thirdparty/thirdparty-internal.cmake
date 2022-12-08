message(STATUS "'internal' dependencies mode selected for Jinja2Cpp. All dependencies are used as submodules")

include (./thirdparty/internal_deps.cmake)

#update_submodule(boost)
set(BOOST_ENABLE_CMAKE ON)
list(APPEND BOOST_INCLUDE_LIBRARIES
    algorithm
    assert
    atomic
    filesystem
    lexical_cast
    optional
    variant
    json
)

include(FetchContent)
FetchContent_Declare(
  Boost
  GIT_REPOSITORY https://github.com/boostorg/boost.git
  GIT_TAG boost-1.80.0
)
FetchContent_MakeAvailable(Boost)


#set(BOOST_INCLUDE_LIBRARIES ${BOOST_INCLUDE_LIBRARIES} CACHE INTERNAL "")
#add_subdirectory(thirdparty/boost)

#include(FetchContent)
#
## BOOST
#
#set(BOOST_REQD_SUBMODULES
#    "tools/cmake;"
#    "libs/assert;libs/exception;libs/throw_exception;libs/static_assert;"
#    "libs/bind;libs/function_types;libs/function;"
#    "libs/chrono;libs/date_time;"
#    "libs/concept_check;"
#    "libs/config;libs/container;libs/container_hash;"
#    "libs/iterator;libs/utility;libs/type_traits;libs/algorithm;;libs/conversion;libs/numeric/conversion;libs/regex;libs/unordered;libs/tokenizer;libs/move;libs/ratio;libs/lexical_cast;"
#    "libs/tuple;libs/variant2;libs/typeof;libs/detail;libs/array;libs/type_index;libs/range;libs/optional;libs/smart_ptr;libs/integer;libs/rational;"
#    "libs/intrusive;libs/io;"
#    "libs/core;libs/align;libs/predef;libs/preprocessor;libs/system;libs/winapi;libs/atomic;"
#    "libs/mpl;libs/fusion;libs/mp11;"
#    "libs/thread"
#)
#
#Set(FETCHCONTENT_QUIET FALSE)
#FetchContent_Declare(
#    boost
#    GIT_REPOSITORY "https://github.com/boostorg/boost.git"
#    GIT_TAG master
#    GIT_SUBMODULES ${BOOST_REQD_SUBMODULES}
#    GIT_PROGRESS TRUE
#    CONFIGURE_COMMAND ""  # tell CMake it's not a cmake project
#)
#
#FetchContent_MakeAvailable(boost)


if(NOT MSVC)
    # Enable -Werror and -Wall on jinja2cpp target, ignoring warning errors from thirdparty libs
    include(CheckCXXCompilerFlag)
    check_cxx_compiler_flag(-Wno-error=parentheses COMPILER_HAS_WNO_ERROR_PARENTHESES_FLAG)
    check_cxx_compiler_flag(-Wno-error=deprecated-declarations COMPILER_HAS_WNO_ERROR_DEPRECATED_DECLARATIONS_FLAG)
    check_cxx_compiler_flag(-Wno-error=maybe-uninitialized COMPILER_HAS_WNO_ERROR_MAYBE_UNINITIALIZED_FLAG)

    if(COMPILER_HAS_WNO_ERROR_PARENTHESES_FLAG)
        target_compile_options(boost_assert INTERFACE -Wno-error=parentheses)
    endif()
    if(COMPILER_HAS_WNO_ERROR_DEPRECATED_DECLARATIONS_FLAG)
        target_compile_options(boost_filesystem PRIVATE -Wno-error=deprecated-declarations)
    endif()
    if(COMPILER_HAS_WNO_ERROR_MAYBE_UNINITIALIZED_FLAG)
        target_compile_options(boost_variant INTERFACE -Wno-error=maybe-uninitialized)
    endif()
        else ()
endif()

# install(TARGETS boost_filesystem boost_algorithm boost_variant boost_optional
#        EXPORT InstallTargets
#        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
#        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
#        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/static
#        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/boost
#        )
