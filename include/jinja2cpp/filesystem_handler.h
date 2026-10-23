#ifndef JINJA2CPP_FILESYSTEM_HANDLER_H
#define JINJA2CPP_FILESYSTEM_HANDLER_H

#include "fwd.h" // IWYU pragma: export
#include "config.h"

#include <jinja2cpp/utils/i_comparable.h>

#include <chrono>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant> // IWYU pragma: keep (public header: user code may rely on it)

JINJA2CPP_SUPPRESS_DLL_INTERFACE_WARNINGS_BEGIN

namespace jinja2
{
inline namespace v2
{

template<typename CharT>
using FileStreamPtr = std::unique_ptr<std::basic_istream<CharT>, void (*)(std::basic_istream<CharT>*)>;
using CharFileStreamPtr = FileStreamPtr<char>;
using WCharFileStreamPtr = FileStreamPtr<wchar_t>;

/*!
 * \brief Generic interface to filesystem handlers (loaders)
 *
 * This interface should be implemented in order to provide custom file system handler. Interface provides most-common methods which are called by
 * the template environment to load the particular template. `OpenStream` methods return the unique pointer to the generic `istream` object implementation.
 * So, the exact type (ex. `ifstream`, `istringstream` etc.) of input stream is unspecified. In order to delete stream object correctly returned pointer
 * provide the custom deleter which should properly delete the stream object.
 *
 * `IsEqual` (from \ref IComparable) defaults to identity; override it only if two handler objects can serve the same files.
 */
class JINJA2CPP_EXPORT IFilesystemHandler : public virtual IComparable
{
public:
    //! Destructor
    ~IFilesystemHandler() override = default;

    /*!
     * \brief Method is called to open the file with the specified name in 'narrow-char' mode.
     *
     * Method should return unique pointer to the std::istream object with custom deleter (\ref CharFileStreamPtr) . Deleter should properly delete pointee
     * stream object.
     *
     * @param name Name of the file to open
     * @return Opened stream object or empty pointer in case of any error
     */
    [[nodiscard]] virtual CharFileStreamPtr OpenStream(const std::string& name) const = 0;
    /*!
     * \brief Method is called to open the file with the specified name in 'wide-char' mode.
     *
     * Method should return unique pointer to the std::wistream object with custom deleter (\ref WCharFileStreamPtr) . Deleter should properly delete pointee
     * stream object.
     *
     * @param name Name of the file to open
     * @return Opened stream object or empty pointer in case of any error
     */
    [[nodiscard]] virtual WCharFileStreamPtr OpenWStream(const std::string& name) const = 0;
    /*!
     * \brief Method is called to obtain the modification date of the specified file (if applicable)
     *
     * If the underlaying filesystem supports retrival of the last modification date of the file this method should return this date when called. In other
     * case it should return the empty optional object. Main purpose of this method is to help templates loader to determine the necessity of cached template
     * reload
     *
     * @param name Name of the file to get the last modification date
     * @return Last modification date (if applicable) or empty optional object otherwise
     */
    [[nodiscard]] virtual std::optional<std::chrono::system_clock::time_point> GetLastModificationDate(const std::string& name) const = 0;
};

using FilesystemHandlerPtr = std::shared_ptr<IFilesystemHandler>;

/*!
 * \brief Filesystem handler for files stored in memory
 *
 * This filesystem handler implements the simple dictionary object which maps name of the file to it's content. New files can be added by \ref AddFile
 * methods. Content of the files automatically converted to narrow/wide strings representation if necessary.
 */
class JINJA2CPP_EXPORT MemoryFileSystem : public IFilesystemHandler
{
public:
    /*!
     * \brief Add new narrow-char "file" to the filesystem handler
     *
     * Adds new file entry to the internal dictionary object or overwrite the existing one. New entry contains the specified content of the file
     *
     * @param fileName Name of the file to add
     * @param fileContent Content of the file to add
     */
    void AddFile(std::string fileName, std::string fileContent);
    /*!
     * \brief Add new wide-char "file" to the filesystem handler
     *
     * Adds new file entry to the internal dictionary object or overwrite the existing one. New entry contains the specified content of the file
     *
     * @param fileName Name of the file to add
     * @param fileContent Content of the file to add
     */
    void AddFile(std::string fileName, std::wstring fileContent);

    CharFileStreamPtr OpenStream(const std::string& name) const override;
    WCharFileStreamPtr OpenWStream(const std::string& name) const override;
    std::optional<std::chrono::system_clock::time_point> GetLastModificationDate(const std::string& name) const override;

    /*!
     * \brief Compares to an object of the same type
     *
     * return true if equal
     */
    bool IsEqual(const IComparable& other) const override;
private:
    struct FileContent
    {
        std::optional<std::string> narrowContent;
        std::optional<std::wstring> wideContent;
        bool operator==(const FileContent& other) const
        {
            if (narrowContent != other.narrowContent)
            {
                return false;
            }
            return wideContent == other.wideContent;
        }
        bool operator!=(const FileContent& other) const
        {
            return !(*this == other);
        }
    };
    mutable std::unordered_map<std::string, FileContent> m_filesMap;
};

/*!
 * \brief Filesystem handler for files stored on the filesystem
 *
 * This filesystem handler is an interface to the real file system. Root directory for file name mapping provided as a constructor argument. Each name (path) of
 * the file to open is appended to the root directory path and then passed to the stream open methods.
 *
 * Names follow the rules of Python Jinja2's `FileSystemLoader`: a name is split at `/` (on Windows, `\` is a separator too), and a name with a `..` piece is
 * not found. Empty and `.` pieces are skipped, so `/a.j2` and `./a.j2` both name `a.j2` under the root. On Windows a piece holding `\` or `:` is not found.
 * Only regular files are found; symbolic links are followed, as in Python.
 */
class JINJA2CPP_EXPORT RealFileSystem : public IFilesystemHandler
{
public:
    /*!
     * \brief Initializing/default constructor
     *
     * @param rootFolder Path to the root folder. This path is used as a root for every opened file
     */
    explicit RealFileSystem(std::string rootFolder = ".");

    /*!
     * \brief Reset path to the root folder to the new value
     *
     * @param newRoot New path to the root folder
     */
    void SetRootFolder(std::string newRoot)
    {
        m_rootFolder = std::move(newRoot);
    }

    /*!
     * \brief Get path to the current root folder
     *
     * @return
     */
    [[nodiscard]] const std::string& GetRootFolder() const
    {
        return m_rootFolder;
    }
    /*!
     * \brief Get full path to the specified file.
     *
     * Appends specified name of the file to the current root folder and returns it. The name follows the rules described for the class
     *
     * @param name Name of the file to get path to
     * @return Full path to the file, or an empty string when the name is not allowed (for example, it has a `..` piece)
     */
    [[nodiscard]] std::string GetFullFilePath(const std::string& name) const;

    [[nodiscard]] CharFileStreamPtr OpenStream(const std::string& name) const override;
    [[nodiscard]] WCharFileStreamPtr OpenWStream(const std::string& name) const override;
    /*!
     * \brief Open the specified file as a binary stream
     *
     * Opens the specified file in the binary mode (instead of text).
     *
     * @param name Name of the file to open
     * @return Opened stream object or empty pointer in case of any error
     */
    [[nodiscard]] CharFileStreamPtr OpenByteStream(const std::string& name) const;
    [[nodiscard]] std::optional<std::chrono::system_clock::time_point> GetLastModificationDate(const std::string& name) const override;

    /*!
     * \brief Compares to an object of the same type
     *
     * return true if equal
     */
    [[nodiscard]] bool IsEqual(const IComparable& other) const override;

private:
    std::string m_rootFolder;
};

/*!
 * \brief Filesystem handler that never opens a file outside its root folder
 *
 * Use it when the template root may hold symbolic links (or, on Windows, junctions) that the host does not control. \ref RealFileSystem follows links
 * wherever they point, as Python's `FileSystemLoader` does; this handler opens a file only if it really lies under the root, and the check applies to the
 * file that is actually read, so a link swapped while a template loads cannot reach outside.
 *
 * Names follow the same rules as \ref RealFileSystem (those of Python's `FileSystemLoader`): a name is split at `/` (on Windows, `\` too), and a name with a
 * `..` piece is not found. Empty and `.` pieces are skipped. On Windows a piece holding `\` or `:` is not found. In addition:
 * - The root path itself is trusted: links in it are followed.
 * - Below the root, a symbolic link is followed only while it resolves inside the root. A link with an absolute target, or one whose `..` pieces climb above
 *   the root, is not found.
 * - Only regular files are opened; FIFOs, devices and directories are not found.
 * - Hard links and mount points under the root are inside the root by definition.
 *
 * Every failure (a refused name, a missing file, an outside link) gives an empty stream pointer or an empty optional; no message carries the host path.
 * There is no `GetFullFilePath`: a path to open again by name is exactly the race this class avoids. Each call opens the root afresh,
 * so the class keeps no descriptors and may be used from many threads.
 */
class JINJA2CPP_EXPORT ConfinedFileSystem : public IFilesystemHandler
{
public:
    /*!
     * \brief Initializing/default constructor
     *
     * @param rootFolder Path to the root folder. Every opened file lies under it
     */
    explicit ConfinedFileSystem(std::string rootFolder = ".");

    /*!
     * \brief Reset path to the root folder to the new value
     *
     * @param newRoot New path to the root folder
     */
    void SetRootFolder(std::string newRoot)
    {
        m_rootFolder = std::move(newRoot);
    }

    /*!
     * \brief Get path to the current root folder
     */
    [[nodiscard]] const std::string& GetRootFolder() const
    {
        return m_rootFolder;
    }

    [[nodiscard]] CharFileStreamPtr OpenStream(const std::string& name) const override;
    [[nodiscard]] WCharFileStreamPtr OpenWStream(const std::string& name) const override;
    /*!
     * \brief Open the specified file as a binary stream
     *
     * Reads the same bytes as \ref OpenStream (no newline translation on any platform).
     *
     * @param name Name of the file to open
     * @return Opened stream object or empty pointer in case of any error
     */
    [[nodiscard]] CharFileStreamPtr OpenByteStream(const std::string& name) const;
    [[nodiscard]] std::optional<std::chrono::system_clock::time_point> GetLastModificationDate(const std::string& name) const override;

    /*!
     * \brief Compares to an object of the same type
     *
     * return true if equal (same root folder string)
     */
    [[nodiscard]] bool IsEqual(const IComparable& other) const override;

private:
    std::string m_rootFolder;
};
} // namespace v2
} // namespace jinja2

JINJA2CPP_SUPPRESS_DLL_INTERFACE_WARNINGS_END

#endif // JINJA2CPP_FILESYSTEM_HANDLER_H
