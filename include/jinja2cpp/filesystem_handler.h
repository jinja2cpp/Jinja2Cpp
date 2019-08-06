#ifndef JINJA2CPP_FILESYSTEM_HANDLER_H
#define JINJA2CPP_FILESYSTEM_HANDLER_H

#include <nonstd/variant.hpp>
#include <nonstd/optional.hpp>

#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>

namespace jinja2
{

template<typename CharT>
using FileStreamPtr = std::unique_ptr<std::basic_istream<CharT>, void (*)(std::basic_istream<CharT>*)>;
using CharFileStreamPtr = FileStreamPtr<char>;
using WCharFileStreamPtr = FileStreamPtr<wchar_t>;

class IFilesystemHandler
{
public:
    virtual ~IFilesystemHandler() = default;

    virtual CharFileStreamPtr OpenStream(const std::string& name) const = 0;
    virtual WCharFileStreamPtr OpenWStream(const std::string& name) const = 0;
};

using FilesystemHandlerPtr = std::shared_ptr<IFilesystemHandler>;

class MemoryFileSystem : public IFilesystemHandler
{
public:
    void AddFile(std::string fileName, std::string fileContent);
    void AddFile(std::string fileName, std::wstring fileContent);

    CharFileStreamPtr OpenStream(const std::string& name) const override;
    WCharFileStreamPtr OpenWStream(const std::string& name) const override;

private:
    // using FileContent = nonstd::variant<std::string, std::wstring>;
    struct FileContent
    {
        nonstd::optional<std::string> narrowContent;
        nonstd::optional<std::wstring> wideContent;
    };
    mutable std::unordered_map<std::string, FileContent> m_filesMap;
};

class RealFileSystem : public IFilesystemHandler
{
public:
    RealFileSystem(std::string rootFolder = ".");

    void SetRootFolder(std::string newRoot)
    {
        m_rootFolder = std::move(newRoot);
    }

    CharFileStreamPtr OpenStream(const std::string& name) const override;
    WCharFileStreamPtr OpenWStream(const std::string& name) const override;

private:
    std::string m_rootFolder;
};
} // jinja2

#endif // FILESYSTEM_HANDLER_H
