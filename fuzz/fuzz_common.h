// Shared setup for the libFuzzer targets in this directory (docs/tasks/0003).
// Each target defines LLVMFuzzerTestOneInput; replay_main.cpp drives the same functions
// without libFuzzer, so the regression inputs in fuzz/regressions/ run under any compiler.
#ifndef JINJA2CPP_FUZZ_COMMON_H
#define JINJA2CPP_FUZZ_COMMON_H

#include <jinja2cpp/error_info.h>
#include <jinja2cpp/filesystem_handler.h>
#include <jinja2cpp/template_env.h>
#include <jinja2cpp/value.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

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

// The settings a host rendering its users' templates may choose, picked by the first byte of the input in the
// untrusted, host and loader targets (docs/tasks/0267). make_corpus.py --prefix writes seeds with these bytes and
// rewrites their delimiters to match.
namespace settings_byte
{
constexpr std::uint8_t Autoescape = 0x01;
constexpr std::uint8_t TrimBlocks = 0x02;
constexpr std::uint8_t LstripBlocks = 0x04;
// `#` line statements and `##` line comments
constexpr std::uint8_t LineStatements = 0x08;
// Two bits: 0 the default delimiters, 1-3 the sets of Delimiters below
constexpr std::uint8_t DelimiterShift = 4;
constexpr std::uint8_t DelimiterMask = 0x30;
constexpr std::uint8_t StrictUndefined = 0x40;
// Render through TemplateW (the input decoded as UTF-8)
constexpr std::uint8_t Wide = 0x80;
} // namespace settings_byte

// Variable start/end, block start/end, comment start/end
struct Delimiters
{
    std::string_view variableStart, variableEnd, blockStart, blockEnd, commentStart, commentEnd;
};

// The examples of Jinja2's documentation and two more: same-length, different-length and a shared start
inline const Delimiters& DelimiterSet(unsigned index)
{
    static const Delimiters sets[] = {
        { "{{", "}}", "{%", "%}", "{#", "#}" },
        { "${", "}", "<%", "%>", "<#", "#>" },
        { "\\VAR{", "}", "\\BLOCK{", "}", "\\#{", "}" },
        { "[[", "]]", "[%", "%]", "[#", "#]" },
    };
    return sets[index & 3];
}

// Writes `text` with the default delimiters replaced by those of `delims`, so that the support templates mean the
// same under every delimiter set
template<typename CharT>
std::basic_string<CharT> WithDelimiters(std::string_view text, const Delimiters& delims)
{
    const std::pair<std::string_view, std::string_view> pairs[] = {
        { "{{", delims.variableStart },
        { "}}", delims.variableEnd },
        { "{%", delims.blockStart },
        { "%}", delims.blockEnd },
        { "{#", delims.commentStart },
        { "#}", delims.commentEnd },
    };
    std::basic_string<CharT> result;
    for (std::size_t pos = 0; pos < text.size();)
    {
        bool replaced = false;
        for (const auto& [from, to] : pairs)
        {
            if (text.compare(pos, from.size(), from) == 0)
            {
                result.append(to.begin(), to.end());
                pos += from.size();
                replaced = true;
                break;
            }
        }
        if (!replaced)
        {
            result.push_back(static_cast<CharT>(text[pos++]));
        }
    }
    return result;
}

// Applies the settings byte
inline void ApplySettingsByte(jinja2::Settings& settings, std::uint8_t byte)
{
    settings.autoescape = (byte & settings_byte::Autoescape) != 0;
    settings.trimBlocks = (byte & settings_byte::TrimBlocks) != 0;
    settings.lstripBlocks = (byte & settings_byte::LstripBlocks) != 0;
    if ((byte & settings_byte::LineStatements) != 0)
    {
        settings.lineStatementPrefix = "#";
        settings.lineCommentPrefix = "##";
    }
    const auto& delims = DelimiterSet(static_cast<unsigned>(byte & settings_byte::DelimiterMask) >> settings_byte::DelimiterShift);
    settings.variableStartString = delims.variableStart;
    settings.variableEndString = delims.variableEnd;
    settings.blockStartString = delims.blockStart;
    settings.blockEndString = delims.blockEnd;
    settings.commentStartString = delims.commentStart;
    settings.commentEndString = delims.commentEnd;
    if ((byte & settings_byte::StrictUndefined) != 0)
    {
        settings.undefinedPolicy = jinja2::UndefinedPolicy::Strict;
    }
}

inline const Delimiters& DelimitersOf(std::uint8_t settingsByte)
{
    return DelimiterSet(static_cast<unsigned>(settingsByte & settings_byte::DelimiterMask) >> settings_byte::DelimiterShift);
}

// UntrustedLimits() (docs/tasks/0262) made tight enough that a template cannot legitimately take longer than the
// fuzzer's timeout or more memory than its RSS limit, so that the targets running under them can treat a timeout or an
// out-of-memory as a finding (docs/tasks/0267). No limit bounds memory directly (docs/tasks/0459): a render may keep
// what each step builds, so memory is bounded by steps times the sequence cap, and both are lowered here.
inline jinja2::Settings::Limits FuzzLimits()
{
    auto limits = jinja2::UntrustedLimits();
    limits.maxSequenceSize = std::uint64_t{ 1 } << 16;
    limits.maxOutputSize = std::uint64_t{ 1 } << 20;
    limits.maxRenderSteps = 20000;
    limits.maxRenderTime = std::chrono::milliseconds(2000);
    limits.maxTemplateSize = std::uint64_t{ 1 } << 16;
    limits.maxTemplateLoads = 64;
    return limits;
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
        { "n", jinja2::NoneValue{} },
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

// Lenient UTF-8 decoder: a malformed sequence becomes U+FFFD, so every input yields a wide template
std::wstring DecodeUtf8(const std::uint8_t* data, std::size_t size);

// ConfigureEnv, FuzzLimits() and the settings byte: the environment of the limits-on targets
void ConfigureUntrustedEnv(jinja2::TemplateEnv& env, std::uint8_t settingsByte);

// FuzzRender's work, with the result kept
Outcome RenderOutcome(const std::uint8_t* data, std::size_t size);

// The fuzz entry points. Each returns normally on any input; a sanitizer report, an
// abort or an uncaught exception is the finding.
void FuzzParse(const std::uint8_t* data, std::size_t size);
void FuzzRender(const std::uint8_t* data, std::size_t size);
void FuzzRenderWide(const std::uint8_t* data, std::size_t size);
void FuzzMetadata(const std::uint8_t* data, std::size_t size);
// AutoescapeMode::Prompt: no special token of the data reaches the output (prompt_escape_case.h), and any template
// renders without a crash
void FuzzPromptEscape(const std::uint8_t* data, std::size_t size);
// FuzzLimits() and the settings byte: a timeout or out-of-memory is a finding (docs/tasks/0267)
void FuzzUntrusted(const std::uint8_t* data, std::size_t size);
// The untrusted target with host callables (typed, *args, **kwargs, *context, throwing), reflected structs and
// lists, a generated list and host filters and tests
void FuzzHostSurface(const std::uint8_t* data, std::size_t size);
// Templates on disk: RealFileSystem or ConfinedFileSystem (the second byte) over a temporary root, with canary files
// outside it and, for ConfinedFileSystem, links inside it that point at them. Canary text in the output aborts
void FuzzLoader(const std::uint8_t* data, std::size_t size);
// Fails the Nth allocation (the first two bytes) of a Load and Render: each ends normally or throws std::bad_alloc,
// and the environment and template render as before afterwards. Defined in alloc_failure_target.cpp, which replaces
// the global operator new, so it is built only into its own executables
void FuzzAllocFailure(const std::uint8_t* data, std::size_t size);

} // namespace jinja2_fuzz

#endif // JINJA2CPP_FUZZ_COMMON_H
