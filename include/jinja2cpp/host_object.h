#ifndef JINJA2CPP_HOST_OBJECT_H
#define JINJA2CPP_HOST_OBJECT_H

#include <jinja2cpp/config.h>
#include <jinja2cpp/fwd.h> // IWYU pragma: export
#include <jinja2cpp/generic_list.h>
#include <jinja2cpp/value.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <new> // IWYU pragma: keep (placement new)
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

JINJA2CPP_SUPPRESS_DLL_INTERFACE_WARNINGS_BEGIN

namespace jinja2
{
inline namespace v2
{
namespace detail
{
struct HostResultAccess;
} // namespace detail

/*!
 * \brief Where a host object writes the value of a read the engine asks for
 *
 * The fast forms of \ref IHostObject (\ref IHostObject::GetAttrTo, \ref IHostObject::GetItemTo, \ref
 * IHostObject::ItemAtTo) write their result here instead of returning a \ref Value: a scalar or a string goes
 * straight into the engine's own storage, with no \ref Value made and converted. Only the library makes one, for
 * the duration of one call; the object writes it at most once per call, and the last write wins. A read that returns
 * true without writing gives `None`.
 */
class JINJA2CPP_EXPORT HostResult
{
public:
    HostResult(const HostResult&) = delete;
    HostResult(HostResult&&) = delete;
    HostResult& operator=(const HostResult&) = delete;
    HostResult& operator=(HostResult&&) = delete;
    ~HostResult() = default;

    //! `None`
    void SetNone();
    //! A `bool`
    void Set(bool value);
    //! An `int` within int64
    void Set(int64_t value);
    //! Any other integer type, as an `int`; an unsigned value above `INT64_MAX` wraps, so pass it as a \ref Value
    template<typename T, typename = std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, bool> && !std::is_same_v<T, int64_t>>>
    void Set(T value)
    {
        Set(static_cast<int64_t>(value));
    }
    //! A `float`
    void Set(double value);
    //! A `str`, UTF-8; the text is copied
    void SetString(std::string_view text);
    //! A `Markup` (printed unescaped under autoescape), UTF-8; the text is copied
    void SetMarkup(std::string_view text);
    //! Any value, as \ref IHostObject::GetAttr would return it
    void Set(Value value);
    //! A pointer is not a `bool`: use \ref SetString for text
    void Set(const char*) = delete;
    /*!
     * \brief A host object, as `Set(MakeHostValue(object))` would write it
     *
     * The engine reads the object as \ref IHostObject describes, with no \ref Value or \ref GenericMap made on the way:
     * a nested object of the host (a member that is itself a dict, a list or any other object) costs one bridge
     * allocation. The object lives as long as the engine or the host holds the value. Two reads that write objects with
     * one \ref IHostObject::Identity are `sameas`.
     *
     * @param object The object; must not be null
     */
    void SetHostObject(std::shared_ptr<const IHostObject> object);
    /*!
     * \brief A new host object of type `T`, made from `args`, as `Set(MakeHostValue<T>(args...))` would write it
     *
     * The object is constructed in the engine's own allocation, as \ref MakeHostValue<T> constructs it: one
     * allocation holds the object and the engine's bridge, and nothing else is made. An object more aligned than `new`
     * guarantees is made in an allocation of its own.
     */
    template<typename T, typename... Args>
    void SetHostObject(Args&&... args);

private:
    friend struct detail::HostResultAccess;
    explicit HostResult(void* target) noexcept;
    // SetHostObject<T>: `construct` places the object at `where`, as detail::MakeHostValueInPlace's does. Frozen ABI
    // with it (docs/api-2.0.md, section 7 item 10)
    void SetHostObjectInPlace(size_t size, size_t align, const IHostObject* (*construct)(void* where, void* context), void* context);

    // The engine's value the result goes to
    void* m_target;
};

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
 * - `x.name`: \ref GetAttrTo, then \ref GetItemTo with the name (Jinja2's `Environment.getattr`)
 * - `x[key]`: \ref GetItemTo for a string key, else \ref GetItem; then \ref GetAttrTo for a string key (Jinja2's
 *   `Environment.getitem`)
 * - `x[start:stop:step]`: \ref GetSlice, with no fallback
 * - `x(...)`: \ref Call, when \ref IsCallable
 * - iteration (`for`, `list`, `in` without \ref Contains): \ref Len and \ref ItemAtTo for a \ref IsSequence object
 *   with a length, else \ref Iterate
 * - `reversed`: \ref Reversed
 * - `len`: \ref Len; truth: \ref Truth
 * - `==`, `!=`: \ref Equals of the left operand, then of the right one, then \ref Identity
 * - a dict key: \ref Hash tells whether it is hashable; the key is the object's \ref Identity
 * - `is sameas`: \ref Identity
 * - `is number`: \ref IsNumber
 * - `int`: \ref ToInt, then \ref ToFloat truncated, then the default; `float`: \ref ToFloat, then the default
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
     * \brief \ref GetAttr, with the value written to `out`
     *
     * The engine reads `x.name` (and `x[key]`'s fallback) through this form only. The default forwards to \ref
     * GetAttr; an object whose attributes are often scalars or strings overrides it to skip the \ref Value.
     *
     * @return false when there is no such attribute (\ref GetAttr's empty result); `out` is then not read
     */
    [[nodiscard]] virtual bool GetAttrTo(std::string_view name, HostResult& out) const;
    /*!
     * \brief \ref GetItem of a string key, with the value written to `out`
     *
     * The engine reads `x["key"]` (and `x.name`'s fallback) through this form only; a key of any other type goes to
     * \ref GetItem. `key` may hold any byte, NUL included. The default forwards to \ref GetItem with
     * `Value(std::string(key))`.
     *
     * @return false when there is no such item; `out` is then not read
     */
    [[nodiscard]] virtual bool GetItemTo(std::string_view key, HostResult& out) const;
    /*!
     * \brief Item `index` of a \ref IsSequence object, with the value written to `out`
     *
     * The engine iterates an \ref IsSequence object that has a \ref Len and \ref IteratesByIndex by index: it asks \ref Len before every step
     * (a list the template shortens or extends while it loops ends where Python's list iterator ends), then this for
     * the item, so one iteration step is two calls and no \ref GenericList. `index` is below the \ref Len the
     * engine just read, unless the object changed in between.
     *
     * The default reads \ref Iterate: by index when its list has an indexer and a size, else by stepping an
     * enumerator `index + 1` times. It is correct for any object, but makes a list per item; a sequence with
     * random access should override it.
     *
     * @return false past the end (`IndexError`), or for an object that is no sequence; `out` is then not read
     */
    [[nodiscard]] virtual bool ItemAtTo(size_t index, HostResult& out) const;
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
    [[nodiscard]] virtual Value Call(const UserCallableParams& params) const;
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
    /*!
     * \brief Whether the engine iterates this \ref IsSequence object by index
     *
     * True by default: a loop over a sequence with a \ref Len reads \ref Len and \ref ItemAtTo at each step. A sequence whose `iter(x)` does not give its items by index, such as a Python `list` subclass that
     * overrides `__iter__`, answers false, and the engine iterates it through \ref Iterate instead. A loop's
     * `loop.length` is still \ref Len, read when the template first asks for it, and the operations of a list (`+`, `*`,
     * comparisons, `str % x`) still read the items by index, as Python's do.
     */
    [[nodiscard]] virtual bool IteratesByIndex() const;
    //! `isinstance(x, tuple)`, a namedtuple too
    [[nodiscard]] virtual bool IsTuple() const;
    //! `hasattr(x, '__html__')`: the object prints as \ref Html under autoescape and is `escaped`
    [[nodiscard]] virtual bool IsMarkupLike() const;
    //! `x.__html__()`, UTF-8; the default is \ref Str
    [[nodiscard]] virtual std::string Html() const;
    //! `isinstance(x, numbers.Number)`, for `is number`; false by default
    [[nodiscard]] virtual bool IsNumber() const;
    /*!
     * \brief `int(x)`, for the `int` filter
     *
     * @return An integer value (int64 or big int); nullopt when the object has no integer value (Python's TypeError
     * or ValueError), which is the default
     */
    [[nodiscard]] virtual std::optional<Value> ToInt() const;
    //! `float(x)`, for the `int` and `float` filters; nullopt when the object has no float value, which is the default
    [[nodiscard]] virtual std::optional<Value> ToFloat() const;
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

namespace detail
{
/*!
 * \brief \ref MakeHostValue of an object the library constructs in its own allocation
 *
 * `construct` places the object at `where` (`size` bytes aligned to `align`, not above the default `new` alignment)
 * and returns it; `context` is passed through. The object is destroyed through its virtual destructor and freed
 * with the value's last reference. One allocation holds the object, the engine's bridge and the ownership.
 */
JINJA2CPP_EXPORT Value MakeHostValueInPlace(size_t size, size_t align, const IHostObject* (*construct)(void* where, void* context), void* context);
} // namespace detail

namespace detail
{
// Whether the library can place a `T` in a block of its own (MakeHostValueInPlace)
template<typename T>
constexpr bool HostObjectFitsInPlace = alignof(T) <= __STDCPP_DEFAULT_NEW_ALIGNMENT__ && std::is_base_of_v<IHostObject, T>;

// The `construct` of MakeHostValueInPlace for a `T` made from the arguments `context` (a `Pack`) forwards
template<typename T, typename Pack>
const IHostObject* PlaceHostObject(void* where, void* context)
{
    return std::apply([where](auto&&... a) -> const IHostObject* { return ::new (where) T(std::forward<decltype(a)>(a)...); }, std::move(*static_cast<Pack*>(context)));
}
} // namespace detail

//! \ref MakeHostValue of a new object of type `T`, made from `args`
template<typename T, typename... Args>
Value MakeHostValue(Args&&... args)
{
    if constexpr (detail::HostObjectFitsInPlace<T>)
    {
        auto pack = std::forward_as_tuple(std::forward<Args>(args)...);
        return detail::MakeHostValueInPlace(sizeof(T), alignof(T), &detail::PlaceHostObject<T, decltype(pack)>, &pack);
    }
    else
    {
        return MakeHostValue(std::shared_ptr<const IHostObject>(std::make_shared<const T>(std::forward<Args>(args)...)));
    }
}

template<typename T, typename... Args>
void HostResult::SetHostObject(Args&&... args)
{
    if constexpr (detail::HostObjectFitsInPlace<T>)
    {
        auto pack = std::forward_as_tuple(std::forward<Args>(args)...);
        SetHostObjectInPlace(sizeof(T), alignof(T), &detail::PlaceHostObject<T, decltype(pack)>, &pack);
    }
    else
    {
        SetHostObject(std::shared_ptr<const IHostObject>(std::make_shared<const T>(std::forward<Args>(args)...)));
    }
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
    //! A sandbox refused the attribute (Jinja2's `unsafe_undefined`): a use that fails reports
    //! ErrorCode::SecurityError instead of ErrorCode::UndefinedError (\ref ISandboxPolicy)
    bool security = false;
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
