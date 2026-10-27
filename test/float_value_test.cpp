#include "../src/decimal_number.h"
#include "../src/value_visitors.h"
#include "test_tools.h"

#include <jinja2cpp/reflected_value.h>
#include <jinja2cpp/template.h>
#include <jinja2cpp/user_callable.h>
#include <jinja2cpp/value.h>

#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

// A C++ float in Value (docs/tasks/0039): the host keeps the float, a template sees the
// Python float its shortest digits name, so 12.123f is 12.123 and not 12.123000144958496
using namespace jinja2;

namespace
{
struct FloatFields
{
    float ratio = 12.123F;
    double exact = 0.1;
};

float FloatFromBits(uint32_t bits)
{
    float val = 0;
    std::memcpy(&val, &bits, sizeof(val));
    return val;
}

uint32_t BitsOfFloat(float val)
{
    uint32_t bits = 0;
    std::memcpy(&bits, &val, sizeof(bits));
    return bits;
}

std::string Render(const std::string& source, const ValuesMap& params)
{
    Template tpl;
    EXPECT_TRUE(tpl.Load(source).has_value()) << source;
    auto result = tpl.RenderAsString(params);
    return result ? result.value() : "<error>";
}

std::wstring RenderWide(const std::wstring& source, const ValuesMap& params)
{
    TemplateW tpl;
    EXPECT_TRUE(tpl.Load(source).has_value());
    auto result = tpl.RenderAsString(params);
    return result ? result.value() : L"<error>";
}
} // namespace

namespace jinja2
{
template<>
struct TypeReflection<FloatFields> : TypeReflected<FloatFields>
{
    static auto& GetAccessors()
    {
        static std::unordered_map<std::string, FieldAccessor> accessors = {
            { "ratio", [](const FloatFields& obj) { return Reflect(obj.ratio); } },
            { "exact", [](const FloatFields& obj) { return Reflect(obj.exact); } },
        };
        return accessors;
    }
};
} // namespace jinja2

TEST(FloatValueTest, HostKeepsTheFloat)
{
    const Value val(12.123F);
    EXPECT_TRUE(val.IsFloat());
    EXPECT_FALSE(val.IsDouble());
    EXPECT_EQ(16U, val.GetData().index());
    EXPECT_EQ(12.123F, val.AsFloat());
    EXPECT_EQ(12.123F, val.As<float>());
    ASSERT_NE(nullptr, val.GetIf<float>());
    EXPECT_EQ(nullptr, val.GetIf<double>());
    float out = 0;
    EXPECT_TRUE(val.AsFloat(out));
    EXPECT_EQ(12.123F, out);
    double notDouble = 0;
    EXPECT_FALSE(val.AsDouble(notDouble));
    EXPECT_THROW((void)val.AsDouble(), std::bad_variant_access);
    EXPECT_THROW((void)Value(12.123).AsFloat(), std::bad_variant_access);
    EXPECT_FALSE(Value(12.123).IsFloat());

    // double and long double stay double; host equality is structural, as Value(1) != Value(1.0)
    EXPECT_TRUE(Value(1.5).IsDouble());
    EXPECT_TRUE(Value(1.5L).IsDouble());
    EXPECT_FALSE(Value(1.5F).IsEqual(Value(1.5)));
    EXPECT_TRUE(Value(1.5F).IsEqual(Value(1.5F)));

    bool visited = false;
    Visit([&visited](const auto& alt) { visited = std::is_same_v<std::decay_t<decltype(alt)>, float>; }, val);
    EXPECT_TRUE(visited);

    EXPECT_EQ("12.123", val.ToString());
    EXPECT_EQ(L"12.123", val.ToWString());
    EXPECT_EQ("12.123", ToString(12.123F));
    char buffer[16];
    const auto printed = val.ToChars(buffer, buffer + sizeof(buffer));
    EXPECT_EQ("12.123", std::string(buffer, printed.ptr));
}

TEST(FloatValueTest, ReflectedFloatsStayFloats)
{
    EXPECT_TRUE(Reflect(12.123F).IsFloat());
    EXPECT_TRUE(Reflect(0.1).IsDouble());
    const FloatFields fields;
    const ValuesMap params{ { "s", Reflect(fields) }, { "v", Reflect(std::vector<float>{ 0.1F, 12.123F, 1e7F }) } };
    EXPECT_EQ("12.123 0.1 [0.1, 12.123, 10000000.0] 10000012.223", Render("{{ s.ratio }} {{ s.exact }} {{ v }} {{ v|sum }}", params));
    EXPECT_EQ(L"12.123 [0.1, 12.123, 10000000.0]", RenderWide(L"{{ s.ratio }} {{ v }}", params));
}

// Expected text from Jinja2 3.1.6, with x = float(str(numpy.float32(12.123)))
TEST(FloatValueTest, TemplateSeesThePythonFloat)
{
    const ValuesMap params{ { "x", 12.123F } };
    const std::string source =
        "{{ x }} {{ x * 2 }} {{ x == 12.123 }} {{ x - 12.123 }} {{ x|string }} {{ '%s'|format(x) }} {{ [x] }} {{ x|tojson }} {{ [x, 1.5]|sum }} "
        "{{ '%.10f'|format(x) }} {{ x is float }} {{ x|round(1) }} {{ x|int }}";
    const std::string expected = "12.123 24.246 True 0.0 12.123 12.123 [12.123] 12.123 13.623 12.1230000000 True 12.1 12";
    EXPECT_EQ(expected, Render(source, params));
    EXPECT_EQ(std::wstring(expected.begin(), expected.end()), RenderWide(std::wstring(source.begin(), source.end()), params));
}

// Python prints the double named by numpy.float32's digits as these
TEST(FloatValueTest, EdgeFloatsPrintAsPython)
{
    const std::vector<std::pair<float, std::string>> cases = {
        { 1e7F, "10000000.0" },
        { 1e16F, "1e+16" },
        { 1e-5F, "1e-05" },
        { 1e-4F, "0.0001" },
        { -0.0F, "-0.0" },
        { 0.0F, "0.0" },
        { 0.1F, "0.1" },
        { std::numeric_limits<float>::infinity(), "inf" },
        { -std::numeric_limits<float>::infinity(), "-inf" },
        { std::numeric_limits<float>::quiet_NaN(), "nan" },
        { std::numeric_limits<float>::max(), "3.4028235e+38" },
        { std::numeric_limits<float>::lowest(), "-3.4028235e+38" },
        { std::numeric_limits<float>::min(), "1.1754944e-38" },
        { std::numeric_limits<float>::denorm_min(), "1e-45" },
        { 16777217.0F, "16777216.0" },
        // The two floats whose shortest decimal names a double that rounds to the neighbouring
        // float (0x15ae43fd prints 7.038531e-26 as a float): one more digit round-trips
        { FloatFromBits(0x15ae43fdU), "7.0385307e-26" },
        { FloatFromBits(0x95ae43fdU), "-7.0385307e-26" },
    };
    for (const auto& [val, text] : cases)
    {
        EXPECT_EQ(text, Value(val).ToString()) << BitsOfFloat(val);
        EXPECT_EQ(text, Render("{{ x }}", { { "x", val } })) << BitsOfFloat(val);
    }
}

// A typed float parameter gets the exact float back; an untyped one sees the double
TEST(FloatValueTest, CallablesGetTheFloatBack)
{
    const float original = FloatFromBits(0x15ae43fdU);
    ValuesMap params{
        { "x", 12.123F },
        { "y", original },
        { "isSame", MakeCallable([](float val) { return val == 12.123F; }, ArgInfo{ "val" }) },
        { "bits", MakeCallable([](float val) { return static_cast<int64_t>(BitsOfFloat(val)); }, ArgInfo{ "val" }) },
        { "kind",
          MakeCallable(
              [](const Value& val) {
                  double out = 0;
                  return std::string(val.AsDouble(out) && out == 12.123 ? "double 12.123" : "other");
              },
              ArgInfo{ "val" }) },
    };
    EXPECT_EQ("True " + std::to_string(BitsOfFloat(original)) + " double 12.123", Render("{{ isSame(x) }} {{ bits(y) }} {{ kind(x) }}", params));
}

// ErrorInfo prints its extra parameters through its own visitor
TEST(FloatValueTest, ErrorInfoPrintsTheFloatAsTheTemplateDoes)
{
    ErrorInfo::Data data;
    data.code = ErrorCode::UndefinedError;
    data.extraParams.emplace_back(12.123F);
    EXPECT_NE(std::string::npos, ErrorInfo(data).ToString().find("Undefined value: 12.123")) << ErrorInfo(data).ToString();
}

// Every 4099th float, plus the bit patterns at the edges: the converted double gives the
// float back, and prints at most 9 significant digits (the full 2^32 sweep is recorded in
// the PR that added this test)
TEST(FloatValueTest, SampledFloatsRoundTrip)
{
    auto check = [](uint32_t bits) {
        const float val = FloatFromBits(bits);
        if (!std::isfinite(val))
        {
            return;
        }
        const double converted = FloatAsDecimalDouble(val);
        ASSERT_EQ(bits, BitsOfFloat(static_cast<float>(converted))) << bits;
        // Significant digits of the printed text: no sign, point, exponent, leading or trailing zeros
        const visitors::PythonFloatText text(converted);
        const std::string_view printed = text.View();
        std::string digits;
        for (const char ch : printed.substr(0, printed.find('e')))
        {
            if (ch >= '0' && ch <= '9' && (ch != '0' || !digits.empty()))
            {
                digits += ch;
            }
        }
        digits.erase(digits.find_last_not_of('0') + 1);
        ASSERT_LE(digits.size(), 9U) << printed;
    };
    for (uint64_t bits = 0; bits <= 0xffffffffU; bits += 4099)
    {
        check(static_cast<uint32_t>(bits));
    }
    for (const uint32_t bits : { 0x00000001U, 0x007fffffU, 0x00800000U, 0x7f7fffffU, 0x80000000U, 0x80000001U, 0x15ae43fdU, 0x95ae43fdU, 0x3727c5acU, 0x5a0e1bcaU })
    {
        check(bits);
    }
}
