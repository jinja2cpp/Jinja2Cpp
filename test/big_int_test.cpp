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

#include <cstddef>
#include <cmath>
#include <cstdint>
#include <limits>
#include <new>
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
};

ValuesMap BigIntParams()
{
    BigInt nbig;
    BigInt::TryParse("-18446744073709551615", nbig);
    return { { "big", std::numeric_limits<uint64_t>::max() }, { "nbig", nbig }, { "nan", std::nan("") }, { "inf", HUGE_VAL }, { "small", BigInt(5) } };
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
    // Printing more than 4300 digits is Python's ValueError; computing with them is fine
    EXPECT_NE(ErrorCode::IntegerOutOfRange, RenderError<char>("{{ 2 ** 20000 }}", toy));
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
    EXPECT_TRUE(rejected([](BigIntBackend& b) { b.structSize = sizeof(BigIntBackend) - sizeof(b.divideTruncated); }));
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
    EXPECT_NE(ErrorCode::IntegerOutOfRange, RenderError<char>("{{ 2 ** 20000 }}", boost));
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
#ifdef JINJA2CPP_TEST_BIGINT_BOOST
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
