#ifndef JINJA2CPP_SRC_OUT_STREAM_H
#define JINJA2CPP_SRC_OUT_STREAM_H

#include "internal_value.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <utility>

namespace jinja2
{
// Where a render writes its output. A render to a string, the common case, gathers the
// output in a small buffer and appends it to the string a buffer at a time: a fragment
// costs a bounds check and a memcpy, with no virtual call (docs/tasks/0113). Any other
// target goes through a StreamWriter.
class OutStream
{
public:
    struct StreamWriter
    {
        virtual ~StreamWriter() = default;

        virtual void WriteBuffer(const void* ptr, size_t length) = 0;
        virtual void WriteValue(const InternalValue& val) = 0;
    };

    // The buffer is left uninitialised: only its written part is ever read
    // NOLINTBEGIN(cppcoreguidelines-pro-type-member-init)
    // Writes through a writer that outlives the stream
    explicit OutStream(StreamWriter* writer)
        : m_writer(writer)
        , m_end(m_buffer)
    {}
    // Appends to `target`, which must outlive the stream. The output reaches the string
    // only when the stream is flushed: a render that throws leaves the rest unwritten.
    explicit OutStream(std::string& target)
        : m_target(&target)
    {}
    explicit OutStream(std::wstring& target)
        : m_target(&target)
        , m_charSize(sizeof(wchar_t))
        , m_end(m_buffer + sizeof(m_buffer))
    {}
    // NOLINTEND(cppcoreguidelines-pro-type-member-init)

    // A stream refers to its own buffer, so it stays where it was made (return it as a prvalue)
    OutStream(const OutStream&) = delete;
    OutStream(OutStream&&) = delete;
    OutStream& operator=(const OutStream&) = delete;
    OutStream& operator=(OutStream&&) = delete;
    ~OutStream() = default;

    // `length` counts characters of the target's type
    void WriteBuffer(const void* ptr, size_t length)
    {
        const size_t bytes = length * m_charSize;
        if (bytes <= static_cast<size_t>(m_end - m_cur))
        {
            CopyToBuffer(ptr, bytes);
            return;
        }
        WriteBufferSlow(ptr, length);
    }

    // A fragment this long skips the buffer: copying it there would only add a copy
    static constexpr size_t LongLength = 128;
    // Writes a fragment of at least LongLength characters (any length works, but a short
    // one is cheaper through WriteBuffer)
    void WriteLong(const void* ptr, size_t length);

    void WriteValue(const InternalValue& val)
    {
        if (m_writer)
        {
            m_writer->WriteValue(val);
        }
        else if (m_charSize == 1)
        {
            WriteValueTo<char>(val);
        }
        else
        {
            WriteValueTo<wchar_t>(val);
        }
    }

    // Appends the buffered output to the target string
    void Flush();

private:
    // The buffer holds this many characters of the target's type: a wide stream gets as
    // many characters as a narrow one, so it flushes as often (docs/tasks/0132)
    static constexpr size_t BufferLength = 512;

    // Most fragments are a few bytes: copy them inline rather than call memcpy
    void CopyToBuffer(const void* ptr, size_t bytes)
    {
        const auto* src = static_cast<const unsigned char*>(ptr);
        unsigned char* dst = m_cur;
        m_cur = dst + bytes;
        if (bytes > 16)
        {
            std::memcpy(dst, src, bytes);
        }
        else if (bytes >= 8)
        {
            CopyEnds<std::uint64_t>(dst, src, bytes);
        }
        else if (bytes >= 4)
        {
            CopyEnds<std::uint32_t>(dst, src, bytes);
        }
        else if (bytes != 0)
        {
            const unsigned char first = src[0];
            const unsigned char middle = src[bytes / 2];
            const unsigned char last = src[bytes - 1];
            dst[0] = first;
            dst[bytes / 2] = middle;
            dst[bytes - 1] = last;
        }
    }
    // Copies sizeof(T) <= bytes <= 2 * sizeof(T) bytes as two words that may overlap
    template<typename T>
    static void CopyEnds(unsigned char* dst, const unsigned char* src, size_t bytes)
    {
        T head{};
        T tail{};
        std::memcpy(&head, src, sizeof(T));
        std::memcpy(&tail, src + bytes - sizeof(T), sizeof(T));
        std::memcpy(dst, &head, sizeof(T));
        std::memcpy(dst + bytes - sizeof(T), &tail, sizeof(T));
    }
    void WriteBufferSlow(const void* ptr, size_t length);
    void WriteAscii(const char* str, size_t length);
    template<typename CharT>
    void WriteInt(int64_t value);
    void WriteBool(bool value);
    template<typename StringT>
    void WriteString(const StringT& str)
    {
        WriteBuffer(str.data(), str.size());
    }
    void FlushBuffer();
    void AppendToTarget(const void* ptr, size_t bytes);
    template<typename CharT>
    void WriteValueTo(const InternalValue& val);

    StreamWriter* m_writer = nullptr;
    // The target string: std::string or std::wstring by m_charSize
    void* m_target = nullptr;
    size_t m_charSize = 1;
    // [m_buffer, m_cur) is output not yet in the target, [m_cur, m_end) the room left. A
    // narrow stream uses the first BufferLength bytes only. A writer stream has no room, so
    // every write takes the slow path to the writer
    alignas(wchar_t) unsigned char m_buffer[BufferLength * sizeof(wchar_t)];
    unsigned char* m_cur = m_buffer;
    unsigned char* m_end = m_buffer + BufferLength;
};

// Renders into a new string with `render(OutStream&)` and returns it
template<typename Callback, typename Fn>
TargetString RenderToString(Callback* callback, Fn&& render)
{
    TargetString result;
    auto stream = callback->GetStreamOnString(result);
    std::forward<Fn>(render)(stream);
    stream.Flush();
    return result;
}

} // namespace jinja2

#endif // JINJA2CPP_SRC_OUT_STREAM_H
