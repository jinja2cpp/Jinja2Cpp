#ifndef JINJA2CPP_MARKUP_H
#define JINJA2CPP_MARKUP_H

#include <string>
#include <string_view>
#include <utility>

namespace jinja2
{
/*!
 * \brief Text that is already safe HTML: Python's `markupsafe.Markup`
 *
 * A \ref Value that holds a Markup renders unescaped under autoescape and passes the `is escaped` test, as a
 * `markupsafe.Markup` does in Python; a plain string is escaped. Like Python's `Markup`, which is a `str`
 * subclass, this type is a `std::basic_string` and can be used wherever one is read, but every constructor is
 * explicit: the text is trusted not to need escaping, so making one is always a deliberate act. The string
 * operations of the base class (`+`, `substr`, ...) return plain strings, not Markup.
 *
 * ```c++
 *  params["banner"] = jinja2::Markup("<b>sale</b>"); // {{ banner }} prints <b>sale</b> even with autoescape on
 * ```
 *
 * A \ref Value also holds a Markup when a template result or a user callable's return value is Markup in the
 * template (for instance after the `safe` filter), and \ref Value::IsString, \ref Value::AsString and
 * `Value::As<std::string>()` accept it; \ref Value::IsMarkup tells the two apart.
 */
template<typename CharT>
class BasicMarkup : public std::basic_string<CharT>
{
public:
    //! The base class
    using string_type = std::basic_string<CharT>;

    //! Empty markup
    BasicMarkup() = default;
    //! Marks a copy of `text` as safe
    explicit BasicMarkup(const string_type& text)
        : string_type(text)
    {
    }
    //! Marks `text` as safe
    explicit BasicMarkup(string_type&& text) noexcept
        : string_type(std::move(text))
    {
    }
    //! Marks a copy of `text` as safe
    explicit BasicMarkup(std::basic_string_view<CharT> text)
        : string_type(text)
    {
    }
    //! Marks a copy of the null-terminated `text` as safe
    explicit BasicMarkup(const CharT* text)
        : string_type(text)
    {
    }
};

//! Safe narrow text, see \ref BasicMarkup
using Markup = BasicMarkup<char>;
//! Safe wide text, see \ref BasicMarkup
using WMarkup = BasicMarkup<wchar_t>;
} // namespace jinja2

#endif // JINJA2CPP_MARKUP_H
