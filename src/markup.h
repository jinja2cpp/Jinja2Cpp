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

// Escapes a narrow string in one pass, without a capacity check per byte. The view is valid until
// the next call on the same thread.
inline std::string_view EscapeHtmlToBuffer(std::string_view str)
{
    thread_local std::string buffer;
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

template<typename CharT>
std::basic_string<CharT> WriteEscapedHtml(std::basic_string_view<CharT> str, size_t size)
{
    std::basic_string<CharT> result(size, CharT{});
    auto* out = result.data();
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
    return result;
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
    return detail::WriteEscapedHtml(str, size);
}

// The same, reusing the string when nothing in it needs escaping
template<typename CharT>
std::basic_string<CharT> EscapeHtml(std::basic_string<CharT>&& str)
{
    std::basic_string_view<CharT> view(str);
    if constexpr (sizeof(CharT) == 1)
    {
        if (str.size() <= detail::maxBufferedHtmlEscape)
        {
            auto escaped = detail::EscapeHtmlToBuffer(std::string_view(str.data(), str.size()));
            if (escaped.size() == str.size())
            {
                return std::move(str);
            }
            return std::basic_string<CharT>(escaped.data(), escaped.size());
        }
    }
    auto size = detail::EscapedHtmlSize(view);
    if (size == str.size())
    {
        return std::move(str);
    }
    return detail::WriteEscapedHtml(view, size);
}

inline TargetString EscapeHtml(const TargetString& str)
{
    if (const auto* narrow = std::get_if<std::string>(&str))
    {
        return EscapeHtml(std::string_view(*narrow));
    }
    return EscapeHtml(std::wstring_view(std::get<std::wstring>(str)));
}

inline TargetString EscapeHtml(TargetString&& str)
{
    return std::visit([](auto&& alt) -> TargetString { return EscapeHtml(std::forward<decltype(alt)>(alt)); }, std::move(str));
}

inline bool IsStringValue(const InternalValue& val)
{
    const auto& data = val.GetData();
    return std::get_if<std::string>(&data) != nullptr || std::get_if<TargetString>(&data) != nullptr || std::get_if<TargetStringView>(&data) != nullptr;
}

// The text of a string value of character type CharT, which its str() renders unchanged
template<typename CharT>
bool GetStringView(const InternalValue& val, std::basic_string_view<CharT>& view)
{
    const auto& data = val.GetData();
    if constexpr (std::is_same_v<CharT, char>)
    {
        if (const auto* str = std::get_if<std::string>(&data))
        {
            view = *str;
            return true;
        }
    }
    if (const auto* str = std::get_if<TargetString>(&data))
    {
        if (const auto* alt = std::get_if<std::basic_string<CharT>>(str))
        {
            view = *alt;
            return true;
        }
        return false;
    }
    if (const auto* str = std::get_if<TargetStringView>(&data))
    {
        if (const auto* alt = std::get_if<std::basic_string_view<CharT>>(str))
        {
            view = *alt;
            return true;
        }
    }
    return false;
}

// markupsafe.escape: Markup is returned as is, anything else becomes Markup of its escaped str().
// A string of the template's character type is escaped from its own text (docs/tasks/0126).
inline InternalValue MarkupEscape(const InternalValue& val, IRendererCallback* callback)
{
    if (val.IsMarkup())
    {
        return val;
    }
    auto escaped = [callback, &val]() -> TargetString {
        if (callback->IsWideTarget())
        {
            std::wstring_view view;
            if (GetStringView(val, view))
            {
                return EscapeHtml(view);
            }
        }
        else
        {
            std::string_view view;
            if (GetStringView(val, view))
            {
                return EscapeHtml(view);
            }
        }
        return EscapeHtml(callback->GetAsTargetString(val));
    };
    InternalValue result(escaped());
    result.SetMarkup();
    return result;
}

// Markup(str(val))
inline InternalValue MakeMarkup(const InternalValue& val, IRendererCallback* callback)
{
    InternalValue result(callback->GetAsTargetString(val));
    result.SetMarkup();
    return result;
}

// An argument of Markup's `%` and format() (markupsafe's _MarkupEscapeHelper): numbers,
// bools and None stay as they are, anything else becomes the escaped str() of it
inline InternalValue EscapeFormatArg(const InternalValue& val, IRendererCallback* callback)
{
    const auto& data = val.GetData();
    if (val.IsUndefined() || val.IsNone() || std::get_if<int64_t>(&data) || std::get_if<double>(&data) || std::get_if<bool>(&data))
    {
        return val;
    }
    return MarkupEscape(val, callback);
}

// The right operand of Markup's `%`: a tuple's items and a mapping's values are escaped
// one by one, any other value as a whole
inline InternalValue EscapeFormatArgs(const InternalValue& args, IRendererCallback* callback)
{
    const auto* list = std::get_if<ListAdapter>(&args.GetData());
    if (list && list->IsTuple())
    {
        InternalValueList items;
        for (const auto& item : *list)
        {
            items.push_back(EscapeFormatArg(item, callback));
        }
        return ListAdapter::CreateAdapter(std::move(items)).MarkAsTuple();
    }
    if (const auto* map = std::get_if<MapAdapter>(&args.GetData()))
    {
        InternalValueMap items;
        for (auto& key : map->GetKeys())
        {
            items[key] = EscapeFormatArg(map->GetValueByName(key), callback);
        }
        return CreateMapAdapter(std::move(items));
    }
    return EscapeFormatArg(args, callback);
}

// Writes the escaped str() of a value that is not Markup. A short narrow string is escaped
// straight into the stream, with no string in between (docs/tasks/0126).
void WriteEscaped(OutStream& stream, const InternalValue& val, IRendererCallback* callback);

// What `{{ val }}` writes: escaped when autoescape is on and the value is not Markup
inline void WriteOutput(OutStream& stream, const InternalValue& val, RenderContext& context)
{
    if (!context.IsAutoescape() || val.IsMarkup())
    {
        stream.WriteValue(val);
        return;
    }
    WriteEscaped(stream, val, context.GetRendererCallback());
}

} // namespace jinja2

#endif // JINJA2CPP_SRC_MARKUP_H
