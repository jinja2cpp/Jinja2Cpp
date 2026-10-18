#include <jinja2cpp/filesystem_handler.h>

#include <jinja2cpp/string_helpers.h>
#include <jinja2cpp/utils/i_comparable.h>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include <chrono>
#include <fstream>
#include <ios>
#include <istream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

namespace jinja2
{

using TargetFileStream = std::variant<CharFileStreamPtr*, WCharFileStreamPtr*>;

struct FileContentConverter
{
    void operator()(const std::string& content, CharFileStreamPtr* sPtr) const
    {
        sPtr->reset(new std::istringstream(content));
    }

    void operator()(const std::wstring& content, WCharFileStreamPtr* sPtr) const
    {
        sPtr->reset(new std::wistringstream(content));
    }
    void operator()(const std::wstring&, CharFileStreamPtr*) const
    {
        //        CharFileStreamPtr stream(new std::istringstream(content), [](std::istream* s) {delete static_cast<std::istringstream>(s);});
        //        std::swap(*sPtr, stream);
    }

    void operator()(const std::string&, WCharFileStreamPtr*) const
    {
        //        WCharFileStreamPtr stream(new std::wistringstream(content), [](std::wistream* s) {delete static_cast<std::wistringstream>(s);});
        //        std::swap(*sPtr, stream);
    }
};

void MemoryFileSystem::AddFile(std::string fileName, std::string fileContent)
{
    m_filesMap[std::move(fileName)] = FileContent{ std::move(fileContent), {} };
}

void MemoryFileSystem::AddFile(std::string fileName, std::wstring fileContent)
{
    m_filesMap[std::move(fileName)] = FileContent{ {}, std::move(fileContent) };
}

CharFileStreamPtr MemoryFileSystem::OpenStream(const std::string& name) const
{
    CharFileStreamPtr result(nullptr, [](std::istream* s) { delete static_cast<std::istringstream*>(s); });
    auto p = m_filesMap.find(name);
    if (p == m_filesMap.end())
    {
        return result;
    }

    auto& content = p->second;

    if (!content.narrowContent && !content.wideContent)
    {
        return result;
    }

    if (!content.narrowContent)
    {
        content.narrowContent = ConvertString<std::string>(content.wideContent.value());
    }

    result.reset(new std::istringstream(content.narrowContent.value()));

    return result;
}

WCharFileStreamPtr MemoryFileSystem::OpenWStream(const std::string& name) const
{
    WCharFileStreamPtr result(nullptr, [](std::wistream* s) { delete static_cast<std::wistringstream*>(s); });
    auto p = m_filesMap.find(name);
    if (p == m_filesMap.end())
    {
        return result;
    }

    auto& content = p->second;

    if (!content.narrowContent && !content.wideContent)
    {
        return result;
    }

    if (!content.wideContent)
    {
        content.wideContent = ConvertString<std::wstring>(content.narrowContent.value());
    }

    result.reset(new std::wistringstream(content.wideContent.value()));

    return result;
}
std::optional<std::chrono::system_clock::time_point> MemoryFileSystem::GetLastModificationDate(const std::string&) const
{
    return std::optional<std::chrono::system_clock::time_point>();
}

bool MemoryFileSystem::IsEqual(const IComparable& other) const
{
    const auto* ptr = dynamic_cast<const MemoryFileSystem*>(&other);
    if (!ptr)
    {
        return false;
    }
    return m_filesMap == ptr->m_filesMap;
}

namespace
{
// Python's FileSystemLoader name rules (split_template_path): the name is split at '/', a ".." piece
// makes it not found, and empty and "." pieces are dropped, so a leading '/' still names a file under
// the root. On Windows '\\' is a separator too, and a piece holding it (or a drive ':') is not found,
// as Python rejects os.sep and os.altsep pieces. The pieces are appended to the root as text: path
// operator/ lets an absolute or drive-qualified piece replace the root
std::optional<std::string> JoinTemplatePath(const std::string& root, const std::string& name)
{
#ifdef _WIN32
    constexpr std::string_view rejectedChars{ "\\:\0", 3 };
#else
    constexpr std::string_view rejectedChars{ "\0", 1 };
#endif
    if (name.find_first_of(rejectedChars) != std::string::npos)
    {
        return {};
    }

    std::string result = root;
    std::string_view rest = name;
    while (true)
    {
        const auto end = rest.find('/');
        const auto piece = rest.substr(0, end);
        if (piece == "..")
        {
            return {};
        }
        if (!piece.empty() && piece != ".")
        {
#ifdef _WIN32
            const bool endsWithSeparator = !result.empty() && (result.back() == '/' || result.back() == '\\');
#else
            const bool endsWithSeparator = !result.empty() && result.back() == '/';
#endif
            if (!result.empty() && !endsWithSeparator)
            {
                result += '/';
            }
            result += piece;
        }
        if (end == std::string_view::npos)
        {
            break;
        }
        rest.remove_prefix(end + 1);
    }
    return result;
}

// Only regular files (symlinks followed) are found, as with Python's os.path.isfile. Never throws
std::optional<std::string> FindTemplateFile(const std::string& root, const std::string& name)
{
    auto filePath = JoinTemplatePath(root, name);
    if (!filePath)
    {
        return {};
    }
    boost::system::error_code ec;
    if (!boost::filesystem::is_regular_file(boost::filesystem::path(*filePath), ec))
    {
        return {};
    }
    return filePath;
}
} // namespace

RealFileSystem::RealFileSystem(std::string rootFolder)
    : m_rootFolder(std::move(rootFolder))
{
}

std::string RealFileSystem::GetFullFilePath(const std::string& name) const
{
    return JoinTemplatePath(m_rootFolder, name).value_or(std::string());
}

CharFileStreamPtr RealFileSystem::OpenStream(const std::string& name) const
{
    auto filePath = FindTemplateFile(m_rootFolder, name);
    if (!filePath)
    {
        return CharFileStreamPtr(nullptr, [](std::istream*) {});
    }

    CharFileStreamPtr result(new std::ifstream(*filePath), [](std::istream* s) { delete static_cast<std::ifstream*>(s); });
    if (result->good())
    {
        return result;
    }

    return CharFileStreamPtr(nullptr, [](std::istream*) {});
}

WCharFileStreamPtr RealFileSystem::OpenWStream(const std::string& name) const
{
    auto filePath = FindTemplateFile(m_rootFolder, name);
    if (!filePath)
    {
        return WCharFileStreamPtr(nullptr, [](std::wistream*) {});
    }

    WCharFileStreamPtr result(new std::wifstream(*filePath), [](std::wistream* s) { delete static_cast<std::wifstream*>(s); });
    if (result->good())
    {
        return result;
    }

    return WCharFileStreamPtr(nullptr, [](std::wistream*) {});
}
std::optional<std::chrono::system_clock::time_point> RealFileSystem::GetLastModificationDate(const std::string& name) const
{
    auto filePath = FindTemplateFile(m_rootFolder, name);
    if (!filePath)
    {
        return {};
    }

    boost::system::error_code ec;
    auto modifyTime = boost::filesystem::last_write_time(boost::filesystem::path(*filePath), ec);
    if (ec)
    {
        return {};
    }

    return std::chrono::system_clock::from_time_t(modifyTime);
}
CharFileStreamPtr RealFileSystem::OpenByteStream(const std::string& name) const
{
    auto filePath = FindTemplateFile(m_rootFolder, name);
    if (!filePath)
    {
        return CharFileStreamPtr(nullptr, [](std::istream*) {});
    }

    CharFileStreamPtr result(new std::ifstream(*filePath, std::ios_base::binary), [](std::istream* s) { delete static_cast<std::ifstream*>(s); });
    if (result->good())
    {
        return result;
    }

    return CharFileStreamPtr(nullptr, [](std::istream*) {});
}

bool RealFileSystem::IsEqual(const IComparable& other) const
{
    const auto* ptr = dynamic_cast<const RealFileSystem*>(&other);
    if (!ptr)
    {
        return false;
    }
    return m_rootFolder == ptr->m_rootFolder;
}

} // namespace jinja2
