#ifndef JINJA2CPP_SRC_FILTERS_H
#define JINJA2CPP_SRC_FILTERS_H

#include "expression_evaluator.h"
#include "function_base.h"
#include "internal_value.h"
#include "node_arena.h"
#include "render_context.h"

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

namespace jinja2
{
using FilterPtr = std::shared_ptr<ExpressionFilter::IExpressionFilter>;
using FilterParams = CallParamsInfo;

extern FilterPtr CreateFilter(std::string_view filterName, const CallParamsInfo& params);
// The same in the arena of the template that names the filter
extern NodeRef<ExpressionFilter::IExpressionFilter> CreateFilter(NodeArena& nodes, std::string_view filterName, const CallParamsInfo& params);
// For filters named at render time (`map('name')`): a filter added to the environment of the
// template comes first, as in Jinja2's env.filters
extern FilterPtr CreateFilter(const std::string& filterName, const CallParamsInfo& params, RenderContext& context);

namespace filters
{
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
        VisitCallParams(refs, m_mappingParams);
    }
private:
    FilterParams m_mappingParams;
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
        VisitCallParams(refs, m_mappingParams);
    }
private:
    FilterParams m_mappingParams;
    // map(attribute=...) looks items up like getattr with a fallback to [], not like attr
    bool m_byAttribute = false;
};

class PrettyPrint : public FilterBase
{
public:
    explicit PrettyPrint(const FilterParams& params);

    InternalValue Filter(const InternalValue& baseVal, RenderContext& context) override;
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
        XmlMode,
        YamlMode
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
    TargetString ApplyTruncate(const InternalValue& baseVal, RenderContext& context);
    TargetString ApplyIndent(const InternalValue& baseVal, RenderContext& context);
    TargetString ApplyUrlize(const InternalValue& baseVal, RenderContext& context);
    TargetString ApplyCenter(const InternalValue& baseVal, RenderContext& context);
    TargetString ApplyWordWrap(const InternalValue& baseVal, RenderContext& context);
    [[nodiscard]] TargetString MapChars(const InternalValue& baseVal) const;
    static int64_t WordCount(const InternalValue& baseVal);
    bool ReturnsMarkup(const InternalValue& baseVal, RenderContext& context) const;
    TargetString Convert(const InternalValue& baseVal, RenderContext& context);

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
        VisitCallParams(refs, m_params);
    }
    // A literal printf-style format is parsed here, once
    void SetConstantBase(const InternalValue& base) override;

private:
    FilterParams m_params;
    // The literal format, parsed; null unless it is a narrow string with a '%'
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
        VisitCallParams(refs, m_testingParams);
    }
private:
    Mode m_mode;
    FilterParams m_testingParams;
    bool m_noParams = false;
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
        VisitCallParams(refs, m_callParams);
    }

private:
    std::string m_filterName;
    FilterParams m_callParams;
    InternalValue m_callable;
};

} // namespace filters
} // namespace jinja2

#endif // JINJA2CPP_SRC_FILTERS_H
