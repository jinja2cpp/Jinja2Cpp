#ifndef JINJA2CPP_TEMPLATE_ENV_H
#define JINJA2CPP_TEMPLATE_ENV_H

#include "config.h"
#include "error_info.h" // IWYU pragma: export
#include "filesystem_handler.h"
#include "template.h"

#include <jinja2cpp/value.h>

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace jinja2
{

class IFilesystemHandler;

namespace detail
{
class TemplateEnvImpl;
struct TemplateEnvAccess;
} // namespace detail

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
        bool doStatement = false;  //!< Enable use of `do` statement (Jinja2 `jinja2.ext.do`)
        bool loopControls = false; //!< Enable use of `break` and `continue` statements in loops (Jinja2 `jinja2.ext.loopcontrols`)
        //! Enable `{% trans %}` blocks and the `_`, `gettext`, `ngettext`, `pgettext` and `npgettext` globals (Jinja2 `jinja2.ext.i18n`
        //! with newstyle gettext). Messages are not translated unless \ref TemplateEnv::InstallGettextCallables provides the translations
        bool i18n = false;
    };

    //! Enables blocks trimming the same way as it does python Jinja2 engine
    bool trimBlocks = false;
    //! Enables blocks stripping (from the left) the same way as it does python Jinja2 engine
    bool lstripBlocks = false;
    //! Templates cache size
    int cacheSize = 400;
    //! If auto_reload is set to true (default) every time a template is requested the loader checks if the source changed and if yes, it will reload the template.
    //! A render resolves each name it includes, extends or imports once, so a template changed during a render is picked up by the next render
    bool autoReload = true;
    //! Extensions set enabled for templates
    Extensions extensions;
    //! Default format for metadata block in the templates
    std::string defaultMetadataType = "json";
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

//! Field by field; two unset \ref Settings::finalize callables are equal
JINJA2CPP_EXPORT bool operator==(const Settings& lhs, const Settings& rhs);
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
 * It's possible to load templates from the environment via \ref LoadTemplate, \ref LoadTemplateW or \ref FromString
 * methods or to pass instance of the environment directly to the \ref Template via constructor.
 *
 * As in Jinja2, configure the environment (settings, filesystem handlers) before loading templates: changes made
 * afterwards are not synchronised with templates being loaded or rendered in other threads. Globals, filters, tests
 * and translations can be changed at any time.
 *
 * The state of the environment is shared with the templates it creates: a template keeps it alive, so it can still
 * be rendered and load templates it includes after the environment object is destroyed (no caching then).
 */
class JINJA2CPP_EXPORT TemplateEnv
{
public:
    TemplateEnv();
    ~TemplateEnv();
    TemplateEnv(const TemplateEnv&) = delete;
    TemplateEnv& operator=(const TemplateEnv&) = delete;

    /*!
     * \brief Returns global settings for the environment
     *
     * @return Constant reference to the global settings
     */
    [[nodiscard]] const Settings& GetSettings() const;
    /*!
     * \brief Returns global settings for the environment available for modification
     *
     * Change them before loading templates: a template copies the settings when it is loaded.
     *
     * @return Reference to the global settings
     */
    Settings& GetSettings();

    /*!
     * \brief Replace global settings for the environment with the new ones
     *
     * @param setts New settings
     */
    void SetSettings(const Settings& setts);

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
    void AddFilesystemHandler(std::string prefix, FilesystemHandlerPtr h);
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
     * @param h      Reference to the handler. Its lifetime is controlled by the caller and must exceed the lifetime of the
     *               environment and of every template loaded from it
     */
    void AddFilesystemHandler(std::string prefix, IFilesystemHandler& h);
    /*!
     * \brief Load narrow char template with the specified name via registered file handlers
     *
     * In case of specified file present in any of the registered handlers, template is loaded and parsed. If any
     * error occurred during the loading or parsing detailed diagnostic will be returned.
     * Method is thread-unsafe. It's dangerous to add new filesystem handlers and load templates simultaneously.
     *
     * @param fileName Template name to load
     *
     * @return Either loaded template or load/parse error. See \ref BasicErrorInfo
     */
    [[nodiscard]] Result<Template> LoadTemplate(std::string fileName);
    /*!
     * \brief Load wide char template with the specified name via registered file handlers
     *
     * In case of specified file present in any of the registered handlers, template is loaded and parsed. If any
     * error occurred during the loading or parsing detailed diagnostic will be returned.
     * Method is thread-unsafe. It's dangerous to add new filesystem handlers and load templates simultaneously.
     *
     * @param fileName Template name to load
     *
     * @return Either loaded template or load/parse error. See \ref BasicErrorInfo
     */
    [[nodiscard]] ResultW<TemplateW> LoadTemplateW(std::string fileName);
    /*!
     * \brief Parse a template from a string within this environment (Jinja2 `env.from_string`)
     *
     * The template is not cached and is not reachable by name from other templates; it can `include`, `extends` and
     * `import` templates of the environment's filesystem handlers.
     *
     * @param source Template source
     * @param name   Name the template reports in errors; "noname.j2tpl" if empty
     *
     * @return Either parsed template or parse error
     */
    [[nodiscard]] Result<Template> FromString(std::string_view source, std::string name = std::string());
    //! Wide char version of \ref FromString
    [[nodiscard]] ResultW<TemplateW> FromString(std::wstring_view source, std::string name = std::string());

    /*!
     * \brief Add global variable to the environment
     *
     * Adds global variable which can be referred in any template which is loaded within this environment object.
     * Method is thread-safe.
     *
     * @param name Name of the variable
     * @param val  Value of the variable
     */
    void AddGlobal(std::string name, Value val);
    /*!
     * \brief Remove global variable from the environment
     *
     * Removes global variable from the environment.
     * Method is thread-safe.
     *
     * @param name Name of the variable
     */
    void RemoveGlobal(const std::string& name);

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
    void AddFilter(std::string name, UserCallable filter);
    /*!
     * \brief Remove a filter added with \ref AddFilter. Method is thread-safe.
     */
    void RemoveFilter(const std::string& name);
    /*!
     * \brief Add a test to the environment (Jinja2 `env.tests[name] = fn`)
     *
     * The test is called like a filter added with \ref AddFilter, with the tested value first: `x is name(1)` calls
     * `name(x, 1)`, and the truth of its result is the result of the test. A test added under the name of a builtin
     * one replaces it. Method is thread-safe.
     *
     * @param name Name of the test
     * @param test The test
     */
    void AddTest(std::string name, UserCallable test);
    /*!
     * \brief Remove a test added with \ref AddTest. Method is thread-safe.
     */
    void RemoveTest(const std::string& name);
    //! The filter added with \ref AddFilter under this name, if any. Method is thread-safe.
    [[nodiscard]] std::optional<UserCallable> FindFilter(const std::string& name) const;
    //! The test added with \ref AddTest under this name, if any. Method is thread-safe.
    [[nodiscard]] std::optional<UserCallable> FindTest(const std::string& name) const;

    /*!
     * \brief Provide the translations used by the i18n extension (Jinja2 `install_gettext_callables` with `newstyle=True`)
     *
     * Takes effect when \ref Settings::Extensions::i18n is on. `gettext` is called with the message, `ngettext` with the
     * singular message, the plural one and the count, `pgettext` and `npgettext` with the message context first. They
     * return the translated message, which is then formatted with the variables of the `{% trans %}` block or the keyword
     * arguments of the `gettext()` call (`%(name)s`). Messages of wide templates are converted to the string type the callable takes. A callable left
     * unset (no `callable`) keeps the untranslated message, as `install_null_translations` does.
     * Method is thread-safe.
     */
    void InstallGettextCallables(UserCallable gettext, UserCallable ngettext, UserCallable pgettext = UserCallable(), UserCallable npgettext = UserCallable());
    //! The translation callable installed with \ref InstallGettextCallables under this name, if any. Method is thread-safe.
    [[nodiscard]] std::optional<UserCallable> FindGettextCallable(const std::string& name) const;

    /*!
     * \brief Call the specified function with the current set of global variables under the internal lock
     *
     * Main purpose of this method is to help external code to enumerate global variables thread-safely. Provided functional object is called under the
     * internal (shared) lock with the current set of global variables as an argument; it must not change the environment.
     *
     * @param fn Functional object to call
     */
    void ApplyGlobals(const std::function<void(const ValuesMap&)>& fn) const;

private:
    friend struct detail::TemplateEnvAccess;
    explicit TemplateEnv(std::shared_ptr<detail::TemplateEnvImpl> impl);

    std::shared_ptr<detail::TemplateEnvImpl> m_impl;
};

} // namespace jinja2

#endif // JINJA2CPP_TEMPLATE_ENV_H
