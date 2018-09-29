#include <jinja2cpp/filesystem_handler.h>

#include <boost/filesystem/path.hpp>

#include <sstream>
#include <fstream>

namespace jinja2
{

using TargetFileStream = nonstd::variant<CharFileStreamPtr*, WCharFileStreamPtr*>;

struct FileContentConverter
{
    void operator() (const std::string& content, CharFileStreamPtr* sPtr) const
    {
        sPtr->reset(new std::istringstream(content));
    }

    void operator() (const std::wstring& content, WCharFileStreamPtr* sPtr) const
    {
        sPtr->reset(new std::wistringstream(content));
    }
    void operator() (const std::wstring& content, CharFileStreamPtr* sPtr) const
    {
//        CharFileStreamPtr stream(new std::istringstream(content), [](std::istream* s) {delete static_cast<std::istringstream>(s);});
//        std::swap(*sPtr, stream);
    }

    void operator() (const std::string& content, WCharFileStreamPtr* sPtr) const
    {
//        WCharFileStreamPtr stream(new std::wistringstream(content), [](std::wistream* s) {delete static_cast<std::wistringstream>(s);});
//        std::swap(*sPtr, stream);
    }
};

void MemoryFileSystem::AddFile(std::string fileName, std::string fileContent)
{
    m_filesMap[std::move(fileName)] = FileContent(std::move(fileContent));
}

void MemoryFileSystem::AddFile(std::string fileName, std::wstring fileContent)
{
    m_filesMap[std::move(fileName)] = FileContent(std::move(fileContent));
}

CharFileStreamPtr MemoryFileSystem::OpenStream(const std::string& name) const
{
    CharFileStreamPtr result(nullptr, [](std::istream* s) {delete static_cast<std::istringstream*>(s);});
    auto p = m_filesMap.find(name);
    if (p == m_filesMap.end())
        return result;

    TargetFileStream targetStream(&result);
    visit(FileContentConverter(), p->second, targetStream);

    return result;
}

WCharFileStreamPtr MemoryFileSystem::OpenWStream(const std::string& name) const
{
    WCharFileStreamPtr result(nullptr, [](std::wistream* s) {delete static_cast<std::wistringstream*>(s);});
    auto p = m_filesMap.find(name);
    if (p == m_filesMap.end())
        return result;

    TargetFileStream targetStream(&result);
    visit(FileContentConverter(), p->second, targetStream);

    return result;
}

RealFileSystem::RealFileSystem(std::string rootFolder)
    : m_rootFolder(rootFolder)
{

}

CharFileStreamPtr RealFileSystem::OpenStream(const std::string& name) const
{
    boost::filesystem::path root(m_rootFolder);
    root /= name;
    const auto& filePath = root.string();

    CharFileStreamPtr result(new std::ifstream(filePath), [](std::istream* s) {delete static_cast<std::ifstream*>(s);});
    if (result->good())
        return result;

    return CharFileStreamPtr(nullptr, [](std::istream* s){});
}

WCharFileStreamPtr RealFileSystem::OpenWStream(const std::string& name) const
{
    boost::filesystem::path root(m_rootFolder);
    root /= name;
    const auto& filePath = root.string();

    WCharFileStreamPtr result(new std::wifstream(filePath), [](std::wistream* s) {delete static_cast<std::wifstream*>(s);});
    if (result->good())
        return result;

    return WCharFileStreamPtr(nullptr, [](std::wistream* s){;});
}

} // jinja2
