#ifndef JINJA2CPP_HOST_OBJECT_H
#define JINJA2CPP_HOST_OBJECT_H

#include <jinja2cpp/config.h>
#include <jinja2cpp/fwd.h> // IWYU pragma: export
#include <jinja2cpp/generic_list.h>
#include <jinja2cpp/value.h>

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

JINJA2CPP_SUPPRESS_DLL_INTERFACE_WARNINGS_BEGIN

namespace jinja2
{
inline namespace v2
{
/*!
 * \brief An object of a host language that templates read as that language reads it
 *
 * A host object is an opaque value: a Python object, a scripting-language object or a C++ object that keeps its own
 * identity and its own rules for attributes, items, printing and calls (docs/tasks/0357, B1). It is not a third
 * container type: \ref MakeHostValue gives a \ref Value holding a \ref GenericMap whose accessor carries the object,
 * and the engine recognises it through \ref IMapItemAccessor::AsHostObject where the value enters a render. Native
 * maps and lists never call into this interface.
 *
 * The template asks the object where Python asks the object:
 * - printing: \ref Str, and \ref Repr inside a container; \ref Html under autoescape when \ref IsMarkupLike
 * - `x.name`: \ref GetAttr, then \ref GetItem with the name (Jinja2's `Environment.getattr`)
 * - `x[key]`: \ref GetItem, then \ref GetAttr for a string key (Jinja2's `Environment.getitem`)
 * - `x[start:stop:step]`: \ref GetSlice, with no fallback
 * - `x(...)`: \ref Call, when \ref IsCallable
 * - iteration (`for`, `list`, `in` without \ref Contains): \ref Iterate
 * - `reversed`: \ref Reversed
 * - `len`: \ref Len; truth: \ref Truth
 * - `==`, `!=`: \ref Equals of the left operand, then of the right one, then \ref Identity
 * - a dict key: \ref Hash tells whether it is hashable; the key is the object's \ref Identity
 * - `is sameas`: \ref Identity
 * - `is mapping`, `dictsort`, `items`, `tojson`, `xmlattr`: a \ref IsMapping object is read as a map whose keys
 *   \ref Iterate gives and whose values \ref GetItem gives
 * - `+ * < <= > >=`, `str % x`, tojson arrays: a \ref IsSequence object is read as the list \ref Iterate gives,
 *   printed as a tuple when \ref IsTuple
 *
 * Values the engine passes in (a key, an operand, call arguments) hold the template's own values: another host object
 * arrives as itself (\ref GenericMap::AsHostObject), an undefined value as \ref MakeUndefined describes it (\ref
 * UndefinedOf), a template callable as a callable lent for the duration of the call.
 *
 * No operation writes: template syntax assigns attributes of `namespace()` only, and a write the template asks for
 * reaches the object through its own method (`{% do d.update(...) %}`, which is \ref GetAttr and \ref Call).
 *
 * An implementation reports an error by throwing; the render reports it as any other exception. The engine treats the
 * object as immutable and may call it from any render that holds it, on that render's thread.
 *
 * Growth: this interface is frozen at the 2.0 API freeze. Operations added in 2.x go to a derived `IHostObject2`,
 * which the engine looks for once, when the object enters a render (docs/api-2.0.md, section 7 item 10).
 */
class JINJA2CPP_EXPORT IHostObject
{
public:
    IHostObject() = default;
    IHostObject(const IHostObject&) = default;
    IHostObject(IHostObject&&) = default;
    IHostObject& operator=(const IHostObject&) = default;
    IHostObject& operator=(IHostObject&&) = default;
    virtual ~IHostObject();

    //! `type(x).__name__` for error messages: never null, null-terminated, valid as long as the object
    [[nodiscard]] virtual const char* TypeName() const = 0;
    //! `str(x)`, UTF-8
    [[nodiscard]] virtual std::string Str() const = 0;
    //! `repr(x)`, UTF-8: how the object prints inside a printed container
    [[nodiscard]] virtual std::string Repr() const = 0;
    //! `getattr(x, name)`; empty when there is no such attribute
    [[nodiscard]] virtual std::optional<Value> GetAttr(std::string_view name) const = 0;
    //! `x[key]`; empty when there is no such item or the object has no items
    [[nodiscard]] virtual std::optional<Value> GetItem(const Value& key) const;
    /*!
     * \brief `x[slice(start, stop, step)]`
     *
     * An omitted bound is `None`. Empty for an object without items, which the engine reports as Python's
     * `TypeError`; an object with items that takes no slice throws its own error.
     */
    [[nodiscard]] virtual std::optional<Value> GetSlice(const Value& start, const Value& stop, const Value& step) const;
    //! `callable(x)`
    [[nodiscard]] virtual bool IsCallable() const;
    /*!
     * \brief `x(*params.extraPosArgs, **params.extraKwArgs)`
     *
     * \ref UserCallableParams::extraPosArgs holds a \ref ValuesList and \ref UserCallableParams::extraKwArgs a \ref
     * ValuesMap, as for a \ref UserCallable declared with \ref ArgInfo::VarArgs and \ref ArgInfo::VarKwArgs. The
     * default throws Python's `TypeError` ("object is not callable").
     */
    virtual Value Call(const UserCallableParams& params) const;
    //! Whether `iter(x)` works, asked without iterating
    [[nodiscard]] virtual bool IsIterable() const;
    //! The items `iter(x)` gives; the list may be single-pass (no size, no indexer). Empty when \ref IsIterable is false
    [[nodiscard]] virtual std::optional<GenericList> Iterate() const;
    //! `reversed(x)`; empty where `reversed()` raises `TypeError`
    [[nodiscard]] virtual std::optional<GenericList> Reversed() const;
    //! `item in x` (`__contains__`); empty makes the engine iterate, as Python does without `__contains__`
    [[nodiscard]] virtual std::optional<bool> Contains(const Value& item) const;
    //! `len(x)`; empty for an object without a length
    [[nodiscard]] virtual std::optional<size_t> Len() const;
    //! `bool(x)`; the default is Python's without `__bool__`: false only for a length of zero
    [[nodiscard]] virtual bool Truth() const;
    //! `x == other`; empty is `NotImplemented`, and the engine asks the other operand, then compares identities
    [[nodiscard]] virtual std::optional<bool> Equals(const Value& other) const;
    //! `hash(x)`; empty for an unhashable object. The default hashes \ref Identity
    [[nodiscard]] virtual std::optional<size_t> Hash() const;
    //! The object behind this interface: the same for two interfaces of one host object. The default is `this`
    [[nodiscard]] virtual const void* Identity() const;
    //! `isinstance(x, collections.abc.Mapping)`
    [[nodiscard]] virtual bool IsMapping() const;
    //! `isinstance(x, (list, tuple))`
    [[nodiscard]] virtual bool IsSequence() const;
    //! `isinstance(x, tuple)`, a namedtuple too
    [[nodiscard]] virtual bool IsTuple() const;
    //! `hasattr(x, '__html__')`: the object prints as \ref Html under autoescape and is `escaped`
    [[nodiscard]] virtual bool IsMarkupLike() const;
    //! `x.__html__()`, UTF-8; the default is \ref Str
    [[nodiscard]] virtual std::string Html() const;
};

/*!
 * \brief A value that carries a host object
 *
 * The value holds a \ref GenericMap whose \ref GenericMap::AsHostObject gives `object` back, and the engine reads it as
 * \ref IHostObject describes. Two values made from one object, and every value the engine hands back for it, compare
 * equal and are `sameas` (\ref IHostObject::Identity).
 *
 * @param object The object; must not be null
 * @return The value
 */
JINJA2CPP_EXPORT Value MakeHostValue(std::shared_ptr<const IHostObject> object);

//! \ref MakeHostValue of a new object of type `T`, made from `args`
template<typename T, typename... Args>
Value MakeHostValue(Args&&... args)
{
    return MakeHostValue(std::shared_ptr<const IHostObject>(std::make_shared<const T>(std::forward<Args>(args)...)));
}

/*!
 * \brief What an undefined value says about the name it was looked up by
 *
 * The fields mirror Jinja2's `Undefined`: `_undefined_name`, `_undefined_obj` (as its type's repr), `_undefined_hint`.
 */
struct UndefinedDescription
{
    //! How the value behaves (Jinja2's `Undefined` class)
    UndefinedPolicy policy{};
    //! The variable or attribute name; for a non-string item key, its repr
    std::string name;
    //! Python's `object_type_repr` of the object the attribute or item is missing from ("dict object"); empty for a
    //! missing variable
    std::string objType;
    //! A string key, reported as a missing attribute rather than a missing item
    bool isAttr = true;
    //! Replaces the message built from the fields above when not empty
    std::string hint;
};

/*!
 * \brief An undefined value, as a host passes one into a render
 *
 * A filter, global or host object that returns it, or a parameter that holds it, is undefined to the template, with
 * the description's behaviour and error message.
 */
JINJA2CPP_EXPORT Value MakeUndefined(UndefinedDescription description);

/*!
 * \brief The description of an undefined value the engine handed to the host
 *
 * The engine hands undefined values to an \ref IHostObject and to a \ref UserCallable with \ref
 * UserCallable::keepUndefined set; every other callable gets `None` for them.
 *
 * @return The description, or empty for any other value
 */
JINJA2CPP_EXPORT std::optional<UndefinedDescription> UndefinedOf(const Value& value);
} // namespace v2
} // namespace jinja2

JINJA2CPP_SUPPRESS_DLL_INTERFACE_WARNINGS_END

#endif // JINJA2CPP_HOST_OBJECT_H
