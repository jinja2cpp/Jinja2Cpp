#include "markup.h"

#include <jinja2cpp/markup.h>

#include <string_view>

namespace jinja2
{
inline namespace v2
{
Markup Escape(std::string_view text)
{
    return Markup(EscapeHtml(text));
}

WMarkup Escape(std::wstring_view text)
{
    return WMarkup(EscapeHtml(text));
}
} // namespace v2
} // namespace jinja2
