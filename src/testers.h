#ifndef JINJA2CPP_SRC_TESTERS_H
#define JINJA2CPP_SRC_TESTERS_H

#include "expression_evaluator.h"
#include "function_base.h"
#include "jinja2cpp/value.h"
#include "render_context.h"

#include <memory>
#include <functional>

namespace jinja2
{
using TesterPtr = std::shared_ptr<IsExpression::ITester>;
using TesterParams = CallParamsInfo;

extern TesterPtr CreateTester(std::string testerName, CallParamsInfo params);
// For tests named at render time (`select('name')`): a test added to the environment of the
// template comes first, as in Jinja2's env.tests
extern TesterPtr CreateTester(std::string testerName, CallParamsInfo params, RenderContext& context);

namespace testers
{

class TesterBase : public FunctionBase
    , public IsExpression::ITester
{
};

class Comparator : public TesterBase
{
public:
    Comparator(const TesterParams& params, BinaryExpression::Operation op);

    bool Test(const InternalValue& baseVal, RenderContext& context) override;
    bool IsEqual(const IComparable& other) const override
    {
        const auto* val = dynamic_cast<const Comparator*>(&other);
        if (!val)
            return false;
        return m_op == val->m_op;
    }
private:
    BinaryExpression::Operation m_op;
};

class StartsWith : public IsExpression::ITester
{
public:
    explicit StartsWith(const TesterParams&);

    bool Test(const InternalValue& baseVal, RenderContext& context) override;

    [[nodiscard]] bool IsEqual(const IComparable& other) const override
    {
        const auto* val = dynamic_cast<const StartsWith*>(&other);
        if (!val)
            return false;
        return m_stringEval == val->m_stringEval;
    }
private:
    ExpressionEvaluatorPtr<> m_stringEval;
};

class ValueTester : public TesterBase
{
public:
    enum Mode
    {
        IsBooleanMode,
        IsCallableMode,
        IsDefinedMode,
        IsDivisibleByMode,
        IsEscapedMode,
        IsEvenMode,
        IsFalseMode,
        IsFilterMode,
        IsFloatMode,
        IsInMode,
        IsIntegerMode,
        IsIterableMode,
        IsLowerMode,
        IsMappingMode,
        IsNoneMode,
        IsNumberMode,
        IsOddMode,
        IsSameAsMode,
        IsSequenceMode,
        IsStringMode,
        IsTestMode,
        IsTrueMode,
        IsUndefinedMode,
        IsUpperMode
    };

    ValueTester(const TesterParams& params, Mode mode);

    bool Test(const InternalValue& baseVal, RenderContext& context) override;

    bool IsEqual(const IComparable& other) const override
    {
        const auto* val = dynamic_cast<const ValueTester*>(&other);
        if (!val)
            return false;
        return m_mode == val->m_mode;
    }
private:
    Mode m_mode;
};

class UserDefinedTester : public TesterBase
{
public:
    // callable: the test added to the environment under this name; without it the test is looked up in the
    // render context, as a user callable passed in the parameters or the globals
    UserDefinedTester(std::string testerName, const TesterParams& params, InternalValue callable = InternalValue());

    bool Test(const InternalValue& baseVal, RenderContext& context) override;

    bool IsEqual(const IComparable& other) const override
    {
        const auto* val = dynamic_cast<const UserDefinedTester*>(&other);
        if (!val)
            return false;
        return m_testerName == val->m_testerName && m_callParams == val->m_callParams;
    }
private:
    std::string m_testerName;
    TesterParams m_callParams;
    InternalValue m_callable;
};
} // namespace testers
} // namespace jinja2

#endif // JINJA2CPP_SRC_TESTERS_H
