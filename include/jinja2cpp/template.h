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

#include <iostream>
#include <memory>
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
    //! `text` is never empty and is valid during the call only
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
 * (a user callable that calls it inside a render passes that error on to the render). Once the
 * template is loaded again, a call fails with ErrorCode::TemplateExpired. Calls of one module's
 * macros from several threads take turns; like a render, they must not run while the template
 * loads. A macro stored in the environment's globals keeps its template, and so the environment,
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
 * from several threads at once. `Load` and `LoadFromFile` replace the template and must not run
 * concurrently with any other call on the same object. Copies share the loaded template.
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
     * Destructor
     */
    ~BasicTemplate();

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
 * them (and what their callables capture) alive after that environment is destroyed. This frees
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
