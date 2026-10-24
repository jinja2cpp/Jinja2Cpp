#include <jinja2cpp/filesystem_handler.h>

#include <jinja2cpp/string_helpers.h>
#include <jinja2cpp/utils/i_comparable.h>

#include "template_path.h"

#include <chrono>
#include <filesystem>
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
inline namespace v2
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
// The pieces are appended to the root as text: path operator/ lets an absolute or drive-qualified
// piece replace the root
std::optional<std::string> JoinTemplatePath(const std::string& root, const std::string& name)
{
    const auto pieces = SplitTemplateName(name);
    if (!pieces)
    {
        return {};
    }

    std::string result = root;
    for (const auto piece : *pieces)
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
    std::error_code ec;
    if (!std::filesystem::is_regular_file(std::filesystem::path(*filePath), ec))
    {
        return {};
    }
    return filePath;
}
// C++17 has no conversion from file_time_type, and its clock's epoch differs between standard
// libraries (1970 in libc++, 2174 in libstdc++, 1601 in MSVC). The offset between the epochs is a
// whole number of seconds, so the difference of the two clocks' current times, rounded to seconds,
// is exact. The result has whole seconds, as Boost's time_t and ConfinedFileSystem give.
std::chrono::system_clock::time_point ToSystemTime(std::filesystem::file_time_type fileTime)
{
    using std::chrono::seconds;
    using FileClock = std::filesystem::file_time_type::clock;
    const auto fileNow = FileClock::now().time_since_epoch();
    const auto systemNow = std::chrono::system_clock::now().time_since_epoch();
    const auto epochOffset = std::chrono::round<seconds>(fileNow - systemNow);
    const auto sinceUnixEpoch = std::chrono::floor<seconds>(fileTime.time_since_epoch()) - epochOffset;
    return std::chrono::system_clock::time_point(std::chrono::duration_cast<std::chrono::system_clock::duration>(sinceUnixEpoch));
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

    std::error_code ec;
    const auto modifyTime = std::filesystem::last_write_time(std::filesystem::path(*filePath), ec);
    if (ec)
    {
        return {};
    }

    return ToSystemTime(modifyTime);
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

} // namespace v2
} // namespace jinja2
