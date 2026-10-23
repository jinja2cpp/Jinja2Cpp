#ifndef JINJA2CPP_SRC_RENDER_FAILURE_H
#define JINJA2CPP_SRC_RENDER_FAILURE_H

#include <jinja2cpp/error_info.h>

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

namespace jinja2
{
inline namespace v2
{
class ArenaView;

namespace detail
{
// The template a sealed tree was parsed from (SealedArena::SetOrigin), which the location of a
// render error is found in: its source after newline normalization, and its name
struct TemplateOrigin
{
    // A std::string or a std::wstring, as `isWide` says
    const void* source = nullptr;
    const std::string* name = nullptr;
    bool isWide = false;
};

// A render error that has its location in the template (docs/api-2.0.md, 2.1 rule 8): thrown
// out of the body of the statement that failed and passed on unchanged by the bodies around it,
// so that the innermost location wins. Not a std::exception, so that no catch of the engine's
// takes it for an error of its own
class RenderFailure
{
public:
    explicit RenderFailure(ErrorInfo error) noexcept
        : m_error(std::move(error))
    {
    }
    explicit RenderFailure(ErrorInfoW error) noexcept
        : m_error(std::move(error))
    {
    }

    // The error as a template of CharT reports it
    template<typename CharT>
    [[nodiscard]] BasicErrorInfo<CharT> ErrorAs() const;

private:
    std::variant<ErrorInfo, ErrorInfoW> m_error;
};

// Throws the RenderFailure of the exception being handled, which a statement of a body of
// `nodes` threw: the statement's tag starts at `offset` in its template's source. `body` is the
// body, which tells whether `nodes` is still the tree it is in. Call it from a catch handler only;
// std::bad_alloc goes on unchanged
[[noreturn]] void ThrowRenderFailure(const ArenaView& nodes, const void* body, std::uint32_t offset);

// The line of a source with a marker under the 0-based column `col`, cut to a window around the
// column when the line is long (a minified template)
template<typename CharT>
std::basic_string<CharT> DescribeLocation(std::basic_string_view<CharT> line, std::size_t col);

extern template std::string DescribeLocation<char>(std::string_view line, std::size_t col);
extern template std::wstring DescribeLocation<wchar_t>(std::wstring_view line, std::size_t col);
extern template ErrorInfo RenderFailure::ErrorAs<char>() const;
extern template ErrorInfoW RenderFailure::ErrorAs<wchar_t>() const;
} // namespace detail
} // namespace v2
} // namespace jinja2

#endif // JINJA2CPP_SRC_RENDER_FAILURE_H
