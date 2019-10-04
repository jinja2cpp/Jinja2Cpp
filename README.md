<div align="center"><img width="200" src="https://avatars0.githubusercontent.com/u/49841676?s=200&v=4"></div>

# Jinja2С++

[![Language](https://img.shields.io/badge/language-C++-blue.svg)](https://isocpp.org/)
[![Standard](https://img.shields.io/badge/c%2B%2B-14-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B#Standardization)
[![Build Status](https://travis-ci.org/jinja2cpp/Jinja2Cpp.svg?branch=master)](https://travis-ci.org/jinja2cpp/Jinja2Cpp)
[![Build status](https://ci.appveyor.com/api/projects/status/vu59lw4r67n8jdxl/branch/master?svg=true)](https://ci.appveyor.com/project/flexferrum/jinja2cpp-n5hjm/branch/master)
[![Coverage Status](https://codecov.io/gh/jinja2cpp/Jinja2Cpp/branch/master/graph/badge.svg)](https://codecov.io/gh/jinja2cpp/Jinja2Cpp)
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/ff01fa4410ac417f8192dce78e919ece)](https://www.codacy.com/app/flexferrum/Jinja2Cpp_2?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=jinja2cpp/Jinja2Cpp&amp;utm_campaign=Badge_Grade)
[![Github Releases](https://img.shields.io/github/release/jinja2cpp/Jinja2Cpp/all.svg)](https://github.com/jinja2cpp/Jinja2Cpp/releases)
[![Github Issues](https://img.shields.io/github/issues/jinja2cpp/Jinja2Cpp.svg)](http://github.com/jinja2cpp/Jinja2Cpp/issues)
[![GitHub License](https://img.shields.io/badge/license-Mozilla-blue.svg)](https://raw.githubusercontent.com/jinja2cpp/Jinja2Cpp/master/LICENSE)
[![conan.io](https://api.bintray.com/packages/flexferrum/conan-packages/jinja2cpp:flexferrum/images/download.svg?version=1.0.0:testing) ](https://bintray.com/flexferrum/conan-packages/jinja2cpp:flexferrum/1.0.0:testing/link)
[![Gitter Chat](https://badges.gitter.im/Join%20Chat.svg)](https://gitter.im/Jinja2Cpp/Lobby)

C++ implementation of Jinja2 Python template engine. This library brings support of powerful Jinja2 template features into the C++ world, reports  dynamic html pages and source code generation.  

## Introduction

Main features of Jinja2C++:
-  Easy-to-use public interface. Just load templates and render them.
-  Conformance to [Jinja2 specification](http://jinja.pocoo.org/docs/2.10/)
-  Full support of narrow- and wide-character strings both for templates and parameters.
-  Built-in reflection for the common C++ types, nlohmann and rapid JSON libraries.
-  Powerful full-featured Jinja2 expressions with filtering (via '|' operator) and 'if'-expressions.
-  Control statements (`set`, `for`, `if`, `filter`, `do`, `with`).
-  Templates extention, including and importing
-  Macros
-  Rich error reporting.
-  Shared template enironment with templates cache support

For instance, this simple code:

```c++
#include <jinja2cpp/template.h>

std::string source = R"(
{{ ("Hello", 'world') | join }}!!!
{{ ("Hello", 'world') | join(', ') }}!!!
{{ ("Hello", 'world') | join(d = '; ') }}!!!
{{ ("Hello", 'world') | join(d = '; ') | lower }}!!!
)";

Template tpl;
tpl.Load(source);

std::string result = tpl.RenderAsString({}).value();
```

produces the result string:

```
Helloworld!!!
Hello, world!!!
Hello; world!!!
hello; world!!!
```

## Getting started

In order to use Jinja2C++ in your project you have to:
* Clone the Jinja2C++ repository
* Build it according with the [instructions](https://jinja2cpp.dev/docs/build_and_install.html)
* Link to your project.

Usage of Jinja2C++ in the code is pretty simple:
1.  Declare the jinja2::Template object:

```c++
jinja2::Template tpl;
```

2.  Populate it with template:

```c++
tpl.Load("{{ 'Hello World' }}!!!");
```

3.  Render the template:

```c++
std::cout << tpl.RenderAsString({}).value() << std::endl;
```

and get:

`
Hello World!!!
`

That's all!

More detailed examples and features describtion can be found in the documentation: [https://jinja2cpp.dev/docs/usage](https://jinja2cpp.dev/docs/usage)

## Current Jinja2 support
Currently, Jinja2C++ supports the limited number of Jinja2 features. By the way, Jinja2C++ is planned to be full [jinja2 specification](http://jinja.pocoo.org/docs/2.10/templates/)-conformant. The current support is limited to:
-  expressions. You can use almost every style of expressions: simple, filtered, conditional, and so on.
-  big number of filters (**sort, default, first, last, length, max, min, reverse, unique, sum, attr, map, reject, rejectattr, select, selectattr, pprint, dictsort, abs, float, int, list, round, random, trim, title, upper, wordcount, replace, truncate, groupby, urlencode, capitalize, escape**)
-  big number of testers (**eq, defined, ge, gt, iterable, le, lt, mapping, ne, number, sequence, string, undefined, in, even, odd, lower, upper**)
-  limited number of functions (**range**, **loop.cycle**)
-  'if' statement (with 'elif' and 'else' branches)
-  'for' statement (with 'else' branch and 'if' part support)
-  'include' statement
-  'import'/'from' statements
-  'set' statement (both line and block)
-  'filter' statement
-  'extends'/'block' statements
-  'macro'/'call' statements
-  'with' statement
-  'do' extension statement
-  recursive loops
-  space control

Full information about Jinja2 specification support and compatibility table can be found here: [https://jinja2cpp.dev/docs/j2_compatibility.html](https://jinja2cpp.dev/docs/j2_compatibility.html).

## Supported compilers
Compilation of Jinja2C++ tested on the following compilers (with C++14 and C++17 enabled features):
-  Linux gcc 5.5 - 9.0
-  Linux clang 5.0 - 9
-  MacOS X-Code 9
-  MacOS X-Code 10
-  MacOS X-Code 11 (C++14 in default build, C++17 with externally-provided boost)
-  Microsoft Visual Studio 2015 - 2019 x86, x64
-  MinGW gcc compiler 7.3
-  MinGW gcc compiler 8.1

**Note:** Support of gcc version >= 9.x or clang version >= 8.0 depends on version of Boost library provided. 

## Build and install
Jinja2C++ has several external dependencies:
-  `boost` library (at least version 1.65) 
-  `nonstd::expected-lite` [https://github.com/martinmoene/expected-lite](https://github.com/martinmoene/expected-lite)
-  `nonstd::variant-lite` [https://github.com/martinmoene/variant-lite](https://github.com/martinmoene/variant-lite)
-  `nonstd::value-ptr-lite` [https://github.com/martinmoene/value-ptr-lite](https://github.com/martinmoene/value-ptr-lite)
-  `nonstd::optional-lite` [https://github.com/martinmoene/optional-lite](https://github.com/martinmoene/optional-lite)
-  `nonstd::string-view-lite` [https://github.com/martinmoene/string-view-lite](https://github.com/martinmoene/string-view-lite)
-  `fmtlib::fmt` [https://github.com/fmtlib/fmt](https://github.com/fmtlib/fmt)
-  `robin-hood-hashing` [https://github.com/martinus/robin-hood-hashing](https://github.com/martinus/robin-hood-hashing)

In simpliest case to compile Jinja2C++ you need:

1.  Install CMake build system (at least version 3.0)
2.  Clone jinja2cpp repository and update submodules:

```
> git clone https://github.com/flexferrum/Jinja2Cpp.git
> git submodule -q update --init
```

3.  Create build directory:

```
> cd Jinja2Cpp
> mkdir build
```

4.  Run CMake and build the library:

```
> cd build
> cmake .. -DCMAKE_INSTALL_PREFIX=<path to install folder>
> cmake --build . --target all
```
"Path to install folder" here is a path to the folder where you want to install Jinja2C++ lib.

5. Install library:

```
> cmake --build . --target install
```

In this case Jinja2C++ will be built with internally-shipped dependencies and install them respectively. But Jinja2C++ supports build with externally-provided deps. Different Jinja2C++ usage scenarios can be found in this repository: https://github.com/jinja2cpp/examples-build

### Usage with conan.io dependency manager
Jinja2C++ can be used as conan.io package. In this case you should do the following steps:

1. Install conan.io according to the documentation ( https://docs.conan.io/en/latest/installation.html )
2. Register the following remote conan.io repositories:
    * https://api.bintray.com/conan/martinmoene/nonstd-lite
    * https://api.bintray.com/conan/bincrafters/public-conan
    * https://api.bintray.com/conan/flexferrum/conan-packages

The sample command is: `conan remote add martin https://api.bintray.com/conan/martinmoene/nonstd-lite`

3. Add reference to Jinja2C++ package (`jinja2cpp/1.0.0@flexferrum/testing`) to your conanfile.txt, conanfile.py or CMakeLists.txt. For instance, with usage of `conan-cmake` integration it could be written this way:

```cmake
include (../../cmake/conan.cmake)
if (NOT MSVC)
    set (CONAN_SETTINGS SETTINGS compiler.libcxx=libstdc++11)
endif ()

conan_cmake_run(REQUIRES 
                    jinja2cpp/1.0.0@flexferrum/testing
                    gtest/1.7.0@bincrafters/stable
                BASIC_SETUP
                ${CONAN_SETTINGS}
                OPTIONS 
                    jinja2cpp:shared=False
                    gtest:shared=False
                BUILD missing)
                
set (TARGET_NAME jinja2cpp_build_test)

add_executable (${TARGET_NAME} main.cpp)

target_link_libraries (${TARGET_NAME} ${CONAN_LIBS})
set_target_properties (${TARGET_NAME} PROPERTIES
            CXX_STANDARD 14
            CXX_STANDARD_REQUIRED ON)

```


### Additional CMake build flags
You can define (via -D command line CMake option) the following build flags:

-  **JINJA2CPP_BUILD_TESTS** (default TRUE) - to build or not to Jinja2C++ tests.
-  **JINJA2CPP_STRICT_WARNINGS** (default TRUE) - Enable strict mode compile-warnings(-Wall -Werror and etc).
-  **JINJA2CPP_MSVC_RUNTIME_TYPE** (default /MD) - MSVC runtime type to link with (if you use Microsoft Visual Studio compiler).
-  **JINJA2CPP_DEPS_MODE** (default "internal") - modes for dependencies handling. Following values possible:
    -  `internal` In this mode Jinja2C++ build script uses dependencies (include `boost`) shipped as subprojects. Nothing needs to be provided externally.
    -  `external-boost` In this mode Jinja2C++ build script uses only `boost` as externally-provided dependency. All other dependencies taken from subprojects.
    -  `external` In this mode all dependencies should be provided externally. Paths to `boost`, `nonstd-*` libs etc. should be specified via standard CMake variables (like `CMAKE_PREFIX_PATH` or libname_DIR)
    -  `conan-build` Special mode for building Jinja2C++ via conan recipe.


### Build with C++17 standard enabled
In case of C++17 standard enabled for your project you should define `variant_CONFIG_SELECT_VARIANT=variant_VARIANT_NONSTD nssv_CONFIG_SELECT_STRING_VIEW=nssv_STRING_VIEW_NONSTD optional_CONFIG_SELECT_OPTIONAL=optional_OPTIONAL_NONSTD` macros in the build settings.

## Acknowledgments
Thanks to **@manu343726** for CMake scripts improvement, bugs hunting and fixing and conan.io packaging.

Thanks to **@martinmoene** for the perfectly implemented xxx-lite libraries.

Thanks to **@vitaut** for the amazing text formatting library

Thanks to **@martinus** for the fast hash maps implementation

Thanks to **@palchukovsky** for the great contribution into codebase

Thanks to **@rmorozov** for stanitized builds setup


## Changelog

### Version 1.0.0
#### Changes and improvements
- `default` attribute added to the `map` filter (#48)
- escape sequences support added to the string literals (#49)
- arbitrary ranges, generated sequences, input iterators etc. now can be used with `GenericList` type (#66)
- nonstd::string_view is now one of the possible types for the `Value`
- `filter` tag support added to the template parser (#44)
- `escape` filter support added to the template parser (#140)
- `capitalize` filter support added to the template parser (#137)
- multiline version of `set` tag added to the parser (#45)
- added built-in reflection for nlohmann json and rapid json libraries (#78)
- `loop.depth` and `loop.depth0` variables support added
- {fmt} is now used as a formatting library instead of iostreams
- robin hood hash maps is now used for internal value storage
- rendering performance improvements
- template cache implemented in `TemplateEnv`
- user-defined callables now can accept global context via `*context` special param
- MinGW, clang >= 7.0, XCode >= 9, gcc >= 7.0 are now officially supported as a target compilers (#79)

#### Fixed bugs
- Fixed pipe (`|`) operator precedence (#47)
- Fixed bug in internal char <-> wchar_t converter on Windows
- Fixed crash in parsing `endblock` tag
- Fixed scope control for `include` and `for` tags
- Fixed bug with macros call within expression context

#### Breaking changes
- MSVC runtime type is now defines by `JINJA2CPP_MSVC_RUNTIME_TYPE` CMake variable

### Version 0.9.2
#### Major changes
- User-defined callables implemented. Now you can define your own callable objects, pass them as input parameters and use them inside templates as regular (global) functions, filters or testers. See details here: https://jinja2cpp.dev/docs/usage/ud_callables.html
- Now you can define global (template environment-wide) parameters which are accessible for all templates bound to this environment.
- `include`, `import` and `from` statements implemented. Now it's possible to include other templates and use macros from other templates.
- `with` statement implemented
- `do` statement implemented
- Sample build projects for various Jinja2C++ usage variants created: https://github.com/jinja2cpp/examples-build
- Documentation site created for Jinja2C++: https://jinja2cpp.dev/

#### Minor changes
- Render-time error handling added
- Dependency management mode added to the build script
- Fix bugs with error reporting during the parse time
- Upgraded versions of external dependencies

#### Breaking changes
- `RenderAsString` method now returns `nonstd::expected` instead of regular `std::string`
- Templates with `import`, `extends` and `include` generate errors if parsed without `TemplateEnv` set
- Release bundles (archives) are configured with `external` dependency management mode by default

### Version 0.9.1
-  `applymacro` filter added which allows to apply arbitrary macro as a filter
-  dependencies to boost removed from the public interface
-  CMake scripts improved
-  Various bugs fixed
-  Improve reflection
-  Warnings cleanup

### Version 0.9
-  Support of 'extents'/'block' statements
-  Support of 'macro'/'call' statements
-  Rich error reporting
-  Support for recursive loops
-  Support for space control before and after control blocks
-  Improve reflection

### Version 0.6
-  A lot of filters has been implemented. Full set of supported filters listed here: [https://github.com/flexferrum/Jinja2Cpp/issues/7](https://github.com/flexferrum/Jinja2Cpp/issues/7)
-  A lot of testers has been implemented. Full set of supported testers listed here: [https://github.com/flexferrum/Jinja2Cpp/issues/8](https://github.com/flexferrum/Jinja2Cpp/issues/8)
-  'Contatenate as string' operator ('~') has been implemented
-  For-loop with 'if' condition has been implemented
-  Fixed some bugs in parser
