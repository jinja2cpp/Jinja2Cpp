#ifndef TEST_TOOLS_H
#define TEST_TOOLS_H

#include <gtest/gtest.h>
#include <jinja2cpp/reflected_value.h>
#include <jinja2cpp/template.h>
#include <jinja2cpp/user_callable.h>
#include <jinja2cpp/filesystem_handler.h>
#include <jinja2cpp/template_env.h>
#include "../src/helpers.h"

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

inline std::string ErrorToString(const jinja2::ErrorInfo& error)
{
    return error.ToString();
}

inline std::wstring ErrorToString(const jinja2::ErrorInfoW& error)
{
    return error.ToString();
}

inline void StringToConsole(const std::string& str)
{
    std::cout << str << std::endl;
}

inline void StringToConsole(const std::wstring& str)
{
    std::wcout << str << std::endl;
}

class BasicTemplateRenderer : public ::testing::Test
{
public:
    template<typename TemplateT, typename CharT>
    static void ExecuteTest(const std::basic_string<CharT>& source, const std::basic_string<CharT>& expectedResult, const jinja2::ValuesMap& params, const char* version = "")
    {
        TemplateT tpl;
        auto parseRes = tpl.Load(source);
        EXPECT_TRUE(parseRes.has_value()) << version;
        if (!parseRes)
        {
            StringToConsole(ErrorToString(parseRes.error()));
            return;
        }

        auto renderRes = tpl.RenderAsString(params);
        EXPECT_TRUE(renderRes.has_value()) << version;
        if (!renderRes)
        {
            StringToConsole(ErrorToString(renderRes.error()));
            return;
        }
        auto result = renderRes.value();
        StringToConsole(result);
        EXPECT_EQ(expectedResult, result) << version;
    }

    template<typename TemplateT, typename CharT>
    void PerformTest(const std::basic_string<CharT>& source, const std::basic_string<CharT>& expectedResult, const jinja2::ValuesMap& params)
    {
        ExecuteTest<TemplateT>(source, expectedResult, params);
    }
};

class SubstitutionTestBase : public ::testing::TestWithParam<InputOutputPair>
{
protected:
    void PerformNarrowTest(const InputOutputPair& testParam)
    {
        BasicTemplateRenderer::ExecuteTest<jinja2::Template>("{{ " + testParam.tpl + " }}", testParam.result, PrepareTestData(), "Narrow version");
    }

    void PerformWideTest(const InputOutputPair& testParam)
    {
        BasicTemplateRenderer::ExecuteTest<jinja2::TemplateW>(L"{{ " + jinja2::ConvertString<std::wstring>(testParam.tpl) + L" }}", jinja2::ConvertString<std::wstring>(testParam.result), PrepareTestData(), "Wide version");
    }

    void PerformBothTests(const std::string& tpl, const std::string result, const jinja2::ValuesMap& params = PrepareTestData())
    {
        BasicTemplateRenderer::ExecuteTest<jinja2::Template>(tpl, result, params, "Narrow version");
        BasicTemplateRenderer::ExecuteTest<jinja2::TemplateW>(jinja2::ConvertString<std::wstring>(tpl), jinja2::ConvertString<std::wstring>(result), params, "Wide version");
    }
};

template<typename Tag, typename Base = SubstitutionTestBase>
class InputOutputPairTest : public Base
{
};

class TemplateEnvFixture : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_templateFs = std::make_shared<jinja2::MemoryFileSystem>();
        m_env.AddFilesystemHandler(std::string(), m_templateFs);
    }

    void AddFile(std::string fileName, std::string content)
    {
        m_templateFs->AddFile(std::move(fileName), std::move(content));
    }

    jinja2::Template Load(std::string tplBody)
    {
        jinja2::Template tpl(&m_env);
        auto loadResult = tpl.Load(std::move(tplBody));
        EXPECT_TRUE(!!loadResult);
        if (!loadResult)
        {
            std::cout << "Template loading error: " << loadResult.error() << std::endl;
            return jinja2::Template{};
        }

        return tpl;
    }

    std::string Render(std::string tplBody, const jinja2::ValuesMap& params = {})
    {
        auto tpl = Load(std::move(tplBody));

        auto renderResult = tpl.RenderAsString(params);
        EXPECT_TRUE(!!renderResult);
        if (!renderResult)
        {
            std::cout << "Template rendering error: " << renderResult.error() << std::endl;
            return "";
        }
        
        return renderResult.value();
    }

protected:
    std::shared_ptr<jinja2::MemoryFileSystem> m_templateFs;
    jinja2::TemplateEnv m_env;
};


#define MULTISTR_TEST_IMPL(Fixture, TestName, StringT, TemplateT, Tpl, Result, ParamsGetter) \
TEST_F(Fixture, TestName) \
{ \
    jinja2::ValuesMap params; \
    ParamsGetter(params, *this); \
    \
    PerformTest<TemplateT>(StringT(Tpl), StringT(Result), params); \
}

#define MULTISTR_TEST(Fixture, TestName, Tpl, Result) \
void Fixture##_##TestName##_Params_Getter(jinja2::ValuesMap& params, const Fixture& test);\
MULTISTR_TEST_IMPL(Fixture, TestName##_Narrow, std::string, jinja2::Template, Tpl, Result, Fixture##_##TestName##_Params_Getter) \
MULTISTR_TEST_IMPL(Fixture, TestName##_Wide, std::wstring, jinja2::TemplateW, L##Tpl, L##Result, Fixture##_##TestName##_Params_Getter) \
void Fixture##_##TestName##_Params_Getter(jinja2::ValuesMap& params, const Fixture& test)

struct SubstitutionGenericTestTag;
using SubstitutionGenericTest = InputOutputPairTest<SubstitutionGenericTestTag>;

#define SUBSTITUION_TEST_P(TestName) \
struct TestName##Tag; \
using TestName = InputOutputPairTest<TestName##Tag, SubstitutionTestBase>;\
TEST_P(TestName, Test##_Narrow) \
{ \
    auto& testParam = GetParam(); \
    PerformNarrowTest(testParam); \
} \
TEST_P(TestName, Test##_Wide) \
{ \
    auto& testParam = GetParam(); \
    PerformWideTest(testParam); \
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
            { "intValue",
              [](const TestStruct& obj) {
                  assert(obj.isAlive);
                  return jinja2::Reflect(obj.intValue);
              } },
            {"intEvenValue", [](const TestStruct& obj) -> Value
             {
                 assert(obj.isAlive);
                 if (obj.intValue % 2)
                    return {};
                 return {obj.intValue};
             }},
            { "dblValue",
              [](const TestStruct& obj) {
                  assert(obj.isAlive);
                  return jinja2::Reflect(obj.dblValue);
              } },
            { "boolValue",
              [](const TestStruct& obj) {
                  assert(obj.isAlive);
                  return jinja2::Reflect(obj.boolValue);
              } },
            { "strValue",
              [](const TestStruct& obj) {
                  assert(obj.isAlive);
                  return jinja2::Reflect(obj.strValue);
              } },
            { "wstrValue",
              [](const TestStruct& obj) {
                  assert(obj.isAlive);
                  return jinja2::Reflect(obj.wstrValue);
              } },
            { "strViewValue",
              [](const TestStruct& obj) {
                  assert(obj.isAlive);
                  return jinja2::Reflect(nonstd::string_view(obj.strValue));
              } },
            { "wstrViewValue",
              [](const TestStruct& obj) {
                  assert(obj.isAlive);
                  return jinja2::Reflect(nonstd::wstring_view(obj.wstrValue));
              } },
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
			{"basicCallable",[](const TestStruct& obj) {
					return jinja2::MakeCallable([&obj]() { assert(obj.isAlive);  return obj.intValue; });
				}},
			{"getInnerStruct",[](const TestStruct& obj) {
					return jinja2::MakeCallable([&obj]() { assert(obj.isAlive);  return jinja2::Reflect(obj.innerStruct); });
				}},
			{"getInnerStructValue",[](const TestStruct& obj) {
					return jinja2::MakeCallable([&obj]() { assert(obj.isAlive);  return jinja2::Reflect(*obj.innerStruct); });
				}},
		};

        return accessors;
    }
};
} // jinja2

#endif // TEST_TOOLS_H
