#ifndef JINJA2CPP_SRC_TEMPLATE_ENV_IMPL_H
#define JINJA2CPP_SRC_TEMPLATE_ENV_IMPL_H

#include <jinja2cpp/filesystem_handler.h>
#include <jinja2cpp/template.h>
#include <jinja2cpp/template_env.h>
#include <jinja2cpp/value.h>

#include "load_settings.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
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

    // The settings templates are made with: shared with the templates made before while `settings` still equals them
    LoadSettingsPtr GetLoadSettings() const;
    // A number that no other state of the globals of any environment has had
    static uint64_t NewGlobalsGeneration();

    std::vector<FsHandler> filesystemHandlers;
    Settings settings;
    // Guarded by `guard`. A render keeps the map it converted alive, so a change while the map is held
    // replaces it instead of changing it (docs/tasks/0139)
    std::shared_ptr<ValuesMap> globalValues = std::make_shared<ValuesMap>();
    // Changes with every change of the globals: a render converts them only when it differs
    // from the one of the copy it converted last
    std::atomic<uint64_t> globalsGeneration{ NewGlobalsGeneration() };
    CallablesMap filters;
    CallablesMap tests;
    CallablesMap translations;
    mutable std::shared_timed_mutex guard;
    TemplateCache<Template> templateCache;
    TemplateCache<TemplateW> templateWCache;
    // The TemplateEnv that owns this state; null once it is destroyed, and nothing is cached then. Changed under `guard`
    std::atomic<const TemplateEnv*> owner{ nullptr };

private:
    mutable std::mutex m_loadSettingsGuard;
    mutable LoadSettingsPtr m_loadSettings;
};

struct TemplateEnvAccess
{
    static const std::shared_ptr<TemplateEnvImpl>& GetImpl(const TemplateEnv& env) { return env.m_impl; }
    // Another handle to the state of `env` (none for null), for a template to keep it alive
    static TemplateEnv MakeHandle(const TemplateEnv* env) { return TemplateEnv(env ? env->m_impl : nullptr); }
};

} // namespace jinja2::detail

#endif // JINJA2CPP_SRC_TEMPLATE_ENV_IMPL_H
