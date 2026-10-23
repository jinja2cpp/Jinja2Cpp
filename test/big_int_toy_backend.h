#ifndef JINJA2CPP_TEST_BIG_INT_TOY_BACKEND_H
#define JINJA2CPP_TEST_BIG_INT_TOY_BACKEND_H

#include <jinja2cpp/big_int_backend.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <new>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

// A big-integer backend over decimal strings, for tests only (docs/tasks/0271): slow, short and
// with no dependency, so every CI row tests the BigIntBackend interface as a user implements it.
// Counts its live objects, so a test can check that the library destroys each one it makes
namespace big_int_toy
{
struct ToyInt
{
    bool negative = false;
    std::string magnitude; // decimal digits, no leading zeros; "0" for zero
};

inline int& LiveObjects()
{
    static int count = 0;
    return count;
}

// Compares two magnitudes
inline int CompareMagnitudes(const std::string& a, const std::string& b)
{
    if (a.size() != b.size())
    {
        return a.size() < b.size() ? -1 : 1;
    }
    const int cmp = a.compare(b);
    return cmp < 0 ? -1 : (cmp > 0 ? 1 : 0);
}

inline std::string Trim(std::string digits)
{
    const auto first = digits.find_first_not_of('0');
    return first == std::string::npos ? std::string("0") : digits.substr(first);
}

inline std::string AddMagnitudes(const std::string& a, const std::string& b)
{
    std::string result;
    int carry = 0;
    for (size_t idx = 0; idx < std::max(a.size(), b.size()) || carry != 0; ++idx)
    {
        int sum = carry;
        sum += idx < a.size() ? a[a.size() - 1 - idx] - '0' : 0;
        sum += idx < b.size() ? b[b.size() - 1 - idx] - '0' : 0;
        result.push_back(static_cast<char>('0' + sum % 10));
        carry = sum / 10;
    }
    std::reverse(result.begin(), result.end());
    return Trim(std::move(result));
}

// a - b for a >= b
inline std::string SubtractMagnitudes(const std::string& a, const std::string& b)
{
    std::string result;
    int borrow = 0;
    for (size_t idx = 0; idx < a.size(); ++idx)
    {
        int diff = (a[a.size() - 1 - idx] - '0') - borrow - (idx < b.size() ? b[b.size() - 1 - idx] - '0' : 0);
        borrow = diff < 0 ? 1 : 0;
        result.push_back(static_cast<char>('0' + diff + borrow * 10));
    }
    std::reverse(result.begin(), result.end());
    return Trim(std::move(result));
}

inline std::string MultiplyMagnitudes(const std::string& a, const std::string& b)
{
    std::string digits(a.size() + b.size(), '\0');
    for (size_t i = a.size(); i-- != 0;)
    {
        int carry = 0;
        for (size_t j = b.size(); j-- != 0;)
        {
            const int cur = digits[i + j + 1] + (a[i] - '0') * (b[j] - '0') + carry;
            digits[i + j + 1] = static_cast<char>(cur % 10);
            carry = cur / 10;
        }
        digits[i] = static_cast<char>(digits[i] + carry);
    }
    for (auto& ch : digits)
    {
        ch = static_cast<char>(ch + '0');
    }
    return Trim(std::move(digits));
}

// Long division of magnitudes: {quotient, remainder}
inline std::pair<std::string, std::string> DivideMagnitudes(const std::string& a, const std::string& b)
{
    std::string quotient;
    std::string rem = "0";
    for (const char digit : a)
    {
        rem.push_back(digit);
        rem = Trim(std::move(rem));
        int count = 0;
        while (CompareMagnitudes(rem, b) >= 0)
        {
            rem = SubtractMagnitudes(rem, b);
            ++count;
        }
        quotient.push_back(static_cast<char>('0' + count));
    }
    return { Trim(std::move(quotient)), rem };
}

inline const ToyInt& Get(const void* obj)
{
    return *static_cast<const ToyInt*>(obj);
}

inline jinja2::BigIntStatus Make(void* out, bool negative, std::string magnitude)
{
    ++LiveObjects();
    new (out) ToyInt{ negative && magnitude != "0", std::move(magnitude) };
    return jinja2::BigIntStatus::Ok;
}

inline jinja2::BigIntStatus Sum(void* out, const ToyInt& a, bool negateB, const ToyInt& b)
{
    const bool bNegative = b.negative != negateB && b.magnitude != "0";
    if (a.negative == bNegative)
    {
        return Make(out, a.negative, AddMagnitudes(a.magnitude, b.magnitude));
    }
    if (CompareMagnitudes(a.magnitude, b.magnitude) >= 0)
    {
        return Make(out, a.negative, SubtractMagnitudes(a.magnitude, b.magnitude));
    }
    return Make(out, bNegative, SubtractMagnitudes(b.magnitude, a.magnitude));
}

inline void Destroy(const jinja2::BigIntBackend*, void* obj) noexcept
{
    --LiveObjects();
    static_cast<ToyInt*>(obj)->~ToyInt();
}

inline jinja2::BigIntStatus FromInt64(const jinja2::BigIntBackend*, void* out, int64_t value) noexcept
{
    // The magnitude of INT64_MIN does not fit int64
    const auto magnitude = value < 0 ? 0 - static_cast<uint64_t>(value) : static_cast<uint64_t>(value);
    return Make(out, value < 0, std::to_string(magnitude));
}

inline jinja2::BigIntStatus FromDecimal(const jinja2::BigIntBackend*, void* out, const char* digits, size_t size) noexcept
{
    std::string_view text(digits, size);
    const bool negative = !text.empty() && text.front() == '-';
    if (negative)
    {
        text.remove_prefix(1);
    }
    if (text.empty() || text.find_first_not_of("0123456789") != std::string_view::npos)
    {
        return jinja2::BigIntStatus::BadInput;
    }
    return Make(out, negative, Trim(std::string(text)));
}

inline bool ToInt64(const jinja2::BigIntBackend*, const void* obj, int64_t* out) noexcept
{
    const auto& val = Get(obj);
    const std::string limit = val.negative ? "9223372036854775808" : "9223372036854775807";
    if (CompareMagnitudes(val.magnitude, limit) > 0)
    {
        return false;
    }
    const auto magnitude = std::stoull(val.magnitude);
    *out = val.negative ? static_cast<int64_t>(0 - magnitude) : static_cast<int64_t>(magnitude);
    return true;
}

inline jinja2::BigIntStatus ToDecimal(const jinja2::BigIntBackend*, const void* obj, void (*sink)(void*, const char*, size_t), void* ctx) noexcept
{
    const auto& val = Get(obj);
    if (val.negative)
    {
        sink(ctx, "-", 1);
    }
    sink(ctx, val.magnitude.data(), val.magnitude.size());
    return jinja2::BigIntStatus::Ok;
}

inline int Sign(const jinja2::BigIntBackend*, const void* obj) noexcept
{
    const auto& val = Get(obj);
    return val.magnitude == "0" ? 0 : (val.negative ? -1 : 1);
}

inline int Compare(const jinja2::BigIntBackend*, const void* a, const void* b) noexcept
{
    const auto& x = Get(a);
    const auto& y = Get(b);
    if (x.negative != y.negative)
    {
        return x.negative ? -1 : 1;
    }
    const int cmp = CompareMagnitudes(x.magnitude, y.magnitude);
    return x.negative ? -cmp : cmp;
}

inline size_t BitLength(const jinja2::BigIntBackend*, const void* obj) noexcept
{
    // Through base-2^32 limbs, nine decimal digits at a time
    const std::string& digits = Get(obj).magnitude;
    std::vector<uint32_t> limbs;
    size_t pos = 0;
    while (pos != digits.size())
    {
        const size_t take = std::min<size_t>(9, digits.size() - pos);
        uint64_t mult = 1;
        uint64_t carry = 0;
        for (size_t idx = 0; idx != take; ++idx)
        {
            mult *= 10;
            carry = carry * 10 + static_cast<uint64_t>(digits[pos + idx] - '0');
        }
        pos += take;
        for (auto& limb : limbs)
        {
            const uint64_t cur = limb * mult + carry;
            limb = static_cast<uint32_t>(cur);
            carry = cur >> 32;
        }
        if (carry != 0)
        {
            limbs.push_back(static_cast<uint32_t>(carry));
        }
    }
    if (limbs.empty())
    {
        return size_t{ 0 };
    }
    size_t bits = (limbs.size() - 1) * 32;
    for (uint32_t top = limbs.back(); top != 0; top >>= 1)
    {
        ++bits;
    }
    return bits;
}

inline jinja2::BigIntStatus Negate(const jinja2::BigIntBackend*, void* out, const void* x) noexcept
{
    return Make(out, !Get(x).negative, Get(x).magnitude);
}

inline jinja2::BigIntStatus Add(const jinja2::BigIntBackend*, void* out, const void* a, const void* b) noexcept
{
    return Sum(out, Get(a), false, Get(b));
}

inline jinja2::BigIntStatus Subtract(const jinja2::BigIntBackend*, void* out, const void* a, const void* b) noexcept
{
    return Sum(out, Get(a), true, Get(b));
}

inline jinja2::BigIntStatus Multiply(const jinja2::BigIntBackend*, void* out, const void* a, const void* b) noexcept
{
    return Make(out, Get(a).negative != Get(b).negative, MultiplyMagnitudes(Get(a).magnitude, Get(b).magnitude));
}

inline jinja2::BigIntStatus DivideTruncated(const jinja2::BigIntBackend*, void* quotient, void* remainder, const void* a, const void* b) noexcept
{
    const auto& x = Get(a);
    const auto& y = Get(b);
    if (y.magnitude == "0")
    {
        return jinja2::BigIntStatus::DivisionByZero;
    }
    auto [quot, rem] = DivideMagnitudes(x.magnitude, y.magnitude);
    if (quotient)
    {
        Make(quotient, x.negative != y.negative, std::move(quot));
    }
    if (remainder)
    {
        Make(remainder, x.negative, std::move(rem));
    }
    return jinja2::BigIntStatus::Ok;
}

inline jinja2::BigIntBackend MakeBackend()
{
    using jinja2::BigIntBackend;
    BigIntBackend backend{};
    backend.structSize = sizeof(BigIntBackend);
    backend.version = 1;
    backend.name = "toy-decimal";
    backend.objectSize = sizeof(ToyInt);
    backend.objectAlign = alignof(ToyInt);
    backend.destroy = Destroy;
    backend.fromInt64 = FromInt64;
    backend.fromDecimal = FromDecimal;
    backend.toInt64 = ToInt64;
    backend.toDecimal = ToDecimal;
    backend.sign = Sign;
    backend.compare = Compare;
    backend.bitLength = BitLength;
    backend.negate = Negate;
    backend.add = Add;
    backend.subtract = Subtract;
    backend.multiply = Multiply;
    backend.divideTruncated = DivideTruncated;
    return backend;
}

// The backend, in static storage as the interface asks
inline const jinja2::BigIntBackend* Backend()
{
    static const jinja2::BigIntBackend backend = MakeBackend();
    return &backend;
}
} // namespace big_int_toy

#endif // JINJA2CPP_TEST_BIG_INT_TOY_BACKEND_H
