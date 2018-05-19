#ifndef TESTERS_H
#define TESTERS_H

#include "expression_evaluator.h"
#include "jinja2cpp/value.h"
#include "render_context.h"

#include <memory>
#include <functional>

namespace jinja2
{
using TesterPtr = std::shared_ptr<IsExpression::ITester>;
using TesterParams = CallParams;

template<typename F>
struct TesterFactory
{
    static TesterPtr Create(TesterParams params)
    {
        return std::make_shared<F>(std::move(params));
    }
};

namespace testers
{
class Defined : public IsExpression::ITester
{
public:
    Defined(TesterParams) {}

    bool Test(const InternalValue& baseVal, RenderContext& context) override;
};

class StartsWith : public IsExpression::ITester
{
public:
    StartsWith(TesterParams);

    bool Test(const InternalValue& baseVal, RenderContext& context) override;

private:
    ExpressionEvaluatorPtr<> m_stringEval;
};

} // testers
} // jinja2

#endif // TESTERS_H
