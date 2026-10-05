#include "testers.h"

#include "expression_evaluator.h"
#include "filters.h"
#include "internal_value.h"
#include "render_context.h"
#include "undefined.h"
#include "value_visitors.h"

#include <jinja2cpp/value.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>

using namespace std::string_literals;

namespace jinja2
{

template<typename F>
struct TesterFactory
{
    static TesterPtr Create(const TesterParams& params)
    {
        return std::make_shared<F>(params);
    }

    template<typename... Args>
    static IsExpression::TesterFactoryFn MakeCreator(const Args&... args)
    {
        return [args...](const TesterParams& params) { return std::make_shared<F>(params, args...); };
    }
};

// NOLINTNEXTLINE(bugprone-throwing-static-initialization): only allocation can throw here, at load time
std::unordered_map<std::string, IsExpression::TesterFactoryFn> s_testers = {
    { "boolean", TesterFactory<testers::ValueTester>::MakeCreator(testers::ValueTester::IsBooleanMode) },
    { "callable", TesterFactory<testers::ValueTester>::MakeCreator(testers::ValueTester::IsCallableMode) },
    { "defined", TesterFactory<testers::ValueTester>::MakeCreator(testers::ValueTester::IsDefinedMode) },
    { "divisibleby", TesterFactory<testers::ValueTester>::MakeCreator(testers::ValueTester::IsDivisibleByMode) },
    { "startsWith", &TesterFactory<testers::StartsWith>::Create },
    { "eq", TesterFactory<testers::Comparator>::MakeCreator(BinaryExpression::LogicalEq) },
    { "==", TesterFactory<testers::Comparator>::MakeCreator(BinaryExpression::LogicalEq) },
    { "equalto", TesterFactory<testers::Comparator>::MakeCreator(BinaryExpression::LogicalEq) },
    { "escaped", TesterFactory<testers::ValueTester>::MakeCreator(testers::ValueTester::IsEscapedMode) },
    { "even", TesterFactory<testers::ValueTester>::MakeCreator(testers::ValueTester::IsEvenMode) },
    { "false", TesterFactory<testers::ValueTester>::MakeCreator(testers::ValueTester::IsFalseMode) },
    { "filter", TesterFactory<testers::ValueTester>::MakeCreator(testers::ValueTester::IsFilterMode) },
    { "float", TesterFactory<testers::ValueTester>::MakeCreator(testers::ValueTester::IsFloatMode) },
    { "ge", TesterFactory<testers::Comparator>::MakeCreator(BinaryExpression::LogicalGe) },
    { ">=", TesterFactory<testers::Comparator>::MakeCreator(BinaryExpression::LogicalGe) },
    { "gt", TesterFactory<testers::Comparator>::MakeCreator(BinaryExpression::LogicalGt) },
    { ">", TesterFactory<testers::Comparator>::MakeCreator(BinaryExpression::LogicalGt) },
    { "greaterthan", TesterFactory<testers::Comparator>::MakeCreator(BinaryExpression::LogicalGt) },
    { "in", TesterFactory<testers::ValueTester>::MakeCreator(testers::ValueTester::IsInMode) },
    { "integer", TesterFactory<testers::ValueTester>::MakeCreator(testers::ValueTester::IsIntegerMode) },
    { "iterable", TesterFactory<testers::ValueTester>::MakeCreator(testers::ValueTester::IsIterableMode) },
    { "le", TesterFactory<testers::Comparator>::MakeCreator(BinaryExpression::LogicalLe) },
    { "<=", TesterFactory<testers::Comparator>::MakeCreator(BinaryExpression::LogicalLe) },
    { "lower", TesterFactory<testers::ValueTester>::MakeCreator(testers::ValueTester::IsLowerMode) },
    { "lt", TesterFactory<testers::Comparator>::MakeCreator(BinaryExpression::LogicalLt) },
    { "<", TesterFactory<testers::Comparator>::MakeCreator(BinaryExpression::LogicalLt) },
    { "lessthan", TesterFactory<testers::Comparator>::MakeCreator(BinaryExpression::LogicalLt) },
    { "mapping", TesterFactory<testers::ValueTester>::MakeCreator(testers::ValueTester::IsMappingMode) },
    { "ne", TesterFactory<testers::Comparator>::MakeCreator(BinaryExpression::LogicalNe) },
    { "!=", TesterFactory<testers::Comparator>::MakeCreator(BinaryExpression::LogicalNe) },
    { "none", TesterFactory<testers::ValueTester>::MakeCreator(testers::ValueTester::IsNoneMode) },
    { "number", TesterFactory<testers::ValueTester>::MakeCreator(testers::ValueTester::IsNumberMode) },
    { "odd", TesterFactory<testers::ValueTester>::MakeCreator(testers::ValueTester::IsOddMode) },
    { "sameas", TesterFactory<testers::ValueTester>::MakeCreator(testers::ValueTester::IsSameAsMode) },
    { "sequence", TesterFactory<testers::ValueTester>::MakeCreator(testers::ValueTester::IsSequenceMode) },
    { "string", TesterFactory<testers::ValueTester>::MakeCreator(testers::ValueTester::IsStringMode) },
    { "test", TesterFactory<testers::ValueTester>::MakeCreator(testers::ValueTester::IsTestMode) },
    { "true", TesterFactory<testers::ValueTester>::MakeCreator(testers::ValueTester::IsTrueMode) },
    { "undefined", TesterFactory<testers::ValueTester>::MakeCreator(testers::ValueTester::IsUndefinedMode) },
    { "upper", TesterFactory<testers::ValueTester>::MakeCreator(testers::ValueTester::IsUpperMode) },
};

TesterPtr CreateTester(std::string testerName, CallParamsInfo params)
{
    auto p = s_testers.find(testerName);
    if (p == s_testers.end())
    {
        return std::make_shared<testers::UserDefinedTester>(std::move(testerName), std::move(params));
    }

    return p->second(std::move(params));
}

TesterPtr CreateTester(std::string testerName, CallParamsInfo params, RenderContext& context)
{
    auto* env = context.GetEnv();
    auto registered = env ? env->FindTest(testerName) : std::optional<UserCallable>();
    if (!registered)
    {
        return CreateTester(std::move(testerName), std::move(params));
    }
    auto callable = visitors::InputValueConvertor::ConvertUserCallable(*registered);
    return std::make_shared<testers::UserDefinedTester>(std::move(testerName), std::move(params), std::move(callable));
}

namespace testers
{

Comparator::Comparator(const TesterParams& params, BinaryExpression::Operation op)
    : m_op(op)
{
    ParseParams({ { "b", true } }, params);
}

bool Comparator::Test(const InternalValue& baseVal, RenderContext& context)
{
    auto b = GetArgumentValue("b", context);

    auto cmpRes = Apply2<visitors::BinaryMathOperation>(baseVal, b, m_op);
    return ConvertToBool(cmpRes);
}

StartsWith::StartsWith(const TesterParams& params)
{
    bool parsed = true;
    auto args = helpers::ParseCallParamsInfo({ { "str", true } }, params, parsed);
    m_stringEval = args["str"];
}

bool StartsWith::Test(const InternalValue& baseVal, RenderContext& context)
{
    InternalValue val = m_stringEval->Evaluate(context);
    std::string baseStr = AsString(baseVal);
    std::string str = AsString(val);
    return baseStr.find(str) == 0;
}

ValueTester::ValueTester(const TesterParams& params, ValueTester::Mode mode)
    : m_mode(mode)
{
    switch (m_mode)
    {
    case IsDivisibleByMode:
        ParseParams({ { "num", true } }, params);
        break;
    case IsInMode:
        ParseParams({ { "seq", true } }, params);
        break;
    case IsSameAsMode:
        ParseParams({ { "other", true } }, params);
        break;
    default:
        break;
    }
}

enum class ValueKind
{
    // First, so the BaseVisitor fallback reads as undefined
    Undefined,
    Empty,
    Boolean,
    String,
    Integer,
    Double,
    List,
    Map,
    KVPair,
    Callable,
    Renderer
};

struct ValueKindGetter : visitors::BaseVisitor<ValueKind>
{
    using visitors::BaseVisitor<ValueKind>::operator();

    ValueKind operator()(const UndefinedValue&) const
    {
        return ValueKind::Undefined;
    }
    ValueKind operator()(const EmptyValue&) const
    {
        return ValueKind::Empty;
    }
    ValueKind operator()(bool) const
    {
        return ValueKind::Boolean;
    }
    template<typename CharT>
    ValueKind operator()(const std::basic_string<CharT>&) const
    {
        return ValueKind::String;
    }
    template<typename CharT>
    ValueKind operator()(const std::basic_string_view<CharT>&) const
    {
        return ValueKind::String;
    }
    ValueKind operator()(int64_t) const
    {
        return ValueKind::Integer;
    }
    ValueKind operator()(double) const
    {
        return ValueKind::Double;
    }
    ValueKind operator()(const ListAdapter&) const
    {
        return ValueKind::List;
    }
    ValueKind operator()(const MapAdapter&) const
    {
        return ValueKind::Map;
    }
    ValueKind operator()(const KeyValuePair&) const
    {
        return ValueKind::KVPair;
    }
    ValueKind operator()(const Callable&) const
    {
        return ValueKind::Callable;
    }
    ValueKind operator()(IRendererBase*) const
    {
        return ValueKind::Renderer;
    }
};

namespace
{

// `name` resolves to a callable the user registered, which filters and tests fall back to
bool IsUserCallableName(const std::string& name, RenderContext& context)
{
    bool found = false;
    const auto* valPtr = context.FindValue(name, found);
    if (!found)
    {
        return false;
    }
    const auto* callable = GetIf<Callable>(&valPtr->second);
    return callable != nullptr && callable->GetKind() == Callable::UserCallable;
}

bool IsFilterName(const std::string& name, RenderContext& context)
{
    // CreateFilter falls back to UserDefinedFilter for names it does not know
    FilterPtr filter;
    try
    {
        filter = CreateFilter(name, CallParamsInfo());
    }
    catch (...)
    {
        // A builtin that rejects an empty argument list still exists
        return true;
    }
    if (!dynamic_cast<filters::UserDefinedFilter*>(filter.get()))
    {
        return true;
    }
    auto* env = context.GetEnv();
    return (env != nullptr && env->FindFilter(name)) || IsUserCallableName(name, context);
}

bool IsTestName(const std::string& name, RenderContext& context)
{
    auto* env = context.GetEnv();
    return s_testers.count(name) != 0 || (env != nullptr && env->FindTest(name)) || IsUserCallableName(name, context);
}

// Python's `is`: one object. Scalars have no identity here, so equal values of one
// kind count as the same object (CPython caches small ints and interns literals);
// lists and mappings compare by the container they view.
bool IsSameObject(const InternalValue& left, const InternalValue& right)
{
    auto kind = Apply<ValueKindGetter>(left);
    if (kind != Apply<ValueKindGetter>(right))
    {
        return false;
    }
    switch (kind)
    {
    case ValueKind::Undefined:
    case ValueKind::Empty:
        return true;
    case ValueKind::Boolean:
    case ValueKind::String:
    case ValueKind::Integer:
    case ValueKind::Double:
        return ConvertToBool(Apply2<visitors::BinaryMathOperation>(left, right, BinaryExpression::LogicalEq));
    case ValueKind::List:
        return GetIf<ListAdapter>(&left)->GetIdentity() == GetIf<ListAdapter>(&right)->GetIdentity();
    case ValueKind::Map:
        return GetIf<MapAdapter>(&left)->GetIdentity() == GetIf<MapAdapter>(&right)->GetIdentity();
    default:
        return false;
    }
}

// Python's value % 2 == 0 (`even`) or == 1 for the number kinds, false for the others
bool IsEvenOrOdd(const InternalValue& val, ValueKind valKind, bool even)
{
    bool result = false;
    // bool is an int in Python, so `false is even` holds
    if (valKind == ValueKind::Integer || valKind == ValueKind::Boolean)
    {
        auto intVal = ConvertToInt(val);
        result = (intVal & 1) == (even ? 0 : 1);
    }
    else if (valKind == ValueKind::Double)
    {
        // Python's value % 2 == 0 (or 1): no conversion to an integer, which a float
        // outside int64_t's range would overflow; inf and nan are neither
        auto remainder = std::fabs(std::fmod(ConvertToDouble(val), 2.0));
        result = remainder == (even ? 0.0 : 1.0);
    }
    return result;
}

// The string `val` has no uppercase letter
bool HasNoUpperLetter(const InternalValue& val)
{
    return ApplyStringConverter(val, [](const auto& str) {
        bool result = true;
        for (auto& ch : str)
        {
            if (std::isalpha(ch, std::locale()) && std::isupper(ch, std::locale()))
            {
                result = false;
                break;
            }
        }
        return result;
    });
}

// The string `val` has no lowercase letter
bool HasNoLowerLetter(const InternalValue& val)
{
    return ApplyStringConverter(val, [](const auto& str) {
        bool result = true;
        for (auto& ch : str)
        {
            if (std::isalpha(ch, std::locale()) && std::islower(ch, std::locale()))
            {
                result = false;
                break;
            }
        }
        return result;
    });
}

} // namespace

namespace
{
bool IsInEqual(const InternalValue& item, const InternalValue& value)
{
    // Two ints, the usual `i in [1, 2, 3]`, compare without the number visitor
    const auto* itemInt = GetIf<int64_t>(&item);
    const auto* valueInt = itemInt ? GetIf<int64_t>(&value) : nullptr;
    if (valueInt)
    {
        return *itemInt == *valueInt;
    }
    if (visitors::IsNumber(item) && visitors::IsNumber(value))
    {
        return ConvertToBool(visitors::ApplyToNumbers(item, value, BinaryExpression::LogicalEq));
    }
    return ConvertToBool(Apply2<visitors::BinaryMathOperation>(item, value, BinaryExpression::LogicalEq));
}
// `value in list`: a list the template owns is compared in place, any other through one
// enumerator (ListAdapter::Iterator clones its enumerator on every copy)
bool IsValueInListValue(const InternalValue& baseVal, const InternalValue& seq)
{
    const auto* list = GetIf<ListAdapter>(&seq);
    bool isConverted = false;
    ListAdapter converted;
    if (!list)
    {
        converted = ConvertToList(seq, InternalValue(), isConverted);
        if (!isConverted)
        {
            return false;
        }
        list = &converted;
    }

    if (const auto* items = list->GetMutableItems())
    {
        return IsValueInList(baseVal, *items);
    }
    auto enumerator = list->GetEnumerator();
    if (!enumerator)
    {
        return false;
    }
    while ((*enumerator)->MoveNext())
    {
        if (IsInEqual((*enumerator)->GetCurrent(), baseVal))
        {
            return true;
        }
    }
    return false;
}

} // namespace

bool IsValueInList(const InternalValue& baseVal, const InternalValueList& items)
{
    return std::any_of(items.begin(), items.end(), [&baseVal](const InternalValue& item) { return IsInEqual(item, baseVal); });
}

// `value in seq`, Python's containment test
bool IsValueIn(const InternalValue& baseVal, const InternalValue& seq)
{
    bool result = false;
    CheckUndefinedUse(seq, UndefinedUse::Operator);
    auto seqKind = Apply<ValueKindGetter>(seq);
    if (seqKind == ValueKind::List)
    {
        result = IsValueInListValue(baseVal, seq);
    }
    else if (seqKind == ValueKind::Map)
    {
        // `key in dict` tests the keys; dict keys are always strings here
        const auto* map = GetIf<MapAdapter>(&seq);
        result = map != nullptr && Apply<ValueKindGetter>(baseVal) == ValueKind::String && map->HasValue(AsString(baseVal));
    }
    else if (seqKind == ValueKind::String)
    {
        if (Apply<ValueKindGetter>(baseVal) != ValueKind::String)
        {
            throw std::runtime_error("'in <string>' requires string as left operand, not "s + Apply<visitors::PythonTypeNameGetter>(baseVal));
        }
        result = ApplyStringConverter(baseVal, [&](const auto& srcStr) {
            std::decay_t<decltype(srcStr)> emptyStrView;
            using CharT = typename decltype(emptyStrView)::value_type;
            std::basic_string<CharT> emptyStr;

            auto substring = std::basic_string(srcStr);
            auto seqStr = GetAsSameString(srcStr, seq).value_or(emptyStr);

            return seqStr.find(substring) != std::string::npos;
        });
    }
    else if (seqKind == ValueKind::Integer || seqKind == ValueKind::Double || seqKind == ValueKind::Boolean)
    {
        throw std::runtime_error("argument of type '"s + Apply<visitors::PythonTypeNameGetter>(seq) + "' is not iterable");
    }
    return result;
}

bool ValueTester::Test(const InternalValue& baseVal, RenderContext& context)
{
    bool result = false;
    auto valKind = Apply<ValueKindGetter>(baseVal);

    switch (m_mode)
    {
    case IsBooleanMode:
        result = valKind == ValueKind::Boolean;
        break;
    case IsCallableMode:
        result = valKind == ValueKind::Callable;
        break;
    case IsEscapedMode:
        // Python checks for __html__, which only Markup has
        result = baseVal.IsMarkup();
        break;
    case IsFalseMode:
        result = valKind == ValueKind::Boolean && !ConvertToBool(baseVal);
        break;
    case IsTrueMode:
        result = valKind == ValueKind::Boolean && ConvertToBool(baseVal);
        break;
    case IsFloatMode:
        result = valKind == ValueKind::Double;
        break;
    case IsIntegerMode:
        // bool is a separate kind here, so `true is integer` is false as in Jinja2
        result = valKind == ValueKind::Integer;
        break;
    case IsDivisibleByMode:
    {
        // Jinja2: value % num == 0, with Python's errors for zero and non-numbers
        auto num = GetArgumentValue("num", context);
        auto rem = Apply2<visitors::BinaryMathOperation>(baseVal, num, BinaryExpression::DivRemainder);
        result = ConvertToBool(Apply2<visitors::BinaryMathOperation>(rem, InternalValue(static_cast<int64_t>(0)), BinaryExpression::LogicalEq));
        break;
    }
    case IsSameAsMode:
        result = IsSameObject(baseVal, GetArgumentValue("other", context));
        break;
    case IsFilterMode:
        result = valKind == ValueKind::String && IsFilterName(AsString(baseVal), context);
        break;
    case IsTestMode:
        result = valKind == ValueKind::String && IsTestName(AsString(baseVal), context);
        break;
    case IsIterableMode:
        result = valKind == ValueKind::List || valKind == ValueKind::Map || valKind == ValueKind::String;
        break;
    case IsMappingMode:
        result = valKind == ValueKind::KVPair || valKind == ValueKind::Map;
        break;
    case IsNumberMode:
        // Python's numbers.Number, which bool belongs to
        result = valKind == ValueKind::Integer || valKind == ValueKind::Double || valKind == ValueKind::Boolean;
        break;
    case IsSequenceMode:
        result = valKind == ValueKind::List || valKind == ValueKind::String;
        break;
    case IsStringMode:
        result = valKind == ValueKind::String;
        break;
    case IsDefinedMode:
        result = valKind != ValueKind::Undefined;
        break;
    case IsUndefinedMode:
        result = valKind == ValueKind::Undefined;
        break;
    case IsNoneMode:
        result = valKind == ValueKind::Empty;
        break;
    case IsInMode:
        result = IsValueIn(baseVal, GetArgumentValue("seq", context));
        break;
    case IsEvenMode:
        result = IsEvenOrOdd(baseVal, valKind, true);
        break;
    case IsOddMode:
        result = IsEvenOrOdd(baseVal, valKind, false);
        break;
    case IsLowerMode:
        result = valKind == ValueKind::String && HasNoUpperLetter(baseVal);
        break;
    case IsUpperMode:
        result = valKind == ValueKind::String && HasNoLowerLetter(baseVal);
        break;
    }
    return result;
}

UserDefinedTester::UserDefinedTester(std::string testerName, const TesterParams& params, InternalValue callable)
    : m_testerName(std::move(testerName))
    , m_callable(std::move(callable))
{
    ParseParams({ { "*args" }, { "**kwargs" } }, params);
    m_callParams.kwParams = m_args.extraKwArgs;
    m_callParams.posParams = m_args.extraPosArgs;
}

bool UserDefinedTester::Test(const InternalValue& baseVal, RenderContext& context)
{
    const Callable* callable = GetIf<Callable>(&m_callable);
    if (!callable)
    {
        bool testerFound = false;
        const auto* testerValPtr = context.FindValue(m_testerName, testerFound);
        callable = testerFound ? GetIf<Callable>(&testerValPtr->second) : nullptr;
    }
    // Jinja2 rejects an unknown test when compiling; tests registered as user callables
    // are only known at render time, so the error is raised here
    if (!callable || callable->GetKind() != Callable::UserCallable)
    {
        throw std::runtime_error("No test named '" + m_testerName + "'.");
    }

    CallParams tmpCallParams = helpers::EvaluateCallParams(m_callParams, context);
    CallParams callParams;
    callParams.kwParams = std::move(tmpCallParams.kwParams);
    callParams.posParams.reserve(tmpCallParams.posParams.size() + 1);
    callParams.posParams.push_back(baseVal);
    for (auto& p : tmpCallParams.posParams)
    {
        callParams.posParams.push_back(std::move(p));
    }

    InternalValue result;
    if (callable->GetType() != Callable::Type::Expression)
    {
        return false;
    }

    return ConvertToBool(callable->GetExpressionCallable()(callParams, context));
}
} // namespace testers
} // namespace jinja2
