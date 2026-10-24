#include "internal_value.h"
#include "markup.h"

#include <jinja2cpp/markup.h>

#include <string_view>

namespace jinja2
{
inline namespace v2
{
namespace
{
// Template text reaches this through Markup parameters of user callables, so the escaped
// text, up to five times longer, stays under the render's maxSequenceSize
template<typename CharT>
BasicMarkup<CharT> EscapeChecked(std::basic_string_view<CharT> text)
{
    CheckSequenceSize(detail::EscapedHtmlSize(text), "an escaped string");
    return BasicMarkup<CharT>(EscapeHtml(text));
}
} // namespace

Markup Escape(std::string_view text)
{
    return EscapeChecked(text);
}

WMarkup Escape(std::wstring_view text)
{
    return EscapeChecked(text);
}
} // namespace v2
} // namespace jinja2
