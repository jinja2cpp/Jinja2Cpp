#ifndef JINJA2CPP_CONFIG_H
#define JINJA2CPP_CONFIG_H

// JINJA2CPP_VERSION (major * 10000 + minor * 100 + patch), its components and
// JINJA2CPP_VERSION_STRING; generated from project(VERSION) by CMake
#include "jinja2cpp/version.h" // IWYU pragma: export

#ifdef _WIN32
#define JINJA2_DECLSPEC(S) __declspec(S)
#else
#define JINJA2_DECLSPEC(S)
#endif

// MSVC warns (C4251) about exported classes holding standard library members. The
// library's headers silence it between these two macros only, so the warning stays
// on for the rest of the user's translation unit.
#ifdef _MSC_VER
#define JINJA2CPP_SUPPRESS_DLL_INTERFACE_WARNINGS_BEGIN __pragma(warning(push)) __pragma(warning(disable : 4251))
#define JINJA2CPP_SUPPRESS_DLL_INTERFACE_WARNINGS_END __pragma(warning(pop))
#else
#define JINJA2CPP_SUPPRESS_DLL_INTERFACE_WARNINGS_BEGIN
#define JINJA2CPP_SUPPRESS_DLL_INTERFACE_WARNINGS_END
#endif

// Marks a 1.x name kept as a forward to its 2.0 replacement, removed in 3.0. The message
// starts with "jinja2cpp-2: " and names the replacement, which the migration script reads.
// Define JINJA2CPP_NO_DEPRECATION_WARNINGS to upgrade now and migrate later.
#ifdef JINJA2CPP_NO_DEPRECATION_WARNINGS
#define JINJA2CPP_DEPRECATED(msg)
#else
#define JINJA2CPP_DEPRECATED(msg) [[deprecated(msg)]]
#endif

#ifdef JINJA2CPP_BUILD_AS_SHARED
#define JINJA2CPP_EXPORT JINJA2_DECLSPEC(dllexport)
#define JINJA2CPP_SHARED_LIB
#elif JINJA2CPP_LINK_AS_SHARED
#define JINJA2CPP_EXPORT JINJA2_DECLSPEC(dllimport)
#define JINJA2CPP_SHARED_LIB
#else
#define JINJA2CPP_EXPORT
#define JINJA2CPP_STATIC_LIB
#endif

#endif // JINJA2CPP_CONFIG_H
