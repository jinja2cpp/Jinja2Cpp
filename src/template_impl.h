#ifndef JINJA2CPP_SRC_TEMPLATE_IMPL_H
#define JINJA2CPP_SRC_TEMPLATE_IMPL_H

#include "internal_value.h"
#include "make_unexpected.h"
#include "jinja2cpp/template_env.h"
#include "template_env_impl.h"
#include "jinja2cpp/value.h"
#include "renderer.h"
#include "template_parser.h"
#include "value_visitors.h"

#ifdef JINJA2CPP_WITH_JSON_BINDINGS_BOOST
#include "binding/boost_json_parser.h"
#include "binding/boost_json_serializer.h"
#include "jinja2cpp/binding/boost_json.h"
#elif JINJA2CPP_WITH_JSON_BINDINGS_NLOHMANN
#include "binding/nlohmann_json_parser.h"
#include "jinja2cpp/binding/nlohmann_json.h"
#else
#include "binding/rapid_json_parser.h"
#include "jinja2cpp/binding/rapid_json.h"
#endif

#include <boost/any.hpp>
#include <boost/any/unique_any.hpp>
#include <boost/optional.hpp>
#include <boost/predef/other/endian.h>
#include <nonstd/expected.hpp>

#include <list>
#include <mutex>
#include <string>
#include <type_traits>

namespace jinja2
{

extern void SetupGlobals(InternalValueMap& globalParams);
extern void SetupI18nGlobals(InternalValueMap& globalParams);

class ITemplateImpl
{
public:
    virtual ~ITemplateImpl() = default;
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
    explicit GenericStreamWriter(std::basic_string<CharT>& os)
        : m_os(os)
    {}

    // StreamWriter interface
    void WriteBuffer(const void* ptr, size_t length) override
    {
        m_os.append(reinterpret_cast<const CharT*>(ptr), length);
    }
    void WriteValue(const InternalValue& val) override
    {
        Apply<visitors::ValueRenderer<CharT>>(val, m_os);
    }

private:
    std::basic_string<CharT>& m_os;
};

template<typename CharT>
class StringStreamWriter : public OutStream::StreamWriter
{
public:
    explicit StringStreamWriter(std::basic_string<CharT>* targetStr)
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
        Apply<visitors::ValueRenderer<CharT>>(val, *m_targetStr);
    }

private:
    std::basic_string<CharT>* m_targetStr;
};

template<typename ErrorTpl1, typename ErrorTpl2>
struct ErrorConverter;

template<typename CharT1, typename CharT2>
struct ErrorConverter<BasicErrorInfo<CharT1>, BasicErrorInfo<CharT2>>
{
    static BasicErrorInfo<CharT1> Convert(const BasicErrorInfo<CharT2>& srcError)
    {
        typename BasicErrorInfo<CharT1>::Data errorData;
        errorData.code = srcError.GetCode();
        errorData.srcLoc = srcError.GetErrorLocation();
        errorData.locationDescr = ConvertString<std::basic_string<CharT1>>(srcError.GetLocationDescr());
        errorData.extraParams = srcError.GetExtraParams();

        return BasicErrorInfo<CharT1>(errorData);
    }
};

template<typename CharT>
struct ErrorConverter<BasicErrorInfo<CharT>, BasicErrorInfo<CharT>>
{
    static BasicErrorInfo<CharT> Convert(const BasicErrorInfo<CharT>& srcError)
    {
        return srcError;
    }
};

template<typename CharT>
inline bool operator==(const MetadataInfo<CharT>& lhs, const MetadataInfo<CharT>& rhs)
{
    if (lhs.metadata != rhs.metadata)
    {
        return false;
    }
    if (lhs.metadataType != rhs.metadataType)
    {
        return false;
    }
    if (lhs.location != rhs.location)
    {
        return false;
    }
    return true;
}

template<typename CharT>
inline bool operator!=(const MetadataInfo<CharT>& lhs, const MetadataInfo<CharT>& rhs)
{
    return !(lhs == rhs);
}

inline bool operator==(const TemplateEnv& lhs, const TemplateEnv& rhs)
{
    return detail::TemplateEnvAccess::GetImpl(lhs)->IsEqual(*detail::TemplateEnvAccess::GetImpl(rhs));
}
inline bool operator!=(const TemplateEnv& lhs, const TemplateEnv& rhs)
{
    return !(lhs == rhs);
}

inline bool operator==(const SourceLocation& lhs, const SourceLocation& rhs)
{
    if (lhs.fileName != rhs.fileName)
    {
        return false;
    }
    if (lhs.line != rhs.line)
    {
        return false;
    }
    if (lhs.col != rhs.col)
    {
        return false;
    }
    return true;
}
inline bool operator!=(const SourceLocation& lhs, const SourceLocation& rhs)
{
    return !(lhs == rhs);
}

template<typename CharT>
class TemplateImpl : public ITemplateImpl
{
public:
    using ThisType = TemplateImpl<CharT>;

    explicit TemplateImpl(TemplateEnv* env)
        : m_envHandle(env ? detail::TemplateEnvAccess::MakeHandle(*env) : nullptr)
        , m_env(m_envHandle.get())
    {
        if (env)
        {
            m_settings = env->GetSettings();
        }
    }

    auto GetRenderer() const { return m_renderer; }
    auto GetTemplateName() const {};

    boost::optional<BasicErrorInfo<CharT>> Load(std::basic_string<CharT> tpl, std::string tplName)
    {
        m_template = std::move(tpl);
        NormalizeTemplateNewlines(m_template, m_settings.keepTrailingNewline);
        m_templateName = tplName.empty() ? std::string("noname.j2tpl") : std::move(tplName);
        TemplateParser<CharT> parser(&m_template, m_settings, m_env, m_templateName);

        auto parseResult = parser.Parse();
        if (!parseResult)
        {
            return parseResult.error()[0];
        }

        m_renderer = *parseResult;
        m_metadataInfo = parser.GetMetadataInfo();
        m_metadata.reset();
        return boost::optional<BasicErrorInfo<CharT>>();
    }

    // Renders with the params of a ValuesMap or a GenericMap. Rendering reads the template only, so
    // several threads may render one template at once.
    template<typename ParamsMap>
    boost::optional<BasicErrorInfo<CharT>> Render(std::basic_string<CharT>& os, const ParamsMap& params) const
    {
        boost::optional<BasicErrorInfo<CharT>> normalResult;

        if (!m_renderer)
        {
            typename BasicErrorInfo<CharT>::Data errorData;
            errorData.code = ErrorCode::TemplateNotParsed;
            errorData.srcLoc.col = 1;
            errorData.srcLoc.line = 1;
            errorData.srcLoc.fileName = "<unknown file>";

            return BasicErrorInfo<CharT>(errorData);
        }

        try
        {
            InternalValueMap extParams;
            InternalValueMap intParams;

            auto convertParam = [&intParams](const std::string& name, const Value& value) {
                intParams[name] = visit(visitors::InputValueConvertor(false, true), value.data());
            };
            auto convertFn = [&convertParam](const ValuesMap& params) {
                for (const auto& ip : params)
                {
                    convertParam(ip.first, ip.second);
                }
            };

            if (m_env)
            {
                m_env->ApplyGlobals(convertFn);
                std::swap(extParams, intParams);
            }

            // A GenericMap returns values by copy; the context refers to them, so they live here
            std::list<Value> genericValues;
            if constexpr (std::is_same_v<ParamsMap, GenericMap>)
            {
                for (auto& name : params.GetKeys())
                {
                    convertParam(name, genericValues.emplace_back(params.GetValueByName(name)));
                }
            }
            else
            {
                convertFn(params);
            }
            SetupGlobals(extParams);
            if (m_settings.extensions.i18n)
            {
                SetupI18nGlobals(extParams);
            }

            RendererCallback callback(this);
            RenderContext context(intParams, extParams, &callback);
            InitRenderContext(context);
            OutStream outStream([writer = GenericStreamWriter<CharT>(os)]() mutable -> OutStream::StreamWriter* { return &writer; });
            m_renderer->Render(outStream, context);
        }
        catch (const BasicErrorInfo<char>& error)
        {
            return ErrorConverter<BasicErrorInfo<CharT>, BasicErrorInfo<char>>::Convert(error);
        }
        catch (const BasicErrorInfo<wchar_t>& error)
        {
            return ErrorConverter<BasicErrorInfo<CharT>, BasicErrorInfo<wchar_t>>::Convert(error);
        }
        catch (const UndefinedError& ex)
        {
            typename BasicErrorInfo<CharT>::Data errorData;
            errorData.code = ErrorCode::UndefinedError;
            errorData.srcLoc.col = 1;
            errorData.srcLoc.line = 1;
            errorData.srcLoc.fileName = m_templateName;
            errorData.extraParams.push_back(Value(std::string(ex.what())));

            return BasicErrorInfo<CharT>(errorData);
        }
        catch (const std::exception& ex)
        {
            typename BasicErrorInfo<CharT>::Data errorData;
            errorData.code = ErrorCode::UnexpectedException;
            errorData.srcLoc.col = 1;
            errorData.srcLoc.line = 1;
            errorData.srcLoc.fileName = m_templateName;
            errorData.extraParams.push_back(Value(std::string(ex.what())));

            return BasicErrorInfo<CharT>(errorData);
        }

        return normalResult;
    }

    static InternalValueMap& InitRenderContext(RenderContext& context)
    {
        auto& curScope = context.GetCurrentScope();
        return curScope;
    }

    using TplLoadResultType = std::variant<EmptyValue,
                                           nonstd::expected<std::shared_ptr<TemplateImpl<char>>, ErrorInfo>,
                                           nonstd::expected<std::shared_ptr<TemplateImpl<wchar_t>>, ErrorInfoW>>;

    using TplOrError = nonstd::expected<std::shared_ptr<TemplateImpl<CharT>>, BasicErrorInfo<CharT>>;

    TplLoadResultType LoadTemplate(const std::string& fileName) const
    {
        if (!m_env)
        {
            return TplLoadResultType(EmptyValue());
        }

        auto tplWrapper = TemplateLoader<CharT>::Load(fileName, m_env);
        if (!tplWrapper)
        {
            return TplLoadResultType(TplOrError(MakeUnexpected(tplWrapper.error())));
        }

        return TplLoadResultType(TplOrError(std::static_pointer_cast<ThisType>(tplWrapper.value().m_impl)));
    }

    TplLoadResultType LoadTemplate(const InternalValue& fileName) const
    {
        auto name = GetAsSameString(std::string(), fileName);
        if (!name)
        {
            typename BasicErrorInfo<CharT>::Data errorData;
            errorData.code = ErrorCode::InvalidTemplateName;
            errorData.srcLoc.col = 1;
            errorData.srcLoc.line = 1;
            errorData.srcLoc.fileName = m_templateName;
            errorData.extraParams.push_back(IntValue2Value(fileName));
            return TplOrError(MakeUnexpected(BasicErrorInfo<CharT>(errorData)));
        }

        return LoadTemplate(name.value());
    }

    nonstd::expected<GenericMap, BasicErrorInfo<CharT>> GetMetadata() const
    {
        // The JSON document is parsed once and kept in the template: the returned maps may refer to
        // it (the RapidJSON binding does), so it must outlive every map handed out
        std::scoped_lock lock(m_metadataMutex);
        if (m_metadata)
        {
            return m_metadata.value();
        }

        auto& metadataString = m_metadataInfo.metadata;
        if (metadataString.empty())
        {
            return GenericMap();
        }

        if (m_metadataInfo.metadataType == "json")
        {
            // The JSON bindings parse narrow strings; wide metadata is converted first and kept,
            // since the parsed document may refer to its source
            std::string_view narrowMetadata;
            if constexpr (std::is_same_v<CharT, char>)
            {
                narrowMetadata = metadataString;
            }
            else
            {
                m_metadataSource = ConvertString<std::string>(metadataString);
                narrowMetadata = m_metadataSource;
            }
            auto result = Parse<char>(narrowMetadata, m_metadataJson);
            if (!result)
            {
                typename BasicErrorInfo<CharT>::Data errorData;
                errorData.code = ErrorCode::MetadataParseError;
                errorData.srcLoc = m_metadataInfo.location;
                errorData.extraParams.push_back(Value(result.error()));
                return MakeUnexpected(BasicErrorInfo<CharT>(errorData));
            }
            m_metadata = std::move(std::get<GenericMap>(result.value().data()));
            return m_metadata.value();
        }
        return GenericMap();
    }

    nonstd::expected<MetadataInfo<CharT>, BasicErrorInfo<CharT>> GetMetadataRaw() const { return m_metadataInfo; }

    bool operator==(const TemplateImpl<CharT>& other) const
    {
        if (m_env && other.m_env)
        {
            if (*m_env != *other.m_env)
            {
                return false;
            }
        }
        if (m_settings != other.m_settings)
        {
            return false;
        }
        if (m_template != other.m_template)
        {
            return false;
        }
        if (m_renderer && other.m_renderer && !m_renderer->IsEqual(*other.m_renderer))
        {
            return false;
        }
        if (m_metadata != other.m_metadata)
        {
            return false;
        }
        // m_metadataJson - only for persistence purposes
        //if (m_metadataJson != other.m_metadataJson)
        //    return false;
        if (m_metadataInfo != other.m_metadataInfo)
        {
            return false;
        }
        return true;
    }
private:
    [[noreturn]] void ThrowRuntimeError(ErrorCode code, ValuesList extraParams) const
    {
        typename BasicErrorInfo<CharT>::Data errorData;
        errorData.code = code;
        errorData.srcLoc.col = 1;
        errorData.srcLoc.line = 1;
        errorData.srcLoc.fileName = m_templateName;
        errorData.extraParams = std::move(extraParams);

        throw BasicErrorInfo<CharT>(std::move(errorData));
    }

    class RendererCallback : public IRendererCallback
    {
    public:
        explicit RendererCallback(const ThisType* host)
            : m_host(host)
        {}

        TargetString GetAsTargetString(const InternalValue& val) override
        {
            std::basic_string<CharT> os;
            Apply<visitors::ValueRenderer<CharT>>(val, os);
            return TargetString(std::move(os));
        }

        [[nodiscard]] const Settings& GetSettings() const override { return m_host->m_settings; }
        [[nodiscard]] TemplateEnv* GetEnv() const override { return m_host->m_env; }

        OutStream GetStreamOnString(TargetString& str) override
        {
            using string_t = std::basic_string<CharT>;
            str = string_t();
            return OutStream([writer = StringStreamWriter<CharT>(&std::get<string_t>(str))]() mutable -> OutStream::StreamWriter* { return &writer; });
        }

        [[nodiscard]] std::variant<EmptyValue,
                                   nonstd::expected<std::shared_ptr<TemplateImpl<char>>, ErrorInfo>,
                                   nonstd::expected<std::shared_ptr<TemplateImpl<wchar_t>>, ErrorInfoW>>
        LoadTemplate(const std::string& fileName) const override
        {
            return m_host->LoadTemplate(fileName);
        }

        [[nodiscard]] std::variant<EmptyValue,
                                   nonstd::expected<std::shared_ptr<TemplateImpl<char>>, ErrorInfo>,
                                   nonstd::expected<std::shared_ptr<TemplateImpl<wchar_t>>, ErrorInfoW>>
        LoadTemplate(const InternalValue& fileName) const override
        {
            return m_host->LoadTemplate(fileName);
        }

        [[noreturn]] void ThrowRuntimeError(ErrorCode code, ValuesList extraParams) override
        {
            m_host->ThrowRuntimeError(code, std::move(extraParams));
        }

        [[nodiscard]] bool IsEqual(const IComparable& other) const override
        {
            auto* callback = dynamic_cast<const RendererCallback*>(&other);
            if (!callback)
            {
                return false;
            }
            if (m_host && callback->m_host)
            {
                return *m_host == *(callback->m_host);
            }
            if ((!m_host && (callback->m_host)) || (m_host && !(callback->m_host)))
            {
                return false;
            }
            return true;
        }
        bool operator==(const IComparable& other) const
        {
            auto* callback = dynamic_cast<const RendererCallback*>(&other);
            if (!callback)
            {
                return false;
            }
            if (m_host && callback->m_host)
            {
                return *m_host == *(callback->m_host);
            }
            if ((!m_host && (callback->m_host)) || (m_host && !(callback->m_host)))
            {
                return false;
            }
            return true;
        }

    private:
        const ThisType* m_host{};
    };

    // Keeps the environment's state alive for as long as the template lives
    std::unique_ptr<TemplateEnv> m_envHandle;
    TemplateEnv* m_env{};
    Settings m_settings;
    std::basic_string<CharT> m_template;
    std::string m_templateName;
    RendererPtr m_renderer;
    mutable std::optional<GenericMap> m_metadata;
    mutable boost::anys::unique_any m_metadataJson;
    mutable std::string m_metadataSource;
    mutable std::mutex m_metadataMutex;
    MetadataInfo<CharT> m_metadataInfo;
};

} // namespace jinja2

#endif // JINJA2CPP_SRC_TEMPLATE_IMPL_H
