#include <jinja2cpp/template_env.h>

#include "load_settings.h"
#include "make_unexpected.h"
#include "template_env_impl.h"

#include <jinja2cpp/error_info.h>
#include <jinja2cpp/filesystem_handler.h>
#include <jinja2cpp/template.h>
#include <jinja2cpp/value.h>

#include <algorithm>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>

namespace jinja2
{

bool operator==(const Settings& lhs, const Settings& rhs)
{
    // Structured bindings name every field: a field added to Settings or Settings::Extensions stops this compiling
    // until it is compared here too
    const auto& [lTrim, lLstrip, lCacheSize, lAutoReload, lExt, lMetaType, lKeepNl, lNlSeq, lVarStart, lVarEnd, lBlockStart, lBlockEnd, lCommentStart,
                 lCommentEnd, lLineStmt, lLineComment, lFinalize, lAutoescape, lUndefined, lLookup] = lhs;
    const auto& [rTrim, rLstrip, rCacheSize, rAutoReload, rExt, rMetaType, rKeepNl, rNlSeq, rVarStart, rVarEnd, rBlockStart, rBlockEnd, rCommentStart,
                 rCommentEnd, rLineStmt, rLineComment, rFinalize, rAutoescape, rUndefined, rLookup] = rhs;
    const auto& [lDo, lLoopControls, lI18n] = lExt;
    const auto& [rDo, rLoopControls, rI18n] = rExt;

    // A default UserCallable still has an identity of its own, so two unset ones are compared by the missing callable
    const bool sameFinalize = lFinalize.callable || rFinalize.callable ? lFinalize.IsEqual(rFinalize) : true;
    return std::tie(lTrim, lLstrip, lCacheSize, lAutoReload, lDo, lLoopControls, lI18n, lMetaType, lKeepNl, lNlSeq, lVarStart, lVarEnd, lBlockStart,
                    lBlockEnd, lCommentStart, lCommentEnd, lLineStmt, lLineComment, lAutoescape, lUndefined, lLookup)
               == std::tie(rTrim, rLstrip, rCacheSize, rAutoReload, rDo, rLoopControls, rI18n, rMetaType, rKeepNl, rNlSeq, rVarStart, rVarEnd, rBlockStart,
                           rBlockEnd, rCommentStart, rCommentEnd, rLineStmt, rLineComment, rAutoescape, rUndefined, rLookup)
           && sameFinalize;
}

namespace detail
{
namespace
{
bool IsSameHandler(const FilesystemHandlerPtr& lhs, const FilesystemHandlerPtr& rhs)
{
    if (lhs && rhs)
    {
        return lhs->IsEqual(*rhs);
    }
    return !lhs && !rhs;
}

bool IsSameCallables(const TemplateEnvImpl::CallablesMap& lhs, const TemplateEnvImpl::CallablesMap& rhs)
{
    return lhs.size() == rhs.size() && std::all_of(lhs.begin(), lhs.end(), [&rhs](const auto& item) {
               auto p = rhs.find(item.first);
               return p != rhs.end() && item.second.IsEqual(p->second);
           });
}

std::optional<UserCallable> FindCallable(const TemplateEnvImpl::CallablesMap& callables, const std::string& name)
{
    auto p = callables.find(name);
    return p == callables.end() ? std::optional<UserCallable>() : std::optional<UserCallable>(p->second);
}

template<typename CharT>
struct TemplateFunctions;

template<>
struct TemplateFunctions<char>
{
    using ResultType = Result<Template>;
    static Template CreateTemplate(TemplateEnv* env) { return Template(env); }
    static auto LoadFile(const std::string& fileName, const IFilesystemHandler* fs) { return fs->OpenStream(fileName); }
    static auto& GetCache(TemplateEnvImpl& impl) { return impl.templateCache; }
};

template<>
struct TemplateFunctions<wchar_t>
{
    using ResultType = ResultW<TemplateW>;
    static TemplateW CreateTemplate(TemplateEnv* env) { return TemplateW(env); }
    static auto LoadFile(const std::string& fileName, const IFilesystemHandler* fs) { return fs->OpenWStream(fileName); }
    static auto& GetCache(TemplateEnvImpl& impl) { return impl.templateWCache; }
};

} // namespace

bool TemplateEnvImpl::FsHandler::operator==(const FsHandler& rhs) const
{
    return prefix == rhs.prefix && IsSameHandler(handler, rhs.handler);
}

bool TemplateEnvImpl::BaseTemplateInfo::operator==(const BaseTemplateInfo& other) const
{
    return lastModification == other.lastModification && lastAccessTime == other.lastAccessTime && IsSameHandler(handler, other.handler);
}

bool TemplateEnvImpl::IsEqual(const TemplateEnvImpl& other) const
{
    if (this == &other)
    {
        return true;
    }
    std::shared_lock<std::shared_timed_mutex> l1(guard, std::defer_lock);
    std::shared_lock<std::shared_timed_mutex> l2(other.guard, std::defer_lock);
    std::lock(l1, l2);
    return filesystemHandlers == other.filesystemHandlers && settings == other.settings && globalValues == other.globalValues && IsSameCallables(filters, other.filters) && IsSameCallables(tests, other.tests) && IsSameCallables(translations, other.translations) && templateCache == other.templateCache && templateWCache == other.templateWCache;
}

LoadSettingsPtr TemplateEnvImpl::GetLoadSettings() const
{
    // Comparing is cheaper than copying the settings and rebuilding the delimiters, and catches changes made through
    // the reference TemplateEnv::GetSettings returns
    // A finalize callable edited in place keeps its identity, so it would compare equal: templates of an environment
    // with one get settings of their own
    if (settings.finalize.callable)
    {
        return std::make_shared<const LoadSettings>(settings);
    }
    std::scoped_lock l(m_loadSettingsGuard);
    if (!m_loadSettings || m_loadSettings->settings != settings)
    {
        m_loadSettings = std::make_shared<const LoadSettings>(settings);
    }
    return m_loadSettings;
}

const LoadSettingsPtr& DefaultLoadSettings()
{
    static const LoadSettingsPtr defaultSettings = std::make_shared<const LoadSettings>(Settings());
    return defaultSettings;
}

template<typename CharT>
auto TemplateEnvImpl::LoadTemplate(TemplateEnv* env, std::string fileName)
{
    using Functions = TemplateFunctions<CharT>;
    using ResultType = typename Functions::ResultType;
    using ErrorType = typename ResultType::error_type;
    auto& cache = Functions::GetCache(*this);

    {
        std::shared_lock<std::shared_timed_mutex> l(guard);
        auto p = cache.find(fileName);
        if (p != cache.end())
        {
            if (settings.autoReload)
            {
                auto lastModified = p->second.handler->GetLastModificationDate(fileName);
                if (!lastModified || (p->second.lastModification && lastModified.value() <= p->second.lastModification.value()))
                {
                    return ResultType(p->second.tpl);
                }
            }
            else
            {
                return ResultType(p->second.tpl);
            }
        }
    }

    // Created only on a miss: a new template allocates its environment handle
    auto tpl = Functions::CreateTemplate(env);
    for (auto& fh : filesystemHandlers)
    {
        if (!fh.prefix.empty() && fileName.find(fh.prefix) != 0)
        {
            continue;
        }

        auto stream = Functions::LoadFile(fileName, fh.handler.get());
        if (stream)
        {
            auto res = tpl.Load(*stream, fileName);
            if (!res)
            {
                return ResultType(MakeUnexpected(res.error()));
            }

            if (settings.cacheSize != 0)
            {
                auto lastModified = fh.handler->GetLastModificationDate(fileName);
                // A template this replaces (another thread loaded the same file concurrently) is
                // released after the lock: it may hold the last handle to this environment, and
                // ~TemplateEnv takes the same lock
                std::optional<decltype(tpl)> replaced;
                std::unique_lock<std::shared_timed_mutex> l(guard);
                if (owner)
                {
                    auto& cacheEntry = cache[fileName];
                    replaced = std::move(cacheEntry.tpl);
                    cacheEntry.tpl = tpl;
                    cacheEntry.handler = fh.handler;
                    cacheEntry.lastModification = lastModified;
                }
            }

            return ResultType(tpl);
        }
    }

    typename ErrorType::Data errorData;
    errorData.code = ErrorCode::FileNotFound;
    errorData.srcLoc.col = 1;
    errorData.srcLoc.line = 1;
    errorData.srcLoc.fileName = "";
    errorData.extraParams.push_back(Value(fileName));

    return ResultType(MakeUnexpected(ErrorType(errorData)));
}
} // namespace detail

TemplateEnv::TemplateEnv()
    : m_impl(std::make_shared<detail::TemplateEnvImpl>())
{
    m_impl->owner = this;
}

TemplateEnv::TemplateEnv(std::shared_ptr<detail::TemplateEnvImpl> impl)
    : m_impl(std::move(impl))
{
}

TemplateEnv::~TemplateEnv()
{
    if (!m_impl)
    {
        return;
    }
    // The handles templates keep never own the state, and only the owner changes `owner`
    if (m_impl->owner != this)
    {
        return;
    }
    detail::TemplateEnvImpl::TemplateCache<Template> templateCache;
    detail::TemplateEnvImpl::TemplateCache<TemplateW> templateWCache;
    {
        std::unique_lock<std::shared_timed_mutex> l(m_impl->guard);
        m_impl->owner = nullptr;
        templateCache.swap(m_impl->templateCache);
        templateWCache.swap(m_impl->templateWCache);
    }
    // The cached templates are released here, outside the lock: they hold handles to this state
}

const Settings& TemplateEnv::GetSettings() const
{
    return m_impl->settings;
}

Settings& TemplateEnv::GetSettings()
{
    return m_impl->settings;
}

void TemplateEnv::SetSettings(const Settings& setts)
{
    m_impl->settings = setts;
}

void TemplateEnv::AddFilesystemHandler(std::string prefix, FilesystemHandlerPtr h)
{
    m_impl->filesystemHandlers.push_back(detail::TemplateEnvImpl::FsHandler{ std::move(prefix), std::move(h) });
}

void TemplateEnv::AddFilesystemHandler(std::string prefix, IFilesystemHandler& h)
{
    AddFilesystemHandler(std::move(prefix), std::shared_ptr<IFilesystemHandler>(&h, [](auto*) {}));
}

Result<Template> TemplateEnv::LoadTemplate(std::string fileName)
{
    return m_impl->LoadTemplate<char>(this, std::move(fileName));
}

ResultW<TemplateW> TemplateEnv::LoadTemplateW(std::string fileName)
{
    return m_impl->LoadTemplate<wchar_t>(this, std::move(fileName));
}

Result<Template> TemplateEnv::FromString(std::string_view source, std::string name)
{
    Template tpl(this);
    auto res = tpl.Load(std::string(source), std::move(name));
    if (!res)
    {
        return MakeUnexpected(res.error());
    }
    return tpl;
}

ResultW<TemplateW> TemplateEnv::FromString(std::wstring_view source, std::string name)
{
    TemplateW tpl(this);
    auto res = tpl.Load(std::wstring(source), std::move(name));
    if (!res)
    {
        return MakeUnexpected(res.error());
    }
    return tpl;
}

void TemplateEnv::AddGlobal(std::string name, Value val)
{
    std::unique_lock<std::shared_timed_mutex> l(m_impl->guard);
    m_impl->globalValues[std::move(name)] = std::move(val);
}

void TemplateEnv::RemoveGlobal(const std::string& name)
{
    std::unique_lock<std::shared_timed_mutex> l(m_impl->guard);
    m_impl->globalValues.erase(name);
}

void TemplateEnv::AddFilter(std::string name, UserCallable filter)
{
    std::unique_lock<std::shared_timed_mutex> l(m_impl->guard);
    m_impl->filters[std::move(name)] = std::move(filter);
}

void TemplateEnv::RemoveFilter(const std::string& name)
{
    std::unique_lock<std::shared_timed_mutex> l(m_impl->guard);
    m_impl->filters.erase(name);
}

void TemplateEnv::AddTest(std::string name, UserCallable test)
{
    std::unique_lock<std::shared_timed_mutex> l(m_impl->guard);
    m_impl->tests[std::move(name)] = std::move(test);
}

void TemplateEnv::RemoveTest(const std::string& name)
{
    std::unique_lock<std::shared_timed_mutex> l(m_impl->guard);
    m_impl->tests.erase(name);
}

std::optional<UserCallable> TemplateEnv::FindFilter(const std::string& name) const
{
    std::shared_lock<std::shared_timed_mutex> l(m_impl->guard);
    return detail::FindCallable(m_impl->filters, name);
}

std::optional<UserCallable> TemplateEnv::FindTest(const std::string& name) const
{
    std::shared_lock<std::shared_timed_mutex> l(m_impl->guard);
    return detail::FindCallable(m_impl->tests, name);
}

void TemplateEnv::InstallGettextCallables(UserCallable gettext, UserCallable ngettext, UserCallable pgettext, UserCallable npgettext)
{
    std::unique_lock<std::shared_timed_mutex> l(m_impl->guard);
    auto& translations = m_impl->translations;
    translations.clear();
    auto install = [&translations](const char* name, UserCallable& fn) {
        if (fn.callable)
        {
            translations[name] = std::move(fn);
        }
    };
    install("gettext", gettext);
    install("ngettext", ngettext);
    install("pgettext", pgettext);
    install("npgettext", npgettext);
}

std::optional<UserCallable> TemplateEnv::FindGettextCallable(const std::string& name) const
{
    std::shared_lock<std::shared_timed_mutex> l(m_impl->guard);
    return detail::FindCallable(m_impl->translations, name);
}

void TemplateEnv::ApplyGlobals(const std::function<void(const ValuesMap&)>& fn) const
{
    std::shared_lock<std::shared_timed_mutex> l(m_impl->guard);
    fn(m_impl->globalValues);
}

} // namespace jinja2
