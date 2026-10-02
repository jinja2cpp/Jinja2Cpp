#ifndef JINJA2CPP_TEMPLATE_H
#define JINJA2CPP_TEMPLATE_H

#include "config.h"
#include "error_info.h"
#include "value.h"

#include <nonstd/expected.hpp>

#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>

namespace jinja2
{
class ITemplateImpl;
class TemplateEnv;
template<typename CharT>
class TemplateImpl;
// Result<T> is expected-lite's nonstd::expected at every C++ standard. Use only what
// std::expected also offers (operator bool, value(), error(), operator*, ->): a later
// release may back Result<T> with std::expected.
template<typename U, typename CharT = char>
using Result = nonstd::expected<U, BasicErrorInfo<CharT>>;
template<typename U>
using ResultW = Result<U, wchar_t>;

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
 * \brief Template object which is used to render templates
 *
 * The main class for rendering templates, instantiated for `char` (\ref Template) and `wchar_t`
 * (\ref TemplateW). It can be used independently or together with \ref TemplateEnv. In the second
 * case it's possible to use templates inheritance and extension.
 *
 * Basic usage:
 * ```c++
 * jinja2::Template tpl;
 * tpl.Load("Hello {{ name }}!");
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
    Result<void, CharT> Load(StringViewType source, std::string name = {});
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
    Result<void, CharT> Load(std::basic_istream<CharT>& stream, std::string name = {});
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
    Result<void, CharT> LoadFromFile(const std::string& fileName);

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
    Result<void, CharT> Render(std::basic_ostream<CharT>& os, const ValuesMap& params) const;
    /*!
     * \brief Render previously loaded template to the stream, taking params from a generic map
     *
     * The same as the \ref ValuesMap overload, for a context that is a reflected object or a JSON
     * object (anything \ref Reflect turns into a \ref GenericMap).
     */
    template<typename Map, std::enable_if_t<std::is_same_v<Map, GenericMap>, int> = 0>
    Result<void, CharT> Render(std::basic_ostream<CharT>& os, const Map& params) const
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

    /* !
     * \brief compares to an other object of the same type
     *
     * @return true if equal
     */
    [[nodiscard]] bool IsEqual(const BasicTemplate& other) const;

private:
    // Out of line, so that the GenericMap overloads (templates only to keep `RenderAsString({})`
    // unambiguous) are still compiled into the library.
    Result<void, CharT> RenderGeneric(std::basic_ostream<CharT>& os, const GenericMap& params) const;
    [[nodiscard]] Result<StringType, CharT> RenderAsStringGeneric(const GenericMap& params) const;

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
// dllexport (C4910), so the declarations are visible to users of the library only.
#ifndef JINJA2CPP_BUILD_AS_SHARED
extern template class JINJA2CPP_EXPORT BasicTemplate<char>;
extern template class JINJA2CPP_EXPORT BasicTemplate<wchar_t>;
#endif

//! Template with narrow (`char`) source and output
using Template = BasicTemplate<char>;
//! Template with wide (`wchar_t`) source and output
using TemplateW = BasicTemplate<wchar_t>;

} // namespace jinja2

#endif // JINJA2CPP_TEMPLATE_H
