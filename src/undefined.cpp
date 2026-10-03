#include "undefined.h"

#include "internal_value.h"
#include "markup.h"
#include "render_context.h"
#include "value_visitors.h"

#include <jinja2cpp/template_env.h>

#include <memory>
#include <string>
#include <utility>
#include <variant>

namespace jinja2
{

namespace
{
UndefinedPolicy PolicyOf(const RenderContext* context)
{
    if (!context)
    {
        return UndefinedPolicy::Default;
    }
    auto* callback = const_cast<RenderContext*>(context)->GetRendererCallback();
    return callback ? callback->GetSettings().undefinedPolicy : UndefinedPolicy::Default;
}

InternalValue MakeUndefinedValue(UndefinedInfo info)
{
    return InternalValue(UndefinedValue{ std::make_shared<const UndefinedInfo>(std::move(info)) });
}

std::string Repr(const InternalValue& val)
{
    std::string result;
    Apply<visitors::ValueRenderer<char>>(val, result, true);
    return result;
}
} // namespace

InternalValue MakeUndefined(const RenderContext& context, std::string name)
{
    UndefinedInfo info;
    info.policy = PolicyOf(&context);
    info.name = std::move(name);
    return MakeUndefinedValue(std::move(info));
}

InternalValue MakeUndefinedWithHint(const RenderContext& context, std::string hint)
{
    UndefinedInfo info;
    info.policy = PolicyOf(&context);
    info.hint = std::move(hint);
    return MakeUndefinedValue(std::move(info));
}

InternalValue MakeUndefined(const RenderContext* context, const InternalValue& obj, const InternalValue& key)
{
    if (GetUndefinedInfo(obj))
    {
        return obj;
    }
    // A key a host map has but reads as undefined (a JSON null or an empty reflected field,
    // task 0047) is not missing: it stays a plain undefined, which fails no use
    const auto* map = std::get_if<MapAdapter>(&obj.GetData());
    if (map != nullptr && IsStringValue(key) && map->HasValue(AsString(key)))
    {
        return InternalValue();
    }
    UndefinedInfo info;
    info.policy = PolicyOf(context);
    // Python looks up a string item as an attribute too, and names it so
    info.isAttr = IsStringValue(key);
    info.name = info.isAttr ? AsString(key) : Repr(key);
    info.objType = obj.IsNone() ? "None" : std::string(Apply<visitors::PythonTypeNameGetter>(obj)) + " object";
    return MakeUndefinedValue(std::move(info));
}

std::string UndefinedMessage(const UndefinedInfo& info)
{
    if (!info.hint.empty())
    {
        return info.hint;
    }
    if (info.objType.empty())
    {
        return "'" + info.name + "' is undefined";
    }
    if (info.isAttr)
    {
        return "'" + info.objType + "' has no attribute '" + info.name + "'";
    }
    return info.objType + " has no element " + info.name;
}

std::string DebugUndefinedText(const UndefinedInfo& info)
{
    if (!info.hint.empty())
    {
        return "{{ undefined value printed: " + info.hint + " }}";
    }
    if (info.objType.empty())
    {
        return "{{ " + info.name + " }}";
    }
    // Python shows the repr of the name for attributes too
    auto name = info.isAttr ? Repr(InternalValue(info.name)) : info.name;
    return "{{ no such element: " + info.objType + "[" + name + "] }}";
}

void ThrowUndefined(const UndefinedInfo& info)
{
    throw UndefinedError(UndefinedMessage(info));
}

} // namespace jinja2
