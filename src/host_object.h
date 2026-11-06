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
// GetHostObject), marked in the value (MapAdapter::MarkAsHost, MakeEngineHostValue) so that native
// maps pay no virtual call for the check; ValueStorage has no alternative of its own for it. The engine asks the object
// where Python asks the object:
// - printing: Str(), and Repr() inside a container; Html() under autoescape when IsMarkupLike()
// - x.name: GetAttr, then GetItem with the name, as Jinja2's Environment.getattr
// - x[key]: GetItem, then GetAttr for a string key, as Jinja2's Environment.getitem
// - x[start:stop:step]: GetSlice, which Jinja2 passes to __getitem__ without getitem's fallback
// - x(...): Call, when IsCallable()
// - iteration (for, list(), `in` without Contains): Iterate
// - reversed() (the last filter, then the reverse filter before it iterates): Reversed
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
class IEngineHostObject
{
public:
    IEngineHostObject() = default;
    IEngineHostObject(const IEngineHostObject&) = default;
    IEngineHostObject(IEngineHostObject&&) = default;
    IEngineHostObject& operator=(const IEngineHostObject&) = default;
    IEngineHostObject& operator=(IEngineHostObject&&) = default;
    virtual ~IEngineHostObject() = default;

    // type(x).__name__, never null, for error messages; it lives as long as the object
    [[nodiscard]] virtual const char* TypeName() const = 0;
    // str(x) and repr(x), UTF-8
    [[nodiscard]] virtual std::string Str() const = 0;
    [[nodiscard]] virtual std::string Repr() const = 0;
    // getattr(x, name); nullopt when there is no such attribute
    [[nodiscard]] virtual std::optional<InternalValue> GetAttr(const std::string& name) const = 0;
    // x[key]; nullopt when there is no such item or the object has no items
    [[nodiscard]] virtual std::optional<InternalValue> GetItem(const InternalValue& /*key*/) const { return std::nullopt; }
    // x[slice(start, stop, step)], where an omitted bound is an empty value (None); nullopt for
    // an object that has no items, which the engine reports as Python's TypeError. An object with
    // items that takes no slice throws its own error, as a mapping's KeyError
    [[nodiscard]] virtual std::optional<InternalValue> GetSlice(const InternalValue& /*start*/, const InternalValue& /*stop*/, const InternalValue& /*step*/) const
    {
        return std::nullopt;
    }
    // callable(x), and the call x(*params.posParams, **params.kwParams)
    [[nodiscard]] virtual bool IsCallable() const { return false; }
    virtual InternalValue Call(const CallParams& params, RenderContext& context) const;
    // Whether iter(x) works, asked without iterating; Iterate gives the items, as a list adapter
    // that may be single-pass (ListAdapter::CreateAdapter of a generator function)
    [[nodiscard]] virtual bool IsIterable() const { return false; }
    [[nodiscard]] virtual std::optional<ListAdapter> Iterate() const { return std::nullopt; }
    // reversed(x), the items last first; nullopt where reversed() raises TypeError: an object
    // with neither __reversed__ nor the sequence protocol (__len__ and __getitem__)
    [[nodiscard]] virtual std::optional<ListAdapter> Reversed() const { return std::nullopt; }
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
    // isinstance(x, collections.abc.Mapping): the engine reads the object as a map where Python
    // asks for a mapping (`is mapping`, dictsort, items, tojson, xmlattr): its keys are what
    // Iterate gives, each value GetItem of its key (HostObjectAccessor::GetEntries). Every other
    // operation still asks the object, as above
    [[nodiscard]] virtual bool IsMapping() const { return false; }
    // isinstance(x, (list, tuple)): the engine reads the object as a list of its own (the items
    // of Iterate, host::AsSequence) where Python's list or tuple has an operation the object's
    // own methods do not give the template: + * < <= > >= (and sort, min, max through them),
    // `str % x`, and tojson's JSON array. IsTuple marks a tuple (a namedtuple too)
    [[nodiscard]] virtual bool IsSequence() const { return false; }
    [[nodiscard]] virtual bool IsTuple() const { return false; }
    // Whether the object has __html__, and its result: the object prints as that Markup under
    // autoescape, and is `escaped`
    [[nodiscard]] virtual bool IsMarkupLike() const { return false; }
    [[nodiscard]] virtual std::string Html() const { return Str(); }
};

// The map accessor that carries a host object into the engine's values. Reading it as a map
// (GetItem, HasValue, FindItem) follows the attribute and item rules above. Only a mapping
// (IsMapping) has keys and entries to enumerate, and no object class; every object iterates
// through Iterate
class HostObjectAccessor : public IMapAccessor
{
public:
    // Every override is out of line: a body in sight lets GCC guess this class at each call
    // of an IMapAccessor virtual (speculative devirtualization), which costs the native maps
    // a compare per call
    [[nodiscard]] const IEngineHostObject* GetHostObject() const override = 0;

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

    [[nodiscard]] const IEngineHostObject* GetHostObject() const override { return &m_object; }

private:
    T m_object;
};

// A value of a new host object of type T, made from args
template<typename T, typename... Args>
InternalValue MakeEngineHostValue(Args&&... args)
{
    return InternalValue(MapAdapter(MakeRef<HostObjectHolder<T>>(std::forward<Args>(args)...)).MarkAsHost());
}

// The host object a value carries; null for every other value
inline const IEngineHostObject* GetHostObject(const InternalValue& value)
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
    return object != nullptr && object->IsMarkupLike();
}
// x(...) of a host object as a callable value of the engine, which keeps the object alive;
// throws Python's TypeError for an object that is not callable
Callable MakeCall(const InternalValue& object, const IEngineHostObject& host);
// x.__html__() as Markup of the template's character type
InternalValue Html(const IEngineHostObject& host, bool wideTarget);
// len(x), or Python's TypeError
int64_t Len(const IEngineHostObject& host);
// x[start:stop:step], or Python's TypeError for an object without items
InternalValue Slice(const IEngineHostObject& host, const InternalValue& start, const InternalValue& stop, const InternalValue& step);
// reversed(x), or Python's TypeError
ListAdapter Reversed(const IEngineHostObject& host);
// A list or tuple (IsSequence) as a list of the engine, marked as a tuple for a tuple; nullopt
// for every other object
std::optional<ListAdapter> AsSequence(const IEngineHostObject& host);
// x == other by Python's rules: left.__eq__, then right.__eq__, then identity. Null hosts
// stand for operands that are no host objects
bool Equal(const IEngineHostObject* left, const InternalValue& leftValue, const IEngineHostObject* right, const InternalValue& rightValue);

// The function of a jinja2::UserCallable made by EngineUserCallable
struct EngineCallableBody
{
    InternalValue callable;
    // A call through the public API, outside a template, which has no values of the engine to
    // pass: refused with std::logic_error
    Value operator()(const UserCallableParams& params) const;
};
// A jinja2::UserCallable for TemplateEnv::AddFilter, AddTest and Settings::finalize that the
// engine takes back as `callable` itself (InputValueConvertor::ConvertUserCallable): a
// Callable of kind UserCallable, called with the engine's values, so the arguments of a host's
// filter keep their identity (the Python module, docs/tasks/0353)
UserCallable EngineUserCallable(InternalValue callable);
} // namespace host

} // namespace v2
} // namespace jinja2

#endif // JINJA2CPP_SRC_HOST_OBJECT_H
