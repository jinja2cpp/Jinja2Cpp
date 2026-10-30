#include "host_object.h"

#include "engine_errors.h"
#include "expression_evaluator.h"
#include "internal_value.h"
#include "intrusive_ptr.h"

#include <jinja2cpp/string_helpers.h>
#include <jinja2cpp/value.h>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace jinja2
{
inline namespace v2
{
namespace
{
// The public view of a host object handed to the host's API (a GenericMap): its attributes by
// name, as the template reads them
class HostObjectView final : public IMapItemAccessor
{
public:
    explicit HostObjectView(IntrusivePtr<const HostObjectAccessor> accessor) noexcept
        : m_accessor(std::move(accessor))
    {
    }

    [[nodiscard]] size_t GetSize() const override { return m_accessor->GetSize(); }
    [[nodiscard]] bool HasValue(const std::string& name) const override { return m_accessor->HasValue(name); }
    [[nodiscard]] Value GetValueByName(const std::string& name) const override { return IntValue2Value(m_accessor->GetItem(name)); }
    [[nodiscard]] std::vector<std::string> GetKeys() const override { return {}; }
    [[nodiscard]] bool IsEqual(const IComparable& other) const override
    {
        const auto* view = dynamic_cast<const HostObjectView*>(&other);
        return view != nullptr && view->m_accessor->GetIdentity() == m_accessor->GetIdentity();
    }

private:
    IntrusivePtr<const HostObjectAccessor> m_accessor;
};

// x.name, as Jinja2's Environment.getattr: the attribute, else the item under the name
std::optional<InternalValue> AttrThenItem(const IHostObject& host, const std::string& name)
{
    if (auto attr = host.GetAttr(name))
    {
        return attr;
    }
    return host.GetItem(InternalValue(name));
}
} // namespace

InternalValue IHostObject::Call(const CallParams& /*params*/, RenderContext& /*context*/) const
{
    throw TemplateRuntimeError(std::string("'") + TypeName() + "' object is not callable");
}

bool IHostObject::Truth() const
{
    const auto length = Len();
    return !length || *length != 0;
}

std::optional<size_t> IHostObject::Hash() const
{
    return std::hash<const void*>()(Identity());
}


std::vector<std::string> HostObjectAccessor::GetKeys() const
{
    return {};
}

std::vector<KeyValuePair> HostObjectAccessor::GetEntries() const
{
    return {};
}

InternalValueList HostObjectAccessor::GetKeyValues() const
{
    return {};
}

bool HostObjectAccessor::ShouldExtendLifetime() const
{
    return false;
}

bool HostObjectAccessor::HasAttributes() const
{
    return true;
}

const void* HostObjectAccessor::GetIdentity() const
{
    return GetHostObject()->Identity();
}

MapAttrPolicy HostObjectAccessor::GetAttrPolicy() const
{
    return MapAttrPolicy::Host;
}

const char* HostObjectAccessor::GetObjectClass() const
{
    return GetHostObject()->TypeName();
}

bool HostObjectAccessor::Iterates() const
{
    return GetHostObject()->IsIterable();
}

size_t HostObjectAccessor::GetSize() const
{
    return GetHostObject()->Len().value_or(0);
}

bool HostObjectAccessor::HasValue(const std::string& name) const
{
    return AttrThenItem(*GetHostObject(), name).has_value();
}

InternalValue HostObjectAccessor::GetItem(const std::string& name) const
{
    auto value = AttrThenItem(*GetHostObject(), name);
    return value ? std::move(*value) : InternalValue();
}

// x[key], as Jinja2's Environment.getitem: the item, else the attribute named by a string key
std::optional<InternalValue> HostObjectAccessor::FindItem(const InternalValue& key) const
{
    const auto* host = GetHostObject();
    if (auto item = host->GetItem(key))
    {
        return item;
    }
    if (IsStringValue(key))
    {
        return host->GetAttr(AsString(key));
    }
    return std::nullopt;
}

GenericMap HostObjectAccessor::CreateGenericMap() const
{
    return GenericMap(std::make_shared<const HostObjectView>(IntrusivePtr<const HostObjectAccessor>(this)));
}

bool HostObjectAccessor::GetIteration(ListAdapter& items) const
{
    auto iteration = GetHostObject()->Iterate();
    if (!iteration)
    {
        return false;
    }
    items = std::move(*iteration);
    return true;
}

namespace host
{
Callable MakeCall(const InternalValue& object, const IHostObject& host)
{
    if (!host.IsCallable())
    {
        throw TemplateRuntimeError(std::string("'") + host.TypeName() + "' object is not callable");
    }
    // The callable holds the value, which keeps the object alive while the callable lives
    return Callable(Callable::GlobalFunc, [object](const CallParams& params, RenderContext& context) { return GetHostObject(object)->Call(params, context); });
}

InternalValue Html(const IHostObject& host, bool wideTarget)
{
    const auto html = host.Html();
    auto result = wideTarget ? InternalValue(ConvertString<std::wstring>(html)) : InternalValue(html);
    result.SetMarkup();
    return result;
}

int64_t Len(const IHostObject& host)
{
    const auto length = host.Len();
    if (!length)
    {
        throw TemplateRuntimeError(std::string("object of type '") + host.TypeName() + "' has no len()");
    }
    return static_cast<int64_t>(*length);
}

bool Equal(const IHostObject* left, const InternalValue& leftValue, const IHostObject* right, const InternalValue& rightValue)
{
    if (left)
    {
        if (auto result = left->Equals(rightValue))
        {
            return *result;
        }
    }
    if (right)
    {
        if (auto result = right->Equals(leftValue))
        {
            return *result;
        }
    }
    // Python's default __eq__: the same object
    return left != nullptr && right != nullptr && left->Identity() == right->Identity();
}
} // namespace host

} // namespace v2
} // namespace jinja2
