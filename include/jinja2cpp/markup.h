#ifndef JINJA2CPP_MARKUP_H
#define JINJA2CPP_MARKUP_H

#include <iosfwd>
#include <string>
#include <string_view>
#include <utility>

namespace jinja2
{
inline namespace v2
{
/*!
 * \brief Text that is already safe HTML: Python's `markupsafe.Markup`
 *
 * A \ref Value that holds a Markup renders unescaped under autoescape and passes the `is escaped` test, as a
 * `markupsafe.Markup` does in Python; a plain string is escaped. Every constructor is explicit: the text is
 * trusted not to need escaping, so making one is always a deliberate act.
 *
 * Like Python's `Markup`, which is an immutable `str`, this type is read-only: it reads as a string (\ref str,
 * the implicit conversions to `const std::basic_string&` and `std::basic_string_view`, `size`, `data`,
 * iterators, comparisons, `operator<<`) but has no member that changes the text, and it is not a
 * `std::basic_string`, so `markup += user_input` and `markup.append(user_input)` do not compile and a function
 * taking `std::string&` cannot be handed one. Python gives a plain `str` for any change; here, build the new text
 * and wrap it, which states that it is safe:
 *
 * ```c++
 *  params["banner"] = jinja2::Markup("<b>sale</b>"); // {{ banner }} prints <b>sale</b> even with autoescape on
 *  jinja2::Markup more(markup.str() + "<i>new</i>"); // deliberately marked safe again
 * ```
 *
 * A \ref Value also holds a Markup when a template result or a user callable's return value is Markup in the
 * template (for instance after the `safe` filter), and \ref Value::IsString, \ref Value::AsString and
 * `Value::As<std::string>()` read it as the string it is; \ref Value::IsMarkup tells the two apart. Mutable access
 * to the text of such a Value turns it into a plain string first, see \ref Value.
 */
template<typename CharT>
class BasicMarkup
{
public:
    //! The text type
    using string_type = std::basic_string<CharT>;
    //! The text view type
    using view_type = std::basic_string_view<CharT>;
    using value_type = CharT;
    using size_type = typename string_type::size_type;
    using const_iterator = typename string_type::const_iterator;
    using iterator = const_iterator;

    //! Empty markup
    BasicMarkup() = default;
    //! Marks a copy of `text` as safe
    explicit BasicMarkup(const string_type& text)
        : m_text(text)
    {
    }
    //! Marks `text` as safe
    explicit BasicMarkup(string_type&& text) noexcept
        : m_text(std::move(text))
    {
    }
    //! Marks a copy of `text` as safe
    explicit BasicMarkup(view_type text)
        : m_text(text)
    {
    }
    //! Marks a copy of the null-terminated `text` as safe
    explicit BasicMarkup(const CharT* text)
        : m_text(text)
    {
    }

    // NOLINTBEGIN(readability-identifier-naming, google-explicit-constructor, hicpp-explicit-conversions)
    //! The text (named as Python's `str`)
    [[nodiscard]] const string_type& str() const& noexcept { return m_text; }
    //! Takes the text out, leaving the Markup empty; the result is a plain string
    [[nodiscard]] string_type str() && noexcept { return std::move(m_text); }
    //! The text, read-only; implicit so that a Markup reads wherever a `const std::string&` is taken
    operator const string_type&() const noexcept { return m_text; }
    //! A view of the text
    operator view_type() const noexcept { return view_type(m_text); }

    [[nodiscard]] size_type size() const noexcept { return m_text.size(); }
    [[nodiscard]] size_type length() const noexcept { return m_text.length(); }
    [[nodiscard]] bool empty() const noexcept { return m_text.empty(); }
    [[nodiscard]] const CharT* data() const noexcept { return m_text.data(); }
    [[nodiscard]] const CharT* c_str() const noexcept { return m_text.c_str(); }
    [[nodiscard]] const_iterator begin() const noexcept { return m_text.begin(); }
    [[nodiscard]] const_iterator end() const noexcept { return m_text.end(); }
    [[nodiscard]] const_iterator cbegin() const noexcept { return m_text.cbegin(); }
    [[nodiscard]] const_iterator cend() const noexcept { return m_text.cend(); }
    [[nodiscard]] const CharT& operator[](size_type pos) const noexcept { return m_text[pos]; }
    // NOLINTEND(readability-identifier-naming, google-explicit-constructor, hicpp-explicit-conversions)

    friend bool operator==(const BasicMarkup& lhs, const BasicMarkup& rhs) noexcept { return lhs.m_text == rhs.m_text; }
    friend bool operator==(const BasicMarkup& lhs, view_type rhs) noexcept { return view_type(lhs.m_text) == rhs; }
#ifndef __cpp_impl_three_way_comparison
    friend bool operator==(view_type lhs, const BasicMarkup& rhs) noexcept { return lhs == view_type(rhs.m_text); }
    friend bool operator!=(const BasicMarkup& lhs, const BasicMarkup& rhs) noexcept { return !(lhs == rhs); }
    friend bool operator!=(const BasicMarkup& lhs, view_type rhs) noexcept { return !(lhs == rhs); }
    friend bool operator!=(view_type lhs, const BasicMarkup& rhs) noexcept { return !(lhs == rhs); }
#endif
    friend bool operator<(const BasicMarkup& lhs, const BasicMarkup& rhs) noexcept { return lhs.m_text < rhs.m_text; }

    //! Writes the text
    template<typename Traits>
    friend std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& os, const BasicMarkup& markup)
    {
        return os << markup.m_text;
    }

private:
    string_type m_text;
};

//! Safe narrow text, see \ref BasicMarkup
using Markup = BasicMarkup<char>;
//! Safe wide text, see \ref BasicMarkup
using WMarkup = BasicMarkup<wchar_t>;
} // namespace v2
} // namespace jinja2

#endif // JINJA2CPP_MARKUP_H
