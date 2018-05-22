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
using FilterParams = CallParams;

extern FilterPtr CreateFilter(std::string filterName, CallParams params);

namespace filters
{
class FilterBase : public ExpressionFilter::IExpressionFilter
{
public:
protected:
    bool ParseParams(const std::initializer_list<ArgumentInfo>& argsInfo, const CallParams& params);
    InternalValue GetArgumentValue(std::string argName, RenderContext& context, InternalValue defVal = InternalValue());

protected:
    ParsedArguments m_args;
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
        ReplaceMode,
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
    enum Mode
    {
        PythonMode,
    };

    StringFormat(FilterParams params, Mode mode);

    InternalValue Filter(const InternalValue& baseVal, RenderContext& context);
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
        RoundMode,

    };

    ValueConverter(FilterParams params, Mode mode);

    InternalValue Filter(const InternalValue& baseVal, RenderContext& context);
};
} // filters
} // jinja2

#endif // FILTERS_H
