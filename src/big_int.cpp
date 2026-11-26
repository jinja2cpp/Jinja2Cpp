#include "big_int.h"

#include "contracts.h"
#include "decimal_number.h"
#include "engine_errors.h"
#include "internal_value.h"
#include "intrusive_ptr.h"
#include "recursion_guard.h"
#include "span.h"

#include <jinja2cpp/big_int.h>
#include <jinja2cpp/big_int_backend.h>

#include <fmt/format.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <charconv>
#include <cmath>
#include <cstring>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <new>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace jinja2
{
inline namespace v2
{
namespace
{
// A magnitude as limbs of a radix of at most 2^32, least significant limb first: the core's only
// arithmetic, used to change bases. The decimal form is in base 10^9 and the binary form in base
// 2^32. A conversion splits the number in halves by a power of the source radix and multiplies
// in the target radix (Karatsuba), so it takes about O(n^1.6) rather than the O(n^2) of a digit
// at a time, which at MaxBigIntBits is the difference between milliseconds and seconds
// (docs/tasks/0287)
constexpr uint32_t LimbBase = 1000000000;
constexpr size_t LimbDigits = 9;
using Limbs = std::vector<uint32_t>;
using LimbSpan = Span<const uint32_t>;

// log2(10): the bits of a decimal digit
constexpr double BitsPerDigit = 3.321928094887362;

// The radix of limbs. The decimal and binary ones are constants, so dividing by them is cheap
struct DecimalRadix
{
    static constexpr uint64_t Value() noexcept { return LimbBase; }
};
struct BinaryRadix
{
    static constexpr uint64_t Value() noexcept { return uint64_t{ 1 } << 32; }
};
// base^k for the digits of another base
struct AnyRadix
{
    uint64_t value;
    [[nodiscard]] uint64_t Value() const noexcept { return value; }
};

// Products and sums of limbs stay within 64 bits: (R - 1)^2 + 2 (R - 1) < R^2 <= 2^64
static_assert(BinaryRadix::Value() <= (uint64_t{ 1 } << 32), "a limb product must fit 64 bits");

// `limbs` without its leading zero limbs
LimbSpan Trimmed(LimbSpan limbs) noexcept
{
    auto size = limbs.size();
    while (size != 0 && limbs[size - 1] == 0)
    {
        --size;
    }
    return limbs.first(size);
}

// acc += addend * radix^offset
template<typename Radix>
void AddAt(Limbs& acc, LimbSpan addend, size_t offset, Radix radix)
{
    const uint64_t base = radix.Value();
    if (acc.size() < offset + addend.size())
    {
        acc.resize(offset + addend.size(), 0);
    }
    uint64_t carry = 0;
    for (size_t idx = 0; idx != addend.size(); ++idx)
    {
        const uint64_t cur = uint64_t{ acc[offset + idx] } + addend[idx] + carry;
        carry = cur >= base ? 1 : 0;
        acc[offset + idx] = static_cast<uint32_t>(cur - (carry * base));
    }
    for (auto idx = offset + addend.size(); carry != 0; ++idx)
    {
        if (idx == acc.size())
        {
            acc.push_back(1);
            break;
        }
        const uint64_t cur = uint64_t{ acc[idx] } + 1;
        carry = cur >= base ? 1 : 0;
        acc[idx] = static_cast<uint32_t>(cur - (carry * base));
    }
}

// acc -= subtrahend, which is at most acc
template<typename Radix>
void SubtractFrom(Limbs& acc, LimbSpan subtrahend, Radix radix)
{
    const uint64_t base = radix.Value();
    assert(subtrahend.size() <= acc.size());
    uint64_t borrow = 0;
    size_t idx = 0;
    for (; idx != subtrahend.size(); ++idx)
    {
        const uint64_t sub = uint64_t{ subtrahend[idx] } + borrow;
        const uint64_t cur = acc[idx];
        borrow = cur < sub ? 1 : 0;
        acc[idx] = static_cast<uint32_t>(cur + (borrow * base) - sub);
    }
    for (; borrow != 0; ++idx)
    {
        assert(idx < acc.size());
        borrow = acc[idx] == 0 ? 1 : 0;
        acc[idx] = static_cast<uint32_t>(borrow != 0 ? base - 1 : acc[idx] - 1);
    }
}

template<typename Radix>
Limbs Sum(LimbSpan left, LimbSpan right, Radix radix)
{
    Limbs result(left.begin(), left.end());
    AddAt(result, right, 0, radix);
    return result;
}

// Operands at most this long multiply digit by digit, which is faster than splitting them
constexpr size_t KaratsubaCutoff = 48;

// The rows of products that 64-bit columns can add up before they must carry. A product is at
// most (R - 1)^2, and the remainder and carry a column holds then are below (rows + 2) R, which
// one more product's room covers for the radixes here (R > 2^26). Zero for radix 2^32
template<typename Radix>
uint64_t LazyRows(Radix radix) noexcept
{
    const uint64_t top = radix.Value() - 1;
    return top > 0xffffffffU ? 0 : (std::numeric_limits<uint64_t>::max() / (top * top)) - 1;
}

// out = left * right digit by digit, both at most KaratsubaCutoff long; out holds
// left.size() + right.size() zero limbs
template<typename Radix>
void MultiplySchoolbook(Limbs& out, LimbSpan left, LimbSpan right, Radix radix)
{
    assert(left.size() <= KaratsubaCutoff && right.size() <= KaratsubaCutoff);
    const uint64_t base = radix.Value();
    const uint64_t rows = LazyRows(radix);
    if (rows < 2)
    {
        for (size_t i = 0; i != left.size(); ++i)
        {
            const uint64_t digit = left[i];
            uint64_t carry = 0;
            for (size_t j = 0; j != right.size(); ++j)
            {
                const uint64_t cur = uint64_t{ out[i + j] } + (digit * right[j]) + carry;
                out[i + j] = static_cast<uint32_t>(cur % base);
                carry = cur / base;
            }
            out[i + right.size()] = static_cast<uint32_t>(carry);
        }
        return;
    }
    // Columns add up products without carrying, which the compiler can vectorise, and carry once
    // every `rows` rows: for base 10^9, one division per 18 products rather than per product
    std::array<uint64_t, 2 * KaratsubaCutoff> columns{};
    const size_t total = left.size() + right.size();
    const auto carryAll = [&columns, total](uint64_t divisor) {
        uint64_t carry = 0;
        for (size_t k = 0; k != total; ++k)
        {
            const uint64_t cur = columns[k] + carry;
            columns[k] = cur % divisor;
            carry = cur / divisor;
        }
        assert(carry == 0);
    };
    uint64_t pending = 0;
    for (size_t i = 0; i != left.size(); ++i)
    {
        const uint64_t digit = left[i];
        for (size_t j = 0; j != right.size(); ++j)
        {
            columns[i + j] += digit * right[j];
        }
        if (++pending == rows)
        {
            carryAll(base);
            pending = 0;
        }
    }
    carryAll(base);
    for (size_t k = 0; k != total; ++k)
    {
        out[k] = static_cast<uint32_t>(columns[k]);
    }
}

// The conversions below recurse about 2 log2(n / 32) deep, which the bit cap bounds; a
// conversion that starts deep in a nested render still checks the stack (REVIEW.md)
void CheckConversionStack()
{
    if (StackNearlyExhausted())
    {
        throw RecursionLimitError();
    }
}

// left * right in left.size() + right.size() limbs
template<typename Radix>
Limbs Multiply(LimbSpan left, LimbSpan right, Radix radix)
{
    if (left.size() < right.size())
    {
        std::swap(left, right);
    }
    Limbs out(left.size() + right.size(), 0);
    if (right.size() <= KaratsubaCutoff)
    {
        // `left` in pieces as long as the cutoff
        Limbs part;
        for (size_t pos = 0; pos < left.size(); pos += KaratsubaCutoff)
        {
            const auto piece = left.subspan(pos, std::min(KaratsubaCutoff, left.size() - pos));
            part.assign(piece.size() + right.size(), 0);
            MultiplySchoolbook(part, piece, right, radix);
            AddAt(out, Trimmed(part), pos, radix);
        }
        return out;
    }
    CheckConversionStack();
    if (2 * right.size() <= left.size())
    {
        // Unbalanced: `left` in pieces as long as `right`
        for (size_t pos = 0; pos < left.size(); pos += right.size())
        {
            const auto piece = Trimmed(left.subspan(pos, std::min(right.size(), left.size() - pos)));
            if (!piece.empty())
            {
                const auto part = Multiply(piece, right, radix);
                AddAt(out, Trimmed(part), pos, radix);
            }
        }
        return out;
    }
    // left = l1 R^h + l0, right = r1 R^h + r0, and right is longer than h:
    // left * right = z2 R^2h + ((l0 + l1)(r0 + r1) - z2 - z0) R^h + z0
    const size_t half = left.size() / 2;
    const auto l0 = Trimmed(left.first(half));
    const auto l1 = left.subspan(half);
    const auto r0 = Trimmed(right.first(half));
    const auto r1 = right.subspan(half);
    const auto z0 = Multiply(l0, r0, radix);
    const auto z2 = Multiply(l1, r1, radix);
    const auto leftSum = Sum(l0, l1, radix);
    const auto rightSum = Sum(r0, r1, radix);
    auto z1 = Multiply(Trimmed(leftSum), Trimmed(rightSum), radix);
    SubtractFrom(z1, Trimmed(z0), radix);
    SubtractFrom(z1, Trimmed(z2), radix);
    AddAt(out, Trimmed(z0), 0, radix);
    AddAt(out, Trimmed(z1), half, radix);
    AddAt(out, Trimmed(z2), 2 * half, radix);
    out.resize(left.size() + right.size());
    return out;
}

// limbs = limbs * mult + add, all in `radix`; mult <= 2^32 and add < mult, so that
// limb * mult + carry <= radix * mult <= 2^64 stays within 64 bits
template<typename Radix>
void MulAdd(Limbs& limbs, uint64_t mult, uint64_t add, Radix radix)
{
    const uint64_t base = radix.Value();
    uint64_t carry = add;
    for (auto& limb : limbs)
    {
        const uint64_t cur = (uint64_t{ limb } * mult) + carry;
        limb = static_cast<uint32_t>(cur % base);
        carry = cur / base;
    }
    while (carry != 0)
    {
        limbs.push_back(static_cast<uint32_t>(carry % base));
        carry /= base;
    }
}

// Changes limbs of radix `from` (at most 2^32) to limbs of `to`. Halves of the source split at
// from^(Block 2^k), whose powers in the target radix it computes once each
template<typename Radix>
class RadixConverter
{
public:
    RadixConverter(uint64_t from, Radix to)
        : m_from(from)
        , m_to(to)
    {
    }

    // The magnitude `source` in the target radix, without leading zero limbs
    Limbs Convert(LimbSpan source)
    {
        auto result = Run(Trimmed(source));
        result.resize(Trimmed(result).size());
        return result;
    }

private:
    // Sources at most this long convert a limb at a time
    static constexpr size_t Block = 32;

    Limbs Run(LimbSpan source)
    {
        if (source.size() <= Block)
        {
            Limbs result;
            for (auto idx = source.size(); idx != 0; --idx)
            {
                MulAdd(result, m_from, source[idx - 1], m_to);
            }
            return result;
        }
        CheckConversionStack();
        // The largest Block 2^k below the length, so that both halves are shorter than the source
        size_t level = 0;
        while ((Block << (level + 1)) < source.size())
        {
            ++level;
        }
        const size_t split = Block << level;
        const auto high = Run(Trimmed(source.subspan(split)));
        const auto low = Run(Trimmed(source.first(split)));
        auto result = Multiply(Trimmed(high), Power(level), m_to);
        AddAt(result, Trimmed(low), 0, m_to);
        return result;
    }

    // from^(Block 2^level) in the target radix
    LimbSpan Power(size_t level)
    {
        if (m_powers.empty())
        {
            Limbs first{ 1 };
            for (size_t idx = 0; idx != Block; ++idx)
            {
                MulAdd(first, m_from, 0, m_to);
            }
            m_powers.push_back(std::move(first));
        }
        while (m_powers.size() <= level)
        {
            const LimbSpan last = Trimmed(m_powers.back());
            auto next = Multiply(last, last, m_to);
            next.resize(Trimmed(next).size());
            m_powers.push_back(std::move(next));
        }
        return m_powers[level];
    }

    uint64_t m_from;
    Radix m_to;
    std::vector<Limbs> m_powers;
};

// The limbs of base^k for the largest k with base^k <= 2^32, and that k
struct ChunkRadix
{
    uint64_t value;
    size_t digits;
};

ChunkRadix DigitChunk(int base) noexcept
{
    auto mult = static_cast<uint64_t>(base);
    size_t digits = 1;
    while (mult * static_cast<uint64_t>(base) <= BinaryRadix::Value())
    {
        mult *= static_cast<uint64_t>(base);
        ++digits;
    }
    return { mult, digits };
}

Limbs ToLimbs(std::string_view digits)
{
    Limbs limbs;
    limbs.reserve((digits.size() / LimbDigits) + 1);
    auto end = digits.size();
    while (end != 0)
    {
        const auto begin = end > LimbDigits ? end - LimbDigits : 0;
        uint32_t limb = 0;
        for (auto idx = begin; idx != end; ++idx)
        {
            limb = (limb * 10) + static_cast<uint32_t>(digits[idx] - '0');
        }
        limbs.push_back(limb);
        end = begin;
    }
    return limbs;
}

std::string FromLimbs(const Limbs& limbs, bool negative)
{
    std::string result;
    if (negative)
    {
        result.push_back('-');
    }
    auto top = limbs.size();
    while (top > 1 && limbs[top - 1] == 0)
    {
        --top;
    }
    result += std::to_string(top == 0 ? 0 : limbs[top - 1]);
    for (auto idx = top > 0 ? top - 1 : 0; idx != 0; --idx)
    {
        char buffer[LimbDigits];
        auto limb = limbs[idx - 1];
        for (size_t pos = LimbDigits; pos != 0; --pos)
        {
            buffer[pos - 1] = static_cast<char>('0' + (limb % 10));
            limb /= 10;
        }
        result.append(buffer, LimbDigits);
    }
    return result;
}

int DigitValue(char ch)
{
    if (ch >= '0' && ch <= '9')
    {
        return ch - '0';
    }
    if (ch >= 'a' && ch <= 'z')
    {
        return ch - 'a' + 10;
    }
    return ch - 'A' + 10;
}

// The decimal form of `digits` (valid in `base` 2-36, no leading zero, not empty)
std::string DecimalFromDigits(std::string_view digits, int base, bool negative)
{
    if (base == 10)
    {
        return (negative ? "-" : "") + std::string(digits);
    }
    const auto chunk = DigitChunk(base);
    // Limbs of base^k, least significant first
    Limbs source;
    source.reserve((digits.size() / chunk.digits) + 1);
    auto end = digits.size();
    while (end != 0)
    {
        const auto begin = end > chunk.digits ? end - chunk.digits : 0;
        uint64_t limb = 0;
        for (auto idx = begin; idx != end; ++idx)
        {
            limb = (limb * static_cast<uint64_t>(base)) + static_cast<uint64_t>(DigitValue(digits[idx]));
        }
        source.push_back(static_cast<uint32_t>(limb));
        end = begin;
    }
    // base^k is 2^32 for bases 2, 4, 16 and 256: a limb of it is its own value then
    RadixConverter<DecimalRadix> converter(chunk.value, DecimalRadix{});
    return FromLimbs(converter.Convert(source), negative);
}

// The magnitude of a canonical decimal form in base 2^32, without leading zero limbs
Limbs BinaryWords(std::string_view magnitude)
{
    RadixConverter<BinaryRadix> converter(LimbBase, BinaryRadix{});
    return converter.Convert(ToLimbs(magnitude));
}

// The bits of a word: 0 for 0
size_t WordBits(uint32_t word) noexcept
{
    size_t bits = 0;
    for (; word != 0; word >>= 1)
    {
        ++bits;
    }
    return bits;
}

bool IsPowerOfTwo(int base)
{
    return (base & (base - 1)) == 0;
}

// The bits of a magnitude in base 2^32 without leading zero words: 0 for 0
size_t WordsBits(LimbSpan words) noexcept
{
    return words.empty() ? 0 : ((words.size() - 1) * 32) + WordBits(words[words.size() - 1]);
}

// The bits of a magnitude given in decimal: 0 for 0
size_t MagnitudeBits(std::string_view magnitude)
{
    return WordsBits(BinaryWords(magnitude));
}

// The set bits of a magnitude in base 2^32
size_t WordsBitCount(LimbSpan words) noexcept
{
    size_t count = 0;
    for (const auto word : words)
    {
        for (auto rest = word; rest != 0; rest &= rest - 1)
        {
            ++count;
        }
    }
    return count;
}

// The digits of a magnitude in base 2^32 without leading zero words, in a power-of-two base (2 to 32), lower case
std::string WordsDigits(LimbSpan words, int base)
{
    if (words.empty())
    {
        return "0";
    }
    const auto bitsPerDigit = WordBits(static_cast<uint32_t>(base)) - 1;
    const auto totalBits = WordsBits(words);
    const auto count = (totalBits + bitsPerDigit - 1) / bitsPerDigit;
    const auto mask = static_cast<uint64_t>(base - 1);
    std::string result(count, '0');
    // Digit `idx` from the least significant end holds the bits from idx * bitsPerDigit up
    for (size_t idx = 0; idx != count; ++idx)
    {
        const auto bit = idx * bitsPerDigit;
        const auto word = bit / 32;
        const auto shift = bit % 32;
        uint64_t window = uint64_t{ words[word] } >> shift;
        if (shift + bitsPerDigit > 32 && word + 1 < words.size())
        {
            window |= uint64_t{ words[word + 1] } << (32 - shift);
        }
        result[count - 1 - idx] = "0123456789abcdefghijklmnopqrstuvwxyz"[window & mask];
    }
    return result;
}

// The digits of a magnitude given in decimal, in a power-of-two base (2 to 32), lower case
std::string MagnitudeDigits(std::string_view magnitude, int base)
{
    return WordsDigits(BinaryWords(magnitude), base);
}

// The words of `digits` (valid in a power-of-two `base`, no leading zero, not empty), without
// leading zero words: each digit is a run of bits, so this packs them without arithmetic
Limbs PackPowerOfTwoDigits(std::string_view digits, int base)
{
    const auto bitsPerDigit = WordBits(static_cast<uint32_t>(base)) - 1;
    Limbs words;
    words.reserve(((digits.size() * bitsPerDigit) / 32) + 1);
    uint64_t acc = 0;
    size_t accBits = 0;
    for (auto idx = digits.size(); idx != 0; --idx)
    {
        acc |= static_cast<uint64_t>(DigitValue(digits[idx - 1])) << accBits;
        accBits += bitsPerDigit;
        if (accBits >= 32)
        {
            words.push_back(static_cast<uint32_t>(acc));
            acc >>= 32;
            accBits -= 32;
        }
    }
    if (accBits != 0)
    {
        words.push_back(static_cast<uint32_t>(acc));
    }
    words.resize(Trimmed(words).size());
    return words;
}

// The canonical decimal form of a magnitude in base 2^32 (not zero)
std::string DecimalFromWords(LimbSpan words, bool negative)
{
    RadixConverter<DecimalRadix> converter(BinaryRadix::Value(), DecimalRadix{});
    return FromLimbs(converter.Convert(words), negative);
}

// The digits of a view's decimal form without its sign
std::string_view InlineMagnitude(const BigIntView& val) noexcept
{
    const auto decimal = val.InlineDecimal();
    return val.Sign() < 0 ? decimal.substr(1) : decimal;
}

// Whether a magnitude of `digits` decimal digits surely has more than MaxBigIntBits bits, by its length alone
bool SurelyExceedsBitCap(std::string_view magnitude) noexcept
{
    return ((static_cast<double>(magnitude.size()) - 1) * BitsPerDigit) >= static_cast<double>(MaxBigIntBits) + 1;
}

// Whether a magnitude of `digits` decimal digits has more than MaxBigIntBits bits
bool ExceedsBitCap(std::string_view magnitude)
{
    if ((static_cast<double>(magnitude.size()) * BitsPerDigit) <= static_cast<double>(MaxBigIntBits) - 1)
    {
        return false;
    }
    return SurelyExceedsBitCap(magnitude) || MagnitudeBits(magnitude) > MaxBigIntBits;
}

[[noreturn]] void ThrowTooManyBits()
{
    throw ResourceLimitError("integer is wider than " + std::to_string(MaxBigIntBits) + " bits");
}

[[noreturn]] void ThrowStrDigits(const std::string& detail)
{
    throw TemplateRuntimeError("Exceeds the limit (" + std::to_string(MaxIntStrDigits) + " digits) for integer string conversion" + detail + "; use sys.set_int_max_str_digits() to increase the limit");
}
} // namespace

namespace detail
{
IntrusivePtr<BigIntObject> BigIntObject::Make(bool negative, Span<const uint32_t> words, std::string_view decimal)
{
    // Huge exactly when it has words; only a huge integer may lack its decimal form (docs/tasks/0543 P2)
    JINJA2CPP_EXPECTS(decimal.empty() ? !words.empty() : (decimal.front() == '-') == negative);
    JINJA2CPP_EXPECTS(words.empty() || (words[words.size() - 1] != 0 && WordsBits(words) > HugeBigIntBits));
    const bool counted = RenderMemoryCounted();
    if (counted)
    {
        ChargeRenderMemory(Bytes(words.size(), decimal.size()));
    }
    const auto wordBytes = WordBytes(words.size());
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory): owned by the intrusive pointer from here on
    IntrusivePtr<BigIntObject> obj(new (CharCount{ wordBytes + decimal.size() }) BigIntObject(static_cast<uint32_t>(words.size()), static_cast<uint32_t>(decimal.size()), negative));
    if (counted)
    {
        obj->MarkCharged();
    }
    // The words and then the digits follow the object
    char* const parts = obj->Parts();
    if (!words.empty())
    {
        std::memcpy(parts, words.data(), wordBytes);
    }
    std::copy(decimal.begin(), decimal.end(), parts + wordBytes);
    return obj;
}
} // namespace detail

namespace
{
// The bits of `digits` (no leading zero, not empty) in a power-of-two base, exactly
size_t PowerOfTwoDigitsBits(std::string_view digits, int base) noexcept
{
    const auto bitsPerDigit = WordBits(static_cast<uint32_t>(base)) - 1;
    return ((digits.size() - 1) * bitsPerDigit) + WordBits(static_cast<uint32_t>(DigitValue(digits.front())));
}

// The value of a new object: `words` empty unless the integer is huge
InternalValue ObjectValue(bool negative, LimbSpan words, std::string_view decimal)
{
    InternalValue result;
    auto obj = detail::BigIntObject::Make(negative, words, decimal);
    detail::InternalValueAccess::Data(result) = detail::BigIntHandle{ obj.get() };
    detail::InternalValueAccess::ObjectPtr(result) = std::move(obj);
    return result;
}

// The words to keep for a magnitude of `words`: all of them for a huge integer, none for another
LimbSpan HugeWords(LimbSpan words) noexcept
{
    return WordsBits(words) > HugeBigIntBits ? words : LimbSpan();
}

// MakeBigIntValue, past the bit check when the caller has made it. Past MaxIntStrDigits digits
// the integer may be huge: the words that tell it are the ones a huge integer keeps (docs/tasks/0543)
InternalValue MakeBigInt(std::string_view decimal, bool withinBitCap)
{
    int64_t small = 0;
    const char* begin = decimal.data();
    const char* end = begin + decimal.size();
    const auto [ptr, ec] = std::from_chars(begin, end, small);
    if (ec == std::errc() && ptr == end)
    {
        return small;
    }
    const bool negative = decimal.front() == '-';
    const auto magnitude = negative ? decimal.substr(1) : decimal;
    if (magnitude.size() <= MaxIntStrDigits)
    {
        return ObjectValue(negative, {}, decimal);
    }
    if (!withinBitCap && SurelyExceedsBitCap(magnitude))
    {
        ThrowTooManyBits();
    }
    const auto words = BinaryWords(magnitude);
    if (WordsBits(words) > MaxBigIntBits)
    {
        ThrowTooManyBits();
    }
    return ObjectValue(negative, HugeWords(words), decimal);
}
} // namespace

InternalValue MakeBigIntValue(std::string_view decimal)
{
    return MakeBigInt(decimal, false);
}

BigIntView BigIntView::FromText(std::string_view decimal)
{
    // At most MaxIntStrDigits digits is fewer than 14285 bits: never huge (docs/tasks/0543 B2)
    JINJA2CPP_EXPECTS(!decimal.empty() && decimal.size() - (decimal.front() == '-' ? 1 : 0) <= MaxIntStrDigits);
    return BigIntView(nullptr, decimal, decimal.front() == '-');
}

InternalValue BigIntValue(const BigIntView& val)
{
    if (const auto* obj = val.Object())
    {
        InternalValue result;
        detail::InternalValueAccess::Data(result) = detail::BigIntHandle{ obj };
        detail::InternalValueAccess::ObjectPtr(result) = ValueObjectPtr(obj);
        return result;
    }
    return MakeBigIntValue(val.InlineDecimal());
}

InternalValue IntegerFromDigits(std::string_view digits, int base, bool negative)
{
    const auto first = digits.find_first_not_of('0');
    if (first == std::string_view::npos)
    {
        return int64_t{ 0 };
    }
    digits.remove_prefix(first);
    if (base == 10)
    {
        if (digits.size() > MaxIntStrDigits)
        {
            ThrowStrDigits(": value has " + std::to_string(digits.size()) + " digits");
        }
        std::string decimal;
        decimal.reserve(digits.size() + 1);
        if (negative)
        {
            decimal.push_back('-');
        }
        decimal.append(digits);
        return MakeBigIntValue(decimal);
    }
    if (!IsPowerOfTwo(base))
    {
        if (digits.size() > MaxIntStrDigits)
        {
            ThrowStrDigits(": value has " + std::to_string(digits.size()) + " digits");
        }
        // Far below the bit cap
        return MakeBigInt(DecimalFromDigits(digits, base, negative), false);
    }
    // Python sets no digit limit here: the exact width, known before converting, keeps to the cap
    if (PowerOfTwoDigitsBits(digits, base) > MaxBigIntBits)
    {
        ThrowTooManyBits();
    }
    // The digits are the binary form already: a huge integer keeps the words packed from them and
    // no decimal form, which it could never print (docs/tasks/0543 P2); another converts, at most
    // 14285 bits
    const auto words = PackPowerOfTwoDigits(digits, base);
    if (!HugeWords(words).empty())
    {
        return ObjectValue(negative, words, {});
    }
    return MakeBigInt(DecimalFromWords(words, negative), true);
}

BigInt BigInt::FromDigits(std::string_view digits, int base, bool negative)
{
    if (base < 2 || base > 36)
    {
        throw std::invalid_argument("BigInt::FromDigits: base must be 2 to 36");
    }
    if (digits.empty())
    {
        throw std::invalid_argument("BigInt::FromDigits: no digits");
    }
    for (const char ch : digits)
    {
        const bool valid = (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z');
        if (!valid || DigitValue(ch) >= base)
        {
            throw std::invalid_argument("BigInt::FromDigits: invalid digit for the base");
        }
    }
    const auto first = digits.find_first_not_of('0');
    if (first == std::string_view::npos)
    {
        return BigInt();
    }
    digits.remove_prefix(first);
    // Past the width a Value may hold, refuse before converting: exactly in a power-of-two base,
    // else by an estimate that lets a number a few bits too wide through to the exact check below
    const bool exact = IsPowerOfTwo(base);
    if (exact ? PowerOfTwoDigitsBits(digits, base) > MaxBigIntBits : static_cast<double>(digits.size() - 1) * std::log2(base) >= static_cast<double>(MaxBigIntBits))
    {
        throw std::length_error("BigInt::FromDigits: integer is wider than " + std::to_string(MaxBigIntBits) + " bits");
    }
    BigInt result;
    result.m_decimal = DecimalFromDigits(digits, base, negative);
    if (!exact && ExceedsBitCap(negative ? std::string_view(result.m_decimal).substr(1) : std::string_view(result.m_decimal)))
    {
        throw std::length_error("BigInt::FromDigits: integer is wider than " + std::to_string(MaxBigIntBits) + " bits");
    }
    return result;
}

std::string BigInt::ToDigits(int base) const
{
    if (base < 2 || base > 36)
    {
        throw std::invalid_argument("BigInt::ToDigits: base must be 2 to 36");
    }
    std::string_view magnitude = m_decimal;
    if (!magnitude.empty() && magnitude.front() == '-')
    {
        magnitude.remove_prefix(1);
    }
    if (magnitude.empty())
    {
        return "0";
    }
    if (base == 10)
    {
        return std::string(magnitude);
    }
    if (IsPowerOfTwo(base))
    {
        return MagnitudeDigits(magnitude, base);
    }
    // Limbs of base^k, each k digits
    const auto chunk = DigitChunk(base);
    RadixConverter<AnyRadix> converter(LimbBase, AnyRadix{ chunk.value });
    const auto limbs = converter.Convert(ToLimbs(magnitude));
    if (limbs.empty())
    {
        return "0";
    }
    std::string result;
    result.reserve(limbs.size() * chunk.digits);
    std::string piece(chunk.digits, '0');
    for (auto idx = limbs.size(); idx != 0; --idx)
    {
        uint64_t limb = limbs[idx - 1];
        for (auto pos = chunk.digits; pos != 0; --pos)
        {
            piece[pos - 1] = "0123456789abcdefghijklmnopqrstuvwxyz"[limb % static_cast<uint64_t>(base)];
            limb /= static_cast<uint64_t>(base);
        }
        // The top limb without its leading zeros
        const auto skip = idx == limbs.size() ? std::min(piece.find_first_not_of('0'), chunk.digits - 1) : 0;
        result.append(std::string_view(piece).substr(skip));
    }
    return result;
}

InternalValue IntegerFromDouble(double val)
{
    const double whole = std::trunc(val);
    if (whole >= -9223372036854775808.0 && whole < 9223372036854775808.0)
    {
        return static_cast<int64_t>(whole);
    }
    // A double this large is a whole number, which fmt prints exactly
    return MakeBigIntValue(fmt::format("{:.0f}", whole));
}

std::string_view CheckedDecimal(const BigIntView& val)
{
    // A huge integer has more than MaxIntStrDigits digits
    if (val.IsHuge() || InlineMagnitude(val).size() > MaxIntStrDigits)
    {
        ThrowStrDigits("");
    }
    return val.InlineDecimal();
}

std::string_view CheckedMagnitude(const BigIntView& val)
{
    const auto decimal = CheckedDecimal(val);
    return val.Sign() < 0 ? decimal.substr(1) : decimal;
}

std::string MagnitudeInBase(const BigIntView& val, int base)
{
    return val.IsHuge() ? WordsDigits(val.Words(), base) : MagnitudeDigits(InlineMagnitude(val), base);
}

size_t BitLength(const BigIntView& val)
{
    return val.IsHuge() ? WordsBits(val.Words()) : MagnitudeBits(InlineMagnitude(val));
}

size_t BitCount(const BigIntView& val)
{
    return val.IsHuge() ? WordsBitCount(val.Words()) : WordsBitCount(BinaryWords(InlineMagnitude(val)));
}

int CompareBigInts(const BigIntView& left, const BigIntView& right) noexcept
{
    const int leftSign = left.Sign();
    const int rightSign = right.Sign();
    if (leftSign != rightSign)
    {
        return leftSign < rightSign ? -1 : 1;
    }
    const bool leftHuge = left.IsHuge();
    const bool rightHuge = right.IsHuge();
    int magnitude = 0;
    if (leftHuge || rightHuge)
    {
        // A huge integer is wider than any other; two huge ones compare their words from the top
        const auto l = left.Words();
        const auto r = right.Words();
        if (l.size() != r.size())
        {
            magnitude = l.size() < r.size() ? -1 : 1;
        }
        for (auto idx = l.size(); magnitude == 0 && idx != 0; --idx)
        {
            if (l[idx - 1] != r[idx - 1])
            {
                magnitude = l[idx - 1] < r[idx - 1] ? -1 : 1;
            }
        }
        return leftSign < 0 ? -magnitude : magnitude;
    }
    const auto l = InlineMagnitude(left);
    const auto r = InlineMagnitude(right);
    if (l.size() != r.size())
    {
        magnitude = l.size() < r.size() ? -1 : 1;
    }
    else
    {
        const int cmp = l.compare(r);
        magnitude = cmp < 0 ? -1 : (cmp > 0 ? 1 : 0);
    }
    return leftSign < 0 ? -magnitude : magnitude;
}

int CompareBigIntDouble(const BigIntView& left, double right)
{
    if (std::isnan(right))
    {
        return 2;
    }
    if (std::isinf(right))
    {
        return right > 0 ? -1 : 1;
    }
    // A big integer is at least 2^63 in magnitude: a smaller double is on the other side of it
    if (right > -9223372036854775808.0 && right < 9223372036854775808.0)
    {
        return left.Sign();
    }
    // Doubles this large are whole numbers
    const auto text = fmt::format("{:.0f}", right);
    return CompareBigInts(left, BigIntView::FromText(text));
}

double BigIntToDouble(const BigIntView& val)
{
    // 10^309 is past the largest double, and a huge integer is far wider
    if (!val.IsHuge() && InlineMagnitude(val).size() <= 309)
    {
        const auto result = ParseDecimal(val.InlineDecimal());
        if (result && std::isfinite(*result))
        {
            return *result;
        }
    }
    throw IntegerOutOfRangeError("int too large to convert to float");
}

InternalValue RoundIntegerDecimal(std::string_view decimal, int64_t ndigits)
{
    const bool negative = decimal.front() == '-';
    const auto magnitude = negative ? decimal.substr(1) : decimal;
    // Digits kept and the unit 10 ** places they round to
    const auto places = ndigits < -static_cast<int64_t>(magnitude.size()) ? magnitude.size() + 1 : static_cast<size_t>(-ndigits);
    if (places > magnitude.size())
    {
        // Below half a unit
        return int64_t{ 0 };
    }
    std::string quotient(magnitude.substr(0, magnitude.size() - places));
    const auto remainder = magnitude.substr(magnitude.size() - places);
    // remainder <=> half a unit, which is 5 followed by places - 1 zeros
    int half = remainder.front() < '5' ? -1 : (remainder.front() > '5' ? 1 : 0);
    if (half == 0 && remainder.find_first_not_of('0', 1) != std::string_view::npos)
    {
        half = 1;
    }
    const bool quotientOdd = !quotient.empty() && ((quotient.back() - '0') & 1) != 0;
    if (half > 0 || (half == 0 && quotientOdd))
    {
        // Adds one to the decimal quotient
        auto pos = quotient.size();
        while (pos != 0 && quotient[pos - 1] == '9')
        {
            quotient[--pos] = '0';
        }
        if (pos == 0)
        {
            quotient.insert(quotient.begin(), '1');
        }
        else
        {
            ++quotient[pos - 1];
        }
    }
    if (quotient.empty() || quotient.find_first_not_of('0') == std::string::npos)
    {
        return int64_t{ 0 };
    }
    std::string result;
    result.reserve(quotient.size() + places + 1);
    if (negative)
    {
        result.push_back('-');
    }
    result += quotient;
    result.append(places, '0');
    return MakeBigIntValue(result);
}

TransientDecimal::TransientDecimal(const BigIntView& val)
{
    // Only a huge integer lacks its decimal form
    JINJA2CPP_EXPECTS(val.IsHuge() && val.InlineDecimal().empty());
    const auto words = val.Words();
    // The digits (at most bits * log10(2) + 1, and a sign) and the converter's working words, which
    // peak at about 6.2 times the input at MaxBigIntBits (measured with a counting operator new)
    const auto digits = static_cast<std::uint64_t>(static_cast<double>(WordsBits(words)) / BitsPerDigit) + 2;
    const auto bytes = digits + (std::uint64_t{ 7 } * detail::BigIntObject::WordBytes(words.size()));
    ChargeRenderMemory(bytes);
    m_charged = bytes;
    try
    {
        m_text = DecimalFromWords(words, val.Sign() < 0);
    }
    catch (...)
    {
        ReleaseRenderMemory(m_charged);
        throw;
    }
}

TransientDecimal::~TransientDecimal()
{
    ReleaseRenderMemory(m_charged);
}

InternalValue NegateBigInt(const BigIntView& val)
{
    if (val.IsHuge())
    {
        // The same words with the other sign; the decimal form with it, if the integer keeps one
        const auto decimal = val.InlineDecimal();
        if (decimal.empty())
        {
            return ObjectValue(val.Sign() > 0, val.Words(), {});
        }
        std::string negated;
        negated.reserve(decimal.size() + 1);
        if (val.Sign() > 0)
        {
            negated.push_back('-');
            negated.append(decimal);
        }
        else
        {
            negated.append(decimal.substr(1));
        }
        return ObjectValue(val.Sign() > 0, val.Words(), negated);
    }
    return WithDecimal(val, [&val](std::string_view decimal) {
        if (val.Sign() < 0)
        {
            return MakeBigIntValue(decimal.substr(1));
        }
        std::string negated;
        negated.reserve(decimal.size() + 1);
        negated.push_back('-');
        negated.append(decimal);
        return MakeBigIntValue(negated);
    });
}

void ThrowBigIntArithmetic()
{
    throw IntegerOutOfRangeError("integer result does not fit in 64 bits");
}

void ThrowIndexOverflow()
{
    throw IntegerOutOfRangeError("cannot fit 'int' into an index-sized integer");
}

bool IsUsableBigIntBackend(const BigIntBackend& backend) noexcept
{
    // Version 1 ends with divideTruncated: a larger table is a later version's, read as far as v1 goes
    constexpr auto v1Size = static_cast<uint32_t>(offsetof(BigIntBackend, divideTruncated) + sizeof(backend.divideTruncated));
    if (backend.structSize < v1Size || backend.version != 1 || !backend.name)
    {
        return false;
    }
    if (backend.objectSize == 0 || backend.objectSize > MaxBigIntObjectSize || backend.objectAlign == 0 || backend.objectAlign > MaxBigIntObjectAlign || (backend.objectAlign & (backend.objectAlign - 1)) != 0)
    {
        return false;
    }
    return backend.destroy != nullptr && backend.fromInt64 != nullptr && backend.fromDecimal != nullptr && backend.toInt64 != nullptr && backend.toDecimal != nullptr && backend.sign != nullptr && backend.compare != nullptr && backend.bitLength != nullptr && backend.negate != nullptr && backend.add != nullptr && backend.subtract != nullptr && backend.multiply != nullptr && backend.divideTruncated != nullptr;
}

namespace
{
// One backend object in a stack slot, destroyed with the slot
class BackendObject
{
public:
    explicit BackendObject(const BigIntBackend& backend) noexcept
        : m_backend(&backend)
    {
    }
    ~BackendObject()
    {
        if (m_madeBy)
        {
            m_madeBy->destroy(m_madeBy, m_storage.data());
        }
    }
    BackendObject(const BackendObject&) = delete;
    BackendObject(BackendObject&&) = delete;
    BackendObject& operator=(const BackendObject&) = delete;
    BackendObject& operator=(BackendObject&&) = delete;

    // Storage for a call that makes the object; null once made, which no call accepts
    void* Out() noexcept // NOLINT(readability-make-member-function-const): the caller makes the object in it
    {
        assert(!m_madeBy);
        return m_storage.data();
    }
    // Records what the call that got Out() reported
    void Made(BigIntStatus status);
    [[nodiscard]] const void* Get() const noexcept
    {
        assert(m_madeBy);
        return m_storage.data();
    }
    [[nodiscard]] bool IsMade() const noexcept { return m_madeBy != nullptr; }

private:
    // The slot first and two pointers after it: no padding, which MSVC would warn about (C4324)
    alignas(MaxBigIntObjectAlign) std::array<unsigned char, MaxBigIntObjectSize> m_storage{};
    const BigIntBackend* m_backend;
    // The backend once the object is made, which destroys it; null before
    const BigIntBackend* m_madeBy = nullptr;
};

[[noreturn]] void ThrowBackendFailure(const BigIntBackend& backend, BigIntStatus status)
{
    switch (status)
    {
    case BigIntStatus::OutOfMemory:
        throw std::bad_alloc();
    case BigIntStatus::TooLarge:
        ThrowTooManyBits();
    case BigIntStatus::DivisionByZero:
        throw TemplateRuntimeError("integer division or modulo by zero");
    default:
        break;
    }
    throw TemplateRuntimeError(std::string("big-integer backend '") + backend.name + "' failed (status " + std::to_string(static_cast<int>(status)) + ")");
}

void BackendObject::Made(BigIntStatus status)
{
    if (status != BigIntStatus::Ok)
    {
        ThrowBackendFailure(*m_backend, status);
    }
    m_madeBy = m_backend;
}

void Load(const BigIntBackend& backend, BackendObject& obj, const IntOperand& val)
{
    if (!val.big)
    {
        obj.Made(backend.fromInt64(&backend, obj.Out(), val.small));
    }
    else
    {
        obj.Made(WithDecimal(*val.big, [&backend, &obj](std::string_view decimal) { return backend.fromDecimal(&backend, obj.Out(), decimal.data(), decimal.size()); }));
    }
}

// The value of a computed object: an int when it fits int64
InternalValue ToValue(const BigIntBackend& backend, const BackendObject& obj)
{
    int64_t small = 0;
    if (backend.toInt64(&backend, obj.Get(), &small))
    {
        return small;
    }
    if (backend.bitLength(&backend, obj.Get()) > MaxBigIntBits)
    {
        ThrowTooManyBits();
    }
    // The sink runs inside the backend's noexcept call, so a failed append is recorded rather than thrown through it
    struct Sink
    {
        std::string decimal;
        bool outOfMemory = false;
    } sink;
    const auto status = backend.toDecimal(
        &backend,
        obj.Get(),
        [](void* ctx, const char* data, size_t size) {
            auto* const out = static_cast<Sink*>(ctx);
            try
            {
                out->decimal.append(data, size);
            }
            catch (const std::bad_alloc&)
            {
                out->outOfMemory = true;
            }
        },
        &sink);
    if (sink.outOfMemory)
    {
        throw std::bad_alloc();
    }
    if (status != BigIntStatus::Ok)
    {
        ThrowBackendFailure(backend, status);
    }
    // Within the cap by the backend's own width, which spares converting the text back to count it
    return MakeBigInt(sink.decimal, true);
}

int Sign(const IntOperand& val) noexcept
{
    if (val.big)
    {
        return val.big->Sign();
    }
    return val.small < 0 ? -1 : (val.small > 0 ? 1 : 0);
}

double ToDouble(const IntOperand& val)
{
    return val.big ? BigIntToDouble(*val.big) : static_cast<double>(val.small);
}

bool IsOdd(const IntOperand& val) noexcept
{
    return val.big ? IsOdd(*val.big) : (val.small & 1) != 0;
}

// a - b < 0 or the like: the floored result from a truncated one. Python's floor division and
// modulo round the quotient down, and the remainder takes the sign of the divisor
InternalValue FloorDivide(const BigIntBackend& backend, const BackendObject& left, const BackendObject& right, bool wantRemainder)
{
    BackendObject quot(backend);
    BackendObject rem(backend);
    const auto status = backend.divideTruncated(&backend, quot.Out(), rem.Out(), left.Get(), right.Get());
    quot.Made(status);
    rem.Made(status);
    const int remSign = backend.sign(&backend, rem.Get());
    if (remSign == 0 || remSign == backend.sign(&backend, right.Get()))
    {
        return ToValue(backend, wantRemainder ? rem : quot);
    }
    BackendObject fixed(backend);
    if (wantRemainder)
    {
        fixed.Made(backend.add(&backend, fixed.Out(), rem.Get(), right.Get()));
    }
    else
    {
        BackendObject one(backend);
        one.Made(backend.fromInt64(&backend, one.Out(), 1));
        fixed.Made(backend.subtract(&backend, fixed.Out(), quot.Get(), one.Get()));
    }
    return ToValue(backend, fixed);
}

static_assert(detail::RenderLimits{}.maxIntBits == MaxBigIntBits);

// The bits a product or power may have: MaxBigIntBits, or the Load's fold budget (docs/tasks/0569)
size_t IntBitsCap()
{
    return detail::t_renderLimits.maxIntBits;
}

// The bits of a product of factors of `leftBits` and `rightBits` bits: at least their sum less one
void CheckProductBits(size_t leftBits, size_t rightBits)
{
    if (leftBits != 0 && rightBits != 0 && leftBits + rightBits - 1 > IntBitsCap())
    {
        ThrowTooManyBits();
    }
}

InternalValue Power(const BigIntBackend* backend, const IntOperand& base, const IntOperand& exp)
{
    const int expSign = Sign(exp);
    if (expSign < 0)
    {
        // As Python: a float, through float(base) ** float(exp)
        if (Sign(base) == 0)
        {
            throw TemplateRuntimeError("0.0 cannot be raised to a negative power");
        }
        return std::pow(ToDouble(base), ToDouble(exp));
    }
    if (!base.big && (base.small == 0 || base.small == 1 || base.small == -1))
    {
        if (expSign == 0)
        {
            return int64_t{ 1 };
        }
        return base.small == -1 && !IsOdd(exp) ? int64_t{ 1 } : base.small;
    }
    if (!backend)
    {
        ThrowBigIntArithmetic();
    }
    // |base| >= 2: the result has at least (bits(base) - 1) * exp + 1 bits
    BackendObject acc(*backend);
    Load(*backend, acc, base);
    const size_t baseBits = backend->bitLength(backend, acc.Get());
    if (exp.big || (baseBits - 1) * static_cast<uint64_t>(exp.small) >= IntBitsCap())
    {
        ThrowTooManyBits();
    }
    // Square and multiply from the low bit: results[r] holds the product so far and `current` is
    // base ** 2^k. Each step makes the next object in the other slot of a pair, then lets go of the old one
    auto bits = static_cast<uint64_t>(exp.small);
    std::array<std::optional<BackendObject>, 2> results;
    std::array<std::optional<BackendObject>, 2> squares;
    size_t r = 0;
    size_t q = 0;
    BackendObject* result = &results[r].emplace(*backend);
    result->Made(backend->fromInt64(backend, result->Out(), 1));
    const void* current = acc.Get();
    while (bits != 0)
    {
        if ((bits & 1) != 0)
        {
            CheckProductBits(backend->bitLength(backend, result->Get()), backend->bitLength(backend, current));
            auto& next = results[1 - r].emplace(*backend);
            next.Made(backend->multiply(backend, next.Out(), result->Get(), current));
            results[r].reset();
            r = 1 - r;
            result = &next;
        }
        bits >>= 1;
        if (bits != 0)
        {
            const size_t currentBits = backend->bitLength(backend, current);
            CheckProductBits(currentBits, currentBits);
            auto& next = squares[1 - q].emplace(*backend);
            next.Made(backend->multiply(backend, next.Out(), current, current));
            squares[q].reset();
            q = 1 - q;
            current = next.Get();
        }
    }
    return ToValue(*backend, *result);
}
} // namespace

InternalValue BigIntBinary(BigIntOp op, IntOperand left, IntOperand right)
{
    const BigIntBackend* backend = detail::t_renderLimits.bigInts;
    if (op == BigIntOp::Power)
    {
        return Power(backend, left, right);
    }
    if (!backend)
    {
        ThrowBigIntArithmetic();
    }
    BackendObject lhs(*backend);
    Load(*backend, lhs, left);
    BackendObject rhs(*backend);
    Load(*backend, rhs, right);
    BackendObject result(*backend);
    switch (op)
    {
    case BigIntOp::Add:
        result.Made(backend->add(backend, result.Out(), lhs.Get(), rhs.Get()));
        break;
    case BigIntOp::Subtract:
        result.Made(backend->subtract(backend, result.Out(), lhs.Get(), rhs.Get()));
        break;
    case BigIntOp::Multiply:
        CheckProductBits(backend->bitLength(backend, lhs.Get()), backend->bitLength(backend, rhs.Get()));
        result.Made(backend->multiply(backend, result.Out(), lhs.Get(), rhs.Get()));
        break;
    case BigIntOp::FloorDivide:
    case BigIntOp::Remainder:
        return FloorDivide(*backend, lhs, rhs, op == BigIntOp::Remainder);
    case BigIntOp::Power:
        break;
    }
    return ToValue(*backend, result);
}
} // namespace v2
} // namespace jinja2
