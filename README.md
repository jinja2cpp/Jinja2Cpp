# Jinja2Cpp

[![Build Status](https://travis-ci.org/flexferrum/Jinja2Cpp.svg?branch=master)](https://travis-ci.org/flexferrum/Jinja2Cpp)
[![Build status](https://ci.appveyor.com/api/projects/status/19v2k3bl63jxl42f/branch/master?svg=true)](https://ci.appveyor.com/project/flexferrum/Jinja2Cpp)
[![Github Releases](https://img.shields.io/github/release/flexferrum/Jinja2Cpp.svg)](https://github.com/flexferrum/Jinja2Cpp/releases)
[![Github Issues](https://img.shields.io/github/issues/flexferrum/Jinja2Cpp.svg)](http://github.com/flexferrum/Jinja2Cpp/issues)
[![GitHub License](https://img.shields.io/badge/license-Mozilla-blue.svg)](https://raw.githubusercontent.com/flexferrum/Jinja2Cpp/master/LICENSE)

C++ implementation of big subset of Jinja 2 template engine features. This library was inspired by [Jinja2CppLight](https://github.com/hughperkins/Jinja2CppLight) project and brings support of mostly all Jinja 2 templates features into C++ world. Unlike [inja](https://github.com/pantor/inja) lib, you have to build Jinja2Cpp, but it has only one dependence: boost.

Main features of Jinja2Cpp:
- Easy-to-use public interface. Just load templates and render them.
- Support for both narrow- and wide-character strings both for templates and parameters.
- Built-in reflection for C++ types.
- Powerful Jinja 2 expressions with filtering (via '|' operator) and 'if' expressions.
- Main control statements (set, for, if).

For example, this simple code:

```c++
std::string source = R"(
{{ ("Hello", 'world') | join }}!!!
{{ ("Hello", 'world') | join(', ') }}!!!
{{ ("Hello", 'world') | join(d = '; ') }}!!!
)";

Template tpl;

std::string result = tpl.RenderAsString(ValuesMap());
```

produces the result string:

```
Helloworld!!!
Hello, world!!!
Hello; world!!!
```

# Getting started

In order to use Jinja2Cpp in your project you have to:
* Clone the Jinja2Cpp repository
* Build it according with the instructions
* Link with your project.

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
- limited number of filters (**join**, **sort**)
- limited number of testers (**defined**, **startsWith**)
- limited number of functions (**range**, **loop.cycle**)
- 'if' statement (with 'elif' and 'else' branches)
- 'for' statement (with 'else' branch support)
- 'set' statement

# Supported compilers
Compilation of Jinja2Cpp tested on the following compilers (with C++14 enabled feature):
- Linux gcc 5.0
- Linux gcc 6.0
- Linux gcc 7.0
- Linux clang 5.0
- Microsoft Visual Studio 2015 x86
- Microsoft Visual Studio 2017 x86
