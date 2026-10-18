#ifndef JINJA2CPP_BIG_INT_H
#define JINJA2CPP_BIG_INT_H

#include <charconv>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <utility>

namespace jinja2
{
namespace detail
{
//! Integral types a \ref BigInt is made from: all except `bool` and the character types, as for \ref Value
template<typename T>
inline constexpr bool IsBigIntSource = std::is_integral_v<T> && !std::is_same_v<T, bool> && !std::is_same_v<T, char> && !std::is_same_v<T, wchar_t> &&
                                       !std::is_same_v<T, char16_t> && !std::is_same_v<T, char32_t>
#ifdef __cpp_char8_t
                                       && !std::is_same_v<T, char8_t>
#endif
    ;
} // namespace detail

/*!
 * \brief An integer of any size, as Python's `int`
 *
 * Holds the canonical decimal form: an optional `-` and digits without leading zeros (`"0"` for zero, never
 * `"-0"`), so two equal numbers always have the same text. It does no arithmetic: it carries a number that does
 * not fit `int64_t` between the host and a template. \ref Value stores one for an integer above `INT64_MAX`
 * (a `uint64_t`, a JSON number), and a host may store one of any size.
 *
 * A `BigInt` may hold a number that fits `int64_t` (`BigInt(5)`); \ref Value keeps it as given.
 */
class BigInt
{
public:
    //! Zero
    BigInt() = default;

    //! The value of any integral type except `bool` and the character types
    template<typename I, std::enable_if_t<detail::IsBigIntSource<I>>* = nullptr>
    explicit BigInt(I value)
    {
        using Unsigned = std::make_unsigned_t<I>;
        const bool negative = value < I(0);
        // Negating in the unsigned type is exact for the most negative value too
        Unsigned magnitude = negative ? static_cast<Unsigned>(Unsigned(0) - static_cast<Unsigned>(value)) : static_cast<Unsigned>(value);
        // 40 digits hold any 128-bit value
        char buffer[48];
        char* end = buffer + sizeof(buffer);
        char* begin = end;
        do
        {
            *--begin = static_cast<char>('0' + static_cast<int>(magnitude % 10));
            magnitude /= 10;
        } while (magnitude != 0);
        if (negative)
        {
            *--begin = '-';
        }
        m_decimal.assign(begin, end);
    }

    /*!
     * \brief Reads `[-]digits` into `out`, as Python's `int()` reads a decimal string with no spaces, sign `+` or
     * underscores; returns whether it did
     *
     * Leading zeros are dropped (`"007"` is 7) and `"-0"` is 0. Anything else, the empty string included, returns
     * `false` and leaves `out` unchanged, as does running out of memory for a long number.
     */
    static bool TryParse(std::string_view decimal, BigInt& out) noexcept
    {
        const bool negative = !decimal.empty() && decimal.front() == '-';
        std::string_view digits = negative ? decimal.substr(1) : decimal;
        if (digits.empty())
        {
            return false;
        }
        for (const char c : digits)
        {
            if (c < '0' || c > '9')
            {
                return false;
            }
        }
        const auto first = digits.find_first_not_of('0');
        if (first == std::string_view::npos)
        {
            out.m_decimal.assign(1, '0');
            return true;
        }
        digits.remove_prefix(first);
        try
        {
            std::string text;
            text.reserve(digits.size() + (negative ? 1 : 0));
            if (negative)
            {
                text.push_back('-');
            }
            text.append(digits);
            out.m_decimal = std::move(text);
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    //! The canonical decimal form, as Python's `str()` prints the number
    [[nodiscard]] const std::string& ToString() const noexcept { return m_decimal; }

    //! -1, 0 or 1
    [[nodiscard]] int Sign() const noexcept
    {
        if (m_decimal.front() == '-')
        {
            return -1;
        }
        return m_decimal == "0" ? 0 : 1;
    }

    //! Stores the value in `out` and returns `true` when it fits `int64_t`; returns `false` otherwise
    bool ToInt64(int64_t& out) const noexcept { return Load(out); }

    //! Stores the value in `out` and returns `true` when it fits `uint64_t`; returns `false` otherwise
    bool ToUInt64(uint64_t& out) const noexcept { return Sign() >= 0 && Load(out); }

    //! Compares by value
    friend bool operator==(const BigInt& lhs, const BigInt& rhs) noexcept { return lhs.m_decimal == rhs.m_decimal; }
    friend bool operator!=(const BigInt& lhs, const BigInt& rhs) noexcept { return !(lhs == rhs); }
    friend bool operator<(const BigInt& lhs, const BigInt& rhs) noexcept { return Compare(lhs, rhs) < 0; }
    friend bool operator<=(const BigInt& lhs, const BigInt& rhs) noexcept { return Compare(lhs, rhs) <= 0; }
    friend bool operator>(const BigInt& lhs, const BigInt& rhs) noexcept { return Compare(lhs, rhs) > 0; }
    friend bool operator>=(const BigInt& lhs, const BigInt& rhs) noexcept { return Compare(lhs, rhs) >= 0; }

private:
    template<typename T>
    bool Load(T& out) const noexcept
    {
        T value{};
        const char* end = m_decimal.data() + m_decimal.size();
        const auto [ptr, ec] = std::from_chars(m_decimal.data(), end, value);
        if (ec != std::errc() || ptr != end)
        {
            return false;
        }
        out = value;
        return true;
    }

    // Canonical forms compare by sign, then by length, then digit by digit
    static int Compare(const BigInt& lhs, const BigInt& rhs) noexcept
    {
        const int lhsSign = lhs.Sign();
        const int rhsSign = rhs.Sign();
        if (lhsSign != rhsSign)
        {
            return lhsSign < rhsSign ? -1 : 1;
        }
        int magnitude = 0;
        if (lhs.m_decimal.size() != rhs.m_decimal.size())
        {
            magnitude = lhs.m_decimal.size() < rhs.m_decimal.size() ? -1 : 1;
        }
        else
        {
            const int cmp = lhs.m_decimal.compare(rhs.m_decimal);
            magnitude = cmp < 0 ? -1 : (cmp > 0 ? 1 : 0);
        }
        return lhsSign < 0 ? -magnitude : magnitude;
    }

    std::string m_decimal = "0";
};
} // namespace jinja2

#endif // JINJA2CPP_BIG_INT_H
