#ifndef FILTERS_H
#define FILTERS_H

#include "expression_evaluator.h"
#include "jinja2cpp/value.h"
#include "render_context.h"

#include <memory>
#include <functional>

namespace jinja2
{
using FilterPtr = std::shared_ptr<ExpressionFilter::IExpressionFilter>;
using FilterParams = std::unordered_map<std::string, ExpressionEvaluatorPtr<>>;

template<typename F>
struct FilterFactory
{
    static FilterPtr Create(FilterParams params)
    {
        return std::make_shared<F>(std::move(params));
    }
};

namespace filters
{
class Join : public ExpressionFilter::IExpressionFilter
{
public:
    Join(FilterParams params);

    Value Filter(const Value& baseVal, RenderContext& context);

private:
    ExpressionEvaluatorPtr<> m_delimiterEval;
    ExpressionEvaluatorPtr<> m_attribute;
};

class Sort : public ExpressionFilter::IExpressionFilter
{
public:
    Sort(FilterParams params);

    Value Filter(const Value& baseVal, RenderContext& context);

private:
    ExpressionEvaluatorPtr<> m_attrNameEvaluator;
    ExpressionEvaluatorPtr<> m_descEvaluator;
};
} // filters
} // jinja2

#endif // FILTERS_H
