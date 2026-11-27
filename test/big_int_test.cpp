#include "../src/big_int.h"
#include "../src/contracts.h"
#include "../src/cycle_collector.h"
#include "../src/internal_value.h"
#include "../src/recursion_guard.h"
#include "big_int_toy_backend.h"
#include "gtest/gtest.h"

#include <jinja2cpp/big_int/boost_cpp_int.h>
#include <jinja2cpp/big_int_backend.h>

#include <jinja2cpp/big_int.h>
#include <jinja2cpp/error_info.h>
#include <jinja2cpp/template.h>
#include <jinja2cpp/template_env.h>
#include <jinja2cpp/user_callable.h>
#include <jinja2cpp/value.h>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cmath>
#include <cstdint>
#include <ratio>
#include <limits>
#include <new>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
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

// A moved-from BigInt is zero, never an empty text that Sign() or a comparison would read past
TEST(BigIntTest, MovedFromIsZero)
{
    BigInt a(-12);
    const BigInt b(std::move(a));
    EXPECT_EQ("-12", b.ToString());
    EXPECT_EQ("0", a.ToString()); // NOLINT(bugprone-use-after-move,clang-analyzer-cplusplus.Move)
    EXPECT_EQ(0, a.Sign());       // NOLINT(bugprone-use-after-move,clang-analyzer-cplusplus.Move)
    EXPECT_TRUE(a < b || b < a);  // NOLINT(bugprone-use-after-move,clang-analyzer-cplusplus.Move)

    BigInt c(7);
    c = std::move(a); // NOLINT(bugprone-use-after-move,clang-analyzer-cplusplus.Move)
    EXPECT_EQ("0", c.ToString());
    EXPECT_EQ("0", a.ToString()); // NOLINT(bugprone-use-after-move,clang-analyzer-cplusplus.Move)

    // Moving a Value moves the BigInt it holds
    Value held(std::numeric_limits<uint64_t>::max());
    const Value moved(std::move(held));
    EXPECT_EQ("18446744073709551615", moved.ToString());
    if (const auto* rest = held.GetIf<BigInt>()) // NOLINT(bugprone-use-after-move,clang-analyzer-cplusplus.Move)
    {
        EXPECT_EQ(0, rest->Sign());
    }
}

// Task 0270: templates hold integers outside int64 in their text form. Every row renders in a
// narrow and a wide template; `python` is what Python Jinja2 3.1.6 renders (nullptr: it raises).
// Without a big-integer backend the core prints, compares and converts big values exactly, and
// arithmetic whose operand or result is outside int64 raises OverflowError (IntegerOutOfRange).
namespace
{
enum class Expect
{
    // As Python renders or fails
    Python,
    // As Python with a backend; IntegerOutOfRange without one (task 0271)
    OverflowWithoutBackend,
    // IntegerOutOfRange in every build: Python's OverflowError, or a deliberate divergence (range)
    Overflow,
};

struct BigIntCase
{
    const char* source;
    const char* python;
    Expect expect;
};

const BigIntCase BigIntCases[] = {
    { "{{ 12345678901234567890 }}|{{ -12345678901234567890 }}|{{ 0x10000000000000000 }}|{{ 0o2000000000000000000000 }}|{{ 0b1_0000000000000000000000000000000000000000000000000000000000000000 }}", "12345678901234567890|-12345678901234567890|18446744073709551616|18446744073709551616|18446744073709551616", Expect::Python },
    { "{{ 9223372036854775808 }}|{{ -9223372036854775808 }}|{{ (-9223372036854775808) is integer }}|{{ +9223372036854775808 }}|{{ 1_000_000_000_000_000_000_000 }}", "9223372036854775808|-9223372036854775808|True|9223372036854775808|1000000000000000000000", Expect::Python },
    { "{{ big }}|{{ nbig }}|{{ big|string }}|{{ big ~ '' }}|{{ [big] }}|{{ (big,) }}|{{ {big: 1} }}|{{ big|tojson }}|{{ [big, nbig]|tojson }}|{{ {big: 1}|tojson }}|{{ big|pprint }}|{{ {big: nbig}|pprint }}", "18446744073709551615|-18446744073709551615|18446744073709551615|18446744073709551615|[18446744073709551615]|(18446744073709551615,)|{18446744073709551615: 1}|18446744073709551615|[18446744073709551615, -18446744073709551615]|{\"18446744073709551615\": 1}|18446744073709551615|{18446744073709551615: -18446744073709551615}", Expect::Python },
    { "{{ big > 1 }}|{{ big < 1 }}|{{ nbig < 1 }}|{{ big == 18446744073709551615 }}|{{ big == 18446744073709551616 }}|{{ big != nbig }}|{{ big > nbig }}|{{ big >= big }}|{{ 1 < big }}|{{ true < big }}|{{ big == 'x' }}", "True|False|True|True|False|True|True|True|True|True|False", Expect::Python },
    { "{{ big == 18446744073709551615.0 }}|{{ big > 1.5 }}|{{ big < 1e30 }}|{{ nbig < -1e19 }}|{{ 9223372036854775808 == 9223372036854775808.0 }}|{{ 9223372036854775809 > 9223372036854775808.0 }}|{{ big < nan }}|{{ big != nan }}|{{ big < inf }}|{{ nbig > -inf }}|{{ 1.5 < big }}", "False|True|True|True|True|True|False|True|True|True|True", Expect::Python },
    { "{{ big|abs }}|{{ nbig|abs }}|{{ -9223372036854775808|abs }}|{{ big|float }}|{{ big|int }}|{{ nbig|round }}|{{ big|round(2) }}|{{ big|round(-3) }}|{{ 12345678901234567890|round(-19) }}|{{ big|round(-1, 'floor') }}|{{ big|round(1, 'ceil') }}|{{ big|filesizeformat }}|{{ big|filesizeformat(true) }}", "18446744073709551615|18446744073709551615|9223372036854775808|1.8446744073709552e+19|18446744073709551615|-18446744073709551615|18446744073709551615|18446744073709552000|10000000000000000000|1.8446744073709552e+19|1.8446744073709552e+19|18.4 EB|16.0 EiB", Expect::Python },
    { "{{ big|round(-1) }}|{{ nbig|round(-20) }}|{{ 15|round(-1) }}|{{ 25|round(-1) }}|{{ 9223372036854775807|round(-1) }}|{{ 1e20|round }}", "18446744073709551620|0|20|20|9223372036854775810|1e+20", Expect::Python },
    { "{{ '99999999999999999999'|int }}|{{ '-99999999999999999999'|int }}|{{ 1e20|int }}|{{ -1e20|int }}|{{ 'ffffffffffffffffff'|int(base=16) }}|{{ '0x1_0000_0000_0000_0000'|int(base=0) }}|{{ ('1' * 30)|int }}|{{ 'zzzzzzzzzzzzzzz'|int(base=36) }}", "99999999999999999999|-99999999999999999999|100000000000000000000|-100000000000000000000|4722366482869645213695|18446744073709551616|111111111111111111111111111111|221073919720733357899775", Expect::Python },
    { "{{ '%d|%i|%u|%x|%X|%o|%#x|%#o|%e|%.3f|%g|%s|%r|%25d|%-+25d|'|format(big, nbig, big, big, big, big, nbig, big, big, big, big, big, nbig, big, big) }}", "18446744073709551615|-18446744073709551615|18446744073709551615|ffffffffffffffff|FFFFFFFFFFFFFFFF|1777777777777777777777|-0xffffffffffffffff|0o1777777777777777777777|1.844674e+19|18446744073709551616.000|1.84467e+19|18446744073709551615|-18446744073709551615|     18446744073709551615|+18446744073709551615    |", Expect::Python },
    { "{{ '{:,}|{:x}|{:#X}|{:b}|{:>25}|{:+}|{:e}|{:_}|{:o}|{:d}|{:%}'.format(big, big, big, big, big, big, big, big, nbig, nbig, big) }}", "18,446,744,073,709,551,615|ffffffffffffffff|0XFFFFFFFFFFFFFFFF|1111111111111111111111111111111111111111111111111111111111111111|     18446744073709551615|+18446744073709551615|1.844674e+19|18_446_744_073_709_551_615|-1777777777777777777777|-18446744073709551615|1844674407370955161600.000000%", Expect::Python },
    { "{{ big is number }}|{{ big is integer }}|{{ big is float }}|{{ big is even }}|{{ big is odd }}|{{ nbig is odd }}|{{ big is defined }}|{{ big is sameas big }}|{{ big in [1, big] }}|{{ big in [1, 2] }}|{{ big is iterable }}|{{ big is string }}", "True|True|False|False|True|True|True|True|True|False|False|False", Expect::Python },
    { "{{ [big, 3, nbig, 1.5]|sort }}|{{ [big, big, 3]|unique|list }}|{{ [big, 3]|max }}|{{ [nbig, 3]|min }}|{{ {big: 'a', 1: 'b'}|dictsort }}|{{ {big: 'a'}[18446744073709551615] }}|{{ {18446744073709551616.0: 'x'}[18446744073709551616] }}|{{ {18446744073709551616: 'y'}[18446744073709551616.0] }}", "[-18446744073709551615, 1.5, 3, 18446744073709551615]|[18446744073709551615, 3]|18446744073709551615|-18446744073709551615|[(1, 'b'), (18446744073709551615, 'a')]|a|x|y", Expect::Python },
    { "{{ [{'n': big}, {'n': 1}]|sort(attribute='n')|map(attribute='n')|join(',') }}|{{ [{'n': big}, {'n': big}, {'n': 1}]|groupby('n')|map(attribute='grouper')|join(',') }}|{{ [big]|join }}|{{ big|string|length }}", "1,18446744073709551615|1,18446744073709551615|18446744073709551615|20", Expect::Python },
    { "{{ not big }}|{{ big and 1 }}|{{ 'yes' if big else 'no' }}|{{ big.bit_length() }}|{{ nbig.bit_length() }}|{{ 9223372036854775808 .bit_length() }}", "False|1|yes|64|64|64", Expect::Python },
    { "{{ [1,2,3][big] }}|{{ 'abc'[big] }}|{{ [1,2,3][big:] }}|{{ [1,2,3][:big] }}|{{ [1,2,3][nbig:] }}|{{ [1,2,3][::nbig] }}|{{ 'abc'[nbig:big] }}|{{ big[0] }}", "||[]|[1, 2, 3]|[1, 2, 3]|[3]|abc|", Expect::Python },
    { "{{ big / 2 }}|{{ big / 2.0 }}|{{ big + 0.5 }}|{{ big * 1.0 }}|{{ 1.5 - big }}|{{ big / big }}", "9.223372036854776e+18|9.223372036854776e+18|1.8446744073709552e+19|1.8446744073709552e+19|-1.8446744073709552e+19|1.0", Expect::Python },
    { "{{ big + 1 }}", "18446744073709551616", Expect::OverflowWithoutBackend },
    { "{{ big - 1 }}", "18446744073709551614", Expect::OverflowWithoutBackend },
    { "{{ 1 - nbig }}", "18446744073709551616", Expect::OverflowWithoutBackend },
    { "{{ big * 2 }}", "36893488147419103230", Expect::OverflowWithoutBackend },
    { "{{ big // 2 }}", "9223372036854775807", Expect::OverflowWithoutBackend },
    { "{{ nbig // 7 }}", "-2635249153387078803", Expect::OverflowWithoutBackend },
    { "{{ big % 7 }}", "1", Expect::OverflowWithoutBackend },
    { "{{ nbig % 7 }}", "6", Expect::OverflowWithoutBackend },
    { "{{ big ** 2 }}", "340282366920938463426481119284349108225", Expect::OverflowWithoutBackend },
    { "{{ 2 ** 64 }}", "18446744073709551616", Expect::OverflowWithoutBackend },
    { "{{ 9223372036854775807 + 1 }}", "9223372036854775808", Expect::OverflowWithoutBackend },
    { "{{ -9223372036854775807 - 2 }}", "-9223372036854775809", Expect::OverflowWithoutBackend },
    { "{{ 4294967296 * 4294967296 }}", "18446744073709551616", Expect::OverflowWithoutBackend },
    { "{{ (-9223372036854775807 - 1) // -1 }}", "9223372036854775808", Expect::OverflowWithoutBackend },
    { "{{ -(-9223372036854775807 - 1) }}", "9223372036854775808", Expect::OverflowWithoutBackend },
    { "{{ -(-9223372036854775808) }}", "9223372036854775808", Expect::OverflowWithoutBackend },
    { "{{ [big, 1]|sum }}", "18446744073709551616", Expect::OverflowWithoutBackend },
    { "{{ big is divisibleby 3 }}", "True", Expect::OverflowWithoutBackend },
    { "{{ big + big }}", "36893488147419103230", Expect::OverflowWithoutBackend },
    { "{{ 'a' * big }}", nullptr, Expect::Overflow },
    { "{{ [1] * big }}", nullptr, Expect::Overflow },
    { "{{ 'x'|center(big) }}", nullptr, Expect::Overflow },
    { "{{ '%c'|format(big) }}", nullptr, Expect::Overflow },
    { "{{ '%*d'|format(big, 1) }}", nullptr, Expect::Overflow },
    { "{{ '{:c}'.format(big) }}", nullptr, Expect::Overflow },
    { "{{ big // 0 }}", nullptr, Expect::Python },
    { "{{ big / 0 }}", nullptr, Expect::Python },
    { "{{ big % 0 }}", nullptr, Expect::Python },
    { "{{ big|list }}", nullptr, Expect::Python },
    { "{{ big + 'a' }}", nullptr, Expect::Python },
    { "{{ -big }}|{{ -nbig }}", "-18446744073709551615|18446744073709551615", Expect::Python },
    { "{{ range(big) }}", "range(0, 18446744073709551615)", Expect::Overflow },
    // Task 0543 P1b: h has 14400 bits, past HugeBigIntBits, and keeps binary words; hbig is the
    // same integer from a host BigInt (decimal). 2^14284 has 4300 digits, 2^14285 - 1 has 14285
    // bits (4301 digits, not huge) and 2^14285 14286 bits (huge). Outputs from Python Jinja2 3.1.6
    { "{% set h = hs|int(base=16) %}{{ h.bit_length() }}|{{ h.bit_count() }}|{{ '{:x}'.format(h)|length }}|{{ ('%x'|format(h))[:4] }}|{{ h is odd }}|{{ (-h) is even }}|{{ ('{:x}'.format(-h))[:5] }}|{{ ('{:b}'.format(h))|length }}|{{ ('{:o}'.format(h))[:3] }}|{{ ('%#X'|format(-h))[:6] }}", "14400|14400|3600|ffff|True|False|-ffff|14400|777|-0XFFF", Expect::Python },
    { "{% set h = hs|int(base=16) %}{{ (h|abs) == h }}|{{ ((-h)|abs) == h }}|{{ h == hbig }}|{{ h != hbig }}|{{ -h == -hbig }}|{{ -h < h }}|{{ {h: 1}[hbig] }}|{{ {hbig: 1}[-(-h)] }}|{{ h in [hbig] }}|{{ [h, hbig]|unique|list|length }}", "True|True|True|False|True|True|1|1|True|1", Expect::Python },
    { "{% set h = hs|int(base=16) %}{{ h > big }}|{{ big < h }}|{{ -h < nbig }}|{{ h > 1e308 }}|{{ -h < -1e308 }}|{{ h < inf }}|{{ h > nan }}|{{ h == 1e308 }}", "True|True|True|True|True|True|False|False", Expect::Python },
    { "{{ (p14284|int(base=2))|string|length }}|{{ (p14285|int(base=2)).bit_length() }}|{{ (p14285m1|int(base=2)).bit_count() }}|{{ (p14285m1|int(base=2)) is odd }}|{{ (p14285|int(base=2)) is odd }}|{{ (p14285|int(base=2)) > (p14285m1|int(base=2)) }}", "4300|14286|14285|True|False|True", Expect::Python },
    { "{% set h = hs|int(base=16) %}{{ (o8|int(base=8)) == h }}|{{ (o32|int(base=32)) == h }}|{{ (o4|int(base=4)) == h }}|{{ (o32|int(base=32)).bit_count() }}|{{ ('-' ~ hs)|int(base=16) == -h }}|{{ (('-' ~ hs)|int(base=16)).bit_length() }}|{{ (h|round(-5)).bit_length() }}|{{ (h|round(-5)) > h }}", "True|True|True|14400|True|14400|14400|False", Expect::Python },
    // Task 0577: the SWAR bit count and the whole-word digits of bases 2, 4 and 16, on words with the
    // top bit set (hm), a single bit (2^14285) and zero, and grouping and prefixes after digits stopped
    // being reversed twice. Outputs from Python Jinja2 3.1.6
    { "{% set m = hm|int(base=16) %}{% set p = p14285|int(base=2) %}{{ m.bit_count() }}|{{ p.bit_count() }}|{{ (0).bit_count() }}|{{ '{:x}'.format(m) == hm }}|{{ ('{:#_x}'.format(m))[:12] }}|{{ ('{:#_x}'.format(m))[-10:] }}|{{ '{:#_x}'.format(m)|length }}|{{ '{:b}'.format(p)|length }}|{{ ('{:o}'.format(p))[:3] }}|{{ ('{:x}'.format(p))[:2] }}|{{ '{:#_x}|{:#_b}|{:_o}|{:x}|{:#o}'.format(0, 5, 4095, 255, 8) }}|{{ ('{:#_X}'.format(-m))[:8] }}|{{ ('{:+x}'.format(m))[:3] }}|{{ ('{:>4000x}'.format(m))[:1] == ' ' }}", "7201|1|0|True|0x8_f0e1_d2c|_b4a5_9687|4503|14286|400|20|0x0|0b101|7777|ff|0o10|-0X8_F0E|+8f|True", Expect::Python },
    { "{{ (p14285m1|int(base=2))|string }}", nullptr, Expect::Python },
    { "{{ (p14285|int(base=2))|string }}", nullptr, Expect::Python },
    { "{{ hs|int(base=16) }}", nullptr, Expect::Python },
    { "{{ hbig|tojson }}", nullptr, Expect::Python },
    { "{{ (hs|int(base=16))|float }}", nullptr, Expect::Python },
    { "{{ hbig / 2 }}", nullptr, Expect::Python },
    { "{{ hbig|filesizeformat }}", nullptr, Expect::Python },
    // Task 0566: an item of a host container is read again and again; hl holds hbig and its negative, hmap hbig
    { "{% set acc = [] %}{% for i in range(20) %}{{ acc.append(hl[0]) }}{% endfor %}|{{ acc|length }}|{{ acc[19] == hl[0] }}|{{ hl[0] > 0 }}|{{ hl[1] < 0 }}|{{ hl[1] == -hl[0] }}|{{ hl[0] == hmap.k }}|{{ hl[0] == hbig }}|{{ hl[0] is sameas hl[0] }}|{{ hl[0].bit_length() }}|{{ hl[1].bit_count() }}", "NoneNoneNoneNoneNoneNoneNoneNoneNoneNoneNoneNoneNoneNoneNoneNoneNoneNoneNoneNone|20|True|True|True|True|True|True|True|14400|14400", Expect::Python },
    { "{{ hl[0] }}", nullptr, Expect::Python },
    { "{% set h = hs|int(base=16) %}{{ (h + 1).bit_length() }}|{{ (h * h).bit_count() }}|{{ h - hbig }}|{{ h // hbig }}|{{ (h + 1) is even }}|{{ (h * -1) == -h }}|{{ {h + 0: 2}[hbig] }}|{{ {(p14285m1|int(base=2)): 3}[(p14285|int(base=2)) - 1] }}", "14401|14400|0|1|True|True|2|3", Expect::OverflowWithoutBackend },
};

// 3601 hex digits (14404 bits, huge): words with the top bit set and every nibble value
std::string HugeMixedHex()
{
    std::string result = "8";
    for (int n = 0; n < 225; ++n)
    {
        result += "f0e1d2c3b4a59687";
    }
    return result;
}

ValuesMap BigIntParams()
{
    BigInt nbig;
    BigInt::TryParse("-18446744073709551615", nbig);
    const std::string hs(3600, 'f');
    const auto hbig = BigInt::FromDigits(hs, 16, false);
    const auto hneg = BigInt::FromDigits(hs, 16, true);
    return { { "hl", ValuesList{ hbig, hneg } }, { "hmap", ValuesMap{ { "k", hbig } } }, { "big", std::numeric_limits<uint64_t>::max() }, { "nbig", nbig }, { "nan", std::nan("") }, { "inf", HUGE_VAL }, { "small", BigInt(5) }, { "hs", hs }, { "hbig", BigInt::FromDigits(hs, 16, false) }, { "o8", BigInt::FromDigits(hs, 16, false).ToDigits(8) }, { "o32", BigInt::FromDigits(hs, 16, false).ToDigits(32) }, { "o4", BigInt::FromDigits(hs, 16, false).ToDigits(4) }, { "p14284", "1" + std::string(14284, '0') }, { "p14285m1", std::string(14285, '1') }, { "p14285", "1" + std::string(14285, '0') }, { "hm", HugeMixedHex() } };
}

template<typename CharT>
void CheckBigIntCase(const BigIntCase& row, const BigIntBackend* backend)
{
    using TemplateT = std::conditional_t<std::is_same_v<CharT, char>, Template, TemplateW>;
    const bool hasBackend = backend != nullptr;
    const std::string_view narrow(row.source);
    TemplateEnv env;
    ASSERT_TRUE(env.SetBigIntBackend(backend));
    TemplateT tpl(&env);
    ASSERT_TRUE(tpl.Load(std::basic_string<CharT>(narrow.begin(), narrow.end()))) << row.source;
    const auto result = tpl.RenderAsString(BigIntParams());
    const bool overflows = row.expect == Expect::Overflow || (row.expect == Expect::OverflowWithoutBackend && !hasBackend);
    if (overflows || !row.python)
    {
        ASSERT_FALSE(result.has_value()) << row.source;
        if (overflows)
        {
            EXPECT_EQ(ErrorCode::IntegerOutOfRange, result.error().GetCode()) << row.source;
        }
        return;
    }
    ASSERT_TRUE(result.has_value()) << row.source << ": error " << static_cast<int>(result.error().GetCode());
    const std::string_view expected(row.python);
    EXPECT_EQ(std::basic_string<CharT>(expected.begin(), expected.end()), result.value()) << row.source;
}
} // namespace

TEST(BigIntNoBackend, RendersAsPythonOrOverflows)
{
    for (const auto& row : BigIntCases)
    {
        CheckBigIntCase<char>(row, nullptr);
        CheckBigIntCase<wchar_t>(row, nullptr);
    }
}

// The same rows with a backend (docs/tasks/0271): arithmetic is exact, as in Python
TEST(BigIntToyBackend, RendersAsPython)
{
    for (const auto& row : BigIntCases)
    {
        CheckBigIntCase<char>(row, big_int_toy::Backend());
        CheckBigIntCase<wchar_t>(row, big_int_toy::Backend());
    }
    EXPECT_EQ(0, big_int_toy::LiveObjects());
}

namespace
{
// Arithmetic rows only a backend renders; outputs from Python Jinja2 3.1.6
const BigIntCase BigIntBackendCases[] = {
    { "{{ -(2 ** 64) // 7 }}|{{ -(2 ** 64) % 7 }}|{{ (2 ** 64) // -7 }}|{{ (2 ** 64) % -7 }}|{{ -(2 ** 64) // -7 }}|{{ -(2 ** 64) % -7 }}", "-2635249153387078803|5|-2635249153387078803|-5|2635249153387078802|-2", Expect::OverflowWithoutBackend },
    { "{{ 2 ** 100 }}|{{ (-3) ** 41 }}|{{ 7 ** 0 }}|{{ big ** 0 }}|{{ big ** 1 }}|{{ (-1) ** big }}|{{ 1 ** big }}|{{ 0 ** big }}|{{ big ** -1 }}|{{ 2 ** -2 }}", "1267650600228229401496703205376|-36472996377170786403|1|1|18446744073709551615|-1|1|0|5.421010862427522e-20|0.25", Expect::OverflowWithoutBackend },
    { "{{ (2 ** 64) - (2 ** 64) + 5 }}|{{ ((2 ** 64) - (2 ** 64) + 5) is integer }}|{{ big - big }}|{{ big + nbig }}|{{ (big // big) }}|{{ 9223372036854775807 + 1 - 1 }}", "5|True|0|0|1|9223372036854775807", Expect::OverflowWithoutBackend },
    { "{{ 9223372036854775807 * 2 }}|{{ (-9223372036854775807 - 1) // -1 }}|{{ -(-9223372036854775807 - 1) }}|{{ 4294967296 * 4294967296 }}|{{ -9223372036854775807 - 2 }}", "18446744073709551614|9223372036854775808|9223372036854775808|18446744073709551616|-9223372036854775809", Expect::OverflowWithoutBackend },
    { "{{ [big, 1, 2]|sum }}|{{ [big, nbig]|sum }}|{{ big is divisibleby 3 }}|{{ big is divisibleby 7 }}|{{ (2 ** 70)|round(-20) }}|{{ (2 ** 64 + 0.5) }}|{{ (2 ** 64) * 1.5 }}", "18446744073709551618|0|True|False|1200000000000000000000|1.8446744073709552e+19|2.7670116110564327e+19", Expect::OverflowWithoutBackend },
    { "{{ big * big }}|{{ big * nbig }}|{{ nbig * nbig }}|{{ big + 1 }}|{{ nbig - 1 }}|{{ big % 1000 }}|{{ nbig % 1000 }}|{{ big // 1000 }}|{{ nbig // 1000 }}", "340282366920938463426481119284349108225|-340282366920938463426481119284349108225|340282366920938463426481119284349108225|18446744073709551616|-18446744073709551616|615|385|18446744073709551|-18446744073709552", Expect::OverflowWithoutBackend },
    { "{{ (2 ** 20000) % 7 }}|{{ ((2 ** 20000) // (2 ** 19990)) }}|{{ (10 ** 30)|string|length }}|{{ (2 ** 64) == 18446744073709551616 }}|{{ (2 ** 64) > 2 ** 63 }}", "4|1024|31|True|True", Expect::OverflowWithoutBackend },
    { "{% set ns = namespace(x=1) %}{% for i in range(30) %}{% set ns.x = ns.x * 7 %}{% endfor %}{{ ns.x }}|{{ ns.x % 1000000007 }}", "22539340290692258087863249|157445110", Expect::OverflowWithoutBackend },
    { "{{ big ** -1 }}|{{ 2 ** -2 }}", "5.421010862427522e-20|0.25", Expect::Python },
    { "{{ big // 0 }}", nullptr, Expect::Python },
    { "{{ (2 ** 64) % 0 }}", nullptr, Expect::Python },
};

template<typename CharT>
ErrorCode RenderError(const std::string& source, const BigIntBackend* backend)
{
    TemplateEnv env;
    env.SetBigIntBackend(backend);
    Template tpl(&env);
    EXPECT_TRUE(tpl.Load(source)) << source;
    const auto result = tpl.RenderAsString(BigIntParams());
    EXPECT_FALSE(result.has_value()) << source;
    return result.has_value() ? ErrorCode::Unspecified : result.error().GetCode();
}
} // namespace

TEST(BigIntToyBackend, ArithmeticAsPython)
{
    for (const auto& row : BigIntBackendCases)
    {
        CheckBigIntCase<char>(row, nullptr);
        CheckBigIntCase<char>(row, big_int_toy::Backend());
        CheckBigIntCase<wchar_t>(row, big_int_toy::Backend());
    }
    EXPECT_EQ(0, big_int_toy::LiveObjects());
}

TEST(BigIntToyBackend, Limits)
{
    const auto* toy = big_int_toy::Backend();
    // The cap of 2^20 bits stops `**` before it computes, and a product past it
    EXPECT_EQ(ErrorCode::ResourceLimitExceeded, RenderError<char>("{{ 2 ** 2097152 }}", toy));
    EXPECT_EQ(ErrorCode::ResourceLimitExceeded, RenderError<char>("{{ 3 ** big }}", toy));
    // Printing more than 4300 digits is Python's ValueError; computing with them is fine. A constant one fails
    // the Load (BigIntTemplate.StrDigitsConstantsFailTheLoad), one made from data the render
    EXPECT_NE(ErrorCode::IntegerOutOfRange, RenderError<char>("{{ 2 ** (small * 4000) }}", toy));
    EXPECT_EQ(0, big_int_toy::LiveObjects());
}

TEST(BigIntToyBackend, EnvironmentChecksTheTable)
{
    TemplateEnv env;
    EXPECT_EQ(GetBuiltinBigIntBackend(), env.GetBigIntBackend());
    const auto* toy = big_int_toy::Backend();
    ASSERT_TRUE(env.SetBigIntBackend(toy));
    EXPECT_EQ(toy, env.GetBigIntBackend());

    auto rejected = [&env, toy](auto change) {
        BigIntBackend bad = *toy;
        change(bad);
        const bool accepted = env.SetBigIntBackend(&bad);
        // A rejected table changes nothing
        EXPECT_EQ(toy, env.GetBigIntBackend());
        return !accepted;
    };
    EXPECT_TRUE(rejected([](BigIntBackend& b) { b.structSize = static_cast<uint32_t>(offsetof(BigIntBackend, divideTruncated)); }));
    EXPECT_TRUE(rejected([](BigIntBackend& b) { b.version = 2; }));
    EXPECT_TRUE(rejected([](BigIntBackend& b) { b.objectSize = MaxBigIntObjectSize + 1; }));
    EXPECT_TRUE(rejected([](BigIntBackend& b) { b.objectAlign = 32; }));
    EXPECT_TRUE(rejected([](BigIntBackend& b) { b.objectAlign = 3; }));
    EXPECT_TRUE(rejected([](BigIntBackend& b) { b.name = nullptr; }));
    EXPECT_TRUE(rejected([](BigIntBackend& b) { b.multiply = nullptr; }));
    EXPECT_TRUE(rejected([](BigIntBackend& b) { b.divideTruncated = nullptr; }));

    // A later version's larger table is used as far as version 1 goes
    struct Larger
    {
        BigIntBackend v1;
        void* extra;
    };
    static Larger larger{ *toy, nullptr };
    larger.v1.structSize = sizeof(Larger);
    EXPECT_TRUE(env.SetBigIntBackend(&larger.v1));
    EXPECT_TRUE(env.SetBigIntBackend(nullptr));
    EXPECT_EQ(nullptr, env.GetBigIntBackend());
}

TEST(BigIntToyBackend, TemplateKeepsTheBackendItWasLoadedWith)
{
    TemplateEnv env;
    ASSERT_TRUE(env.SetBigIntBackend(big_int_toy::Backend()));
    Template withBackend(&env);
    ASSERT_TRUE(withBackend.Load("{{ big + 1 }}"));
    ASSERT_TRUE(env.SetBigIntBackend(nullptr));
    Template without(&env);
    ASSERT_TRUE(without.Load("{{ big + 1 }}"));
    EXPECT_EQ("18446744073709551616", withBackend.RenderAsString(BigIntParams()).value());
    const auto failed = without.RenderAsString(BigIntParams());
    ASSERT_FALSE(failed.has_value());
    EXPECT_EQ(ErrorCode::IntegerOutOfRange, failed.error().GetCode());

    // A render inside another one uses its own template's backend, and the outer one's comes back after it
    jinja2::ValuesMap params = BigIntParams();
    params["inner"] = MakeCallable([&without]() { return without.RenderAsString(BigIntParams()).has_value(); });
    ASSERT_TRUE(env.SetBigIntBackend(big_int_toy::Backend()));
    Template outer(&env);
    ASSERT_TRUE(outer.Load("{{ inner() }}|{{ big + 2 }}"));
    const auto nested = outer.RenderAsString(params);
    ASSERT_TRUE(nested.has_value()) << nested.error().ToString();
    EXPECT_EQ("False|18446744073709551617", nested.value());

    // The same when the inner render has limits of its own, and when the outer one has them
    // (a backend alone does not install limits, docs/tasks/0272)
    TemplateEnv limited;
    Settings withLimits;
    withLimits.limits.maxRenderSteps = 1000;
    limited.SetSettings(withLimits);
    ASSERT_TRUE(limited.SetBigIntBackend(nullptr));
    Template innerLimited(&limited);
    ASSERT_TRUE(innerLimited.Load("{{ big + 1 }}"));
    params["inner"] = MakeCallable([&innerLimited]() { return innerLimited.RenderAsString(BigIntParams()).has_value(); });
    EXPECT_EQ("False|18446744073709551617", outer.RenderAsString(params).value());
    ASSERT_TRUE(limited.SetBigIntBackend(big_int_toy::Backend()));
    Template outerLimited(&limited);
    ASSERT_TRUE(outerLimited.Load("{{ inner() }}|{{ big + 2 }}"));
    params["inner"] = MakeCallable([&without]() { return without.RenderAsString(BigIntParams()).has_value(); });
    EXPECT_EQ("False|18446744073709551617", outerLimited.RenderAsString(params).value());
    Template innerWith(&env);
    ASSERT_TRUE(innerWith.Load("{{ big + 1 }}"));
    params["inner"] = MakeCallable([&innerWith]() { return innerWith.RenderAsString(BigIntParams()).value(); });
    ASSERT_TRUE(env.SetBigIntBackend(nullptr));
    Template plainOuter(&env);
    ASSERT_TRUE(plainOuter.Load("{{ inner() }}|{{ big + 2 }}"));
    const auto restored = plainOuter.RenderAsString(params);
    ASSERT_FALSE(restored.has_value());
    EXPECT_EQ(ErrorCode::IntegerOutOfRange, restored.error().GetCode());
    ASSERT_TRUE(env.SetBigIntBackend(big_int_toy::Backend()));

    // Settings changed after the backend was set keep it, through the env and its templates
    Settings changed = env.GetSettings();
    changed.trimBlocks = true;
    env.SetSettings(changed);
    EXPECT_EQ(big_int_toy::Backend(), env.GetBigIntBackend());
    Template afterSettings(&env);
    ASSERT_TRUE(afterSettings.Load("{{ big + 3 }}"));
    EXPECT_EQ("18446744073709551618", afterSettings.RenderAsString(BigIntParams()).value());
    TemplateEnv fresh;
    ASSERT_TRUE(fresh.SetBigIntBackend(big_int_toy::Backend()));
    fresh.SetSettings(changed);
    Template beforeFirstLoad(&fresh);
    ASSERT_TRUE(beforeFirstLoad.Load("{{ big + 3 }}"));
    EXPECT_EQ("18446744073709551618", beforeFirstLoad.RenderAsString(BigIntParams()).value());
    EXPECT_EQ(0, big_int_toy::LiveObjects());
}

namespace
{
// A backend that reports a status from every operation that makes an object
template<BigIntStatus Status>
const BigIntBackend* FailingBackend()
{
    static const BigIntBackend backend = [] {
        BigIntBackend b = *big_int_toy::Backend();
        b.add = [](const BigIntBackend*, void*, const void*, const void*) noexcept { return Status; };
        b.multiply = [](const BigIntBackend*, void*, const void*, const void*) noexcept { return Status; };
        return b;
    }();
    return &backend;
}
} // namespace

TEST(BigIntToyBackend, FailuresOfTheBackend)
{
    // Out of memory is std::bad_alloc, the one exception a render lets through (docs/api-2.0.md 2.1)
    TemplateEnv env;
    ASSERT_TRUE(env.SetBigIntBackend(FailingBackend<BigIntStatus::OutOfMemory>()));
    Template tpl(&env);
    ASSERT_TRUE(tpl.Load("{{ big + 1 }}"));
    EXPECT_THROW((void)tpl.RenderAsString(BigIntParams()), std::bad_alloc);
    EXPECT_EQ(ErrorCode::ResourceLimitExceeded, RenderError<char>("{{ big * 3 }}", FailingBackend<BigIntStatus::TooLarge>()));
    EXPECT_NE(ErrorCode::IntegerOutOfRange, RenderError<char>("{{ big + 1 }}", FailingBackend<BigIntStatus::Unsupported>()));
    EXPECT_EQ(0, big_int_toy::LiveObjects());
}

// Task 0543 P4: a backend with fromWords and toWords takes and gives a huge integer (past 14285 bits) as words, and a
// huge result keeps no decimal form; a table without them, or one whose structSize ends before them, converts
// through decimal text as before, with the same results
TEST(BigIntToyBackend, HugeOperandsAndResultsGoThroughWords)
{
    // h has 14401 bits; (h + 1) * 3 and the like stay huge, (h * 2) // 2 - h is small again
    const std::string hex = "1" + std::string(3599, '0') + "f";
    const std::string source = "{% set h = s|int(base=16) %}{{ (h * 2) // 2 - h }}|{{ (h + 1) is odd }}|{{ ((h * 3) % 1000000007) }}|{{ (-h - 1).bit_length() }}|{{ '{:x}'.format(h + 1)[-3:] }}|{{ (h * 3) > h }}";
    const auto render = [&source, &hex](const BigIntBackend* backend, auto charType) -> std::string {
        using CharT = decltype(charType);
        TemplateEnv env;
        EXPECT_TRUE(env.SetBigIntBackend(backend));
        std::conditional_t<std::is_same_v<CharT, char>, Template, TemplateW> tpl(&env);
        const auto loaded = tpl.Load(std::basic_string<CharT>(source.begin(), source.end()));
        EXPECT_TRUE(loaded.has_value());
        const auto result = tpl.RenderAsString({ { "s", hex } });
        if (!result.has_value())
        {
            return "error";
        }
        return std::string(result.value().begin(), result.value().end());
    };
    // From Python
    const std::string expected = "0|False|939304124|14401|010|True";
    big_int_toy::WordCalls() = 0;
    const auto withWords = render(big_int_toy::Backend(), char{});
    EXPECT_GT(big_int_toy::WordCalls(), 0);
    EXPECT_EQ(withWords, render(big_int_toy::Backend(), wchar_t{}));
    big_int_toy::WordCalls() = 0;
    EXPECT_EQ(withWords, render(big_int_toy::DecimalOnlyBackend(), char{}));
    EXPECT_EQ(withWords, render(big_int_toy::DecimalOnlyBackend(), wchar_t{}));
    EXPECT_EQ(0, big_int_toy::WordCalls());
    EXPECT_EQ(expected, withWords);

#ifndef JINJA2CPP_LINK_AS_SHARED
    // A huge result made through toWords keeps only its words (the render limits are the library's own thread-local)
    {
        const auto h = jinja2::IntegerFromDigits(hex, 16, false);
        // A render's limits, none set, with the backend
        const jinja2::Settings::Limits limits;
        jinja2::RenderLimitsScope scope(&limits, big_int_toy::Backend());
        const auto sum = jinja2::BigIntBinary(jinja2::BigIntOp::Add, jinja2::AsBigInt(h).value(), int64_t{ 1 });
        const auto view = jinja2::AsBigInt(sum).value();
        ASSERT_TRUE(view.IsHuge());
        EXPECT_TRUE(view.InlineDecimal().empty());
        EXPECT_EQ(14401U, jinja2::BitLength(view));
    }
#endif
    EXPECT_EQ(0, big_int_toy::LiveObjects());
}

namespace
{
// A backend whose toWords reports words that are not the value's magnitude: a zero top word
const BigIntBackend* ZeroTopWordBackend()
{
    static const BigIntBackend backend = [] {
        BigIntBackend b = *big_int_toy::Backend();
        b.toWords = [](const BigIntBackend*, const void*, uint32_t* out, size_t count) noexcept {
            std::fill(out, out + count, 0U);
            return BigIntStatus::Ok;
        };
        return b;
    }();
    return &backend;
}
} // namespace

TEST(BigIntToyBackend, WordsThatDoNotMatchTheBitLengthFailTheRender)
{
    TemplateEnv env;
    ASSERT_TRUE(env.SetBigIntBackend(ZeroTopWordBackend()));
    Template tpl(&env);
    ASSERT_TRUE(tpl.Load("{{ ((s|int(base=16)) + 1) is odd }}"));
    const auto result = tpl.RenderAsString({ { "s", "1" + std::string(3600, '0') } });
    ASSERT_FALSE(result.has_value());
    EXPECT_NE(std::string::npos, result.error().ToString().find("big-integer backend 'toy-decimal' failed")) << result.error().ToString();
    EXPECT_EQ(0, big_int_toy::LiveObjects());
}

// The Boost.Multiprecision adapter (docs/tasks/0272) renders every row as Python, and so does the
// built-in default of a -DJINJA2CPP_WITH_BIGINT=boost build
namespace
{
void CheckBackendRendersAsPython(const BigIntBackend* backend)
{
    for (const auto& row : BigIntCases)
    {
        CheckBigIntCase<char>(row, backend);
        CheckBigIntCase<wchar_t>(row, backend);
    }
    for (const auto& row : BigIntBackendCases)
    {
        CheckBigIntCase<char>(row, backend);
        CheckBigIntCase<wchar_t>(row, backend);
    }
}
} // namespace

TEST(BigIntBoost, RendersAsPython)
{
    const auto* boost = BoostCppIntBackend();
    EXPECT_EQ(std::string_view("boost-cpp_int"), boost->name);
    TemplateEnv env;
    EXPECT_TRUE(env.SetBigIntBackend(boost));
    CheckBackendRendersAsPython(boost);
}

TEST(BigIntBoost, Limits)
{
    const auto* boost = BoostCppIntBackend();
    EXPECT_EQ(ErrorCode::ResourceLimitExceeded, RenderError<char>("{{ 2 ** 2097152 }}", boost));
    EXPECT_EQ(ErrorCode::ResourceLimitExceeded, RenderError<char>("{{ 3 ** big }}", boost));
    EXPECT_NE(ErrorCode::IntegerOutOfRange, RenderError<char>("{{ 2 ** (small * 4000) }}", boost));
}

TEST(BigIntBoost, Operations)
{
    const auto* b = BoostCppIntBackend();
    alignas(MaxBigIntObjectAlign) unsigned char x[MaxBigIntObjectSize];
    alignas(MaxBigIntObjectAlign) unsigned char y[MaxBigIntObjectSize];
    alignas(MaxBigIntObjectAlign) unsigned char q[MaxBigIntObjectSize];
    alignas(MaxBigIntObjectAlign) unsigned char r[MaxBigIntObjectSize];
    const std::string_view text = "-18446744073709551616";
    ASSERT_EQ(BigIntStatus::Ok, b->fromDecimal(b, x, text.data(), text.size()));
    ASSERT_EQ(BigIntStatus::Ok, b->fromInt64(b, y, 7));
    // Only canonical decimal text is taken, though cpp_int itself parses hex, octal and ""
    for (const std::string_view bad : { "", "-", "0x10", "010", "-01", "+1", "1 2", "1.5" })
    {
        alignas(MaxBigIntObjectAlign) unsigned char z[MaxBigIntObjectSize];
        EXPECT_EQ(BigIntStatus::BadInput, b->fromDecimal(b, z, bad.data(), bad.size())) << bad;
    }
    EXPECT_EQ(-1, b->sign(b, x));
    EXPECT_EQ(65U, b->bitLength(b, x));
    // compare runs through the template rows (the analyzer misreads cpp_int's limb loop when called here)
    int64_t small = 0;
    EXPECT_FALSE(b->toInt64(b, x, &small));
    ASSERT_TRUE(b->toInt64(b, y, &small));
    EXPECT_EQ(7, small);
    // Truncating division: -(2^64) / 7 is -2635249153387078802 remainder -2
    ASSERT_EQ(BigIntStatus::Ok, b->divideTruncated(b, q, r, x, y));
    std::string out;
    auto sink = [](void* ctx, const char* data, size_t size) { static_cast<std::string*>(ctx)->append(data, size); };
    ASSERT_EQ(BigIntStatus::Ok, b->toDecimal(b, q, sink, &out));
    EXPECT_EQ("-2635249153387078802", out);
    ASSERT_TRUE(b->toInt64(b, r, &small));
    EXPECT_EQ(-2, small);
    b->destroy(b, q);
    b->destroy(b, r);
    // Either output may be left out, and division by zero makes neither
    ASSERT_EQ(BigIntStatus::Ok, b->divideTruncated(b, nullptr, r, x, y));
    b->destroy(b, r);
    ASSERT_EQ(BigIntStatus::Ok, b->fromInt64(b, q, 0));
    EXPECT_EQ(0U, b->bitLength(b, q));
    EXPECT_EQ(0, b->sign(b, q));
    EXPECT_EQ(BigIntStatus::DivisionByZero, b->divideTruncated(b, r, nullptr, x, q));
    b->destroy(b, q);
    b->destroy(b, y);
    b->destroy(b, x);
}

TEST(BigIntBuiltin, MatchesTheBuildOption)
{
    const auto* builtin = GetBuiltinBigIntBackend();
    EXPECT_EQ(builtin, TemplateEnv().GetBigIntBackend());
#ifdef JINJA2CPP_WITH_BIGINT_BOOST
    ASSERT_NE(nullptr, builtin);
    EXPECT_EQ(std::string_view("boost-cpp_int"), builtin->name);
    CheckBackendRendersAsPython(builtin);
    // A default environment computes beyond int64
    Template tpl;
    ASSERT_TRUE(tpl.Load("{{ 2 ** 64 }}|{{ big * big }}"));
    EXPECT_EQ("18446744073709551616|340282366920938463426481119284349108225", tpl.RenderAsString(BigIntParams()).value());
#else
    EXPECT_EQ(nullptr, builtin);
#endif
}

// BigInt in other bases (task 0357 B5)

TEST(BigIntTest, FromDigitsAndToDigitsRoundTrip)
{
    const std::string wide(2000, 'f');
    std::string wideMixed;
    for (int i = 0; i < 2000; ++i)
    {
        wideMixed.push_back("0123456789abcdef"[((i * 7) + 3) % 16]);
    }
    wideMixed[0] = '9';
    struct Row
    {
        std::string digits;
        int base;
        bool negative;
        std::string decimal;
    };
    const std::vector<Row> rows = {
        { "0", 16, false, "0" },
        { "0", 2, true, "0" },
        { "1", 2, false, "1" },
        { "1", 36, true, "-1" },
        { "ff", 16, false, "255" },
        { "zz", 36, true, "-1295" },
        { "10000000000000000", 16, false, "18446744073709551616" },
        { "10000000000000000000000000000000000000000000000000000000000000000", 2, false, "18446744073709551616" },
        { "7777777777777777777777", 8, false, "73786976294838206463" },
        { "256", 10, true, "-256" },
    };
    for (const auto& row : rows)
    {
        const auto value = BigInt::FromDigits(row.digits, row.base, row.negative);
        EXPECT_EQ(row.decimal, value.ToString()) << row.digits << " base " << row.base;
        EXPECT_EQ(row.digits.size() > 1 && row.digits[0] == '0' ? row.digits.substr(row.digits.find_first_not_of('0')) : row.digits, value.ToDigits(row.base));
    }

    // 2^200 in the three bases, via the decimal form
    const std::string pow200Dec = "1606938044258990275541962092341162602522202993782792835301376";
    BigInt pow200;
    ASSERT_TRUE(BigInt::TryParse(pow200Dec, pow200));
    EXPECT_EQ("1" + std::string(50, '0'), pow200.ToDigits(16));
    EXPECT_EQ("1" + std::string(200, '0'), pow200.ToDigits(2));
    EXPECT_EQ(pow200, BigInt::FromDigits(pow200.ToDigits(36), 36, false));
    EXPECT_EQ(pow200, BigInt::FromDigits("1" + std::string(200, '0'), 2, false));
    BigInt negPow200;
    ASSERT_TRUE(BigInt::TryParse("-" + pow200Dec, negPow200));
    EXPECT_EQ(pow200.ToDigits(16), negPow200.ToDigits(16));
    EXPECT_EQ(negPow200, BigInt::FromDigits(negPow200.ToDigits(36), 36, true));

    // Thousands of digits, past Python's decimal limit of 4300 digits
    for (const int base : { 2, 3, 8, 10, 16, 36 })
    {
        for (const auto& source : { wide, wideMixed })
        {
            std::string digits;
            for (const char c : source)
            {
                digits.push_back(c);
            }
            if (base < 16)
            {
                for (auto& c : digits)
                {
                    c = static_cast<char>('0' + (c % base));
                }
                digits[0] = '1';
            }
            const auto value = BigInt::FromDigits(digits, base, false);
            EXPECT_EQ(digits, value.ToDigits(base)) << "base " << base;
        }
    }
    const std::string wider(5000, 'f');
    const auto huge = BigInt::FromDigits(wider, 16, true);
    EXPECT_EQ(-1, huge.Sign());
    EXPECT_GT(huge.ToString().size(), 4300u);
    EXPECT_EQ(wider, huge.ToDigits(16));
    EXPECT_EQ(huge, BigInt::FromDigits(huge.ToDigits(10), 10, true));
}

// Task 0287: conversions split the number and multiply (Karatsuba) past a few dozen limbs. Every
// size from one limb to past the splitting thresholds converts both ways and agrees with Python
TEST(BigIntTest, RadixConversionsAgreeAtEverySize)
{
    uint32_t state = 12345;
    const auto next = [&state]() {
        state = (state * 1103515245U) + 12345U;
        return state >> 16;
    };
    for (const size_t size : std::vector<size_t>{ 1, 7, 8, 9, 47, 48, 49, 96, 97, 300, 385, 1000, 3000, 12000 })
    {
        std::string hex;
        for (size_t i = 0; i != size; ++i)
        {
            hex.push_back("0123456789abcdef"[next() % 16]);
        }
        hex[0] = 'c';
        const auto value = BigInt::FromDigits(hex, 16, false);
        // Back from the decimal form, which the two directions compute in different radixes
        EXPECT_EQ(hex, BigInt::FromDigits(value.ToString(), 10, false).ToDigits(16)) << size;
        for (const int base : { 2, 3, 7, 32, 36 })
        {
            EXPECT_EQ(value, BigInt::FromDigits(value.ToDigits(base), base, false)) << size << " base " << base;
        }
        // Base 2 from base 16 digit by digit
        std::string binary;
        for (const char c : hex)
        {
            const int digit = c <= '9' ? c - '0' : c - 'a' + 10;
            for (int bit = 3; bit >= 0; --bit)
            {
                binary.push_back(((digit >> bit) & 1) != 0 ? '1' : '0');
            }
        }
        EXPECT_EQ(binary, value.ToDigits(2)) << size;
    }

    // Python: str(int('0123456789abcdef' * 1250, 16)), 24081 digits
    const auto pattern = BigInt::FromDigits([] {
        std::string digits;
        for (int i = 0; i != 1250; ++i)
        {
            digits += "0123456789abcdef";
        }
        return digits;
    }(),
                                            16, false);
    const auto& decimal = pattern.ToString();
    ASSERT_EQ(24081u, decimal.size());
    EXPECT_EQ("1115502634713506683283205363772616989488", decimal.substr(0, 40));
    EXPECT_EQ("1766335204961454869936206472544267259375", decimal.substr(decimal.size() - 40));
    // Python: int('2' * 3000, 3) % 10**30
    const auto ternary = BigInt::FromDigits(std::string(3000, '2'), 3, false).ToString();
    EXPECT_EQ("483881594201769798453765660000", ternary.substr(ternary.size() - 30));
    EXPECT_EQ(std::string(3000, '2'), BigInt::FromDigits(ternary, 10, false).ToDigits(3));
}

TEST(BigIntTest, FromDigitsIsCaseInsensitiveAndDropsLeadingZeros)
{
    EXPECT_EQ(BigInt::FromDigits("DeadBEEF", 16, false), BigInt::FromDigits("deadbeef", 16, false));
    EXPECT_EQ("3735928559", BigInt::FromDigits("DEADbeef", 16, false).ToString());
    EXPECT_EQ("deadbeef", BigInt::FromDigits("DEADBEEF", 16, false).ToDigits(16));
    EXPECT_EQ("255", BigInt::FromDigits("000ff", 16, false).ToString());
    EXPECT_EQ("0", BigInt::FromDigits("0000", 16, true).ToString());
    EXPECT_EQ(0, BigInt::FromDigits("0", 2, true).Sign());
    EXPECT_EQ("0", BigInt().ToDigits(36));
    EXPECT_EQ("z", BigInt(35).ToDigits(36));
    EXPECT_EQ("10", BigInt(36).ToDigits(36));
}

TEST(BigIntTest, FromDigitsAndToDigitsRejectBadInput)
{
    EXPECT_THROW(BigInt::FromDigits("1", 1, false), std::invalid_argument);
    EXPECT_THROW(BigInt::FromDigits("1", 37, false), std::invalid_argument);
    EXPECT_THROW(BigInt::FromDigits("1", 0, false), std::invalid_argument);
    EXPECT_THROW(BigInt::FromDigits("1", -16, false), std::invalid_argument);
    EXPECT_THROW(BigInt::FromDigits("", 10, false), std::invalid_argument);
    EXPECT_THROW(BigInt::FromDigits("2", 2, false), std::invalid_argument);
    EXPECT_THROW(BigInt::FromDigits("g", 16, false), std::invalid_argument);
    EXPECT_THROW(BigInt::FromDigits("-1", 10, false), std::invalid_argument);
    EXPECT_THROW(BigInt::FromDigits("+1", 10, false), std::invalid_argument);
    EXPECT_THROW(BigInt::FromDigits("0x1", 16, false), std::invalid_argument);
    EXPECT_THROW(BigInt::FromDigits("1_0", 10, false), std::invalid_argument);
    EXPECT_THROW(BigInt::FromDigits(" 1", 10, false), std::invalid_argument);
    EXPECT_THROW(BigInt::FromDigits("1\xe9", 36, false), std::invalid_argument);
    EXPECT_THROW(BigInt::FromDigits(std::string(300000, '1'), 16, false), std::length_error);
    // Past the estimate by a few bits: 315653 nines are 1048577 bits, one over the cap
    EXPECT_THROW(BigInt::FromDigits(std::string(315653, '9'), 10, false), std::length_error);
    EXPECT_THROW(static_cast<void>(BigInt(5).ToDigits(1)), std::invalid_argument);
    EXPECT_THROW(static_cast<void>(BigInt(5).ToDigits(37)), std::invalid_argument);
}

// A view of a value reads its object, and the value made back from it shares that object rather
// than copying the digits (docs/tasks/0543 P1a): unary +, a dict key, abs() of a positive integer
TEST(BigIntView, ValueOfAViewSharesTheObject)
{
    const auto value = jinja2::MakeBigIntValue("123456789012345678901234567890");
    ASSERT_TRUE(jinja2::AsBigInt(value).has_value());
    const auto view = jinja2::AsBigInt(value).value();
    ASSERT_NE(nullptr, view.Object());
    EXPECT_EQ("123456789012345678901234567890", view.InlineDecimal());
    const auto copy = jinja2::BigIntValue(view);
    ASSERT_TRUE(jinja2::AsBigInt(copy).has_value());
    EXPECT_EQ(view.Object(), jinja2::AsBigInt(copy).value().Object());

    const std::string text = "-98765432109876543210";
    const auto textView = jinja2::BigIntView::FromText(text);
    EXPECT_EQ(nullptr, textView.Object());
    EXPECT_EQ(-1, textView.Sign());
    const auto made = jinja2::BigIntValue(textView);
    ASSERT_TRUE(jinja2::AsBigInt(made).has_value());
    const auto madeView = jinja2::AsBigInt(made).value();
    EXPECT_NE(nullptr, madeView.Object());
    EXPECT_EQ(text, madeView.InlineDecimal());
}

// A view of text has no object, so it may never be a huge integer: its precondition caps it at
// MaxIntStrDigits digits (docs/tasks/0543 B2)
TEST(BigIntView, TextViewIsCappedAtTheStrDigitLimit)
{
    const std::string atLimit = "-" + std::string(jinja2::MaxIntStrDigits, '9');
    EXPECT_NO_THROW(static_cast<void>(jinja2::BigIntView::FromText(atLimit)));
    if constexpr (jinja2::detail::ContractChecks >= 1)
    {
        const std::string past(jinja2::MaxIntStrDigits + 1, '9');
        EXPECT_THROW(static_cast<void>(jinja2::BigIntView::FromText(past)), jinja2::detail::ContractViolation);
    }
}

// Task 0287: a power-of-two literal at the bit cap converts in well under a second in Release
// (it took seconds when every conversion was quadratic), and the exact width of one past the cap
// is known before converting. The numbers are Python's
TEST(BigIntTemplate, RadixLiteralsAtTheBitCap)
{
    // A constant this wide fails the Load, as Jinja2's repr() of it does (docs/tasks/0564): the value comes from data
    Template tpl;
    ASSERT_TRUE(tpl.Load("{% set x = s|int(base=16) %}{% set h = '{:x}'.format(-x) %}{{ x.bit_length() }}|{{ h[:4] }}|{{ h|length }}"));
    const auto result = tpl.RenderAsString({ { "s", std::string(262144, 'f') } });
    ASSERT_TRUE(result.has_value()) << result.error().ToString();
    EXPECT_EQ("1048576|-fff|262145", result.value());

    const auto loadError = [](const std::string& source) {
        Template other;
        const auto loaded = other.Load(source);
        return loaded ? ErrorCode::Unspecified : loaded.error().GetCode();
    };
    // 1 + 3 * 349525 = 1048576 bits fit, in a fold that takes the literal away; a leading 2 is one bit more
    EXPECT_EQ(ErrorCode::Unspecified, loadError("{{ (0o1" + std::string(349525, '0') + ") is odd }}"));
    EXPECT_EQ(ErrorCode::TemplateRuntimeError, loadError("{{ 0o1" + std::string(349525, '0') + " }}"));
    EXPECT_EQ(ErrorCode::ResourceLimitExceeded, loadError("{{ 0o2" + std::string(349525, '0') + " }}"));
    EXPECT_EQ(ErrorCode::ResourceLimitExceeded, loadError("{{ 0x1" + std::string(262144, '0') + " }}"));
    EXPECT_EQ(ErrorCode::ResourceLimitExceeded, loadError("{{ 0b1" + std::string(1048576, '0') + " }}"));

    // The int filter takes the same path from data
    TemplateW wide;
    ASSERT_TRUE(wide.Load(L"{{ (s|int(base=16)).bit_length() }}"));
    EXPECT_EQ(L"4096", wide.RenderAsString({ { "s", std::string(1024, 'F') } }).value());
    const auto over = wide.RenderAsString({ { "s", "1" + std::string(262144, '0') } });
    ASSERT_FALSE(over.has_value());
    EXPECT_EQ(ErrorCode::ResourceLimitExceeded, over.error().GetCode());
}

// Task 0564: Jinja2's code generator writes each constant left after its optimizer with repr(), which raises
// ValueError past 4300 digits, also where the constant never renders; a fold around it takes it away. Expected
// from Jinja2 3.1.6 on CPython 3.12
TEST(BigIntTemplate, StrDigitsConstantsFailTheLoad)
{
    const std::string past = "0x" + std::string(3572, 'f');
    const auto loads = [](const std::string& source, const BigIntBackend* backend = nullptr) {
        TemplateEnv env;
        env.SetBigIntBackend(backend);
        Template tpl(&env);
        const auto loaded = tpl.Load(source);
        if (!loaded)
        {
            EXPECT_EQ(ErrorCode::TemplateRuntimeError, loaded.error().GetCode()) << source.substr(0, 60);
            EXPECT_NE(std::string::npos, loaded.error().ToString().find("Exceeds the limit (4300 digits) for integer string conversion")) << loaded.error().ToString().substr(0, 200);
        }
        return loaded.has_value();
    };
    // 3571 f's are 4300 digits, 3572 are 4302; 14285 bits are 4301 digits, with a leading 1 and zeros 4300
    EXPECT_TRUE(loads("{% set x = 0x" + std::string(3571, 'f') + " %}"));
    EXPECT_FALSE(loads("{% set x = " + past + " %}"));
    EXPECT_TRUE(loads("{% set x = 0b1" + std::string(14284, '0') + " %}"));
    EXPECT_FALSE(loads("{% set x = 0b" + std::string(14285, '1') + " %}"));
    // Unrendered, in a comparison, a call, a list or a filter argument: no fold takes it away
    for (const auto& source : { "{% if false %}{{ " + past + " }}{% endif %}", "{{ x == " + past + " }}", "{{ (" + past + ").bit_length() }}",
                                "{{ [" + past + "] }}", "{{ x|default(" + past + ") }}", "{% macro m() %}{{ -" + past + " }}{% endmacro %}" })
    {
        EXPECT_FALSE(loads(source)) << source.substr(0, 60);
    }
    // A fold around it takes it away
    for (const auto& source : { "{{ (" + past + ") is odd }}", "{{ ((" + past + ") is odd) and x }}", "{{ [" + past + "]|length }}", "{{ '%x'|format(" + past + ")|length }}",
                                "{{ true or " + past + " }}", "{{ (" + past + ") % 7 }}" })
    {
        EXPECT_TRUE(loads(source)) << source.substr(0, 60);
    }
    // A folded filter or operator gives one: no literal is too long
    EXPECT_FALSE(loads("{% set x = ('f' * 3600)|int(base=16) %}"));
    EXPECT_TRUE(loads("{% set x = ('f' * 3571)|int(base=16) %}"));
    EXPECT_TRUE(loads("{% set x = (('f' * 3600)|int(base=16)) % 10 %}"));
    // `(2 ** 20000) ~ x` needs a backend to compute the power; without one, it overflows when rendered
    EXPECT_TRUE(loads("{{ x ~ (2 ** 20000) }}"));
    EXPECT_FALSE(loads("{{ x ~ (2 ** 20000) }}", big_int_toy::Backend()));
    EXPECT_FALSE(loads("{{ 2 ** 20000 }}", big_int_toy::Backend()));
    EXPECT_FALSE(loads("{{ (10 ** 2000) * (10 ** 2400) }}", big_int_toy::Backend()));
    EXPECT_FALSE(loads("{{ 0x" + std::string(3571, 'f') + " * 16 }}", big_int_toy::Backend()));
    EXPECT_TRUE(loads("{{ 10 ** 4299 }}", big_int_toy::Backend()));
    EXPECT_TRUE(loads("{{ (2 ** 20000) % 7 }}", big_int_toy::Backend()));
    EXPECT_TRUE(loads("{{ false and 2 ** 20000 }}", big_int_toy::Backend()));
    EXPECT_TRUE(loads("{{ 2 ** (small * 4000) }}", big_int_toy::Backend()));
    // In volatile code Jinja2 folds nothing: a power stays an operator (verifier round 1; a statement's expression
    // there is docs/tasks/0570)
    EXPECT_TRUE(loads("{% autoescape x %}{{ [2 ** 20000]|length }}{% if 0 %}{{ 2 ** 20000 }}{% endif %}{% endautoescape %}", big_int_toy::Backend()));
    // and a statement's expressions there neither (docs/tasks/0570); a block's body is not volatile, as Jinja2
    // compiles it on its own, and neither is a constant autoescape
    for (const char* source : { "{% autoescape x %}{% set q = 10 ** 4300 %}{% endautoescape %}", "{% autoescape x %}{% if 10 ** 4300 %}{% endif %}{% endautoescape %}",
                                "{% autoescape x %}{% for i in [10 ** 4300] %}{% endfor %}{% endautoescape %}", "{% autoescape x %}{% with q = 10 ** 4300 %}{% endwith %}{% endautoescape %}",
                                "{% autoescape x %}{% macro m(a=10 ** 4300) %}{% endmacro %}{% endautoescape %}",
                                "{% autoescape x %}{% filter upper %}{% set q = 10 ** 4300 %}{% endfilter %}{% endautoescape %}" })
    {
        EXPECT_TRUE(loads(source, big_int_toy::Backend())) << source;
    }
    for (const char* source : { "{% set q = 10 ** 4300 %}", "{% autoescape x %}{% block b %}{% set q = 10 ** 4300 %}{% endblock %}{% endautoescape %}",
                                "{% autoescape true %}{% set q = 10 ** 4300 %}{% endautoescape %}" })
    {
        EXPECT_FALSE(loads(source, big_int_toy::Backend())) << source;
    }
    // str() of an output refuses the item past the limit whatever the other items are
    EXPECT_FALSE(loads("{% if false %}{{ [(1).y, " + past + "] }}{% endif %}"));
    // A filter's value error in an `and` around a watched product is Impossible, not a Load error
    EXPECT_TRUE(loads("{% set x = 1 %}{% if false %}{{ 2 * 3 and ('abc'|abs or x) }}{% endif %}"));
    // A step limit that the repr walk of a watched product reaches leaves it unfolded, as one its evaluation reaches
    // (Codex on #987)
    TemplateEnv limited;
    Settings withLimits;
    withLimits.limits.maxRenderSteps = 1000;
    limited.SetSettings(withLimits);
    for (const char* source : { "{% if false %}{% set q = [1] * (100 * 100) %}{% endif %}", "{% if false %}{{ [1] * (100 * 100) }}{% endif %}" })
    {
        Template tpl(&limited);
        const auto loaded = tpl.Load(source);
        EXPECT_TRUE(loaded.has_value()) << source << ": " << loaded.error().ToString();
        EXPECT_EQ("", tpl.RenderAsString({}).value());
    }
    EXPECT_EQ(0, big_int_toy::LiveObjects());
}

// Task 0569: Jinja2 folds constants when it compiles, in code that never renders too, so the Load's fold check
// computes them; without limits of the template's own it does under a budget (FoldBudget), past which a candidate
// does not fold and its value is left to the render
// Task 0588: in the fold check a big-integer product, power or division takes steps for its operands' words, so
// that roots of big arithmetic spend the Load's steps as other work does. Thirty roots of ten products of 15850-bit
// powers spend them, and the tuple-slice subscript after them, which needs more than a root's own steps to fold,
// then fails the Load; the same roots on small powers do not. The 0588 template (a hundred roots of ten products of 31700-bit powers) took 7.5 s to load (Release)
TEST(BigIntTemplate, FoldBudgetChargesBigIntWork)
{
    const auto loadError = [](const std::string& source) {
        TemplateEnv env;
        env.SetBigIntBackend(BoostCppIntBackend());
        Template tpl(&env);
        const auto loaded = tpl.Load(source);
        if (!loaded)
        {
            return loaded.error().ToString().substr(0, 200);
        }
        const auto rendered = tpl.RenderAsString({});
        return rendered ? rendered.value() : "render: " + rendered.error().ToString().substr(0, 200);
    };
    const auto roots = [](int count, const std::string& power) {
        std::string term = "((" + power + " * " + power + ") % 7)";
        std::string sum = term;
        for (int idx = 1; idx != 10; ++idx)
        {
            sum += " + " + term;
        }
        std::string result;
        for (int root = 0; root != count; ++root)
        {
            result += "{% if false %}{{ " + sum + " }}{% endif %}";
        }
        return result;
    };
    // A fold that needs more than a root's own steps, as in FoldBudgetBoundsDeadConstants
    std::string chain = "(2 * 3)";
    for (int filter = 0; filter != 300; ++filter)
    {
        chain += "|abs";
    }
    const std::string folding = "{{ ()[:, :] ~ (" + chain + ") }}";
    EXPECT_EQ("6", loadError(roots(30, "3 ** 100") + folding));
    EXPECT_NE(std::string::npos, loadError(roots(30, "3 ** 10000") + folding).find("']' expected"));
    EXPECT_EQ("ok", loadError(roots(100, "3 ** 20000") + "ok"));
}

TEST(BigIntTemplate, FoldBudgetBoundsDeadConstants)
{
    const auto loadError = [](const std::string& source, const Settings* settings = nullptr) {
        TemplateEnv env;
        env.SetBigIntBackend(big_int_toy::Backend());
        if (settings)
        {
            env.SetSettings(*settings);
        }
        Template tpl(&env);
        const auto loaded = tpl.Load(source);
        if (!loaded)
        {
            return loaded.error().ToString().substr(0, 200);
        }
        const auto rendered = tpl.RenderAsString({});
        return rendered ? rendered.value() : "render: " + rendered.error().ToString().substr(0, 200);
    };
    const auto fails = [](const std::string& error, const char* what) { return error.find(what) != std::string::npos; };
    const std::string past = "0x" + std::string(3572, 'f');
    // A power past 2^16 bits is not computed: this one took a second. One past the 4300-digit limit then stays
    // unfolded, and fails the Load only under the budget's bits
    EXPECT_EQ("ok", loadError("{% if false %}{{ 1 if 3 ** 600000 else 2 }}{% endif %}ok"));
    EXPECT_EQ("ok", loadError("{% if false %}{{ 10 ** 30000 }}{% endif %}ok"));
    EXPECT_TRUE(fails(loadError("{% if false %}{{ (" + past + ") * 1 }}{% endif %}ok"), "Exceeds the limit (4300 digits)"));
    // A repetition past 2^16 items is not built: 100 of these took 64 MB each
    std::string repeated;
    for (int root = 0; root != 100; ++root)
    {
        repeated += "{% if false %}{{ ('a'|upper) * 67108864 }}{% endif %}";
    }
    EXPECT_EQ("ok", loadError(repeated + "ok"));
    // Each root has steps of its own for its literals, however many roots come before
    std::string cheap;
    for (int root = 0; root != 2000; ++root)
    {
        cheap += "{% if false %}{{ 2 * 3 }}{% endif %}";
    }
    EXPECT_TRUE(fails(loadError(cheap + "{% if false %}{{ " + past + " }}{% endif %}ok"), "Exceeds the limit (4300 digits)"));
    // Past them a root draws on the Load's steps: once roots before it spent them, it does not fold. A tuple-slice
    // subscript it should fold away then fails the Load, as one past the budget's sizes does
    std::string chain = "(2 * 3)";
    for (int filter = 0; filter != 300; ++filter)
    {
        chain += "|abs";
    }
    const std::string folding = "{{ ()[:, :] ~ (" + chain + ") }}";
    EXPECT_EQ("6", loadError(folding));
    std::string spent;
    for (int root = 0; root != 60; ++root)
    {
        spent += "{% if false %}{{ " + chain + " }}{% endif %}";
    }
    EXPECT_TRUE(fails(loadError(spent + folding), "']' expected"));
    EXPECT_EQ("a", loadError("{{ ()[:, :] ~ ('a' * 1000)|first }}"));
    EXPECT_TRUE(fails(loadError("{{ ()[:, :] ~ ('a' * 100000)|first }}"), "']' expected"));
    // Under StrictUndefined every filter of a constant is a candidate: one the budget stops neither folds nor fails
    // the Load as the undefined it would have reached (verifier round 1)
    Settings strict;
    strict.undefinedPolicy = UndefinedPolicy::Strict;
    std::string items = "'a'|upper";
    for (int item = 1; item != 2000; ++item)
    {
        items += ", 'a'|upper";
    }
    EXPECT_EQ("ok", loadError("{% set v = [" + items + "] %}ok", &strict));
    EXPECT_EQ("ok", loadError(spent + "{% set v = [" + items.substr(0, 9 + (11 * 300)) + "] %}ok", &strict));
    EXPECT_TRUE(fails(loadError("{% if false %}{{ " + past + " }}{% endif %}{% set v = [" + items + "] %}ok", &strict), "Exceeds the limit (4300 digits)"));
    // A size limit stops only the candidate that reaches it: a fold beside it still folds, and a strict undefined
    // after it still fails the Load (verifier round 2)
    const std::string big = "('a' * 100000)|first";
    EXPECT_EQ("['a', 'a']", loadError("{{ [" + big + ", ()[:, :] ~ 'a'] }}"));
    EXPECT_EQ("['a', 'a']", loadError("{{ [()[:, :] ~ 'a', " + big + "] }}"));
    EXPECT_EQ("[True, 'a']", loadError("{{ [(" + past + ") is odd, " + big + "] }}"));
    EXPECT_EQ("aTrue", loadError("{{ " + big + " ~ ((" + past + ") is odd) }}"));
    EXPECT_TRUE(fails(loadError("{{ " + big + " ~ ([1][5] ~ 'x') }}", &strict), "a constant expression gives a strict undefined"));
    // Steps counted only to poll the host (UINT64_MAX, the Python module's default) are no limit of its own
    Settings polled;
    polled.limits.maxRenderSteps = UINT64_MAX;
    EXPECT_EQ("ok", loadError("{% if false %}{{ 10 ** 30000 }}{% endif %}ok", &polled));
    // A template with limits of its own folds under those, as it renders
    Settings limited;
    limited.limits.maxRenderSteps = 1000000;
    EXPECT_EQ("a", loadError("{{ ()[:, :] ~ ('a' * 100000)|first }}", &limited));
    EXPECT_EQ("6", loadError(spent + folding, &limited));
    EXPECT_EQ(0, big_int_toy::LiveObjects());
}

// Task 0543 P1b: an integer is huge, and keeps binary words, exactly when it has more than
// HugeBigIntBits bits, whichever form it was made from; the words agree with its decimal form
TEST(BigIntView, HugeExactlyPastTheHugeBitWidth)
{
    const auto fromHex = [](const std::string& digits) { return jinja2::IntegerFromDigits(digits, 16, false); };
    const auto viewOf = [](const jinja2::InternalValue& value) { return jinja2::AsBigInt(value).value(); };

    // 2^14285 - 1 (4301 digits) is not huge, 2^14285 is: from binary digits and from decimal text
    const auto below = jinja2::IntegerFromDigits(std::string(14285, '1'), 2, false);
    const auto at = jinja2::IntegerFromDigits("1" + std::string(14285, '0'), 2, false);
    EXPECT_FALSE(viewOf(below).IsHuge());
    EXPECT_EQ(4301U, viewOf(below).InlineDecimal().size());
    ASSERT_TRUE(viewOf(at).IsHuge());
    EXPECT_EQ(447U, viewOf(at).Words().size());
    EXPECT_EQ(1U << 13, viewOf(at).Words()[446]); // 14285 = 446 * 32 + 13
    const auto belowFromText = jinja2::MakeBigIntValue(viewOf(below).InlineDecimal());
    const auto atFromText = jinja2::WithDecimal(viewOf(at), [](std::string_view decimal) { return jinja2::MakeBigIntValue(decimal); });
    EXPECT_FALSE(viewOf(belowFromText).IsHuge());
    ASSERT_TRUE(viewOf(atFromText).IsHuge());
    EXPECT_TRUE(std::equal(viewOf(at).Words().begin(), viewOf(at).Words().end(), viewOf(atFromText).Words().begin(), viewOf(atFromText).Words().end()));
    EXPECT_EQ(0, jinja2::CompareBigInts(viewOf(at), viewOf(atFromText)));
    EXPECT_EQ(1, jinja2::CompareBigInts(viewOf(at), viewOf(below)));
    EXPECT_EQ(-1, jinja2::CompareBigInts(viewOf(below), viewOf(at)));

    // Negation keeps the words and flips the sign, and of the decimal form when the integer keeps one
    const auto huge = fromHex(std::string(3600, 'f'));
    const auto negated = jinja2::NegateBigInt(viewOf(huge));
    ASSERT_TRUE(viewOf(negated).IsHuge());
    EXPECT_EQ(-1, viewOf(negated).Sign());
    EXPECT_TRUE(viewOf(negated).InlineDecimal().empty());
    const auto decimalOf = [](const jinja2::InternalValue& value) { return jinja2::WithDecimal(jinja2::AsBigInt(value).value(), [](std::string_view decimal) { return std::string(decimal); }); };
    EXPECT_EQ(decimalOf(huge), decimalOf(negated).substr(1));
    const auto hugeFromText = jinja2::MakeBigIntValue(decimalOf(huge));
    const auto negatedFromText = jinja2::NegateBigInt(viewOf(hugeFromText));
    EXPECT_EQ('-', viewOf(negatedFromText).InlineDecimal().front());
    EXPECT_EQ(viewOf(hugeFromText).InlineDecimal(), viewOf(negatedFromText).InlineDecimal().substr(1));
    EXPECT_EQ(0, jinja2::CompareBigInts(viewOf(negated), viewOf(negatedFromText)));
    EXPECT_EQ(-1, jinja2::CompareBigInts(viewOf(negated), viewOf(huge)));
    EXPECT_EQ(1, jinja2::CompareBigInts(viewOf(huge), viewOf(negated)));
    EXPECT_EQ(14400U, jinja2::BitLength(viewOf(negated)));
    EXPECT_EQ(14400U, jinja2::BitCount(viewOf(negated)));
    EXPECT_TRUE(jinja2::IsOdd(viewOf(negated)));
    EXPECT_THROW(static_cast<void>(jinja2::CheckedDecimal(viewOf(huge))), std::runtime_error);
    EXPECT_THROW(static_cast<void>(jinja2::BigIntToDouble(viewOf(huge))), std::overflow_error);
}

// Task 0543 P2: a huge integer made from binary digits keeps only its words, and builds its
// decimal form for each WithDecimal call, which agrees with the public BigInt's conversion; one
// made from decimal text keeps that text
TEST(BigIntView, HugeFromBinaryKeepsNoDecimal)
{
    const auto viewOf = [](const jinja2::InternalValue& value) { return jinja2::AsBigInt(value).value(); };
    const std::string hex = "1" + std::string(3599, '0') + "f";
    const auto expected = "-" + BigInt::FromDigits(hex, 16, false).ToDigits(10);
    const auto fromHex = jinja2::IntegerFromDigits(hex, 16, true);
    ASSERT_TRUE(viewOf(fromHex).IsHuge());
    EXPECT_TRUE(viewOf(fromHex).InlineDecimal().empty());
    EXPECT_EQ(expected, jinja2::WithDecimal(viewOf(fromHex), [](std::string_view decimal) { return std::string(decimal); }));
    EXPECT_TRUE(jinja2::IsOdd(viewOf(fromHex)));
    EXPECT_EQ(14401U, jinja2::BitLength(viewOf(fromHex)));

    const auto fromText = jinja2::MakeBigIntValue(expected);
    ASSERT_TRUE(viewOf(fromText).IsHuge());
    EXPECT_EQ(expected, viewOf(fromText).InlineDecimal());
    EXPECT_EQ(0, jinja2::CompareBigInts(viewOf(fromHex), viewOf(fromText)));

    // The form made from words and a host BigInt (kept in decimal) are one dict key and equal
    BigInt hostBig;
    ASSERT_TRUE(BigInt::TryParse(expected, hostBig));
    Template tpl;
    ASSERT_TRUE(tpl.Load("{% set h = s|int(base=16) %}{{ {h: 1}[t] }}|{{ {t: 2}[h] }}|{{ h == t }}|{{ -(-h) == h }}"));
    const auto result = tpl.RenderAsString({ { "s", "-" + hex }, { "t", hostBig } });
    ASSERT_TRUE(result.has_value()) << result.error().ToString();
    EXPECT_EQ("1|2|True|True", result.value());
    TemplateW wide;
    ASSERT_TRUE(wide.Load(L"{% set h = s|int(base=16) %}{{ {h: 1}[t] }}|{{ (-h)|abs == -t }}|{{ h|round(-4336) }}|{{ h|round(-10) is odd }}"));
    const auto wideResult = wide.RenderAsString({ { "s", "-" + hex }, { "t", hostBig } });
    ASSERT_TRUE(wideResult.has_value()) << wideResult.error().ToString();
    EXPECT_EQ(L"1|True|0|False", wideResult.value());
}

// Task 0543 P2: a cap-sized integer from data costs its words, not its decimal form, against the
// memory budget. Bit operations, negation and the hex digits (128 KB each, then 256 KB) fit in
// 640 KB; round(x, -1) and leaving the render need the decimal form (about 315 KB of digits and
// up to 896 KB for the converter's words next to x), which does not. round(x, -n) past the
// value's digits is 0 without converting
TEST(BigIntTemplate, HugeFromBinaryBuildsDecimalWithinTheBudget)
{
    TemplateEnv env;
    Settings settings;
    settings.limits.maxRenderMemory = std::uint64_t{ 640 } * 1024;
    env.SetSettings(settings);
    const ValuesMap params{ { "s", std::string(262144, 'f') } };
    const auto render = [&env, &params](const std::string& source) {
        Template tpl(&env);
        const auto loaded = tpl.Load(source);
        EXPECT_TRUE(loaded.has_value()) << source;
        return tpl.RenderAsString(params);
    };
    const auto ok = render("{% set x = s|int(base=16) %}{% set y = -x %}{{ x.bit_length() }}|{{ x.bit_count() }}|{{ '{:x}'.format(y)|length }}|{{ y is odd }}|{{ y < x }}");
    ASSERT_TRUE(ok.has_value()) << ok.error().ToString();
    EXPECT_EQ("1048576|1048576|262145|True|True", ok.value());
    const auto roundedAway = render("{% set x = s|int(base=16) %}{{ x|round(-315700, 'common') }}|{{ (-x)|round(-400000, 'common') }}|{{ x|round(-9223372036854775807 - 1) }}|{{ (s[:20]|int(base=16))|round(-9223372036854775807 - 1) }}");
    ASSERT_TRUE(roundedAway.has_value()) << roundedAway.error().ToString();
    EXPECT_EQ("0|0|0|0", roundedAway.value());

    for (const auto* source : { "{% set x = s|int(base=16) %}{{ (x|round(-1, 'common')) is odd }}", "{{ f(s|int(base=16)) }}" })
    {
        Template tpl(&env);
        ASSERT_TRUE(tpl.Load(source)) << source;
        auto withHost = params;
        withHost["f"] = MakeCallable([](const Value& val) { return val.IsBigInt() ? 1 : 0; }, ArgInfo{ "val" });
        const auto failed = tpl.RenderAsString(withHost);
        ASSERT_FALSE(failed.has_value()) << source;
        EXPECT_EQ(ErrorCode::ResourceLimitExceeded, failed.error().GetCode()) << source << ": " << failed.error().ToString();
    }
}

// Task 0566: the item of a host container converts from decimal on every access; a render now holds the last
// four huge ones it converted
namespace
{
// 2^262143 + 12345: 78914 digits, 262144 bits
BigInt HostHuge262144()
{
    return BigInt::FromDigits("8" + std::string(65531, '0') + "3039", 16, false);
}

// A huge integer of 4400 digits whose first digit is `lead` and whose digits are otherwise 1
std::string HugeText(char lead)
{
    return std::string(1, lead) + std::string(4399, '1');
}
} // namespace

TEST(BigIntTemplate, HugeHostItemConvertsOncePerRender)
{
    TemplateEnv env;
    Settings settings;
    settings.limits.maxRenderMemory = std::uint64_t{ 640 } * 1024;
    env.SetSettings(settings);
    const ValuesMap params{ { "l", ValuesList{ HostHuge262144() } } };
    const auto render = [&env, &params](const std::string& source) {
        Template tpl(&env);
        const auto loaded = tpl.Load(source);
        EXPECT_TRUE(loaded.has_value()) << source;
        return tpl.RenderAsString(params);
    };
    // (a) Twenty values of 111 KB each, held by the list, would pass the budget; one shared object does not
    const auto held = render("{% set acc = [] %}{% for i in range(20) %}{{ acc.append(l[0]) }}{% endfor %}{{ acc|length }}|{{ acc[19] == l[0] }}|{{ acc[0].bit_length() }}");
    ASSERT_TRUE(held.has_value()) << held.error().ToString();
    std::string expected;
    for (int n = 0; n != 20; ++n)
    {
        expected += "None";
    }
    EXPECT_EQ(expected + "20|True|262144", held.value());
    // The same twenty values from another item each each convert: they do not fit
    const ValuesMap distinct{ { "l", ValuesList{ HostHuge262144() } }, { "m", ValuesList{ BigInt::FromDigits("8" + std::string(65531, '0') + "303a", 16, false), BigInt::FromDigits("8" + std::string(65531, '0') + "303b", 16, false), BigInt::FromDigits("8" + std::string(65531, '0') + "303c", 16, false), BigInt::FromDigits("8" + std::string(65531, '0') + "303d", 16, false), BigInt::FromDigits("8" + std::string(65531, '0') + "303e", 16, false), BigInt::FromDigits("8" + std::string(65531, '0') + "303f", 16, false) } } };
    Template tooMany(&env);
    ASSERT_TRUE(tooMany.Load("{% set acc = [] %}{% for x in m %}{{ acc.append(x) }}{% endfor %}"));
    const auto failed = tooMany.RenderAsString(distinct);
    ASSERT_FALSE(failed.has_value());
    EXPECT_EQ(ErrorCode::ResourceLimitExceeded, failed.error().GetCode());

    // (b) Twenty accesses take about the time of one: before, twenty times as long (the done-when of 0566)
    TemplateEnv plain;
    const auto timed = [&plain, &params](int count) {
        Template tpl(&plain);
        EXPECT_TRUE(tpl.Load("{% for i in range(" + std::to_string(count) + ") %}{{ l[0] > 0 }}{% endfor %}").has_value());
        auto best = std::chrono::steady_clock::duration::max();
        for (int n = 0; n != 3; ++n)
        {
            const auto start = std::chrono::steady_clock::now();
            const auto result = tpl.RenderAsString(params);
            best = std::min(best, std::chrono::steady_clock::now() - start);
            EXPECT_TRUE(result.has_value());
        }
        return std::chrono::duration<double, std::milli>(best).count();
    };
    const double one = timed(1);
    const double twenty = timed(20);
    RecordProperty("one_access_ms", std::to_string(one));
    RecordProperty("twenty_accesses_ms", std::to_string(twenty));
    // Recorded, not asserted: a test does not depend on timing (REVIEW.md). (a) pins the sharing
}

#ifndef JINJA2CPP_LINK_AS_SHARED
// (c) The cache is the running render's collector's: its objects are shared there, charged to the budget that paid,
// not shared into a render without that budget, and given back to it when they leave the cache
TEST(BigIntTemplate, HostBigIntValueSharesWithinTheCollector)
{
    const std::string text = HugeText('7');
    const auto objectOf = [](const InternalValue& value) { return AsBigInt(value).value().Object(); };
    {
        const auto first = HostBigIntValue(text);
        const auto second = HostBigIntValue(text);
        ASSERT_NE(nullptr, objectOf(first));
        EXPECT_NE(objectOf(first), objectOf(second)) << "no collector: no cache";
    }
    CycleCollector collector;
    {
        const auto first = HostBigIntValue(text);
        const auto second = HostBigIntValue("-" + text);
        const auto third = HostBigIntValue(text);
        EXPECT_EQ(objectOf(first), objectOf(third));
        EXPECT_NE(objectOf(first), objectOf(second));
        EXPECT_FALSE(objectOf(first)->IsCharged());
        EXPECT_TRUE(objectOf(second)->IsNegative());
        // Only the huge are held: a value that is not (4300 digits) is made again
        const auto small = std::string(4300, '9');
        EXPECT_NE(objectOf(HostBigIntValue(small)), objectOf(HostBigIntValue(small)));
    }
    Settings::Limits limits;
    limits.maxRenderMemory = std::uint64_t{ 1 } << 20;
    const RenderLimitsScope scope(&limits, nullptr);
    const auto budget = detail::t_renderLimits.memoryBudget;
    ASSERT_EQ(budget, detail::t_renderLimits.memoryLeft);
    {
        const auto counted = HostBigIntValue(text);
        EXPECT_TRUE(objectOf(counted)->IsCharged());
        const auto bytes = objectOf(counted)->ChargedBytes();
        EXPECT_EQ(budget - bytes, detail::t_renderLimits.memoryLeft);
        EXPECT_EQ(objectOf(counted), objectOf(HostBigIntValue(text)));
        EXPECT_EQ(budget - bytes, detail::t_renderLimits.memoryLeft) << "a shared object is charged once";
        {
            // Uncounted: the object the budget paid for is not for a snapshot of the globals
            const UncountedMemoryScope uncounted;
            const auto outside = HostBigIntValue(text);
            EXPECT_NE(objectOf(counted), objectOf(outside));
            EXPECT_FALSE(objectOf(outside)->IsCharged());
            EXPECT_EQ(objectOf(outside), objectOf(HostBigIntValue(text)));
        }
        EXPECT_EQ(objectOf(counted), objectOf(HostBigIntValue(text)));
    }
    // The cache alone holds it now: still charged, and credited when it lets go. Four more take the four slots: the
    // two uncharged ones and the charged one go, and the budget holds the four new ones alone
    EXPECT_LT(detail::t_renderLimits.memoryLeft, budget);
    std::uint64_t four = 0;
    for (const char lead : { '1', '2', '3', '4' })
    {
        four += objectOf(HostBigIntValue(HugeText(lead)))->ChargedBytes();
    }
    EXPECT_EQ(budget - four, detail::t_renderLimits.memoryLeft);
}
#endif
