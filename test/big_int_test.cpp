#include "gtest/gtest.h"

#include <jinja2cpp/big_int.h>
#include <jinja2cpp/value.h>

#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

using namespace jinja2;

// The public big integer (task 0269): canonical decimal text, no arithmetic

// Adding the alternative leaves sizeof(Value) as it was: BigInt is no larger than the strings already in it
static_assert(sizeof(BigInt) == sizeof(std::string));
static_assert(!std::is_constructible_v<BigInt, bool>);
static_assert(!std::is_constructible_v<BigInt, char>);
static_assert(!std::is_constructible_v<BigInt, wchar_t>);
static_assert(!std::is_convertible_v<int, BigInt>);
static_assert(std::is_nothrow_move_constructible_v<BigInt>);

TEST(BigIntTest, ConstructsFromIntegralTypes)
{
    EXPECT_EQ("0", BigInt().ToString());
    EXPECT_EQ("5", BigInt(5).ToString());
    EXPECT_EQ("-5", BigInt(-5).ToString());
    EXPECT_EQ("0", BigInt(0u).ToString());
    EXPECT_EQ("200", BigInt(static_cast<unsigned char>(200)).ToString());
    EXPECT_EQ("-128", BigInt(static_cast<signed char>(-128)).ToString());
    EXPECT_EQ("-32768", BigInt(std::numeric_limits<short>::min()).ToString());
    EXPECT_EQ("9223372036854775807", BigInt(std::numeric_limits<int64_t>::max()).ToString());
    EXPECT_EQ("-9223372036854775808", BigInt(std::numeric_limits<int64_t>::min()).ToString());
    EXPECT_EQ("18446744073709551615", BigInt(std::numeric_limits<uint64_t>::max()).ToString());
}

TEST(BigIntTest, TryParseAcceptsDecimalOnly)
{
    struct Row
    {
        std::string_view input;
        const char* expected; // nullptr: rejected
    };
    const Row rows[] = {
        { "0", "0" },
        { "7", "7" },
        { "007", "7" },
        { "-0", "0" },
        { "-000", "0" },
        { "-007", "-7" },
        { "18446744073709551616", "18446744073709551616" },
        { "-123456789012345678901234567890", "-123456789012345678901234567890" },
        { "", nullptr },
        { "-", nullptr },
        { "+1", nullptr },
        { "1e3", nullptr },
        { " 1", nullptr },
        { "1 ", nullptr },
        { "1_000", nullptr },
        { "--1", nullptr },
        { "0x10", nullptr },
        { "1.0", nullptr },
    };
    for (const auto& row : rows)
    {
        BigInt out(42);
        const bool parsed = BigInt::TryParse(row.input, out);
        if (row.expected)
        {
            EXPECT_TRUE(parsed) << row.input;
            EXPECT_EQ(row.expected, out.ToString()) << row.input;
        }
        else
        {
            EXPECT_FALSE(parsed) << row.input;
            EXPECT_EQ("42", out.ToString()) << row.input << ": left unchanged";
        }
    }
}

TEST(BigIntTest, SignAndConversions)
{
    EXPECT_EQ(0, BigInt().Sign());
    EXPECT_EQ(1, BigInt(3).Sign());
    EXPECT_EQ(-1, BigInt(-3).Sign());

    int64_t i = 0;
    uint64_t u = 0;
    EXPECT_TRUE(BigInt(std::numeric_limits<int64_t>::min()).ToInt64(i));
    EXPECT_EQ(std::numeric_limits<int64_t>::min(), i);
    EXPECT_FALSE(BigInt(std::numeric_limits<uint64_t>::max()).ToInt64(i));
    EXPECT_EQ(std::numeric_limits<int64_t>::min(), i);
    EXPECT_TRUE(BigInt(std::numeric_limits<uint64_t>::max()).ToUInt64(u));
    EXPECT_EQ(std::numeric_limits<uint64_t>::max(), u);
    EXPECT_FALSE(BigInt(-1).ToUInt64(u));
    BigInt huge;
    ASSERT_TRUE(BigInt::TryParse("18446744073709551616", huge));
    EXPECT_FALSE(huge.ToUInt64(u));
    EXPECT_FALSE(huge.ToInt64(i));
    EXPECT_TRUE(BigInt().ToUInt64(u));
    EXPECT_EQ(0u, u);
}

TEST(BigIntTest, ComparesByValue)
{
    // Ascending; every pair compares as its position does
    std::vector<BigInt> sorted;
    for (const char* text : { "-100000000000000000000", "-99999999999999999999", "-10", "-9", "0", "9", "10", "99999999999999999999",
                              "100000000000000000000" })
    {
        BigInt value;
        ASSERT_TRUE(BigInt::TryParse(text, value)) << text;
        sorted.push_back(value);
    }
    for (size_t a = 0; a < sorted.size(); ++a)
    {
        for (size_t b = 0; b < sorted.size(); ++b)
        {
            const auto& lhs = sorted[a];
            const auto& rhs = sorted[b];
            EXPECT_EQ(a == b, lhs == rhs) << lhs.ToString() << " " << rhs.ToString();
            EXPECT_EQ(a != b, lhs != rhs) << lhs.ToString() << " " << rhs.ToString();
            EXPECT_EQ(a < b, lhs < rhs) << lhs.ToString() << " " << rhs.ToString();
            EXPECT_EQ(a <= b, lhs <= rhs) << lhs.ToString() << " " << rhs.ToString();
            EXPECT_EQ(a > b, lhs > rhs) << lhs.ToString() << " " << rhs.ToString();
            EXPECT_EQ(a >= b, lhs >= rhs) << lhs.ToString() << " " << rhs.ToString();
        }
    }
    BigInt padded;
    ASSERT_TRUE(BigInt::TryParse("-0009", padded));
    EXPECT_EQ(BigInt(-9), padded);
}
