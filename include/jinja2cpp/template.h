#ifndef JINJA2CPP_TEMPLATE_H
#define JINJA2CPP_TEMPLATE_H

#include "fwd.h" // IWYU pragma: export
#include "config.h"
#include "error_info.h"
#include "value.h"

// The library is built with expected-lite's own nonstd::expected (CMakeLists.txt), so every consumer
// must see the same type: one that picked std::expected (C++23 without the CMake/pkg-config flag)
// would read Result<T> with another layout (docs/tasks/0002)
#ifndef nsel_CONFIG_SELECT_EXPECTED
#define nsel_CONFIG_SELECT_EXPECTED nsel_EXPECTED_NONSTD
#endif
#include <nonstd/expected.hpp>
#if nsel_USES_STD_EXPECTED
#error "Jinja2C++ needs expected-lite's nonstd::expected: define nsel_CONFIG_SELECT_EXPECTED=nsel_EXPECTED_NONSTD"
#endif

#include <cstddef>
#include <cstdint>
#include <functional>
#include <iosfwd>
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
class ITemplateImpl;
template<typename CharT>
class TemplateImpl;
// Result<T> is expected-lite's nonstd::expected at every C++ standard. Use only what
// std::expected also offers (operator bool, value(), error(), operator*, ->): a later
// release may back Result<T> with std::expected.
template<typename U, typename CharT = char>
using Result = nonstd::expected<U, BasicErrorInfo<CharT>>;
template<typename U>
using ResultW = Result<U, wchar_t>;
//! What `Result::value()` throws on an error; the spelling survives a switch of Result to std::expected
template<typename CharT = char>
using BadResultAccess = nonstd::bad_expected_access<BasicErrorInfo<CharT>>;

/*!
 * \brief Where the output of one `{% generation %}` block lies (\ref RenderOptions::generationSpans)
 *
 * The half-open range `[start, end)` of the block's text in the output, in the units of the
 * output: bytes of UTF-8 for a narrow template, `wchar_t` units for a wide one.
 */
struct GenerationSpan
{
    std::size_t start = 0;
    std::size_t end = 0;
};

/*!
 * \brief The stack a render to a sink runs on, whose sink may switch the thread away (\ref RenderOptions::suspension)
 *
 * A host that runs a render on a stack of its own (a fiber) and switches back to its own code
 * inside `Write`, to resume the render later, gives the render that stack's bounds here
 * (docs/tasks/0457). The Python module's lazy `Template.generate()` does so.
 */
struct RenderSuspension
{
    //! The lowest usable address of the stack, above any guard page
    const void* stackLow = nullptr;
    //! Its size in bytes; 0 when the render runs on its thread's own stack
    std::size_t stackSize = 0;
};

/*!
 * \brief Defaults of the filters Jinja2 reads from `Environment.policies` (\ref Settings::filterPolicies, \ref RenderOptions::filterPolicies)
 *
 * Each field is what a filter uses when its own argument is not given (or is None). They are read
 * when the filter runs, not when the template is loaded, so a render may replace them
 * (\ref RenderOptions::filterPolicies). Jinja2's `compiler.ascii_str` has no counterpart.
 */
struct FilterPolicies
{
    /*!
     * Words merged into the `rel` of every link `urlize` makes, space separated (Jinja2 `urlize.rel`,
     * "noopener" by default). Empty is Jinja2's None: only the filter's `rel` argument and `nofollow` add words.
     */
    std::string urlizeRel = "noopener";
    //! The `target` of `urlize` links when its `target` argument is None or not given (Jinja2 `urlize.target`); unset adds none
    std::optional<std::string> urlizeTarget;
    //! The extra URL schemes of `urlize` when its `extra_schemes` argument is None or not given (Jinja2 `urlize.extra_schemes`),
    //! checked as the argument is. An explicit empty list as the argument is not None and replaces them
    std::vector<std::string> urlizeExtraSchemes;
    //! The `leeway` of `truncate` when its argument is None or not given (Jinja2 `truncate.leeway`); a negative one fails
    //! the filter as a negative `leeway` argument does
    std::int64_t truncateLeeway = 5;
    //! `sort_keys` of the `tojson` filter's JSON (Jinja2 `json.dumps_kwargs`)
    bool jsonSortKeys = true;
    //! `ensure_ascii` of the `tojson` filter's JSON
    bool jsonEnsureAscii = true;
    //! `indent` of the `tojson` filter's JSON, as the text of one level; the filter's own `indent` argument overrides it
    std::optional<std::string> jsonIndent;
    //! `separators` of the `tojson` filter's JSON, (item separator, key separator); unset is `", "` and `": "`, or `","` and `": "`
    //! with an indent, as Python's `json.dumps`
    std::optional<std::pair<std::string, std::string>> jsonSeparators;
    /*!
     * Jinja2's `json.dumps_function`: when set, `tojson` calls it instead of the built-in serializer with the value as its first
     * positional argument and an `indent` keyword argument only when the filter was given one. It must return a string, or
     * the filter fails; the result is then made HTML-safe and marked safe as `tojson`'s own is. The JSON options above
     * do not apply to it.
     */
    UserCallable jsonDumps;
};

//! Field by field; two unset \ref FilterPolicies::jsonDumps callables are equal
JINJA2CPP_EXPORT bool operator==(const FilterPolicies& lhs, const FilterPolicies& rhs);
inline bool operator!=(const FilterPolicies& lhs, const FilterPolicies& rhs)
{
    return !(lhs == rhs);
}

/*!
 * \brief What a render takes besides its params (\ref BasicTemplate::RenderTo)
 *
 * ```cpp
 * jinja2::RenderOptions options;
 * options.globals = std::make_shared<const jinja2::ValuesMap>(jinja2::ValuesMap{{"site", "example.org"}});
 * options.poll = [&] { if (cancelled) throw Cancelled(); };
 * auto result = tpl.RenderTo(out, params, options);
 * ```
 */
struct RenderOptions
{
    /*!
     * The render's globals instead of the environment's (\ref TemplateEnv::AddGlobal): the
     * template and every template it includes, imports or extends see these, under its params.
     * Null, the default, keeps the environment's. Shared, as a value of the map that a callable
     * keeps stays valid after the render.
     */
    std::shared_ptr<const ValuesMap> globals;
    /*!
     * Called on the rendering thread about every 1024 render steps, and more often while a render
     * time limit (Settings::Limits::maxRenderTime) runs: a way to cancel a render, or to check a
     * host's signals. It may throw to stop the render; the render then fails with the exception in
     * \ref BasicErrorInfo::GetException. A render that the poll's render starts with no poll of
     * its own (a callable rendering another template) is polled with it too.
     */
    std::function<void()> poll;
    /*!
     * Receives where the `{% generation %}` blocks of the chat-template preset
     * (\ref ConfigureForChatTemplates) put their text: the assistant token mask of transformers'
     * `apply_chat_template(return_assistant_tokens_mask=True)`. Null, the default, records nothing
     * and costs nothing.
     *
     * - The vector is the host's and must live through the call; it is not shared with another
     *   render running at the same time (a callable's render included). The render clears it first.
     * - Offsets are in units of the output (UTF-8 bytes for a narrow template, `wchar_t` units for a
     *   wide one) counted from the first character this render writes: a render that appends to a
     *   string already holding text counts from the end of that text, so add its size.
     * - The order is the order in which the blocks end, so a nested block comes before the block that
     *   holds it, as in transformers. A block whose body is empty gives an empty span.
     * - A render that fails leaves the vector empty, as it leaves a string target without the output.
     * - As in transformers, a block whose text a buffered part keeps (a `{% set %}` or `{% filter %}`
     *   block, a macro's result used in an expression) is placed where that part began, so its span
     *   need not lie inside the output: check it against the output's size before slicing.
     * - Only `{% generation %}` records spans. A template without one leaves the vector empty; a
     *   host that wants the pre-check of `apply_chat_template` (an error when the template has no
     *   generation tag) can match the template source with transformers' regex
     *   `\{\%-?\s*generation\s*-?\%\}`.
     * - transformers counts characters of a Python str (code points). Convert the byte offsets of a
     *   narrow render to code points (or to tokenizer offsets) before comparing them with its mask.
     * - Each recorded span counts against Settings::Limits::maxSequenceSize: a render with more
     *   blocks than the limit fails with ErrorCode::ResourceLimitExceeded.
     */
    std::vector<GenerationSpan>* generationSpans = nullptr;
    /*!
     * A render to a sink (\ref BasicOutputSink) calls `Write` once for each piece of output the
     * template makes, in the order Jinja2's `Template.generate()` yields them: a run of template
     * text, the value of a `{{ }}`, a block or macro call's output where Jinja2 writes it as one
     * item, however long: a run of text and constant `{{ }}` values Jinja2's compiler folds into one string
     * is one piece, and a value Jinja2 yields when it is empty (`{{ '' }}`, an empty macro call) is an empty
     * `Write`, the only one a sink gets (docs/tasks/0456). False, the
     * default, gathers the pieces into chunks of up to 512 characters, which costs one call a
     * chunk. A render to a string ignores it.
     */
    bool writeEachPiece = false;
    /*!
     * With \ref writeEachPiece, receives before each `Write` whether Jinja2's `generate()` yields that
     * piece as `Markup` rather than `str` (docs/tasks/0493): the value of a `{{ }}` it evaluates at run time
     * under autoescape, text in code whose autoescape is not constant (`{% autoescape x %}`) while
     * autoescape is on, the output of a `{% call %}` block or a recursive loop while autoescape is on, and
     * a `{% filter %}` block whose result is Markup. Text and constants Jinja2 folds are `str`. The text
     * is the same either way. Null, the default, records nothing; the pointee must live through the call.
     */
    bool* pieceIsMarkup = nullptr;
    /*!
     * Makes a render to a sink (\ref BasicOutputSink) suspendable: its sink's `Write` may switch the
     * thread to other code and back before it returns, as a fiber does (docs/tasks/0457). Null, the
     * default, costs nothing. A render to a string ignores it.
     *
     * - While a `Write` runs, the thread is as if this render were not running: other renders,
     *   plain or suspendable, may run, start and finish there, and interleave with this one.
     * - `Write` returns on the thread the render started on. Resuming it on another thread is
     *   undefined behaviour.
     * - The render checks its stack against these bounds, as it checks a thread's: a deep
     *   template fails with a recursion error rather than overflow the stack. A stack no larger
     *   than the engine's reserve (128 KiB, 512 KiB under AddressSanitizer) fails at the first
     *   check. Give at least as much as a thread has (8 MiB) where callables recurse deeply.
     * - The render does not inherit a poll, limits or render depth from a render that is running
     *   where it starts, and lends no state to one: each suspendable render has its own.
     * - Settings::Limits::maxRenderTime counts the time it is suspended too.
     * - The template, and the environment it was loaded from, must outlive the render: a Load of
     *   the same Template object while it is suspended keeps the template it replaces alive until
     *   the render ends.
     * - The pointee must live through the call.
     */
    const RenderSuspension* suspension = nullptr;
    /*!
     * The filter policies of this render instead of \ref Settings::filterPolicies: the template and every template it
     * includes, imports or extends use these. Null, the default, keeps the template's. Shared, as a callable
     * (\ref FilterPolicies::jsonDumps) that a value keeps stays valid after the render.
     */
    std::shared_ptr<const FilterPolicies> filterPolicies;
};

/*!
 * \brief Raw contents of the {% meta %} tag
 *
 * `metadata` is a view into the template source: it stays valid while the template it came from
 * is alive and not reloaded.
 */
template<typename CharT>
struct MetadataInfo
{
    std::string metadataType;
    std::basic_string_view<CharT> metadata;
    SourceLocation location;
};

/*!
 * \brief Receives the output of a render in order (\ref BasicTemplate::RenderTo)
 *
 * Implemented by the host and not owned by the library. `Write` is called on the rendering thread
 * with the output in chunks: up to 512 characters gathered from the template, or a long fragment of
 * the template source on its own. An exception thrown by `Write` ends the render, which returns it
 * as an error the way it returns one from a user callable: a \ref jinja2::Exception keeps its code
 * (so a sink enforcing an output budget throws `Exception{ErrorCode::ResourceLimitExceeded}`), any
 * other exception is kept in the error, and `std::bad_alloc` propagates. No virtual function will be
 * added in 2.x.
 */
template<typename CharT>
class BasicOutputSink
{
public:
    //! `text` is valid during the call only, and never empty but for an empty piece of a render with
    //! RenderOptions::writeEachPiece
    virtual void Write(std::basic_string_view<CharT> text) = 0;

protected:
    BasicOutputSink() = default;
    BasicOutputSink(const BasicOutputSink&) = default;
    BasicOutputSink(BasicOutputSink&&) noexcept = default;
    BasicOutputSink& operator=(const BasicOutputSink&) = default;
    BasicOutputSink& operator=(BasicOutputSink&&) noexcept = default;
    //! Never deleted through the base
    ~BasicOutputSink() = default;
};

namespace detail
{
template<typename Container, typename = void>
struct HasRangeAppend : std::false_type
{
};
template<typename Container>
struct HasRangeAppend<Container, std::void_t<decltype(std::declval<Container&>().append(std::declval<const typename Container::value_type*>(), std::declval<const typename Container::value_type*>()))>>
    : std::true_type
{
};
} // namespace detail

/*!
 * \brief Appends the output of a render to a container
 *
 * Works with any container with `append(first, last)` (`std::basic_string`,
 * `fmt::basic_memory_buffer`) or `insert(end, first, last)` (`std::vector`). Header-only and
 * compiled in the caller, so the library takes no dependency on the container's library. Unlike
 * `RenderTo(StringType&)` it keeps what was written before an error, as any sink does. `out` must
 * outlive the sink, and the sink the `RenderTo` call:
 * ```c++
 * fmt::memory_buffer buffer;
 * jinja2::ContainerSink sink(buffer);
 * auto result = tpl.RenderTo(sink, params);
 * ```
 */
template<typename Container, typename CharT>
class ContainerSink final : public BasicOutputSink<CharT>
{
public:
    explicit ContainerSink(Container& out) noexcept
        : m_out(out)
    {
    }

    void Write(std::basic_string_view<CharT> text) override
    {
        const CharT* first = text.data();
        const CharT* last = text.data() + text.size();
        if constexpr (detail::HasRangeAppend<Container>::value)
        {
            m_out.append(first, last);
        }
        else
        {
            m_out.insert(m_out.end(), first, last);
        }
    }

private:
    Container& m_out;
};

/*!
 * \brief What a template exports, as Python Jinja2's `Template.module` (\ref BasicTemplate::MakeModule)
 *
 * `exports` holds the names the template's top level sets (`set`, `set` blocks, macros, and those
 * of the templates it extends), but not the ones that start with an underscore, the ones set in a
 * loop or a `with` block, or the ones an `import` or `from` binds. Values are copies the host
 * owns; a container that contains itself is cut where it repeats (an empty Value). A macro is a
 * \ref UserCallable that takes positional arguments as `extraPosArgs` (a \ref ValuesList) and
 * keyword arguments as `extraKwArgs` (a \ref ValuesMap) or `args`, and returns its output: a
 * string, or a \ref BasicMarkup "Markup" when the template autoescapes. Its calls run in the
 * module, which they share and keep alive with the template, so a namespace a macro changes stays
 * changed for the next call:
 * ```c++
 * auto module = tpl.MakeModule().value();
 * jinja2::UserCallableParams params;
 * params.extraPosArgs = jinja2::ValuesList{ "World" };
 * auto text = module.exports["greet"].get<jinja2::UserCallable>().callable(params);
 * ```
 * A macro call that fails throws \ref BadResultAccess "BadResultAccess<CharT>" with the error
 * (a user callable that calls it inside a render passes that error on to the render). The module
 * keeps the template it was made from: loading the template again does not change it. Calls of one
 * module's macros from several threads take turns. A macro stored in the environment's globals keeps its template, and so the environment,
 * alive: remove it to free them. So does one passed to a macro of its own module that stores it
 * in the module (`{% set ns.f = f %}`): that module is then never freed.
 */
template<typename CharT>
struct BasicTemplateModule
{
    //! The exported names and their values
    ValuesMap exports;
    //! The output of the template's top level, as Jinja2's `str(module)`
    std::basic_string<CharT> body;
};

//! The exports of a template with narrow output
using TemplateModule = BasicTemplateModule<char>;
//! The exports of a template with wide output
using TemplateModuleW = BasicTemplateModule<wchar_t>;

/*!
 * \brief The attributes Jinja2's `Macro` has, for a macro a module exports
 *
 * Reads a callable of \ref BasicTemplateModule::exports, also one inside an exported list or dict, as Jinja2 reads
 * `module.foo.arguments`: `name` (a string, or none for the caller of a call block), `arguments` (a \ref ValuesList of
 * the parameter names), and the booleans `catch_kwargs`, `catch_varargs` and `caller`.
 *
 * @param callable A callable of a module's exports
 * @return The attributes; an empty map for any other callable
 */
JINJA2CPP_EXPORT ValuesMap MacroAttributes(const UserCallable& callable);

/*!
 * \brief How a module's exports reach the host (\ref ModuleOptions)
 */
enum class ModuleExports
{
    //! The default: values the host can copy, as \ref BasicTemplate::MakeModule "MakeModule" gives without options. An
    //! undefined value is an empty value, a tuple a list, a dictsort pair a map of `key` and `value`, and a dict with
    //! keys that are not strings a map with their str() spelling
    Copies,
    //! The values keep what the template made of them: an undefined value is a \ref MakeUndefined value (\ref UndefinedOf
    //! describes it), a tuple and a dictsort pair an owned \ref GenericList for which \ref GenericList::IsTuple holds, and a
    //! dict with keys that are not strings an owned \ref GenericMap whose \ref IMapItemAccessor::GetItems gives the real
    //! keys (lookups by name use the str() spelling). All of these own their strings and items, as the copies do. A
    //! namespace, a cycler and a joiner are the module's own objects, not copies: a \ref GenericMap whose
    //! \ref IMapItemAccessor::GetObjectClass is "Namespace", "Cycler" or "Joiner", of which every read (\ref
    //! IMapItemAccessor::Find, GetKeys, GetItems, ...) runs in the module, under its lock and limits as a macro call does,
    //! and gives a copy of the attribute. What a macro set is read, and a cycler's `next` and `reset` and a joiner's
    //! `__call__` are macro-like callables of the module that change the object the macros read. A view keeps the module
    //! alive; it is the object again when passed back as an argument to a macro of the same module. Nested in a list or dict
    //! argument a namespace view becomes a copy, but a cycler or a joiner there is a lent map of the live object; to
    //! another module's macro a view is a map to read. A read throws \ref BadResultAccess as a macro
    //! call does. The rest is as in Copies
    Objects
};

/*!
 * \brief What \ref BasicTemplate::MakeModule "MakeModule" changes in how a module's exports reach the host
 */
struct ModuleOptions
{
    //! How the exports, and the results of calls of the module's macros, are given to the host
    ModuleExports exports = ModuleExports::Copies;
    /*!
     * Called on a thread that has to wait for another thread's call of the module (or a read of one of its live objects)
     * to end, with the function that waits: a host whose callables need a lock that waiting thread holds (Python's GIL)
     * lets go of it around `wait()`. Empty, the default, waits as it is. Not called when the module is free or held by
     * this thread. It must call `wait` at most once; if it returns without calling it the module waits itself, and what
     * it throws fails the call or the read
     */
    std::function<void(const std::function<void()>& wait)> waitForTurn;
};

/*!
 * \brief What one host's call of a module's macro changes (\ref CallModuleMacro)
 */
struct ModuleCallOptions
{
    /*!
     * The filter policies of this call instead of the module's: those of \ref RenderOptions::filterPolicies given to
     * \ref BasicTemplate::MakeModule "MakeModule", else the template's \ref Settings::filterPolicies. The macro and every
     * template it includes, imports or extends use these, as a render does. Null, the default, keeps the module's. The call
     * shares them until it returns: Python Jinja2 reads `Environment.policies` each time a filter runs, so a host that
     * mirrors a changed policy passes the current one to each call
     */
    std::shared_ptr<const FilterPolicies> filterPolicies;
};

/*!
 * \brief Calls a macro of a module's exports as its `callable` would, with `options` for this call (docs/tasks/0557)
 *
 * A callable of \ref BasicTemplateModule::exports, also one inside an exported list or dict or one a macro returned, runs
 * in its module with the options' filter policies. Any other callable is called as it is, `options` unused. Throws what the
 * call throws (\ref BadResultAccess "BadResultAccess<CharT>" for a failed macro).
 *
 * @param callable A callable of a module's exports
 * @param params   The call's arguments, as `callable` takes them
 * @param options  What this call changes
 * @return What the call returns
 */
JINJA2CPP_EXPORT Value CallModuleMacro(const UserCallable& callable, const UserCallableParams& params, const ModuleCallOptions& options);

/*!
 * \brief How Python Jinja2 prints a template callable lent to a host's call (\ref DescribeLentCallable)
 */
struct LentCallableInfo
{
    //! Python's repr: `<class 'range'>`, `<Macro 'name'>`, `<function generate_lorem_ipsum at 0x...>`,
    //! `<bound method LoopContext.cycle of <LoopContext 1/2>>`
    std::string repr;
    //! The name of its Python type: `type` for a class among the globals (`range`, `dict`, `cycler`, `joiner`,
    //! `namespace`), `Macro`, `function`, `method` or `builtin_function_or_method`
    std::string typeName;
};

/*!
 * \brief Describes a template callable the engine lends to a host's call, as Python Jinja2 prints it
 *
 * A user callable gets the template's callables as \ref UserCallable values that call back into the render: an
 * argument such as `range` or a macro, or a value read through its context. While that call runs, and on its thread,
 * this tells what the callable is, as a host that mirrors Jinja2's objects needs: `{{ f(range) }}` with `f` printing
 * its argument prints `<class 'range'>` in Jinja2.
 *
 * @param callable A callable a host's call was given
 * @return The description; std::nullopt for any other callable (a host's own, a module's export) and once the call
 *         it was lent to has returned
 */
JINJA2CPP_EXPORT std::optional<LentCallableInfo> DescribeLentCallable(const UserCallable& callable);

/*!
 * \brief Which names of a host call's context a view of it shows (\ref ContextView), as Python Jinja2's `Context`
 * splits them
 */
enum class ContextNames
{
    //! `Context.vars`: the names the template's top level binds where the call is made (`set`, `import`, macros), but
    //! not a loop's variables, the names of a `with` block or a macro's arguments and locals
    TopLevel,
    //! `Context.parent`: the render's params, the globals and the built-ins (`range`, `dict`, ...)
    Parent,
};

/*!
 * \brief A view of the context a user callable gets (\ref UserCallableParams::context) that shows some of its names
 *
 * The context shows every name the template sees where it calls the callable, a loop variable included. Python Jinja2's
 * `Context` holds the template's top-level names and its params and globals only, apart: a host that mirrors it (a
 * `pass_context` callable) reads the two views. A name in both is the top level's where the template reads it. The view
 * reads the render as the context does, lends its callables as it does, and expires with it: once the call returns, a
 * read fails with ErrorCode::TemplateExpired. No name is `loop`.
 *
 * @param context The context of a call (UserCallableParams::context)
 * @param names   Which names the view shows
 * @return The view, a GenericMap (of a call that already returned too, whose reads then fail); a none Value when
 *         `context` is not a call's context
 */
JINJA2CPP_EXPORT Value ContextView(const Value& context, ContextNames names);

/*!
 * \brief Template object which is used to render templates
 *
 * The main class for rendering templates, instantiated for `char` (\ref Template) and `wchar_t`
 * (\ref TemplateW). It can be used independently or together with \ref TemplateEnv. In the second
 * case it's possible to use templates inheritance and extension.
 *
 * Basic usage:
 * ```c++
 * jinja2::Template tpl;
 * if (!tpl.Load("Hello {{ name }}!"))
 *     return; // the result holds the parse error
 * std::string result = tpl.RenderAsString({{"name", "World"}}).value();
 * ```
 *
 * Thread safety: the `const` members (rendering and metadata access) may be called on one template
 * from several threads at once. `Load` and `LoadFromFile` must not run concurrently with any other
 * call on the same object.
 *
 * Copies share the loaded template until one of them loads again: `Load` replaces the template for
 * that object only. Other copies, the environment's cache, renders already running (including the
 * one of a host callable that calls `Load`) and modules keep the template they had, as a Python
 * `Template` is immutable.
 */
template<typename CharT>
class BasicTemplate
{
public:
    using CharType = CharT;
    using StringType = std::basic_string<CharT>;
    using StringViewType = std::basic_string_view<CharT>;

    /*!
     * \brief Default constructor
     */
    BasicTemplate()
        : BasicTemplate(nullptr)
    {
    }
    /*!
     * \brief Initializing constructor
     *
     * Creates instance of the template with the specified template environment object
     *
     * @param env Template environment object which created template should refer to
     */
    explicit BasicTemplate(TemplateEnv* env);
    /*!
     * \brief Copy constructor: the copy shares the loaded template, which neither changes
     */
    BasicTemplate(const BasicTemplate& other) = default;
    /*!
     * Destructor
     */
    ~BasicTemplate();
    /*!
     * \brief Copy assignment: this object then renders the template `other` holds
     *
     * Host code that a `Load` of this object calls (an autoescape selector) may assign it or load
     * it again, on this thread or on one it joins: that `Load` finishes on the template it was
     * parsing, and the object keeps what was assigned or loaded last. Destroying the object from
     * such host code is not supported. There are no move operations: a move copies.
     */
    BasicTemplate& operator=(const BasicTemplate& other);

    /*!
     * \brief Load template from a string
     *
     * Parses the specified string as a Jinja2 template. Accepts anything convertible to a string view:
     * string literals, `std::basic_string`, `std::basic_string_view`. The source is copied. In case of
     * error returns detailed diagnostic
     *
     * @param source   Template source
     * @param name     Optional name of the template (for the error reporting purposes)
     *
     * @return Either nothing or instance of \ref BasicErrorInfo as an error
     */
    [[nodiscard]] Result<void, CharT> Load(StringViewType source, std::string name = {});
    /*!
     * \brief Load template from the stream
     *
     * Takes specified stream object and parses it as a source of Jinja2 template. In case of error returns detailed
     * diagnostic
     *
     * @param stream   Stream object with template description
     * @param name     Optional name of the template (for the error reporting purposes)
     *
     * @return Either nothing or instance of \ref BasicErrorInfo as an error
     */
    [[nodiscard]] Result<void, CharT> Load(std::basic_istream<CharT>& stream, std::string name = {});
    /*!
     * \brief Load template from the specified file
     *
     * Loads file with the specified name and parses it as a source of Jinja2 template. In case of error returns
     * detailed diagnostic
     *
     * @param fileName Name of the file to load
     *
     * @return Either nothing or instance of \ref BasicErrorInfo as an error
     */
    [[nodiscard]] Result<void, CharT> LoadFromFile(const std::string& fileName);

    /*!
     * \brief Render previously loaded template to the stream
     *
     * Renders previously loaded template to the specified stream and specified set of params.
     *
     * @param os      Stream to render template to
     * @param params  Set of params which should be passed to the template engine and can be used within the template
     *
     * @return Either nothing or instance of \ref BasicErrorInfo as an error
     */
    [[nodiscard]] Result<void, CharT> Render(std::basic_ostream<CharT>& os, const ValuesMap& params) const;
    /*!
     * \brief Render previously loaded template to the stream, taking params from a generic map
     *
     * The same as the \ref ValuesMap overload, for a context that is a reflected object or a JSON
     * object (anything \ref Reflect turns into a \ref GenericMap).
     */
    template<typename Map, std::enable_if_t<std::is_same_v<Map, GenericMap>, int> = 0>
    [[nodiscard]] Result<void, CharT> Render(std::basic_ostream<CharT>& os, const Map& params) const
    {
        return RenderGeneric(os, params);
    }
    /*!
     * \brief Render previously loaded template to a string
     *
     * Renders previously loaded template as a string and with specified set of params.
     *
     * @param params  Set of params which should be passed to the template engine and can be used within the template
     *
     * @return Either rendered string or instance of \ref BasicErrorInfo as an error
     */
    [[nodiscard]] Result<StringType, CharT> RenderAsString(const ValuesMap& params) const;
    /*!
     * \brief Render previously loaded template to a string, taking params from a generic map
     *
     * The same as the \ref ValuesMap overload, for a context that is a reflected object or a JSON
     * object (anything \ref Reflect turns into a \ref GenericMap).
     */
    template<typename Map, std::enable_if_t<std::is_same_v<Map, GenericMap>, int> = 0>
    [[nodiscard]] Result<StringType, CharT> RenderAsString(const Map& params) const
    {
        return RenderAsStringGeneric(params);
    }
    /*!
     * \brief Render previously loaded template, appending the output to a string
     *
     * Appends to `out`, so a string can be reused (`out.clear()` first) or collect several renders.
     * It allocates only when `out` lacks the capacity. On an error, `std::bad_alloc` included, `out`
     * is truncated back to its size at entry; its capacity may have grown.
     *
     * @param out     String to append the output to
     * @param params  Set of params which should be passed to the template engine and can be used within the template
     *
     * @return Either nothing or instance of \ref BasicErrorInfo as an error
     */
    [[nodiscard]] Result<void, CharT> RenderTo(StringType& out, const ValuesMap& params) const;
    /*!
     * \brief Render previously loaded template, appending the output to a string, taking params from a generic map
     *
     * The same as the \ref ValuesMap overload, for a context that is a reflected object or a JSON
     * object (anything \ref Reflect turns into a \ref GenericMap).
     */
    template<typename Map, std::enable_if_t<std::is_same_v<Map, GenericMap>, int> = 0>
    [[nodiscard]] Result<void, CharT> RenderTo(StringType& out, const Map& params) const
    {
        return RenderToGeneric(out, params);
    }
    //! A temporary string would lose the output; deleted rather than left to MSVC, which binds a
    //! temporary to `StringType&` as an extension
    Result<void, CharT> RenderTo(StringType&& out, const ValuesMap& params) const = delete;
    template<typename Map, std::enable_if_t<std::is_same_v<Map, GenericMap>, int> = 0>
    Result<void, CharT> RenderTo(StringType&& out, const Map& params) const = delete;
    /*!
     * \brief Render previously loaded template, streaming the output to a sink
     *
     * Writes the output to `sink` while rendering, so it is never held whole. The chunks the sink
     * received before an error stay written, as with Python's `Template.generate()`; output still
     * in the render's buffer (under 512 characters) when the error happens is not written.
     *
     * @param sink    Sink to write the output to (\ref BasicOutputSink)
     * @param params  Set of params which should be passed to the template engine and can be used within the template
     *
     * @return Either nothing or instance of \ref BasicErrorInfo as an error
     */
    [[nodiscard]] Result<void, CharT> RenderTo(BasicOutputSink<CharT>& sink, const ValuesMap& params) const;
    /*!
     * \brief Render previously loaded template, streaming the output to a sink, taking params from a generic map
     *
     * The same as the \ref ValuesMap overload, for a context that is a reflected object or a JSON
     * object (anything \ref Reflect turns into a \ref GenericMap).
     */
    template<typename Map, std::enable_if_t<std::is_same_v<Map, GenericMap>, int> = 0>
    [[nodiscard]] Result<void, CharT> RenderTo(BasicOutputSink<CharT>& sink, const Map& params) const
    {
        return RenderToGeneric(sink, params);
    }
    /*!
     * \brief Render previously loaded template, appending the output to a string, with \ref RenderOptions
     *
     * As `RenderTo(out, params)`, with the render's own globals and poll.
     */
    [[nodiscard]] Result<void, CharT> RenderTo(StringType& out, const ValuesMap& params, const RenderOptions& options) const;
    Result<void, CharT> RenderTo(StringType&& out, const ValuesMap& params, const RenderOptions& options) const = delete;
    /*!
     * \brief Render previously loaded template, streaming the output to a sink, with \ref RenderOptions
     *
     * As `RenderTo(sink, params)`, with the render's own globals and poll.
     */
    [[nodiscard]] Result<void, CharT> RenderTo(BasicOutputSink<CharT>& sink, const ValuesMap& params, const RenderOptions& options) const;
    /*!
     * \brief Get metadata, provided in the {% meta %} tag
     *
     * @return Parsed metadata as a generic map value or instance of \ref BasicErrorInfo as an error
     */
    [[nodiscard]] Result<GenericMap, CharT> GetMetadata() const;
    /*!
     * \brief Get non-parsed metadata, provided in the {% meta %} tag
     *
     * @return Non-parsed metadata information or instance of \ref BasicErrorInfo as an error
     */
    [[nodiscard]] Result<MetadataInfo<CharT>, CharT> GetMetadataRaw() const;
    /*!
     * \brief The names the template reads from its params, as `jinja2.meta.find_undeclared_variables`
     *
     * Every name a render of the template looks up among the params and the environment's
     * globals, sorted, without the globals: a name the template reads before or without setting
     * it, or sets only in a branch of `if`. Templates the template includes, imports or extends
     * are not looked into, as in Jinja2. Parses the template's source again.
     *
     * @return The names, or the error of the parse (TemplateNotParsed before a successful Load)
     */
    [[nodiscard]] Result<std::vector<std::string>, CharT> FindUndeclaredVariables() const;
    /*!
     * \brief The names of the templates the template refers to, as `jinja2.meta.find_referenced_templates`
     *
     * The names the `extends`, `include`, `import` and `from` tags give, in source order. A tag
     * whose name is a string literal gives it, a list or tuple literal of strings gives each of them,
     * and any other expression gives `std::nullopt`, as Jinja2 gives `None`. The templates are not
     * loaded or looked into. Parses the template's source again.
     *
     * @return The names, or the error of the parse (TemplateNotParsed before a successful Load)
     */
    [[nodiscard]] Result<std::vector<std::optional<std::string>>, CharT> FindReferencedTemplates() const;
    /*!
     * \brief Renders the template once and returns what it exports, as Jinja2's `make_module(vars)`
     *
     * The template is rendered with `params` on top of the environment's globals, as by \ref Render,
     * and its top level's names and output are returned (\ref BasicTemplateModule). Without params
     * this is Jinja2's `Template.module`, except that each call makes a new module.
     *
     * @param params  Set of params which the template's top level and its macros see
     *
     * @return The module, or the error of the render (TemplateNotParsed before a successful Load)
     */
    [[nodiscard]] Result<BasicTemplateModule<CharT>, CharT> MakeModule(const ValuesMap& params = {}) const;
    /*!
     * \brief \ref MakeModule with the host's globals, poll and filter policies of \ref RenderOptions
     *
     * `options.globals`, when not null, take the place of the environment's globals for the module: its top level
     * and the calls of its macros see them under `params`. They are copied into the module, as the params are.
     * `options.poll` is called as it is in a render, during the render of the module's top level and during each
     * call of one of its macros (\ref BasicTemplateModule), which keeps a copy of it for those calls: a call made in
     * a render polled by another poll is polled by this one. It may throw to stop the render or the call, which
     * then fails with the exception (a call throws \ref BadResultAccess "BadResultAccess<CharT>" holding it in its
     * error). It is called on the thread of the call and must stay callable while a macro of the module may be.
     * Calls of one module take turns (\ref BasicTemplateModule), and the poll runs inside its call: a poll that
     * waits for another thread calling the same module deadlocks.
     * `options.filterPolicies`, when not null, take the place of the template's for the module's top level and the
     * calls of its macros, which share them (\ref CallModuleMacro replaces them for one call).
     * The other options are for a render's output and are ignored.
     *
     * @param params   Set of params which the template's top level and its macros see
     * @param options  The globals, the poll and the filter policies of the module
     *
     * @return The module, or the error of the render (TemplateNotParsed before a successful Load)
     */
    [[nodiscard]] Result<BasicTemplateModule<CharT>, CharT> MakeModule(const ValuesMap& params, const RenderOptions& options) const;
    /*!
     * \brief \ref MakeModule with the options of the previous overload and \ref ModuleOptions
     *
     * With \ref ModuleExports::Objects the exports, and the results of the calls of the module's macros, keep an undefined
     * value, a tuple and a dict with keys that are not strings (\ref ModuleExports::Objects). With \ref ModuleExports::Copies
     * this is the previous overload.
     *
     * @param params   Set of params which the template's top level and its macros see
     * @param options  The globals, the poll and the filter policies of the module
     * @param module   How the exports reach the host
     *
     * @return The module, or the error of the render (TemplateNotParsed before a successful Load)
     */
    [[nodiscard]] Result<BasicTemplateModule<CharT>, CharT> MakeModule(const ValuesMap& params, const RenderOptions& options, const ModuleOptions& module) const;

    /* !
     * \brief compares to an other object of the same type
     *
     * @return true if equal
     */
    [[nodiscard]] bool IsEqual(const BasicTemplate& other) const;

private:
    // Out of line, so that the GenericMap overloads (templates only to keep `RenderAsString({})`
    // unambiguous) are still compiled into the library.
    [[nodiscard]] Result<void, CharT> RenderGeneric(std::basic_ostream<CharT>& os, const GenericMap& params) const;
    [[nodiscard]] Result<StringType, CharT> RenderAsStringGeneric(const GenericMap& params) const;
    [[nodiscard]] Result<void, CharT> RenderToGeneric(StringType& out, const GenericMap& params) const;
    [[nodiscard]] Result<void, CharT> RenderToGeneric(BasicOutputSink<CharT>& sink, const GenericMap& params) const;

    std::shared_ptr<ITemplateImpl> m_impl;
    friend class TemplateImpl<CharT>;
};

template<typename CharT>
bool operator==(const BasicTemplate<CharT>& lhs, const BasicTemplate<CharT>& rhs)
{
    return lhs.IsEqual(rhs);
}

template<typename CharT>
bool operator!=(const BasicTemplate<CharT>& lhs, const BasicTemplate<CharT>& rhs)
{
    return !lhs.IsEqual(rhs);
}

// Both instantiations are compiled into the library. MSVC rejects `extern` together with
// dllexport (C4910), so there the declarations are visible to users of the library only.
// GCC takes the visibility of an explicit instantiation only from its first declaration,
// so elsewhere the library sees them too (src/template.cpp).
#if !defined(JINJA2CPP_BUILD_AS_SHARED) || !defined(_WIN32)
extern template class JINJA2CPP_EXPORT BasicTemplate<char>;
extern template class JINJA2CPP_EXPORT BasicTemplate<wchar_t>;
#endif

//! Template with narrow (`char`) source and output
using Template = BasicTemplate<char>;
//! Template with wide (`wchar_t`) source and output
using TemplateW = BasicTemplate<wchar_t>;

/*!
 * \brief Frees what renders on the calling thread keep for the next render
 *
 * Each thread that renders keeps reusable memory: slot and loop frames, the tables of the params,
 * an escape buffer, and the globals of the last \ref TemplateEnv it rendered with, which keeps
 * them (and what their callables capture) alive after that environment is destroyed. A template
 * that a Load called from a render replaced is kept too, until the next Load on the thread
 * outside a render, as is a template that nested Loads replaced while its own first Load ran
 * (host code that Load called), once that Load returned. This frees
 * them on the calling thread: before an idle pool thread parks, or before a plugin is unloaded.
 * The next render on the thread allocates them again.
 *
 * @return false, having freed nothing, when a render is running on this thread (called from a
 *         user callable or a sink); true otherwise
 */
JINJA2CPP_EXPORT bool ReleaseThreadCaches() noexcept;

} // namespace v2
} // namespace jinja2

JINJA2CPP_SUPPRESS_DLL_INTERFACE_WARNINGS_END

#endif // JINJA2CPP_TEMPLATE_H
