#ifndef JINJA2CPP_TEMPLATE_ENV_H
#define JINJA2CPP_TEMPLATE_ENV_H

#include "error_info.h"

#include "filesystem_handler.h"
#include "template.h"

#include <unordered_map>
#include <shared_mutex>

namespace jinja2
{

class IErrorHandler;
class IFilesystemHandler;

enum class Jinja2CompatMode
{
    None,
    Vesrsion_2_10,
};

struct Settings
{
    struct Extensions
    {
        bool Do = false;
    };

    bool useLineStatements = false;
    bool trimBlocks = false;
    bool lstripBlocks = false;
    Extensions extensions;
    Jinja2CompatMode jinja2CompatMode = Jinja2CompatMode::None;
};

class TemplateEnv
{
public:
    void SetErrorHandler(IErrorHandler* h)
    {
        m_errorHandler = h;
    }
    auto GetErrorHandler() const
    {
        return m_errorHandler;
    }

    const Settings& GetSettings() const {return m_settings;}
    Settings& GetSettings() {return m_settings;}
    void SetSettings(const Settings& setts) {m_settings = setts;}

    void AddFilesystemHandler(std::string prefix, FilesystemHandlerPtr h)
    {
        m_filesystemHandlers.push_back(FsHandler{std::move(prefix), h});
    }
    void AddFilesystemHandler(std::string prefix, IFilesystemHandler& h)
    {
        m_filesystemHandlers.push_back(FsHandler{std::move(prefix), std::shared_ptr<IFilesystemHandler>(&h, [](auto*) {})});
    }
    nonstd::expected<Template, ErrorInfo> LoadTemplate(std::string fileName);
    nonstd::expected<TemplateW, ErrorInfoW> LoadTemplateW(std::string fileName);

    void AddGlobal(std::string name, Value val)
    {
        std::unique_lock<std::shared_timed_mutex> l(m_guard);
        m_globalValues[std::move(name)] = std::move(val);
    }

    void RemoveGlobal(const std::string& name)
    {
        std::unique_lock<std::shared_timed_mutex> l(m_guard);
        m_globalValues.erase(name);
    }

    template<typename Fn>
    void ApplyGlobals(Fn&& fn)
    {
        std::shared_lock<std::shared_timed_mutex> l(m_guard);
        fn(m_globalValues);
    }
private:
    IErrorHandler* m_errorHandler;
    struct FsHandler
    {
        std::string prefix;
        FilesystemHandlerPtr handler;
    };
    std::vector<FsHandler> m_filesystemHandlers;
    Settings m_settings;
    ValuesMap m_globalValues;
    std::shared_timed_mutex m_guard;
};

} // jinja2

#endif // JINJA2CPP_TEMPLATE_ENV_H
