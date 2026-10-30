#ifndef JINJA2CPP_SRC_HOST_OBJECT_H
#define JINJA2CPP_SRC_HOST_OBJECT_H

#include "internal_value.h"

#include <jinja2cpp/value.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace jinja2
{
inline namespace v2
{
struct CallParams;
class RenderContext;

// An object of a host language the template reads as that language reads it: a Python object
// of the drop-in module (docs/tasks/0248, 0352). The module links the library statically and
// builds on this private interface, so the public headers do not change for it (0248 D3);
// 0357 moves the module to the public API before the G1 freeze.
//
// A host object is a map value whose accessor is a HostObjectAccessor (IMapAccessor::
// GetHostObject), marked in the value (MapAdapter::MarkAsHost, MakeHostValue) so that native
// maps pay no virtual call for the check; ValueStorage has no alternative of its own for it. The engine asks the object
// where Python asks the object:
// - printing: Str(), and Repr() inside a container; Html() under autoescape when IsMarkupLike()
// - x.name: GetAttr, then GetItem with the name, as Jinja2's Environment.getattr
// - x[key]: GetItem, then GetAttr for a string key, as Jinja2's Environment.getitem
// - x(...): Call, when IsCallable()
// - iteration (for, list(), `in` without Contains): Iterate
// - len(): Len; truth: Truth
// - ==, !=: Equals of the left operand, then of the right one, then identity
// - a dict key: Hash says whether it is hashable; the key is the object's Identity()
//   (ObjectKey in template_dict.cpp; plan 0248 section 7)
// - `is sameas`: Identity()
//
// No operation writes an item: template syntax cannot assign one (`set x.attr` takes
// namespace() only, as in Jinja2), and a write the template asks for reaches the host through
// the object's own method, `{% do d.update(...) %}`, which is GetAttr and Call.
//
// The implementation reports a host error by throwing; the render reports it as any other
// exception. The object is immutable to the engine, which may call it from any render that
// holds it (docs/tasks/0049: host data stays immutable across renders)
class IHostObject
{
public:
    IHostObject() = default;
    IHostObject(const IHostObject&) = default;
    IHostObject(IHostObject&&) = default;
    IHostObject& operator=(const IHostObject&) = default;
    IHostObject& operator=(IHostObject&&) = default;
    virtual ~IHostObject() = default;

    // type(x).__name__, never null, for error messages; it lives as long as the object
    [[nodiscard]] virtual const char* TypeName() const = 0;
    // str(x) and repr(x), UTF-8
    [[nodiscard]] virtual std::string Str() const = 0;
    [[nodiscard]] virtual std::string Repr() const = 0;
    // getattr(x, name); nullopt when there is no such attribute
    [[nodiscard]] virtual std::optional<InternalValue> GetAttr(const std::string& name) const = 0;
    // x[key]; nullopt when there is no such item or the object has no items
    [[nodiscard]] virtual std::optional<InternalValue> GetItem(const InternalValue& /*key*/) const { return std::nullopt; }
    // callable(x), and the call x(*params.posParams, **params.kwParams)
    [[nodiscard]] virtual bool IsCallable() const { return false; }
    virtual InternalValue Call(const CallParams& params, RenderContext& context) const;
    // Whether iter(x) works, asked without iterating; Iterate gives the items, as a list adapter
    // that may be single-pass (ListAdapter::CreateAdapter of a generator function)
    [[nodiscard]] virtual bool IsIterable() const { return false; }
    [[nodiscard]] virtual std::optional<ListAdapter> Iterate() const { return std::nullopt; }
    // item in x, as __contains__; nullopt makes the engine iterate, as Python does without it
    [[nodiscard]] virtual std::optional<bool> Contains(const InternalValue& /*item*/) const { return std::nullopt; }
    // len(x); nullopt for an object without a length
    [[nodiscard]] virtual std::optional<size_t> Len() const { return std::nullopt; }
    // bool(x): by default, as Python without __bool__, false only for a length of zero
    [[nodiscard]] virtual bool Truth() const;
    // x == other; nullopt is NotImplemented, and the engine asks the other operand, then
    // compares identities
    [[nodiscard]] virtual std::optional<bool> Equals(const InternalValue& /*other*/) const { return std::nullopt; }
    // hash(x); nullopt for an unhashable object (__hash__ = None). A dict the template makes
    // keys the object by its identity (see above), so only hashability matters to the engine
    [[nodiscard]] virtual std::optional<size_t> Hash() const;
    // The object behind this view: the same for two views of one host object (a PyObject*)
    [[nodiscard]] virtual const void* Identity() const { return this; }
    // Whether the object has __html__, and its result: the object prints as that Markup under
    // autoescape, and is `escaped`
    [[nodiscard]] virtual bool IsMarkupLike() const { return false; }
    [[nodiscard]] virtual std::string Html() const { return Str(); }
};

// The map accessor that carries a host object into the engine's values. Reading it as a map
// (GetItem, HasValue, FindItem) follows the attribute and item rules above; it has no keys of
// its own to enumerate, so a mapping is iterated through Iterate
class HostObjectAccessor : public IMapAccessor
{
public:
    // Every override is out of line: a body in sight lets GCC guess this class at each call
    // of an IMapAccessor virtual (speculative devirtualization), which costs the native maps
    // a compare per call
    [[nodiscard]] const IHostObject* GetHostObject() const override = 0;

    [[nodiscard]] size_t GetSize() const override;
    [[nodiscard]] bool HasValue(const std::string& name) const override;
    [[nodiscard]] InternalValue GetItem(const std::string& name) const override;
    [[nodiscard]] std::vector<std::string> GetKeys() const override;
    [[nodiscard]] std::vector<KeyValuePair> GetEntries() const override;
    [[nodiscard]] InternalValueList GetKeyValues() const override;
    [[nodiscard]] std::optional<InternalValue> FindItem(const InternalValue& key) const override;
    [[nodiscard]] GenericMap CreateGenericMap() const override;
    [[nodiscard]] bool ShouldExtendLifetime() const override;
    [[nodiscard]] bool HasAttributes() const override;
    [[nodiscard]] const void* GetIdentity() const override;
    [[nodiscard]] MapAttrPolicy GetAttrPolicy() const final;
    [[nodiscard]] const char* GetObjectClass() const final;
    [[nodiscard]] bool Iterates() const override;
    bool GetIteration(ListAdapter& items) const override;
};

// The accessor that owns a host object of type T, in one allocation with it
template<typename T>
class HostObjectHolder final : public HostObjectAccessor
{
public:
    template<typename... Args>
    explicit HostObjectHolder(Args&&... args)
        : m_object(std::forward<Args>(args)...)
    {
    }

    [[nodiscard]] const IHostObject* GetHostObject() const override { return &m_object; }

private:
    T m_object;
};

// A value of a new host object of type T, made from args
template<typename T, typename... Args>
InternalValue MakeHostValue(Args&&... args)
{
    return InternalValue(MapAdapter(MakeRef<HostObjectHolder<T>>(std::forward<Args>(args)...)).MarkAsHost());
}

// The host object a value carries; null for every other value
inline const IHostObject* GetHostObject(const InternalValue& value)
{
    const auto map = AsMap(value);
    return map ? map->GetHostObject() : nullptr;
}

// The Python operations of a host object the engine reaches outside the accessor
namespace host
{
// hasattr(x, '__html__') of a host object, for the places that ask before converting it with str()
inline bool IsMarkupLike(const InternalValue& value)
{
    const auto* object = GetHostObject(value);
    return object && object->IsMarkupLike();
}
// x(...) of a host object as a callable value of the engine, which keeps the object alive;
// throws Python's TypeError for an object that is not callable
Callable MakeCall(const InternalValue& object, const IHostObject& host);
// x.__html__() as Markup of the template's character type
InternalValue Html(const IHostObject& host, bool wideTarget);
// len(x), or Python's TypeError
int64_t Len(const IHostObject& host);
// x == other by Python's rules: left.__eq__, then right.__eq__, then identity. Null hosts
// stand for operands that are no host objects
bool Equal(const IHostObject* left, const InternalValue& leftValue, const IHostObject* right, const InternalValue& rightValue);
} // namespace host

} // namespace v2
} // namespace jinja2

#endif // JINJA2CPP_SRC_HOST_OBJECT_H
