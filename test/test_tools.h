#ifndef TEST_TOOLS_H
#define TEST_TOOLS_H

#include <gtest/gtest.h>
#include <jinja2cpp/reflected_value.h>
#include <jinja2cpp/template.h>

struct InputOutputPair
{
    std::string tpl;
    std::string result;

    friend std::ostream& operator << (std::ostream& os, const InputOutputPair& rp)
    {
        os << rp.tpl << " -> " << rp.result;
        return os;
    }
};

template<typename Tag, typename Base = ::testing::TestWithParam<InputOutputPair>>
class InputOutputPairTest : public Base
{
};

struct TestInnerStruct
{
    ~TestInnerStruct() {isAlive = false;}

    bool isAlive = true;
    std::string strValue = "Hello World!";
};

struct TestStruct
{
    ~TestStruct() {isAlive = false;}

    bool isAlive = true;
    int64_t intValue;
    double dblValue;
    bool boolValue;
    std::string strValue;
    std::wstring wstrValue;
    std::shared_ptr<TestInnerStruct> innerStruct;
    std::vector<std::shared_ptr<TestInnerStruct>> innerStructList;
};

inline jinja2::ValuesMap PrepareTestData()
{
    jinja2::ValuesList testData;
    TestStruct sampleStruct;
    for (int n = 0; n < 10; ++ n)
    {
        TestStruct s;
        std::ostringstream str;
        std::wostringstream wstr;

        str << "test string " << n;
        wstr << L"test string " << n;

        s.intValue = n;
        s.dblValue = static_cast<double>(n) / 2;
        s.boolValue = n % 2 == 1;
        s.strValue = str.str();
        s.wstrValue = wstr.str();

        if (testData.empty())
            sampleStruct = s;
        testData.push_back(jinja2::Reflect(std::move(s)));
    }

    std::shared_ptr<TestStruct> emptyTestStruct;
    std::shared_ptr<TestStruct> filledTestStruct = std::make_shared<TestStruct>(sampleStruct);
    sampleStruct.innerStruct = std::make_shared<TestInnerStruct>();
    for (int n = 0; n < 10; ++ n)
        sampleStruct.innerStructList.push_back(std::make_shared<TestInnerStruct>());

    return jinja2::ValuesMap {
        {"intValue", 3},
        {"intList", jinja2::ValuesList{9, 0, 8, 1, 7, 2, 6, 3, 5, 4}},
        {"doubleValue", 12.123f},
        {"doubleList", jinja2::ValuesList{9.5, 0.5, 8.5, 1.5, 7.5, 2.5, 6.4, 3.8, 5.2, -4.7}},
        {"intAsDoubleList", jinja2::ValuesList{9.0, 0.0, 8.0, 1.0, 7.0, 2.0, 6.0, 3.0, 5.0, 4.0}},
        {"stringValue", "rain"},
        {"wstringValue", std::wstring(L"  hello world ")},
        {"stringList", jinja2::ValuesList{"string9", "string0", "string8", "string1", "string7", "string2", "string6", "string3", "string5", "string4"}},
        {"boolFalseValue", false},
        {"boolTrueValue", true},
        {"mapValue", jinja2::ValuesMap{
                {"intVal", 10},
                {"dblVal", 100.5},
                {"stringVal", "string100.5"},
                {"boolValue", true},
                {"reflectedList", testData}
            }},
        {"simpleMapValue", jinja2::ValuesMap{
                {"intVal", 10},
                {"dblVal", 100.5},
                {"stringVal", "string100.5"},
                {"boolValue", true}
            }},
        {"reflectedVal", jinja2::Reflect(sampleStruct)},
        {"emptyReflectedPtrVal", jinja2::Reflect(emptyTestStruct)},
        {"filledReflectedPtrVal", jinja2::Reflect(filledTestStruct)},
        {"reflectedIntVector", jinja2::Reflect(std::vector<int64_t>{9, 0, 8, 1, 7, 2, 6, 3, 5, 4})},
        {"reflectedList", std::move(testData)}
    };
}

class SubstitutionTestBase : public ::testing::TestWithParam<InputOutputPair>
{
protected:
    void PerformTest(const InputOutputPair& testParam)
    {
        std::string source = "{{ " + testParam.tpl + " }}";

        jinja2::Template tpl;
        auto parseRes = tpl.Load(source);
        EXPECT_TRUE(parseRes.has_value());
        if (!parseRes)
        {
            std::cout << parseRes.error() << std::endl;
            return;
        }

        std::string result = tpl.RenderAsString(PrepareTestData());
        std::cout << result << std::endl;
        std::string expectedResult = testParam.result;
        EXPECT_EQ(expectedResult, result);
    }
};

struct SubstitutionGenericTestTag;
using SubstitutionGenericTest = InputOutputPairTest<SubstitutionGenericTestTag>;

#define SUBSTITUION_TEST_P(TestName) \
struct TestName##Tag; \
using TestName = InputOutputPairTest<TestName##Tag, SubstitutionTestBase>;\
TEST_P(TestName, Test) \
{ \
    auto& testParam = GetParam(); \
    PerformTest(testParam); \
}

namespace jinja2
{
template<>
struct TypeReflection<TestInnerStruct> : TypeReflected<TestInnerStruct>
{
    static auto& GetAccessors()
    {
        static std::unordered_map<std::string, FieldAccessor> accessors = {
            {"strValue", [](const TestInnerStruct& obj) {assert(obj.isAlive); return obj.strValue;}},
        };

        return accessors;
    }
};

template<>
struct TypeReflection<TestStruct> : TypeReflected<TestStruct>
{
    static auto& GetAccessors()
    {
        static std::unordered_map<std::string, FieldAccessor> accessors = {
            {"intValue", [](const TestStruct& obj) {assert(obj.isAlive); return obj.intValue;}},
            {"dblValue", [](const TestStruct& obj) {assert(obj.isAlive); return obj.dblValue;}},
            {"boolValue", [](const TestStruct& obj) {assert(obj.isAlive); return obj.boolValue;}},
            {"strValue", [](const TestStruct& obj) {assert(obj.isAlive); return obj.strValue;}},
            {"wstrValue", [](const TestStruct& obj) {assert(obj.isAlive); return obj.wstrValue;}},
            {"innerStruct", [](const TestStruct& obj)
             {
                 assert(obj.isAlive);
                 return obj.innerStruct ? jinja2::Reflect(obj.innerStruct) : Value();
             }},
            {"innerStructList", [](const TestStruct& obj) {assert(obj.isAlive); return jinja2::Reflect(obj.innerStructList);}},
            {"tmpStructList", [](const TestStruct& obj)
                {
                    assert(obj.isAlive);
                    using list_t = std::vector<std::shared_ptr<TestInnerStruct>>;
                    list_t vals;
                    for (int n = 0; n < 10; ++ n)
                        vals.push_back(std::make_shared<TestInnerStruct>());
                    return jinja2::Reflect(list_t(vals.begin(), vals.end()));
                }},
        };

        return accessors;
    }
};
} // jinja2

#endif // TEST_TOOLS_H
