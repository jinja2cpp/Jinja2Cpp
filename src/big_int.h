#ifndef JINJA2CPP_SRC_BIG_INT_H
#define JINJA2CPP_SRC_BIG_INT_H

#include "internal_value.h"

#include <jinja2cpp/big_int_backend.h>
#include <jinja2cpp/config.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace jinja2
{
inline namespace v2
{
// Integers outside int64 in their text form (docs/tasks/0270, docs/design/0178-big-integer-plan.md
// 3.3): the core keeps such an integer as its canonical decimal form, and a huge one (past
// HugeBigIntBits) as binary words, with its decimal form only when it was made from decimal
// text (docs/tasks/0543), and does everything that needs only the
// representation (printing, comparison, conversion to float and between bases, bit operations).
// Arithmetic that needs a big result raises OverflowError (IntegerOutOfRangeError).
//
// Every function here is out of line and off the int64 paths: they run only once a value is big.
// Inline code in headers (value_visitors.h) calls them, so the shared library exports them.

// The widest integer a value may hold, 128 KiB of digits (plan Q6): ResourceLimitExceeded past it
constexpr size_t MaxBigIntBits = size_t{ 1 } << 20;
// MaxIntStrDigits, CPython's limit on converting an integer to or from decimal text, is in
// internal_value.h, which caps a BigIntView of text at it

// The integer whose canonical decimal form is `decimal` ("-" and digits, no leading zero): an
// int when it fits int64, else a big integer. Throws ResourceLimitError past MaxBigIntBits
JINJA2CPP_EXPORT InternalValue MakeBigIntValue(std::string_view decimal);

// The viewed integer as a value: the view's object shared, a new one only for a view of text
JINJA2CPP_EXPORT InternalValue BigIntValue(const BigIntView& val);

// The canonical decimal form of a huge integer that keeps only its binary words, built for one
// WithDecimal call. While it lives it counts against the running render's memory budget (its
// digits and the converter's peak working words, 7 times the input words): ResourceLimitError
// before converting when they do not fit (docs/tasks/0543 P2)
class TransientDecimal
{
public:
    JINJA2CPP_EXPORT explicit TransientDecimal(const BigIntView& val);
    JINJA2CPP_EXPORT ~TransientDecimal();
    TransientDecimal(const TransientDecimal&) = delete;
    TransientDecimal(TransientDecimal&&) = delete;
    TransientDecimal& operator=(const TransientDecimal&) = delete;
    TransientDecimal& operator=(TransientDecimal&&) = delete;

    [[nodiscard]] std::string_view Text() const noexcept { return m_text; }

private:
    std::uint64_t m_charged = 0;
    std::string m_text;
};

// Calls `fn` with the canonical decimal form of `val` ("-" and digits) and returns its result.
// The text lives only for the call: a huge integer made from its binary form has no decimal
// one and builds it for each call (docs/tasks/0543 Q3, about 0.1 s at MaxBigIntBits), so the
// text cannot escape. Only paths that need every digit use it: a host BigInt, a backend's
// fromDecimal, round(x, -n)
template<typename Fn>
decltype(auto) WithDecimal(const BigIntView& val, Fn&& fn)
{
    if (val.InlineDecimal().empty())
    {
        const TransientDecimal decimal(val);
        return std::forward<Fn>(fn)(decimal.Text());
    }
    return std::forward<Fn>(fn)(val.InlineDecimal());
}

// Python's int(text, base) for digits already checked to be valid in `base` (2-36, no sign, no
// separators, at least one digit). A base that is not a power of two raises ValueError past
// MaxIntStrDigits, as CPython does
JINJA2CPP_EXPORT InternalValue IntegerFromDigits(std::string_view digits, int base, bool negative);

// Python's int(x) of a finite double: its whole part, exactly
JINJA2CPP_EXPORT InternalValue IntegerFromDouble(double val);

// The decimal form for str() and repr(): ValueError past MaxIntStrDigits, as CPython raises
JINJA2CPP_EXPORT std::string_view CheckedDecimal(const BigIntView& val);
// The same without the sign: the decimal digits of |val|
JINJA2CPP_EXPORT std::string_view CheckedMagnitude(const BigIntView& val);

// The digits of |val| in a power-of-two base (2 to 32), lower case; packed from the words of a huge integer
JINJA2CPP_EXPORT std::string MagnitudeInBase(const BigIntView& val, int base);

// Python's int.bit_length(): the bits of |val|, from the words of a huge integer without converting
JINJA2CPP_EXPORT size_t BitLength(const BigIntView& val);
// Python's int.bit_count(): the set bits of |val|
JINJA2CPP_EXPORT size_t BitCount(const BigIntView& val);

// Three-way comparisons: -1, 0 or 1, and 2 when `right` is NaN (unordered). Python compares
// an int and a float exactly. Two huge integers compare their words
JINJA2CPP_EXPORT int CompareBigInts(const BigIntView& left, const BigIntView& right) noexcept;
JINJA2CPP_EXPORT int CompareBigIntDouble(const BigIntView& left, double right);

// Python's float(x): correctly rounded; OverflowError when it is out of a double's range
JINJA2CPP_EXPORT double BigIntToDouble(const BigIntView& val);

// Python's round(x, ndigits) of an integer for ndigits < 0, x in decimal: half to even at
// 10 ** -ndigits. Only digits move, so it needs no arithmetic
JINJA2CPP_EXPORT InternalValue RoundIntegerDecimal(std::string_view decimal, int64_t ndigits);

// -x, an int again for -(-2**63)
JINJA2CPP_EXPORT InternalValue NegateBigInt(const BigIntView& val);

// Whether the integer is odd
inline bool IsOdd(const BigIntView& val) noexcept
{
    if (val.IsHuge())
    {
        return (val.Words()[0] & 1U) != 0;
    }
    return ((val.InlineDecimal().back() - '0') & 1) != 0;
}

// OverflowError for integer arithmetic whose operand or result is outside int64
[[noreturn]] JINJA2CPP_EXPORT void ThrowBigIntArithmetic();
// OverflowError for a big integer where Python needs an index-sized one (a repeat count, an argument)
[[noreturn]] JINJA2CPP_EXPORT void ThrowIndexOverflow();

// Arithmetic with a big-integer backend (docs/tasks/0271, plan sections 3.2-4)

// Whether the library can use the table: version 1, every field set, the object within the caps
JINJA2CPP_EXPORT bool IsUsableBigIntBackend(const BigIntBackend& backend) noexcept;

// An integer operand of BigIntBinary: an int64, or a big integer when `big` is set. The view
// borrows from the caller's value, which outlives the call
struct IntOperand
{
    IntOperand(int64_t val) noexcept // NOLINT(google-explicit-constructor): either kind of integer is an operand
        : small(val)
    {
    }
    IntOperand(const BigIntView& val) noexcept // NOLINT(google-explicit-constructor): either kind of integer is an operand
        : big(val)
    {
    }

    int64_t small = 0;
    std::optional<BigIntView> big;
};

enum class BigIntOp
{
    Add,
    Subtract,
    Multiply,
    FloorDivide,
    Remainder,
    Power,
};

// Python's int `left op right` where the result or an operand is outside int64: exact through the
// backend of the render, an int again when the result fits int64. Without a backend it raises
// OverflowError, except that `**` with a negative exponent is a float as in Python. The caller
// has already reported division by zero. Out of line and off the int64 paths
JINJA2CPP_EXPORT InternalValue BigIntBinary(BigIntOp op, IntOperand left, IntOperand right);
} // namespace v2
} // namespace jinja2

#endif // JINJA2CPP_SRC_BIG_INT_H
