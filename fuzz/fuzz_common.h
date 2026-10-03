// Shared setup for the libFuzzer targets in this directory (docs/tasks/0003).
// Each target defines LLVMFuzzerTestOneInput; replay_main.cpp drives the same functions
// without libFuzzer, so the regression inputs in fuzz/regressions/ run under any compiler.
#ifndef JINJA2CPP_FUZZ_COMMON_H
#define JINJA2CPP_FUZZ_COMMON_H

#include <jinja2cpp/error_info.h>
#include <jinja2cpp/filesystem_handler.h>
#include <jinja2cpp/template_env.h>
#include <jinja2cpp/value.h>

#include <cstddef>
#include <cstdint>
#include <string>

namespace jinja2_fuzz
{

// Inputs above this size only slow the fuzzer down: a template that crashes at 4 KiB
// almost always crashes at a few hundred bytes too.
constexpr std::size_t MaxInputSize = 4096;

// Templates the fuzzed one can include, import or extend. "self" is the fuzzed input,
// so recursion through include/extends/import is reachable.
inline void AddSupportTemplates(jinja2::MemoryFileSystem& fs, const std::string& self)
{
    fs.AddFile("self", self);
    fs.AddFile("base", "<{% block content %}base{% endblock %}|{% block other %}{% endblock %}>");
    fs.AddFile("header", "[{{ foo }}|{{ bar }}]");
    fs.AddFile("macros", "{% macro m(a, b=2) %}{{ a }}-{{ b }}-{{ varargs }}-{{ kwargs }}{{ caller() if caller }}{% endmacro %}{% set v = 42 %}");
}

inline void AddSupportTemplates(jinja2::MemoryFileSystem& fs, const std::wstring& self)
{
    fs.AddFile("self", self);
    fs.AddFile("base", L"<{% block content %}base{% endblock %}|{% block other %}{% endblock %}>");
    fs.AddFile("header", L"[{{ foo }}|{{ bar }}]");
    fs.AddFile("macros", L"{% macro m(a, b=2) %}{{ a }}-{{ b }}-{{ varargs }}-{{ kwargs }}{{ caller() if caller }}{% endmacro %}{% set v = 42 %}");
}

// Every optional statement is on, so the fuzzer reaches their parsers too.
inline void ConfigureEnv(jinja2::TemplateEnv& env)
{
    auto& settings = env.GetSettings();
    settings.extensions.doStatement = true;
    settings.extensions.loopControls = true;
    settings.extensions.i18n = true;
    env.AddGlobal("bar", 23);
}

// Mirrors the variety of the parity corpus context (test/parity/cases/filters.py), so
// seeds taken from it reach the same code paths instead of stopping at undefined names.
inline jinja2::ValuesMap MakeContext()
{
    jinja2::ValuesMap users{
        { "name", "bob" },
        { "age", 30 },
    };
    return jinja2::ValuesMap{
        { "x", 3 },
        { "neg", -3 },
        { "y", 2.567 },
        { "s", "Hello World" },
        { "l", jinja2::ValuesList{ 3, 1, 2 } },
        { "dup", jinja2::ValuesList{ 3, 1, 3, 2, 1 } },
        { "words", jinja2::ValuesList{ "b", "A", "c" } },
        { "d", jinja2::ValuesMap{ { "b", 2 }, { "a", 1 }, { "C", 3 } } },
        { "e", jinja2::ValuesList{} },
        { "n", jinja2::EmptyValue{} },
        { "t", true },
        { "html", "<b>Tom & \"Jerry\"</b>" },
        { "users", jinja2::ValuesList{ users, jinja2::ValuesMap{ { "name", "alice" }, { "age", 25 }, { "city", "Rome" } } } },
        { "foo", 42 },
        { "ws", std::wstring(L"wide \u00e9") },
    };
}

// How the narrow render target ended, for the differential check (fuzz/differential.py)
struct Outcome
{
    enum Kind
    {
        Rendered,
        ParseError,
        RenderError,
    };
    Kind kind = Rendered;
    std::string output;
    jinja2::ErrorCode code = jinja2::ErrorCode::Unspecified;
};

// FuzzRender's work, with the result kept
Outcome RenderOutcome(const std::uint8_t* data, std::size_t size);

// The fuzz entry points. Each returns normally on any input; a sanitizer report, an
// abort or an uncaught exception is the finding.
void FuzzParse(const std::uint8_t* data, std::size_t size);
void FuzzRender(const std::uint8_t* data, std::size_t size);
void FuzzRenderWide(const std::uint8_t* data, std::size_t size);

} // namespace jinja2_fuzz

#endif // JINJA2CPP_FUZZ_COMMON_H
