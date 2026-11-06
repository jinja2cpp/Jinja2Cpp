#include "public_host_object.h"

#include "engine_errors.h"
#include "host_object.h"
#include "internal_value.h"
#include "intrusive_ptr.h"
#include "undefined.h"

#include <jinja2cpp/generic_list_iterator.h>
#include <jinja2cpp/host_object.h>
#include <jinja2cpp/value.h>

#include <atomic>
#include <cstddef>
#include <new>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <typeinfo>
#include <utility>
#include <vector>

namespace jinja2
{
inline namespace v2
{
// The defaults of the public interfaces that the library defines out of line

IHostObject::~IHostObject() = default;

std::optional<Value> IHostObject::GetItem(const Value& /*key*/) const
{
    return std::nullopt;
}

std::optional<Value> IHostObject::GetSlice(const Value& /*start*/, const Value& /*stop*/, const Value& /*step*/) const
{
    return std::nullopt;
}

bool IHostObject::IsCallable() const
{
    return false;
}

Value IHostObject::Call(const UserCallableParams& /*params*/) const
{
    throw TemplateRuntimeError(std::string("'") + TypeName() + "' object is not callable");
}

bool IHostObject::IsIterable() const
{
    return false;
}

std::optional<GenericList> IHostObject::Iterate() const
{
    return std::nullopt;
}

std::optional<GenericList> IHostObject::Reversed() const
{
    return std::nullopt;
}

std::optional<bool> IHostObject::Contains(const Value& /*item*/) const
{
    return std::nullopt;
}

std::optional<size_t> IHostObject::Len() const
{
    return std::nullopt;
}

bool IHostObject::Truth() const
{
    const auto length = Len();
    return !length || *length != 0;
}

std::optional<bool> IHostObject::Equals(const Value& /*other*/) const
{
    return std::nullopt;
}

std::optional<size_t> IHostObject::Hash() const
{
    return std::hash<const void*>()(Identity());
}

const void* IHostObject::Identity() const
{
    return this;
}

bool IHostObject::IsMapping() const
{
    return false;
}

bool IHostObject::IsSequence() const
{
    return false;
}

bool IHostObject::IsTuple() const
{
    return false;
}

bool IHostObject::IsMarkupLike() const
{
    return false;
}

std::string IHostObject::Html() const
{
    return Str();
}

std::vector<std::pair<Value, Value>> IMapItemAccessor::GetItems() const
{
    std::vector<std::pair<Value, Value>> items;
    for (auto& key : GetKeys())
    {
        auto value = Find(key);
        items.emplace_back(Value(key), value ? std::move(*value) : Value());
    }
    return items;
}

std::vector<std::pair<Value, Value>> GenericMap::GetItems() const
{
    return m_accessor ? m_accessor->GetItems() : std::vector<std::pair<Value, Value>>();
}

namespace
{
// The value MakeUndefined makes: an object the engine reads as undefined, where it enters a render
class UndefinedCarrier final : public IHostObject
{
public:
    explicit UndefinedCarrier(UndefinedDescription description)
        : m_description(std::move(description))
    {
    }

    [[nodiscard]] const UndefinedDescription& Description() const noexcept { return m_description; }

    [[nodiscard]] const char* TypeName() const override
    {
        switch (m_description.policy)
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
    [[nodiscard]] std::string Str() const override { return {}; }
    [[nodiscard]] std::string Repr() const override { return TypeName(); }
    [[nodiscard]] std::optional<Value> GetAttr(std::string_view /*name*/) const override { return std::nullopt; }
    [[nodiscard]] bool Truth() const override { return false; }

private:
    UndefinedDescription m_description;
};

// UndefinedCarrier is final, so its type is exact
const UndefinedCarrier* AsCarrier(const IHostObject* object)
{
    return object && typeid(*object) == typeid(UndefinedCarrier) ? static_cast<const UndefinedCarrier*>(object) : nullptr;
}

// The undefined value of the engine that a description stands for. A Default description with no name, object type or
// hint is a plain undefined, which fails no use (what the engine hands out for a name it has no information about)
InternalValue UndefinedFromDescription(const UndefinedDescription& description)
{
    if (description.policy == UndefinedPolicy::Default && description.name.empty() && description.objType.empty() && description.hint.empty())
    {
        return InternalValue(UndefinedValue{});
    }
    auto info = MakeRef<UndefinedInfo>();
    info->policy = description.policy;
    info->name = description.name;
    info->objType = description.objType;
    info->isAttr = description.isAttr;
    info->hint = description.hint;
    return InternalValue(UndefinedValue{ info.get() });
}

// The engine's operations on a public host object (the private interface), with the values converted at each
class EngineBridge final : public IEngineHostObject
{
public:
    // `object` is owned by `owner`, or lives in the bridge's own block when `owner` is empty
    EngineBridge(const IHostObject* object, std::shared_ptr<const IHostObject> owner)
        : m_owner(std::move(owner))
        , m_object(object)
    {
    }

    [[nodiscard]] const IHostObject* Object() const noexcept { return m_object; }
    [[nodiscard]] const std::shared_ptr<const IHostObject>& Owner() const noexcept { return m_owner; }
    // The object's destructor runs in the bridge's, when it lives in the bridge's block
    void DestroyInline() noexcept { m_object->~IHostObject(); }

    [[nodiscard]] const char* TypeName() const override { return m_object->TypeName(); }
    [[nodiscard]] std::string Str() const override { return m_object->Str(); }
    [[nodiscard]] std::string Repr() const override { return m_object->Repr(); }
    [[nodiscard]] std::optional<InternalValue> GetAttr(const std::string& name) const override { return ToEngine(m_object->GetAttr(name)); }
    [[nodiscard]] std::optional<InternalValue> GetItem(const InternalValue& key) const override { return ToEngine(m_object->GetItem(ToHostValue(key))); }
    [[nodiscard]] std::optional<InternalValue> GetItemByName(const std::string& name) const override { return ToEngine(m_object->GetItem(Value(name))); }
    [[nodiscard]] std::optional<InternalValue> GetSlice(const InternalValue& start, const InternalValue& stop, const InternalValue& step) const override
    {
        return ToEngine(m_object->GetSlice(ToHostValue(start), ToHostValue(stop), ToHostValue(step)));
    }
    [[nodiscard]] bool IsCallable() const override { return m_object->IsCallable(); }
    InternalValue Call(const CallParams& params, RenderContext& context) const override
    {
        std::call_once(m_callOnce, [this] { m_call = MakeHostObjectCallable(m_owner ? m_owner : std::shared_ptr<const IHostObject>(std::shared_ptr<void>(), m_object)); }); // the callable lives in the bridge, which owns the object
        const auto& fn = *GetIf<Callable>(&m_call);
        return fn.GetExpressionCallable()(params, context);
    }
    [[nodiscard]] bool IsIterable() const override { return m_object->IsIterable(); }
    [[nodiscard]] std::optional<ListAdapter> Iterate() const override { return ToEngine(m_object->Iterate()); }
    [[nodiscard]] std::optional<ListAdapter> Reversed() const override { return ToEngine(m_object->Reversed()); }
    [[nodiscard]] std::optional<bool> Contains(const InternalValue& item) const override { return m_object->Contains(ToHostValue(item)); }
    [[nodiscard]] std::optional<size_t> Len() const override { return m_object->Len(); }
    [[nodiscard]] bool Truth() const override { return m_object->Truth(); }
    [[nodiscard]] std::optional<bool> Equals(const InternalValue& other) const override { return m_object->Equals(ToHostValue(other)); }
    [[nodiscard]] std::optional<size_t> Hash() const override { return m_object->Hash(); }
    [[nodiscard]] const void* Identity() const override { return m_object->Identity(); }
    [[nodiscard]] bool IsMapping() const override { return m_object->IsMapping(); }
    [[nodiscard]] bool IsSequence() const override { return m_object->IsSequence(); }
    [[nodiscard]] bool IsTuple() const override { return m_object->IsTuple(); }
    [[nodiscard]] bool IsMarkupLike() const override { return m_object->IsMarkupLike(); }
    [[nodiscard]] std::string Html() const override { return m_object->Html(); }

private:
    static std::optional<InternalValue> ToEngine(std::optional<Value> value)
    {
        if (!value)
        {
            return std::nullopt;
        }
        return Value2IntValue(std::move(*value));
    }
    static std::optional<ListAdapter> ToEngine(std::optional<GenericList> list)
    {
        if (!list)
        {
            return std::nullopt;
        }
        return ListAdapter::CreateAdapter(std::move(*list));
    }

    std::shared_ptr<const IHostObject> m_owner;
    const IHostObject* m_object;
    // The call of the object as a callable of the engine, made with the first call
    mutable std::once_flag m_callOnce;
    mutable InternalValue m_call;
};

// The accessor of a public host object, which is the engine's accessor (HostObjectAccessor) and the host's
// (IMapItemAccessor) at once. The engine holds it by intrusive reference, GenericMap by a shared_ptr whose control block
// lives in the bridge (SlotAllocator) and holds one intrusive reference: the same object serves both, and a host
// value made by MakeHostValue<T> is one allocation holding the bridge, the object and the control block
class HostBridge final : public HostObjectAccessor
    , public IMapItemAccessor
{
public:
    HostBridge(const IHostObject* object, std::shared_ptr<const IHostObject> owner, bool inlineObject)
        : m_engine(object, std::move(owner))
        , m_inline(inlineObject)
    {
    }
    HostBridge(const HostBridge&) = delete;
    HostBridge& operator=(const HostBridge&) = delete;
    HostBridge(HostBridge&&) = delete;
    HostBridge& operator=(HostBridge&&) = delete;
    ~HostBridge() override
    {
        if (m_inline)
        {
            m_engine.DestroyInline();
        }
    }

    // The bridge and the object after it are one block
    static void operator delete(void* block) noexcept { ::operator delete(block); }

    [[nodiscard]] const IHostObject& Object() const noexcept { return *m_engine.Object(); }
    // The description when the object is MakeUndefined's, else null
    [[nodiscard]] const UndefinedDescription* Undefined() const noexcept { return m_undefined; }
    void SetUndefined(const UndefinedDescription* description) noexcept { m_undefined = description; }

    // The GenericMap that holds `bridge`
    static GenericMap Share(IntrusivePtr<const HostBridge> bridge) { return GenericMap(bridge->MakeShared()); }

    // The engine's accessor
    [[nodiscard]] const IEngineHostObject* GetHostObject() const override { return &m_engine; }
    [[nodiscard]] GenericMap CreateGenericMap() const override { return GenericMap(MakeShared()); }

    // Both interfaces: the object read as a template reads it
    [[nodiscard]] size_t GetSize() const override { return HostObjectAccessor::GetSize(); }
    [[nodiscard]] bool HasValue(const std::string& name) const override
    {
        const auto& object = Object();
        return object.GetAttr(name).has_value() || object.GetItem(Value(name)).has_value();
    }
    [[nodiscard]] std::vector<std::string> GetKeys() const override { return HostObjectAccessor::GetKeys(); }
    [[nodiscard]] const void* GetIdentity() const override { return Object().Identity(); }

    // The host's accessor
    [[nodiscard]] std::optional<Value> Find(std::string_view name) const override
    {
        const auto& object = Object();
        if (auto attr = object.GetAttr(name))
        {
            return attr;
        }
        return object.GetItem(Value(std::string(name)));
    }
    [[nodiscard]] Value GetValueByName(const std::string& name) const override
    {
        auto value = Find(name);
        return value ? std::move(*value) : Value();
    }
    [[nodiscard]] std::vector<std::pair<Value, Value>> GetItems() const override
    {
        std::vector<std::pair<Value, Value>> items;
        const auto& object = Object();
        if (!object.IsMapping())
        {
            return items;
        }
        if (auto keys = object.Iterate())
        {
            for (auto it = keys->begin(); it != keys->end(); ++it)
            {
                Value key = *it;
                auto value = object.GetItem(key);
                items.emplace_back(std::move(key), value ? std::move(*value) : Value());
            }
        }
        return items;
    }
    [[nodiscard]] const IHostObject* AsHostObject() const noexcept override { return m_engine.Object(); }
    [[nodiscard]] bool IsEqual(const IComparable& other) const override
    {
        const auto* view = dynamic_cast<const HostBridge*>(&other); // IComparable is a virtual base; == on maps is no hot path
        return view != nullptr && view->GetIdentity() == GetIdentity();
    }

private:
    // The allocator of the shared_ptr's control block: the bridge's own slot while it is free, else the heap. Each block
    // holds one intrusive reference, released after the control block is gone, so the bridge outlives it; the deleter
    // does nothing
    template<typename U>
    struct SlotAllocator
    {
        using value_type = U;
        explicit SlotAllocator(const HostBridge* owner) noexcept
            : bridge(owner)
        {
        }
        template<typename V>
        SlotAllocator(const SlotAllocator<V>& other) noexcept // NOLINT(google-explicit-constructor): rebinding
            : bridge(other.bridge)
        {
        }
        U* allocate(size_t count)
        {
            intrusive_ptr_add_ref(bridge);
            if (sizeof(U) * count <= sizeof(HostBridge::m_slot) && alignof(U) <= alignof(std::max_align_t) && !bridge->m_slotBusy.exchange(true, std::memory_order_acquire))
            {
                return reinterpret_cast<U*>(bridge->m_slot);
            }
            try
            {
                return static_cast<U*>(::operator new(sizeof(U) * count));
            }
            catch (...)
            {
                intrusive_ptr_release(bridge);
                throw;
            }
        }
        void deallocate(U* block, size_t /*count*/) noexcept
        {
            const HostBridge* owner = bridge; // the bridge may go with its reference, below
            if (reinterpret_cast<unsigned char*>(block) == owner->m_slot)
            {
                owner->m_slotBusy.store(false, std::memory_order_release);
            }
            else
            {
                ::operator delete(block);
            }
            intrusive_ptr_release(owner);
        }
        template<typename V>
        bool operator==(const SlotAllocator<V>& other) const noexcept
        {
            return bridge == other.bridge;
        }
        template<typename V>
        bool operator!=(const SlotAllocator<V>& other) const noexcept
        {
            return bridge != other.bridge;
        }

        const HostBridge* bridge;
    };
    struct NoDeleter
    {
        void operator()(const IMapItemAccessor*) const noexcept {}
    };

    std::shared_ptr<const IMapItemAccessor> MakeShared() const
    {
        return std::shared_ptr<const IMapItemAccessor>(static_cast<const IMapItemAccessor*>(this), NoDeleter{}, SlotAllocator<char>(this));
    }

    EngineBridge m_engine;
    const UndefinedDescription* m_undefined = nullptr;
    bool m_inline;
    mutable std::atomic<bool> m_slotBusy{ false };
    alignas(std::max_align_t) mutable unsigned char m_slot[48]{};
};

// A bridge for an object `construct` places in the same block (the object is not owned elsewhere)
IntrusivePtr<HostBridge> NewInlineBridge(size_t size, size_t align, const IHostObject* (*construct)(void* where, void* context), void* context)
{
    constexpr size_t Header = (sizeof(HostBridge) + alignof(std::max_align_t) - 1) / alignof(std::max_align_t) * alignof(std::max_align_t);
    const size_t offset = (Header + align - 1) / align * align;
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory): owned by the bridge below
    void* block = ::operator new(offset + size);
    const IHostObject* object = nullptr;
    try
    {
        object = construct(static_cast<char*>(block) + offset, context);
    }
    catch (...)
    {
        ::operator delete(block);
        throw;
    }
    // The bridge's constructor and the intrusive pointer cannot throw
    return IntrusivePtr<HostBridge>(::new (block) HostBridge(object, nullptr, true));
}
} // namespace

Value MakeHostValue(std::shared_ptr<const IHostObject> object)
{
    if (!object)
    {
        throw std::invalid_argument("jinja2::MakeHostValue: the object is null");
    }
    const IHostObject* raw = object.get();
    return Value(HostBridge::Share(MakeRef<const HostBridge>(raw, std::move(object), false)));
}

namespace detail
{
Value MakeHostValueInPlace(size_t size, size_t align, const IHostObject* (*construct)(void* where, void* context), void* context)
{
    return Value(HostBridge::Share(NewInlineBridge(size, align, construct, context)));
}
} // namespace detail

Value MakeUndefined(UndefinedDescription description)
{
    auto* pack = &description;
    auto bridge = NewInlineBridge(
        sizeof(UndefinedCarrier), alignof(UndefinedCarrier),
        [](void* where, void* context) -> const IHostObject* { return ::new (where) UndefinedCarrier(std::move(*static_cast<UndefinedDescription*>(context))); }, pack);
    bridge->SetUndefined(&static_cast<const UndefinedCarrier&>(bridge->Object()).Description());
    return Value(HostBridge::Share(std::move(bridge)));
}

std::optional<UndefinedDescription> UndefinedOf(const Value& value)
{
    const auto* map = value.GetIf<GenericMap>();
    const auto* accessor = map ? map->GetAccessor() : nullptr;
    if (const auto* carrier = accessor ? AsCarrier(accessor->AsHostObject()) : nullptr)
    {
        return carrier->Description();
    }
    return std::nullopt;
}

bool IsSelfOwningHostMap(const IMapItemAccessor& accessor)
{
    return typeid(accessor) == typeid(HostBridge);
}

InternalValue HostMapToInternal(const GenericMap& map)
{
    const auto* accessor = map.GetAccessor();
    if (accessor && typeid(*accessor) == typeid(HostBridge))
    {
        const auto* bridge = static_cast<const HostBridge*>(accessor);
        if (const auto* undefined = bridge->Undefined())
        {
            return UndefinedFromDescription(*undefined);
        }
        return InternalValue(MapAdapter(IntrusivePtr<const HostBridge>(bridge)).MarkAsHost());
    }
    // An accessor of the host's own that carries an object
    if (const auto* carrier = accessor ? AsCarrier(accessor->AsHostObject()) : nullptr)
    {
        return UndefinedFromDescription(carrier->Description());
    }
    return InternalValue(MapAdapter(MakeRef<const HostBridge>(map.AsHostObject().get(), map.AsHostObject(), false)).MarkAsHost());
}

Value UndefinedToHost(const UndefinedValue& undefined)
{
    UndefinedDescription description;
    if (const auto* info = undefined.info)
    {
        description.policy = info->policy;
        description.name = info->name;
        description.objType = info->objType;
        description.isAttr = info->isAttr;
        description.hint = info->hint;
    }
    return MakeUndefined(std::move(description));
}

Value ToHostValue(const InternalValue& value)
{
    if (const auto* undefined = GetIf<UndefinedValue>(&value))
    {
        return UndefinedToHost(*undefined);
    }
    return IntValue2Value(value);
}
} // namespace v2
} // namespace jinja2
