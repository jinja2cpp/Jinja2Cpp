#include "fuzz_common.h"

#include <jinja2cpp/error_info.h>
#include <jinja2cpp/filesystem_handler.h>
#include <jinja2cpp/template.h>
#include <jinja2cpp/template_env.h>
#include <jinja2cpp/value.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>

namespace jinja2_fuzz
{
namespace
{

// Lenient UTF-8 decoder: a malformed sequence becomes U+FFFD, so every input yields a
// wide template and the narrow seeds keep their meaning in the wide target.
std::wstring DecodeUtf8(const std::uint8_t* data, std::size_t size)
{
    std::wstring result;
    result.reserve(size);
    std::size_t pos = 0;
    while (pos < size)
    {
        const std::uint8_t lead = data[pos];
        std::size_t len = 0;
        char32_t cp = 0;
        if (lead < 0x80)
        {
            len = 1;
            cp = lead;
        }
        else if ((lead & 0xE0) == 0xC0)
        {
            len = 2;
            cp = lead & 0x1F;
        }
        else if ((lead & 0xF0) == 0xE0)
        {
            len = 3;
            cp = lead & 0x0F;
        }
        else if ((lead & 0xF8) == 0xF0)
        {
            len = 4;
            cp = lead & 0x07;
        }
        bool valid = len != 0 && pos + len <= size;
        for (std::size_t i = 1; valid && i < len; ++i)
        {
            if ((data[pos + i] & 0xC0) != 0x80)
            {
                valid = false;
            }
            else
            {
                cp = (cp << 6) | (data[pos + i] & 0x3F);
            }
        }
        if (!valid || cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF))
        {
            result.push_back(static_cast<wchar_t>(0xFFFD));
            ++pos;
            continue;
        }
        if (sizeof(wchar_t) == 2 && cp > 0xFFFF)
        {
            cp -= 0x10000;
            result.push_back(static_cast<wchar_t>(0xD800 + (cp >> 10)));
            result.push_back(static_cast<wchar_t>(0xDC00 + (cp & 0x3FF)));
        }
        else
        {
            result.push_back(static_cast<wchar_t>(cp));
        }
        pos += len;
    }
    return result;
}

template<typename CharT>
Outcome LoadAndRender(const std::basic_string<CharT>& source)
{
    jinja2::TemplateEnv env;
    ConfigureEnv(env);
    auto fs = std::make_shared<jinja2::MemoryFileSystem>();
    AddSupportTemplates(*fs, source);
    env.AddFilesystemHandler(std::string(), fs);

    jinja2::BasicTemplate<CharT> tpl(&env);
    auto loaded = tpl.Load(source, "self");
    if (!loaded)
    {
        return { Outcome::ParseError, {}, loaded.error().GetCode() };
    }
    // A render error is a normal outcome, a crash is not
    auto rendered = tpl.RenderAsString(MakeContext());
    if (!rendered)
    {
        return { Outcome::RenderError, {}, rendered.error().GetCode() };
    }
    if constexpr (std::is_same_v<CharT, char>)
    {
        return { Outcome::Rendered, std::move(rendered.value()), jinja2::ErrorCode::Unspecified };
    }
    else
    {
        return { Outcome::Rendered, {}, jinja2::ErrorCode::Unspecified };
    }
}

} // namespace

void FuzzParse(const std::uint8_t* data, std::size_t size)
{
    if (size > MaxInputSize)
    {
        return;
    }
    jinja2::Template tpl;
    (void)tpl.Load(std::string(reinterpret_cast<const char*>(data), size));
}

void FuzzRender(const std::uint8_t* data, std::size_t size)
{
    if (size > MaxInputSize)
    {
        return;
    }
    (void)LoadAndRender(std::string(reinterpret_cast<const char*>(data), size));
}

Outcome RenderOutcome(const std::uint8_t* data, std::size_t size)
{
    return LoadAndRender(std::string(reinterpret_cast<const char*>(data), size));
}

void FuzzRenderWide(const std::uint8_t* data, std::size_t size)
{
    if (size > MaxInputSize)
    {
        return;
    }
    (void)LoadAndRender(DecodeUtf8(data, size));
}

} // namespace jinja2_fuzz
