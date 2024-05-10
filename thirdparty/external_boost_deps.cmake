if (JINJA2CPP_VERBOSE)
	set (FIND_BOOST_PACKAGE_QUIET)
else ()
	set (FIND_BOOST_PACKAGE_QUIET QUIET)
endif ()

if (MSVC)
	if (NOT DEFINED Boost_USE_STATIC_LIBS)
		if (THIRDPARTY_RUNTIME_TYPE STREQUAL "/MD" OR THIRDPARTY_RUNTIME_TYPE STREQUAL "/MDd")
			set (Boost_USE_STATIC_LIBS OFF)
			set (Boost_USE_STATIC_RUNTIME OFF)
		else ()
			set (Boost_USE_STATIC_LIBS ON)
			set (Boost_USE_STATIC_RUNTIME ON)
		endif ()
		if (JINJA2CPP_VERBOSE)
			message (STATUS ">>>DEBUG<<< Boost_USE_STATIC_RUNTIME = ${Boost_USE_STATIC_RUNTIME}")
		endif ()
	endif ()
endif ()

find_package(boost_algorithm  ${FIND_BOOST_PACKAGE_QUIET})
find_package(boost_filesystem ${FIND_BOOST_PACKAGE_QUIET})
find_package(boost_json       ${FIND_BOOST_PACKAGE_QUIET})
find_package(boost_optional   ${FIND_BOOST_PACKAGE_QUIET})
find_package(boost_variant    ${FIND_BOOST_PACKAGE_QUIET})
find_package(boost_regex      ${FIND_BOOST_PACKAGE_QUIET})

if (boost_algorithm_FOUND AND
   boost_filesystem_FOUND AND
   boost_json_FOUND AND
   boost_optional_FOUND AND
   boost_variant_FOUND AND boost_regex_FOUND)
   imported_target_alias(boost_algorithm  ALIAS boost_algorithm::boost_algorithm)
   imported_target_alias(boost_filesystem ALIAS boost_filesystem::boost_filesystem)
   imported_target_alias(boost_json       ALIAS boost_json::boost_json)
   imported_target_alias(boost_optional   ALIAS boost_optional::boost_optional)
   imported_target_alias(boost_variant    ALIAS boost_variant::boost_variant)
   imported_target_alias(boost_regex      ALIAS boost_regex::boost_regex)
else ()
    find_package(Boost COMPONENTS system filesystem json regex ${FIND_BOOST_PACKAGE_QUIET} REQUIRED)

    if (Boost_FOUND)
        imported_target_alias(boost_algorithm  ALIAS Boost::boost)
        imported_target_alias(boost_filesystem ALIAS Boost::filesystem)
        imported_target_alias(boost_json       ALIAS Boost::json)
        imported_target_alias(boost_optional   ALIAS Boost::boost)
        imported_target_alias(boost_variant    ALIAS Boost::boost)
        imported_target_alias(boost_regex      ALIAS Boost::regex)
    endif ()
endif ()

set(_additional_boost_install_targets)
if ("${JINJA2CPP_USE_REGEX}" STREQUAL "boost")
    set(_additional_boost_install_targets "boost_regex")
endif()

if(JINJA2CPP_INSTALL)
    install(TARGETS boost_algorithm boost_filesystem boost_json boost_optional boost_variant ${_additional_boost_install_targets}
            EXPORT InstallTargets
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/static
            PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/boost
            )
endif()
