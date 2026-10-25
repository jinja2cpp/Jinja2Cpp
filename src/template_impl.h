#ifndef JINJA2CPP_SRC_TEMPLATE_IMPL_H
#define JINJA2CPP_SRC_TEMPLATE_IMPL_H

#include "cycle_collector.h"
#include "error_boundary.h"
#include "internal_value.h"
#include "json_reader.h"
#include "load_settings.h"
#include "make_unexpected.h"
#include "node_arena.h"
#include "recursion_guard.h"
#include "render_context.h"
#include "render_failure.h"
#include "render_workspace.h"
#include "renderer.h"
#include "statements.h"
#include "template_env_impl.h"
#include "template_parser.h"
#include "template_slots.h"
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
#include <forward_list>
#include <memory>
#include <new>
#include <optional>
#include <random>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include <nonstd/expected.hpp>

#include <list>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <type_traits>
#include <unordered_map>

namespace jinja2
{
inline namespace v2
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

// The maps a RenderWorkspace lends a render, returned emptied when they go out of scope
class WorkspaceMaps
{
public:
    WorkspaceMaps()
        : m_workspace(RenderWorkspace::ForThisThread())
        , m_maps(m_workspace.LendMaps())
    {
    }
    // Forced: GCC sometimes keeps this out of line in Render, which costs a call on every render
    JINJA2CPP_ALWAYS_INLINE ~WorkspaceMaps() { m_workspace.ReturnMaps(m_maps); }
    WorkspaceMaps(const WorkspaceMaps&) = delete;
    WorkspaceMaps(WorkspaceMaps&&) = delete;
    WorkspaceMaps& operator=(const WorkspaceMaps&) = delete;
    WorkspaceMaps& operator=(WorkspaceMaps&&) = delete;

    [[nodiscard]] InternalValueMap& Params() const { return m_maps.params; }
    [[nodiscard]] InternalValueMap& TopScope() const { return m_maps.topScope; }

private:
    RenderWorkspace& m_workspace;
    RenderWorkspace::RenderMaps& m_maps;
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
            snapshot->values[name] = visit(visitors::InputValueConvertor(false, true), value.GetData());
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
        errorData.relatedLocs = srcError.GetRelatedLocations();
        errorData.extraParams = srcError.GetExtraParams();
        errorData.exception = srcError.GetException();

        return BasicErrorInfo<CharT1>(std::move(errorData));
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

// The state the macros of a Template::MakeModule module share (template_module.cpp)
template<typename CharT>
struct TemplateModuleState;

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
    // Whether the template, or a parent it extends, may read a `loop` it does not bind itself
    // (an include of it must hide the including loop)
    [[nodiscard]] bool MayReadLoop() const { return m_mayReadLoop; }
    // The name `self` prints: the one the template was loaded with, empty (None) without one
    [[nodiscard]] std::string_view SelfName() const { return m_isNamed ? std::string_view(m_templateName) : std::string_view(); }
    [[nodiscard]] const ArenaView& Nodes() const { return m_nodes.View(); }
    auto GetTemplateName() const {};

    // Parses into fresh state and replaces the loaded template only if parsing succeeds: the tree
    // points into its source, so a failed Load keeps the previous source, tree and name together
    // The Load overloads check the size before they copy or read the whole source
    [[nodiscard]] bool SourceTooLong(std::size_t size) const
    {
        const auto maxSize = m_settings->settings.limits.maxTemplateSize;
        return maxSize != 0 && size > maxSize;
    }
    [[nodiscard]] BasicErrorInfo<CharT> SourceTooLongError(const std::string& tplName) const
    {
        typename BasicErrorInfo<CharT>::Data errorData;
        errorData.code = ErrorCode::ResourceLimitExceeded;
        errorData.srcLoc.col = 1;
        errorData.srcLoc.line = 1;
        errorData.srcLoc.fileName = tplName.empty() ? std::string("noname.j2tpl") : tplName;
        errorData.extraParams.push_back(
            Value("the template's source is longer than " + std::to_string(m_settings->settings.limits.maxTemplateSize) + " characters"));
        errorData.extraParams.push_back(Value(std::string("maxTemplateSize")));
        return BasicErrorInfo<CharT>(errorData);
    }

    std::optional<BasicErrorInfo<CharT>> Load(std::basic_string<CharT> tpl, std::string tplName)
    {
        if (SourceTooLong(tpl.size()))
        {
            return SourceTooLongError(tplName);
        }
        // On the heap, so the tree's pointers into it survive the hand-over to m_template
        auto source = std::make_unique<std::basic_string<CharT>>(std::move(tpl));
        NormalizeTemplateNewlines(*source, m_settings->settings.keepTrailingNewline);
        using namespace std::string_literals;
        const bool isNamed = !tplName.empty();
        std::string name = isNamed ? std::move(tplName) : "noname.j2tpl"s;
        NodeArena nodes;
        // Whatever parsing throws, except std::bad_alloc, becomes the error: the template that was loaded before stays.
        // The checks of the tree's handles (NodeRefChecks) in Seal fail only on a bug in the parser.
        try
        {
            TemplateParser<CharT> parser(source.get(), *m_settings, m_env, name, nodes);
            // Jinja2 passes None for a template from a string. A selector that throws fails the Load
            // as host code, whatever it throws (an ErrorInfo too)
            if (const auto& selector = m_settings->settings.autoescapeSelector)
            {
                bool autoescape = false;
                try
                {
                    autoescape = selector(isNamed ? name : std::string());
                }
                catch (...)
                {
                    return detail::HostExceptionToError<CharT>(name);
                }
                parser.SetAutoescape(autoescape);
            }

            const ParseLimitsScope parseLimits(m_settings->settings.limits);
            auto parseResult = parser.Parse();
            if (!parseResult)
            {
                return parseResult.error()[0];
            }

            parser.ReleaseParseState();
            // Its fields are set once the Load succeeds
            nodes.SetOrigin(&m_origin);
            auto sealed = nodes.Seal(*parseResult);
            auto convertedTexts = parser.TakeConvertedTexts();
            auto metadataInfo = parser.GetMetadataInfo();
            m_nodes = std::move(sealed);
            m_renderer = *parseResult;
            m_mayReadLoop = m_nodes[m_renderer].MayReadLoop();
            m_convertedTexts = std::move(convertedTexts);
            m_metadataInfo = std::move(metadataInfo);
        }
        catch (...)
        {
            return detail::CurrentExceptionToError<CharT>(name);
        }
        m_template = std::move(source);
        // Last: the parser refers to the name
        m_templateName = std::move(name);
        m_isNamed = isNamed;
        m_origin.source = m_template.get();
        m_origin.name = &m_templateName;
        m_origin.isWide = std::is_same_v<CharT, wchar_t>;
        m_metadata.reset();
        // The modules made from the tree this replaced see it gone (TemplateModuleState)
        m_loadGeneration.fetch_add(1, std::memory_order_relaxed);
        return std::optional<BasicErrorInfo<CharT>>();
    }

    // Template::MakeModule: renders the template, which `self` owns, as a module for the host
    // (template_module.cpp)
    static nonstd::expected<BasicTemplateModule<CharT>, BasicErrorInfo<CharT>> MakeModule(const std::shared_ptr<const TemplateImpl>& self, const ValuesMap& params);

    // Renders with the params of a ValuesMap or a GenericMap, appending to a string or writing to a
    // host's sink. Rendering reads the template only, so several threads may render one template at once.
    template<typename Target, typename ParamsMap>
    std::optional<BasicErrorInfo<CharT>> Render(Target& os, const ParamsMap& params) const
    {
        static_assert(std::is_same_v<Target, std::basic_string<CharT>> || std::is_same_v<Target, BasicOutputSink<CharT>>);
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
            // First, so that it ends last: when nothing of the render holds values any more
            const CycleCollector cycles;
            // Before anything else the render does, the conversion of the params included
            const RenderLimitsScope limitsScope(m_settings->hasRenderLimits ? &m_settings->settings.limits : nullptr, m_settings->bigIntBackend);
            // The tables of the params and the top scope are lent by the thread's workspace and
            // go back to it, emptied, when the render ends (docs/tasks/0160)
            const WorkspaceMaps maps;
            auto& intParams = maps.Params();

            auto convertParam = [&intParams](const std::string& name, const Value& value) {
                intParams[name] = visit(visitors::InputValueConvertor(false, true), value.GetData());
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
            RenderContext context(intParams,
                                  globals ? globals->values : noGlobals,
                                  &callback,
                                  &GetBuiltinGlobals(m_settings->settings.extensions.i18n),
                                  maps.TopScope());
            context.SetLookupCache(&LookupCache::ForThisThread());
            context.SetNodes(Nodes());
            const auto output = BeginOutput(os);
            OutStream outStream(os);
            if (m_settings->hasRenderLimits)
            {
                LimitOutput(os, output, outStream);
            }
            m_nodes[m_renderer].RenderNamed(outStream, context, SelfName());
            outStream.Flush();
            if (m_settings->hasRenderLimits)
            {
                CheckRenderOutput();
                detail::PollDeadline();
            }
            context.TakeTopScope(maps.TopScope());
            EndOutput(os, output);
        }
        catch (const std::bad_alloc&)
        {
            throw;
        }
        catch (const detail::RenderFailure& failure)
        {
            // Out of a statement: located there
            return failure.ErrorAs<CharT>();
        }
        catch (...)
        {
            // Out of no statement: the conversion of the params, the output limit at the end
            return detail::CurrentExceptionToError<CharT>(m_templateName);
        }

        return normalResult;
    }

    // Where a render's output starts in its string, and the size hint it was reserved from
    struct OutputStart
    {
        size_t start = 0;
        size_t hint = 0;
    };
    // The output of earlier renders sizes this one, so that the string does not regrow while
    // it is written (docs/tasks/0100). A hint only: concurrent renders may race on it
    // harmlessly. A sink holds no output: nothing to reserve
    template<typename Target>
    OutputStart BeginOutput(Target& os) const
    {
        OutputStart output;
        if constexpr (std::is_same_v<Target, std::basic_string<CharT>>)
        {
            output.start = os.size();
            output.hint = m_outputSizeHint.value.load(std::memory_order_relaxed);
            os.reserve(output.start + output.hint);
        }
        return output;
    }
    // Bounds the output by maxOutputSize: what the string gained, or what the sink received
    template<typename Target>
    static void LimitOutput(const Target& os, const OutputStart& output, const OutStream& stream)
    {
        if constexpr (std::is_same_v<Target, std::basic_string<CharT>>)
        {
            static_cast<void>(stream);
            LimitRenderOutput(os, output.start);
        }
        else
        {
            static_cast<void>(output);
            LimitRenderOutput(stream.SinkWritten());
        }
    }
    // Stored only when the output outgrows the hint or needs less than half of it, so that
    // renders of a steady size store nothing: every core rendering the template reads the
    // hint, and a store makes them all fetch it again (docs/tasks/0138). One huge render does
    // not make every later one reserve as much. A sink's renders leave it as it is
    template<typename Target>
    void EndOutput(const Target& os, const OutputStart& output) const
    {
        if constexpr (std::is_same_v<Target, std::basic_string<CharT>>)
        {
            constexpr size_t maxOutputSizeHint = size_t{ 16 } << 20;
            const auto newHint = std::min(os.size() - output.start, maxOutputSizeHint);
            if (newHint > output.hint || newHint < output.hint / 2)
            {
                m_outputSizeHint.value.store(newHint, std::memory_order_relaxed);
            }
        }
        else
        {
            static_cast<void>(os);
            static_cast<void>(output);
        }
    }

    using TplLoadResultType = std::variant<NoneValue,
                                           nonstd::expected<std::shared_ptr<TemplateImpl<char>>, ErrorInfo>,
                                           nonstd::expected<std::shared_ptr<TemplateImpl<wchar_t>>, ErrorInfoW>>;

    using TplOrError = nonstd::expected<std::shared_ptr<TemplateImpl<CharT>>, BasicErrorInfo<CharT>>;

    TplLoadResultType LoadTemplate(const std::string& fileName) const
    {
        if (!m_env)
        {
            return TplLoadResultType(NoneValue());
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
        // it, so it must outlive every map handed out
        std::scoped_lock lock(m_metadataMutex);
        try
        {
            return ParseMetadata();
        }
        catch (...)
        {
            return MakeUnexpected(detail::CurrentExceptionToError<CharT>(m_templateName));
        }
    }

    nonstd::expected<MetadataInfo<CharT>, BasicErrorInfo<CharT>> GetMetadataRaw() const
    {
        // Copied before it goes into the expected, whose converting constructor is noexcept:
        // std::bad_alloc must reach the caller, not std::terminate
        MetadataInfo<CharT> info = m_metadataInfo;
        return info;
    }

    // jinja2.meta.find_undeclared_variables (docs/tasks/0236). The tree keeps no record of where
    // names are declared, so the source is parsed again, into an arena of its own, and the
    // resolver of that parse reports them: Load pays nothing for it
    nonstd::expected<std::vector<std::string>, BasicErrorInfo<CharT>> FindUndeclaredVariables() const
    {
        if (!m_renderer)
        {
            typename BasicErrorInfo<CharT>::Data errorData;
            errorData.code = ErrorCode::TemplateNotParsed;
            errorData.srcLoc.col = 1;
            errorData.srcLoc.line = 1;
            errorData.srcLoc.fileName = "<unknown file>";
            return MakeUnexpected(BasicErrorInfo<CharT>(errorData));
        }
        NodeArena nodes;
        TemplateParser<CharT> parser(m_template.get(), *m_settings, m_env, m_templateName, nodes);
        const ParseLimitsScope parseLimits(m_settings->settings.limits);
        auto parseResult = parser.Parse();
        if (!parseResult)
        {
            return MakeUnexpected(parseResult.error()[0]);
        }
        const auto found = parser.GetNames().UndeclaredNames(nodes);
        std::vector<std::string> names(found.begin(), found.end());
        std::sort(names.begin(), names.end());
        names.erase(std::unique(names.begin(), names.end()), names.end());
        // Jinja2 leaves out the environment's globals: the built-in functions and the host's
        const auto& builtins = GetBuiltinGlobals(m_settings->settings.extensions.i18n);
        const auto isGlobal = [this, &builtins](const std::string& name) {
            if (builtins.find(name) != builtins.end())
            {
                return true;
            }
            bool isHostGlobal = false;
            if (m_env)
            {
                m_env->ApplyGlobals([&name, &isHostGlobal](const ValuesMap& globals) { isHostGlobal = globals.find(name) != globals.end(); });
            }
            return isHostGlobal;
        };
        names.erase(std::remove_if(names.begin(), names.end(), isGlobal), names.end());
        return names;
    }

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
        // The same source, settings and environment parse to the same tree (but for the
        // autoescape setting an autoescapeSelector chooses by name)
        const bool sameSource = m_template && other.m_template ? *m_template == *other.m_template : m_template == other.m_template;
        if (!sameSource)
        {
            return false;
        }
        if (m_metadata != other.m_metadata)
        {
            return false;
        }
        if (m_metadataInfo != other.m_metadataInfo)
        {
            return false;
        }
        return true;
    }
private:
    template<typename>
    friend struct TemplateModuleState;

    // GetMetadata with m_metadataMutex held
    nonstd::expected<GenericMap, BasicErrorInfo<CharT>> ParseMetadata() const
    {
        if (m_metadata)
        {
            // Copied first: expected's converting constructor is noexcept (see GetMetadataRaw)
            GenericMap metadata = m_metadata.value();
            return metadata;
        }

        auto& metadataString = m_metadataInfo.metadata;
        if (metadataString.empty())
        {
            return GenericMap();
        }

        if (m_metadataInfo.metadataType == "json")
        {
            // Read as UTF-8: wide metadata is converted first
            std::string converted;
            std::string_view narrowMetadata;
            if constexpr (std::is_same_v<CharT, char>)
            {
                narrowMetadata = metadataString;
            }
            else
            {
                converted = ConvertString<std::string>(metadataString);
                narrowMetadata = converted;
            }
            JsonReadLimits limits;
            limits.maxSequenceSize = m_settings->settings.limits.maxSequenceSize;
            auto result = ReadJsonObject(narrowMetadata, limits);
            if (!result)
            {
                typename BasicErrorInfo<CharT>::Data errorData;
                errorData.code = ErrorCode::MetadataParseError;
                errorData.srcLoc = m_metadataInfo.location;
                errorData.extraParams.push_back(Value(std::move(result.error())));
                return MakeUnexpected(BasicErrorInfo<CharT>(errorData));
            }
            m_metadata = Value(std::move(result.value())).ToGenericMap();
            // Copied first: expected's converting constructor is noexcept (see GetMetadataRaw)
            GenericMap metadata = m_metadata.value();
            return metadata;
        }
        return GenericMap();
    }

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
                CountTemplateLoad();
                auto& entry = loaded.emplace(fileName, LoadedTemplate{ m_host->LoadTemplate(fileName), nullptr }).first->second;
                entry.latest = &entry.first;
                return entry.first;
            }
            auto& entry = p->second;
            if (m_host->m_settings->settings.templateLookup == TemplateLookup::EveryUse)
            {
                CountTemplateLoad();
                auto result = m_host->LoadTemplate(fileName);
                // A reloaded template is kept as a new result: a render may still be running the one it replaces
                if (!IsSameTemplate(result, *entry.latest))
                {
                    entry.latest = &Loaded().replaced.emplace_back(std::move(result));
                }
            }
            return *entry.latest;
        }

        [[nodiscard]] const LoadTemplateResult* FindLoadedBy(const void* site) const override
        {
            if (!m_loaded)
            {
                return nullptr;
            }
            const auto& bySite = m_loaded->bySite;
            auto p = std::find_if(bySite.begin(), bySite.end(), [site](const auto& entry) { return entry.first == site; });
            return p != bySite.end() ? p->second : nullptr;
        }
        void SetLoadedBy(const void* site, const LoadTemplateResult& tpl) const override
        {
            // With EveryUse a template changed during the render is seen by its next use
            if (m_host->m_settings->settings.templateLookup == TemplateLookup::OncePerRender)
            {
                Loaded().bySite.emplace_back(site, &tpl);
            }
        }

        [[nodiscard]] const LoadTemplateResult& LoadTemplate(const InternalValue& fileName) const override
        {
            auto name = GetAsSameString(std::string(), fileName);
            if (!name)
            {
                CountTemplateLoad();
                return Loaded().invalidNames.emplace_back(m_host->LoadTemplate(fileName));
            }
            return LoadTemplate(name.value());
        }

        [[noreturn]] void ThrowRuntimeError(ErrorCode code, ValuesList extraParams) override
        {
            m_host->ThrowRuntimeError(code, std::move(extraParams));
        }

        // What a host's call of a module's macro has added so far (Template::MakeModule,
        // docs/tasks/0301)
        struct CallMark
        {
            std::size_t modules = 0;
            std::size_t invalidNames = 0;
        };
        [[nodiscard]] CallMark MarkCall() const noexcept
        {
            return m_loaded ? CallMark{ m_loaded->modules.size(), m_loaded->invalidNames.size() } : CallMark{};
        }
        // Ends what the call that took `mark` added: the modules it imported, but those shared
        // without context, which see nothing of the call and stay for the module's life, as
        // Jinja2 keeps a template's default module. The trees stay too: what a module's macro
        // stored views its tree. What still refers to an ended module does so through its
        // anchor (ClosureSite), which a module made in the call marks as ending with it
        void EndCall(const CallMark& mark) noexcept
        {
            if (!m_loaded)
            {
                return;
            }
            auto& modules = m_loaded->modules;
            const auto& shared = m_loaded->sharedModules;
            // The newest first, as a render ends them
            for (auto p = modules.size(); p-- > mark.modules;)
            {
                auto& module = modules[p];
                const bool isShared = std::any_of(shared.begin(), shared.end(), [&module](const auto& entry) { return entry.second == module.get(); });
                if (isShared)
                {
                    continue;
                }
                // Its values may hold themselves: the call's collector frees those cycles. Noting
                // them allocates; when that fails, the rest of the module's cycles leak
                try
                {
                    for (const auto& [name, value] : module->Names())
                    {
                        CycleCollector::NoteStore(value);
                    }
                }
                catch (const std::bad_alloc&) // NOLINT(bugprone-empty-catch): best effort, the comment above says what is lost
                {
                }
                module.reset();
            }
            modules.erase(std::remove(modules.begin() + static_cast<std::ptrdiff_t>(mark.modules), modules.end(), nullptr), modules.end());
            // Errors only, which the statement that looked them up has thrown
            auto& invalid = m_loaded->invalidNames;
            while (invalid.size() > mark.invalidNames)
            {
                invalid.pop_back();
            }
        }

        TemplateSlots& Templates() override { return Loaded().templates; }
        ImportedModule& AddModule(std::unique_ptr<ImportedModule> module) override { return *Loaded().modules.emplace_back(std::move(module)); }
        void ShareModule(const LoadTemplateResult& tpl, ImportedModule& module) override { Loaded().sharedModules.emplace_back(&tpl, &module); }
        [[nodiscard]] ImportedModule* FindModule(const LoadTemplateResult& tpl) const override
        {
            if (!m_loaded)
            {
                return nullptr;
            }
            const auto& shared = m_loaded->sharedModules;
            auto p = std::find_if(shared.begin(), shared.end(), [&tpl](const auto& entry) { return entry.first == &tpl; });
            return p != shared.end() ? p->second : nullptr;
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
                    if constexpr (std::is_same_v<T, NoneValue>)
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
            LoadedTemplates() = default;
            LoadedTemplates(const LoadedTemplates&) = delete;
            LoadedTemplates(LoadedTemplates&&) = delete;
            LoadedTemplates& operator=(const LoadedTemplates&) = delete;
            LoadedTemplates& operator=(LoadedTemplates&&) = delete;
            // Out of line, so that a render that loads nothing only checks for none
            JINJA2CPP_NOINLINE_INLINE ~LoadedTemplates() = default;

            std::unordered_map<std::string, LoadedTemplate> byName;
            // Results that are not the first for their name: templates reloaded during the render
            std::list<LoadTemplateResult> replaced;
            // The errors of names that are not strings, which no statement keeps past its throw
            std::list<LoadTemplateResult> invalidNames;
            // The trees of the templates above, and of the rendered one, that handles link to
            TemplateSlots templates;
            // What each `include` of a constant name loaded (FindLoadedBy). A render has few
            // such statements, so a list beats hashing
            std::vector<std::pair<const void*, const LoadTemplateResult*>> bySite;
            // The modules the render imported, freed before the trees their macros run in
            std::vector<std::unique_ptr<ImportedModule>> modules;
            // The modules imported without context, by the template they were made from. A
            // render imports few templates, so a list beats hashing
            std::vector<std::pair<const LoadTemplateResult*, ImportedModule*>> sharedModules;
        };
        // Made on the first lookup or handle, so a render that loads nothing does not pay for it
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
    // Text the tree points to besides the source (newline_sequence copies), if any
    std::unique_ptr<std::forward_list<std::basic_string<CharT>>> m_convertedTexts;
    std::string m_templateName;
    bool m_isNamed = false;
    bool m_mayReadLoop = false;
    // Counts the successful Loads: a module's macro refuses to run in a tree a later Load
    // replaced (TemplateModuleState). Atomic only so that a misuse is not a data race; 32 bits
    // fit the padding after the flags, so a template is no bigger, and a module sees a stale
    // tree as current only after exactly 2^32 more Loads
    std::atomic<uint32_t> m_loadGeneration{ 0 };
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
    mutable std::mutex m_metadataMutex;
    MetadataInfo<CharT> m_metadataInfo;
    // The source and name the tree's render errors are located in; read on an error only
    detail::TemplateOrigin m_origin;
};

} // namespace v2
} // namespace jinja2

#endif // JINJA2CPP_SRC_TEMPLATE_IMPL_H
