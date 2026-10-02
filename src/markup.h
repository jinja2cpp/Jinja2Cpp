#ifndef JINJA2CPP_SRC_MARKUP_H
#define JINJA2CPP_SRC_MARKUP_H

#include "internal_value.h"
#include "render_context.h"

#include <string>

namespace jinja2
{

// markupsafe.escape of a string
template<typename CharT>
std::basic_string<CharT> EscapeHtml(std::basic_string_view<CharT> str)
{
    std::basic_string<CharT> result;
    result.reserve(str.size());
    auto append = [&result](const char* entity) {
        for (; *entity; ++entity)
            result.push_back(static_cast<CharT>(*entity));
    };
    for (auto ch : str)
    {
        switch (ch)
        {
        case '<':
            append("&lt;");
            break;
        case '>':
            append("&gt;");
            break;
        case '&':
            append("&amp;");
            break;
        case '\'':
            append("&#39;");
            break;
        case '\"':
            append("&#34;");
            break;
        default:
            result.push_back(ch);
            break;
        }
    }
    return result;
}

inline TargetString EscapeHtml(const TargetString& str)
{
    if (const auto* narrow = std::get_if<std::string>(&str))
        return EscapeHtml(std::string_view(*narrow));
    return EscapeHtml(std::wstring_view(std::get<std::wstring>(str)));
}

inline bool IsStringValue(const InternalValue& val)
{
    const auto& data = val.GetData();
    return std::get_if<std::string>(&data) != nullptr || std::get_if<TargetString>(&data) != nullptr || std::get_if<TargetStringView>(&data) != nullptr;
}

// markupsafe.escape: Markup is returned as is, anything else becomes Markup of its escaped str()
inline InternalValue MarkupEscape(const InternalValue& val, IRendererCallback* callback)
{
    if (val.IsMarkup())
        return val;
    InternalValue result(EscapeHtml(callback->GetAsTargetString(val)));
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
    if (val.IsUndefined() || val.IsNone() || std::get_if<int64_t>(&data) != nullptr || std::get_if<double>(&data) != nullptr || std::get_if<bool>(&data) != nullptr)
        return val;
    return MarkupEscape(val, callback);
}

// The right operand of Markup's `%`: a tuple's items and a mapping's values are escaped
// one by one, any other value as a whole
inline InternalValue EscapeFormatArgs(const InternalValue& args, IRendererCallback* callback)
{
    const auto* list = std::get_if<ListAdapter>(&args.GetData());
    if (list != nullptr && list->IsTuple())
    {
        InternalValueList items;
        for (const auto& item : *list)
            items.push_back(EscapeFormatArg(item, callback));
        return ListAdapter::CreateAdapter(std::move(items)).MarkAsTuple();
    }
    if (const auto* map = std::get_if<MapAdapter>(&args.GetData()))
    {
        InternalValueMap items;
        for (auto& key : map->GetKeys())
            items[key] = EscapeFormatArg(map->GetValueByName(key), callback);
        return CreateMapAdapter(std::move(items));
    }
    return EscapeFormatArg(args, callback);
}

// What `{{ val }}` writes: escaped when autoescape is on and the value is not Markup
inline InternalValue OutputValue(InternalValue val, RenderContext& context)
{
    if (!context.IsAutoescape() || val.IsMarkup())
        return val;
    return MarkupEscape(val, context.GetRendererCallback());
}

} // namespace jinja2

#endif // JINJA2CPP_SRC_MARKUP_H
