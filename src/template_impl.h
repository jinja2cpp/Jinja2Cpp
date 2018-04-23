#ifndef TEMPLATE_IMPL_H
#define TEMPLATE_IMPL_H

#include <string>
#include "jinja2cpp/value.h"
#include "renderer.h"
#include "template_parser.h"
#include "value_visitors.h"

namespace jinja2
{

class ITemplateImpl
{
public:
    virtual ~ITemplateImpl() {}
};

template<typename CharT>
class TemplateImpl : public ITemplateImpl
{
public:
    TemplateImpl()
    {
    }

    bool Load(std::basic_string<CharT> tpl)
    {
        m_template = std::move(tpl);
        TemplateParser<CharT> parser(m_template);

        auto renderer = parser.Parse();
        if (!renderer)
            return false;

        m_renderer = renderer;
        return true;
    }

    void Render(std::basic_ostream<CharT>& os, const ValuesMap& params)
    {
        if (m_renderer)
        {
            RenderContext context(params);
            OutStream outStream(
            [this, &os](size_t offset, size_t length) {
                os.write(m_template.data() + offset, length);
            },
            [this, &os](const Value& val) {
                boost::apply_visitor(visitors::ValueRenderer<CharT>(os), val.data());
            }
            );
            m_renderer->Render(outStream, context);
        }
    }

private:
    std::basic_string<CharT> m_template;
    RendererPtr m_renderer;
};

} // jinja2

#endif // TEMPLATE_IMPL_H
