#include "out_stream.h"

#include "internal_value.h"
#include "value_visitors.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>

namespace jinja2
{
void OutStream::Flush()
{
    if (!m_writer)
    {
        FlushBuffer();
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

void OutStream::AppendToTarget(const void* ptr, size_t bytes)
{
    if (m_charSize == 1)
    {
        static_cast<std::string*>(m_target)->append(static_cast<const char*>(ptr), bytes);
    }
    else
    {
        static_cast<std::wstring*>(m_target)->append(static_cast<const wchar_t*>(ptr), bytes / sizeof(wchar_t));
    }
}

void OutStream::WriteBufferSlow(const void* ptr, size_t length)
{
    if (m_writer)
    {
        m_writer->WriteBuffer(ptr, length);
        return;
    }
    const size_t bytes = length * m_charSize;
    if (bytes < BufferSize)
    {
        FlushBuffer();
        CopyToBuffer(ptr, bytes);
        return;
    }
    WriteLong(ptr, length);
}

void OutStream::WriteLong(const void* ptr, size_t length)
{
    if (m_writer)
    {
        m_writer->WriteBuffer(ptr, length);
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
    if (length * sizeof(wchar_t) > static_cast<size_t>(m_buffer + BufferSize - m_cur))
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

void OutStream::WriteInt(int64_t value)
{
    static constexpr char pairs[] = "00010203040506070809101112131415161718192021222324252627282930313233343536373839"
                                    "40414243444546474849505152535455565758596061626364656667686970717273747576777879"
                                    "8081828384858687888990919293949596979899";
    // A sign and up to 19 digits
    constexpr size_t maxLength = 20;
    char text[maxLength];
    const bool isNarrow = m_charSize == 1;
    if (isNarrow && static_cast<size_t>(m_buffer + BufferSize - m_cur) < maxLength)
    {
        FlushBuffer();
    }

    auto digits = value < 0 ? 0 - static_cast<uint64_t>(value) : static_cast<uint64_t>(value);
    size_t length = value < 0 ? 2 : 1;
    for (auto rest = digits; rest >= 10; rest /= 10)
    {
        ++length;
    }
    // A narrow number goes straight into the buffer, written from the end two digits at a time.
    // The analyzer does not tie the digits written below to `length` counted above
    // NOLINTBEGIN(clang-analyzer-security.ArrayBound)
    char* const start = isNarrow ? reinterpret_cast<char*>(m_cur) : text;
    char* pos = start + length;
    while (digits >= 100)
    {
        const auto idx = static_cast<size_t>(digits % 100) * 2;
        digits /= 100;
        pos -= 2;
        pos[0] = pairs[idx];
        pos[1] = pairs[idx + 1];
    }
    if (digits >= 10)
    {
        const auto idx = static_cast<size_t>(digits) * 2;
        pos[-2] = pairs[idx];
        pos[-1] = pairs[idx + 1];
    }
    else
    {
        pos[-1] = static_cast<char>('0' + digits);
    }
    if (value < 0)
    {
        *start = '-';
    }
    // NOLINTEND(clang-analyzer-security.ArrayBound)

    if (isNarrow)
    {
        m_cur += length;
    }
    else
    {
        WriteAscii(text, length);
    }
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
    const auto& data = val.GetData();
    // The values a template prints most, written as ValueRenderer writes them
    switch (data.index())
    {
    case IndexOf<int64_t, InternalValueData>:
        WriteInt(*std::get_if<int64_t>(&data));
        return;
    case IndexOf<std::string, InternalValueData>:
        if constexpr (std::is_same_v<CharT, char>)
        {
            WriteString(*std::get_if<std::string>(&data));
            return;
        }
        break;
    case IndexOf<ValueRef, InternalValueData>:
    {
        const auto& refData = std::get_if<ValueRef>(&data)->get().data();
        using RefData = std::decay_t<decltype(refData)>;
        switch (refData.index())
        {
        case IndexOf<int64_t, RefData>:
            WriteInt(*std::get_if<int64_t>(&refData));
            return;
        case IndexOf<string_t, RefData>:
            WriteString(*std::get_if<string_t>(&refData));
            return;
        case IndexOf<view_t, RefData>:
            WriteString(*std::get_if<view_t>(&refData));
            return;
        case IndexOf<bool, RefData>:
            WriteBool(*std::get_if<bool>(&refData));
            return;
        default:
            break;
        }
        break;
    }
    case IndexOf<bool, InternalValueData>:
        WriteBool(*std::get_if<bool>(&data));
        return;
    case IndexOf<TargetString, InternalValueData>:
        if (const auto* str = std::get_if<string_t>(std::get_if<TargetString>(&data)))
        {
            WriteString(*str);
            return;
        }
        break;
    case IndexOf<TargetStringView, InternalValueData>:
        if (const auto* str = std::get_if<view_t>(std::get_if<TargetStringView>(&data)))
        {
            WriteString(*str);
            return;
        }
        break;
    default:
        break;
    }

    FlushBuffer();
    RenderValue(val, *static_cast<string_t*>(m_target));
}

template void OutStream::WriteValueTo<char>(const InternalValue& val);

void OutStream::WriteValueSlow(const InternalValue& val)
{
    if (m_writer)
    {
        m_writer->WriteValue(val);
    }
    else
    {
        WriteValueTo<wchar_t>(val);
    }
}
} // namespace jinja2
