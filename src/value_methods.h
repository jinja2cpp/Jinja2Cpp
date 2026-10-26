#ifndef JINJA2CPP_SRC_VALUE_METHODS_H
#define JINJA2CPP_SRC_VALUE_METHODS_H

#include "internal_value.h"

#include <string>
#include <string_view>

namespace jinja2
{
inline namespace v2
{
struct CallParams;

// Python's builtin methods on template values: s.upper(), l.append(x), d.items(), ...
// (docs/tasks/0020). Jinja2 does not sandbox by default, so templates call them; only this
// whitelist is exposed, never methods of host objects.
namespace methods
{
using MethodFn = InternalValue (*)(const InternalValue& self, const CallParams& params, RenderContext& context);

struct MethodInfo
{
    std::string_view name;
    MethodFn invoke;
    // The method changes its receiver (append, pop, update, ...)
    bool isMutating;
};

// Whether any value kind has a method of this name; the parser asks once per `x.name`, so a
// name that is no method costs nothing at render time
bool IsMethodName(std::string_view name);
// Whether a dict has a method of this name (items, get, ...), for a parser that tells
// map attributes apart from the methods of other kinds (title of a string)
bool IsDictMethodName(std::string_view name);

// The method `name` of `self`, or null. A map finds dict methods only by its policy
// (MapAttrPolicy): for KeysFirst maps the caller checks the keys first.
const MethodInfo* FindMethod(const InternalValue& self, std::string_view name);

// x.name: Python's getattr first, then the item, as Jinja2's Environment.getattr
InternalValue GetAttr(const InternalValue& obj, const std::string& name, RenderContext* context);
// x[key]: the item first, then a method of that name, as Jinja2's Environment.getitem
InternalValue GetItem(const InternalValue& obj, const InternalValue& key, RenderContext* context);

// self.name as a callable value (s.upper is callable, {% set f = s.upper %})
InternalValue MakeBoundMethod(const InternalValue& self, const MethodInfo& method);

// A list or dict that a mutating method can change in place: the value itself when the
// template owns it, otherwise a shallow copy that the caller stores where the value came
// from. Anything else is returned unchanged.
InternalValue MakeMutable(const InternalValue& value);
bool IsMutable(const InternalValue& value);
// Gives a list or dict of the host's data its cell in the running render, the way a write
// does (MakeMutable), so that every name of it reads the render's copy; false when it stays as
// it is (a tuple, a range, outside a render). True for a list or dict the template owns
bool PromoteBorrowed(const InternalValue& value);
// Whether a method of this name changes its receiver for some value kind
bool IsMutatingName(std::string_view name);
// Called before `item` is stored in `container`, a list or dict the template owns. A store
// that may close a reference cycle is noted for the render's CycleCollector, which frees
// the cycles when the render ends (docs/tasks/0110, 0251)
void NoteContainerStore(const InternalValue& container, const InternalValue& item);
// container[key] = value for a list or dict the template owns
void StoreItem(const InternalValue& container, const InternalValue& key, InternalValue value);
bool IsContainer(const InternalValue& value);

// Python's AttributeError text, "'str object' has no attribute 'x'" as Jinja2 words it
[[noreturn]] void ThrowNoAttribute(const InternalValue& obj, const std::string& name);

} // namespace methods
} // namespace v2
} // namespace jinja2

#endif // JINJA2CPP_SRC_VALUE_METHODS_H
