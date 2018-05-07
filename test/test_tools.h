#ifndef TEST_TOOLS_H
#define TEST_TOOLS_H

#include <gtest/gtest.h>
#include <jinja2cpp/reflected_value.h>

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

template<typename Tag>
class InputOutputPairTest : public ::testing::TestWithParam<InputOutputPair>
{
};

struct TestStruct
{
    int64_t intValue;
    double dblValue;
    bool boolValue;
    std::string strValue;
    std::wstring wstrValue;
};

namespace jinja2
{
template<>
struct TypeReflection<TestStruct> : TypeReflected<TestStruct>
{
    static auto& GetAccessors()
    {
        static std::unordered_map<std::string, FieldAccessor> accessors = {
            {"intValue", [](const TestStruct& obj) {return obj.intValue;}},
            {"dblValue", [](const TestStruct& obj) {return obj.dblValue;}},
            {"boolValue", [](const TestStruct& obj) { return obj.boolValue;}},
            {"strValue", [](const TestStruct& obj) {return obj.strValue;}},
            {"wstrValue", [](const TestStruct& obj) {return obj.wstrValue;}},
        };

        return accessors;
    }
};
} // jinja2

#endif // TEST_TOOLS_H
