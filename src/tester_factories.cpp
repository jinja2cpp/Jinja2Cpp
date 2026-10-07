// The table of the built-in tests, apart from their bodies in testers.cpp: the factories
// instantiate every tester class's constructors, which would push the testers' own code past
// what the compiler inlines in one translation unit
#include "testers.h"

#include "expression_evaluator.h"
#include "internal_value.h"
#include "node_arena.h"
#include "render_context.h"
#include "value_visitors.h"

#include <jinja2cpp/template_env.h>
#include <jinja2cpp/value.h>

#include <algorithm>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace jinja2
{

namespace
{
using ITester = IsExpression::ITester;

// Makes an F, with its mode if the class serves several tests
template<typename F, auto... mode>
struct TesterFactory
{
    // On the heap, for a test named at render time
    static TesterPtr Create(const TesterParams& params) { return std::make_shared<F>(params, mode...); }
    // In the arena of the template that names it
    static NodeRef<ITester> Make(NodeArena& nodes, const TesterParams& params) { return nodes.MakeObject<ITester, F>(params, mode...); }
};

struct TesterEntry
{
    std::string_view name;
    TesterPtr (*create)(const TesterParams& params);
    NodeRef<ITester> (*make)(NodeArena& nodes, const TesterParams& params);
};

template<typename F, auto... mode>
TesterEntry Entry(std::string_view name)
{
    return { name, &TesterFactory<F, mode...>::Create, &TesterFactory<F, mode...>::Make };
}

// Sorted by name for a binary search
const std::vector<TesterEntry>& BuiltinTesters()
{
    static const std::vector<TesterEntry> testers = [] {
        std::vector<TesterEntry> result = {
            Entry<testers::ValueTester, testers::ValueTester::IsBooleanMode>("boolean"),
            Entry<testers::ValueTester, testers::ValueTester::IsCallableMode>("callable"),
            Entry<testers::ValueTester, testers::ValueTester::IsDefinedMode>("defined"),
            Entry<testers::ValueTester, testers::ValueTester::IsDivisibleByMode>("divisibleby"),
            Entry<testers::StartsWith>("startsWith"),
            Entry<testers::Comparator, BinaryExpression::LogicalEq>("eq"),
            Entry<testers::Comparator, BinaryExpression::LogicalEq>("=="),
            Entry<testers::Comparator, BinaryExpression::LogicalEq>("equalto"),
            Entry<testers::ValueTester, testers::ValueTester::IsEscapedMode>("escaped"),
            Entry<testers::ValueTester, testers::ValueTester::IsEvenMode>("even"),
            Entry<testers::ValueTester, testers::ValueTester::IsFalseMode>("false"),
            Entry<testers::ValueTester, testers::ValueTester::IsFilterMode>("filter"),
            Entry<testers::ValueTester, testers::ValueTester::IsFloatMode>("float"),
            Entry<testers::Comparator, BinaryExpression::LogicalGe>("ge"),
            Entry<testers::Comparator, BinaryExpression::LogicalGe>(">="),
            Entry<testers::Comparator, BinaryExpression::LogicalGt>("gt"),
            Entry<testers::Comparator, BinaryExpression::LogicalGt>(">"),
            Entry<testers::Comparator, BinaryExpression::LogicalGt>("greaterthan"),
            Entry<testers::ValueTester, testers::ValueTester::IsInMode>("in"),
            Entry<testers::ValueTester, testers::ValueTester::IsIntegerMode>("integer"),
            Entry<testers::ValueTester, testers::ValueTester::IsIterableMode>("iterable"),
            Entry<testers::Comparator, BinaryExpression::LogicalLe>("le"),
            Entry<testers::Comparator, BinaryExpression::LogicalLe>("<="),
            Entry<testers::ValueTester, testers::ValueTester::IsLowerMode>("lower"),
            Entry<testers::Comparator, BinaryExpression::LogicalLt>("lt"),
            Entry<testers::Comparator, BinaryExpression::LogicalLt>("<"),
            Entry<testers::Comparator, BinaryExpression::LogicalLt>("lessthan"),
            Entry<testers::ValueTester, testers::ValueTester::IsMappingMode>("mapping"),
            Entry<testers::Comparator, BinaryExpression::LogicalNe>("ne"),
            Entry<testers::Comparator, BinaryExpression::LogicalNe>("!="),
            Entry<testers::ValueTester, testers::ValueTester::IsNoneMode>("none"),
            Entry<testers::ValueTester, testers::ValueTester::IsNumberMode>("number"),
            Entry<testers::ValueTester, testers::ValueTester::IsOddMode>("odd"),
            Entry<testers::ValueTester, testers::ValueTester::IsSameAsMode>("sameas"),
            Entry<testers::ValueTester, testers::ValueTester::IsSequenceMode>("sequence"),
            Entry<testers::ValueTester, testers::ValueTester::IsStringMode>("string"),
            Entry<testers::ValueTester, testers::ValueTester::IsTestMode>("test"),
            Entry<testers::ValueTester, testers::ValueTester::IsTrueMode>("true"),
            Entry<testers::ValueTester, testers::ValueTester::IsUndefinedMode>("undefined"),
            Entry<testers::ValueTester, testers::ValueTester::IsUpperMode>("upper"),
        };
        std::sort(result.begin(), result.end(), [](const TesterEntry& lhs, const TesterEntry& rhs) { return lhs.name < rhs.name; });
        return result;
    }();
    return testers;
}

const TesterEntry* FindBuiltinTester(std::string_view testerName)
{
    const auto& testers = BuiltinTesters();
    auto p = std::lower_bound(testers.begin(), testers.end(), testerName, [](const TesterEntry& entry, std::string_view name) { return entry.name < name; });
    return p != testers.end() && p->name == testerName ? &*p : nullptr;
}
} // namespace

bool IsBuiltinTester(std::string_view testerName)
{
    return FindBuiltinTester(testerName) != nullptr;
}

TesterPtr CreateTester(std::string testerName, const CallParamsInfo& params)
{
    if (const auto* entry = FindBuiltinTester(testerName))
    {
        return entry->create(params);
    }
    return std::make_shared<testers::UserDefinedTester>(std::move(testerName), params);
}

NodeRef<IsExpression::ITester> CreateTester(NodeArena& nodes, const std::string& testerName, const CallParamsInfo& params)
{
    if (const auto* entry = FindBuiltinTester(testerName))
    {
        return entry->make(nodes, params);
    }
    return nodes.MakeObject<ITester, testers::UserDefinedTester>(testerName, params);
}

TesterPtr CreateTester(std::string testerName, const CallParamsInfo& params, RenderContext& context)
{
    auto* env = context.GetEnv();
    auto registered = env ? env->FindTest(testerName) : std::optional<UserCallable>();
    if (!registered)
    {
        return CreateTester(std::move(testerName), params);
    }
    auto callable = visitors::InputValueConvertor::ConvertUserCallable(*registered);
    return std::make_shared<testers::UserDefinedTester>(std::move(testerName), params, std::move(callable));
}

} // namespace jinja2
