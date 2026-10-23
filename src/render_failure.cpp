#include "render_failure.h"

#include "error_boundary.h"
#include "node_arena.h"

#include <jinja2cpp/error_info.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iterator>
#include <locale>
#include <new>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>

namespace jinja2
{
inline namespace v2
{
namespace detail
{
// A line longer than maxShownLen is cut to a window around the column, so the message stays bounded
template<typename CharT>
std::basic_string<CharT> DescribeLocation(std::basic_string_view<CharT> origLine, std::size_t col)
{
    static constexpr std::size_t maxShownLen = 160;
    static constexpr std::size_t windowHead = 40;
    static constexpr std::size_t windowLen = 120;
    static constexpr std::size_t headLen = 3;
    static constexpr std::size_t tailLen = 7;
    const CharT ellipsis[] = { static_cast<CharT>('.'), static_cast<CharT>('.'), static_cast<CharT>('.') };

    std::basic_string<CharT> result;
    std::size_t caretCol = col;
    if (origLine.size() > maxShownLen)
    {
        const std::size_t start = col > windowHead ? std::min<std::size_t>(col - windowHead, origLine.size() - windowLen) : 0;
        const std::size_t end = start + windowLen;
        result.reserve(windowLen + (2 * std::size(ellipsis)) + 1 + windowHead + headLen + 1 + tailLen + std::size(ellipsis));
        if (start != 0)
        {
            result.append(ellipsis, std::size(ellipsis));
        }
        caretCol = col - start + result.size();
        result.append(origLine.data() + start, windowLen);
        if (end != origLine.size())
        {
            result.append(ellipsis, std::size(ellipsis));
        }
    }
    else
    {
        result.reserve((origLine.size() * 2) + headLen + 1 + tailLen + 1);
        result.append(origLine.data(), origLine.size());
    }
    const std::size_t shownLen = result.size();
    result.append(1, static_cast<CharT>('\n'));

    // Leading whitespace is copied as is so that tabs keep the marker aligned
    auto locale = std::locale();
    std::size_t spacePrefixLen = 0;
    while (spacePrefixLen < shownLen && std::isspace(result[spacePrefixLen], locale))
    {
        ++spacePrefixLen;
    }

    if (caretCol < spacePrefixLen)
    {
        result.append(caretCol, static_cast<CharT>(' '));
    }
    else
    {
        const std::basic_string<CharT> spacePrefix = result.substr(0, spacePrefixLen);
        result.append(spacePrefix);
        const std::size_t actualHeadLen = std::min(caretCol - spacePrefixLen, headLen);
        result.append(caretCol - actualHeadLen - spacePrefixLen, static_cast<CharT>(' '));
        result.append(actualHeadLen, static_cast<CharT>('-'));
    }
    result.append(1, static_cast<CharT>('^'));
    result.append(tailLen, static_cast<CharT>('-'));

    return result;
}

template std::string DescribeLocation<char>(std::string_view line, std::size_t col);
template std::wstring DescribeLocation<wchar_t>(std::wstring_view line, std::size_t col);

template<typename CharT>
BasicErrorInfo<CharT> RenderFailure::ErrorAs() const
{
    return std::visit(
        [](const auto& error) {
            using Src = std::decay_t<decltype(error)>;
            if constexpr (std::is_same_v<Src, BasicErrorInfo<CharT>>)
            {
                return error;
            }
            else
            {
                return BasicErrorInfo<CharT>(CopyErrorData<CharT>(error));
            }
        },
        m_error);
}

template ErrorInfo RenderFailure::ErrorAs<char>() const;
template ErrorInfoW RenderFailure::ErrorAs<wchar_t>() const;

namespace
{
// The RenderFailure of the exception being handled, at `offset` in the source of `origin`, a
// template of CharT; at the start of no file without an origin
template<typename CharT>
RenderFailure Locate(const TemplateOrigin* origin, std::uint32_t offset)
{
    bool located = false;
    auto errorData = CurrentExceptionToData<CharT>(located);
    if (!located)
    {
        // A thrown error that names a template of its own (one too long to load) keeps the name
        if (!errorData.srcLoc.fileName.empty() && (!origin || !origin->name || errorData.srcLoc.fileName != *origin->name))
        {
            errorData.relatedLocs.push_back(errorData.srcLoc);
        }
        errorData.srcLoc.fileName = origin && origin->name ? *origin->name : std::string();
        errorData.srcLoc.line = 1;
        errorData.srcLoc.col = 1;
        if (origin && origin->source)
        {
            // Newlines are normalized to '\n' when the template loads
            const std::basic_string_view<CharT> text(*static_cast<const std::basic_string<CharT>*>(origin->source));
            const std::size_t at = std::min<std::size_t>(offset, text.size());
            const auto newline = static_cast<CharT>('\n');
            const auto lastNewline = at == 0 ? std::basic_string_view<CharT>::npos : text.rfind(newline, at - 1);
            const std::size_t lineStart = lastNewline == std::basic_string_view<CharT>::npos ? 0 : lastNewline + 1;
            const std::size_t lineEnd = std::min(text.find(newline, at), text.size());
            errorData.srcLoc.line = static_cast<unsigned>(std::count(text.begin(), text.begin() + static_cast<std::ptrdiff_t>(lineStart), newline) + 1);
            errorData.srcLoc.col = static_cast<unsigned>(at - lineStart + 1);
            errorData.locationDescr = DescribeLocation(text.substr(lineStart, lineEnd - lineStart), at - lineStart);
        }
    }
    return RenderFailure(BasicErrorInfo<CharT>(std::move(errorData)));
}
} // namespace

void ThrowRenderFailure(std::exception_ptr failure, const ArenaView& nodes, const void* child, std::uint32_t offset)
{
    std::optional<RenderFailure> located;
    try
    {
        std::rethrow_exception(failure);
    }
    catch (const std::bad_alloc&) // NOLINT(bugprone-empty-catch): goes on unchanged below
    {
    }
    catch (const RenderFailure&) // NOLINT(bugprone-empty-catch): located by a body inside the statement
    {
    }
    catch (...)
    {
        // A body runs in the tree it is in, which the render context names: should it ever not,
        // the error gets no location rather than one in another template
        const TemplateOrigin* origin = nodes.Holds(child) ? nodes.Origin() : nullptr;
        located.emplace(origin && origin->isWide ? Locate<wchar_t>(origin, offset) : Locate<char>(origin, offset));
    }
    // Out of the handlers
    if (!located)
    {
        std::rethrow_exception(std::move(failure));
    }
    throw std::move(*located);
}
} // namespace detail
} // namespace v2
} // namespace jinja2
