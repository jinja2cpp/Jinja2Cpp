#include "gtest/gtest.h"

#include <jinja2cpp/template.h>
#include <jinja2cpp/user_callable.h>
#include <jinja2cpp/value.h>

#include <string>

using namespace jinja2;

// Python evaluates a dict literal key before its value (task 0013 review)
TEST(LiteralsTest, DictKeyIsEvaluatedBeforeValue)
{
    std::string log;
    int counter = 0;

    ValuesMap params;
    params["key"] = MakeCallable([&log, &counter]() {
        log += "K";
        return std::to_string(counter);
    });
    params["val"] = MakeCallable([&log, &counter]() {
        log += "V";
        return ++counter;
    });

    Template tpl;
    ASSERT_TRUE(tpl.Load("{% set d = {key(): val()} %}{{ d['0'] }}").has_value());
    auto result = tpl.RenderAsString(params);
    ASSERT_TRUE(result.has_value()) << result.error().ToString();

    EXPECT_EQ("1", result.value());
    EXPECT_EQ("KV", log);
}
