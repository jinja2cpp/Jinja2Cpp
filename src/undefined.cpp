#include "undefined.h"

#include "engine_errors.h"
#include "host_object.h"
#include "internal_value.h"
#include "markup.h"
#include "render_context.h"
#include "value_visitors.h"

#include <jinja2cpp/template_env.h>

#include <charconv>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <variant>

namespace jinja2
{
inline namespace v2
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
    const auto shared = MakeRef<const UndefinedInfo>(std::move(info));
    return InternalValue(UndefinedValue{ shared.get() });
}

std::string Repr(const InternalValue& val)
{
    std::string result;
    Apply<visitors::ValueRenderer<char>>(val, result, true);
    return result;
}

// Jinja2's object_type_repr: a builtin type by its name, an engine class with its module
// (jinja2.utils.Namespace, jinja2.runtime.LoopContext, jinja2.runtime.Macro), followed by
// "object". A host object is named by its own class
std::string ObjectTypeRepr(const InternalValue& obj)
{
    if (obj.IsNone())
    {
        return "None";
    }
    if (!GetHostObject(obj))
    {
        const auto map = AsMap(obj);
        if (const char* objectClass = map ? map->GetObjectClass() : nullptr)
        {
            const std::string_view name(objectClass);
            const char* module = name == "TemplateModule"                               ? "jinja2.environment."
                                 : name == "LoopContext" || name == "TemplateReference" ? "jinja2.runtime."
                                                                                        : "jinja2.utils.";
            return module + std::string(name) + " object";
        }
        const auto* callable = GetIf<Callable>(&obj);
        if (callable && callable->GetKind() == Callable::Macro)
        {
            return "jinja2.runtime.Macro object";
        }
    }
    return std::string(Apply<visitors::PythonTypeNameGetter>(obj)) + " object";
}
} // namespace

std::optional<InternalValue> SandboxedNativeAttribute(const RenderContext* context, const InternalValue& obj, const std::string& name)
{
    const auto* policy = context ? context->GetSandboxPolicy() : nullptr;
    if (!policy || GetHostObject(obj))
    {
        return std::nullopt;
    }
    return policy->NativeAttribute(obj, name);
}

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

InternalValue MakeCondExprUndefined(unsigned line)
{
    UndefinedInfo info;
    info.condExprLine = line;
    return MakeUndefinedValue(std::move(info));
}

namespace
{
std::string HintOf(const UndefinedInfo& info)
{
    if (info.condExprLine)
    {
        return "the inline if-expression on line " + std::to_string(info.condExprLine) + " evaluated to false and no else section was defined.";
    }
    return info.hint;
}
} // namespace

InternalValue MakeUndefined(const RenderContext* context, const InternalValue& obj, const InternalValue& key)
{
    if (GetUndefinedInfo(obj))
    {
        return obj;
    }
    if (context && context->GetSandboxPolicy() && IsStringValue(key))
    {
        if (auto replacement = SandboxedNativeAttribute(context, obj, AsString(key)))
        {
            return std::move(*replacement);
        }
    }
    // A key a host map has but reads as undefined (a JSON null or an empty reflected field,
    // task 0047) is not missing: it stays a plain undefined, which fails no use
    auto map = AsMap(obj);
    if (map && (IsStringValue(key) ? map->HasValue(AsString(key)) : map->FindItem(key).has_value()))
    {
        return InternalValue();
    }
    UndefinedInfo info;
    info.policy = PolicyOf(context);
    // Python looks up a string item as an attribute too, and names it so
    info.isAttr = IsStringValue(key);
    info.name = info.isAttr ? AsString(key) : Repr(key);
    info.objType = ObjectTypeRepr(obj);
    return MakeUndefinedValue(std::move(info));
}

InternalValue MakeUndefinedAttr(const RenderContext* context, const InternalValue& obj, std::string name)
{
    if (auto replacement = SandboxedNativeAttribute(context, obj, name))
    {
        return std::move(*replacement);
    }
    UndefinedInfo info;
    info.policy = PolicyOf(context);
    info.isAttr = true;
    info.name = std::move(name);
    info.objType = ObjectTypeRepr(obj);
    return MakeUndefinedValue(std::move(info));
}

InternalValue MakeUndefinedItem(const RenderContext* context, const InternalValue& obj, std::string keyRepr)
{
    UndefinedInfo info;
    info.policy = PolicyOf(context);
    info.isAttr = false;
    info.name = std::move(keyRepr);
    info.objType = ObjectTypeRepr(obj);
    return MakeUndefinedValue(std::move(info));
}

std::string UndefinedMessage(const UndefinedInfo& info)
{
    if (auto hint = HintOf(info); !hint.empty())
    {
        return hint;
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
    if (auto hint = HintOf(info); !hint.empty())
    {
        return "{{ undefined value printed: " + hint + " }}";
    }
    if (info.objType.empty())
    {
        return "{{ " + info.name + " }}";
    }
    // Python shows the repr of the name for attributes too
    auto name = info.isAttr ? Repr(InternalValue(info.name)) : info.name;
    return "{{ no such element: " + info.objType + "[" + name + "] }}";
}

InternalValue MakeUnsafeUndefined(const RenderContext& context, std::string hint)
{
    UndefinedInfo info;
    info.policy = PolicyOf(&context);
    info.hint = std::move(hint);
    info.security = true;
    return MakeUndefinedValue(std::move(info));
}

void ThrowUndefined(const UndefinedInfo& info)
{
    throw UndefinedError(UndefinedMessage(info), info.security);
}

std::optional<InternalValue> UndefinedField(const UndefinedInfo& info, std::string_view name)
{
    if (name == "_undefined_name")
    {
        if (info.name.empty())
        {
            return InternalValue(NoneValue{});
        }
        // A non-string key is kept as its repr: an int key is the int again
        int64_t number = 0;
        const auto* end = info.name.data() + info.name.size();
        if (!info.isAttr)
        {
            if (const auto parsed = std::from_chars(info.name.data(), end, number); parsed.ec == std::errc() && parsed.ptr == end)
            {
                return InternalValue(number);
            }
        }
        return InternalValue(info.name);
    }
    if (name == "_undefined_hint")
    {
        auto hint = HintOf(info);
        if (hint.empty())
        {
            return InternalValue(NoneValue{});
        }
        return InternalValue(hint);
    }
    return std::nullopt;
}

const char* UndefinedClassName(const UndefinedInfo& info)
{
    switch (info.policy)
    {
    case UndefinedPolicy::Strict:
        return "StrictUndefined";
    case UndefinedPolicy::Chainable:
        return "ChainableUndefined";
    case UndefinedPolicy::Debug:
        return "DebugUndefined";
    case UndefinedPolicy::Default:
        break;
    }
    return "Undefined";
}

} // namespace v2
} // namespace jinja2
