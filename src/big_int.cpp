#include "big_int.h"

#include "decimal_number.h"
#include "engine_errors.h"
#include "internal_value.h"
#include "intrusive_ptr.h"
#include "recursion_guard.h"

#include <jinja2cpp/big_int_backend.h>

#include <fmt/format.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <charconv>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <new>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace jinja2
{
inline namespace v2
{
namespace detail
{
IntrusivePtr<BigIntObject> BigIntObject::Make(std::string_view decimal)
{
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory): owned by the intrusive pointer from here on
    IntrusivePtr<BigIntObject> obj(new (CharCount{ decimal.size() }) BigIntObject(decimal.size()));
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast): the digits follow the object
    std::copy(decimal.begin(), decimal.end(), reinterpret_cast<char*>(obj.get()) + sizeof(BigIntObject));
    return obj;
}
} // namespace detail

namespace
{
// A magnitude in base 10^9, least significant limb first: the core's only arithmetic, used to
// change bases. Every conversion is quadratic in the length, which MaxBigIntBits bounds
constexpr uint32_t LimbBase = 1000000000;
constexpr size_t LimbDigits = 9;
using Limbs = std::vector<uint32_t>;

// log2(10): the bits of a decimal digit
constexpr double BitsPerDigit = 3.321928094887362;

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

// limbs = limbs * mult + add, with mult * LimbBase within 64 bits
void MulAdd(Limbs& limbs, uint64_t mult, uint64_t add)
{
    uint64_t carry = add;
    for (auto& limb : limbs)
    {
        const uint64_t cur = (static_cast<uint64_t>(limb) * mult) + carry;
        limb = static_cast<uint32_t>(cur % LimbBase);
        carry = cur / LimbBase;
    }
    while (carry != 0)
    {
        limbs.push_back(static_cast<uint32_t>(carry % LimbBase));
        carry /= LimbBase;
    }
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

bool IsPowerOfTwo(int base)
{
    return (base & (base - 1)) == 0;
}

// Whether a magnitude of `digits` decimal digits has more than MaxBigIntBits bits
bool ExceedsBitCap(std::string_view magnitude)
{
    const auto digits = static_cast<double>(magnitude.size());
    if ((digits * BitsPerDigit) <= static_cast<double>(MaxBigIntBits) - 1)
    {
        return false;
    }
    if (((digits - 1) * BitsPerDigit) >= static_cast<double>(MaxBigIntBits) + 1)
    {
        return true;
    }
    return BitLength(BigIntView{ magnitude }) > MaxBigIntBits;
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

InternalValue MakeBigIntValue(std::string_view decimal)
{
    int64_t small = 0;
    const char* begin = decimal.data();
    const char* end = begin + decimal.size();
    const auto [ptr, ec] = std::from_chars(begin, end, small);
    if (ec == std::errc() && ptr == end)
    {
        return small;
    }
    if (ExceedsBitCap(decimal.front() == '-' ? decimal.substr(1) : decimal))
    {
        ThrowTooManyBits();
    }
    InternalValue result;
    auto obj = detail::BigIntObject::Make(decimal);
    detail::InternalValueAccess::Data(result) = detail::BigIntHandle{ obj.get() };
    detail::InternalValueAccess::ObjectPtr(result) = std::move(obj);
    return result;
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
    if (!IsPowerOfTwo(base) && digits.size() > MaxIntStrDigits)
    {
        ThrowStrDigits(": value has " + std::to_string(digits.size()) + " digits");
    }
    // Rejected before converting, which is quadratic in the length
    if (static_cast<double>(digits.size() - 1) * std::log2(base) >= static_cast<double>(MaxBigIntBits))
    {
        ThrowTooManyBits();
    }
    // As many digits at a time as fit 32 bits
    auto chunkMult = static_cast<uint64_t>(base);
    size_t chunkSize = 1;
    while (chunkMult * static_cast<uint64_t>(base) <= std::numeric_limits<uint32_t>::max())
    {
        chunkMult *= static_cast<uint64_t>(base);
        ++chunkSize;
    }
    Limbs limbs;
    limbs.reserve(static_cast<size_t>(static_cast<double>(digits.size()) * std::log10(base) / LimbDigits) + 2);
    size_t pos = 0;
    // The first chunk takes the remainder, so that every later one is full
    size_t take = digits.size() % chunkSize == 0 ? chunkSize : digits.size() % chunkSize;
    while (pos != digits.size())
    {
        uint64_t chunk = 0;
        uint64_t mult = 1;
        for (size_t idx = 0; idx != take; ++idx)
        {
            chunk = (chunk * static_cast<uint64_t>(base)) + static_cast<uint64_t>(DigitValue(digits[pos + idx]));
            mult *= static_cast<uint64_t>(base);
        }
        MulAdd(limbs, mult, chunk);
        pos += take;
        take = chunkSize;
    }
    return MakeBigIntValue(FromLimbs(limbs, negative));
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
    if (val.Magnitude().size() > MaxIntStrDigits)
    {
        ThrowStrDigits("");
    }
    return val.decimal;
}

std::string MagnitudeInBase(const BigIntView& val, int base)
{
    // Binary words, least significant first, built from the decimal limbs most significant first
    const auto limbs = ToLimbs(val.Magnitude());
    std::vector<uint32_t> words;
    words.reserve((static_cast<size_t>(static_cast<double>(val.Magnitude().size()) * BitsPerDigit) / 32) + 2);
    for (auto idx = limbs.size(); idx != 0; --idx)
    {
        uint64_t carry = limbs[idx - 1];
        for (auto& word : words)
        {
            const uint64_t cur = (static_cast<uint64_t>(word) * LimbBase) + carry;
            word = static_cast<uint32_t>(cur);
            carry = cur >> 32;
        }
        if (carry != 0)
        {
            words.push_back(static_cast<uint32_t>(carry));
        }
    }
    const int bitsPerDigit = base == 2 ? 1 : (base == 8 ? 3 : 4);
    const auto totalBits = words.size() * 32;
    std::string result;
    result.reserve((totalBits / static_cast<size_t>(bitsPerDigit)) + 1);
    // Digits from the least significant bit up, then reversed
    for (size_t bit = 0; bit < totalBits; bit += static_cast<size_t>(bitsPerDigit))
    {
        uint32_t digit = 0;
        for (int idx = 0; idx != bitsPerDigit; ++idx)
        {
            const auto pos = bit + static_cast<size_t>(idx);
            if (pos < totalBits && ((words[pos / 32] >> (pos % 32)) & 1U) != 0)
            {
                digit |= 1U << idx;
            }
        }
        result.push_back("0123456789abcdef"[digit]);
    }
    while (result.size() > 1 && result.back() == '0')
    {
        result.pop_back();
    }
    std::reverse(result.begin(), result.end());
    return result;
}

size_t BitLength(const BigIntView& val)
{
    const auto binary = MagnitudeInBase(val, 2);
    return binary == "0" ? 0 : binary.size();
}

int CompareBigInts(const BigIntView& left, const BigIntView& right) noexcept
{
    const int leftSign = left.Sign();
    const int rightSign = right.Sign();
    if (leftSign != rightSign)
    {
        return leftSign < rightSign ? -1 : 1;
    }
    const auto l = left.Magnitude();
    const auto r = right.Magnitude();
    int magnitude = 0;
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
    return CompareBigInts(left, BigIntView{ text });
}

double BigIntToDouble(const BigIntView& val)
{
    // 10^309 is past the largest double
    if (val.Magnitude().size() <= 309)
    {
        const auto result = ParseDecimal(val.decimal);
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

InternalValue NegateBigInt(const BigIntView& val)
{
    if (val.Sign() < 0)
    {
        return MakeBigIntValue(val.Magnitude());
    }
    std::string negated;
    negated.reserve(val.decimal.size() + 1);
    negated.push_back('-');
    negated.append(val.decimal);
    return MakeBigIntValue(negated);
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
    if (val.decimal.empty())
    {
        obj.Made(backend.fromInt64(&backend, obj.Out(), val.small));
    }
    else
    {
        obj.Made(backend.fromDecimal(&backend, obj.Out(), val.decimal.data(), val.decimal.size()));
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
    return MakeBigIntValue(sink.decimal);
}

int Sign(const IntOperand& val) noexcept
{
    if (!val.decimal.empty())
    {
        return val.decimal.front() == '-' ? -1 : 1;
    }
    return val.small < 0 ? -1 : (val.small > 0 ? 1 : 0);
}

double ToDouble(const IntOperand& val)
{
    return val.decimal.empty() ? static_cast<double>(val.small) : BigIntToDouble(BigIntView{ val.decimal });
}

bool IsOdd(const IntOperand& val) noexcept
{
    return val.decimal.empty() ? (val.small & 1) != 0 : IsOdd(BigIntView{ val.decimal });
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

// The bits of a product of factors of `leftBits` and `rightBits` bits: at least their sum less one
void CheckProductBits(size_t leftBits, size_t rightBits)
{
    if (leftBits != 0 && rightBits != 0 && leftBits + rightBits - 1 > MaxBigIntBits)
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
    if (base.decimal.empty() && (base.small == 0 || base.small == 1 || base.small == -1))
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
    if (!exp.decimal.empty() || (baseBits - 1) * static_cast<uint64_t>(exp.small) >= MaxBigIntBits)
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
