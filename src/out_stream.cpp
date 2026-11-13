#include "out_stream.h"

#include "internal_value.h"
#include "recursion_guard.h"
#include "value_visitors.h"

#include <jinja2cpp/template.h>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>

namespace jinja2
{
inline namespace v2
{
void OutStream::Flush()
{
    if (m_writer)
    {
        return;
    }
    FlushBuffer();
    // A render that writes each piece: what is flushed is a piece's end, unless a piece is held
    if (m_heldPending && !m_holding)
    {
        EndPiece(false);
    }
}

void OutStream::FlushBuffer()
{
    if (m_cur != m_buffer)
    {
        AppendToTarget(m_buffer, static_cast<size_t>(m_cur - m_buffer));
        m_cur = m_buffer;
    }
}

void OutStream::FlushPiece()
{
    if (m_writer || m_holding)
    {
        return;
    }
    FlushBuffer();
    if (m_heldPending)
    {
        EndPiece(true);
    }
}

// The piece gathered in m_held goes to the sink in one Write; an empty one only when `writeEmpty`
JINJA2CPP_NOINLINE void OutStream::EndPiece(bool writeEmpty)
{
    if (m_held.empty() && !writeEmpty)
    {
        return;
    }
    const std::string piece = std::move(m_held);
    m_held.clear();
    ReleaseRenderMemory(m_heldCharged);
    m_heldCharged = 0;
    // Counted when gathered (GatherPiece)
    WriteSinkChunk(piece.data(), piece.size(), 0);
}

void OutStream::AppendToTarget(const void* ptr, size_t bytes)
{
    if (m_sink)
    {
        WriteToSink(ptr, bytes);
        return;
    }
    if (m_charSize == 1)
    {
        static_cast<std::string*>(m_target)->append(static_cast<const char*>(ptr), bytes);
    }
    else
    {
        static_cast<std::wstring*>(m_target)->append(static_cast<const wchar_t*>(ptr), bytes / sizeof(wchar_t));
    }
}

void OutStream::WriteToSink(const void* ptr, size_t bytes)
{
    // One flag on the usual path: a render that writes each piece is the rare case
    if (m_heldPending)
    {
        GatherPiece(ptr, bytes);
        return;
    }
    if (bytes == 0)
    {
        return;
    }
    WriteSinkChunk(ptr, bytes, bytes / m_charSize);
}

// A render that writes each piece (RenderOptions::writeEachPiece) gathers a piece until it ends (Flush,
// FlushPiece), so that it reaches the sink in one Write however long it is, as Jinja2's generate() yields
// it. The bytes are counted as written when gathered, as the limit on the output
// (Settings::Limits::maxOutputSize) and the offsets of the generation spans read the count, and against
// the render's memory budget while they are held
JINJA2CPP_NOINLINE void OutStream::GatherPiece(const void* ptr, size_t bytes)
{
    m_held.append(static_cast<const char*>(ptr), bytes);
    m_sinkWritten += bytes / m_charSize;
    if (RenderMemoryCounted())
    {
        ChargeObjectMemory(bytes);
        m_heldCharged += bytes;
    }
}

void OutStream::WriteSinkChunk(const void* ptr, size_t bytes, size_t newLength)
{
    const size_t length = bytes / m_charSize;
    if (m_charSize == 1)
    {
        static_cast<OutputSink*>(m_sink)->Write(std::string_view(static_cast<const char*>(ptr), length));
    }
    else
    {
        static_cast<OutputSinkW*>(m_sink)->Write(std::wstring_view(static_cast<const wchar_t*>(ptr), length));
    }
    // What a gathered piece adds was counted when it was gathered (GatherPiece)
    m_sinkWritten += newLength;
}

void OutStream::WriteBufferSlow(const void* ptr, size_t length)
{
    if (m_writer)
    {
        m_writer->WriteBuffer(ptr, length);
        return;
    }
    if (length < BufferLength)
    {
        FlushBuffer();
        CopyToBuffer(ptr, length * m_charSize);
        return;
    }
    WriteLong(ptr, length);
}

void OutStream::WriteLong(const void* ptr, size_t length)
{
    // Writer and sink streams have no target string: testing m_target, as m_writer was tested
    // before sinks came, keeps the string's path one test long
    if (!m_target)
    {
        if (m_writer)
        {
            m_writer->WriteBuffer(ptr, length);
            return;
        }
        FlushBuffer();
        WriteToSink(ptr, length * m_charSize);
        return;
    }
    if (m_charSize == 1)
    {
        auto& str = *static_cast<std::string*>(m_target);
        if (m_cur != m_buffer)
        {
            str.append(reinterpret_cast<const char*>(m_buffer), static_cast<size_t>(m_cur - m_buffer));
            m_cur = m_buffer;
        }
        str.append(static_cast<const char*>(ptr), length);
        return;
    }
    FlushBuffer();
    AppendToTarget(ptr, length * m_charSize);
}

void OutStream::WriteAscii(const char* str, size_t length)
{
    if (m_charSize == 1)
    {
        WriteBuffer(str, length);
        return;
    }
    // Numbers and names only: always shorter than the buffer
    if (length * sizeof(wchar_t) > static_cast<size_t>(m_end - m_cur))
    {
        FlushBuffer();
    }
    for (size_t idx = 0; idx != length; ++idx)
    {
        const auto ch = static_cast<wchar_t>(static_cast<unsigned char>(str[idx]));
        std::memcpy(m_cur, &ch, sizeof(ch));
        m_cur += sizeof(ch);
    }
}

void OutStream::WriteBool(bool value)
{
    if (value)
    {
        WriteAscii("True", 4);
    }
    else
    {
        WriteAscii("False", 5);
    }
}

template<typename CharT>
void OutStream::WriteInt(int64_t value)
{
    static constexpr char pairs[] = "00010203040506070809101112131415161718192021222324252627282930313233343536373839"
                                    "40414243444546474849505152535455565758596061626364656667686970717273747576777879"
                                    "8081828384858687888990919293949596979899";
    // A sign and up to 19 digits
    constexpr size_t maxLength = 20;
    if (static_cast<size_t>(m_end - m_cur) < maxLength * sizeof(CharT))
    {
        FlushBuffer();
    }

    auto digits = value < 0 ? 0 - static_cast<uint64_t>(value) : static_cast<uint64_t>(value);
    size_t length = value < 0 ? 2 : 1;
    for (auto rest = digits; rest >= 10; rest /= 10)
    {
        ++length;
    }
    // The number goes straight into the buffer, written from the end two digits at a time.
    // A wide character is stored through memcpy: the buffer holds bytes
    const auto put = [](unsigned char* at, char ch) {
        const auto wch = static_cast<CharT>(ch);
        std::memcpy(at, &wch, sizeof(CharT));
    };
    // The analyzer does not tie the digits written below to `length` counted above
    // NOLINTBEGIN(clang-analyzer-security.ArrayBound)
    // Offsets are in bytes: `start` points at unsigned char
    constexpr size_t width = sizeof(CharT);
    unsigned char* const start = m_cur;
    size_t pos = length * width;
    while (digits >= 100)
    {
        const auto idx = static_cast<size_t>(digits % 100) * 2;
        digits /= 100;
        pos -= 2 * width;
        put(start + pos, pairs[idx]);
        put(start + pos + width, pairs[idx + 1]);
    }
    if (digits >= 10)
    {
        const auto idx = static_cast<size_t>(digits) * 2;
        put(start + pos - (2 * width), pairs[idx]);
        put(start + pos - width, pairs[idx + 1]);
    }
    else
    {
        put(start + pos - width, static_cast<char>('0' + digits));
    }
    if (value < 0)
    {
        put(start, '-');
    }
    // NOLINTEND(clang-analyzer-security.ArrayBound)
    m_cur += length * sizeof(CharT);
}

namespace
{
#ifdef _MSC_VER
#define JINJA2CPP_NOINLINE __declspec(noinline)
#else
#define JINJA2CPP_NOINLINE __attribute__((noinline))
#endif

// Any other value, as ValueRenderer prints it. Kept out of line: its frame would slow
// down the common values
template<typename CharT>
JINJA2CPP_NOINLINE void RenderValue(const InternalValue& val, std::basic_string<CharT>& target)
{
    Apply<visitors::ValueRenderer<CharT>>(val, target);
}

// The index of alternative T in variant V
template<typename T, typename V>
struct IndexOfImpl;
template<typename T, typename... Alts>
struct IndexOfImpl<T, std::variant<Alts...>>
{
    static constexpr size_t Find()
    {
        constexpr bool matches[] = { std::is_same_v<T, Alts>... };
        size_t idx = 0;
        while (!matches[idx])
        {
            ++idx;
        }
        return idx;
    }
};
template<typename T, typename V>
constexpr size_t IndexOf = IndexOfImpl<T, V>::Find();
} // namespace

template<typename CharT>
void OutStream::WriteValueTo(const InternalValue& val)
{
    using string_t = std::basic_string<CharT>;
    using view_t = std::basic_string_view<CharT>;
    // The values a template prints most, written as ValueRenderer writes them. A test per
    // kind, most frequent first, costs less than a dispatch on the kind; strings meet at
    // one WriteString, which then stays inline
    if (const auto* num = GetIf<int64_t>(&val))
    {
        WriteInt<CharT>(*num);
        return;
    }
    auto text = AsStringView<CharT>(val);
    if (!text)
    {
        if (const auto* ref = GetIf<ValueRef>(&val))
        {
            const auto& refData = ref->get().GetData();
            using RefData = std::decay_t<decltype(refData)>;
            switch (refData.index())
            {
            case IndexOf<int64_t, RefData>:
                WriteInt<CharT>(*std::get_if<int64_t>(&refData));
                return;
            case IndexOf<string_t, RefData>:
                text = view_t(*std::get_if<string_t>(&refData));
                break;
            case IndexOf<view_t, RefData>:
                text = *std::get_if<view_t>(&refData);
                break;
            case IndexOf<bool, RefData>:
                WriteBool(*std::get_if<bool>(&refData));
                return;
            default:
                break;
            }
        }
        else if (const auto* flag = GetIf<bool>(&val))
        {
            WriteBool(*flag);
            return;
        }
    }
    if (text)
    {
        WriteString(*text);
        return;
    }

    if (m_sink)
    {
        WriteRenderedToSink<CharT>(val);
        return;
    }
    FlushBuffer();
    RenderValue(val, *static_cast<string_t*>(m_target));
}

// Through the buffer: such values are mostly short (a float, None), and each would otherwise
// be a chunk of its own. Out of line: the string's frame would slow down WriteValueTo
template<typename CharT>
JINJA2CPP_NOINLINE void OutStream::WriteRenderedToSink(const InternalValue& val)
{
    std::basic_string<CharT> rendered;
    RenderValue(val, rendered);
    WriteBuffer(rendered.data(), rendered.size());
}

template void OutStream::WriteValueTo<char>(const InternalValue& val);
template void OutStream::WriteValueTo<wchar_t>(const InternalValue& val);
} // namespace v2
} // namespace jinja2
