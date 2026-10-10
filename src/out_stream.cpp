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
    unsigned char* const start = m_cur;
    unsigned char* pos = start + (length * sizeof(CharT));
    while (digits >= 100)
    {
        const auto idx = static_cast<size_t>(digits % 100) * 2;
        digits /= 100;
        pos -= 2 * sizeof(CharT);
        put(pos, pairs[idx]);
        put(pos + sizeof(CharT), pairs[idx + 1]);
    }
    if (digits >= 10)
    {
        const auto idx = static_cast<size_t>(digits) * 2;
        put(pos - (2 * sizeof(CharT)), pairs[idx]);
        put(pos - sizeof(CharT), pairs[idx + 1]);
    }
    else
    {
        put(pos - sizeof(CharT), static_cast<char>('0' + digits));
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
            const auto& refData = ref->get().data();
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

    FlushBuffer();
    RenderValue(val, *static_cast<string_t*>(m_target));
}

template void OutStream::WriteValueTo<char>(const InternalValue& val);
template void OutStream::WriteValueTo<wchar_t>(const InternalValue& val);
} // namespace jinja2
