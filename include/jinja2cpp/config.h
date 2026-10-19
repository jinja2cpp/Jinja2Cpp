#ifndef JINJA2CPP_CONFIG_H
#define JINJA2CPP_CONFIG_H

// JINJA2CPP_VERSION (major * 10000 + minor * 100 + patch), its components and
// JINJA2CPP_VERSION_STRING; generated from project(VERSION) by CMake
#include "jinja2cpp/version.h" // IWYU pragma: export

// Jinja2C++ reports errors through exceptions internally and in a few documented calls
// (docs/api-2.0.md, 2.1): building without them is not supported
#if !defined(__cpp_exceptions) && !defined(__EXCEPTIONS) && !defined(_CPPUNWIND)
#error "Jinja2C++ requires C++ exceptions"
#endif

// MSVC warns (C4251, C4275) about exported classes holding or deriving from standard library types. The
// library's headers silence it between these two macros only, so the warning stays
// on for the rest of the user's translation unit.
#ifdef _MSC_VER
#define JINJA2CPP_SUPPRESS_DLL_INTERFACE_WARNINGS_BEGIN __pragma(warning(push)) __pragma(warning(disable : 4251 4275))
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

// The shared library exports only what JINJA2CPP_EXPORT marks: on Windows by
// dllexport/dllimport, elsewhere by default visibility, the library being compiled
// with -fvisibility=hidden (CMakeLists.txt). The same markings serve both, so the
// Windows build checks they are complete.
#ifdef JINJA2CPP_BUILD_AS_SHARED
#ifdef _WIN32
#define JINJA2CPP_EXPORT __declspec(dllexport)
#else
#define JINJA2CPP_EXPORT __attribute__((visibility("default")))
#endif
#define JINJA2CPP_SHARED_LIB
#elif JINJA2CPP_LINK_AS_SHARED
#ifdef _WIN32
#define JINJA2CPP_EXPORT __declspec(dllimport)
#else
#define JINJA2CPP_EXPORT __attribute__((visibility("default")))
#endif
#define JINJA2CPP_SHARED_LIB
#else
#define JINJA2CPP_EXPORT
#define JINJA2CPP_STATIC_LIB
#endif

#endif // JINJA2CPP_CONFIG_H
