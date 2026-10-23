#ifndef JINJA2CPP_SRC_TEMPLATE_PATH_H
#define JINJA2CPP_SRC_TEMPLATE_PATH_H

#include <optional>
#include <string_view>
#include <vector>

namespace jinja2
{
inline namespace v2
{

// Python's FileSystemLoader name rules (split_template_path): the name is split at '/', a ".." piece
// makes it not found, and empty and "." pieces are dropped, so a leading '/' still names a file under
// the root. On Windows '\\' is a separator too, and a piece holding it (or a drive ':') is not found,
// as Python rejects os.sep and os.altsep pieces. Returns the remaining pieces (views into name), or
// nothing when the name is not allowed. The list may be empty (the name is the root itself)
inline std::optional<std::vector<std::string_view>> SplitTemplateName(std::string_view name)
{
#ifdef _WIN32
    constexpr std::string_view rejectedChars{ "\\:\0", 3 };
#else
    constexpr std::string_view rejectedChars{ "\0", 1 };
#endif
    if (name.find_first_of(rejectedChars) != std::string_view::npos)
    {
        return std::nullopt;
    }

    std::vector<std::string_view> pieces;
    std::string_view rest = name;
    while (true)
    {
        const auto end = rest.find('/');
        const auto piece = rest.substr(0, end);
        if (piece == "..")
        {
            return std::nullopt;
        }
        if (!piece.empty() && piece != ".")
        {
            pieces.push_back(piece);
        }
        if (end == std::string_view::npos)
        {
            break;
        }
        rest.remove_prefix(end + 1);
    }
    return pieces;
}

} // namespace v2
} // namespace jinja2

#endif // JINJA2CPP_SRC_TEMPLATE_PATH_H
