#ifndef JINJA2CPP_SRC_MARKUP_H
#define JINJA2CPP_SRC_MARKUP_H

#include "internal_value.h"
#include "out_stream.h"
#include "render_context.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>

namespace jinja2
{
inline namespace v2
{

namespace detail
{
// What markupsafe.escape writes for each code unit below 256: the unit itself, or the entity of
// ", &, ', < and >. Each is padded to 8 bytes, so that one 8-byte copy writes any of them.
struct HtmlEscapeTable
{
    char text[256][8] = {};
    uint8_t size[256] = {};

    constexpr HtmlEscapeTable()
    {
        for (int ch = 0; ch != 256; ++ch)
        {
            text[ch][0] = static_cast<char>(ch);
            size[ch] = 1;
        }
        Set('<', "&lt;");
        Set('>', "&gt;");
        Set('&', "&amp;");
        Set('\'', "&#39;");
        Set('"', "&#34;");
    }

    constexpr void Set(unsigned char ch, const char* entity)
    {
        uint8_t n = 0;
        for (; entity[n] != 0; ++n)
        {
            text[ch][n] = entity[n];
        }
        size[ch] = n;
    }
};

inline constexpr HtmlEscapeTable htmlEscapeTable{};

// Strings up to this long are escaped in one pass through a per-thread buffer of 5 times their
// size; longer ones are measured first, so the buffer stays small
constexpr size_t maxBufferedHtmlEscape = 4096;

// The buffer of EscapeHtmlToBuffer on this thread; ReleaseThreadCaches frees it
inline std::string& HtmlEscapeBuffer()
{
    thread_local std::string buffer;
    return buffer;
}

// Escapes a narrow string in one pass, without a capacity check per byte. The view is valid until
// the next call on the same thread.
inline std::string_view EscapeHtmlToBuffer(std::string_view str)
{
    auto& buffer = HtmlEscapeBuffer();
    auto worstCase = (str.size() * 5) + 8;
    if (buffer.size() < worstCase)
    {
        buffer.resize(worstCase);
    }
    auto* out = buffer.data();
    for (auto ch : str)
    {
        auto unit = static_cast<unsigned char>(ch);
        std::memcpy(out, htmlEscapeTable.text[unit], 8);
        out += htmlEscapeTable.size[unit];
    }
    return { buffer.data(), static_cast<size_t>(out - buffer.data()) };
}

// The size of the escaped string, for strings too long for EscapeHtmlToBuffer and wide ones
template<typename CharT>
size_t EscapedHtmlSize(std::basic_string_view<CharT> str)
{
    size_t size = 0;
    for (auto ch : str)
    {
        auto unit = CodeUnit(ch);
        size += unit < 256 ? htmlEscapeTable.size[unit] : 1;
    }
    return size;
}

// Writes the escaped string into chars, of the size EscapedHtmlSize measured
template<typename CharT>
void WriteEscapedHtml(std::basic_string_view<CharT> str, boost::span<CharT> chars)
{
    auto out = chars.begin();
    for (auto ch : str)
    {
        auto unit = CodeUnit(ch);
        if (unit >= 256 || htmlEscapeTable.size[unit] == 1)
        {
            *out++ = ch;
            continue;
        }
        const auto* entity = htmlEscapeTable.text[unit];
        out = std::copy(entity, entity + htmlEscapeTable.size[unit], out);
    }
}
} // namespace detail

// markupsafe.escape of a string
template<typename CharT>
std::basic_string<CharT> EscapeHtml(std::basic_string_view<CharT> str)
{
    if constexpr (sizeof(CharT) == 1)
    {
        if (str.size() <= detail::maxBufferedHtmlEscape)
        {
            auto escaped = detail::EscapeHtmlToBuffer(std::string_view(str.data(), str.size()));
            return std::basic_string<CharT>(escaped.data(), escaped.size());
        }
    }
    auto size = detail::EscapedHtmlSize(str);
    if (size == str.size())
    {
        return std::basic_string<CharT>(str);
    }
    std::basic_string<CharT> result(size, CharT{});
    detail::WriteEscapedHtml(str, boost::span<CharT>(result.data(), result.size()));
    return result;
}

// markupsafe.escape: Markup is returned as is, anything else becomes Markup of its escaped str().
// A string of the template's character type is escaped from its own text (docs/tasks/0126).
inline InternalValue MarkupEscape(const InternalValue& val, IRendererCallback* callback)
{
    if (val.IsMarkup())
    {
        return val;
    }
    // A string of the template's character type is escaped from its own text into the result,
    // allocated once, or shared when it has nothing to escape (docs/tasks/0151)
    auto escape = [&val](auto text) {
        using CharT = typename decltype(text)::value_type;
        InternalValue result;
        if constexpr (sizeof(CharT) == 1)
        {
            if (text.size() <= detail::maxBufferedHtmlEscape)
            {
                auto escaped = detail::EscapeHtmlToBuffer(std::string_view(text.data(), text.size()));
                result = escaped.size() == text.size() && AsStringView<CharT>(val) ? val : InternalValue::FromText(escaped);
                result.SetMarkup();
                return result;
            }
        }
        auto size = detail::EscapedHtmlSize(text);
        if (size == text.size() && AsStringView<CharT>(val))
        {
            result = val;
        }
        else
        {
            result = InternalValue::MakeText<CharT>(size, [text](boost::span<CharT> chars) { detail::WriteEscapedHtml(text, chars); });
        }
        result.SetMarkup();
        return result;
    };
    if (callback->IsWideTarget())
    {
        if (auto view = AsStringView<wchar_t>(val))
        {
            return escape(*view);
        }
    }
    else if (auto view = AsStringView<char>(val))
    {
        return escape(*view);
    }
    // Any other value is escaped from its str()
    auto text = callback->GetAsTargetString(val);
    return std::visit([&escape](const auto& str) { return escape(std::basic_string_view<typename std::decay_t<decltype(str)>::value_type>(str)); }, text);
}

// str(val) as a string value of the template's character type: a string of that type is shared,
// not copied (docs/tasks/0151)
inline InternalValue TextValue(const InternalValue& val, IRendererCallback* callback)
{
    if (callback->IsWideTarget() ? AsStringView<wchar_t>(val).has_value() : AsStringView<char>(val).has_value())
    {
        InternalValue result = val;
        result.SetMarkup(false);
        return result;
    }
    return InternalValue(callback->GetAsTargetString(val));
}

// Markup(str(val))
inline InternalValue MakeMarkup(const InternalValue& val, IRendererCallback* callback)
{
    InternalValue result = TextValue(val, callback);
    result.SetMarkup();
    return result;
}

// Writes the escaped str() of a value that is not Markup. A short narrow string is escaped
// straight into the stream, with no string in between (docs/tasks/0126).
void WriteEscaped(OutStream& stream, const InternalValue& val, IRendererCallback* callback);

// What `{{ val }}` writes: escaped when autoescape is on and the value is not Markup
inline void WriteOutput(OutStream& stream, const InternalValue& val, RenderContext& context)
{
    if (!context.IsOutputAutoescape() || val.IsMarkup())
    {
        stream.WriteValue(val);
        return;
    }
    WriteEscaped(stream, val, context.GetRendererCallback());
}

} // namespace v2
} // namespace jinja2

#endif // JINJA2CPP_SRC_MARKUP_H
