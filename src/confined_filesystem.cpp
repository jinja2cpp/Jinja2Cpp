#include <jinja2cpp/filesystem_handler.h>

#include <jinja2cpp/string_helpers.h>

#include "template_path.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <limits>
#else
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cerrno>
#include <deque>
#endif

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <ctime>
#include <istream>
#include <iterator>
#include <optional>
#include <streambuf>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace jinja2
{
inline namespace v2
{

namespace
{
#ifdef _WIN32
using NativeHandle = HANDLE;
inline HANDLE InvalidNativeHandle()
{
    return INVALID_HANDLE_VALUE;
}
#else
using NativeHandle = int;
constexpr int InvalidNativeHandle()
{
    return -1;
}
#endif

// An open file or directory (a descriptor or a handle) that is closed on destruction
class NativeFile
{
public:
    NativeFile() = default;
    explicit NativeFile(NativeHandle handle)
        : m_handle(handle)
    {
    }
    NativeFile(const NativeFile&) = delete;
    NativeFile& operator=(const NativeFile&) = delete;
    NativeFile(NativeFile&& other) noexcept
        : m_handle(std::exchange(other.m_handle, InvalidNativeHandle()))
    {
    }
    NativeFile& operator=(NativeFile&& other) noexcept
    {
        if (this != &other)
        {
            Close();
            m_handle = std::exchange(other.m_handle, InvalidNativeHandle());
        }
        return *this;
    }
    ~NativeFile()
    {
        Close();
    }

    [[nodiscard]] bool Valid() const
    {
        return m_handle != InvalidNativeHandle();
    }
    [[nodiscard]] NativeHandle Get() const
    {
        return m_handle;
    }

    // Bytes read, 0 at the end of the file, -1 on an error
    [[nodiscard]] std::ptrdiff_t Read(char* buffer, std::size_t size) const
    {
#ifdef _WIN32
        const DWORD toRead = static_cast<DWORD>(std::min<std::size_t>(size, 1U << 20));
        DWORD done = 0;
        if (!::ReadFile(m_handle, buffer, toRead, &done, nullptr))
        {
            return -1;
        }
        return static_cast<std::ptrdiff_t>(done);
#else
        while (true)
        {
            const auto done = ::read(m_handle, buffer, size);
            if (done < 0 && errno == EINTR)
            {
                continue;
            }
            return static_cast<std::ptrdiff_t>(done);
        }
#endif
    }

    [[nodiscard]] std::optional<std::chrono::system_clock::time_point> LastWriteTime() const
    {
#ifdef _WIN32
        FILETIME ft{};
        if (!::GetFileTime(m_handle, nullptr, nullptr, &ft))
        {
            return {};
        }
        ULARGE_INTEGER ticks{};
        ticks.LowPart = ft.dwLowDateTime;
        ticks.HighPart = ft.dwHighDateTime;
        constexpr unsigned long long unixEpochTicks = 116444736000000000ULL; // 1601-01-01 to 1970-01-01, 100 ns units
        if (ticks.QuadPart < unixEpochTicks)
        {
            return {};
        }
        const auto seconds = (ticks.QuadPart - unixEpochTicks) / 10000000ULL;
        return std::chrono::system_clock::from_time_t(static_cast<std::time_t>(seconds));
#else
        struct stat st
        {
        };
        if (::fstat(m_handle, &st) != 0)
        {
            return {};
        }
        return std::chrono::system_clock::from_time_t(st.st_mtime);
#endif
    }

private:
    void Close()
    {
        if (!Valid())
        {
            return;
        }
#ifdef _WIN32
        ::CloseHandle(m_handle);
#else
        ::close(m_handle);
#endif
        m_handle = InvalidNativeHandle();
    }

    NativeHandle m_handle = InvalidNativeHandle();
};

#ifdef _WIN32

std::wstring Widen(std::string_view text)
{
    if (text.empty() || text.size() > static_cast<std::size_t>(std::numeric_limits<int>::max()))
    {
        return {};
    }
    const int length = static_cast<int>(text.size());
    const int count = ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), length, nullptr, 0);
    if (count <= 0)
    {
        return {};
    }
    std::wstring result(static_cast<std::size_t>(count), L'\0');
    if (::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), length, result.data(), count) != count)
    {
        return {};
    }
    return result;
}

// The final path of an open handle (long names, no 8.3 aliases, links resolved), or empty
std::wstring FinalPath(HANDLE handle)
{
    std::vector<wchar_t> buffer(512);
    while (true)
    {
        const DWORD needed = ::GetFinalPathNameByHandleW(handle, buffer.data(), static_cast<DWORD>(buffer.size()), FILE_NAME_NORMALIZED | VOLUME_NAME_DOS);
        if (needed == 0)
        {
            return {};
        }
        if (needed < buffer.size())
        {
            return std::wstring(buffer.data(), needed);
        }
        buffer.resize(static_cast<std::size_t>(needed) + 1);
    }
}

bool IsUnderRoot(const std::wstring& rootFinal, const std::wstring& fileFinal)
{
    std::wstring prefix = rootFinal;
    if (prefix.empty() || prefix.back() != L'\\')
    {
        prefix += L'\\';
    }
    if (fileFinal.size() <= prefix.size() || prefix.size() > static_cast<std::size_t>(std::numeric_limits<int>::max()))
    {
        return false;
    }
    // Exact, not case-folded: a normalized final path spells every component as it is stored, so the root's
    // directories compare equal to themselves, while a sibling that differs only in case (a case-sensitive
    // directory may hold `Root` and `root`) does not
    const int length = static_cast<int>(prefix.size());
    return ::CompareStringOrdinal(fileFinal.data(), length, prefix.data(), length, FALSE) == CSTR_EQUAL;
}

// The root is opened and resolved on every call (it is trusted, links in it are followed). The file is opened
// normally, and the final path of the opened handle must lie under the root's final path: the check is made on
// the object that is read, so a link swapped in between cannot lead outside
NativeFile OpenConfinedFile(const std::string& root, const std::vector<std::string_view>& pieces)
{
    if (pieces.empty())
    {
        return {};
    }
    const std::wstring rootPath = root.empty() ? std::wstring(L".") : Widen(root);
    if (rootPath.empty())
    {
        return {};
    }
    constexpr DWORD shareAll = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
    std::wstring rootFinal;
    {
        NativeFile rootDir(::CreateFileW(rootPath.c_str(), FILE_READ_ATTRIBUTES, shareAll, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr));
        if (!rootDir.Valid())
        {
            return {};
        }
        rootFinal = FinalPath(rootDir.Get());
    }
    if (rootFinal.empty())
    {
        return {};
    }

    std::wstring path = rootFinal;
    for (const auto piece : pieces)
    {
        const std::wstring wide = Widen(piece);
        if (wide.empty())
        {
            return {};
        }
        if (path.back() != L'\\')
        {
            path += L'\\';
        }
        path += wide;
    }

    NativeFile file(::CreateFileW(path.c_str(), GENERIC_READ, shareAll, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
    if (!file.Valid() || ::GetFileType(file.Get()) != FILE_TYPE_DISK)
    {
        return {};
    }
    BY_HANDLE_FILE_INFORMATION info{};
    if (!::GetFileInformationByHandle(file.Get(), &info) || (info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
    {
        return {};
    }
    if (!IsUnderRoot(rootFinal, FinalPath(file.Get())))
    {
        return {};
    }
    return file;
}

#else // POSIX

constexpr int maxLinkExpansions = 40;

// The pieces of the target of the symbolic link `name` in the directory dirFd, or nothing when the entry is not
// a link (the errno of O_NOFOLLOW on a link differs between systems, so the entry itself is asked), or the
// target is empty, too long or absolute
std::optional<std::vector<std::string>> ReadLinkPieces(int dirFd, const std::string& name)
{
    struct stat linkStat
    {
    };
    if (::fstatat(dirFd, name.c_str(), &linkStat, AT_SYMLINK_NOFOLLOW) != 0 || !S_ISLNK(linkStat.st_mode))
    {
        return std::nullopt;
    }
    std::string target(4096, '\0');
    const auto length = ::readlinkat(dirFd, name.c_str(), target.data(), target.size());
    if (length <= 0 || static_cast<std::size_t>(length) >= target.size() || target.front() == '/')
    {
        return std::nullopt;
    }
    target.resize(static_cast<std::size_t>(length));

    std::vector<std::string> result;
    std::string_view rest = target;
    while (true)
    {
        const auto end = rest.find('/');
        result.emplace_back(rest.substr(0, end));
        if (end == std::string_view::npos)
        {
            return result;
        }
        rest.remove_prefix(end + 1);
    }
}

enum class OpenStep
{
    NotOpened, // the entry could not be opened (it may be a symbolic link)
    Refused,   // the entry was opened, but it is not usable: a last piece that is not a regular file
    Opened     // `file` is the directory entered, or for the last piece the regular file
};

// Whether st describes a regular file, and when `same` is given, the same one
bool IsRegularFile(const struct stat& st, const struct stat* same)
{
    if (!S_ISREG(st.st_mode))
    {
        return false;
    }
    if (!same)
    {
        return true;
    }
    return st.st_dev == same->st_dev && st.st_ino == same->st_ino;
}

// One step of a walk: the last piece is opened as a file, an earlier one as a directory. Links are not followed.
// A device or FIFO is never opened for reading, as opening one may act (a tape rewinds, a FIFO's writer wakes).
// On Linux the last piece is pinned with O_PATH, which opens nothing, checked, and only then reopened for reading
// through /proc/self/fd, so an entry swapped for a FIFO in between cannot be opened. Elsewhere, or without /proc,
// the entry is checked before the openat and the opened descriptor after it: a swap in between may open a special
// file, which is then closed unread
OpenStep OpenPiece(int dirFd, const std::string& name, bool last, NativeFile* file)
{
    if (!last)
    {
        *file = NativeFile(::openat(dirFd, name.c_str(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC));
        return file->Valid() ? OpenStep::Opened : OpenStep::NotOpened;
    }

    struct stat pinned
    {
    };
#ifdef O_PATH
    const NativeFile path(::openat(dirFd, name.c_str(), O_PATH | O_NOFOLLOW | O_CLOEXEC));
    if (!path.Valid() || ::fstat(path.Get(), &pinned) != 0)
    {
        return OpenStep::NotOpened;
    }
    if (S_ISLNK(pinned.st_mode))
    {
        return OpenStep::NotOpened; // the caller reads the link
    }
    if (!IsRegularFile(pinned, nullptr))
    {
        return OpenStep::Refused;
    }
    const std::string procPath = "/proc/self/fd/" + std::to_string(path.Get());
    *file = NativeFile(::open(procPath.c_str(), O_RDONLY | O_CLOEXEC | O_NOCTTY));
    if (!file->Valid() && errno == ENOENT)
#else
    if (::fstatat(dirFd, name.c_str(), &pinned, AT_SYMLINK_NOFOLLOW) != 0 || S_ISLNK(pinned.st_mode))
    {
        return OpenStep::NotOpened;
    }
    if (!IsRegularFile(pinned, nullptr))
    {
        return OpenStep::Refused;
    }
#endif
    {
        // No /proc (or no O_PATH): open by name, which the identity check below ties to the entry checked above
        *file = NativeFile(::openat(dirFd, name.c_str(), O_RDONLY | O_NOFOLLOW | O_CLOEXEC | O_NONBLOCK | O_NOCTTY));
    }
    struct stat st
    {
    };
    if (!file->Valid() || ::fstat(file->Get(), &st) != 0 || !IsRegularFile(st, &pinned))
    {
        return OpenStep::Refused;
    }
    return OpenStep::Opened;
}

// Walks the pieces from a descriptor of the root, one openat per piece with O_NOFOLLOW, and keeps the
// descriptors of the directories entered so far. A symbolic link met on the way is read and its target is
// walked the same way (spliced in front of the remaining pieces), so no step can leave the root: an absolute
// target is refused and a ".." that would climb above the root is refused. The descriptor returned is the one
// opened by the last step, and it is checked to be a regular file
NativeFile OpenConfinedFile(const std::string& root, const std::vector<std::string_view>& pieces)
{
    std::vector<NativeFile> dirs;
    dirs.emplace_back(::open(root.empty() ? "." : root.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC));
    if (pieces.empty() || !dirs.back().Valid())
    {
        return {};
    }

    std::deque<std::string> pending(pieces.begin(), pieces.end());
    int expansions = 0;
    while (!pending.empty())
    {
        const std::string piece = std::move(pending.front());
        pending.pop_front();
        if (piece.empty() || piece == ".")
        {
            continue;
        }
        if (piece == "..")
        {
            if (dirs.size() < 2)
            {
                return {};
            }
            dirs.pop_back();
            continue;
        }

        const int dirFd = dirs.back().Get();
        const bool last = pending.empty();
        NativeFile opened;
        const auto step = OpenPiece(dirFd, piece, last, &opened);
        if (step == OpenStep::Refused)
        {
            return {};
        }
        if (step == OpenStep::Opened)
        {
            if (last)
            {
                return opened;
            }
            dirs.push_back(std::move(opened));
            continue;
        }

        auto targetPieces = ReadLinkPieces(dirFd, piece);
        if (!targetPieces || ++expansions > maxLinkExpansions)
        {
            return {};
        }
        pending.insert(pending.begin(), std::make_move_iterator(targetPieces->begin()), std::make_move_iterator(targetPieces->end()));
    }
    return {};
}

#endif

NativeFile OpenTemplateFile(const std::string& root, const std::string& name)
{
    const auto pieces = SplitTemplateName(name);
    if (!pieces)
    {
        return {};
    }
    return OpenConfinedFile(root, *pieces);
}

// Bytes of the file, read when the stream needs them (a size limit on the reader stops the reading). With
// `text` (Windows), "\r\n" reads as "\n", as RealFileSystem's text-mode streams read it there
class FileStreambuf : public std::streambuf
{
public:
    FileStreambuf(NativeFile file, bool text)
        : m_file(std::move(file))
        , m_text(text)
    {
    }

protected:
    int_type underflow() override
    {
        while (gptr() == egptr())
        {
            // A '\r' held back from the end of the last chunk starts this one
            const std::size_t start = m_heldCr ? 1 : 0;
            m_buffer[0] = '\r';
            const auto count = m_file.Read(m_buffer + start, BufferSize - start);
            if (count <= 0)
            {
                if (!m_heldCr)
                {
                    return traits_type::eof();
                }
                m_heldCr = false;
                setg(m_buffer, m_buffer, m_buffer + 1);
                break;
            }
            const std::size_t size = start + static_cast<std::size_t>(count);
            std::size_t out = size;
            m_heldCr = false;
            if (m_text)
            {
                out = 0;
                for (std::size_t i = 0; i < size; ++i)
                {
                    if (m_buffer[i] == '\r' && i + 1 == size)
                    {
                        m_heldCr = true;
                    }
                    else if (m_buffer[i] != '\r' || m_buffer[i + 1] != '\n')
                    {
                        m_buffer[out++] = m_buffer[i];
                    }
                }
            }
            setg(m_buffer, m_buffer, m_buffer + out);
        }
        return traits_type::to_int_type(*gptr());
    }

private:
    NativeFile m_file;
    bool m_text;
    bool m_heldCr = false;
    static constexpr std::size_t BufferSize = 16384;
    char m_buffer[BufferSize]{};
};

#ifdef _WIN32
constexpr bool textStreams = true;
#else
constexpr bool textStreams = false;
#endif

class FileIStream : public std::istream
{
public:
    FileIStream(NativeFile file, bool text)
        : std::istream(nullptr)
        , m_buf(std::move(file), text)
    {
        rdbuf(&m_buf);
    }

private:
    FileStreambuf m_buf;
};

// Length of an incomplete UTF-8 sequence at the end of the bytes (0 when the end is complete or malformed)
std::size_t IncompleteUtf8Tail(const std::string& bytes)
{
    const std::size_t limit = std::min<std::size_t>(3, bytes.size());
    for (std::size_t back = 1; back <= limit; ++back)
    {
        const auto lead = static_cast<unsigned char>(bytes[bytes.size() - back]);
        if ((lead & 0xC0U) == 0x80U)
        {
            continue; // a continuation byte, look further back
        }
        std::size_t needed = 1;
        if (lead >= 0xF0U && lead < 0xF8U)
        {
            needed = 4;
        }
        else if (lead >= 0xE0U && lead < 0xF0U)
        {
            needed = 3;
        }
        else if (lead >= 0xC0U && lead < 0xE0U)
        {
            needed = 2;
        }
        return needed > back ? back : 0;
    }
    return 0;
}

// Decodes the UTF-8 bytes of the file chunk by chunk; an incomplete sequence at the end of a chunk waits for
// the next one. A malformed sequence becomes U+FFFD, as in ConvertString
class FileWideStreambuf : public std::wstreambuf
{
public:
    explicit FileWideStreambuf(NativeFile file)
        : m_in(std::move(file), textStreams)
    {
    }

protected:
    int_type underflow() override
    {
        while (true)
        {
            if (gptr() < egptr())
            {
                return traits_type::to_int_type(*gptr());
            }
            if (m_done)
            {
                return traits_type::eof();
            }
            char chunk[8192];
            std::string bytes = std::move(m_carry);
            m_carry.clear();
            const auto count = m_in.sgetn(chunk, sizeof(chunk));
            if (count <= 0)
            {
                m_done = true; // what is left is an incomplete sequence: it decodes to U+FFFD
            }
            else
            {
                bytes.append(chunk, static_cast<std::size_t>(count));
                const std::size_t keep = IncompleteUtf8Tail(bytes);
                m_carry.assign(bytes, bytes.size() - keep, keep);
                bytes.resize(bytes.size() - keep);
            }
            if (bytes.empty())
            {
                continue;
            }
            m_text = ConvertString<std::wstring>(std::string_view(bytes));
            setg(m_text.data(), m_text.data(), m_text.data() + m_text.size());
        }
    }

private:
    FileStreambuf m_in;
    std::string m_carry;
    std::wstring m_text;
    bool m_done = false;
};

class FileWIStream : public std::wistream
{
public:
    explicit FileWIStream(NativeFile file)
        : std::wistream(nullptr)
        , m_buf(std::move(file))
    {
        rdbuf(&m_buf);
    }

private:
    FileWideStreambuf m_buf;
};

void DeleteNarrow(std::istream* s)
{
    delete static_cast<FileIStream*>(s);
}

void DeleteWide(std::wistream* s)
{
    delete static_cast<FileWIStream*>(s);
}
} // namespace

ConfinedFileSystem::ConfinedFileSystem(std::string rootFolder)
    : m_rootFolder(std::move(rootFolder))
{
}

CharFileStreamPtr ConfinedFileSystem::OpenStream(const std::string& name) const
{
    auto file = OpenTemplateFile(m_rootFolder, name);
    if (!file.Valid())
    {
        return CharFileStreamPtr(nullptr, DeleteNarrow);
    }
    return CharFileStreamPtr(new FileIStream(std::move(file), textStreams), DeleteNarrow);
}

WCharFileStreamPtr ConfinedFileSystem::OpenWStream(const std::string& name) const
{
    auto file = OpenTemplateFile(m_rootFolder, name);
    if (!file.Valid())
    {
        return WCharFileStreamPtr(nullptr, DeleteWide);
    }
    return WCharFileStreamPtr(new FileWIStream(std::move(file)), DeleteWide);
}

CharFileStreamPtr ConfinedFileSystem::OpenByteStream(const std::string& name) const
{
    auto file = OpenTemplateFile(m_rootFolder, name);
    if (!file.Valid())
    {
        return CharFileStreamPtr(nullptr, DeleteNarrow);
    }
    return CharFileStreamPtr(new FileIStream(std::move(file), false), DeleteNarrow);
}

std::optional<std::chrono::system_clock::time_point> ConfinedFileSystem::GetLastModificationDate(const std::string& name) const
{
    const auto file = OpenTemplateFile(m_rootFolder, name);
    if (!file.Valid())
    {
        return {};
    }
    return file.LastWriteTime();
}

bool ConfinedFileSystem::IsEqual(const IComparable& other) const
{
    const auto* ptr = dynamic_cast<const ConfinedFileSystem*>(&other);
    if (!ptr)
    {
        return false;
    }
    return m_rootFolder == ptr->m_rootFolder;
}

} // namespace v2
} // namespace jinja2
