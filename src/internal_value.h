#ifndef JINJA2CPP_SRC_INTERNAL_VALUE_H
#define JINJA2CPP_SRC_INTERNAL_VALUE_H

#include "loop_attr.h"

#include <jinja2cpp/config.h>
#include <jinja2cpp/generic_list.h>
#include <jinja2cpp/utils/i_comparable.h>
#include <jinja2cpp/utils/ordered_map.h>
#include <jinja2cpp/value.h>
#include <jinja2cpp/value_ptr.h>

#include <boost/iterator/iterator_facade.hpp>
#include <boost/core/span.hpp>
#include <boost/smart_ptr/intrusive_ptr.hpp>
#include <boost/unordered_map.hpp>
#include <fmt/core.h>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <new>
#include <stdexcept>
#include <string>
#include <utility>

#if defined(_MSC_VER) && _MSC_VER <= 1900 // robin_hood hash map doesn't compatible with MSVC 14.0
#include <unordered_map>
#else
#include "robin_hood.h"
#endif

#include <optional>
#include <string_view>
#include <type_traits>
#include <variant>

#include <algorithm>
#include <array>
#include <atomic>
#include <functional>
#include <initializer_list>
#include <vector>

namespace jinja2
{

template<class T>
class ReferenceWrapper
{
public:
    using type = T;

    ReferenceWrapper(T& ref) noexcept // NOLINT(google-explicit-constructor)
        : m_ptr(std::addressof(ref))
    {
    }

    ReferenceWrapper(T&&) = delete;
    ReferenceWrapper(const ReferenceWrapper&) noexcept = default;
    ReferenceWrapper(ReferenceWrapper&&) noexcept = default;
    ~ReferenceWrapper() = default;

    // assignment
    ReferenceWrapper& operator=(const ReferenceWrapper& x) noexcept = default;
    ReferenceWrapper& operator=(ReferenceWrapper&& x) noexcept = default;

    // access
    [[nodiscard]] T& get() const noexcept
    {
        return *m_ptr;
    }

private:
    T* m_ptr;
};

class ValueObject;

namespace detail
{
// Deletes an object whose last reference went. A value nested as deep as a loop iterates
// would otherwise be deleted by as many nested destructors and overflow the stack
// (docs/tasks/0093): past a fixed depth the object waits on a per-thread list, which the
// outermost deletion empties in a loop, as CPython's "trashcan" does.
// Exported: intrusive_ptr_release calls it from code outside a shared library.
JINJA2CPP_EXPORT void DestroyValueObject(const ValueObject* obj) noexcept;
} // namespace detail

// The base of every object a value owns: a long string, a list or map accessor, a pair, a
// callable, the info of an undefined value. Values share such objects through one
// boost::intrusive_ptr (docs/tasks/0140), so the rest of a value is trivially copyable and
// copying it is a memcpy plus, for an owning value, one reference count increment. The
// objects are immutable once shared, apart from state their classes mark mutable; the count
// is atomic, as templates and their constants are shared by concurrent renders.
class ValueObject
{
public:
    ValueObject() = default;
    // A copy is a new object: its count starts at zero
    ValueObject(const ValueObject& other) noexcept
        : m_flags(other.m_flags)
    {
    }
    ValueObject(ValueObject&& other) noexcept
        : m_flags(other.m_flags)
    {
    }
    ValueObject& operator=(const ValueObject& other) noexcept
    {
        m_flags = other.m_flags;
        return *this;
    }
    ValueObject& operator=(ValueObject&& other) noexcept
    {
        m_flags = other.m_flags;
        return *this;
    }
    virtual ~ValueObject() = default;

    // NOLINTNEXTLINE(readability-identifier-naming): the name boost::intrusive_ref_counter uses
    [[nodiscard]] unsigned use_count() const noexcept { return m_refCount.load(std::memory_order_acquire); }

    // An object with state of its own that each copy of the value must not share (a
    // generator's iteration): copying the value clones it (IListAccessor::Clone)
    [[nodiscard]] bool ClonesOnCopy() const noexcept { return (m_flags & CloneFlag) != 0; }
    // The object InternalValue::SetParentData makes: it keeps the value's own object and the
    // value it was taken from alive
    [[nodiscard]] bool IsAnchor() const noexcept { return (m_flags & AnchorFlag) != 0; }

    // boost::intrusive_ptr's hooks
    // NOLINTNEXTLINE(readability-identifier-naming)
    friend void intrusive_ptr_add_ref(const ValueObject* obj) noexcept { obj->m_refCount.fetch_add(1, std::memory_order_relaxed); }
    // NOLINTNEXTLINE(readability-identifier-naming)
    friend void intrusive_ptr_release(const ValueObject* obj) noexcept
    {
        if (obj->m_refCount.fetch_sub(1, std::memory_order_acq_rel) == 1)
        {
            detail::DestroyValueObject(obj);
        }
    }

protected:
    void SetClonesOnCopy(bool clones = true) noexcept { m_flags = clones ? static_cast<uint8_t>(m_flags | CloneFlag) : static_cast<uint8_t>(m_flags & ~CloneFlag); }
    void SetAnchor() noexcept { m_flags |= AnchorFlag; }

private:
    static constexpr uint8_t CloneFlag = 1;
    static constexpr uint8_t AnchorFlag = 2;
    mutable std::atomic<unsigned> m_refCount{ 0 };
    uint8_t m_flags = 0;
};

using ValueObjectPtr = boost::intrusive_ptr<const ValueObject>;

// Makes a refcounted object: no call site writes `new`
template<typename T, typename... Args>
boost::intrusive_ptr<T> MakeRef(Args&&... args)
{
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory): owned by the intrusive pointer from here on
    return boost::intrusive_ptr<T>(new T(std::forward<Args>(args)...));
}

// An object of a type a value holds by reference (a pair, a callable, a renderer)
template<typename T>
class ValueBox final : public ValueObject
{
public:
    explicit ValueBox(T&& value)
        : m_value(std::move(value))
    {
    }

    [[nodiscard]] const T& Get() const noexcept { return m_value; }

private:
    T m_value;
};

#ifdef _MSC_VER
#define JINJA2CPP_COLD_INLINE __declspec(noinline) inline
#define JINJA2CPP_OUTLINE_INLINE __declspec(noinline) inline
#else
#define JINJA2CPP_COLD_INLINE __attribute__((noinline, cold)) inline
#define JINJA2CPP_OUTLINE_INLINE __attribute__((noinline)) inline
#endif

// Holds a value whose type is incomplete where InternalValue is declared (a pair or a
// callable), in the box a value of it then shares: copying or moving it never allocates
template<typename T>
class RecursiveWrapper
{
public:
    RecursiveWrapper(const T& value) // NOLINT(google-explicit-constructor)
        : m_data(Make(T(value)))
    {}

    RecursiveWrapper(T&& value) // NOLINT(google-explicit-constructor)
        : m_data(Make(std::move(value)))
    {}

    [[nodiscard]] const T& GetValue() const { return m_data->Get(); }
    [[nodiscard]] const boost::intrusive_ptr<const ValueBox<T>>& GetBox() const& noexcept { return m_data; }
    [[nodiscard]] boost::intrusive_ptr<const ValueBox<T>> GetBox() && noexcept { return std::move(m_data); }

private:
    // Out of line: making a pair or callable is rare on hot paths, and inlining the
    // allocation into every function that returns one (loop attributes, filters) pushes
    // those functions past the inliner's budget
    static JINJA2CPP_COLD_INLINE boost::intrusive_ptr<const ValueBox<T>> Make(T&& value) { return MakeRef<ValueBox<T>>(std::move(value)); }

    boost::intrusive_ptr<const ValueBox<T>> m_data;
};

template<typename T>
auto MakeWrapped(T&& val)
{
    return RecursiveWrapper<std::decay_t<T>>(std::forward<T>(val));
}

using ValueRef = ReferenceWrapper<const Value>;
using TargetString = std::variant<std::string, std::wstring>;
using TargetStringView = std::variant<std::string_view, std::wstring_view>;

class ListAdapter;
class MapAdapter;
class RenderContext;
class OutStream;
class Callable;
struct CallParams;
struct KeyValuePair;
class IRendererBase;

// What a missing name, attribute or item evaluates to (Python's Undefined). It is the
// default alternative, so every lookup miss and error path that returns InternalValue()
// is undefined, while NoneValue is Python's None. A named lookup miss carries info
// (src/undefined.h): what was missing and the undefined policy that decides how its uses
// fail. Without info (internal error paths) it behaves like Python's default Undefined
// that is only printed or tested.
struct UndefinedInfo;
// The info is owned by the value, which shares it (UndefinedInfo is a ValueObject)
struct UndefinedValue
{
    const UndefinedInfo* info = nullptr;
    // Set only in a Slot that holds no value (slot_frame.h)
    bool unbound = false;
    // Bound by a frame as it starts, for a name it stores later (BindUndefinedStatement): a
    // block, or a template included or imported with context, passes over it to the value
    // outside, as Jinja2 hands those only the names already stored (docs/tasks/0201)
    bool atEntry = false;
    // The low bits of the hash of the name it is bound to, so that a copy stored under another
    // name, which Jinja2 sees undefined, is not passed over
    std::uint32_t entryHash = 0;
};

inline bool operator==(const UndefinedValue&, const UndefinedValue&)
{
    return true;
}

inline bool operator!=(const UndefinedValue&, const UndefinedValue&)
{
    return false;
}

class InternalValue;

// What a value is, whatever represents it: code outside internal_value.* tells values apart
// by it rather than by the alternatives of the value's storage (docs/tasks/0140)
enum class ValueKind : uint8_t
{
    Undefined,
    None,
    Bool,
    // Narrow or wide, owned or viewed
    String,
    Int,
    Double,
    // A host value the template reads without converting it
    HostValue,
    List,
    Map,
    KeyValuePair,
    Callable,
    Renderer
};

struct IListAccessor;
struct IMapAccessor;

namespace detail
{
// What a value holds in place (docs/tasks/0140): every alternative is trivially copyable, so
// copying a value never dispatches on its kind. A value that owns something holds a typed
// pointer to it here and shares the object (InternalValue::m_obj), which keeps it alive.

// The flag of a string value marked as Python's Markup, in the last byte of each string kind
inline constexpr uint8_t MarkupFlag = 0x80;

// A string short enough to keep in place: 15 narrow characters, as many as std::string keeps
// without allocating, or 3 (7 on Windows) wide ones
template<typename CharT>
struct SmallString
{
    static constexpr size_t Capacity = 15 / sizeof(CharT);
    std::array<CharT, Capacity> chars;
    // The size in the low bits, MarkupFlag in the top one
    uint8_t meta;

    [[nodiscard]] std::basic_string_view<CharT> View() const noexcept { return { chars.data(), static_cast<size_t>(meta & 0x0F) }; }
};

// A string the value owns in a StringObject
template<typename CharT>
struct StringObject;

template<typename CharT>
struct HeapString
{
    const StringObject<CharT>* object;
    std::array<uint8_t, 7> pad;
    uint8_t meta;
};

// Text the value does not own (an item of a host list or map read in place). Views longer
// than 2^32-1 characters are stored as owned strings instead
template<typename CharT>
struct StringView
{
    const CharT* data;
    uint32_t size;
    std::array<uint8_t, 3> pad;
    uint8_t meta;

    [[nodiscard]] std::basic_string_view<CharT> View() const noexcept { return { data, size }; }
};

// A list or a map: the accessor, which the value's object keeps alive (it is that object, or
// part of it)
struct ListHandle
{
    const IListAccessor* accessor;
    bool isTuple;
};

struct MapHandle
{
    const IMapAccessor* accessor;
};

// A pair, a callable or a renderer, in a ValueBox
template<typename T>
struct BoxHandle
{
    const T* value;
};

using RendererPtr = std::shared_ptr<IRendererBase>;

using ValueStorage = std::variant<
    UndefinedValue,
    NoneValue,
    bool,
    int64_t,
    double,
    SmallString<char>,
    SmallString<wchar_t>,
    HeapString<char>,
    HeapString<wchar_t>,
    StringView<char>,
    StringView<wchar_t>,
    ValueRef,
    ListHandle,
    MapHandle,
    BoxHandle<KeyValuePair>,
    BoxHandle<Callable>,
    BoxHandle<RendererPtr>>;

static_assert(std::is_trivially_copyable_v<ValueStorage>, "copying a value must not dispatch on its kind");
static_assert(sizeof(ValueStorage) <= 24);
} // namespace detail

using InternalValueRef = ReferenceWrapper<InternalValue>;
using InternalValueList = std::vector<InternalValue>;

// The most items a list, or characters a string, may hold when an operation builds it in
// one go. range() reports up to 2^64 items without storing them, and reserving that many
// asks for terabytes, which ASan aborts on (docs/tasks/0097). Python fails such a list with
// MemoryError; Jinja2C++ reports it before allocating. 2^26 list items take 2 GB, which is
// what the fuzz jobs' memory limit allows in one allocation (docs/tasks/0185).
constexpr size_t MaxSequenceSize = size_t{ 1 } << 26;

// Throws when an operation is about to build a sequence longer than MaxSequenceSize
inline void CheckSequenceSize(uint64_t size, const char* what)
{
    if (size > MaxSequenceSize)
    {
        throw std::length_error(std::string(what) + " of length " + std::to_string(size) + " is too long");
    }
}

// A template so that reserve() is instantiated where it is called, with InternalValue complete
template<typename List>
void ReserveHint(List& list, size_t size)
{
    CheckSequenceSize(size, "a list");
    list.reserve(size);
}

// Mappings a template can iterate (dict literals, kwargs) keep insertion order, as Python
// dicts do; scopes and other lookup-only maps stay InternalValueMap (docs/tasks/0031)
using InternalDict = OrderedMap<std::string, InternalValue>;

// A dict with Python's keys: any hashable value, over the string keys of InternalDict (template_dict.h)
class TemplateDict;


struct IListAccessor;
struct IListAccessorEnumerator;
using ListAccessorEnumeratorPtr = types::ValuePtr<IListAccessorEnumerator>;
struct IListAccessorEnumerator : virtual IComparable
{
    ~IListAccessorEnumerator() override = default;

    virtual void Reset() = 0;

    virtual bool MoveNext() = 0;
    [[nodiscard]] virtual InternalValue GetCurrent() const = 0;

    [[nodiscard]] virtual std::optional<ListAccessorEnumeratorPtr> Clone() const = 0;
    virtual std::optional<ListAccessorEnumeratorPtr> Transfer() = 0;
    // Points the enumerator at the start of `list`, so that a loop entered again reuses it
    // instead of allocating another (docs/tasks/0133); null lets go of the list it was
    // on. False when it cannot enumerate that list: the caller makes a new one
    virtual bool Rebind(const IListAccessor* /*list*/) { return false; }
    /*
    struct Cloner
    {
        Cloner() = default;

        IListAccessorEnumerator* operator()(const IListAccessorEnumerator &x) const
        {
            return x.Clone();
        }

        IListAccessorEnumerator* operator()(IListAccessorEnumerator &&x) const
        {
            return x.Transfer();
        }
    };
    */
};

using ListAccessorEnumeratorPtr = types::ValuePtr<IListAccessorEnumerator>;

// The arguments of a range() call, kept so that the range prints as range(0, 3)
struct RangeInfo
{
    int64_t start;
    int64_t stop;
    int64_t step;
};

// A non-owning reference to a callable: a callback that does not outlive the call, without
// the allocation and indirection of std::function
template<typename Fn>
class FunctionRef;

template<typename R, typename... Args>
class FunctionRef<R(Args...)>
{
public:
    template<typename F, typename = std::enable_if_t<!std::is_same_v<std::decay_t<F>, FunctionRef>>>
    // NOLINTNEXTLINE(google-explicit-constructor,bugprone-forwarding-reference-overload,cppcoreguidelines-missing-std-forward): binds like a function parameter, keeps only the address
    FunctionRef(F&& fn) noexcept
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast): restored by the cast in m_call
        : m_obj(const_cast<void*>(static_cast<const void*>(std::addressof(fn))))
        , m_call([](void* obj, Args... args) -> R { return (*static_cast<std::remove_reference_t<F>*>(obj))(std::forward<Args>(args)...); })
    {
    }

    R operator()(Args... args) const { return m_call(m_obj, std::forward<Args>(args)...); }

private:
    using Thunk = R (*)(void*, Args...);
    void* m_obj;
    Thunk m_call;
};

struct IListAccessor : ValueObject
{

    [[nodiscard]] virtual std::optional<size_t> GetSize() const = 0;
    [[nodiscard]] virtual std::optional<InternalValue> GetItem(int64_t idx) const = 0;
    [[nodiscard]] virtual std::optional<ListAccessorEnumeratorPtr> CreateListAccessorEnumerator() const = 0;
    // Passes the items to fn in order until it returns false. By default through an
    // enumerator; an indexed list reads its items directly
    using ItemVisitor = FunctionRef<bool(InternalValue&&)>;
    virtual void ForEach(ItemVisitor fn) const;
    [[nodiscard]] virtual GenericList CreateGenericList() const = 0;
    [[nodiscard]] virtual bool ShouldExtendLifetime() const = 0;
    // The object behind the list: the same for two accessors that share their data, so
    // printing can tell a list that contains itself
    [[nodiscard]] virtual const void* GetIdentity() const { return this; }
    // Set only for the lists made by range()
    [[nodiscard]] virtual const RangeInfo* GetRangeInfo() const { return nullptr; }
    // The items of a list the template owns (shared by every copy of the value, as a
    // Python list is), so that methods like append() can change them; null for borrowed
    // and computed lists (docs/tasks/0020)
    [[nodiscard]] virtual InternalValueList* GetMutableItems() const { return nullptr; }
    // An accessor with iteration state of its own (a generator, which says so with
    // SetClonesOnCopy) is copied with the list value, so that copies iterate independently;
    // every other accessor is shared
    [[nodiscard]] virtual boost::intrusive_ptr<const IListAccessor> Clone() const { return nullptr; }
    // The field names of a namedtuple, read by x.name; null for other lists
    [[nodiscard]] virtual const std::vector<std::string>* GetFieldNames() const { return nullptr; }
};

// How x.name looks a name up on a map (docs/tasks/0020)
enum class MapAttrPolicy
{
    // An object stored as a map (loop, cycler, self, imported namespaces): keys only
    KeysOnly,
    // A Python dict (dict literals, kwargs, context mappings): dict methods first, as
    // Python's getattr finds them before the items
    MethodsFirst,
    // A host object (reflected structs, JSON bindings): its keys, then dict methods
    KeysFirst
};

struct IMapAccessor : ValueObject
{
    [[nodiscard]] virtual size_t GetSize() const = 0;
    [[nodiscard]] virtual bool HasValue(const std::string& name) const = 0;
    [[nodiscard]] virtual InternalValue GetItem(const std::string& name) const = 0;
    [[nodiscard]] virtual std::vector<std::string> GetKeys() const = 0;
    // The pairs in GetKeys() order. The default looks every key up again; adapters that
    // own a map read its entries directly
    [[nodiscard]] virtual std::vector<KeyValuePair> GetEntries() const;
    // The keys as template values, in GetKeys() order. The default turns each string into a value
    [[nodiscard]] virtual InternalValueList GetKeyValues() const;
    // The value under `key`, if there is one. The default answers a string key through
    // HasValue/GetItem and has no other key, as host maps hold only strings
    [[nodiscard]] virtual std::optional<InternalValue> FindItem(const InternalValue& key) const;
    // Const as reading is, so that a value can hold its accessor as a const object; the
    // accessor changes its own mutable state (docs/tasks/0140). By value: overrides store
    // the key. `set ns.attr = ...` has no use for the result.
    // NOLINTNEXTLINE(performance-unnecessary-value-param,modernize-use-nodiscard)
    virtual bool SetValue(std::string, const InternalValue&) const { return false; }
    [[nodiscard]] virtual GenericMap CreateGenericMap() const = 0;
    [[nodiscard]] virtual bool ShouldExtendLifetime() const = 0;
    // Whether the names are attributes of an object (a user-provided map, such as a reflected
    // struct) rather than the keys of a dict, which Python's getattr does not see
    [[nodiscard]] virtual bool HasAttributes() const { return false; }
    // See IListAccessor::GetIdentity
    [[nodiscard]] virtual const void* GetIdentity() const { return this; }
    // See IListAccessor::GetMutableItems
    [[nodiscard]] virtual TemplateDict* GetMutableItems() const { return nullptr; }
    // How x.name resolves against Python's dict methods (items, get, ...)
    [[nodiscard]] virtual MapAttrPolicy GetAttrPolicy() const { return MapAttrPolicy::KeysOnly; }
    // A namespace() object, the only one `set obj.attr = ...` can change
    [[nodiscard]] virtual bool IsNamespace() const { return false; }
    // The Python class of an object stored as a map (namespace(), cycler(), joiner()); null
    // for a mapping. Such an object is neither a mapping nor iterable, and prints as itself
    [[nodiscard]] virtual const char* GetObjectClass() const { return nullptr; }
    // The value an object prints with as <Class value> (`self`, <TemplateReference 'name'>);
    // null for one that prints as <module.Class object at 0x...>
    [[nodiscard]] virtual const InternalValue* GetReprArgument() const { return nullptr; }
    // A for loop's `loop` object puts the attribute `attr` in `value` and returns true when
    // it can be read without the generic lookup (`loop.index`, 0117 P1); others return false
    virtual bool GetLoopAttr(LoopAttr /*attr*/, InternalValue& /*value*/) const { return false; }
};

// A non-owning view of a list: code outside internal_value.* reads list values through it,
// whatever stores them (docs/tasks/0140). It lives no longer than the value or adapter it was
// taken from. ToAdapter() makes an owning copy for a site that keeps the list; the view itself
// never copies the list, so reading a generator through it cannot iterate a copy of it.
class ListRef
{
public:
    ListRef(const IListAccessor* accessor, const ValueObject* owner, bool isTuple) noexcept
        : m_accessor(accessor)
        , m_owner(owner)
        , m_isTuple(isTuple)
    {
    }

    [[nodiscard]] std::optional<size_t> GetSize() const
    {
        if (m_accessor)
        {
            return m_accessor->GetSize();
        }
        return 0;
    }
    [[nodiscard]] InternalValue GetValueByIndex(int64_t idx) const;
    [[nodiscard]] bool ShouldExtendLifetime() const { return m_accessor != nullptr && m_accessor->ShouldExtendLifetime(); }
    [[nodiscard]] InternalValueList ToValueList() const;
    // Passes the items to fn in order until it returns false, without an iterator
    void ForEach(IListAccessor::ItemVisitor fn) const;
    [[nodiscard]] const void* GetIdentity() const { return m_accessor ? m_accessor->GetIdentity() : nullptr; }
    [[nodiscard]] const RangeInfo* GetRangeInfo() const { return m_accessor ? m_accessor->GetRangeInfo() : nullptr; }
    [[nodiscard]] InternalValueList* GetMutableItems() const { return m_accessor ? m_accessor->GetMutableItems() : nullptr; }
    [[nodiscard]] GenericList CreateGenericList() const { return m_accessor ? m_accessor->CreateGenericList() : GenericList(); }
    [[nodiscard]] std::optional<ListAccessorEnumeratorPtr> GetEnumerator() const;
    // Sets `enumerator` to the start of this list, reusing the one it holds when that can
    // enumerate this list (IListAccessorEnumerator::Rebind)
    void RebindEnumerator(std::optional<ListAccessorEnumeratorPtr>& enumerator) const;

    class Iterator;

    [[nodiscard]] Iterator begin() const;
    [[nodiscard]] Iterator end() const; // NOLINT(readability-convert-member-functions-to-static): container API

    // Tuples are lists that print as (a, b) instead of [a, b]
    [[nodiscard]] bool IsTuple() const { return m_isTuple; }
    [[nodiscard]] const std::vector<std::string>* GetFieldNames() const { return m_accessor ? m_accessor->GetFieldNames() : nullptr; }

    // An owning copy, which shares the items (or clones a generator) as copying the value does
    [[nodiscard]] ListAdapter ToAdapter() const;

    [[nodiscard]] const IListAccessor* GetAccessor() const noexcept { return m_accessor; }
    [[nodiscard]] const ValueObject* GetOwner() const noexcept { return m_owner; }

private:
    const IListAccessor* m_accessor;
    const ValueObject* m_owner;
    bool m_isTuple;
};

// A list and what keeps it alive: an owning list, for code that builds or keeps one. A value
// holds the same parts (the accessor in place, the owner as its object), so converting between
// the two moves two pointers
class ListAdapter
{
public:
    using Iterator = ListRef::Iterator;

    ListAdapter() = default;
    // Copies of the list share the accessor, as names share a Python list, unless it has
    // iteration state of its own (IListAccessor::Clone)
    explicit ListAdapter(boost::intrusive_ptr<const IListAccessor> accessor) noexcept
        : m_accessor(accessor.get())
        , m_owner(std::move(accessor))
    {
    }
    // An accessor that is part of a bigger object, which owner keeps alive
    ListAdapter(const IListAccessor* accessor, ValueObjectPtr owner, bool isTuple = false) noexcept
        : m_accessor(accessor)
        , m_owner(std::move(owner))
        , m_isTuple(isTuple)
    {
    }
    ListAdapter(const ListAdapter& other)
        : m_accessor(other.m_accessor)
        , m_owner(other.m_owner)
        , m_isTuple(other.m_isTuple)
    {
        if (m_owner && m_owner->ClonesOnCopy())
        {
            Clone();
        }
    }
    ListAdapter(ListAdapter&& other) noexcept
        : m_accessor(std::exchange(other.m_accessor, nullptr))
        , m_owner(std::move(other.m_owner))
        , m_isTuple(other.m_isTuple)
    {
    }

    static ListAdapter CreateAdapter(InternalValueList&& values);
    static ListAdapter CreateAdapter(const GenericList& values);
    static ListAdapter CreateAdapter(const ValuesList& values);
    static ListAdapter CreateAdapter(GenericList&& values);
    static ListAdapter CreateAdapter(ValuesList&& values);
    static ListAdapter CreateAdapter(std::function<std::optional<InternalValue>()> fn);
    static ListAdapter CreateAdapter(size_t listSize, std::function<InternalValue(size_t idx)> fn);
    // The lazy list of range(start, stop, step), as in Python; step must not be zero
    static ListAdapter CreateRange(int64_t start, int64_t stop, int64_t step);

    ListAdapter& operator=(const ListAdapter& other)
    {
        if (this != &other)
        {
            *this = ListAdapter(other);
        }
        return *this;
    }
    ListAdapter& operator=(ListAdapter&& other) noexcept
    {
        if (this != &other)
        {
            m_accessor = std::exchange(other.m_accessor, nullptr);
            m_owner = std::move(other.m_owner);
            m_isTuple = other.m_isTuple;
        }
        return *this;
    }
    ~ListAdapter() = default;

    [[nodiscard]] ListRef Ref() const noexcept { return ListRef(m_accessor, m_owner.get(), m_isTuple); }

    [[nodiscard]] std::optional<size_t> GetSize() const { return Ref().GetSize(); }
    [[nodiscard]] InternalValue GetValueByIndex(int64_t idx) const;
    [[nodiscard]] bool ShouldExtendLifetime() const { return Ref().ShouldExtendLifetime(); }
    [[nodiscard]] InternalValueList ToValueList() const;
    // Passes the items to fn in order until it returns false, without an iterator
    void ForEach(IListAccessor::ItemVisitor fn) const { Ref().ForEach(fn); }
    [[nodiscard]] const void* GetIdentity() const { return Ref().GetIdentity(); }
    [[nodiscard]] const RangeInfo* GetRangeInfo() const { return Ref().GetRangeInfo(); }
    [[nodiscard]] InternalValueList* GetMutableItems() const { return Ref().GetMutableItems(); }
    [[nodiscard]] GenericList CreateGenericList() const { return Ref().CreateGenericList(); }
    [[nodiscard]] std::optional<ListAccessorEnumeratorPtr> GetEnumerator() const { return Ref().GetEnumerator(); }
    void RebindEnumerator(std::optional<ListAccessorEnumeratorPtr>& enumerator) const { Ref().RebindEnumerator(enumerator); }

    [[nodiscard]] Iterator begin() const;
    [[nodiscard]] Iterator end() const; // NOLINT(readability-convert-member-functions-to-static): container API

    [[nodiscard]] bool IsTuple() const { return m_isTuple; }
    ListAdapter& MarkAsTuple()
    {
        m_isTuple = true;
        return *this;
    }
    // A namedtuple: a tuple whose items can also be read by field name, like groupby's (grouper, list)
    ListAdapter& MarkAsNamedTuple(std::shared_ptr<const std::vector<std::string>> fieldNames);
    [[nodiscard]] const std::vector<std::string>* GetFieldNames() const { return Ref().GetFieldNames(); }

    [[nodiscard]] const IListAccessor* GetAccessor() const noexcept { return m_accessor; }

private:
    friend class InternalValue;

    // Gives this copy its own accessor (a generator's, IListAccessor::Clone)
    void Clone();

    const IListAccessor* m_accessor = nullptr;
    ValueObjectPtr m_owner;
    bool m_isTuple = false;
};

// A non-owning view of a map, as ListRef is of a list
class MapRef
{
public:
    MapRef(const IMapAccessor* accessor, const ValueObject* owner) noexcept
        : m_accessor(accessor)
        , m_owner(owner)
    {
    }

    [[nodiscard]] size_t GetSize() const { return m_accessor ? m_accessor->GetSize() : 0; }
    [[nodiscard]] bool HasValue(const std::string& name) const { return m_accessor != nullptr && m_accessor->HasValue(name); }
    [[nodiscard]] InternalValue GetValueByName(const std::string& name) const;
    [[nodiscard]] const void* GetIdentity() const { return m_accessor ? m_accessor->GetIdentity() : nullptr; }
    [[nodiscard]] bool HasAttributes() const { return m_accessor != nullptr && m_accessor->HasAttributes(); }
    [[nodiscard]] std::vector<KeyValuePair> GetEntries() const;
    [[nodiscard]] InternalValueList GetKeyValues() const;
    [[nodiscard]] std::optional<InternalValue> FindItem(const InternalValue& key) const;
    [[nodiscard]] TemplateDict* GetMutableItems() const { return m_accessor ? m_accessor->GetMutableItems() : nullptr; }
    [[nodiscard]] MapAttrPolicy GetAttrPolicy() const { return m_accessor ? m_accessor->GetAttrPolicy() : MapAttrPolicy::KeysOnly; }
    [[nodiscard]] bool IsNamespace() const { return m_accessor != nullptr && m_accessor->IsNamespace(); }
    [[nodiscard]] const char* GetObjectClass() const { return m_accessor ? m_accessor->GetObjectClass() : nullptr; }
    [[nodiscard]] const InternalValue* GetReprArgument() const { return m_accessor ? m_accessor->GetReprArgument() : nullptr; }
    // NOLINTNEXTLINE(modernize-use-nodiscard): see IMapAccessor::SetValue
    bool SetValue(std::string name, const InternalValue& val) const { return m_accessor != nullptr && m_accessor->SetValue(std::move(name), val); }
    [[nodiscard]] bool ShouldExtendLifetime() const { return m_accessor != nullptr && m_accessor->ShouldExtendLifetime(); }
    [[nodiscard]] GenericMap CreateGenericMap() const { return m_accessor ? m_accessor->CreateGenericMap() : GenericMap(); }
    // See IMapAccessor::GetLoopAttr
    bool GetLoopAttr(LoopAttr attr, InternalValue& value) const { return m_accessor != nullptr && m_accessor->GetLoopAttr(attr, value); }

    // An owning copy, which shares the map as copying the value does
    [[nodiscard]] MapAdapter ToAdapter() const;

    [[nodiscard]] const IMapAccessor* GetAccessor() const noexcept { return m_accessor; }

private:
    const IMapAccessor* m_accessor;
    const ValueObject* m_owner;
};

// A map and what keeps it alive, as ListAdapter is for a list
class MapAdapter
{
public:
    MapAdapter() = default;
    // Copies of the map share the accessor, as names share a Python dict
    explicit MapAdapter(boost::intrusive_ptr<const IMapAccessor> accessor) noexcept
        : m_accessor(accessor.get())
        , m_owner(std::move(accessor))
    {
    }
    // An accessor that is part of a bigger object, which owner keeps alive
    MapAdapter(const IMapAccessor* accessor, ValueObjectPtr owner) noexcept
        : m_accessor(accessor)
        , m_owner(std::move(owner))
    {
    }

    [[nodiscard]] MapRef Ref() const noexcept { return MapRef(m_accessor, m_owner.get()); }

    [[nodiscard]] size_t GetSize() const { return Ref().GetSize(); }
    [[nodiscard]] bool HasValue(const std::string& name) const { return Ref().HasValue(name); }
    [[nodiscard]] InternalValue GetValueByName(const std::string& name) const;
    [[nodiscard]] const void* GetIdentity() const { return Ref().GetIdentity(); }
    [[nodiscard]] bool HasAttributes() const { return Ref().HasAttributes(); }
    [[nodiscard]] std::vector<KeyValuePair> GetEntries() const;
    [[nodiscard]] InternalValueList GetKeyValues() const;
    [[nodiscard]] std::optional<InternalValue> FindItem(const InternalValue& key) const;
    [[nodiscard]] TemplateDict* GetMutableItems() const { return Ref().GetMutableItems(); }
    [[nodiscard]] MapAttrPolicy GetAttrPolicy() const { return Ref().GetAttrPolicy(); }
    [[nodiscard]] bool IsNamespace() const { return Ref().IsNamespace(); }
    [[nodiscard]] const char* GetObjectClass() const { return Ref().GetObjectClass(); }
    // NOLINTNEXTLINE(modernize-use-nodiscard): see IMapAccessor::SetValue
    bool SetValue(std::string name, const InternalValue& val) const { return Ref().SetValue(std::move(name), val); }
    [[nodiscard]] bool ShouldExtendLifetime() const { return Ref().ShouldExtendLifetime(); }
    [[nodiscard]] GenericMap CreateGenericMap() const { return Ref().CreateGenericMap(); }
    // See IMapAccessor::GetLoopAttr
    bool GetLoopAttr(LoopAttr attr, InternalValue& value) const { return Ref().GetLoopAttr(attr, value); }

private:
    friend class InternalValue;

    const IMapAccessor* m_accessor = nullptr;
    ValueObjectPtr m_owner;
};


namespace detail
{
// The one way into InternalValue's representation, for internal_value.* and the visitor
// machinery in value_visitors.h; everything else goes through GetIf, AsList, AsMap,
// AsStringView, TakeString, Kind() and Apply (docs/tasks/0140)
struct InternalValueAccess;

template<typename CharT>
struct StringObject final : ValueObject
{
    // An object of `size` characters, which follow it in the same allocation (docs/tasks/0151).
    // The caller writes them before it shares the object
    static boost::intrusive_ptr<StringObject> Make(size_t size)
    {
        // NOLINTNEXTLINE(cppcoreguidelines-owning-memory): owned by the intrusive pointer from here on
        return boost::intrusive_ptr<StringObject>(new (CharCount{ size }) StringObject(size));
    }

    // The room the characters take after the object
    struct CharCount
    {
        size_t count;
    };
    static void* operator new(size_t objectSize, CharCount chars) { return ::operator new(objectSize + (chars.count * sizeof(CharT))); }
    // Frees the block if the constructor throws (it does not)
    static void operator delete(void* ptr, CharCount /*chars*/) noexcept { ::operator delete(ptr); }
    // Frees the block: the deleting destructor calls this. Its operator new is the one above
    static void operator delete(void* ptr) noexcept { ::operator delete(ptr); } // NOLINT(misc-new-delete-overloads)

    // NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast): the characters follow the object, as
    // bytes counted from its start (a char pointer, so that the offset is not scaled)
    [[nodiscard]] boost::span<CharT> Chars() noexcept { return { reinterpret_cast<CharT*>(reinterpret_cast<unsigned char*>(this) + sizeof(StringObject)), m_size }; }
    [[nodiscard]] std::basic_string_view<CharT> View() const noexcept
    {
        return { reinterpret_cast<const CharT*>(reinterpret_cast<const unsigned char*>(this) + sizeof(StringObject)), m_size };
    }
    // NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast)

private:
    explicit StringObject(size_t size) noexcept
        : m_size(size)
    {
    }

    size_t m_size;
};

// The index of T among the alternatives of ValueStorage
template<typename T, typename Variant>
struct AlternativeIndex;

template<typename T, typename... Ts>
struct AlternativeIndex<T, std::variant<Ts...>>
{
    static constexpr size_t value = []() { // NOLINT(readability-identifier-naming): as std::integral_constant
        constexpr std::array<bool, sizeof...(Ts)> matches{ std::is_same_v<T, Ts>... };
        for (size_t idx = 0; idx != matches.size(); ++idx)
        {
            if (matches[idx])
            {
                return idx;
            }
        }
        return matches.size();
    }();
};

template<typename T>
inline constexpr size_t IndexOf = AlternativeIndex<T, ValueStorage>::value;

// The types a value holds in a ValueBox
template<typename T>
inline constexpr bool IsBoxed = std::is_same_v<T, KeyValuePair> || std::is_same_v<T, Callable> || std::is_same_v<T, RendererPtr>;
} // namespace detail

// A template value: 32 bytes, a trivially copyable ValueStorage (scalars, short strings,
// views, and typed pointers to what the value owns) plus the one pointer that owns it
// (docs/tasks/0140)
class InternalValue
{
public:
    InternalValue() noexcept = default;

    InternalValue(bool val) noexcept // NOLINT(google-explicit-constructor)
        : m_imm(val)
    {
    }
    // A string literal would otherwise convert to bool: say which string it is
    InternalValue(const char*) = delete;
    InternalValue(const wchar_t*) = delete;

    template<typename T, std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, bool>, int> = 0>
    InternalValue(T val) noexcept // NOLINT(google-explicit-constructor)
        : m_imm(static_cast<int64_t>(val))
    {
    }

    template<typename T, std::enable_if_t<std::is_floating_point_v<T>, int> = 0>
    InternalValue(T val) noexcept // NOLINT(google-explicit-constructor)
        : m_imm(static_cast<double>(val))
    {
    }

    InternalValue(NoneValue /*val*/) noexcept // NOLINT(google-explicit-constructor)
        : m_imm(NoneValue{})
    {
    }

    // Shares the info of val (undefined.h)
    InternalValue(const UndefinedValue& val) noexcept; // NOLINT(google-explicit-constructor)

    InternalValue(const std::string& val) { InitString(std::string_view(val)); }   // NOLINT(google-explicit-constructor)
    InternalValue(const std::wstring& val) { InitString(std::wstring_view(val)); } // NOLINT(google-explicit-constructor)
    // The text is borrowed: the caller keeps it alive for as long as the value lives
    InternalValue(std::string_view val) { InitView(val); }  // NOLINT(google-explicit-constructor)
    InternalValue(std::wstring_view val) { InitView(val); } // NOLINT(google-explicit-constructor)

    // A string value of `size` characters that `write(boost::span<CharT>)` fills: in the value itself
    // when short, else in its object, allocated once (docs/tasks/0151)
    template<typename CharT, typename Fn>
    static InternalValue MakeText(size_t size, const Fn& write)
    {
        InternalValue result;
        if (size <= detail::SmallString<CharT>::Capacity)
        {
            detail::SmallString<CharT> small{};
            write(boost::span<CharT>(small.chars.data(), size));
            small.meta = static_cast<uint8_t>(size);
            result.m_imm = small;
        }
        else
        {
            result.InitHeapString<CharT>(size, write);
        }
        return result;
    }

    // A string value of its own holding a copy of the parts, one after another; unlike the
    // view constructors it does not borrow them
    template<typename CharT>
    static InternalValue FromText(std::initializer_list<std::basic_string_view<CharT>> parts)
    {
        size_t size = 0;
        for (const auto& part : parts)
        {
            size += part.size();
        }
        return MakeText<CharT>(size, [parts](boost::span<CharT> chars) {
            auto out = chars.begin();
            for (const auto& part : parts)
            {
                out = std::copy(part.begin(), part.end(), out);
            }
        });
    }
    static InternalValue FromText(std::string_view text) { return FromText<char>({ text }); }
    static InternalValue FromText(std::wstring_view text) { return FromText<wchar_t>({ text }); }

    // A string of either character type is stored as itself
    InternalValue(const TargetString& val) // NOLINT(google-explicit-constructor)
    {
        std::visit([this](const auto& str) { InitString(std::basic_string_view<typename std::decay_t<decltype(str)>::value_type>(str)); }, val);
    }

    InternalValue(TargetStringView val) // NOLINT(google-explicit-constructor)
    {
        std::visit([this](auto view) { InitView(view); }, val);
    }

    InternalValue(ValueRef val) noexcept // NOLINT(google-explicit-constructor)
        : m_imm(val)
    {
    }

    // NOLINTNEXTLINE(google-explicit-constructor,cppcoreguidelines-rvalue-reference-param-not-moved): takes its parts
    InternalValue(ListAdapter&& val) noexcept
        : m_imm(detail::ListHandle{ std::exchange(val.m_accessor, nullptr), val.m_isTuple })
        , m_obj(std::move(val.m_owner))
    {
    }

    InternalValue(const ListAdapter& val) // NOLINT(google-explicit-constructor)
        : InternalValue(ListAdapter(val))
    {
    }

    // NOLINTNEXTLINE(google-explicit-constructor,cppcoreguidelines-rvalue-reference-param-not-moved): takes its parts
    InternalValue(MapAdapter&& val) noexcept
        : m_imm(detail::MapHandle{ std::exchange(val.m_accessor, nullptr) })
        , m_obj(std::move(val.m_owner))
    {
    }

    InternalValue(const MapAdapter& val) noexcept // NOLINT(google-explicit-constructor)
        : m_imm(detail::MapHandle{ val.m_accessor })
        , m_obj(val.m_owner)
    {
    }

    template<typename T, std::enable_if_t<detail::IsBoxed<T>, int> = 0>
    InternalValue(RecursiveWrapper<T>&& val) noexcept // NOLINT(google-explicit-constructor)
        : m_imm(detail::BoxHandle<T>{ &val.GetValue() })
        , m_obj(std::move(val).GetBox())
    {
    }

    template<typename T, std::enable_if_t<detail::IsBoxed<T>, int> = 0>
    InternalValue(const RecursiveWrapper<T>& val) noexcept // NOLINT(google-explicit-constructor)
        : m_imm(detail::BoxHandle<T>{ &val.GetValue() })
        , m_obj(val.GetBox())
    {
    }

    InternalValue(KeyValuePair&& val);      // NOLINT(google-explicit-constructor)
    InternalValue(const KeyValuePair& val); // NOLINT(google-explicit-constructor)
    InternalValue(Callable&& val);          // NOLINT(google-explicit-constructor)
    InternalValue(const Callable& val);     // NOLINT(google-explicit-constructor)
    InternalValue(detail::RendererPtr val); // NOLINT(google-explicit-constructor)

    InternalValue(const InternalValue& other)
        : m_imm(other.m_imm)
        , m_obj(other.m_obj)
    {
        if (m_obj && m_obj->ClonesOnCopy())
        {
            CloneObject();
        }
    }

    InternalValue(InternalValue&& other) noexcept
        : m_imm(other.m_imm)
        , m_obj(std::move(other.m_obj))
    {
        other.m_imm = detail::ValueStorage();
    }

    // Both assignments read everything from other before letting go of what this value
    // held, which may own other (x = x[0])
    InternalValue& operator=(const InternalValue& other)
    {
        if (this != &other)
        {
            if (other.m_obj && other.m_obj->ClonesOnCopy())
            {
                return *this = InternalValue(other);
            }
            ValueObjectPtr obj = other.m_obj;
            m_imm = other.m_imm;
            m_obj.swap(obj);
        }
        return *this;
    }

    InternalValue& operator=(InternalValue&& other) noexcept
    {
        if (this != &other)
        {
            const auto imm = other.m_imm;
            ValueObjectPtr obj = std::move(other.m_obj);
            other.m_imm = detail::ValueStorage();
            m_imm = imm;
            m_obj.swap(obj);
        }
        return *this;
    }

    ~InternalValue() = default;

    void SetParentData(const InternalValue& val);

    [[nodiscard]] bool ShouldExtendLifetime() const
    {
        if (m_obj && m_obj->IsAnchor())
        {
            return true;
        }
        if (const auto* list = std::get_if<detail::ListHandle>(&m_imm))
        {
            return list->accessor != nullptr && list->accessor->ShouldExtendLifetime();
        }
        if (const auto* map = std::get_if<detail::MapHandle>(&m_imm))
        {
            return map->accessor != nullptr && map->accessor->ShouldExtendLifetime();
        }
        return false;
    }

    [[nodiscard]] ValueKind Kind() const noexcept
    {
        // In the order of ValueStorage's alternatives
        static constexpr std::array kinds{ ValueKind::Undefined, ValueKind::None, ValueKind::Bool, ValueKind::Int, ValueKind::Double,
                                           ValueKind::String, ValueKind::String, ValueKind::String, ValueKind::String, ValueKind::String,
                                           ValueKind::String, ValueKind::HostValue, ValueKind::List, ValueKind::Map, ValueKind::KeyValuePair,
                                           ValueKind::Callable, ValueKind::Renderer };
        static_assert(kinds.size() == std::variant_size_v<detail::ValueStorage>);
        const auto index = m_imm.index();
        return index < kinds.size() ? kinds[index] : ValueKind::Undefined;
    }

    [[nodiscard]] bool IsUndefined() const noexcept { return m_imm.index() == detail::IndexOf<UndefinedValue>; }
    [[nodiscard]] bool IsNone() const noexcept { return m_imm.index() == detail::IndexOf<NoneValue>; }

    [[nodiscard]] bool IsEqual(const InternalValue& other) const;

    //! Python's Markup: a string already safe for HTML output. Autoescape leaves it as is.
    //! Only strings are Markup: marking any other value does nothing
    [[nodiscard]] bool IsMarkup() const noexcept
    {
        const auto* meta = StringMeta(m_imm);
        return meta != nullptr && (*meta & detail::MarkupFlag) != 0;
    }
    InternalValue& SetMarkup(bool isMarkup = true) noexcept
    {
        if (auto* meta = StringMeta(m_imm))
        {
            *meta = isMarkup ? static_cast<uint8_t>(*meta | detail::MarkupFlag) : static_cast<uint8_t>(*meta & ~detail::MarkupFlag);
        }
        return *this;
    }

private:
    // The meta byte of a string alternative, null for any other
    template<typename Storage, typename Meta = std::conditional_t<std::is_const_v<Storage>, const uint8_t, uint8_t>>
    static Meta* StringMeta(Storage& imm) noexcept
    {
        switch (imm.index())
        {
        case detail::IndexOf<detail::SmallString<char>>:
            return static_cast<Meta*>(&std::get_if<detail::SmallString<char>>(&imm)->meta);
        case detail::IndexOf<detail::SmallString<wchar_t>>:
            return static_cast<Meta*>(&std::get_if<detail::SmallString<wchar_t>>(&imm)->meta);
        case detail::IndexOf<detail::HeapString<char>>:
            return static_cast<Meta*>(&std::get_if<detail::HeapString<char>>(&imm)->meta);
        case detail::IndexOf<detail::HeapString<wchar_t>>:
            return static_cast<Meta*>(&std::get_if<detail::HeapString<wchar_t>>(&imm)->meta);
        case detail::IndexOf<detail::StringView<char>>:
            return static_cast<Meta*>(&std::get_if<detail::StringView<char>>(&imm)->meta);
        case detail::IndexOf<detail::StringView<wchar_t>>:
            return static_cast<Meta*>(&std::get_if<detail::StringView<wchar_t>>(&imm)->meta);
        default:
            return static_cast<Meta*>(nullptr);
        }
    }

    // A string is copied into the value (or its object) whether it is moved in or not
    template<typename CharT>
    void InitString(std::basic_string_view<CharT> str)
    {
        if (str.size() <= detail::SmallString<CharT>::Capacity)
        {
            InitSmall(str);
            return;
        }
        InitHeapString(str);
    }

    template<typename CharT>
    void InitSmall(std::basic_string_view<CharT> str) noexcept
    {
        detail::SmallString<CharT> small{};
        std::copy(str.begin(), str.end(), small.chars.begin());
        small.meta = static_cast<uint8_t>(str.size());
        m_imm = small;
    }

    template<typename CharT>
    void InitView(std::basic_string_view<CharT> str)
    {
        if (str.size() > std::numeric_limits<uint32_t>::max())
        {
            InitHeapString(str);
            return;
        }
        m_imm = detail::StringView<CharT>{ str.data(), static_cast<uint32_t>(str.size()), {}, 0 };
    }

    // Out of line: the allocation would otherwise be inlined into every function that makes a string
    template<typename CharT>
    JINJA2CPP_OUTLINE_INLINE void InitHeapString(std::basic_string_view<CharT> str)
    {
        InitHeapString<CharT>(str.size(), [str](boost::span<CharT> chars) { std::copy(str.begin(), str.end(), chars.begin()); });
    }

    template<typename CharT, typename Fn>
    JINJA2CPP_OUTLINE_INLINE void InitHeapString(size_t size, const Fn& write)
    {
        auto obj = detail::StringObject<CharT>::Make(size);
        write(obj->Chars());
        m_imm = detail::HeapString<CharT>{ obj.get(), {}, 0 };
        m_obj = std::move(obj);
    }

    // Gives this value its own copy of the object (a generator, IListAccessor::Clone).
    // Exported: the inline copy constructor calls it from code outside a shared library
    JINJA2CPP_EXPORT void CloneObject();

    detail::ValueStorage m_imm;
    // Owns what m_imm points to; null for the values that own nothing
    ValueObjectPtr m_obj;

    friend class Slot;
    friend struct detail::InternalValueAccess;
};

static_assert(sizeof(InternalValue) == 32, "docs/tasks/0140");

namespace detail
{
struct InternalValueAccess
{
    static const ValueStorage& Data(const InternalValue& val) noexcept { return val.m_imm; }
    static ValueStorage& Data(InternalValue& val) noexcept { return val.m_imm; }
    static const ValueObject* Object(const InternalValue& val) noexcept { return val.m_obj.get(); }
    static const ValueObjectPtr& ObjectPtr(const InternalValue& val) noexcept { return val.m_obj; }
    static ValueObjectPtr& ObjectPtr(InternalValue& val) noexcept { return val.m_obj; }
};
} // namespace detail

inline bool operator==(const InternalValue& lhs, const InternalValue& rhs)
{
    return lhs.IsEqual(rhs);
}
inline bool operator!=(const InternalValue& lhs, const InternalValue& rhs)
{
    return !(lhs == rhs);
}

JINJA2CPP_SUPPRESS_DLL_INTERFACE_WARNINGS_BEGIN
class JINJA2CPP_EXPORT ListRef::Iterator
    : public boost::iterator_facade<
          Iterator,
          const InternalValue,
          boost::forward_traversal_tag>
{
public:
    Iterator();

    explicit Iterator(std::optional<ListAccessorEnumeratorPtr>&& iter)
        : m_iterator(std::move(iter))
        , m_isFinished(m_iterator ? !(*m_iterator)->MoveNext() : true)
        , m_currentVal(m_isFinished || !m_iterator ? InternalValue() : (*m_iterator)->GetCurrent())
    {}

private:
    friend class boost::iterator_core_access;

    void increment();

    bool equal(const Iterator& other) const;

    const InternalValue& dereference() const
    {
        return m_currentVal;
    }

    std::optional<ListAccessorEnumeratorPtr> m_iterator;
    bool m_isFinished = true;
    mutable uint64_t m_currentIndex = 0;
    mutable InternalValue m_currentVal;
};
JINJA2CPP_SUPPRESS_DLL_INTERFACE_WARNINGS_END

// A variable name with its hash, computed once when the template is parsed, so that a
// lookup through several scopes hashes nothing
struct HashedName
{
    std::string_view name;
    size_t hash;

    static size_t Hash(std::string_view name) noexcept { return robin_hood::hash_bytes(name.data(), name.size()); }
    // The key a scope keeps for the name (ScopeRef)
    explicit operator std::string() const { return std::string(name); }
};

struct NameHash
{
    using is_transparent = void;
    size_t operator()(const std::string& name) const noexcept { return HashedName::Hash(name); }
    size_t operator()(const HashedName& name) const noexcept { return name.hash; }
};

struct NameEqual
{
    using is_transparent = void;
    bool operator()(const std::string& lhs, const std::string& rhs) const noexcept { return lhs == rhs; }
    bool operator()(const std::string& lhs, const HashedName& rhs) const noexcept { return Equal(lhs, rhs.name); }
    bool operator()(const HashedName& lhs, const std::string& rhs) const noexcept { return Equal(lhs.name, rhs); }

    // Variable names are short: up to 16 bytes they compare as two overlapping words
    // instead of a call to memcmp (docs/tasks/0100)
    static bool Equal(std::string_view lhs, std::string_view rhs) noexcept
    {
        const auto size = lhs.size();
        if (size != rhs.size())
        {
            return false;
        }
        if (size >= 8 && size <= 16)
        {
            return Load<uint64_t>(lhs, 0) == Load<uint64_t>(rhs, 0) && Load<uint64_t>(lhs, size - 8) == Load<uint64_t>(rhs, size - 8);
        }
        if (size >= 4 && size < 8)
        {
            return Load<uint32_t>(lhs, 0) == Load<uint32_t>(rhs, 0) && Load<uint32_t>(lhs, size - 4) == Load<uint32_t>(rhs, size - 4);
        }
        if (size < 4)
        {
            // Covers every byte of 1-3 byte names
            return size == 0 || (lhs[0] == rhs[0] && lhs[size / 2] == rhs[size / 2] && lhs[size - 1] == rhs[size - 1]);
        }
        return std::memcmp(lhs.data(), rhs.data(), size) == 0;
    }

private:
    // sizeof(T) bytes of str from offset, which the caller keeps in range
    template<typename T>
    static T Load(std::string_view str, size_t offset) noexcept
    {
        T result;
        std::memcpy(&result, &str[offset], sizeof(T));
        return result;
    }
};

using InternalValueMap = robin_hood::unordered_map<std::string, InternalValue, NameHash, NameEqual>;

MapAdapter CreateMapAdapter(InternalValueMap&& values);
MapAdapter CreateMapAdapter(InternalDict&& values);
MapAdapter CreateMapAdapter(TemplateDict&& values);
// Jinja2's namespace(): a shared mapping whose attributes `set ns.attr = ...` assigns
MapAdapter CreateNamespaceAdapter(InternalDict&& values);
MapAdapter CreateNamespaceAdapter(TemplateDict&& values);
// An object of the Python class `className` (a string literal) whose attributes are `values`:
// cycler() and joiner()
MapAdapter CreateObjectAdapter(InternalValueMap&& values, const char* className);
// An object of `className` that prints as <className repr(reprArgument)>
MapAdapter CreateObjectAdapter(InternalValueMap&& values, const char* className, InternalValue reprArgument);
MapAdapter CreateMapAdapter(const InternalValueMap* values);
// Shares the map: the adapter keeps it alive (a loop object kept past its loop)
MapAdapter CreateMapAdapter(std::shared_ptr<InternalValueMap> values);
MapAdapter CreateMapAdapter(const GenericMap& values);
MapAdapter CreateMapAdapter(GenericMap&& values);
MapAdapter CreateMapAdapter(const ValuesMap& values);
MapAdapter CreateMapAdapter(ValuesMap&& values);

// The scalar, host value reference or held object of type T in val, if it holds one
template<typename T>
auto GetIf(const InternalValue* val)
{
    const auto& data = detail::InternalValueAccess::Data(*val);
    if constexpr (detail::IsBoxed<T>)
    {
        const auto* box = std::get_if<detail::BoxHandle<T>>(&data);
        return box ? box->value : static_cast<const T*>(nullptr);
    }
    else
    {
        return std::get_if<T>(&data);
    }
}

template<typename T>
auto GetIf(InternalValue* val)
{
    if constexpr (detail::IsBoxed<T>)
    {
        return GetIf<T>(static_cast<const InternalValue*>(val));
    }
    else
    {
        return std::get_if<T>(&detail::InternalValueAccess::Data(*val));
    }
}

inline InternalValue ListRef::GetValueByIndex(int64_t idx) const
{
    if (m_accessor)
    {
        if (auto val = m_accessor->GetItem(idx))
        {
            return std::move(*val);
        }
    }
    return InternalValue();
}

inline InternalValue ListAdapter::GetValueByIndex(int64_t idx) const
{
    return Ref().GetValueByIndex(idx);
}

inline InternalValue MapRef::GetValueByName(const std::string& name) const
{
    return m_accessor ? m_accessor->GetItem(name) : InternalValue();
}

inline InternalValue MapAdapter::GetValueByName(const std::string& name) const
{
    return Ref().GetValueByName(name);
}

inline std::optional<ListAccessorEnumeratorPtr> ListRef::GetEnumerator() const
{
    return m_accessor ? m_accessor->CreateListAccessorEnumerator() : std::optional<ListAccessorEnumeratorPtr>();
}
inline void ListRef::RebindEnumerator(std::optional<ListAccessorEnumeratorPtr>& enumerator) const
{
    if (enumerator && m_accessor && (*enumerator)->Rebind(m_accessor))
    {
        return;
    }
    enumerator = GetEnumerator();
}
inline ListRef::Iterator ListRef::begin() const
{
    return m_accessor ? Iterator(m_accessor->CreateListAccessorEnumerator()) : Iterator();
}
inline ListRef::Iterator ListRef::end() const { return Iterator(); } // NOLINT(readability-convert-member-functions-to-static): container API
inline InternalValueList ListAdapter::ToValueList() const
{
    return Ref().ToValueList();
}

inline ListAdapter::Iterator ListAdapter::begin() const
{
    return Ref().begin();
}
inline ListAdapter::Iterator ListAdapter::end() const { return Iterator(); } // NOLINT(readability-convert-member-functions-to-static): container API

inline ListAdapter ListRef::ToAdapter() const
{
    // Through the copy constructor, which clones a generator
    const ListAdapter shared(m_accessor, ValueObjectPtr(m_owner), m_isTuple);
    return ListAdapter(shared);
}

inline MapAdapter MapRef::ToAdapter() const
{
    return MapAdapter(m_accessor, ValueObjectPtr(m_owner));
}

struct KeyValuePair
{
    InternalValue key;
    InternalValue value;
};

inline InternalValue::InternalValue(KeyValuePair&& val)
    : InternalValue(RecursiveWrapper<KeyValuePair>(std::move(val)))
{
}

inline InternalValue::InternalValue(const KeyValuePair& val)
    : InternalValue(RecursiveWrapper<KeyValuePair>(val))
{
}

inline std::vector<KeyValuePair> MapRef::GetEntries() const
{
    return m_accessor ? m_accessor->GetEntries() : std::vector<KeyValuePair>();
}

inline std::vector<KeyValuePair> MapAdapter::GetEntries() const
{
    return Ref().GetEntries();
}

inline InternalValueList MapRef::GetKeyValues() const
{
    return m_accessor ? m_accessor->GetKeyValues() : InternalValueList();
}

inline InternalValueList MapAdapter::GetKeyValues() const
{
    return Ref().GetKeyValues();
}

inline std::optional<InternalValue> MapRef::FindItem(const InternalValue& key) const
{
    return m_accessor ? m_accessor->FindItem(key) : std::nullopt;
}

inline std::optional<InternalValue> MapAdapter::FindItem(const InternalValue& key) const
{
    return Ref().FindItem(key);
}

// The list or map a value holds, if it holds one: `if (auto list = AsList(val)) list->GetSize()`.
// Not for temporaries, which the view would outlive
inline std::optional<ListRef> AsList(const InternalValue& val)
{
    if (const auto* list = std::get_if<detail::ListHandle>(&detail::InternalValueAccess::Data(val)))
    {
        return ListRef(list->accessor, detail::InternalValueAccess::Object(val), list->isTuple);
    }
    return std::nullopt;
}
std::optional<ListRef> AsList(const InternalValue&&) = delete;

inline std::optional<MapRef> AsMap(const InternalValue& val)
{
    if (const auto* map = std::get_if<detail::MapHandle>(&detail::InternalValueAccess::Data(val)))
    {
        return MapRef(map->accessor, detail::InternalValueAccess::Object(val));
    }
    return std::nullopt;
}
std::optional<MapRef> AsMap(const InternalValue&&) = delete;

// The text of a string value of character type CharT, stored or viewed. None for any other
// value, a string of the other character type and a reference to a host string included.
// A short string is stored in the value itself: the view lives no longer than the value
template<typename CharT>
std::optional<std::basic_string_view<CharT>> AsStringView(const InternalValue& val)
{
    const auto& data = detail::InternalValueAccess::Data(val);
    if (const auto* str = std::get_if<detail::SmallString<CharT>>(&data))
    {
        return str->View();
    }
    if (const auto* str = std::get_if<detail::HeapString<CharT>>(&data))
    {
        return str->object->View();
    }
    if (const auto* str = std::get_if<detail::StringView<CharT>>(&data))
    {
        return str->View();
    }
    return std::nullopt;
}
template<typename CharT>
std::optional<std::basic_string_view<CharT>> AsStringView(const InternalValue&&) = delete;

// The text of a string value as a string of its own, for code that edits it (a value keeps its
// characters in its object, so they are always copied). None for any other value
std::optional<TargetString> TakeString(InternalValue&& val);

// A string value of either character type, Markup included; not a reference to a host string
inline bool IsStringValue(const InternalValue& val)
{
    return val.Kind() == ValueKind::String;
}

// A string value that views text it does not own (an item of a host list or map read in place)
inline bool IsStringView(const InternalValue& val)
{
    const auto index = detail::InternalValueAccess::Data(val).index();
    return index == detail::IndexOf<detail::StringView<char>> || index == detail::IndexOf<detail::StringView<wchar_t>>;
}


class Callable
{
public:
    enum Kind
    {
        GlobalFunc,
        SpecialFunc,
        Macro,
        UserCallable
    };
    using ExpressionCallable = std::function<InternalValue(const CallParams&, RenderContext&)>;
    using StatementCallable = std::function<void(const CallParams&, OutStream&, RenderContext&)>;

    using CallableHolder = std::variant<ExpressionCallable, StatementCallable>;

    enum class Type
    {
        Expression,
        Statement
    };

    // The function is shared by the copies: a callable is copied each time its name is looked
    // up for a call, and copying a std::function copies everything it captured
    Callable(Kind kind, ExpressionCallable&& callable)
        : m_kind(kind)
        , m_callable(std::make_shared<const CallableHolder>(std::move(callable)))
    {
    }

    Callable(Kind kind, StatementCallable&& callable)
        : m_kind(kind)
        , m_callable(std::make_shared<const CallableHolder>(std::move(callable)))
    {
    }

    [[nodiscard]] auto GetType() const
    {
        return m_callable->index() == 0 ? Type::Expression : Type::Statement;
    }

    [[nodiscard]] auto GetKind() const
    {
        return m_kind;
    }

    [[nodiscard]] const CallableHolder& GetCallable() const
    {
        return *m_callable;
    }

    [[nodiscard]] auto& GetExpressionCallable() const
    {
        return std::get<ExpressionCallable>(*m_callable);
    }

    [[nodiscard]] auto& GetStatementCallable() const
    {
        return std::get<StatementCallable>(*m_callable);
    }

    // Attributes visible through `callable.name` (macro.name, macro.arguments, ...)
    void SetAttributes(std::shared_ptr<const InternalValueMap> attributes)
    {
        m_attributes = std::move(attributes);
    }

    [[nodiscard]] const std::shared_ptr<const InternalValueMap>& GetAttributes() const
    {
        return m_attributes;
    }

    // Python compares functions and macros by identity: the copies of one callable share its
    // function, and a user callable converted from the same jinja2::UserCallable is the same
    [[nodiscard]] bool IsSame(const Callable& other) const;

private:
    Kind m_kind;
    std::shared_ptr<const CallableHolder> m_callable;
    std::shared_ptr<const InternalValueMap> m_attributes;
};


inline InternalValue::InternalValue(Callable&& val)
    : InternalValue(RecursiveWrapper<Callable>(std::move(val)))
{
}

inline InternalValue::InternalValue(const Callable& val)
    : InternalValue(RecursiveWrapper<Callable>(val))
{
}

inline bool IsEmpty(const InternalValue& val)
{
    return val.IsUndefined() || val.IsNone();
}

class RenderContext;

template<typename Fn>
auto MakeDynamicProperty(Fn&& fn)
{
    return CreateMapAdapter(InternalValueMap{
        { "value()", Callable(Callable::GlobalFunc, std::forward<Fn>(fn)) } });
}

// A "character" of a template string is a Unicode code point, as in Python: narrow strings
// are read as UTF-8, wide ones as UTF-16 or UTF-32 depending on the size of wchar_t. Malformed
// input never fails: a stray continuation unit stays with the code point before it.
inline bool IsCodePointTail(char ch)
{
    return (static_cast<unsigned char>(ch) & 0xC0) == 0x80;
}

// A code unit as a number, without sign extension: char is signed on most platforms, and
// wchar_t is signed on Linux and unsigned on Windows
inline uint32_t CodeUnit(char ch)
{
    return static_cast<unsigned char>(ch);
}

inline uint32_t CodeUnit(wchar_t ch)
{
    return static_cast<uint32_t>(static_cast<std::make_unsigned_t<wchar_t>>(ch));
}

inline bool IsCodePointTail(wchar_t ch)
{
    if constexpr (sizeof(wchar_t) != 2)
    {
        return false;
    }
    const auto unit = CodeUnit(ch);
    return unit >= 0xDC00 && unit <= 0xDFFF;
}

template<typename CharT>
size_t CodePointCount(std::basic_string_view<CharT> str)
{
    // A leading continuation unit still starts a character (see SplitCodePoints)
    auto starts = std::count_if(str.begin(), str.end(), [](CharT ch) { return !IsCodePointTail(ch); });
    return static_cast<size_t>(starts) + (!str.empty() && IsCodePointTail(str[0]) ? 1 : 0);
}

// The offset of the character after the one starting at pos (pos < str.size())
template<typename CharT>
size_t NextCodePoint(std::basic_string_view<CharT> str, size_t pos)
{
    ++pos;
    while (pos < str.size() && IsCodePointTail(str[pos]))
    {
        ++pos;
    }
    return pos;
}

// Calls fn with each character of str in order, as SplitCodePoints splits them
template<typename CharT, typename Fn>
void ForEachCodePoint(std::basic_string_view<CharT> str, const Fn& fn)
{
    for (size_t start = 0; start != str.size();)
    {
        const auto end = NextCodePoint(str, start);
        fn(str.substr(start, end - start));
        start = end;
    }
}

template<typename CharT>
std::vector<std::basic_string_view<CharT>> SplitCodePoints(std::basic_string_view<CharT> str)
{
    std::vector<std::basic_string_view<CharT>> result;
    ForEachCodePoint(str, [&result](auto ch) { result.push_back(ch); });
    return result;
}

InternalValue Subscript(const InternalValue& val, const InternalValue& subscript, RenderContext* values);
InternalValue Subscript(const InternalValue& val, const std::string& subscript, RenderContext* values);
// Python's val[start:stop:step] on lists and strings; an empty start, stop or step is omitted
InternalValue Slice(const InternalValue& val, const InternalValue& start, const InternalValue& stop, const InternalValue& step);
std::string AsString(const InternalValue& val);
ListAdapter ConvertToList(const InternalValue& val, bool& isConverted, bool strictConversion = true);

// Containers of values (lists, call arguments, slots) move them when they grow only if the
// move cannot throw; otherwise they copy every item (docs/tasks/0140)
static_assert(std::is_nothrow_move_constructible_v<InternalValue>);
static_assert(std::is_nothrow_move_assignable_v<InternalValue>);
Value IntValue2Value(const InternalValue& val);
Value OptIntValue2Value(std::optional<InternalValue> val);

} // namespace jinja2

#endif // JINJA2CPP_SRC_INTERNAL_VALUE_H
