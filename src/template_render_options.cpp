#include "template_impl.h"

#include <jinja2cpp/error_info.h>
#include <jinja2cpp/template.h>
#include <jinja2cpp/value.h>

#include <optional>
#include <string>

namespace jinja2
{
inline namespace v2
{
// Template::RenderTo with RenderOptions (docs/tasks/0357 B3). Kept out of template.cpp: compiled there, the
// RenderCore with hooks made GCC outline part of the ordinary render's ~RenderContext (+14 instructions a render)
template<typename CharT>
template<typename Target>
std::optional<BasicErrorInfo<CharT>> TemplateImpl<CharT>::RenderWithOptions(Target& os, const ValuesMap& params, const RenderOptions& options) const
{
    RenderHooks hooks;
    if (options.globals)
    {
        hooks.hostGlobals = &options.globals;
    }
    if (options.poll)
    {
        hooks.poll = &options.poll;
    }
    return RenderCore<true>(os, params, &hooks);
}

template std::optional<BasicErrorInfo<char>> TemplateImpl<char>::RenderWithOptions(std::string&, const ValuesMap&, const RenderOptions&) const;
template std::optional<BasicErrorInfo<char>> TemplateImpl<char>::RenderWithOptions(BasicOutputSink<char>&, const ValuesMap&, const RenderOptions&) const;
template std::optional<BasicErrorInfo<wchar_t>> TemplateImpl<wchar_t>::RenderWithOptions(std::wstring&, const ValuesMap&, const RenderOptions&) const;
template std::optional<BasicErrorInfo<wchar_t>> TemplateImpl<wchar_t>::RenderWithOptions(BasicOutputSink<wchar_t>&, const ValuesMap&, const RenderOptions&) const;
} // namespace v2
} // namespace jinja2
