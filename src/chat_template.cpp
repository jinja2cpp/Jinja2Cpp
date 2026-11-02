// The chat-template preset (docs/tasks/0360): settings, the transformers dialect and the two
// globals of transformers' apply_chat_template. Compiled only with JINJA2CPP_WITH_CHAT_TEMPLATES
#include <jinja2cpp/chat_template.h>

#include "dialect.h"
#include "template_env_impl.h"

#include <jinja2cpp/error_info.h>
#include <jinja2cpp/exception.h>
#include <jinja2cpp/user_callable.h>
#include <jinja2cpp/value.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <iomanip>
#include <locale>
#include <sstream>
#include <string>
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

// datetime.strftime: %f is the microseconds and %z, %:z and %Z are empty for a naive datetime;
// the rest is the C library's strftime, here in the "C" locale
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

    std::string pyFormat;
    pyFormat.reserve(format.size());
    for (std::size_t pos = 0; pos < format.size(); ++pos)
    {
        if (format[pos] != '%' || pos + 1 == format.size())
        {
            pyFormat.push_back(format[pos]);
            continue;
        }
        const char directive = format[++pos];
        if (directive == 'f')
        {
            std::string digits = std::to_string(micros);
            pyFormat.append(6 - std::min<std::size_t>(6, digits.size()), '0');
            pyFormat += digits;
        }
        else if (directive == 'z' || directive == 'Z')
        {
        }
        else if (directive == ':' && pos + 1 < format.size() && format[pos + 1] == 'z')
        {
            ++pos;
        }
        else
        {
            pyFormat.push_back('%');
            pyFormat.push_back(directive);
        }
    }

    std::ostringstream os;
    os.imbue(std::locale::classic());
    os << std::put_time(&local, pyFormat.c_str());
    return os.str();
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

    // transformers: raise jinja2.exceptions.TemplateError(message)
    env.AddGlobal("raise_exception", MakeCallable([](const std::string& message) -> Value { throw Exception(ErrorCode::TemplateRuntimeError, message); }, ArgInfo{ "message", true }));
    auto now = options.now ? std::move(options.now) : [] { return std::chrono::system_clock::now(); };
    env.AddGlobal("strftime_now", MakeCallable([now = std::move(now)](const std::string& format) { return StrftimeNow(format, now()); }, ArgInfo{ "format", true }));
}

} // namespace v2
} // namespace jinja2
