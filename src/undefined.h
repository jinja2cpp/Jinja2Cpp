#ifndef JINJA2CPP_SRC_UNDEFINED_H
#define JINJA2CPP_SRC_UNDEFINED_H

#include "internal_value.h"

#include <jinja2cpp/template_env.h>

#include <exception>
#include <string>

namespace jinja2
{

// What a named lookup missed, and the policy that decides how the undefined result may be
// used. It travels with the value, as the class of a Python Undefined does: a variable set
// from a missing name fails where it is used, not where it is set.
struct UndefinedInfo
{
    UndefinedPolicy policy = UndefinedPolicy::Default;
    // The variable or attribute name; for a non-string item key, its repr
    std::string name;
    // Python's object_type_repr of the object the attribute or item is missing from ("dict
    // object", "None"); empty for a missing variable
    std::string objType;
    // A string key, reported as an attribute
    bool isAttr = true;
    // Replaces the message built from the fields above when set
    std::string hint;
};

// Python's UndefinedError. Template rendering reports it as ErrorCode::UndefinedError
class UndefinedError : public std::exception
{
public:
    explicit UndefinedError(std::string message)
        : m_message(std::move(message))
    {
    }
    const char* what() const noexcept override { return m_message.c_str(); }

private:
    std::string m_message;
};

// The ways a template uses a value that an undefined value may refuse
enum class UndefinedUse
{
    Attribute,  // an attribute or item of it; Chainable allows it
    Call,       // calling it; every policy refuses
    Arithmetic, // unary - and +, int(), float(); every policy refuses (binary arithmetic fails on its own)
    Operator,   // ==, !=, ~, in, `and`, `or`; only Strict refuses
};

// The undefined result of looking up the missing variable name
InternalValue MakeUndefined(const RenderContext& context, std::string name);
// An undefined value whose message is hint, as Python's environment.undefined(hint)
InternalValue MakeUndefinedWithHint(const RenderContext& context, std::string hint);
// The undefined result of the missing attribute or item key of obj. A value that is
// already undefined with info is returned as is
InternalValue MakeUndefined(const RenderContext* context, const InternalValue& obj, const InternalValue& key);

inline const UndefinedInfo* GetUndefinedInfo(const InternalValue& val)
{
    const auto* undef = std::get_if<UndefinedValue>(&val.GetData());
    return undef != nullptr ? undef->info.get() : nullptr;
}

// Python's message for the missing name, as UndefinedError reports it
std::string UndefinedMessage(const UndefinedInfo& info);
// What DebugUndefined prints: {{ name }}, {{ no such element: dict object['key'] }} or
// {{ undefined value printed: hint }}
std::string DebugUndefinedText(const UndefinedInfo& info);
[[noreturn]] void ThrowUndefined(const UndefinedInfo& info);

// Throws UndefinedError when val is an undefined value with info whose policy refuses use
inline void CheckUndefinedUse(const InternalValue& val, UndefinedUse use)
{
    const auto* info = GetUndefinedInfo(val);
    if (info == nullptr)
        return;
    if (info->policy == UndefinedPolicy::Strict || use == UndefinedUse::Call || use == UndefinedUse::Arithmetic || (use == UndefinedUse::Attribute && info->policy != UndefinedPolicy::Chainable))
        ThrowUndefined(*info);
}

// For str(), bool(), len() and iteration, which only StrictUndefined refuses
inline void CheckStrictUndefined(const UndefinedValue& val)
{
    if (val.info && val.info->policy == UndefinedPolicy::Strict)
        ThrowUndefined(*val.info);
}

} // namespace jinja2

#endif // JINJA2CPP_SRC_UNDEFINED_H
