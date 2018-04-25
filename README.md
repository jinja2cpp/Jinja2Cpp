# Jinja2Cpp

[![Build Status](https://travis-ci.org/flexferrum/Jinja2Cpp.svg?branch=master)](https://travis-ci.org/flexferrum/Jinja2Cpp)
[![Build status](https://ci.appveyor.com/api/projects/status/qtgniyyg6fn8ich8/branch/master?svg=true)](https://ci.appveyor.com/project/flexferrum/Jinja2Cpp)
[![Github Releases](https://img.shields.io/github/release/flexferrum/Jinja2Cpp.svg)](https://github.com/flexferrum/Jinja2Cpp/releases)
[![Github Issues](https://img.shields.io/github/issues/flexferrum/Jinja2Cpp.svg)](http://github.com/flexferrum/Jinja2Cpp/issues)
[![GitHub License](https://img.shields.io/badge/license-Mozilla-blue.svg)](https://raw.githubusercontent.com/flexferrum/Jinja2Cpp/master/LICENSE)

C++ implementation of big subset of Jinja 2 template engine features
Welcome to the Jinja2Cpp wiki!

# Jinja2Cpp

[![Build Status](https://travis-ci.org/flexferrum/Jinja2Cpp.svg?branch=master)](https://travis-ci.org/flexferrum/Jinja2Cpp)[![Build status](https://ci.appveyor.com/api/projects/status/qtgniyyg6fn8ich8/branch/master?svg=true)](https://ci.appveyor.com/project/flexferrum/Jinja2Cpp)[![Github Releases](https://img.shields.io/github/release/flexferrum/Jinja2Cpp.svg)](https://github.com/flexferrum/Jinja2Cpp/releases)[![Github Issues](https://img.shields.io/github/issues/flexferrum/Jinja2Cpp.svg)](http://github.com/flexferrum/Jinja2Cpp/issues)[![GitHub License](https://img.shields.io/badge/license-Mozilla-blue.svg)](https://raw.githubusercontent.com/flexferrum/Jinja2Cpp/master/LICENSE)

C++ implementation of big subset of Jinja 2 template engine features. This library was inspired by Jinja2CppLight project and brings support of mostly all Jinja 2 templates features into C++ world. Unlike inja lib, you have to build Jinja2Cpp, but it has only one dependence: boost.

Main features of Jinja2Cpp:
- Easy-to-use public interface. Just load templates and render them.
- Support for both narrow- and wide-character strings both for templates and parameters.
- Built-in reflection for C++ types.
- Powerful Jinja 2 expressions with filtering (via '¦' operator) and 'if' expressions.
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
