#ifndef JINJA2CPP_SRC_STRING_BUILDER_H
#define JINJA2CPP_SRC_STRING_BUILDER_H

#include "internal_value.h"

#include <jinja2cpp/string_helpers.h>

#include <fmt/format.h>

#include <cstddef>
#include <string>
#include <string_view>
#include <type_traits>

namespace jinja2
{
inline namespace v2
{

// The text of a string value being built: the first 500 characters on the stack, so that the
// finished value allocates once, its object holding the characters (docs/tasks/0151). Code that
// builds a std::basic_string and moves it into a value allocates twice
template<typename CharT>
class StringBuilder
{
public:
    void Append(std::basic_string_view<CharT> text) { m_buffer.append(text.data(), text.data() + text.size()); }
    void Append(CharT ch) { m_buffer.push_back(ch); }
    // The std::basic_string names, for code written against one
    void push_back(CharT ch) { Append(ch); }                                                           // NOLINT(readability-identifier-naming)
    void append(const CharT* text, size_t size) { Append(std::basic_string_view<CharT>(text, size)); } // NOLINT(readability-identifier-naming)
    StringBuilder& operator+=(std::basic_string_view<CharT> text)
    {
        Append(text);
        return *this;
    }
    // Text of the other character type, converted
    template<typename OtherCharT>
    std::enable_if_t<!std::is_same_v<CharT, OtherCharT>> Append(std::basic_string_view<OtherCharT> text)
    {
        Append(std::basic_string_view<CharT>(ConvertString<std::basic_string<CharT>>(text)));
    }
    // The text of a string value of either character type; false for any other value
    bool AppendString(const InternalValue& val)
    {
        if (auto text = AsStringView<CharT>(val))
        {
            Append(*text);
            return true;
        }
        if (auto text = AsStringView<OtherChar>(val))
        {
            Append(*text);
            return true;
        }
        return false;
    }

    [[nodiscard]] std::basic_string_view<CharT> View() const noexcept { return { m_buffer.data(), m_buffer.size() }; }
    [[nodiscard]] InternalValue Build() const { return InternalValue::FromText(View()); }
    fmt::basic_memory_buffer<CharT>& Buffer() noexcept { return m_buffer; }

private:
    using OtherChar = std::conditional_t<std::is_same_v<CharT, char>, wchar_t, char>;

    fmt::basic_memory_buffer<CharT> m_buffer;
};

} // namespace v2
} // namespace jinja2

#endif // JINJA2CPP_SRC_STRING_BUILDER_H
