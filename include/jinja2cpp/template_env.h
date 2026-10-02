#ifndef JINJA2CPP_TEMPLATE_ENV_H
#define JINJA2CPP_TEMPLATE_ENV_H

#include "config.h"
#include "error_info.h"
#include "filesystem_handler.h"
#include "template.h"

#include <mutex>
#include <shared_mutex>
#include <unordered_map>

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

//! What a template may do with an undefined value: a missing variable, attribute or item (Jinja2 `undefined`)
enum class UndefinedPolicy
{
    Default,   //!< Prints as empty, is false and an empty sequence; attribute, item, call and arithmetic use is an error (Jinja2 `Undefined`)
    Strict,    //!< Any use but the `defined`/`undefined` tests and the `default` filter is an error (Jinja2 `StrictUndefined`)
    Chainable, //!< As Default, but an attribute or item of it is undefined too (Jinja2 `ChainableUndefined`)
    Debug,     //!< As Default, but prints as `{{ name }}` (Jinja2 `DebugUndefined`)
};

//! Global template environment settings
struct Settings
{
    /// Extensions set which should be supported
    struct Extensions
    {
        bool Do = false;           //!< Enable use of `do` statement
        bool LoopControls = false; //!< Enable use of `break` and `continue` statements in loops (Jinja2 `jinja2.ext.loopcontrols`)
        //! Enable `{% trans %}` blocks and the `_`, `gettext`, `ngettext`, `pgettext` and `npgettext` globals (Jinja2 `jinja2.ext.i18n`
        //! with newstyle gettext). Messages are not translated unless \ref TemplateEnv::InstallGettextCallables provides the translations
        bool I18n = false;
    };

    //! Enables line statements with the `#` prefix; same as setting \ref lineStatementPrefix to "#" (kept for compatibility)
    bool useLineStatements = false;
    //! Enables blocks trimming the same way as it does python Jinja2 engine
    bool trimBlocks = false;
    //! Enables blocks stripping (from the left) the same way as it does python Jinja2 engine
    bool lstripBlocks = false;
    //! Templates cache size
    int cacheSize = 400;
    //! If auto_reload is set to true (default) every time a template is requested the loader checks if the source changed and if yes, it will reload the template
    bool autoReload = true;
    //! Extensions set enabled for templates
    Extensions extensions;
    //! Controls Jinja2 compatibility mode
    Jinja2CompatMode jinja2CompatMode = Jinja2CompatMode::None;
    //! Default format for metadata block in the templates
    std::string m_defaultMetadataType = "json";
    //! Keeps the single newline at the end of a template source (Jinja2 `keep_trailing_newline`). By default it is removed, as Jinja2 does
    bool keepTrailingNewline = false;
    //! Sequence that starts a new line in the output (Jinja2 `newline_sequence`): "\n" (default), "\r\n" or "\r". Newlines in template text and string literals are converted to it; other values are used as given
    std::string newlineSequence = "\n";
    //! Delimiters of expressions, statements and comments (Jinja2 `variable_start_string`, `variable_end_string`,
    //! `block_start_string`, `block_end_string`, `comment_start_string`, `comment_end_string`). An empty one keeps its default
    std::string variableStartString = "{{";
    std::string variableEndString = "}}";
    std::string blockStartString = "{%";
    std::string blockEndString = "%}";
    std::string commentStartString = "{#";
    std::string commentEndString = "#}";
    //! Prefix of line statements (Jinja2 `line_statement_prefix`): a line that starts with it, after optional spaces, is a statement. Empty disables them
    std::string lineStatementPrefix;
    //! Prefix of line comments (Jinja2 `line_comment_prefix`): the rest of the line after it is ignored. Empty disables them
    std::string lineCommentPrefix;
    //! Called with the value of every `{{ ... }}` expression before it is printed (Jinja2 `finalize`); its result is
    //! printed instead. Template text is not passed to it. Not set (no `callable`) by default
    UserCallable finalize;
    //! HTML-escapes the output of every `{{ }}` unless the value is marked safe (Jinja2 `autoescape`, a bool)
    bool autoescape = false;
    //! How undefined values behave (Jinja2 `undefined`). A failed use reports ErrorCode::UndefinedError
    UndefinedPolicy undefinedPolicy = UndefinedPolicy::Default;
};

inline bool operator==(const Settings& lhs, const Settings& rhs)
{
    auto tie = [](const Settings& s) {
        return std::tie(s.useLineStatements,
                        s.trimBlocks,
                        s.lstripBlocks,
                        s.cacheSize,
                        s.autoReload,
                        s.extensions.Do,
                        s.extensions.LoopControls,
                        s.extensions.I18n,
                        s.jinja2CompatMode,
                        s.m_defaultMetadataType,
                        s.keepTrailingNewline,
                        s.newlineSequence,
                        s.variableStartString,
                        s.variableEndString,
                        s.blockStartString,
                        s.blockEndString,
                        s.commentStartString,
                        s.commentEndString,
                        s.lineStatementPrefix,
                        s.lineCommentPrefix,
                        s.autoescape,
                        s.undefinedPolicy);
    };
    // A default UserCallable still has an identity of its own, so two unset ones are compared by the missing callable
    const bool sameFinalize = lhs.finalize.callable || rhs.finalize.callable ? lhs.finalize.IsEqual(rhs.finalize) : true;
    return tie(lhs) == tie(rhs) && sameFinalize;
}
inline bool operator!=(const Settings& lhs, const Settings& rhs)
{
    return !(lhs == rhs);
}

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
class JINJA2CPP_EXPORT TemplateEnv
{
public:
    using TimePoint = std::chrono::system_clock::time_point;
    using TimeStamp = std::chrono::steady_clock::time_point;

    /*!
     * \brief Returns global settings for the environment
     *
     * @return Constant reference to the global settings
     */
    const Settings& GetSettings() const { return m_settings; }
    /*!
     * \brief Returns global settings for the environment available for modification
     *
     * @return Reference to the global settings
     */
    Settings& GetSettings() { return m_settings; }

    /*!
     * \brief Replace global settings for the environment with the new ones
     *
     * @param setts New settings
     */
    void SetSettings(const Settings& setts) { m_settings = setts; }

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
        m_filesystemHandlers.push_back(FsHandler{ std::move(prefix), std::move(h) });
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
        m_filesystemHandlers.push_back(FsHandler{ std::move(prefix), std::shared_ptr<IFilesystemHandler>(&h, [](auto*) {}) });
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

    /*!
     * \brief Add a filter to the environment (Jinja2 `env.filters[name] = fn`)
     *
     * The filter is called with the filtered value as its first positional argument, followed by the arguments of
     * the filter call, mapped by \ref UserCallable::argsInfo as for any user callable: `{{ x|name(1, b=2) }}` calls it
     * as `name(x, 1, b=2)`. A filter added under the name of a builtin one replaces it. Templates bind their filters
     * when they are loaded, so a change does not affect templates that are already loaded.
     * Method is thread-safe.
     *
     * @param name   Name of the filter
     * @param filter The filter
     */
    void AddFilter(std::string name, UserCallable filter)
    {
        std::unique_lock<std::shared_timed_mutex> l(m_guard);
        m_filters[std::move(name)] = std::move(filter);
    }
    /*!
     * \brief Remove a filter added with \ref AddFilter. Method is thread-safe.
     */
    void RemoveFilter(const std::string& name)
    {
        std::unique_lock<std::shared_timed_mutex> l(m_guard);
        m_filters.erase(name);
    }
    /*!
     * \brief Add a test to the environment (Jinja2 `env.tests[name] = fn`)
     *
     * The test is called like a filter added with \ref AddFilter, with the tested value first: `x is name(1)` calls
     * `name(x, 1)`, and the truth of its result is the result of the test. A test added under the name of a builtin
     * one replaces it. Method is thread-safe.
     *
     * @param name   Name of the test
     * @param tester The test
     */
    void AddTester(std::string name, UserCallable tester)
    {
        std::unique_lock<std::shared_timed_mutex> l(m_guard);
        m_testers[std::move(name)] = std::move(tester);
    }
    /*!
     * \brief Remove a test added with \ref AddTester. Method is thread-safe.
     */
    void RemoveTester(const std::string& name)
    {
        std::unique_lock<std::shared_timed_mutex> l(m_guard);
        m_testers.erase(name);
    }
    //! The filter added with \ref AddFilter under this name, if any. Method is thread-safe.
    std::optional<UserCallable> FindFilter(const std::string& name) const
    {
        std::shared_lock<std::shared_timed_mutex> l(m_guard);
        auto p = m_filters.find(name);
        return p == m_filters.end() ? std::optional<UserCallable>() : std::optional<UserCallable>(p->second);
    }
    //! The test added with \ref AddTester under this name, if any. Method is thread-safe.
    std::optional<UserCallable> FindTester(const std::string& name) const
    {
        std::shared_lock<std::shared_timed_mutex> l(m_guard);
        auto p = m_testers.find(name);
        return p == m_testers.end() ? std::optional<UserCallable>() : std::optional<UserCallable>(p->second);
    }

    /*!
     * \brief Provide the translations used by the i18n extension (Jinja2 `install_gettext_callables` with `newstyle=True`)
     *
     * Takes effect when \ref Settings::Extensions::I18n is on. `gettext` is called with the message, `ngettext` with the
     * singular message, the plural one and the count, `pgettext` and `npgettext` with the message context first. They
     * return the translated message, which is then formatted with the variables of the `{% trans %}` block or the keyword
     * arguments of the `gettext()` call (`%(name)s`). Messages of wide templates are converted to the string type the callable takes. A callable left
     * unset (no `callable`) keeps the untranslated message, as `install_null_translations` does.
     * Method is thread-safe.
     */
    void InstallGettextCallables(UserCallable gettext, UserCallable ngettext, UserCallable pgettext = UserCallable(), UserCallable npgettext = UserCallable())
    {
        std::unique_lock<std::shared_timed_mutex> l(m_guard);
        m_translations.clear();
        auto install = [this](const char* name, UserCallable& fn) {
            if (fn.callable)
                m_translations[name] = std::move(fn);
        };
        install("gettext", gettext);
        install("ngettext", ngettext);
        install("pgettext", pgettext);
        install("npgettext", npgettext);
    }
    //! The translation callable installed with \ref InstallGettextCallables under this name, if any. Method is thread-safe.
    std::optional<UserCallable> FindGettextCallable(const std::string& name) const
    {
        std::shared_lock<std::shared_timed_mutex> l(m_guard);
        auto p = m_translations.find(name);
        return p == m_translations.end() ? std::optional<UserCallable>() : std::optional<UserCallable>(p->second);
    }

    /*!
     * \brief Call the specified function with the current set of global variables under the internal lock
     *
     * Main purpose of this method is to help external code to enumerate global variables thread-safely. Provided functional object is called under the
     * internal lock with the current set of global variables as an argument.
     *
     * @tparam Fn Type of the functional object to call
     * @param fn Functional object to call
     */
    template<typename Fn>
    void ApplyGlobals(Fn&& fn)
    {
        std::shared_lock<std::shared_timed_mutex> l(m_guard);
        fn(m_globalValues);
    }

    bool IsEqual(const TemplateEnv& other) const
    {
        if (m_filesystemHandlers != other.m_filesystemHandlers)
            return false;
        if (m_settings != other.m_settings)
            return false;
        if (m_globalValues != other.m_globalValues)
            return false;
        if (!IsSameCallables(m_filters, other.m_filters) || !IsSameCallables(m_testers, other.m_testers) || !IsSameCallables(m_translations, other.m_translations))
            return false;
        if (m_templateCache != other.m_templateCache)
            return false;
        if (m_templateWCache != other.m_templateWCache)
            return false;

        return true;
    }

private:
    using CallablesMap = std::unordered_map<std::string, UserCallable>;
    static bool IsSameCallables(const CallablesMap& lhs, const CallablesMap& rhs)
    {
        if (lhs.size() != rhs.size())
            return false;
        for (auto& item : lhs)
        {
            auto p = rhs.find(item.first);
            if (p == rhs.end() || !item.second.IsEqual(p->second))
                return false;
        }
        return true;
    }

    template<typename CharT, typename T, typename Cache>
    auto LoadTemplateImpl(TemplateEnv* env, std::string fileName, const T& filesystemHandlers, Cache& cache);


private:
    struct FsHandler
    {
        std::string prefix;
        FilesystemHandlerPtr handler;
        bool operator==(const FsHandler& rhs) const
        {
            if (prefix != rhs.prefix)
                return false;
            if (handler && rhs.handler && !handler->IsEqual(*rhs.handler))
                return false;
            if ((!handler && rhs.handler) || (handler && !rhs.handler))
                return false;
            return true;
        }
        bool operator!=(const FsHandler& rhs) const
        {
            return !(*this == rhs);
        }
    };

    struct BaseTemplateInfo
    {
        std::optional<TimePoint> lastModification;
        TimeStamp lastAccessTime;
        FilesystemHandlerPtr handler;
        bool operator==(const BaseTemplateInfo& other) const
        {
            if (lastModification != other.lastModification)
                return false;
            if (lastAccessTime != other.lastAccessTime)
                return false;
            if (handler && other.handler && !handler->IsEqual(*other.handler))
                return false;
            if ((!handler && other.handler) || (handler && !other.handler))
                return false;
            return true;
        }
        bool operator!=(const BaseTemplateInfo& other) const
        {
            return !(*this == other);
        }
    };

    struct TemplateCacheEntry : public BaseTemplateInfo
    {
        Template tpl;
        bool operator==(const TemplateCacheEntry& other) const
        {
            return BaseTemplateInfo::operator==(other) && tpl == other.tpl;
        }
        bool operator!=(const TemplateCacheEntry& other) const
        {
            return !(*this == other);
        }
    };

    struct TemplateWCacheEntry : public BaseTemplateInfo
    {
        TemplateW tpl;
        bool operator==(const TemplateWCacheEntry& other) const
        {
            return BaseTemplateInfo::operator==(other) && tpl == other.tpl;
        }
        bool operator!=(const TemplateWCacheEntry& other) const
        {
            return !(*this == other);
        }
    };

    std::vector<FsHandler> m_filesystemHandlers;
    Settings m_settings;
    ValuesMap m_globalValues;
    CallablesMap m_filters;
    CallablesMap m_testers;
    CallablesMap m_translations;
    mutable std::shared_timed_mutex m_guard;
    std::unordered_map<std::string, TemplateCacheEntry> m_templateCache;
    std::unordered_map<std::string, TemplateWCacheEntry> m_templateWCache;
};

} // namespace jinja2

#endif // JINJA2CPP_TEMPLATE_ENV_H
