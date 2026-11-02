// The chat-template preset (docs/tasks/0360): settings, the transformers dialect and the two
// globals of transformers' apply_chat_template. Compiled only with JINJA2CPP_WITH_CHAT_TEMPLATES
#include <jinja2cpp/chat_template.h>

#include "dialect.h"
#include "template_env_impl.h"

#include <jinja2cpp/error_info.h>
#include <jinja2cpp/exception.h>
#include <jinja2cpp/string_helpers.h>
#include <jinja2cpp/user_callable.h>
#include <jinja2cpp/value.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <iomanip>
#include <locale>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

namespace jinja2
{
inline namespace v2
{
namespace
{

// The sandbox's MAX_RANGE
constexpr std::uint64_t SandboxMaxRange = 100000;

std::tm LocalTime(std::time_t time)
{
    std::tm result{};
#ifdef _WIN32
    localtime_s(&result, &time);
#else
    localtime_r(&time, &result);
#endif
    return result;
}

// The directives C++'s std::put_time formats in the "C" locale on every platform (C++11's
// strftime set). Anything else is copied as text: the C library of MSVC aborts on a directive
// it does not know, and templates are not trusted
bool IsPortableDirective(char directive)
{
    static constexpr std::string_view portable = "aAbBcCdDeFgGhHIjmMnprRStTuUVwWxXyY";
    return portable.find(directive) != std::string_view::npos;
}

std::string PutTime(const std::tm& local, char directive)
{
    const char format[] = { '%', directive, '\0' };
    std::ostringstream os;
    os.imbue(std::locale::classic());
    os << std::put_time(&local, format);
    return os.str();
}

// One directive of StrftimeNow; none for a directive it copies as written
std::optional<std::string> FormatDirective(const std::tm& local, long long micros, char directive, bool noPadding)
{
    if (!noPadding)
    {
        switch (directive)
        {
        case '%':
            return std::string("%");
        case 'z':
        case 'Z':
            return std::string();
        default:
            break;
        }
    }
    if (directive == 'f')
    {
        const auto digits = std::to_string(micros);
        return std::string(6 - std::min<std::size_t>(6, digits.size()), '0') + digits;
    }
    if (!IsPortableDirective(directive))
    {
        return std::nullopt;
    }
    auto text = PutTime(local, directive);
    if (noPadding)
    {
        // glibc's `-` flag: no zeros or spaces before a number, and 0 stays 0
        const auto first = text.find_first_not_of(" 0");
        text.erase(0, first == std::string::npos ? (text.empty() ? 0 : text.size() - 1) : first);
    }
    return text;
}

// datetime.strftime as CPython on Linux runs it: %f is the microseconds, %z, %:z and %Z are
// empty for a naive datetime, glibc's `-` flag drops the padding (%-d is 1, %-H is 9), %% is %,
// and the rest is strftime in the "C" locale. A directive outside that set and a trailing %
// stay as they are written
std::string StrftimeNow(const std::string& format, std::chrono::system_clock::time_point now)
{
    const auto seconds = std::chrono::time_point_cast<std::chrono::seconds>(now);
    auto micros = std::chrono::duration_cast<std::chrono::microseconds>(now - seconds).count();
    auto time = std::chrono::system_clock::to_time_t(seconds);
    if (micros < 0)
    {
        micros += 1000000;
        --time;
    }
    const auto local = LocalTime(time);

    std::string result;
    result.reserve(format.size() + 16);
    std::size_t pos = 0;
    while (pos < format.size())
    {
        if (format[pos] != '%' || pos + 1 == format.size())
        {
            result.push_back(format[pos++]);
            continue;
        }
        const auto start = pos++;
        const bool noPadding = format[pos] == '-' && pos + 1 < format.size();
        pos += noPadding ? 1 : 0;
        const char directive = format[pos++];
        if (directive == ':' && !noPadding && pos < format.size() && format[pos] == 'z')
        {
            ++pos; // %:z, empty
            continue;
        }
        auto text = FormatDirective(local, micros, directive, noPadding);
        result += text ? *text : format.substr(start, pos - start);
    }
    return result;
}

// Python's type(value).__name__, for the TypeError of a non-string argument
const char* PythonTypeName(const Value& value)
{
    if (value.IsNone())
    {
        return "NoneType";
    }
    if (value.IsBool())
    {
        return "bool";
    }
    if (value.IsInt() || value.IsBigInt())
    {
        return "int";
    }
    if (value.IsDouble() || value.IsFloat())
    {
        return "float";
    }
    if (value.IsList())
    {
        return "list";
    }
    if (value.IsMap())
    {
        return "dict";
    }
    return "object";
}

// A string argument, narrow or wide as the template is, as UTF-8; null for any other value
std::optional<std::string> StringArgument(const Value& value)
{
    std::string_view narrow;
    if (value.AsString(narrow))
    {
        return std::string(narrow);
    }
    std::wstring_view wide;
    if (value.AsWString(wide))
    {
        return ConvertString<std::string>(std::wstring(wide));
    }
    return std::nullopt;
}

// raise_exception(message): Python's TemplateError(message) shows str(message)
std::string MessageText(const Value& message)
{
    auto text = StringArgument(message);
    return text ? std::move(*text) : ToString(message);
}

std::string FormatArgument(const Value& format)
{
    auto text = StringArgument(format);
    if (!text)
    {
        throw Exception(ErrorCode::TemplateRuntimeError, std::string("strftime() argument 1 must be str, not ") + PythonTypeName(format));
    }
    return std::move(*text);
}

} // namespace

void ConfigureForChatTemplates(TemplateEnv& env, ChatTemplateOptions options)
{
    auto& settings = env.GetSettings();
    settings.trimBlocks = true;
    settings.lstripBlocks = true;
    settings.extensions.loopControls = true;
    settings.autoescape = false;
    settings.autoescapeSelector = AutoescapeSelector();
    if (settings.limits.maxRange == 0 || settings.limits.maxRange > SandboxMaxRange)
    {
        settings.limits.maxRange = SandboxMaxRange;
    }
    detail::TemplateEnvAccess::GetImpl(env)->SetDialect(detail::Dialect::Transformers);

    // transformers: raise jinja2.exceptions.TemplateError(message), with str(message) as its text
    env.AddGlobal("raise_exception", UserCallable{ [](const UserCallableParams& params) -> Value { throw Exception(ErrorCode::TemplateRuntimeError, MessageText(params["message"])); },
                                                   { ArgInfo{ "message", true } } });
    auto now = options.now ? std::move(options.now) : [] { return std::chrono::system_clock::now(); };
    env.AddGlobal("strftime_now", UserCallable{ [now = std::move(now)](const UserCallableParams& params) -> Value { return StrftimeNow(FormatArgument(params["format"]), now()); },
                                                { ArgInfo{ "format", true } } });
}

} // namespace v2
} // namespace jinja2
