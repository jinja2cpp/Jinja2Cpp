#include "../src/contracts.h"
#include "../src/error_boundary.h"

#include "gtest/gtest.h"

#include <jinja2cpp/error_info.h>
#include <jinja2cpp/template.h>
#include <jinja2cpp/template_env.h>
#include <jinja2cpp/user_callable.h>
#include <jinja2cpp/value.h>

#include <string>

// src/contracts.h (docs/tasks/0265): a violated check throws, and the boundary reports it as an error
namespace
{
using jinja2::detail::ContractChecks;
using jinja2::detail::ContractViolation;

int g_evaluated = 0;
bool Counted(bool value)
{
    ++g_evaluated;
    return value;
}
} // namespace

TEST(ContractsTest, HoldingChecksPass)
{
    EXPECT_NO_THROW(JINJA2CPP_EXPECTS(1 + 1 == 2));
    EXPECT_NO_THROW(JINJA2CPP_ASSERT(1 + 1 == 2));
}

TEST(ContractsTest, ExpectsViolationThrows)
{
    if constexpr (ContractChecks < 1)
    {
        GTEST_SKIP() << "contract checks are OFF";
    }
    try
    {
        JINJA2CPP_EXPECTS(1 + 1 == 3);
        FAIL() << "no violation";
    }
    catch (const ContractViolation& violation)
    {
        const std::string what = violation.what();
        EXPECT_NE(what.find("1 + 1 == 3"), std::string::npos) << what;
        EXPECT_NE(what.find("contracts_test.cpp"), std::string::npos) << what;
    }
}

TEST(ContractsTest, AssertViolationThrowsOnlyAtFull)
{
    if constexpr (ContractChecks >= 2)
    {
        EXPECT_THROW(JINJA2CPP_ASSERT(false), ContractViolation);
    }
    else
    {
        EXPECT_NO_THROW(JINJA2CPP_ASSERT(false));
    }
}

TEST(ContractsTest, UncheckedConditionIsNotEvaluated)
{
    g_evaluated = 0;
    JINJA2CPP_EXPECTS(Counted(true));
    JINJA2CPP_ASSERT(Counted(true));
    EXPECT_EQ(ContractChecks >= 2 ? 2 : (ContractChecks >= 1 ? 1 : 0), g_evaluated);
}

TEST(ContractsTest, ViolationBecomesErrorAtTheBoundary)
{
    if constexpr (ContractChecks < 1)
    {
        GTEST_SKIP() << "contract checks are OFF";
    }
    try
    {
        JINJA2CPP_EXPECTS(false);
    }
    catch (...)
    {
        const auto error = jinja2::detail::CurrentExceptionToError<char>("name");
        EXPECT_EQ(jinja2::ErrorCode::UnexpectedException, error.GetCode());
        EXPECT_NE(error.ToString().find("precondition violated: false"), std::string::npos) << error.ToString();
        return;
    }
    FAIL() << "no violation";
}

TEST(ContractsTest, ViolationInARenderIsAnErrorResult)
{
    if constexpr (ContractChecks < 1)
    {
        GTEST_SKIP() << "contract checks are OFF";
    }
    jinja2::TemplateEnv env;
    env.AddGlobal("broken", jinja2::MakeCallable([]() -> jinja2::Value {
                      JINJA2CPP_EXPECTS(false && "the callable's precondition");
                      return jinja2::Value();
                  }));
    jinja2::Template tpl(&env);
    ASSERT_TRUE(tpl.Load("a{{ broken() }}b").has_value());
    const auto result = tpl.RenderAsString({});
    ASSERT_FALSE(result.has_value());
    EXPECT_NE(result.error().ToString().find("the callable's precondition"), std::string::npos) << result.error().ToString();
}
