#ifndef JINJA2CPP_SRC_TEMPLATE_IMPL_H
#define JINJA2CPP_SRC_TEMPLATE_IMPL_H

#include "internal_value.h"
#include "load_settings.h"
#include "make_unexpected.h"
#include "node_arena.h"
#include "recursion_guard.h"
#include "render_context.h"
#include "render_workspace.h"
#include "renderer.h"
#include "statements.h"
#include "template_env_impl.h"
#include "template_parser.h"
#include "undefined.h"
#include "value_visitors.h"

#include <jinja2cpp/error_info.h>
#include <jinja2cpp/template.h>
#include <jinja2cpp/template_env.h>
#include <jinja2cpp/utils/i_comparable.h>
#include <jinja2cpp/value.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <memory>
#include <optional>
#include <random>
#include <string_view>
#include <utility>
#include <variant>

#ifdef JINJA2CPP_WITH_JSON_BINDINGS_BOOST
#include "binding/boost_json_parser.h"
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
#include <boost/predef/other/endian.h>
#include <nonstd/expected.hpp>

#include <list>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <type_traits>
#include <unordered_map>

namespace jinja2
{

// The default globals of every render, looked up after the environment's (global_functions.cpp)
extern const InternalValueMap& GetBuiltinGlobals(bool withI18n);

// The globals of an environment converted for the renders of one thread (docs/tasks/0139)
struct GlobalsSnapshot
{
    uint64_t generation = 0;
    // The converted values refer to it
    std::shared_ptr<const ValuesMap> source;
    InternalValueMap values;
};

// The globals converted last on this thread
inline std::shared_ptr<const GlobalsSnapshot>& ThreadGlobalsSnapshot()
{
    return RenderWorkspace::ForThisThread().Globals();
}

// The globals of `env` for a render on this thread: converted again only when they changed since
// the last render here, or a render changed one in place. The render holds the result, which a
// render it starts may replace
inline std::shared_ptr<const GlobalsSnapshot> GetGlobalsSnapshot(detail::TemplateEnvImpl& env)
{
    auto& cached = ThreadGlobalsSnapshot();
    if (!cached || cached->generation != env.globalsGeneration.load(std::memory_order_acquire))
    {
        auto snapshot = std::make_shared<GlobalsSnapshot>();
        {
            std::shared_lock<std::shared_timed_mutex> lock(env.guard);
            snapshot->generation = env.globalsGeneration.load(std::memory_order_relaxed);
            snapshot->source = env.globalValues;
        }
        for (const auto& [name, value] : *snapshot->source)
        {
            snapshot->values[name] = visit(visitors::InputValueConvertor(false, true), value.data());
        }
        cached = std::move(snapshot);
    }
    return cached;
}

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
    using CharType = CharT;

    explicit TemplateImpl(TemplateEnv* env)
        : m_envHandle(detail::TemplateEnvAccess::MakeHandle(env))
        , m_env(env ? &m_envHandle : nullptr)
        , m_settings(env ? detail::TemplateEnvAccess::GetImpl(*env)->GetLoadSettings() : detail::DefaultLoadSettings())
    {
    }

    // The root of the tree, resolved through Nodes()
    [[nodiscard]] NodeRef<TemplateRenderer> GetRenderer() const { return m_renderer; }
    [[nodiscard]] ArenaView Nodes() const { return m_nodes.View(); }
    auto GetTemplateName() const {};

    // Parses into fresh state and replaces the loaded template only if parsing succeeds: the tree
    // points into its source, so a failed Load keeps the previous source, tree and name together
    std::optional<BasicErrorInfo<CharT>> Load(std::basic_string<CharT> tpl, std::string tplName)
    {
        // On the heap, so the tree's pointers into it survive the hand-over to m_template
        auto source = std::make_unique<std::basic_string<CharT>>(std::move(tpl));
        NormalizeTemplateNewlines(*source, m_settings->settings.keepTrailingNewline);
        using namespace std::string_literals;
        std::string name = tplName.empty() ? "noname.j2tpl"s : std::move(tplName);
        NodeArena nodes;
        TemplateParser<CharT> parser(source.get(), *m_settings, m_env, name, nodes);

        auto parseResult = parser.Parse();
        if (!parseResult)
        {
            return parseResult.error()[0];
        }

        m_nodes = nodes.Seal();
        m_renderer = *parseResult;
        m_template = std::move(source);
        m_metadataInfo = parser.GetMetadataInfo();
        // Last: the parser refers to the name
        m_templateName = std::move(name);
        m_metadata.reset();
        return std::optional<BasicErrorInfo<CharT>>();
    }

    // Renders with the params of a ValuesMap or a GenericMap. Rendering reads the template only, so
    // several threads may render one template at once.
    template<typename ParamsMap>
    std::optional<BasicErrorInfo<CharT>> Render(std::basic_string<CharT>& os, const ParamsMap& params) const
    {
        std::optional<BasicErrorInfo<CharT>> normalResult;

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
            InternalValueMap intParams;

            auto convertParam = [&intParams](const std::string& name, const Value& value) {
                intParams[name] = visit(visitors::InputValueConvertor(false, true), value.data());
            };

            static const InternalValueMap noGlobals;
            const auto globals = m_env ? GetGlobalsSnapshot(*detail::TemplateEnvAccess::GetImpl(*m_env)) : nullptr;

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
                for (const auto& [name, value] : params)
                {
                    convertParam(name, value);
                }
            }
            RendererCallback callback(this);
            callback.SetGlobals(globals.get());
            RenderContext context(intParams, globals ? globals->values : noGlobals, &callback, &GetBuiltinGlobals(m_settings->settings.extensions.i18n));
            context.SetLookupCache(&LookupCache::ForThisThread());
            context.SetNodes(Nodes());
            // The output of earlier renders sizes this one, so that the string does not
            // regrow while it is written (docs/tasks/0100). A hint only: concurrent renders
            // may race on it harmlessly.
            const auto start = os.size();
            const auto hint = m_outputSizeHint.value.load(std::memory_order_relaxed);
            os.reserve(start + hint);
            OutStream outStream(os);
            m_nodes[m_renderer].Render(outStream, context);
            outStream.Flush();
            // Stored only when the output outgrows the hint or needs less than half of it, so
            // that renders of a steady size store nothing: every core rendering the template
            // reads the hint, and a store makes them all fetch it again (docs/tasks/0138). One
            // huge render does not make every later one reserve as much.
            constexpr size_t maxOutputSizeHint = size_t{ 16 } << 20;
            const auto newHint = std::min(os.size() - start, maxOutputSizeHint);
            if (newHint > hint || newHint < hint / 2)
            {
                m_outputSizeHint.value.store(newHint, std::memory_order_relaxed);
            }
        }
        catch (const BasicErrorInfo<char>& error)
        {
            return ErrorConverter<BasicErrorInfo<CharT>, BasicErrorInfo<char>>::Convert(error);
        }
        catch (const BasicErrorInfo<wchar_t>& error)
        {
            return ErrorConverter<BasicErrorInfo<CharT>, BasicErrorInfo<wchar_t>>::Convert(error);
        }
        catch (const RecursionLimitError&)
        {
            typename BasicErrorInfo<CharT>::Data errorData;
            errorData.code = ErrorCode::RecursionLimitExceeded;
            errorData.srcLoc.col = 1;
            errorData.srcLoc.line = 1;
            errorData.srcLoc.fileName = m_templateName;

            return BasicErrorInfo<CharT>(errorData);
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
        if (m_settings->settings != other.m_settings->settings)
        {
            return false;
        }
        // The same source, settings and environment parse to the same tree
        const bool sameSource = m_template && other.m_template ? *m_template == *other.m_template : m_template == other.m_template;
        if (!sameSource)
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

        [[nodiscard]] bool IsWideTarget() const override { return std::is_same_v<CharT, wchar_t>; }

        [[nodiscard]] const Settings& GetSettings() const override { return m_host->m_settings->settings; }
        [[nodiscard]] TemplateEnv* GetEnv() const override { return m_host->m_env; }
        std::minstd_rand& GetRandomEngine() override { return m_random; }
        void GlobalScopeWritten() override
        {
            // The changed globals are this render's: the next one converts them again
            auto& cached = ThreadGlobalsSnapshot();
            if (cached && cached.get() == m_globals)
            {
                cached.reset();
            }
        }
        void SetGlobals(const GlobalsSnapshot* globals) { m_globals = globals; }

        OutStream GetStreamOnString(TargetString& str) override
        {
            using string_t = std::basic_string<CharT>;
            str = string_t();
            return OutStream(std::get<string_t>(str));
        }

        [[nodiscard]] const LoadTemplateResult& LoadTemplate(const std::string& fileName) const override
        {
            auto& loaded = Loaded().byName;
            auto p = loaded.find(fileName);
            if (p == loaded.end())
            {
                auto& entry = loaded.emplace(fileName, LoadedTemplate{ m_host->LoadTemplate(fileName), nullptr }).first->second;
                entry.latest = &entry.first;
                return entry.first;
            }
            auto& entry = p->second;
            if (m_host->m_settings->settings.templateLookup == TemplateLookup::EveryUse)
            {
                auto result = m_host->LoadTemplate(fileName);
                // A reloaded template is kept as a new result: a render may still be running the one it replaces
                if (!IsSameTemplate(result, *entry.latest))
                {
                    entry.latest = &Loaded().replaced.emplace_back(std::move(result));
                }
            }
            return *entry.latest;
        }

        [[nodiscard]] const LoadTemplateResult& LoadTemplate(const InternalValue& fileName) const override
        {
            auto name = GetAsSameString(std::string(), fileName);
            if (!name)
            {
                return Loaded().replaced.emplace_back(m_host->LoadTemplate(fileName));
            }
            return LoadTemplate(name.value());
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
        // What this render has loaded, by name: a template in a loop is looked up in the environment once, not on
        // every iteration (the environment's lock is shared by every thread rendering from it). Node-based, so
        // references handed out stay valid as entries are added
        struct LoadedTemplate
        {
            LoadTemplateResult first;
            // `first`, or the newest entry of `replaced` when TemplateLookup::EveryUse reloaded the template
            const LoadTemplateResult* latest = nullptr;
        };
        // The same template, or both lookups failed (an error is reported the same way each time)
        static bool IsSameTemplate(const LoadTemplateResult& lhs, const LoadTemplateResult& rhs)
        {
            if (lhs.index() != rhs.index())
            {
                return false;
            }
            return std::visit(
                [&rhs](const auto& l) {
                    using T = std::decay_t<decltype(l)>;
                    if constexpr (std::is_same_v<T, EmptyValue>)
                    {
                        return true;
                    }
                    else
                    {
                        const auto& r = std::get<T>(rhs);
                        return l.has_value() == r.has_value() && (!l || l.value() == r.value());
                    }
                },
                lhs);
        }
        struct LoadedTemplates
        {
            std::unordered_map<std::string, LoadedTemplate> byName;
            // Results that are not the first for their name: templates reloaded during the render and invalid names
            std::list<LoadTemplateResult> replaced;
        };
        // Made on the first lookup, so a render that loads nothing does not pay for it
        LoadedTemplates& Loaded() const
        {
            if (!m_loaded)
            {
                m_loaded = std::make_unique<LoadedTemplates>();
            }
            return *m_loaded;
        }
        mutable std::unique_ptr<LoadedTemplates> m_loaded;
        // lipsum's generator: default-seeded, so each render draws the same text
        std::minstd_rand m_random;
        // The globals this render uses, which it keeps alive; only compared
        const GlobalsSnapshot* m_globals = nullptr;
    };

    // Keeps the environment's state alive for as long as the template lives
    TemplateEnv m_envHandle;
    TemplateEnv* m_env{};
    // Shared with the other templates of the environment
    detail::LoadSettingsPtr m_settings;
    std::unique_ptr<std::basic_string<CharT>> m_template;
    std::string m_templateName;
    // Owns the tree
    SealedArena m_nodes;
    NodeRef<TemplateRenderer> m_renderer;
    // The size of the output to reserve. It has cache lines of its own, so that a store to it does
    // not evict the fields around it from the other cores rendering the template (docs/tasks/0138);
    // padded rather than aligned, which would need aligned allocation
    struct OutputSizeHint
    {
        static constexpr size_t CacheLineSize = 64;
        std::array<char, CacheLineSize> padBefore{};
        std::atomic<size_t> value{ 0 };
        std::array<char, CacheLineSize> padAfter{};
    };
    mutable OutputSizeHint m_outputSizeHint;
    mutable std::optional<GenericMap> m_metadata;
    mutable boost::anys::unique_any m_metadataJson;
    mutable std::string m_metadataSource;
    mutable std::mutex m_metadataMutex;
    MetadataInfo<CharT> m_metadataInfo;
};

} // namespace jinja2

#endif // JINJA2CPP_SRC_TEMPLATE_IMPL_H
