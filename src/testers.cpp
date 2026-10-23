#include "testers.h"

#include "engine_errors.h"
#include "big_int.h"
#include "expression_evaluator.h"
#include "filters.h"
#include "function_base.h"
#include "internal_value.h"
#include "render_context.h"
#include "template_dict.h"
#include "undefined.h"
#include "value_visitors.h"

#include <jinja2cpp/value.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <new>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

using namespace std::string_literals;

namespace jinja2
{
inline namespace v2
{
namespace testers
{

Comparator::Comparator(const TesterParams& params, BinaryExpression::Operation op)
    : m_op(op)
{
    static const auto args = MakeArgumentsTable({ { "b", true } });
    ParseParams(args, params);
    // Python's operator.eq and friends take their arguments by position only
    if (!params.kwParams.empty())
    {
        SetArgumentsError("takes no keyword arguments");
    }
}

namespace
{
// Out of line, so that the tests that fit their call pay only the check
[[noreturn]] JINJA2CPP_NOINLINE_INLINE void ThrowArgumentsError(const std::string& error)
{
    throw TemplateRuntimeError(error);
}
} // namespace

bool Comparator::Test(const InternalValue& baseVal, RenderContext& context)
{
    if (const auto& error = GetArgumentsError(); !error.empty())
    {
        ThrowArgumentsError(error);
    }
    auto b = GetArgumentValue("b", context);

    auto cmpRes = Apply2<visitors::BinaryMathOperation>(baseVal, b, m_op);
    return ConvertToBool(cmpRes);
}

StartsWith::StartsWith(const TesterParams& params)
{
    static const auto args = MakeArgumentsTable({ { "str", true } });
    ParseParams(args, params);
}

bool StartsWith::Test(const InternalValue& baseVal, RenderContext& context)
{
    if (const auto& error = GetArgumentsError(); !error.empty())
    {
        ThrowArgumentsError(error);
    }
    InternalValue val = GetArgumentValue("str", context);
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
    {
        static const auto args = MakeArgumentsTable({ { "num", true } });
        ParseParams(args, params);
        break;
    }
    case IsInMode:
    {
        static const auto args = MakeArgumentsTable({ { "seq", true } });
        ParseParams(args, params);
        break;
    }
    case IsSameAsMode:
    {
        static const auto args = MakeArgumentsTable({ { "other", true } });
        ParseParams(args, params);
        break;
    }
    default:
        // The other tests take no argument beyond the tested value
        ParseParams(NoArguments(), params);
        break;
    }
}

enum class TestKind
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
    // An object stored as a map (namespace(), cycler()): not a mapping, not iterable
    Object,
    KVPair,
    Callable,
    Renderer
};

struct ValueKindGetter : visitors::BaseVisitor<TestKind>
{
    using visitors::BaseVisitor<TestKind>::operator();

    TestKind operator()(const UndefinedValue&) const
    {
        return TestKind::Undefined;
    }
    TestKind operator()(const NoneValue&) const
    {
        return TestKind::Empty;
    }
    TestKind operator()(bool) const
    {
        return TestKind::Boolean;
    }
    template<typename CharT>
    TestKind operator()(const std::basic_string<CharT>&) const
    {
        return TestKind::String;
    }
    template<typename CharT>
    TestKind operator()(const std::basic_string_view<CharT>&) const
    {
        return TestKind::String;
    }
    TestKind operator()(int64_t) const
    {
        return TestKind::Integer;
    }
    // NOLINTNEXTLINE(bugprone-derived-method-shadowing-base-method): replaces BaseVisitor's deleted overload
    TestKind operator()(const BigIntView&) const
    {
        return TestKind::Integer;
    }
    TestKind operator()(double) const
    {
        return TestKind::Double;
    }
    TestKind operator()(const ListRef&) const
    {
        return TestKind::List;
    }
    TestKind operator()(const MapRef& map) const
    {
        return map.GetObjectClass() ? TestKind::Object : TestKind::Map;
    }
    TestKind operator()(const KeyValuePair&) const
    {
        return TestKind::KVPair;
    }
    TestKind operator()(const Callable&) const
    {
        return TestKind::Callable;
    }
    TestKind operator()(IRendererBase*) const
    {
        return TestKind::Renderer;
    }
};

namespace
{

// `name` resolves to a callable the user registered, which filters and tests fall back to
bool IsUserCallableName(const std::string& name, RenderContext& context)
{
    if (!context.UserCallablesAsFilters())
    {
        return false;
    }
    const auto value = context.FindValue(name);
    if (!value)
    {
        return false;
    }
    const auto* callable = GetIf<Callable>(&*value);
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
    catch (const std::bad_alloc&)
    {
        throw;
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
    return IsBuiltinTester(name) || (env != nullptr && env->FindTest(name)) || IsUserCallableName(name, context);
}

// Python's `is`: one object. Scalars have no identity here, so equal values of one
// kind count as the same object (CPython caches small ints and interns literals);
// lists and mappings compare by the container they view, pairs by their box.
bool IsSameObject(const InternalValue& left, const InternalValue& right)
{
    auto kind = Apply<ValueKindGetter>(left);
    if (kind != Apply<ValueKindGetter>(right))
    {
        return false;
    }
    switch (kind)
    {
    case TestKind::Undefined:
    case TestKind::Empty:
        return true;
    case TestKind::Boolean:
    case TestKind::String:
    case TestKind::Integer:
    case TestKind::Double:
        return ConvertToBool(Apply2<visitors::BinaryMathOperation>(left, right, BinaryExpression::LogicalEq));
    case TestKind::List:
    {
        auto leftList = AsList(left);
        auto rightList = AsList(right);
        return leftList && rightList && leftList->GetIdentity() == rightList->GetIdentity();
    }
    case TestKind::Map:
    case TestKind::Object:
    {
        auto leftMap = AsMap(left);
        auto rightMap = AsMap(right);
        return leftMap && rightMap && leftMap->GetIdentity() == rightMap->GetIdentity();
    }
    case TestKind::KVPair:
    {
        // A pair lives in a box its copies share: the box is its identity
        const auto* leftPair = GetIf<KeyValuePair>(&left);
        const auto* rightPair = GetIf<KeyValuePair>(&right);
        return leftPair != nullptr && leftPair == rightPair;
    }
    case TestKind::Callable:
    {
        const auto* leftCallable = GetIf<Callable>(&left);
        const auto* rightCallable = GetIf<Callable>(&right);
        return leftCallable != nullptr && rightCallable != nullptr && leftCallable->IsSame(*rightCallable);
    }
    default:
        return false;
    }
}

// Python's value % 2 == 0 (`even`) or == 1 for the number kinds, false for the others
bool IsEvenOrOdd(const InternalValue& val, TestKind valKind, bool even)
{
    bool result = false;
    // bool is an int in Python, so `false is even` holds
    if (const auto big = AsBigInt(val))
    {
        result = IsOdd(*big) != even;
    }
    else if (valKind == TestKind::Integer || valKind == TestKind::Boolean)
    {
        auto intVal = ConvertToInt(val);
        result = (intVal & 1) == (even ? 0 : 1);
    }
    else if (valKind == TestKind::Double)
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
// Out of line, so that the int comparison below needs no frame of its own (docs/tasks/0286)
JINJA2CPP_NOINLINE_INLINE bool IsInEqualByValue(const InternalValue& item, const InternalValue& value)
{
    if (visitors::IsNumber(item) && visitors::IsNumber(value))
    {
        return ConvertToBool(visitors::ApplyToNumbers(item, value, BinaryExpression::LogicalEq));
    }
    return ConvertToBool(Apply2<visitors::BinaryMathOperation>(item, value, BinaryExpression::LogicalEq));
}

bool IsInEqual(const InternalValue& item, const InternalValue& value)
{
    // Two ints, the usual `i in [1, 2, 3]`, compare without the number visitor
    const auto* itemInt = GetIf<int64_t>(&item);
    const auto* valueInt = itemInt ? GetIf<int64_t>(&value) : nullptr;
    if (valueInt)
    {
        return *itemInt == *valueInt;
    }
    return IsInEqualByValue(item, value);
}
// `value in list`: a list the template owns is compared in place, any other through one
// enumerator (ListAdapter::Iterator clones its enumerator on every copy)
bool IsValueInListValue(const InternalValue& baseVal, const InternalValue& seq)
{
    auto list = AsList(seq);
    bool isConverted = false;
    ListAdapter converted;
    if (!list)
    {
        converted = ConvertToList(seq, isConverted);
        if (!isConverted)
        {
            return false;
        }
        list = converted.Ref();
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
    if (seqKind == TestKind::List || seqKind == TestKind::KVPair)
    {
        result = IsValueInListValue(baseVal, seq);
    }
    else if (seqKind == TestKind::Map)
    {
        // `key in dict` tests the keys: any hashable value for a dict the template owns, strings
        // for every other map
        auto map = AsMap(seq);
        if (map && Apply<ValueKindGetter>(baseVal) == TestKind::String)
        {
            result = map->HasValue(AsString(baseVal));
        }
        else if (map)
        {
            if (const char* type = UnhashableType(baseVal))
            {
                ThrowUnhashable(type);
            }
            result = map->FindItem(baseVal).has_value();
        }
    }
    else if (seqKind == TestKind::String)
    {
        if (Apply<ValueKindGetter>(baseVal) != TestKind::String)
        {
            throw TemplateRuntimeError("'in <string>' requires string as left operand, not "s + Apply<visitors::PythonTypeNameGetter>(baseVal));
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
    else if (seqKind == TestKind::Integer || seqKind == TestKind::Double || seqKind == TestKind::Boolean || seqKind == TestKind::Object)
    {
        throw TemplateRuntimeError("argument of type '"s + Apply<visitors::PythonTypeNameGetter>(seq) + "' is not iterable");
    }
    return result;
}

bool ValueTester::Test(const InternalValue& baseVal, RenderContext& context)
{
    // divisibleby, in and sameas take exactly one argument, the others none, as in Jinja2
    if (const auto& error = GetArgumentsError(); !error.empty())
    {
        ThrowArgumentsError(error);
    }
    bool result = false;
    auto valKind = Apply<ValueKindGetter>(baseVal);

    switch (m_mode)
    {
    case IsBooleanMode:
        result = valKind == TestKind::Boolean;
        break;
    case IsCallableMode:
        // Python's Undefined defines __call__ (that fails). A plain undefined may be a host's
        // null (task 0047), so only a missing name counts
        result = valKind == TestKind::Callable || GetUndefinedInfo(baseVal) != nullptr;
        if (valKind == TestKind::Map || valKind == TestKind::Object)
        {
            // An object stored as a map that defines a call (joiner(), a recursive loop)
            auto map = AsMap(baseVal);
            result = map && map->GetAttrPolicy() == MapAttrPolicy::KeysOnly && map->HasValue("operator()");
        }
        break;
    case IsEscapedMode:
        // Python checks for __html__, which only Markup has
        result = baseVal.IsMarkup();
        break;
    case IsFalseMode:
        result = valKind == TestKind::Boolean && !ConvertToBool(baseVal);
        break;
    case IsTrueMode:
        result = valKind == TestKind::Boolean && ConvertToBool(baseVal);
        break;
    case IsFloatMode:
        result = valKind == TestKind::Double;
        break;
    case IsIntegerMode:
        // bool is a separate kind here, so `true is integer` is false as in Jinja2
        result = valKind == TestKind::Integer;
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
        result = valKind == TestKind::String && IsFilterName(AsString(baseVal), context);
        break;
    case IsTestMode:
        result = valKind == TestKind::String && IsTestName(AsString(baseVal), context);
        break;
    case IsIterableMode:
        result = valKind == TestKind::List || valKind == TestKind::KVPair || valKind == TestKind::Map || valKind == TestKind::String;
        // Of the objects, only `self` is: Jinja2's TemplateReference has __getitem__
        if (valKind == TestKind::Object)
        {
            auto map = AsMap(baseVal);
            result = map && std::string_view(map->GetObjectClass()) == "TemplateReference";
        }
        break;
    case IsMappingMode:
        // A dictsort pair is a 2-tuple, not a mapping, and neither is an object stored as a
        // map (loop, an imported template)
        if (valKind == TestKind::Map)
        {
            auto map = AsMap(baseVal);
            result = map && map->GetAttrPolicy() != MapAttrPolicy::KeysOnly;
        }
        break;
    case IsNumberMode:
        // Python's numbers.Number, which bool belongs to
        result = valKind == TestKind::Integer || valKind == TestKind::Double || valKind == TestKind::Boolean;
        break;
    case IsSequenceMode:
        // A dict is a sequence in Python (len and __getitem__), a reflected object is not
        result = valKind == TestKind::List || valKind == TestKind::KVPair || valKind == TestKind::String;
        if (valKind == TestKind::Map)
        {
            auto map = AsMap(baseVal);
            // Dicts only: a reflected object, loop and namespace() are not
            result = map && map->GetAttrPolicy() == MapAttrPolicy::MethodsFirst;
        }
        break;
    case IsStringMode:
        result = valKind == TestKind::String;
        break;
    case IsDefinedMode:
        result = valKind != TestKind::Undefined;
        break;
    case IsUndefinedMode:
        result = valKind == TestKind::Undefined;
        break;
    case IsNoneMode:
        result = valKind == TestKind::Empty;
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
        result = valKind == TestKind::String && HasNoUpperLetter(baseVal);
        break;
    case IsUpperMode:
        result = valKind == TestKind::String && HasNoLowerLetter(baseVal);
        break;
    }
    return result;
}

UserDefinedTester::UserDefinedTester(std::string testerName, const TesterParams& params, InternalValue callable)
    : m_testerName(std::move(testerName))
    , m_callable(std::move(callable))
{
    static const auto args = MakeArgumentsTable({ { "*args" }, { "**kwargs" } });
    ParseParams(args, params, ExtraArgs::Reject, &m_callParams);
}

bool UserDefinedTester::Test(const InternalValue& baseVal, RenderContext& context)
{
    const Callable* callable = GetIf<Callable>(&m_callable);
    if (!callable && context.UserCallablesAsFilters())
    {
        const auto testerVal = context.FindValue(m_testerName);
        callable = testerVal ? GetIf<Callable>(&*testerVal) : nullptr;
    }
    // Neither built in nor added to the environment: Jinja2 has checked the name at load
    // unless it is in an `if`. Settings::userCallablesAsFilters looks it up among the values
    if (!callable || callable->GetKind() != Callable::UserCallable)
    {
        throw TemplateRuntimeError("No test named '" + m_testerName + "' found.");
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
} // namespace v2
} // namespace jinja2
