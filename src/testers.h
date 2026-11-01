#ifndef JINJA2CPP_SRC_TESTERS_H
#define JINJA2CPP_SRC_TESTERS_H

#include "expression_evaluator.h"
#include "function_base.h"
#include "internal_value.h"
#include "node_arena.h"
#include "render_context.h"

#include <memory>
#include <string>
#include <string_view>

namespace jinja2
{
inline namespace v2
{
using TesterPtr = std::shared_ptr<IsExpression::ITester>;
using TesterParams = FunctionArgs;

extern TesterPtr CreateTester(std::string testerName, FunctionArgs params);
// The same in the arena of the template that names the test
// A name that is no built-in test makes a UserDefinedTester and sets `*unknown`
extern NodeRef<IsExpression::ITester> CreateTester(NodeArena& nodes, const std::string& testerName, FunctionArgs params, bool* unknown = nullptr);
// For tests named at render time (`select('name')`): a test added to the environment of the
// template comes first, as in Jinja2's env.tests
extern TesterPtr CreateTester(std::string testerName, FunctionArgs params, RenderContext& context);
// For a call that unpacks `*args` or `**kwargs` (docs/tasks/0306): the test is made from the
// values each time the call runs. registered and unknown as for IsExpression::Make
extern NodeRef<IsExpression::ITester> CreateUnpackingTester(NodeArena& nodes, const std::string& testerName, const CallArgsInfo& params, InternalValue registered, bool* unknown);
// Whether `testerName` is one of the built-in tests
bool IsBuiltinTester(std::string_view testerName);

namespace testers
{

// `value in seq`: Python's containment test, shared by the `in` operator and the `in` test
bool IsValueIn(const InternalValue& baseVal, const InternalValue& seq);
// The same over a list's items
bool IsValueInList(const InternalValue& baseVal, const InternalValueList& items);

// The interface first, so that the object starts with it
class TesterBase : public IsExpression::ITester
    , public FunctionBase
{
public:
    void VisitRefs(detail::RefChecker& refs) const override { FunctionBase::VisitRefs(refs); }
};

class Comparator : public TesterBase
{
public:
    Comparator(const TesterParams& params, BinaryExpression::Operation op);

    bool Test(const InternalValue& baseVal, RenderContext& context) override;
private:
    BinaryExpression::Operation m_op;
};

class StartsWith : public TesterBase
{
public:
    explicit StartsWith(const TesterParams&);

    bool Test(const InternalValue& baseVal, RenderContext& context) override;
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
    void VisitRefs(detail::RefChecker& refs) const override
    {
        TesterBase::VisitRefs(refs);
        m_callParams.VisitRefs(refs);
    }
private:
    std::string m_testerName;
    ForwardedArgs m_callParams;
    InternalValue m_callable;
};
// A test call that unpacks `*args` or `**kwargs`: Python binds it when it runs, so the test is
// made then, from the values
class UnpackingTester : public IsExpression::ITester
{
public:
    UnpackingTester(std::string testerName, ArenaCallParams params, InternalValue registered)
        : m_testerName(std::move(testerName))
        , m_params(params)
        , m_registered(std::move(registered))
    {
    }

    bool Test(const InternalValue& baseVal, RenderContext& context) override;
    void VisitRefs(detail::RefChecker& refs) const override { m_params.VisitRefs(refs); }

private:
    std::string m_testerName;
    ArenaCallParams m_params;
    // The test the environment adds under this name, if any
    InternalValue m_registered;
};
} // namespace testers
} // namespace v2
} // namespace jinja2

#endif // JINJA2CPP_SRC_TESTERS_H
