#ifndef TEMPLATE_IMPL_H
#define TEMPLATE_IMPL_H

#include "jinja2cpp/value.h"
#include "jinja2cpp/template_env.h"
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


template<typename U>
struct TemplateLoader;

template<>
struct TemplateLoader<char>
{
    static auto Load(const std::string& fileName, TemplateEnv* env)
    {
        return env->LoadTemplate(fileName);
    }
};

template<>
struct TemplateLoader<wchar_t>
{
    static auto Load(const std::string& fileName, TemplateEnv* env)
    {
        return env->LoadTemplateW(fileName);
    }
};

template<typename CharT>
class TemplateImpl : public ITemplateImpl
{
public:
    using ThisType = TemplateImpl<CharT>;

    TemplateImpl(TemplateEnv* env)
        : m_env(env)
    {
    }

    auto GetRenderer() const {return m_renderer;}

    bool Load(std::basic_string<CharT> tpl)
    {
        m_template = std::move(tpl);
        TemplateParser<CharT> parser(&m_template);

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
            RendererCallback callback(this);
            RenderContext context(intParams, &callback);
            OutStream outStream(
            [this, &os](const void* ptr, size_t length) {
                os.write(reinterpret_cast<const CharT*>(ptr), length);
            },
            [this, &os](const InternalValue& val) {
                Apply<visitors::ValueRenderer<CharT>>(val, os);
            }
            );
            m_renderer->Render(outStream, context);
        }
    }

    auto LoadTemplate(const std::string& fileName)
    {
        using ResultType = boost::variant<EmptyValue, std::shared_ptr<TemplateImpl<char>>, std::shared_ptr<TemplateImpl<wchar_t>>>;

        if (!m_env)
            return ResultType(EmptyValue());

        auto tplWrapper = TemplateLoader<CharT>::Load(fileName, m_env);
        return ResultType(std::static_pointer_cast<ThisType>(tplWrapper.m_impl));
    }

    class RendererCallback : public IRendererCallback
    {
    public:
        RendererCallback(ThisType* host)
            : m_host(host)
        {}

        TargetString GetAsTargetString(const InternalValue& val) override
        {
            std::basic_ostringstream<CharT> os;
            Apply<visitors::ValueRenderer<CharT>>(val, os);
            return TargetString(os.str());
        }

        boost::variant<EmptyValue, std::shared_ptr<TemplateImpl<char>>, std::shared_ptr<TemplateImpl<wchar_t>>> LoadTemplate(const std::string& fileName) const override
        {
            return m_host->LoadTemplate(fileName);
        }

    private:
        ThisType* m_host;
    };

private:
    TemplateEnv* m_env;
    std::basic_string<CharT> m_template;
    RendererPtr m_renderer;
};

} // jinja2

#endif // TEMPLATE_IMPL_H
