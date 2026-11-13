#ifndef JINJA2CPP_SRC_UNDEFINED_H
#define JINJA2CPP_SRC_UNDEFINED_H

#include "internal_value.h"

#include <jinja2cpp/template_env.h>

#include <exception>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

namespace jinja2
{
inline namespace v2
{

// What a named lookup missed, and the policy that decides how the undefined result may be
// used. It travels with the value, as the class of a Python Undefined does: a variable set
// from a missing name fails where it is used, not where it is set. Shared by the values
// that carry it.
struct UndefinedInfo final : ValueObject
{
    UndefinedPolicy policy = UndefinedPolicy::Default;
    // The variable or attribute name; for a non-string item key, its repr
    std::string name;
    // Python's object_type_repr of the object the attribute or item is missing from ("dict
    // object", "None"); empty for a missing variable
    std::string objType;
    // A string key, reported as an attribute
    bool isAttr = true;
    // The sandbox refused the attribute (Jinja2's unsafe_undefined, exc=SecurityError): a use that fails
    // reports ErrorCode::SecurityError
    bool security = false;
    // Not 0: the line of the inline if-expression without else this is the value of, whose
    // message replaces the one built from the other fields (MakeCondExprUndefined). It sits in
    // the padding after the flags, so the info is no larger
    unsigned condExprLine = 0;
    // Replaces the message built from the fields above when set
    std::string hint;
};

// The ways a template uses a value that an undefined value may refuse
enum class UndefinedUse
{
    Attribute,  // an attribute or item of it; Chainable allows it
    Call,       // calling it; every policy refuses
    Arithmetic, // unary - and +, int(), float(); every policy refuses (binary arithmetic fails on its own)
    Operator,   // ==, !=, ~, in, `and`, `or`; only Strict refuses
    Method,     // what Undefined has no method for: dictsort's items(), indent's +=, wordwrap; every policy refuses
};

// What the sandbox policy of the render (IEngineSandboxPolicy) gives for the attribute `name` that the value `obj` of the engine has
// none of: the unsafe undefined of an attribute Python's object has and the sandbox refuses, or a method the engine
// lacks; nothing without a sandbox, and for a host object, which the host reads itself
std::optional<InternalValue> SandboxedNativeAttribute(const RenderContext* context, const InternalValue& obj, const std::string& name);

// The undefined result of looking up the missing variable name
InternalValue MakeUndefined(const RenderContext& context, std::string name);
// An undefined value whose message is hint, as Python's environment.undefined(hint)
InternalValue MakeUndefinedWithHint(const RenderContext& context, std::string hint);
// Jinja2's cond_expr_undefined for an inline if-expression on `line` without else: an undefined
// of the default policy whatever the policy of the render (jinja2.Undefined in every
// environment); its message is formatted only when it is reported
InternalValue MakeCondExprUndefined(unsigned line);
// The undefined result of the missing attribute or item key of obj. A value that is
// already undefined with info is returned as is
InternalValue MakeUndefined(const RenderContext* context, const InternalValue& obj, const InternalValue& key);
// The undefined result of the missing attribute name of obj, whether or not obj has an item of
// that name (getattr, the attr filter)
InternalValue MakeUndefinedAttr(const RenderContext* context, const InternalValue& obj, std::string name);
// The undefined result of the item of obj whose repr is keyRepr, a key the engine has no value
// for (a slice: Jinja2 folds a slice of literals through getitem, docs/tasks/0362)
InternalValue MakeUndefinedItem(const RenderContext* context, const InternalValue& obj, std::string keyRepr);

inline const UndefinedInfo* GetUndefinedInfo(const InternalValue& val)
{
    const auto* undef = GetIf<UndefinedValue>(&val);
    return undef ? undef->info : nullptr;
}

// Python's message for the missing name, as UndefinedError reports it
std::string UndefinedMessage(const UndefinedInfo& info);
// What DebugUndefined prints: {{ name }}, {{ no such element: dict object['key'] }} or
// {{ undefined value printed: hint }}
std::string DebugUndefinedText(const UndefinedInfo& info);
[[noreturn]] void ThrowUndefined(const UndefinedInfo& info);
// The undefined value of an attribute the sandbox refuses: its use fails with ErrorCode::SecurityError
InternalValue MakeUnsafeUndefined(const RenderContext& context, std::string hint);
// The Python class of the undefined value, as a TypeError names it: Undefined, StrictUndefined,
// ChainableUndefined or DebugUndefined
const char* UndefinedClassName(const UndefinedInfo& info);

// The attribute `name` of an undefined value that Jinja2's Undefined has as a field, whatever its policy:
// `_undefined_name` (the first missing step of a chain) and `_undefined_hint`, None when unset; nullopt for any
// other name
std::optional<InternalValue> UndefinedField(const UndefinedInfo& info, std::string_view name);
class RenderContext;
// x.name (attrName set) or x[key] of an undefined x: outside a sandbox the field UndefinedField gives, else x itself
// once its policy allows an attribute of it. Out of line, so that the lookup of every value stays small
InternalValue UndefinedAttribute(const InternalValue& undefined, const UndefinedInfo& info, const std::string* attrName, const RenderContext& context);

// Throws UndefinedError when val is an undefined value with info whose policy refuses use
inline void CheckUndefinedUse(const InternalValue& val, UndefinedUse use)
{
    const auto* info = GetUndefinedInfo(val);
    if (!info)
    {
        return;
    }
    if (info->policy == UndefinedPolicy::Strict || use == UndefinedUse::Call || use == UndefinedUse::Arithmetic || use == UndefinedUse::Method || (use == UndefinedUse::Attribute && info->policy != UndefinedPolicy::Chainable))
    {
        ThrowUndefined(*info);
    }
}

// Python's == of two items of a list, tuple or dict, and `in`, first takes an item as equal to
// itself (identity): a copy of one undefined value is the same object, a new lookup is not.
// Otherwise a StrictUndefined item refuses == (docs/tasks/0484)
inline bool CheckItemsCompare(const InternalValue& left, const InternalValue& right)
{
    const auto* leftInfo = GetUndefinedInfo(left);
    const auto* rightInfo = GetUndefinedInfo(right);
    if (leftInfo && leftInfo == rightInfo)
    {
        return true;
    }
    CheckUndefinedUse(left, UndefinedUse::Operator);
    CheckUndefinedUse(right, UndefinedUse::Operator);
    return false;
}

// For str(), bool(), len() and iteration, which only StrictUndefined refuses
inline void CheckStrictUndefined(const UndefinedValue& val)
{
    if (val.info && val.info->policy == UndefinedPolicy::Strict)
    {
        ThrowUndefined(*val.info);
    }
}

} // namespace v2
} // namespace jinja2

#endif // JINJA2CPP_SRC_UNDEFINED_H
