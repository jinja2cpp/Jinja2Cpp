#ifndef JINJA2CPP_TEMPLATE_ENV_H
#define JINJA2CPP_TEMPLATE_ENV_H

#include "fwd.h" // IWYU pragma: export
#include "config.h"
#include "error_info.h" // IWYU pragma: export
#include "filesystem_handler.h"
#include "template.h"

#include <jinja2cpp/value.h>

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

JINJA2CPP_SUPPRESS_DLL_INTERFACE_WARNINGS_BEGIN

namespace jinja2
{
inline namespace v2
{

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

//! When `include`, `extends` and `import` look up the template they name (\ref Settings::templateLookup)
enum class TemplateLookup
{
    OncePerRender, //!< The first use of a name in a render looks it up; later uses in the same render reuse it
    EveryUse,      //!< Every use looks it up, as Jinja2 does: with autoReload a template changed during a render is seen by its next use
};

//! Chooses whether a template escapes its output from the template's name (Jinja2 `autoescape` given a callable, usually
//! `select_autoescape`). The name is the one the template is loaded with, empty for a template loaded from a string without one.
//! Copies share the callable and compare equal; selectors made apart compare unequal, whatever they call
class AutoescapeSelector
{
public:
    using Function = std::function<bool(const std::string& templateName)>;

    AutoescapeSelector() = default;
    template<typename Fn, typename = std::enable_if_t<std::is_invocable_r_v<bool, const Fn&, const std::string&>>>
    AutoescapeSelector(Fn fn) // NOLINT(google-explicit-constructor): assigned from a lambda, as Jinja2 takes a callable
        : m_fn(std::make_shared<const Function>(std::move(fn)))
    {
    }

    //! Whether a callable is set
    explicit operator bool() const { return m_fn && *m_fn; }
    //! Throws std::bad_function_call when no callable is set, as std::function does
    bool operator()(const std::string& templateName) const
    {
        if (!m_fn)
        {
            throw std::bad_function_call();
        }
        return (*m_fn)(templateName);
    }

    friend bool operator==(const AutoescapeSelector& lhs, const AutoescapeSelector& rhs) { return lhs.m_fn == rhs.m_fn; }
    friend bool operator!=(const AutoescapeSelector& lhs, const AutoescapeSelector& rhs) { return !(lhs == rhs); }

private:
    std::shared_ptr<const Function> m_fn;
};

//! Jinja2 `select_autoescape`: a template whose name ends with `.` and one of `enabledExtensions` escapes, one with one of
//! `disabledExtensions` does not, ignoring the case of ASCII letters; any other named template takes `defaultValue`, and a template loaded from a
//! string without a name `defaultForString`
JINJA2CPP_EXPORT AutoescapeSelector SelectAutoescape(std::vector<std::string> enabledExtensions = { "html", "htm", "xml" },
                                                     std::vector<std::string> disabledExtensions = {},
                                                     bool defaultForString = true,
                                                     bool defaultValue = false);

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
    //! How often a render requests the templates it includes, extends or imports is \ref templateLookup
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
    //! printed instead. Template text is not passed to it. An undefined value is passed as its text: an empty string,
    //! or `{{ name }}` with \ref UndefinedPolicy::Debug; with \ref UndefinedPolicy::Strict printing it raises before
    //! the call. Not set (no `callable`) by default
    UserCallable finalize;
    //! HTML-escapes the output of every `{{ }}` unless the value is marked safe (Jinja2 `autoescape`, a bool)
    bool autoescape = false;
    //! When set, decides \ref autoescape for each template by its name instead (Jinja2 `autoescape` given a callable),
    //! for example `SelectAutoescape({"html", "xml"})`. A template keeps what it was loaded with
    AutoescapeSelector autoescapeSelector;
    //! How undefined values behave (Jinja2 `undefined`). A failed use reports ErrorCode::UndefinedError
    UndefinedPolicy undefinedPolicy = UndefinedPolicy::Default;
    //! When `include`, `extends` and `import` look up their template in the environment. `OncePerRender` (default) takes the
    //! environment's lock and checks autoReload once per name per render, so renders on many threads do not contend;
    //! `EveryUse` matches Jinja2, which looks the template up each time the statement runs. A template keeps the value it
    //! was loaded with. With `EveryUse` and `cacheSize = 0` every use parses the file again and the copies are freed when
    //! the render ends, not after each use
    TemplateLookup templateLookup = TemplateLookup::OncePerRender;
    //! Lets a \ref UserCallable passed in the render parameters or the globals be used as a filter or a test (`{{ x|fn }}`,
    //! `x is fn`, `map('fn')`), as in Jinja2C++ 1.x. By default, as in Jinja2, a filter or test is one built in or added
    //! with \ref TemplateEnv::AddFilter or \ref TemplateEnv::AddTest, and a name that is neither fails \ref Template::Load
    //! (ErrorCode::UnknownFilter, ErrorCode::UnknownTest) unless it is inside an `if`, where it fails when it runs.
    //! With this flag such names are looked up when they run instead
    bool userCallablesAsFilters = false;

    //! Bounds on what one Load or Render may do, for templates that are not trusted (docs/api-2.0.md). A Render that
    //! passes one fails with ErrorCode::ResourceLimitExceeded, `extraParams[0]` the message and `extraParams[1]` the
    //! field's name (`"maxRenderTime"`); the depth limits report ErrorCode::RecursionLimitExceeded as before. A Render
    //! started by a host callable inside another one keeps the limits of its own template. \ref UntrustedLimits has
    //! values for a host rendering templates its users write
    struct Limits
    {
        //! Items of a `range()`; 0 is no limit (Jinja2 sandbox `MAX_RANGE`)
        std::uint64_t maxRange = 0;
        //! Items or characters one operation builds (a list, a string); 0 keeps the built-in cap of 2^26, which a
        //! larger value does not raise
        std::uint64_t maxSequenceSize = 0;
        //! Characters one Render writes to its output, checked at every step: one expression may pass it by its own
        //! output. Text a template captures (`{% set %}` blocks, `caller()`, `{% filter %}`) is not output; 0 is
        //! no limit
        std::uint64_t maxOutputSize = 0;
        //! Characters of a template's source at Load, a template that `include`, `extends` or `import` loads
        //! included; 0 is no limit
        std::uint64_t maxTemplateSize = 0;
        //! Templates one Render may load for `include`, `extends` and `import`; 0 is no limit
        std::uint64_t maxTemplateLoads = 0;
        //! Steps one Render may take: a step is a statement body (each loop iteration's among them), an item a loop's
        //! `if` looks at, a level of \ref maxRenderDepth (the template itself, a macro call) or the evaluation of an
        //! operator, call or filter. An approximate unit, which may change between releases; 0 is no limit
        std::uint64_t maxRenderSteps = 0;
        //! Time one Render may take, checked every 16 steps, after each host callable returns and when the render
        //! ends. Nothing is interrupted, so one step (a filter over a long string, a host callable) may overrun it;
        //! 0, a negative time or one past the clock's range is no limit
        std::chrono::milliseconds maxRenderTime{ 0 };
        //! Nesting of the template, macro and caller() calls, super(), recursive loops, included, imported and parent
        //! templates (Python fails at about 250 macro calls). The depth limits stop at the thread's stack too; 0
        //! leaves only that check
        std::uint32_t maxRenderDepth = 256;
        //! Nesting of an expression: brackets, calls, subscripts, filter arguments. The default stops where Python
        //! Jinja2 does (70 brackets)
        std::uint32_t maxExpressionDepth = 70;
        //! Operators chained along one path of an expression (a + b + c, x|f|g, a.b.c). The default stops where
        //! Python Jinja2 does (492 unary operators or 492 `+`)
        std::uint32_t maxExpressionOperators = 491;
        //! Statement blocks open at once, the template's top level included, `elif` and `else` not. The default
        //! stops where Python Jinja2 does (248 nested `block`, `with`, `filter` tags); CPython's compiler stops
        //! nested `if` and `for` earlier, which Jinja2C++ does not copy
        std::uint32_t maxBlockNesting = 248;
    };
    //! Bounds on Load and Render; none but the depth limits by default
    Limits limits;
};

//! Limits for templates a host's users write, anchored on Jinja2's `SandboxedEnvironment`: `range()` of at most
//! 100000 items, sequences of at most 2^20 items, 16 MiB of output, 10^7 render steps, no time limit. The values may
//! change between releases. Not every operation that builds a string or list checks maxSequenceSize yet, so
//! these limits do not bound a render's memory: a host that needs that bound runs untrusted renders where memory is
//! limited too
inline Settings::Limits UntrustedLimits()
{
    Settings::Limits limits;
    limits.maxRange = 100000;
    limits.maxSequenceSize = std::uint64_t{ 1 } << 20;
    limits.maxOutputSize = std::uint64_t{ 16 } << 20;
    limits.maxRenderSteps = 10000000;
    return limits;
}

//! Settings for templates a host's users write: \ref UntrustedLimits with `autoescape` on
inline Settings UntrustedSettings()
{
    Settings settings;
    settings.limits = UntrustedLimits();
    settings.autoescape = true;
    return settings;
}

//! Field by field, \ref Settings::Limits included; two unset \ref Settings::finalize callables are equal
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
     * \brief Set the big-integer arithmetic of the templates made from now on (see \ref BigIntBackend)
     *
     * With a backend, integer arithmetic in templates is exact at any size, as in Python; without one (`nullptr`),
     * a result outside `int64_t` fails the render with ErrorCode::IntegerOutOfRange. Every environment starts with
     * \ref GetBuiltinBigIntBackend. A template uses the backend its environment had when the template was made
     * (\ref LoadTemplate, or a template constructed with this environment), as it does the settings. A template \ref LoadTemplate has cached keeps its backend until it is loaded again. The backend must outlive every template that uses it.
     * Method is thread-unsafe, as \ref SetSettings is.
     *
     * @param backend The backend, or `nullptr` for none
     *
     * @return false, changing nothing, when the table is not one this library can use: `structSize` smaller than
     *         version 1's, `version` other than 1, a null name or function, or an object larger than
     *         \ref MaxBigIntObjectSize or aligned beyond \ref MaxBigIntObjectAlign; or when memory runs out
     */
    bool SetBigIntBackend(const BigIntBackend* backend) noexcept;
    //! The backend set by \ref SetBigIntBackend, or the built-in one
    [[nodiscard]] const BigIntBackend* GetBigIntBackend() const noexcept;

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
     * May run concurrently with other loads, \ref FromString, renders and changes to the globals, filters, tests and
     * translations, and from host code a render calls. Not with configuration: \ref SetSettings, the mutable
     * \ref GetSettings, \ref AddFilesystemHandler, \ref SetBigIntBackend (docs/api-2.0.md, 3.10).
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
     * May run concurrently with other loads, \ref FromString, renders and changes to the globals, filters, tests and
     * translations, and from host code a render calls. Not with configuration: \ref SetSettings, the mutable
     * \ref GetSettings, \ref AddFilesystemHandler, \ref SetBigIntBackend (docs/api-2.0.md, 3.10).
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
     * `import` templates of the environment's filesystem handlers. Thread-safe as \ref LoadTemplate is.
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

} // namespace v2
} // namespace jinja2

JINJA2CPP_SUPPRESS_DLL_INTERFACE_WARNINGS_END

#endif // JINJA2CPP_TEMPLATE_ENV_H
