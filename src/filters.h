#ifndef FILTERS_H
#define FILTERS_H

#include "expression_evaluator.h"
#include "function_base.h"
#include "jinja2cpp/value.h"
#include "render_context.h"

#include <memory>
#include <functional>

namespace jinja2
{
using FilterPtr = std::shared_ptr<ExpressionFilter::IExpressionFilter>;
using FilterParams = CallParams;

extern FilterPtr CreateFilter(std::string filterName, CallParams params);

namespace filters
{
class FilterBase : public FunctionBase, public ExpressionFilter::IExpressionFilter
{
};

class ApplyMacro : public FilterBase
{
public:
    ApplyMacro(FilterParams params);

    InternalValue Filter(const InternalValue& baseVal, RenderContext& context);

private:
    FilterParams m_mappingParams;
};

class Attribute : public  FilterBase
{
public:
    Attribute(FilterParams params);

    InternalValue Filter(const InternalValue& baseVal, RenderContext& context);
};

class Default : public  FilterBase
{
public:
    Default(FilterParams params);

    InternalValue Filter(const InternalValue& baseVal, RenderContext& context);
};

class DictSort : public  FilterBase
{
public:
    DictSort(FilterParams params);

    InternalValue Filter(const InternalValue& baseVal, RenderContext& context);
};

class GroupBy : public FilterBase
{
public:
    GroupBy(FilterParams params);

    InternalValue Filter(const InternalValue& baseVal, RenderContext& context);
};

class Join : public FilterBase
{
public:
    Join(FilterParams params);

    InternalValue Filter(const InternalValue& baseVal, RenderContext& context);
};

class Map : public FilterBase
{
public:
    Map(FilterParams params);

    InternalValue Filter(const InternalValue& baseVal, RenderContext& context);

private:
    static FilterParams MakeParams(FilterParams);

    FilterParams m_mappingParams;
};

class PrettyPrint : public FilterBase
{
public:
    PrettyPrint(FilterParams params);

    InternalValue Filter(const InternalValue& baseVal, RenderContext& context);
};

class Random : public FilterBase
{
public:
    Random(FilterParams params);

    InternalValue Filter(const InternalValue& baseVal, RenderContext& context);
};

class SequenceAccessor : public  FilterBase
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

    SequenceAccessor(FilterParams params, Mode mode);

    InternalValue Filter(const InternalValue& baseVal, RenderContext& context);

private:
    Mode m_mode;
};

class Serialize : public  FilterBase
{
public:
    enum Mode
    {
        JsonMode,
        XmlMode,
        YamlMode
    };

    Serialize(FilterParams params, Mode mode);

    InternalValue Filter(const InternalValue& baseVal, RenderContext& context);

private:
    Mode m_mode;
};

class Slice : public  FilterBase
{
public:
    enum Mode
    {
        BatchMode,
        SliceMode,
    };

    Slice(FilterParams params, Mode mode);

    InternalValue Filter(const InternalValue& baseVal, RenderContext& context);
private:
    InternalValue Batch(const InternalValue& baseVal, RenderContext& context);

    Mode m_mode;
};

class Sort : public  FilterBase
{
public:
    Sort(FilterParams params);

    InternalValue Filter(const InternalValue& baseVal, RenderContext& context);
};

class StringConverter : public  FilterBase
{
public:
    enum Mode
    {
        CapitalMode,
        CamelMode,
        EscapeCppMode,
        EscapeHtmlMode,
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
        UrlEncodeMode
    };

    StringConverter(FilterParams params, Mode mode);

    InternalValue Filter(const InternalValue& baseVal, RenderContext& context);

private:
    Mode m_mode;
};

class StringFormat : public  FilterBase
{
public:
    StringFormat(FilterParams params);

    InternalValue Filter(const InternalValue& baseVal, RenderContext& context);
private:
    CallParams m_params;
};

class Tester : public  FilterBase
{
public:
    enum Mode
    {
        RejectMode,
        RejectAttrMode,
        SelectMode,
        SelectAttrMode,
    };

    Tester(FilterParams params, Mode mode);

    InternalValue Filter(const InternalValue& baseVal, RenderContext& context);

private:
    Mode m_mode;
    CallParams m_testingParams;
    bool m_noParams = false;
};

class ValueConverter : public  FilterBase
{
public:
    enum Mode
    {
        ToFloatMode,
        ToIntMode,
        ToListMode,
        AbsMode,
        RoundMode,
    };

    ValueConverter(FilterParams params, Mode mode);

    InternalValue Filter(const InternalValue& baseVal, RenderContext& context);

private:
    Mode m_mode;
};

class UserDefinedFilter : public FilterBase
{
public:
    UserDefinedFilter(std::string filterName, FilterParams params);

    InternalValue Filter(const InternalValue& baseVal, RenderContext& context);

private:
    std::string m_filterName;
    FilterParams m_callParams;
};

} // filters
} // jinja2

#endif // FILTERS_H
