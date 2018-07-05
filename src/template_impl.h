#ifndef TEMPLATE_IMPL_H
#define TEMPLATE_IMPL_H

#include "jinja2cpp/value.h"
#include "internal_value.h"
#include "renderer.h"
#include "template_parser.h"
#include "value_visitors.h"

#include <boost/optional.hpp>
#include <string>


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
            InternalValueMap intParams;
            for (auto& ip : params)
            {
                auto valRef = &ip.second.data();
                auto newParam = boost::apply_visitor(visitors::InputValueConvertor(), *valRef);
                if (!newParam)
                    intParams[ip.first] = std::move(ValueRef(static_cast<const Value&>(*valRef)));
                else
                    intParams[ip.first] = newParam.get();
            }
            RendererCallback callback;
            RenderContext context(intParams, &callback);
            OutStream outStream(
            [this, &os](size_t offset, size_t length) {
                os.write(m_template.data() + offset, length);
            },
            [this, &os](const InternalValue& val) {
                Apply<visitors::ValueRenderer<CharT>>(val, os);
            }
            );
            m_renderer->Render(outStream, context);
        }
    }

    class RendererCallback : public IRendererCallback
    {
    public:
        TargetString GetAsTargetString(const InternalValue& val) override
        {
            std::basic_ostringstream<CharT> os;
            Apply<visitors::ValueRenderer<CharT>>(val, os);
            return TargetString(os.str());
        }
    };

private:

    std::basic_string<CharT> m_template;
    RendererPtr m_renderer;
};

} // jinja2

#endif // TEMPLATE_IMPL_H
