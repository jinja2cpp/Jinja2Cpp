#include "gtest/gtest.h"

#include "../src/helpers.h"

using namespace jinja2;

TEST(Helpers, CompileEscapes)
{
    EXPECT_STREQ("\n", CompileEscapes(std::string{"\\n"}).c_str());
    EXPECT_STREQ("\t", CompileEscapes(std::string{"\\t"}).c_str());
    EXPECT_STREQ("\r", CompileEscapes(std::string{"\\r"}).c_str());
    EXPECT_STREQ("\r\n\t", CompileEscapes(std::string{R"(\r\n\t)"}).c_str());
    EXPECT_STREQ(
        "aa\rbb\ncc\tdd",
        CompileEscapes(std::string{R"(aa\rbb\ncc\tdd)"}).c_str());
    EXPECT_STREQ("", CompileEscapes(std::string{""}).c_str());
    EXPECT_STREQ(
        "aa bb cc dd",
        CompileEscapes(std::string{"aa bb cc dd"}).c_str());
}


