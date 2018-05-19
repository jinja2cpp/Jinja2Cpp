#include "statements.h"
#include "value_visitors.h"

namespace jinja2
{

void ForStatement::Render(OutStream& os, RenderContext& values)
{
    InternalValue loopVal = m_value->Evaluate(values);

    auto& context = values.EnterScope();

    InternalValueMap loopVar;
    context["loop"] = MapAdapter::CreateAdapter(&loopVar);

    bool isConverted = false;
    auto loopItems = ConvertToList(loopVal, InternalValue(), isConverted);
    if (!isConverted)
        return;

    int64_t itemsNum = static_cast<int64_t>(loopItems.GetSize());
    loopVar["length"] = InternalValue(itemsNum);
    bool loopRendered = false;
    for (int64_t itemIdx = 0; itemIdx != itemsNum; ++ itemIdx)
    {
        loopRendered = true;
        loopVar["index"] = InternalValue(itemIdx + 1);
        loopVar["index0"] = InternalValue(itemIdx);
        loopVar["first"] = InternalValue(itemIdx == 0);
        loopVar["last"] = InternalValue(itemIdx == itemsNum - 1);
        if (itemIdx != 0)
            loopVar["previtem"] = loopItems.GetValueByIndex(static_cast<size_t>(itemIdx - 1));
        if (itemIdx != itemsNum - 1)
            loopVar["nextitem"] = loopItems.GetValueByIndex(static_cast<size_t>(itemIdx + 1));
        else
            loopVar.erase("nextitem");

        const auto& curValue = loopItems.GetValueByIndex(static_cast<size_t>(itemIdx));
        if (m_vars.size() > 1)
        {
            for (auto& varName : m_vars)
                context[varName] = Subscript(curValue, varName);
        }
        else
            context[m_vars[0]] = curValue;

        m_mainBody->Render(os, values);
    }

    if (!loopRendered && m_elseBody)
        m_elseBody->Render(os, values);

    values.ExitScope();
}

void IfStatement::Render(OutStream& os, RenderContext& values)
{
    InternalValue val = m_expr->Evaluate(values);
    bool isTrue = Apply<visitors::BooleanEvaluator>(val);

    if (isTrue)
    {
        m_mainBody->Render(os, values);
        return;
    }

    for (auto& b : m_elseBranches)
    {
        if (b->ShouldRender(values))
        {
            b->Render(os, values);
            break;
        }
    }
}

bool ElseBranchStatement::ShouldRender(RenderContext& values) const
{
    if (!m_expr)
        return true;

    return Apply<visitors::BooleanEvaluator>(m_expr->Evaluate(values));
}

void ElseBranchStatement::Render(OutStream& os, RenderContext& values)
{
    m_mainBody->Render(os, values);
}

void SetStatement::Render(OutStream&, RenderContext& values)
{
   if (m_expr)
   {
       InternalValue val = m_expr->Evaluate(values);
       if (m_fields.size() == 1)
           values.GetCurrentScope()[m_fields[0]] = val;
       else
       {
           for (auto& name : m_fields)
               values.GetCurrentScope()[name] = Subscript(val, name);
       }
   }
}

} // jinja2
