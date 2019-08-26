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

//! Compatibility mode for jinja2c++ engine
enum class Jinja2CompatMode
{
    None,          //!< Default mode
    Vesrsion_2_10, //!< Compatibility with Jinja2 v.2.10 specification
};

//! Global template environment settings
struct Settings
{
    /// Extensions set which should be supported
    struct Extensions
    {
        bool Do = false;  //!< Enable use of `do` statement
    };

    //! Enables use of line statements (yet not supported)
    bool useLineStatements = false;
    //! Enables blocks trimming the same way as it does python Jinja2 engine
    bool trimBlocks = false;
    //! Enables blocks stripping (from the left) the same way as it does python Jinja2 engine
    bool lstripBlocks = false;
    //! Extensions set enabled for templates
    Extensions extensions;
    //! Controls Jinja2 compatibility mode
    Jinja2CompatMode jinja2CompatMode = Jinja2CompatMode::None;
};

/*!
 * \brief Global template environment which controls behaviour of the different \ref Template instances
 *
 * This class is used for fine tuning of the templates behaviour and for state sharing between them. With this class
 * it's possible to control template loading, provide template sources, set global variables, use template inheritance
 * and inclusion.
 *
 * It's possible to load templates from the environment via \ref LoadTemplate or \ref LoadTemplateW methods
 * or to pass instance of the environment directly to the \ref Template via constructor.
 */
class TemplateEnv
{
public:
    /*!
     * \brief Returns global settings for the environment
     *
     * @return Constant reference to the global settings
     */
    const Settings& GetSettings() const {return m_settings;}
    /*!
     * \brief Returns global settings for the environment available for modification
     *
     * @return Reference to the global settings
     */
    Settings& GetSettings() {return m_settings;}

    /*!
     * \brief Replace global settings for the environment with the new ones
     *
     * @param setts New settings
     */
    void SetSettings(const Settings& setts) {m_settings = setts;}

    /*!
     * \brief Add pointer to file system handler with the specified prefix
     *
     * Adds filesystem handler which provides access to the external source of templates. With added handlers it's
     * possible to load templates from the `import`, `extends` and `include` jinja2 tags. Prefix is used for
     * distinguish one templates source from another. \ref LoadTemplate or \ref LoadTemplateW methods use
     * handlers to load templates with the specified name.
     * Method is thread-unsafe. It's dangerous to add new filesystem handlers and load templates simultaneously.
     *
     * Basic usage:
     * ```c++
     *  jinja2::TemplateEnv env;
     *
     *  auto fs = std::make_shared<jinja2::MemoryFileSystem>();
     *  env.AddFilesystemHandler(std::string(), fs);
     *  fs->AddFile("base.j2tpl", "Hello World!");
     * ```
     *
     * @param prefix Optional prefix of the handler's filesystem. Prefix is a part of the file name and passed to the handler's \ref IFilesystemHandler::OpenStream method
     * @param h      Shared pointer to the handler
     */
    void AddFilesystemHandler(std::string prefix, FilesystemHandlerPtr h)
    {
        m_filesystemHandlers.push_back(FsHandler{std::move(prefix), std::move(h)});
    }
    /*!
     * \brief Add reference to file system handler with the specified prefix
     *
     * Adds filesystem handler which provides access to the external source of templates. With added handlers it's
     * possible to load templates from the `import`, `extends` and `include` jinja2 tags. Prefix is used for
     * distinguish one templates source from another. \ref LoadTemplate or \ref LoadTemplateW methods use
     * handlers to load templates with the specified name.
     * Method is thread-unsafe. It's dangerous to add new filesystem handlers and load templates simultaneously.
     *
     * Basic usage:
     * ```c++
     *  jinja2::TemplateEnv env;
     *
     *  MemoryFileSystem fs;
     *  env.AddFilesystemHandler(std::string(), fs);
     *  fs.AddFile("base.j2tpl", "Hello World!");
     * ```
     *
     * @param prefix Optional prefix of the handler's filesystem. Prefix is a part of the file name and passed to the handler's \ref IFilesystemHandler::OpenStream method
     * @param h      Reference to the handler. It's assumed that lifetime of the handler is controlled externally
     */
    void AddFilesystemHandler(std::string prefix, IFilesystemHandler& h)
    {
        m_filesystemHandlers.push_back(FsHandler{std::move(prefix), std::shared_ptr<IFilesystemHandler>(&h, [](auto*) {})});
    }
    /*!
     * \brief Load narrow char template with the specified name via registered file handlers
     *
     * In case of specified file present in any of the registered handlers, template is loaded and parsed. If any
     * error occurred during the loading or parsing detailed diagnostic will be returned.
     * Method is thread-unsafe. It's dangerous to add new filesystem handlers and load templates simultaneously.
     *
     * @param fileName Template name to load
     *
     * @return Either loaded template or load/parse error. See \ref ErrorInfoTpl
     */
    nonstd::expected<Template, ErrorInfo> LoadTemplate(std::string fileName);
    /*!
     * \brief Load wide char template with the specified name via registered file handlers
     *
     * In case of specified file present in any of the registered handlers, template is loaded and parsed. If any
     * error occurred during the loading or parsing detailed diagnostic will be returned.
     * Method is thread-unsafe. It's dangerous to add new filesystem handlers and load templates simultaneously.
     *
     * @param fileName Template name to load
     *
     * @return Either loaded template or load/parse error. See \ref ErrorInfoTpl
     */
    nonstd::expected<TemplateW, ErrorInfoW> LoadTemplateW(std::string fileName);

    /*!
     * \brief Add global variable to the environment
     *
     * Adds global variable which can be referred in any template which is loaded within this environment object.
     * Method is thread-safe.
     *
     * @param name Name of the variable
     * @param val  Value of the variable
     */
    void AddGlobal(std::string name, Value val)
    {
        std::unique_lock<std::shared_timed_mutex> l(m_guard);
        m_globalValues[std::move(name)] = std::move(val);
    }
    /*!
     * \brief Remove global variable from the environment
     *
     * Removes global variable from the environment.
     * Method is thread-safe.
     *
     * @param name Name of the variable
     */
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
