#include <iostream>
#include <string>

#include "test_tools.h"
#include "jinja2cpp/template.h"

using namespace jinja2;

struct JoinTestTag;
using JoinTest = InputOutputPairTest<JoinTestTag>;

TEST_P(JoinTest, IntegersRangeLoop)
{
    auto& testParam = GetParam();
    std::string source = "{{" + testParam.tpl + "}}";

    Template tpl;
    ASSERT_TRUE(tpl.Load(source));

    ValuesList testData;
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

        testData.push_back(Reflect(std::move(s)));
    }

    ValuesMap params = {
        {"intValue", 3},
        {"doubleValue", 12.123f},
        {"stringValue", "rain"},
        {"boolFalseValue", false},
        {"boolTrueValue", true},
        {"reflectedList", std::move(testData)}
    };

    std::string result = tpl.RenderAsString(params);
    std::cout << result << std::endl;
    std::string expectedResult = testParam.result;
    EXPECT_EQ(expectedResult, result);
}

INSTANTIATE_TEST_CASE_P(StringJoin, JoinTest, ::testing::Values(
                            InputOutputPair{"['str1', 'str2', 'str3'] | join", "str1str2str3"},
                            InputOutputPair{"['str1', 'str2', 'str3'] | join(' ')", "str1 str2 str3"},
                            InputOutputPair{"['str1', 'str2', 'str3'] | join(d='-')", "str1-str2-str3"}
                            ));
