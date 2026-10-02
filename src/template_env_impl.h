#ifndef JINJA2CPP_SRC_TEMPLATE_ENV_IMPL_H
#define JINJA2CPP_SRC_TEMPLATE_ENV_IMPL_H

#include <jinja2cpp/template_env.h>

#include <chrono>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

namespace jinja2::detail
{

// The state of a TemplateEnv. The TemplateEnv the user creates owns it; every template made in the environment holds
// a handle to it (TemplateEnvAccess::MakeHandle) and keeps it alive. Cached templates hold handles too, so the owner's
// destructor drops the caches and turns caching off, or the state would keep itself alive.
class TemplateEnvImpl
{
public:
    using TimePoint = std::chrono::system_clock::time_point;
    using TimeStamp = std::chrono::steady_clock::time_point;
    using CallablesMap = std::unordered_map<std::string, UserCallable>;

    struct FsHandler
    {
        std::string prefix;
        FilesystemHandlerPtr handler;
        bool operator==(const FsHandler& rhs) const;
        bool operator!=(const FsHandler& rhs) const { return !(*this == rhs); }
    };

    struct BaseTemplateInfo
    {
        std::optional<TimePoint> lastModification;
        TimeStamp lastAccessTime;
        FilesystemHandlerPtr handler;
        bool operator==(const BaseTemplateInfo& other) const;
        bool operator!=(const BaseTemplateInfo& other) const { return !(*this == other); }
    };

    template<typename TemplateT>
    struct TemplateCacheEntry : public BaseTemplateInfo
    {
        TemplateT tpl;
        bool operator==(const TemplateCacheEntry& other) const { return BaseTemplateInfo::operator==(other) && tpl == other.tpl; }
        bool operator!=(const TemplateCacheEntry& other) const { return !(*this == other); }
    };

    template<typename TemplateT>
    using TemplateCache = std::unordered_map<std::string, TemplateCacheEntry<TemplateT>>;

    bool IsEqual(const TemplateEnvImpl& other) const;

    // Loads a template through the filesystem handlers; `env` is the handle the new template is created with
    template<typename CharT>
    auto LoadTemplate(TemplateEnv* env, std::string fileName);

    std::vector<FsHandler> filesystemHandlers;
    Settings settings;
    ValuesMap globalValues;
    CallablesMap filters;
    CallablesMap tests;
    CallablesMap translations;
    mutable std::shared_timed_mutex guard;
    TemplateCache<Template> templateCache;
    TemplateCache<TemplateW> templateWCache;
    // The TemplateEnv that owns this state; null once it is destroyed, and nothing is cached then. Guarded by `guard`
    const TemplateEnv* owner = nullptr;
};

struct TemplateEnvAccess
{
    static const std::shared_ptr<TemplateEnvImpl>& GetImpl(const TemplateEnv& env) { return env.m_impl; }
    // Another handle to the state of `env`, for a template to keep it alive
    static std::unique_ptr<TemplateEnv> MakeHandle(const TemplateEnv& env) { return std::unique_ptr<TemplateEnv>(new TemplateEnv(env.m_impl)); }
};

} // namespace jinja2::detail

#endif // JINJA2CPP_SRC_TEMPLATE_ENV_IMPL_H
