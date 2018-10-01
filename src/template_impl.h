#ifndef TEMPLATE_IMPL_H
#define TEMPLATE_IMPL_H

#include "jinja2cpp/value.h"
#include "jinja2cpp/template_env.h"
#include "internal_value.h"
#include "renderer.h"
#include "template_parser.h"
#include "value_visitors.h"

#include <boost/optional.hpp>
#include <nonstd/expected.hpp>
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
class GenericStreamWriter : public OutStream::StreamWriter
{
public:
    GenericStreamWriter(std::basic_ostream<CharT>& os)
        : m_os(os)
    {}

    // StreamWriter interface
    void WriteBuffer(const void* ptr, size_t length) override
    {
        m_os.write(reinterpret_cast<const CharT*>(ptr), length);
    }
    void WriteValue(const InternalValue& val) override
    {
        Apply<visitors::ValueRenderer<CharT>>(val, m_os);
    }

private:
    std::basic_ostream<CharT>& m_os;
};

template<typename CharT>
class StringStreamWriter : public OutStream::StreamWriter
{
public:
    StringStreamWriter(std::basic_string<CharT>* targetStr)
        : m_targetStr(targetStr)
    {}

    // StreamWriter interface
    void WriteBuffer(const void* ptr, size_t length) override
    {
        m_targetStr->append(reinterpret_cast<const CharT*>(ptr), length);
        // m_os.write(reinterpret_cast<const CharT*>(ptr), length);
    }
    void WriteValue(const InternalValue& val) override
    {
        std::basic_ostringstream<CharT> os;
        Apply<visitors::ValueRenderer<CharT>>(val, os);
        (*m_targetStr) += os.str();
    }

private:
    std::basic_string<CharT>* m_targetStr;
};

template<typename CharT>
class TemplateImpl : public ITemplateImpl
{
public:
    using ThisType = TemplateImpl<CharT>;

    TemplateImpl(TemplateEnv* env)
        : m_env(env)
    {
        if (env)
            m_settings = env->GetSettings();
    }

    auto GetRenderer() const {return m_renderer;}

    boost::optional<ErrorInfoTpl<CharT>> Load(std::basic_string<CharT> tpl, std::string tplName)
    {
        m_template = std::move(tpl);
        TemplateParser<CharT> parser(&m_template, m_settings, tplName.empty() ? std::string("noname.j2tpl") : std::move(tplName));

        auto parseResult = parser.Parse();
        if (!parseResult)
            return parseResult.error()[0];

        m_renderer = *parseResult;
        return boost::optional<ErrorInfoTpl<CharT>>();
    }

    void Render(std::basic_ostream<CharT>& os, const ValuesMap& params)
    {
        if (!m_renderer)
            return;

        InternalValueMap intParams;
        for (auto& ip : params)
        {
            auto valRef = &ip.second.data();
            auto newParam = visit(visitors::InputValueConvertor(), *valRef);
            if (!newParam)
                intParams[ip.first] = std::move(ValueRef(static_cast<const Value&>(*valRef)));
            else
                intParams[ip.first] = newParam.get();
        }
        RendererCallback callback(this);
        RenderContext context(intParams, &callback);
        InitRenderContext(context);
        OutStream outStream([writer = GenericStreamWriter<CharT>(os)]() mutable -> OutStream::StreamWriter* {return &writer;});

        m_renderer->Render(outStream, context);
    }

    InternalValueMap& InitRenderContext(RenderContext& context)
    {
        auto& curScope = context.GetCurrentScope();
        return curScope;
    }

    auto LoadTemplate(const std::string& fileName)
    {
        using ResultType = nonstd::variant<EmptyValue,
            nonstd::expected<std::shared_ptr<TemplateImpl<char>>, ErrorInfo>,
            nonstd::expected<std::shared_ptr<TemplateImpl<wchar_t>>, ErrorInfoW>>;

        using TplOrError = nonstd::expected<std::shared_ptr<TemplateImpl<CharT>>, ErrorInfoTpl<CharT>>;

        if (!m_env)
            return ResultType(EmptyValue());

        auto tplWrapper = TemplateLoader<CharT>::Load(fileName, m_env);
        if (!tplWrapper)
            return ResultType(TplOrError(tplWrapper.get_unexpected()));

        return ResultType(TplOrError(std::static_pointer_cast<ThisType>(tplWrapper.value().m_impl)));
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

        OutStream GetStreamOnString(TargetString& str) override
        {
            using string_t = std::basic_string<CharT>;
            str = string_t();
            return OutStream([writer = StringStreamWriter<CharT>(&str.get<string_t>())]() mutable -> OutStream::StreamWriter* {return &writer;});
        }

        nonstd::variant<EmptyValue,
            nonstd::expected<std::shared_ptr<TemplateImpl<char>>, ErrorInfo>,
            nonstd::expected<std::shared_ptr<TemplateImpl<wchar_t>>, ErrorInfoW>> LoadTemplate(const std::string& fileName) const override
        {
            return m_host->LoadTemplate(fileName);
        }

    private:
        ThisType* m_host;
    };

private:
    TemplateEnv* m_env;
    Settings m_settings;
    std::basic_string<CharT> m_template;
    RendererPtr m_renderer;
};

} // jinja2

#endif // TEMPLATE_IMPL_H
