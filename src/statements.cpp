#include "statements.h"

#include "expression_evaluator.h"
#include "template_impl.h"
#include "value_visitors.h"

#include <boost/core/null_deleter.hpp>

#include <string>

using namespace std::string_literals;

namespace jinja2
{

void ForStatement::Render(OutStream& os, RenderContext& values)
{
    InternalValue loopVal = m_value->Evaluate(values);

    RenderLoop(loopVal, os, values, 0);
}

void ForStatement::RenderLoop(const InternalValue& loopVal, OutStream& os, RenderContext& values, int level)
{
    auto& context = values.EnterScope();

    InternalValueMap loopVar;
    context["loop"s] = CreateMapAdapter(&loopVar);
    if (m_isRecursive)
    {
        loopVar["operator()"s] = Callable(Callable::GlobalFunc, [this, level](const CallParams& params, OutStream& stream, RenderContext& context) {
            bool isSucceeded = false;
            auto parsedParams = helpers::ParseCallParams({ { "var", true } }, params, isSucceeded);
            if (!isSucceeded)
                return;

            auto var = parsedParams["var"];
            if (var.IsEmpty())
                return;

            RenderLoop(var, stream, context, level + 1);
        });
        loopVar["depth"s] = static_cast<int64_t>(level + 1);
        loopVar["depth0"s] = static_cast<int64_t>(level);
    }

    bool isConverted = false;
    auto loopItems = ConvertToList(loopVal, isConverted, false);
    ListAdapter filteredList;
    ListAdapter indexedList;
    nonstd::optional<ListAccessorEnumeratorPtr> enumerator;
    size_t itemIdx = 0;
    if (!isConverted)
    {
        if (m_elseBody)
            m_elseBody->Render(os, values);
        values.ExitScope();
        return;
    }

    nonstd::optional<size_t> listSize;
    if (m_ifExpr)
    {
        filteredList = CreateFilteredAdapter(loopItems, values);
        enumerator = filteredList.GetEnumerator();
    }
    else
    {
        enumerator = loopItems.GetEnumerator();
        listSize = loopItems.GetSize();
    }

    bool isLast = false;
    auto makeIndexedList = [&enumerator, &listSize, &indexedList, &itemIdx, &isLast] {
        if (isLast)
            listSize = itemIdx;

        InternalValueList items;
        do
        {
            items.push_back((*enumerator)->GetCurrent());
        } while ((*enumerator)->MoveNext());

        listSize = itemIdx + items.size() + 1;
        indexedList = ListAdapter::CreateAdapter(std::move(items));
        enumerator = indexedList.GetEnumerator();
        isLast = !(*enumerator)->MoveNext();
    };

    if (listSize)
    {
        int64_t itemsNum = static_cast<int64_t>(listSize.value());
        loopVar["length"s] = InternalValue(itemsNum);
    }
    else
    {
        loopVar["length"s] = MakeDynamicProperty([&listSize, &makeIndexedList](const CallParams& /*params*/, RenderContext& /*context*/) -> InternalValue {
            if (!listSize)
                makeIndexedList();
            return static_cast<int64_t>(listSize.value());
        });
    }
    bool loopRendered = false;
    isLast = !(*enumerator)->MoveNext();
    InternalValue prevValue;
    InternalValue curValue;
    InternalValue nextValue;
    loopVar["cycle"s] = static_cast<int64_t>(LoopCycleFn);
    for (; !isLast; ++itemIdx)
    {
        prevValue = std::move(curValue);
        if (itemIdx != 0)
        {
            std::swap(curValue, nextValue);
            loopVar["previtem"s] = prevValue;
        }
        else
            curValue = (*enumerator)->GetCurrent();

        isLast = !(*enumerator)->MoveNext();
        if (!isLast)
        {
            nextValue = (*enumerator)->GetCurrent();
            loopVar["nextitem"s] = nextValue;
        }
        else
        {
            loopVar.erase("nextitem"s);
        }

        loopRendered = true;
        loopVar["index"s] = static_cast<int64_t>(itemIdx + 1);
        loopVar["index0"s] = static_cast<int64_t>(itemIdx);
        loopVar["first"s] = itemIdx == 0;
        loopVar["last"s] = isLast;

        if (m_vars.size() > 1)
        {
            const auto& valList = ConvertToList(curValue, isConverted);
            if (!isConverted)
                continue;

            auto b = valList.begin();
            auto e = valList.end();

            for (auto& varName : m_vars)
            {
                if (b == e)
                    continue;
                context[varName] = *b;
                ++b;
            }
        }
        else
        {
            context[m_vars[0]] = curValue;
        }

        values.EnterScope();
        m_mainBody->Render(os, values);
        values.ExitScope();
    }

    if (!loopRendered && m_elseBody)
        m_elseBody->Render(os, values);

    values.ExitScope();
}

ListAdapter ForStatement::CreateFilteredAdapter(const ListAdapter& loopItems, RenderContext& values) const
{
    return ListAdapter::CreateAdapter([eo = loopItems.GetEnumerator(), this, &values]() mutable {
        using ResultType = nonstd::optional<InternalValue>;

        auto& tempContext = values.EnterScope();
        if (!eo.has_value())
            return ResultType();
        auto& e = *eo;
        for (bool finish = !e->MoveNext(); !finish; finish = !e->MoveNext())
        {
            auto curValue = e->GetCurrent();
            if (m_vars.size() > 1)
            {
                for (auto& varName : m_vars)
                    tempContext[varName] = Subscript(curValue, varName, &values);
            }
            else
            {
                tempContext[m_vars[0]] = curValue;
            }

            if (ConvertToBool(m_ifExpr->Evaluate(values)))
            {
                values.ExitScope();
                return ResultType(std::move(curValue));
            }
        }
        values.ExitScope();

        return ResultType();
    });
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

void SetStatement::AssignBody(InternalValue body, RenderContext& values)
{
    auto& scope = values.GetCurrentScope();
    if (m_fields.size() == 1)
        scope[m_fields.front()] = std::move(body);
    else
    {
        for (const auto& name : m_fields)
            scope[name] = Subscript(body, name, &values);
    }
}

void SetLineStatement::Render(OutStream&, RenderContext& values)
{
    if (!m_expr)
        return;
    AssignBody(m_expr->Evaluate(values), values);
}

InternalValue SetBlockStatement::RenderBody(RenderContext& values)
{
    TargetString result;
    auto stream = values.GetRendererCallback()->GetStreamOnString(result);
    auto innerValues = values.Clone(true);
    m_body->Render(stream, innerValues);
    return result;
}

void SetRawBlockStatement::Render(OutStream&, RenderContext& values)
{
    AssignBody(RenderBody(values), values);
}

void SetFilteredBlockStatement::Render(OutStream&, RenderContext& values)
{
    if (!m_expr)
        return;
    AssignBody(m_expr->Evaluate(RenderBody(values), values), values);
}

class IBlocksRenderer : public IRendererBase
{
public:
    virtual bool HasBlock(const std::string& blockName) = 0;
    virtual void RenderBlock(const std::string& blockName, OutStream& os, RenderContext& values) = 0;
};

void ParentBlockStatement::Render(OutStream& os, RenderContext& values)
{
    RenderContext innerContext = values.Clone(m_isScoped);
    bool found = false;
    auto parentTplVal = values.FindValue("$$__parent_template", found);
    if (!found)
    {
        m_mainBody->Render(os, values);
        return;
    }

    bool isConverted = false;
    auto parentTplsList = ConvertToList(parentTplVal->second, isConverted);
    if (!isConverted)
        return;

    IBlocksRenderer* blockRenderer = nullptr; // static_cast<BlocksRenderer*>(*parentTplPtr);
    for (auto& tplVal : parentTplsList)
    {
        auto ptr = GetIf<RendererPtr>(&tplVal);
        if (!ptr)
            continue;

        auto parentTplPtr = static_cast<IBlocksRenderer*>(ptr->get());

        if (parentTplPtr->HasBlock(m_name))
        {
            blockRenderer = parentTplPtr;
            break;
        }
    }

    if (!blockRenderer)
    {
        m_mainBody->Render(os, values);
        return;
    }

    auto& scope = innerContext.EnterScope();
    scope["$$__super_block"] = RendererPtr(this, boost::null_deleter());
    scope["super"] =
        Callable(Callable::SpecialFunc, [this](const CallParams&, OutStream& stream, RenderContext& context) { m_mainBody->Render(stream, context); });
    if (!m_isScoped)
        scope["$$__parent_template"] = parentTplsList;

    blockRenderer->RenderBlock(m_name, os, innerContext);
    innerContext.ExitScope();

    auto& globalScope = values.GetGlobalScope();
    auto selfMap = GetIf<MapAdapter>(&globalScope[std::string("self")]);
    if (!selfMap->HasValue(m_name))
        selfMap->SetValue(m_name, MakeWrapped(Callable(Callable::SpecialFunc, [this](const CallParams&, OutStream& stream, RenderContext& context) {
                              Render(stream, context);
                          })));
}

void BlockStatement::Render(OutStream& os, RenderContext& values)
{
    m_mainBody->Render(os, values);
}

template<typename CharT>
class ParentTemplateRenderer : public IBlocksRenderer
{
public:
    ParentTemplateRenderer(std::shared_ptr<TemplateImpl<CharT>> tpl, ExtendsStatement::BlocksCollection* blocks)
        : m_template(tpl)
        , m_blocks(blocks)
    {
    }

    void Render(OutStream& os, RenderContext& values) override
    {
        auto& scope = values.GetCurrentScope();
        InternalValueList parentTemplates;
        parentTemplates.push_back(InternalValue(RendererPtr(this, boost::null_deleter())));
        bool isFound = false;
        auto p = values.FindValue("$$__parent_template", isFound);
        if (isFound)
        {
            bool isConverted = false;
            auto prevTplsList = ConvertToList(p->second, isConverted);
            if (isConverted)
            {
                for (auto& tpl : prevTplsList)
                    parentTemplates.push_back(tpl);
            }
        }
        scope["$$__parent_template"] = ListAdapter::CreateAdapter(std::move(parentTemplates));
        m_template->GetRenderer()->Render(os, values);
    }

    void RenderBlock(const std::string& blockName, OutStream& os, RenderContext& values) override
    {
        auto p = m_blocks->find(blockName);
        if (p == m_blocks->end())
            return;

        p->second->Render(os, values);
    }

    bool HasBlock(const std::string& blockName) override { return m_blocks->count(blockName) != 0; }

    bool IsEqual(const IComparable& other) const override
    {
        auto* val = dynamic_cast<const ParentTemplateRenderer*>(&other);
        if (!val)
            return false;
        if (m_template != val->m_template)
            return false;
        if (m_blocks && val->m_blocks && *m_blocks != *(val->m_blocks))
            return false;
        if ((m_blocks && !val->m_blocks) || (!m_blocks && val->m_blocks))
            return false;
        return true;
    }

private:
    std::shared_ptr<TemplateImpl<CharT>> m_template;
    ExtendsStatement::BlocksCollection* m_blocks;
};

template<typename Result, typename Fn>
struct TemplateImplVisitor
{
    // ExtendsStatement::BlocksCollection* m_blocks;
    const Fn& m_fn;
    bool m_throwError{};

    explicit TemplateImplVisitor(const Fn& fn, bool throwError)
        : m_fn(fn)
        , m_throwError(throwError)
    {
    }

    template<typename CharT>
    Result operator()(nonstd::expected<std::shared_ptr<TemplateImpl<CharT>>, ErrorInfoTpl<CharT>> tpl) const
    {
        if (!m_throwError && !tpl)
        {
            return Result{};
        }
        else if (!tpl)
        {
            throw tpl.error();
        }
        return m_fn(tpl.value());
    }

    Result operator()(EmptyValue) const { return Result(); }
};

template<typename Result, typename Fn, typename Arg>
Result VisitTemplateImpl(Arg&& tpl, bool throwError, Fn&& fn)
{
    return visit(TemplateImplVisitor<Result, Fn>(fn, throwError), tpl);
}

template<template<typename T> class RendererTpl, typename CharT, typename... Args>
auto CreateTemplateRenderer(std::shared_ptr<TemplateImpl<CharT>> tpl, Args&&... args)
{
    return std::make_shared<RendererTpl<CharT>>(tpl, std::forward<Args>(args)...);
}

void ExtendsStatement::Render(OutStream& os, RenderContext& values)
{
    if (!m_isPath)
    {
        // FIXME: Implement processing of templates
        return;
    }
    auto tpl = values.GetRendererCallback()->LoadTemplate(m_templateName);
    auto renderer =
        VisitTemplateImpl<RendererPtr>(tpl, true, [this](auto tplPtr) { return CreateTemplateRenderer<ParentTemplateRenderer>(tplPtr, &m_blocks); });
    if (renderer)
        renderer->Render(os, values);
}

template<typename CharT>
class IncludedTemplateRenderer : public IRendererBase
{
public:
    IncludedTemplateRenderer(std::shared_ptr<TemplateImpl<CharT>> tpl, bool withContext)
        : m_template(tpl)
        , m_withContext(withContext)
    {
    }

    void Render(OutStream& os, RenderContext& values) override
    {
        RenderContext innerContext = values.Clone(m_withContext);
        if (m_withContext)
            innerContext.EnterScope();

        m_template->GetRenderer()->Render(os, innerContext);
        if (m_withContext)
        {
            auto& innerScope = innerContext.GetCurrentScope();
            auto& scope = values.GetCurrentScope();
            for (auto& v : innerScope)
            {
                scope[v.first] = std::move(v.second);
            }
        }
    }

    bool IsEqual(const IComparable& other) const override
    {
        auto* val = dynamic_cast<const IncludedTemplateRenderer<CharT>*>(&other);
        if (!val)
            return false;
        if (m_template != val->m_template)
            return false;
        if (m_withContext != val->m_withContext)
            return false;
        return true;
    }

private:
    std::shared_ptr<TemplateImpl<CharT>> m_template;
    bool m_withContext{};
};

void IncludeStatement::Render(OutStream& os, RenderContext& values)
{
    auto templateNames = m_expr->Evaluate(values);
    bool isConverted = false;
    ListAdapter list = ConvertToList(templateNames, isConverted);

    auto doRender = [this, &values, &os](auto&& name) -> bool {
        auto tpl = values.GetRendererCallback()->LoadTemplate(name);

        try
        {
            auto renderer = VisitTemplateImpl<RendererPtr>(
                tpl, true, [this](auto tplPtr) { return CreateTemplateRenderer<IncludedTemplateRenderer>(tplPtr, m_withContext); });

            if (renderer)
            {
                renderer->Render(os, values);
                return true;
            }
        }
        catch (const ErrorInfoTpl<char>& err)
        {
            if (err.GetCode() != ErrorCode::FileNotFound)
                throw;
        }
        catch (const ErrorInfoTpl<wchar_t>& err)
        {
            if (err.GetCode() != ErrorCode::FileNotFound)
                throw;
        }

        return false;
    };

    bool rendered = false;
    if (isConverted)
    {
        for (auto& name : list)
        {
            rendered = doRender(name);
            if (rendered)
                break;
        }
    }
    else
    {
        rendered = doRender(templateNames);
    }

    if (!rendered && !m_ignoreMissing)
    {
        InternalValueList files;
        ValuesList extraParams;
        if (isConverted)
        {
            extraParams.push_back(IntValue2Value(templateNames));
        }
        else
        {
            files.push_back(templateNames);
            extraParams.push_back(IntValue2Value(ListAdapter::CreateAdapter(std::move(files))));
        }

        values.GetRendererCallback()->ThrowRuntimeError(ErrorCode::TemplateNotFound, std::move(extraParams));
    }
}

class ImportedMacroRenderer : public IRendererBase
{
public:
    explicit ImportedMacroRenderer(InternalValueMap&& map, bool withContext)
        : m_importedContext(std::move(map))
        , m_withContext(withContext)
    {
    }

    void Render(OutStream& /*os*/, RenderContext& /*values*/) override {}

    void InvokeMacro(const Callable& callable, const CallParams& params, OutStream& stream, RenderContext& context)
    {
        auto ctx = context.Clone(m_withContext);
        ctx.BindScope(&m_importedContext);
        callable.GetStatementCallable()(params, stream, ctx);
    }

    static void InvokeMacro(const std::string& contextName, const Callable& callable, const CallParams& params, OutStream& stream, RenderContext& context)
    {
        bool contextValFound = false;
        auto contextVal = context.FindValue(contextName, contextValFound);
        if (!contextValFound)
            return;

        auto rendererPtr = GetIf<RendererPtr>(&contextVal->second);
        if (!rendererPtr)
            return;

        auto renderer = static_cast<ImportedMacroRenderer*>(rendererPtr->get());
        renderer->InvokeMacro(callable, params, stream, context);
    }

    bool IsEqual(const IComparable& other) const override
    {
        auto* val = dynamic_cast<const ImportedMacroRenderer*>(&other);
        if (!val)
            return false;
        if (m_importedContext != val->m_importedContext)
            return false;
        if (m_withContext != val->m_withContext)
            return false;
        return true;
    }

private:
    InternalValueMap m_importedContext;
    bool m_withContext{};
};

void ImportStatement::Render(OutStream& /*os*/, RenderContext& values)
{
    auto name = m_nameExpr->Evaluate(values);

    if (!m_renderer)
    {
        auto tpl = values.GetRendererCallback()->LoadTemplate(name);
        m_renderer = VisitTemplateImpl<RendererPtr>(tpl, true, [](auto tplPtr) { return CreateTemplateRenderer<IncludedTemplateRenderer>(tplPtr, true); });
    }

    if (!m_renderer)
        return;

    std::string scopeName;
    {
        TargetString tsScopeName = values.GetRendererCallback()->GetAsTargetString(name);
        scopeName = "$$_imported_" + GetAsSameString(scopeName, tsScopeName).value();
    }

    TargetString str;
    auto tmpStream = values.GetRendererCallback()->GetStreamOnString(str);

    RenderContext newContext = values.Clone(m_withContext);
    InternalValueMap importedScope;
    {
        auto& intImportedScope = newContext.EnterScope();
        m_renderer->Render(tmpStream, newContext);
        importedScope = std::move(intImportedScope);
    }

    ImportNames(values, importedScope, scopeName);
    values.GetCurrentScope()[scopeName] =
        std::static_pointer_cast<IRendererBase>(std::make_shared<ImportedMacroRenderer>(std::move(importedScope), m_withContext));
}

void ImportStatement::ImportNames(RenderContext& values, InternalValueMap& importedScope, const std::string& scopeName) const
{
    InternalValueMap importedNs;

    for (auto& var : importedScope)
    {
        if (var.first.empty())
            continue;

        if (var.first[0] == '_')
            continue;

        auto mappedP = m_namesToImport.find(var.first);
        if (!m_namespace && mappedP == m_namesToImport.end())
            continue;

        InternalValue imported;
        auto callable = GetIf<Callable>(&var.second);
        if (!callable)
        {
            imported = std::move(var.second);
        }
        else if (callable->GetKind() == Callable::Macro)
        {
            auto attributes = callable->GetAttributes();
            Callable wrapper(Callable::Macro, [fn = std::move(*callable), scopeName](const CallParams& params, OutStream& stream, RenderContext& context) {
                ImportedMacroRenderer::InvokeMacro(scopeName, fn, params, stream, context);
            });
            wrapper.SetAttributes(std::move(attributes));
            imported = std::move(wrapper);
        }
        else
        {
            continue;
        }

        if (m_namespace)
            importedNs[var.first] = std::move(imported);
        else
            values.GetCurrentScope()[mappedP->second] = std::move(imported);
    }

    if (m_namespace)
        values.GetCurrentScope()[m_namespace.value()] = CreateMapAdapter(std::move(importedNs));
}

Callable MacroStatement::MakeCallable(RenderContext& values) const
{
    std::vector<InternalValue> definedDefaults(m_params.size());
    for (std::size_t idx = 0; idx < m_params.size(); ++idx)
    {
        auto& p = m_params[idx];
        if (p.defaultValue && !p.defaultRefersToArgs)
            definedDefaults[idx] = p.defaultValue->Evaluate(values);
    }

    Callable result(Callable::Macro, [this, defaults = std::move(definedDefaults)](const CallParams& callParams, OutStream& stream, RenderContext& context) {
        InvokeMacroRenderer(defaults, callParams, stream, context);
    });
    result.SetAttributes(m_attributes);
    return result;
}

void MacroStatement::Render(OutStream&, RenderContext& values)
{
    values.GetCurrentScope()[m_name] = MakeCallable(values);
}

InternalValue MacroStatement::GetMacroName() const
{
    return InternalValue(m_name);
}

std::string MacroStatement::GetDisplayName() const
{
    return IsEmpty(GetMacroName()) ? "None"s : "'" + m_name + "'";
}

std::shared_ptr<const InternalValueMap> MacroStatement::MakeAttributes() const
{
    InternalValueList arguments;
    for (auto& p : m_params)
        arguments.emplace_back(p.paramName);

    auto attributes = std::make_shared<InternalValueMap>();
    (*attributes)["name"s] = GetMacroName();
    (*attributes)["arguments"s] = ListAdapter::CreateAdapter(std::move(arguments));
    (*attributes)["catch_kwargs"s] = InternalValue((m_specialNames & UsesKwargs) != 0);
    (*attributes)["catch_varargs"s] = InternalValue((m_specialNames & UsesVarargs) != 0);
    (*attributes)["caller"s] = InternalValue((m_specialNames & UsesCaller) != 0);
    return attributes;
}

// Binds the call arguments the way Jinja2's Macro.__call__ does
void MacroStatement::InvokeMacroRenderer(const std::vector<InternalValue>& definedDefaults,
                                         const CallParams& callParams,
                                         OutStream& stream,
                                         RenderContext& context) const
{
    const auto& posParams = callParams.posParams;
    auto kwParams = callParams.kwParams;
    const auto argsCount = m_params.size();

    std::vector<InternalValue> args(argsCount);
    std::vector<bool> isProvided(argsCount, false);
    bool hasCallerParam = false;
    for (std::size_t idx = 0; idx < argsCount; ++idx)
    {
        auto& name = m_params[idx].paramName;
        hasCallerParam = hasCallerParam || name == "caller";
        if (idx < posParams.size())
        {
            args[idx] = posParams[idx];
            isProvided[idx] = true;
            continue;
        }

        auto p = kwParams.find(name);
        if (p == kwParams.end())
            continue;

        args[idx] = std::move(p->second);
        isProvided[idx] = true;
        kwParams.erase(p);
    }

    const bool catchCaller = (m_specialNames & UsesCaller) != 0 && !hasCallerParam;
    InternalValue caller;
    if (catchCaller)
    {
        auto p = kwParams.find("caller");
        if (p != kwParams.end())
        {
            caller = std::move(p->second);
            kwParams.erase(p);
        }
    }

    const bool catchKwargs = (m_specialNames & UsesKwargs) != 0;
    if (!catchKwargs && !kwParams.empty())
    {
        if (kwParams.count("caller"))
            throw std::runtime_error("macro " + GetDisplayName() + " was invoked with two values for the special caller argument. This is most likely a bug.");
        throw std::runtime_error("macro " + GetDisplayName() + " takes no keyword argument '" + kwParams.begin()->first + "'");
    }

    const bool catchVarargs = (m_specialNames & UsesVarargs) != 0;
    if (!catchVarargs && posParams.size() > argsCount)
        throw std::runtime_error("macro " + GetDisplayName() + " takes not more than " + std::to_string(argsCount) + " argument(s)");

    // Missing arguments are bound before the defaults are evaluated, so a default sees the
    // earlier arguments and never an outer variable named like a later one
    auto& scope = context.EnterScope();
    for (std::size_t idx = 0; idx < argsCount; ++idx)
        scope[m_params[idx].paramName] = std::move(args[idx]);

    for (std::size_t idx = 0; idx < argsCount; ++idx)
    {
        auto& p = m_params[idx];
        if (isProvided[idx] || !p.defaultValue)
            continue;

        auto value = p.defaultRefersToArgs ? p.defaultValue->Evaluate(context) : definedDefaults[idx];
        scope[p.paramName] = std::move(value);
    }

    if (catchCaller)
        scope["caller"s] = std::move(caller);
    if (catchKwargs)
    {
        InternalValueMap kwArgs;
        for (auto& kw : kwParams)
            kwArgs[kw.first] = std::move(kw.second);
        scope["kwargs"s] = CreateMapAdapter(std::move(kwArgs));
    }
    if (catchVarargs)
    {
        InternalValueList varArgs;
        for (auto idx = argsCount; idx < posParams.size(); ++idx)
            varArgs.push_back(posParams[idx]);
        scope["varargs"s] = ListAdapter::CreateAdapter(std::move(varArgs));
    }

    m_mainBody->Render(stream, context);

    context.ExitScope();
}

void MacroCallStatement::Render(OutStream& os, RenderContext& values)
{
    bool isMacroFound = false;
    auto macroPtr = values.FindValue(m_macroName, isMacroFound);
    if (!isMacroFound)
        return;

    auto& fnVal = macroPtr->second;
    const Callable* callable = GetIf<Callable>(&fnVal);
    if (callable == nullptr || callable->GetType() == Callable::Type::Expression)
        return;

    auto callParams = helpers::EvaluateCallParams(m_callParams, values);
    callParams.kwParams["caller"s] = MakeCallable(values);
    callable->GetStatementCallable()(callParams, os, values);
}

InternalValue MacroCallStatement::GetMacroName() const
{
    return InternalValue();
}

void DoStatement::Render(OutStream& /*os*/, RenderContext& values)
{
    m_expr->Evaluate(values);
}

void WithStatement::Render(OutStream& os, RenderContext& values)
{
    auto innerValues = values.Clone(true);
    auto& scope = innerValues.EnterScope();

    for (auto& var : m_scopeVars)
        scope[var.first] = var.second->Evaluate(values);

    m_mainBody->Render(os, innerValues);

    innerValues.ExitScope();
}

void FilterStatement::Render(OutStream& os, RenderContext& values)
{
    TargetString arg;
    auto argStream = values.GetRendererCallback()->GetStreamOnString(arg);
    auto innerValues = values.Clone(true);
    m_body->Render(argStream, innerValues);
    const auto result = m_expr->Evaluate(std::move(arg), values);
    os.WriteValue(result);
}
} // namespace jinja2
