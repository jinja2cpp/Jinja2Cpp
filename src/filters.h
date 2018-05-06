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

template<typename F>
struct FilterFactory
{
    static FilterPtr Create(FilterParams params)
    {
        return std::make_shared<F>(std::move(params));
    }

    template<typename ... Args>
    static ExpressionFilter::FilterFactoryFn MakeCreator(Args&& ... args)
    {
        return [args...](FilterParams params) {return std::make_shared<F>(std::move(params), args...);};
    }
};

namespace filters
{
class FilterBase : public ExpressionFilter::IExpressionFilter
{
public:
protected:
    bool ParseParams(const std::initializer_list<ArgumentInfo>& argsInfo, const CallParams& params);

protected:
    ParsedArguments m_args;
};

class Attribute : public  FilterBase
{
public:
    Attribute(FilterParams params);

    Value Filter(const Value& baseVal, RenderContext& context);
};

class Default : public  FilterBase
{
public:
    Default(FilterParams params);

    Value Filter(const Value& baseVal, RenderContext& context);
};

class DictSort : public  FilterBase
{
public:
    DictSort(FilterParams params);

    Value Filter(const Value& baseVal, RenderContext& context);
};

class GroupBy : public FilterBase
{
public:
    GroupBy(FilterParams params);

    Value Filter(const Value& baseVal, RenderContext& context);
};

class Join : public FilterBase
{
public:
    Join(FilterParams params);

    Value Filter(const Value& baseVal, RenderContext& context);
};

class Map : public FilterBase
{
public:
    Map(FilterParams params);

    Value Filter(const Value& baseVal, RenderContext& context);
};

class PrettyPrint : public FilterBase
{
public:
    PrettyPrint(FilterParams params);

    Value Filter(const Value& baseVal, RenderContext& context);
};

class Random : public FilterBase
{
public:
    Random(FilterParams params);

    Value Filter(const Value& baseVal, RenderContext& context);
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

    Value Filter(const Value& baseVal, RenderContext& context);
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

    Value Filter(const Value& baseVal, RenderContext& context);
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

    Value Filter(const Value& baseVal, RenderContext& context);
};

class Sort : public  FilterBase
{
public:
    Sort(FilterParams params);

    Value Filter(const Value& baseVal, RenderContext& context);
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
    };

    StringConverter(FilterParams params, Mode mode);

    Value Filter(const Value& baseVal, RenderContext& context);
};

class StringFormat : public  FilterBase
{
public:
    enum Mode
    {
        PythonMode,
    };

    StringFormat(FilterParams params, Mode mode);

    Value Filter(const Value& baseVal, RenderContext& context);
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

    Value Filter(const Value& baseVal, RenderContext& context);
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

    Value Filter(const Value& baseVal, RenderContext& context);
};
} // filters
} // jinja2

#endif // FILTERS_H
