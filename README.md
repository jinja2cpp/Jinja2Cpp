<div align="center"><img width="200" src="https://avatars0.githubusercontent.com/u/49841676?s=200&v=4"></div>

# Jinja2С++

[![Language](https://img.shields.io/badge/language-C++-blue.svg)](https://isocpp.org/)
[![Standard](https://img.shields.io/badge/c%2B%2B-14-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B#Standardization)
[![Build Status](https://travis-ci.org/jinja2cpp/Jinja2Cpp.svg?branch=master)](https://travis-ci.org/jinja2cpp/Jinja2Cpp)
[![Build status](https://ci.appveyor.com/api/projects/status/19v2k3bl63jxl42f/branch/master?svg=true)](https://ci.appveyor.com/project/flexferrum/Jinja2Cpp)
[![Coverage Status](https://codecov.io/gh/flexferrum/Jinja2Cpp/branch/master/graph/badge.svg)](https://codecov.io/gh/flexferrum/Jinja2Cpp)
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/d932d23e9288404ba44a1f500ab42778)](https://www.codacy.com/app/flexferrum/Jinja2Cpp?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=flexferrum/Jinja2Cpp&amp;utm_campaign=Badge_Grade)
[![Github Releases](https://img.shields.io/github/release/jinja2cpp/Jinja2Cpp/all.svg)](https://github.com/flexferrum/Jinja2Cpp/releases)
[![Github Issues](https://img.shields.io/github/issues/jinja2cpp/Jinja2Cpp.svg)](http://github.com/jinja2cpp/Jinja2Cpp/issues)
[![GitHub License](https://img.shields.io/badge/license-Mozilla-blue.svg)](https://raw.githubusercontent.com/jinja2cpp/Jinja2Cpp/master/LICENSE)
[ ![conan.io](https://api.bintray.com/packages/manu343726/conan-packages/jinja2cpp%3AManu343726/images/download.svg) ](https://bintray.com/manu343726/conan-packages/jinja2cpp%3AManu343726/_latestVersion)
[![Gitter Chat](https://badges.gitter.im/Join%20Chat.svg)](https://gitter.im/Jinja2Cpp/Lobby)

C++ implementation of big subset of Jinja2 template engine features. This library was inspired by [Jinja2CppLight](https://github.com/hughperkins/Jinja2CppLight) project and brings support of mostly all Jinja2 templates features into C++ world.

# Table of contents

<!-- START doctoc generated TOC please keep comment here to allow auto update -->
<!-- DON'T EDIT THIS SECTION, INSTEAD RE-RUN doctoc TO UPDATE -->


- [Introduction](#introduction)
- [Getting started](#getting-started)
  - [More complex example](#more-complex-example)
    - [The simplest case](#the-simplest-case)
    - [Reflection](#reflection)
    - ['set' statement](#set-statement)
    - ['extends' statement](#extends-statement)
    - [Macros](#macros)
    - [User-defined callables](#user-defined-callables)
    - [Error reporting](#error-reporting)
  - [Other features](#other-features)
- [Current Jinja2 support](#current-jinja2-support)
- [Supported compilers](#supported-compilers)
- [Build and install](#build-and-install)
  - [Additional CMake build flags](#additional-cmake-build-flags)
- [Link with you projects](#link-with-you-projects)
- [Acknowledgments](#acknowledgments)
- [Changelog](#changelog)
  - [Version 0.9.1](#version-091)
  - [Version 0.9](#version-09)
  - [Version 0.6](#version-06)

<!-- END doctoc generated TOC please keep comment here to allow auto update -->

# Introduction

Main features of Jinja2Cpp:
- Easy-to-use public interface. Just load templates and render them.
- Conformance to [Jinja2 specification](http://jinja.pocoo.org/docs/2.10/)
- Partial support for both narrow- and wide-character strings both for templates and parameters.
- Built-in reflection for C++ types.
- Powerful full-featured Jinja2 expressions with filtering (via '|' operator) and 'if'-expressions.
- Control statements (set, for, if).
- Templates extention.
- Macros
- Rich error reporting.

For instance, this simple code:

```c++
std::string source = R"(
{{ ("Hello", 'world') | join }}!!!
{{ ("Hello", 'world') | join(', ') }}!!!
{{ ("Hello", 'world') | join(d = '; ') }}!!!
{{ ("Hello", 'world') | join(d = '; ') | lower }}!!!
)";

Template tpl;
tpl.Load(source);

std::string result = tpl.RenderAsString(ValuesMap());
```

produces the result string:

```
Helloworld!!!
Hello, world!!!
Hello; world!!!
hello; world!!!
```

# Getting started

In order to use Jinja2Cpp in your project you have to:
* Clone the Jinja2Cpp repository
* Build it according with the instructions
* Link to your project.

Usage of Jinja2Cpp in the code is pretty simple:
1. Declare the jinja2::Template object:

```c++
jinja2::Template tpl;
```

2. Populate it with template:

```c++
tpl.Load("{{'Hello World' }}!!!");
```

3. Render the template:

```c++
std::cout << tpl.RenderAsString(jinja2::ValuesMap{}) << std::endl;
```

and get:

`
Hello World!!!
`

That's all!

## More complex example
Let's say you have the following enum:

```c++
enum Animals
{
    Dog,
    Cat,
    Monkey,
    Elephant
};
```

And you want to automatically produce string-to-enum and enum-to-string convertor. Like this:

```c++
inline const char* AnimalsToString(Animals e)
{
    switch (e)
    {
    case Dog:
        return "Dog";
    case Cat:
        return "Cat";
    case Monkey:
        return "Monkey";
    case Elephant:
        return "Elephant";
    }
    return "Unknown Item";
}
```

Of course, you can write this producer in the way like [this](https://github.com/flexferrum/autoprogrammer/blob/87a9dc8ff61c7bdd30fede249757b71984e4b954/src/generators/enum2string_generator.cpp#L140). It's too complicated for writing 'from scratch'. Actually, there is a better and simpler way.

### The simplest case

Firstly, you should define the simple jinja2 template (in the C++ manner):
```c++
std::string enum2StringConvertor = R"(
inline const char* {{enumName}}ToString({{enumName}} e)
{
    switch (e)
    {
{% for item in items %}
    case {{item}}:
        return "{{item}}";
{% endfor %}
    }
    return "Unknown Item";
})";
```
As you can see, this template is similar to the C++ sample code above, but some parts replaced by placeholders ("parameters"). These placeholders will be replaced with the actual text during template rendering process. In order to this happen, you should fill up the rendering parameters. This is a simple dictionary which maps the parameter name to the corresponding value:

```c++
jinja2::ValuesMap params {
    {"enumName", "Animals"},
    {"items", {"Dog", "Cat", "Monkey", "Elephant"}},
};
```
An finally, you can render this template with Jinja2Cpp library:

```c++
jinja2::Template tpl;
tpl.Load(enum2StringConvertor);
std::cout << tpl.RenderAsString(params);
```
And you will get on the console the conversion function mentioned above!

You can call 'Render' method many times, with different parameters set, from several threads. Everything will be fine: every time you call 'Render' you will get the result depended only on provided params. Also you can render some part of template many times (for different parameters) with help of 'for' loop and 'extend' statement (described below). It allows you to iterate through the list of items from the first to the last one and render the loop body for each item respectively. In this particular sample it allows to put as many 'case' blocks in conversion function as many items in the 'reflected' enum.

### Reflection
Let's imagine you don't want to fill the enum descriptor by hand, but want to fill it with help of some code parsing tool ([autoprogrammer](https://github.com/flexferrum/autoprogrammer) or [cppast](https://github.com/foonathan/cppast)). In this case you can define structure like this:

```c++
// Enum declaration description
struct EnumDescriptor
{
// Enumeration name
std::string enumName;
// Namespace scope prefix
std::string nsScope;
// Collection of enum items
std::vector<std::string> enumItems;
};
```
This structure holds the enum name, enum namespace scope prefix, and list of enum items (we need just names). Then, you can populate instances of this descriptor automatically using chosen tool (ex. here: [clang-based enum2string converter generator](https://github.com/flexferrum/flex_lib/blob/accu2017/tools/codegen/src/main.cpp) ). For our sample we can create the instance manually:
```c++
EnumDescriptor descr;
descr.enumName = "Animals";
descr.nsScope = "";
descr.enumItems = {"Dog", "Cat", "Monkey", "Elephant"};
```

And now you need to transfer data from this internal enum descriptor to Jinja2 value params map. Of course it's possible to do it by hands:
```c++
jinja2::ValuesMap params {
    {"enumName", descr.enumName},
    {"nsScope", descr.nsScope},
    {"items", {descr.enumItems[0], descr.enumItems[1], descr.enumItems[2], descr.enumItems[3]}},
};
```

But actually, with Jinja2Cpp you don't have to do it manually. Library can do it for you. You just need to define reflection rules. Something like this:

```c++
namespace jinja2
{
template<>
struct TypeReflection<EnumDescriptor> : TypeReflected<EnumDescriptor>
{
    static auto& GetAccessors()
    {
        static std::unordered_map<std::string, FieldAccessor> accessors = {
            {"name", [](const EnumDescriptor& obj) {return obj.name;}},
            {"nsScope", [](const EnumDescriptor& obj) { return obj.nsScope;}},
            {"items", [](const EnumDescriptor& obj) {return Reflect(obj.items);}},
        };

        return accessors;
    }
};
```
And in this case you need to correspondingly change the template itself and it's invocation:
```c++
std::string enum2StringConvertor = R"(
inline const char* {{enum.enumName}}ToString({{enum.enumName}} e)
{
    switch (e)
    {
{% for item in enum.items %}
    case {{item}}:
        return "{{item}}";
{% endfor %}
    }
    return "Unknown Item";
})";

// ...
    jinja2::ValuesMap params = {
        {"enum", jinja2::Reflect(descr)},
    };
// ...
```
Every specified field will be reflected into Jinja2Cpp internal data structures and can be accessed from the template without additional efforts. Quite simply! As you can see, you can use 'dot' notation to access named members of some parameter as well, as index notation like this: `enum['enumName']`. With index notation you can access to the particular item of a list: `enum.items[3]` or `enum.items[itemIndex]` or `enum['items'][itemIndex]`.

### 'set' statement
But what if enum `Animals` will be in the namespace?

```c++
namespace world
{
enum Animals
{
    Dog,
    Cat,
    Monkey,
    Elephant
};
}
```
In this case you need to prefix both enum name and it's items with namespace prefix in the generated code. Like this:
```c++
std::string enum2StringConvertor = R"(
inline const char* {{enum.enumName}}ToString({{enum.nsScope}}::{{enum.enumName}} e)
{
    switch (e)
    {
{% for item in enum.items %}
    case {{enum.nsScope}}::{{item}}:
        return "{{item}}";
{% endfor %}
    }
    return "Unknown Item";
})";
```
This template will produce 'world::' prefix for our new scoped enum (and enum itmes). And '::' for the ones in global scope. But you may want to eliminate the unnecessary global scope prefix. And you can do it this way:
```c++
{% set prefix = enum.nsScope + '::' if enum.nsScope else '' %}
std::string enum2StringConvertor = R"(inline const char* {{enum.enumName}}ToString({{prefix}}::{{enum.enumName}} e)
{
    switch (e)
    {
{% for item in enum.items %}
    case {{prefix}}::{{item}}:
        return "{{item}}";
{% endfor %}
    }
    return "Unknown Item";
})";
```
This template uses two significant jinja2 template features:
1. The 'set' statement. You can declare new variables in your template. And you can access them by the name.
2. if-expression. It works like a ternary '?:' operator in C/C++. In C++ the code from the sample could be written in this way:
```c++
std::string prefix = !descr.nsScope.empty() ? descr.nsScope + "::" : "";
```
I.e. left part of this expression (before 'if') is a true-branch of the statement. Right part (after 'else') - false-branch, which can be omitted. As a condition you can use any expression convertible to bool.

## 'extends' statement
In general, C++ header files look similar to each other. Almost every header file has got header guard, block of 'include' directives and then block of declarations wrapped into namespaces. So, if you have several different Jinja2 templates for header files production it can be a good idea to extract the common header structure into separate template. Like this:
```c++
{% if headerGuard is defined %}
 #ifndef {{headerGuard}}
 #define {{headerGuard}}
{% else %}
 #pragma once
{% endif %}

{% for fileName in inputFiles | sort %}
 #include "{{fileName}}"
{% endfor %}

{% for fileName in extraHeaders | sort %}
{% if fileName is startsWith('<') %}
 #include {{fileName}}
{% else %}
 #include "{{fileName}}"
{% endif %}
{% endfor %}

{% block generator_headers %}{% endblock %}

{% block namespaced_decls %}
{% set ns = rootNamespace %}
{#ns | pprint}
{{rootNamespace | pprint} #}
{% block namespace_content scoped %}{%endblock%}
{% for ns in rootNamespace.innerNamespaces recursive %}namespace {{ns.name}}
{
{{self.namespace_content()}}
{{ loop(ns.innerNamespaces) }}
}
{% endfor %}
{% endblock %}

{% block global_decls %}{% endblock %}

{% if headerGuard is defined %}
 #endif // {{headerGuard}}
{% endif %}
```

In this sample you can see the '**block**' statements. They are placeholders. Each block is a part of generic template which can be replaced by more specific template which 'extends' generic:
```c++
{% extends "header_skeleton.j2tpl" %}

{% block namespaced_decls %}{{super()}}{% endblock %}

{% block namespace_content %}
{% for class in ns.classes | sort(attribute="name") %}

class {{ class.name }}
{
public:
    {% for method in class.methods | rejectattr('isImplicit') | selectattr('accessType', 'equalto', 'Public') %}
    {{ method.fullPrototype }};
    {% endfor %}
protected:
    {% for method in class.methods | rejectattr('isImplicit') | selectattr('accessType', 'equalto', 'Protected') %}
    {{ method.fullPrototype }};
    {% endfor %}
private:
    {% for method in class.methods | rejectattr('isImplicit') | selectattr('accessType', 'in', ['Private', 'Undefined']) %}
    {{ method.fullPrototype }};
    {% endfor %}
};

{% endfor %}
{% endblock %}
```

'**extends**' statement here defines the template to extend. Set of '**block**' statements after defines actual filling of the corresponding blocks from the extended template. If block from the extended template contains something (like ```namespaced_decls``` from the example above), this content can be rendered with help of '**super()**' function. In other case the whole content of the block will be replaced. More detailed description of template inheritance feature can be found in [Jinja2 documentation](http://jinja.pocoo.org/docs/2.10/templates/#template-inheritance).

## Macros
Ths sample above violates 'DRY' rule. It contains the code which could be generalized. And Jinja2 supports features for such kind generalization. This feature called '**macro**'. The sample can be rewritten the following way:
```c++
{% macro MethodsDecl(class, access) %}
{% for method in class.methods | rejectattr('isImplicit') | selectattr('accessType', 'in', access) %}
    {{ method.fullPrototype }};
{% endfor %}
{% endmacro %}

class {{ class.name }}
{
public:
    {{ MethodsDecl(class, ['Public']) }}
protected:
    {{ MethodsDecl(class, ['Protected']) }}
private:
    {{ MethodsDecl(class, ['Private', 'Undefined']) }}
};

{% endfor %}
```

`MethodsDecl` statement here is a **macro** which takes two arguments. First one is a class with method definitions. The second is a tuple of access specifiers. Macro takes non-implicit methods from the methods collection (`rejectattr('isImplicit')` filter) then select such methods which have right access specifier (`selectattr('accessType', 'in', access)`), then just prints the method full prototype. Finally, the macro is invoked as a regular function call: `MethodsDecl(class, ['Public'])` and replaced with rendered macro body.

There is another way to invoke macro: the **call** statement. Simply put, this is a way to call macro with *callback*. Let's take another sample:

```c++
{% macro InlineMethod(resultType='void', methodName, methodParams=[]) %}
inline {{ resultType }} {{ methodName }}({{ methodParams | join(', ') }} )
{
    {{ caller() }}
}
{% endmacro %}

{% call InlineMethod('const char*', enum.enumName + 'ToString', [enum.nsScope ~ '::' ~ enum.enumName ~ ' e']) %}
    switch (e)
    {
{% for item in enum.items %}
    case {{enum.nsScope}}::{{item}}:
        return "{{item}}";
{% endfor %}
    }
    return "Unknown Item";
{% endcall %}
```

Here is an `InlineMacro` which just describe the inline method definition skeleton. This macro doesn't contain the actual method body. Instead of this it calls `caller` special function. This function invokes the special **callback** macro which is a body of `call` statement. And this macro can have parameters as well. More detailed this mechanics described in the [Jinja2 documentation](http://jinja.pocoo.org/docs/2.10/templates/#macros).

### 'applymacro' filter

With help of `applymacro` filter macro can be called in filtering context. `applymacro` works similar to `map` (or `test`) filter with one exception: instead of name of other filter it takes name of macro via `macro` param and pass the rest of arguments to it. The object which is been filtered is passed as the first positional argument. For example:

```
{% macro toUpper(str) %}{{ str | upper }}{% endmacro %}
{{ 'Hello World!' | applymacro(macro='toUpper') }}
```

produces the result `HELLO WORLD`. `applymacro` can be applied to the sequences via `map` filter. Also, macro name can be `caller`. In this case outer `call` statement will be invoked during macro application.

## User-defined callables

Not only C++ types can be reflected into Jinja2 template context, but the functions (and lambdas, and any other callable objects) as well. These refelected callable objects are called 'user-defined callables' and can be accessed from Jinja2 templates in the same manner as any other callables (like macros or global functions). In order to reflect callable object into Jinja2 context the `jinja2::MakeCallable` method should be used:

```c++
jinja2::ValuesMap params;
    params["concat"] = MakeCallable(
                [](const std::string& str1, const std::string& str2) {
                    return str1 + " " + str2;
                },
                ArgInfo{"str1"}, ArgInfo{"str2", false, "default"}
    );
```

As a first parameter this method takes the callable itself. It can be lambda, the std::function<> instance or pointer to function. The rest of params are callable arguments descriptors, which are provided via `ArgInfo` structure. In the sample above user-defined callable `concat` is introduced, which take two argument: `str1` and `str2`. This callable can be accessed from the template in the following ways:

```
{{ concat('Hello', 'World!') }}
{{ concat(str2='World!', str1='Hello') }}
{{ concat(str2='World!') }}
{{ concat('Hello') }}
```

## Error reporting
It's difficult to write complex template completely without errors. Missed braces, wrong characters, incorrect names... Everything is possible. So, it's crucial to be able to get informative error report from the template engine. Jinja2Cpp provides such kind of report. ```Template::Load``` method (and TemplateEnv::LoadTemplate respectively) return instance of ```ErrorInfo``` class which contains details about the error. These details include:
- Error code
- Error description
- File name and position (1-based line, col) of the error
- Location description

For example, this template:
```
{{ {'key'=,} }}
```
produces the following error message:
```
noname.j2tpl:1:11: error: Expected expression, got: ','
{{ {'key'=,} }}
       ---^-------
```

## Other features
The render procedure is stateless, so you can perform several renderings simultaneously in different threads. Even if you pass parameters:

```c++
    ValuesMap params = {
        {"intValue", 3},
        {"doubleValue", 12.123f},
        {"stringValue", "rain"},
        {"boolFalseValue", false},
        {"boolTrueValue", true},
    };

    std::string result = tpl.RenderAsString(params);
    std::cout << result << std::endl;
```

Parameters could have the following types:
- std::string/std::wstring
- integer (int64_t)
- double
- boolean (bool)
- Tuples (also known as arrays)
- Dictionaries (also known as maps)

Tuples and dictionaries can be mapped to the C++ types. So you can smoothly reflect your structures and collections into the template engine:

```c++
namespace jinja2
{
template<>
struct TypeReflection<reflection::EnumInfo> : TypeReflected<reflection::EnumInfo>
{
    static auto& GetAccessors()
    {
        static std::unordered_map<std::string, FieldAccessor> accessors = {
            {"name", [](const reflection::EnumInfo& obj) {return Reflect(obj.name);}},
            {"scopeSpecifier", [](const reflection::EnumInfo& obj) {return Reflect(obj.scopeSpecifier);}},
            {"namespaceQualifier", [](const reflection::EnumInfo& obj) { return obj.namespaceQualifier;}},
            {"isScoped", [](const reflection::EnumInfo& obj) {return obj.isScoped;}},
            {"items", [](const reflection::EnumInfo& obj) {return Reflect(obj.items);}},
        };

        return accessors;
    }
};

// ...
    jinja2::ValuesMap params = {
        {"enum", jinja2::Reflect(enumInfo)},
    };
```

In this cases method 'jinja2::reflect' reflects regular C++ type into jinja2 template param. If type is a user-defined class or structure then handwritten mapper 'TypeReflection<>' should be provided.

# Current Jinja2 support
Currently, Jinja2Cpp supports the limited number of Jinja2 features. By the way, Jinja2Cpp is planned to be full [jinja2 specification](http://jinja.pocoo.org/docs/2.10/templates/)-conformant. The current support is limited to:
- expressions. You can use almost every style of expressions: simple, filtered, conditional, and so on.
- big number of filters (**sort, default, first, last, length, max, min, reverse, unique, sum, attr, map, reject, rejectattr, select, selectattr, pprint, dictsort, abs, float, int, list, round, random, trim, title, upper, wordcount, replace, truncate, groupby, urlencode**)
- big number of testers (**eq, defined, ge, gt, iterable, le, lt, mapping, ne, number, sequence, string, undefined, in, even, odd, lower, upper**)
- limited number of functions (**range**, **loop.cycle**)
- 'if' statement (with 'elif' and 'else' branches)
- 'for' statement (with 'else' branch and 'if' part support)
- 'extends' statement
- 'set' statement
- 'extends'/'block' statements
- 'macro'/'call' statements
- recursive loops
- space control

# Supported compilers
Compilation of Jinja2Cpp tested on the following compilers (with C++14 enabled feature):
- Linux gcc 5.0
- Linux gcc 6.0
- Linux gcc 7.0
- Linux clang 5.0
- Microsoft Visual Studio 2015 x86, x64
- Microsoft Visual Studio 2017 x86, x64

# Build and install
Jinja2Cpp has five external dependencies: boost library (at least version 1.55) and several header-only dependecies from nonstd project(expected-lite, variant-lite, value-ptr-lite, optional-lite). Because of types from boost are used inside library, you should compile both your projects and Jinja2Cpp library with similar compiler settings. Otherwise ABI could be broken.

In order to compile Jinja2Cpp you need:

1. Install CMake build system (at least version 3.0)
2. Clone jinja2cpp repository and update submodules:

```
> git clone https://github.com/flexferrum/Jinja2Cpp.git
> git submodule -q update --init
```

3. Create build directory:

```
> cd Jinja2Cpp
> mkdir build
```

4. Run CMake and build the library:

```
> cd build
> cmake .. -DCMAKE_INSTALL_PREFIX=<path to install folder>
> cmake --build . --target all
```
"Path to install folder" here is a path to the folder where you want to install Jinja2Cpp lib.

5. Install library:

```
> cmake --build . --target install
```

6. Also you can run the tests:

```
> ctest -C Release
```

## Additional CMake build flags
You can define (via -D command line CMake option) the following build flags:

* **JINJA2CPP_BUILD_TESTS** (default TRUE) - to build or not to Jinja2Cpp tests.
* **JINJA2CPP_STRICT_WARNINGS** (default TRUE) - Enable strict mode compile-warnings(-Wall -Werror and etc).
* **JINJA2CPP_BUILD_SHARED** (default OFF) - Specify Jinja2Cpp library library link type.
* **MSVC_RUNTIME_TYPE** (default /MD) - MSVC runtime type to link with (if you use Microsoft Visual Studio compiler).
* **BOOST_ROOT** - Path to the prebuilt boost installation

# Link with you projects
Jinja2Cpp is shipped with cmake finder script. So you can:

1. Include Jinja2Cpp cmake scripts to the project:
```cmake
list (APPEND CMAKE_MODULE_PATH ${JINJA2CPP_INSTALL_DIR}/cmake)
```

2. Use regular 'find' script:
```cmake
find_package(Jinja2Cpp)
```

3. Add found paths to the project settings:
```cmake
#...
include_directories(
    #...
    ${JINJA2CPP_INCLUDE_DIR}
    )

target_link_libraries(YourTarget
    #...
    ${JINJA2CPP_LIBRARY}
    )
#...
```

or just link with Jinja2Cpp target:
```cmake
#...
target_link_libraries(YourTarget
    #...
    Jinja2Cpp
    )
#...
```
4. Build with C++17 standard enabled
In case of C++17 standard enabled for your project you should define `variant_CONFIG_SELECT_VARIANT=variant_VARIANT_NONSTD` macro in the build settings.

# Acknowledgments
Thanks to @manu343726 for CMake scripts improvement, bugs hunting and fixing and conan.io packaging.

Thanks to @martinmoene for perfectly implemented xxx-lite libraries.

# Changelog
## Version 0.9.1
* `applymacro` filter added which allows to apply arbitrary macro as a filter
* dependencies to boost removed from the public interface
* CMake scripts improved
* Various bugs fixed
* Improve reflection
* Warnings cleanup

## Version 0.9
* Support of 'extents'/'block' statements
* Support of 'macro'/'call' statements
* Rich error reporting
* Support for recursive loops
* Support for space control before and after control blocks
* Improve reflection

## Version 0.6
* A lot of filters has been implemented. Full set of supported filters listed here: https://github.com/flexferrum/Jinja2Cpp/issues/7
* A lot of testers has been implemented. Full set of supported testers listed here: https://github.com/flexferrum/Jinja2Cpp/issues/8
* 'Contatenate as string' operator ('~') has been implemented
* For-loop with 'if' condition has been implemented
* Fixed some bugs in parser
