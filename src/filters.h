#ifndef JINJA2CPP_SRC_FILTERS_H
#define JINJA2CPP_SRC_FILTERS_H

#include "dialect.h"
#include "expression_evaluator.h"
#include "function_base.h"
#include "internal_value.h"
#include "node_arena.h"
#include "render_context.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace jinja2
{
inline namespace v2
{
using FilterPtr = std::shared_ptr<ExpressionFilter::IExpressionFilter>;
using FilterParams = FunctionArgs;

extern FilterPtr CreateFilter(std::string_view filterName, FunctionArgs params, detail::Dialect dialect = detail::Dialect::Jinja2);
// The same in the arena of the template that names the filter
// A name that is no built-in filter makes a UserDefinedFilter and sets `*unknown`
extern NodeRef<ExpressionFilter::IExpressionFilter> CreateFilter(NodeArena& nodes, std::string_view filterName, FunctionArgs params, bool* unknown = nullptr);
// For filters named at render time (`map('name')`): a filter added to the environment of the
// template comes first, as in Jinja2's env.filters
extern FilterPtr CreateFilter(const std::string& filterName, FunctionArgs params, RenderContext& context);
// For a call that unpacks `*args` or `**kwargs` (docs/tasks/0306): the filter is made from the
// values each time the call runs. registered and unknown as for ExpressionFilter::Make
extern NodeRef<ExpressionFilter::IExpressionFilter> CreateUnpackingFilter(NodeArena& nodes, const std::string& filterName, const CallArgsInfo& params, InternalValue registered, bool* unknown);

namespace filters
{
// The ASCII text Python's int() and float() parse for a string value of either width, or
// nullopt for a value that is no string or has characters they reject
std::optional<std::string> PythonNumberText(const InternalValue& val);
// Python's int() of such text in base 10 (a big integer beyond int64), or nullopt where it raises
std::optional<InternalValue> PythonIntOfText(const std::string& text);
// Python's float() of such text, or nullopt where it raises
std::optional<double> PythonFloatOfText(const std::string& text);

// The interface first, so that the object starts with it
class FilterBase : public ExpressionFilter::IExpressionFilter
    , public FunctionBase
{
public:
    std::string GetArgumentsError() const override { return FunctionBase::GetArgumentsError(); }
    void VisitRefs(detail::RefChecker& refs) const override { FunctionBase::VisitRefs(refs); }
};

class ApplyMacro : public FilterBase
{
public:
    explicit ApplyMacro(const FilterParams& params);

    InternalValue Filter(const InternalValue& baseVal, RenderContext& context) override;
    void VisitRefs(detail::RefChecker& refs) const override
    {
        FilterBase::VisitRefs(refs);
        m_mappingParams.VisitRefs(refs);
    }
private:
    ForwardedArgs m_mappingParams;
};

class Attribute : public FilterBase
{
public:
    explicit Attribute(const FilterParams& params);

    InternalValue Filter(const InternalValue& baseVal, RenderContext& context) override;
};

class Default : public FilterBase
{
public:
    explicit Default(const FilterParams& params);

    InternalValue Filter(const InternalValue& baseVal, RenderContext& context) override;
};

class DictSort : public FilterBase
{
public:
    explicit DictSort(const FilterParams& params);

    InternalValue Filter(const InternalValue& baseVal, RenderContext& context) override;
};

class GroupBy : public FilterBase
{
public:
    explicit GroupBy(const FilterParams& params);

    InternalValue Filter(const InternalValue& baseVal, RenderContext& context) override;
};

class Join : public FilterBase
{
public:
    explicit Join(const FilterParams& params);

    InternalValue Filter(const InternalValue& baseVal, RenderContext& context) override;
};

class Map : public FilterBase
{
public:
    explicit Map(const FilterParams& params);

    InternalValue Filter(const InternalValue& baseVal, RenderContext& context) override;
    void VisitRefs(detail::RefChecker& refs) const override
    {
        FilterBase::VisitRefs(refs);
        m_mappingParams.VisitRefs(refs);
    }
private:
    ForwardedArgs m_mappingParams;
    // map(attribute=...) looks items up like getattr with a fallback to [], not like attr
    bool m_byAttribute = false;
};

class PrettyPrint : public FilterBase
{
public:
    explicit PrettyPrint(const FilterParams& params);

    InternalValue Filter(const InternalValue& baseVal, RenderContext& context) override;
};

// neutralize and fence (docs/tasks/0358): the filters that protect a prompt from the text put into it
class PromptFilter : public FilterBase
{
public:
    enum Mode
    {
        NeutralizeMode,
        FenceMode
    };

    PromptFilter(const FilterParams& params, Mode mode);
    PromptFilter(PromptFilter&&) noexcept;
    PromptFilter(const PromptFilter&) = delete;
    PromptFilter& operator=(const PromptFilter&) = delete;
    PromptFilter& operator=(PromptFilter&&) = delete;
    ~PromptFilter() override;

    InternalValue Filter(const InternalValue& baseVal, RenderContext& context) override;

private:
    struct Cache;
    InternalValue Neutralize(const InternalValue& baseVal, RenderContext& context);
    InternalValue Fence(const InternalValue& baseVal, RenderContext& context);

    Mode m_mode;
    // The escaper built from the last arguments of neutralize that replace the environment's (a call from several
    // threads shares it)
    std::unique_ptr<Cache> m_cache;
};

class Random : public FilterBase
{
public:
    explicit Random(const FilterParams& params);

    InternalValue Filter(const InternalValue& baseVal, RenderContext& context) override;
};

class SequenceAccessor : public FilterBase
{
public:
    enum Mode
    {
        FirstItemMode,
        LastItemMode,
        LengthMode,
        MaxItemMode,
        MinItemMode,
        RandomMode,
        ReverseMode,
        SumItemsMode,
        UniqueItemsMode,
    };

    SequenceAccessor(const FilterParams& params, Mode mode);

    InternalValue Filter(const InternalValue& baseVal, RenderContext& context) override;
private:
    Mode m_mode;
};

class Serialize : public FilterBase
{
public:
    enum Mode
    {
        JsonMode,
        YamlMode,
        // transformers' tojson (Dialect::Transformers)
        TransformersJsonMode
    };

    Serialize(const FilterParams& params, Mode mode);

    InternalValue Filter(const InternalValue& value, RenderContext& context) override;
private:
    Mode m_mode;
};

class Slice : public FilterBase
{
public:
    enum Mode
    {
        BatchMode,
        SliceMode,
    };

    Slice(const FilterParams& params, Mode mode);

    InternalValue Filter(const InternalValue& baseVal, RenderContext& context) override;
private:
    InternalValue Batch(const InternalValue& baseVal, RenderContext& context);

    Mode m_mode;
};

class Sort : public FilterBase
{
public:
    explicit Sort(const FilterParams& params);

    InternalValue Filter(const InternalValue& baseVal, RenderContext& context) override;
};

class StringConverter : public FilterBase
{
public:
    enum Mode
    {
        CapitalMode,
        CamelMode,
        EscapeCppMode,
        EscapeHtmlMode,
        ForceEscapeMode,
        LowerMode,
        ReplaceMode,
        StriptagsMode,
        TitleMode,
        TrimMode,
        TruncateMode,
        UpperMode,
        WordCountMode,
        WordWrapMode,
        UnderscoreMode,
        UrlEncodeMode,
        CenterMode,
        IndentMode,
        SafeMode,
        ToStringMode,
        UrlizeMode
    };

    StringConverter(const FilterParams& params, Mode mode);

    InternalValue Filter(const InternalValue& baseVal, RenderContext& context) override;
private:
    static InternalValue ApplyUrlEncode(const InternalValue& baseVal, RenderContext& context);
    InternalValue ApplyReplace(const InternalValue& baseVal, RenderContext& context);
    InternalValue ApplyTruncate(const InternalValue& baseVal, RenderContext& context);
    InternalValue TruncateNonString(const InternalValue& baseVal, RenderContext& context);
    TargetString ApplyIndent(const InternalValue& baseVal, RenderContext& context);
    TargetString ApplyUrlize(const InternalValue& baseVal, RenderContext& context);
    TargetString ApplyCenter(const InternalValue& baseVal, RenderContext& context);
    TargetString ApplyWordWrap(const InternalValue& baseVal, RenderContext& context);
    [[nodiscard]] InternalValue MapChars(const InternalValue& baseVal) const;
    static int64_t WordCount(const InternalValue& baseVal);
    bool ReturnsMarkup(const InternalValue& baseVal, RenderContext& context) const;
    InternalValue Convert(const InternalValue& baseVal, RenderContext& context);

    Mode m_mode;
};

class StringFormat : public FilterBase
{
public:
    explicit StringFormat(const FilterParams& params);

    InternalValue Filter(const InternalValue& baseVal, RenderContext& context) override;
    void VisitRefs(detail::RefChecker& refs) const override
    {
        FilterBase::VisitRefs(refs);
        m_params.VisitRefs(refs);
    }
    // A literal printf-style format is parsed here, once
    void SetConstantBase(const InternalValue& base) override;

private:
    ForwardedArgs m_params;
    // The literal format, parsed; null unless it is a narrow string
    std::shared_ptr<const CompiledPercentFormat> m_constFormat;
};

class Tester : public FilterBase
{
public:
    enum Mode
    {
        RejectMode,
        RejectAttrMode,
        SelectMode,
        SelectAttrMode,
    };

    Tester(const FilterParams& params, Mode mode);

    InternalValue Filter(const InternalValue& baseVal, RenderContext& context) override;
    void VisitRefs(detail::RefChecker& refs) const override
    {
        FilterBase::VisitRefs(refs);
        m_testingParams.VisitRefs(refs);
    }
private:
    Mode m_mode;
    ForwardedArgs m_testingParams;
    bool m_hasAttribute = false;
};

class ValueConverter : public FilterBase
{
public:
    enum Mode
    {
        ToFloatMode,
        ToIntMode,
        ToListMode,
        AbsMode,
        RoundMode,
        FileSizeFormatMode,
        ItemsMode,
    };

    ValueConverter(const FilterParams& params, Mode mode);

    InternalValue Filter(const InternalValue& baseVal, RenderContext& context) override;
private:
    InternalValue FileSizeFormat(const InternalValue& baseVal, RenderContext& context);
    static InternalValue Items(const InternalValue& baseVal, RenderContext& context);
    InternalValue ToInt(const InternalValue& baseVal, RenderContext& context);
    InternalValue ToFloat(const InternalValue& baseVal, RenderContext& context);
    static InternalValue Abs(const InternalValue& baseVal, RenderContext& context);
    InternalValue Round(const InternalValue& baseVal, RenderContext& context);

    Mode m_mode;
};

class XmlAttrFilter : public FilterBase
{
public:
    explicit XmlAttrFilter(const FilterParams& params);

    InternalValue Filter(const InternalValue& baseVal, RenderContext& context) override;
};

class UserDefinedFilter : public FilterBase
{
public:
    // callable: the filter added to the environment under this name; without it the filter is looked up in the
    // render context, as a user callable passed in the parameters or the globals
    UserDefinedFilter(std::string filterName, const FilterParams& params, InternalValue callable = InternalValue());

    InternalValue Filter(const InternalValue& baseVal, RenderContext& context) override;
    void VisitRefs(detail::RefChecker& refs) const override
    {
        FilterBase::VisitRefs(refs);
        m_callParams.VisitRefs(refs);
    }

private:
    std::string m_filterName;
    ForwardedArgs m_callParams;
    InternalValue m_callable;
};

// A filter call that unpacks `*args` or `**kwargs`: Python binds it when it runs, so the filter
// is made then, from the values, and checks them as a call written out would be checked
class UnpackingFilter : public ExpressionFilter::IExpressionFilter
{
public:
    UnpackingFilter(std::string filterName, ArenaCallParams params, InternalValue registered)
        : m_filterName(std::move(filterName))
        , m_params(params)
        , m_registered(std::move(registered))
    {
    }

    InternalValue Filter(const InternalValue& baseVal, RenderContext& context) override;
    void VisitRefs(detail::RefChecker& refs) const override { m_params.VisitRefs(refs); }

private:
    std::string m_filterName;
    ArenaCallParams m_params;
    // The filter the environment adds under this name, if any
    InternalValue m_registered;
};

} // namespace filters
} // namespace v2
} // namespace jinja2

#endif // JINJA2CPP_SRC_FILTERS_H
