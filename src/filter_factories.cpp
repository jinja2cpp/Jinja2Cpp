// The table of the built-in filters, apart from their bodies in filters.cpp: the factories
// instantiate every filter class's constructors, which would push the filters' own code past
// what the compiler inlines in one translation unit
#include "filters.h"

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
using IExpressionFilter = ExpressionFilter::IExpressionFilter;

// Makes an F, with its mode if the class serves several filters
template<typename F, auto... mode>
struct FilterFactory
{
    // On the heap, for a filter named at render time
    static FilterPtr Create(const FilterParams& params) { return std::make_shared<F>(params, mode...); }
    // In the arena of the template that names it
    static NodeRef<IExpressionFilter> Make(NodeArena& nodes, const FilterParams& params) { return nodes.MakeObject<IExpressionFilter, F>(params, mode...); }
};

struct FilterEntry
{
    std::string_view name;
    FilterPtr (*create)(const FilterParams& params);
    NodeRef<IExpressionFilter> (*make)(NodeArena& nodes, const FilterParams& params);
};

template<typename F, auto... mode>
FilterEntry Entry(std::string_view name)
{
    return { name, &FilterFactory<F, mode...>::Create, &FilterFactory<F, mode...>::Make };
}

// Sorted by name for a binary search
const std::vector<FilterEntry>& BuiltinFilters()
{
    static const std::vector<FilterEntry> filters = [] {
        std::vector<FilterEntry> result = {
            Entry<filters::ValueConverter, filters::ValueConverter::AbsMode>("abs"),
            Entry<filters::ApplyMacro>("applymacro"),
            Entry<filters::Attribute>("attr"),
            Entry<filters::Slice, filters::Slice::BatchMode>("batch"),
            Entry<filters::StringConverter, filters::StringConverter::CamelMode>("camelize"),
            Entry<filters::StringConverter, filters::StringConverter::CapitalMode>("capitalize"),
            Entry<filters::StringConverter, filters::StringConverter::CenterMode>("center"),
            Entry<filters::SequenceAccessor, filters::SequenceAccessor::LengthMode>("count"),
            Entry<filters::Default>("default"),
            Entry<filters::Default>("d"),
            Entry<filters::DictSort>("dictsort"),
            Entry<filters::StringConverter, filters::StringConverter::EscapeHtmlMode>("e"),
            Entry<filters::StringConverter, filters::StringConverter::EscapeHtmlMode>("escape"),
            Entry<filters::StringConverter, filters::StringConverter::EscapeCppMode>("escapecpp"),
            Entry<filters::ValueConverter, filters::ValueConverter::FileSizeFormatMode>("filesizeformat"),
            Entry<filters::SequenceAccessor, filters::SequenceAccessor::FirstItemMode>("first"),
            Entry<filters::ValueConverter, filters::ValueConverter::ToFloatMode>("float"),
            Entry<filters::StringConverter, filters::StringConverter::ForceEscapeMode>("forceescape"),
            Entry<filters::StringFormat>("format"),
            Entry<filters::GroupBy>("groupby"),
            Entry<filters::StringConverter, filters::StringConverter::IndentMode>("indent"),
            Entry<filters::ValueConverter, filters::ValueConverter::ToIntMode>("int"),
            Entry<filters::ValueConverter, filters::ValueConverter::ItemsMode>("items"),
            Entry<filters::Join>("join"),
            Entry<filters::SequenceAccessor, filters::SequenceAccessor::LastItemMode>("last"),
            Entry<filters::SequenceAccessor, filters::SequenceAccessor::LengthMode>("length"),
            Entry<filters::ValueConverter, filters::ValueConverter::ToListMode>("list"),
            Entry<filters::StringConverter, filters::StringConverter::LowerMode>("lower"),
            Entry<filters::Map>("map"),
            Entry<filters::SequenceAccessor, filters::SequenceAccessor::MaxItemMode>("max"),
            Entry<filters::SequenceAccessor, filters::SequenceAccessor::MinItemMode>("min"),
            Entry<filters::PrettyPrint>("pprint"),
            Entry<filters::SequenceAccessor, filters::SequenceAccessor::RandomMode>("random"),
            Entry<filters::Tester, filters::Tester::RejectMode>("reject"),
            Entry<filters::Tester, filters::Tester::RejectAttrMode>("rejectattr"),
            Entry<filters::StringConverter, filters::StringConverter::ReplaceMode>("replace"),
            Entry<filters::ValueConverter, filters::ValueConverter::RoundMode>("round"),
            Entry<filters::SequenceAccessor, filters::SequenceAccessor::ReverseMode>("reverse"),
            Entry<filters::StringConverter, filters::StringConverter::SafeMode>("safe"),
            Entry<filters::Tester, filters::Tester::SelectMode>("select"),
            Entry<filters::Tester, filters::Tester::SelectAttrMode>("selectattr"),
            Entry<filters::Slice, filters::Slice::SliceMode>("slice"),
            Entry<filters::Sort>("sort"),
            Entry<filters::StringConverter, filters::StringConverter::ToStringMode>("string"),
            Entry<filters::StringConverter, filters::StringConverter::StriptagsMode>("striptags"),
            Entry<filters::SequenceAccessor, filters::SequenceAccessor::SumItemsMode>("sum"),
            Entry<filters::StringConverter, filters::StringConverter::TitleMode>("title"),
            Entry<filters::Serialize, filters::Serialize::JsonMode>("tojson"),
            Entry<filters::Serialize, filters::Serialize::XmlMode>("toxml"),
            Entry<filters::Serialize, filters::Serialize::YamlMode>("toyaml"),
            Entry<filters::StringConverter, filters::StringConverter::TrimMode>("trim"),
            Entry<filters::StringConverter, filters::StringConverter::TruncateMode>("truncate"),
            Entry<filters::SequenceAccessor, filters::SequenceAccessor::UniqueItemsMode>("unique"),
            Entry<filters::StringConverter, filters::StringConverter::UpperMode>("upper"),
            Entry<filters::StringConverter, filters::StringConverter::UrlEncodeMode>("urlencode"),
            Entry<filters::StringConverter, filters::StringConverter::UrlizeMode>("urlize"),
            Entry<filters::StringConverter, filters::StringConverter::WordCountMode>("wordcount"),
            Entry<filters::StringConverter, filters::StringConverter::WordWrapMode>("wordwrap"),
            Entry<filters::StringConverter, filters::StringConverter::UnderscoreMode>("underscorize"),
            Entry<filters::XmlAttrFilter>("xmlattr"),
        };
        std::sort(result.begin(), result.end(), [](const FilterEntry& lhs, const FilterEntry& rhs) { return lhs.name < rhs.name; });
        return result;
    }();
    return filters;
}

const FilterEntry* FindBuiltinFilter(std::string_view filterName)
{
    const auto& filters = BuiltinFilters();
    auto p = std::lower_bound(filters.begin(), filters.end(), filterName, [](const FilterEntry& entry, std::string_view name) { return entry.name < name; });
    return p != filters.end() && p->name == filterName ? &*p : nullptr;
}
} // namespace

FilterPtr CreateFilter(std::string_view filterName, const CallParamsInfo& params)
{
    if (const auto* entry = FindBuiltinFilter(filterName))
    {
        return entry->create(params);
    }
    return std::make_shared<filters::UserDefinedFilter>(std::string(filterName), params);
}

NodeRef<ExpressionFilter::IExpressionFilter> CreateFilter(NodeArena& nodes, std::string_view filterName, const CallParamsInfo& params)
{
    if (const auto* entry = FindBuiltinFilter(filterName))
    {
        return entry->make(nodes, params);
    }
    return nodes.MakeObject<IExpressionFilter, filters::UserDefinedFilter>(std::string(filterName), params);
}

FilterPtr CreateFilter(const std::string& filterName, const CallParamsInfo& params, RenderContext& context)
{
    auto* env = context.GetEnv();
    auto registered = env ? env->FindFilter(filterName) : std::optional<UserCallable>();
    if (!registered)
    {
        return CreateFilter(filterName, params);
    }
    auto callable = visitors::InputValueConvertor::ConvertUserCallable(*registered);
    return std::make_shared<filters::UserDefinedFilter>(filterName, params, std::move(callable));
}

} // namespace jinja2
