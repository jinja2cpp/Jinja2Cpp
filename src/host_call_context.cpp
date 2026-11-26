#include "host_call_context.h"

#include "error_boundary.h"
#include "internal_value.h"
#include "render_context.h"
#include "render_failure.h"
#include "shared_owner.h"
#include "value_visitors.h"

#include <jinja2cpp/error_info.h>
#include <jinja2cpp/template.h>

#include <cstddef>
#include <memory>
#include <new>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace jinja2
{
inline namespace v2
{
namespace host_call
{
namespace
{
class ContextMapper : public IMapItemAccessor
{
public:
    ContextMapper(RenderContext* context, void* scope, LendFn lend)
        : m_context(context)
        , m_scope(scope)
        , m_lend(lend)
    {
    }

    // Given to a new call on this thread, once nothing but the caller holds it (MakeContextMap)
    void Renew(RenderContext* context, void* scope, LendFn lend) const noexcept
    {
        m_context = context;
        m_scope = scope;
        m_lend = lend;
    }

    // The number of names GetKeys lists
    [[nodiscard]] size_t GetSize() const override { return Live().CollectLookupNames().size(); }
    [[nodiscard]] bool HasValue(const std::string& name) const override
    {
        return static_cast<bool>(Live().FindValue(name));
    }
    [[nodiscard]] Value GetValueByName(const std::string& name) const override
    {
        const auto value = Live().FindValue(name);
        return value ? ToHost(*value) : Value();
    }
    // A value the template sees, as the call's arguments are given: a macro, `range` or `loop` is lent for the call
    [[nodiscard]] std::optional<Value> Find(std::string_view name) const override
    {
        const auto value = Live().FindValue(std::string(name));
        if (!value)
        {
            return std::nullopt;
        }
        return ToHost(*value);
    }
    // The names a lookup finds, each once, sorted: the scopes, the external and global values and the built-ins
    [[nodiscard]] std::vector<std::string> GetKeys() const override { return Live().CollectLookupNames(); }

    // A view of the names as Python Jinja2's Context splits them (ContextView, docs/tasks/0416)
    [[nodiscard]] std::optional<Value> FindPart(ContextNames part, std::string_view name) const
    {
        const auto& context = Live();
        const std::string key(name);
        const auto* value = part == ContextNames::TopLevel ? context.FindTopLevelName(key) : context.FindParentName(key);
        if (!value)
        {
            return std::nullopt;
        }
        return ToHost(*value);
    }
    [[nodiscard]] std::vector<std::string> KeysOfPart(ContextNames part) const
    {
        const auto& context = Live();
        return part == ContextNames::TopLevel ? context.CollectTopLevelNames() : context.CollectParentNames();
    }

    // The host's call this context was given to ended (UserCallableBody): its render context and scope are gone, so a
    // copy the host kept fails with TemplateExpired, as a lent macro does
    void Expire() const noexcept
    {
        m_context = nullptr;
        m_scope = nullptr;
    }

    [[nodiscard]] bool IsEqual(const IComparable& other) const override
    {
        const auto* val = dynamic_cast<const ContextMapper*>(&other);
        if (!val)
        {
            return false;
        }
        if (m_context && val->m_context && !m_context->IsEqual(*val->m_context))
        {
            return false;
        }
        if ((m_context && !val->m_context) || (!m_context && val->m_context))
        {
            return false;
        }
        return true;
    }

private:
    [[nodiscard]] RenderContext& Live() const
    {
        if (!m_context)
        {
            ErrorInfo::Data errorData;
            errorData.code = ErrorCode::TemplateExpired;
            errorData.srcLoc.line = 1;
            errorData.srcLoc.col = 1;
            errorData.extraParams.emplace_back(std::string("lent"));
            throw ErrorInfo(std::move(errorData));
        }
        return *m_context;
    }
    [[nodiscard]] Value ToHost(const InternalValue& value) const { return m_lend && m_scope ? m_lend(value, *m_context, m_scope) : IntValue2Value(value); }

    mutable RenderContext* m_context;
    mutable void* m_scope;
    mutable LendFn m_lend;
};

// A view of a call's context that shows some of its names (ContextView): it expires with the context it views, which it
// keeps, and which then is never given to another call (MakeContextMap)
class ContextPartMapper final : public IMapItemAccessor
{
public:
    ContextPartMapper(std::shared_ptr<const ContextMapper> source, ContextNames part)
        : m_source(std::move(source))
        , m_part(part)
    {
    }

    [[nodiscard]] size_t GetSize() const override { return m_source->KeysOfPart(m_part).size(); }
    [[nodiscard]] bool HasValue(const std::string& name) const override { return static_cast<bool>(m_source->FindPart(m_part, name)); }
    [[nodiscard]] Value GetValueByName(const std::string& name) const override
    {
        auto value = m_source->FindPart(m_part, name);
        return value ? std::move(*value) : Value();
    }
    [[nodiscard]] std::optional<Value> Find(std::string_view name) const override { return m_source->FindPart(m_part, name); }
    [[nodiscard]] std::vector<std::string> GetKeys() const override { return m_source->KeysOfPart(m_part); }
    [[nodiscard]] bool IsEqual(const IComparable& other) const override
    {
        const auto* val = dynamic_cast<const ContextPartMapper*>(&other);
        if (!val)
        {
            return false;
        }
        return val->m_part == m_part && m_source->IsEqual(*val->m_source);
    }

private:
    std::shared_ptr<const ContextMapper> m_source;
    ContextNames m_part;
};

// The map of the host's call that ended last on this thread, for the next one (EndContextMap)
thread_local std::shared_ptr<const IMapItemAccessor> t_spareContext;
} // namespace

ContextMap MakeContextMap(RenderContext& context, void* scope, LendFn lend)
{
    // Taken from the slot, so that a call made during this one makes a map of its own
    auto spare = std::move(t_spareContext);
    // No copy of the ended call's map is left, on any thread: none can see the change, and the
    // host's last read of it is ordered before it (IsSoleOwner)
    if (spare && IsSoleOwner(spare))
    {
        const auto* handle = spare.get();
        // Only MakeContextMap makes the maps the slot holds
        static_cast<const ContextMapper*>(handle)->Renew(&context, scope, lend);
        return ContextMap{ GenericMap(std::move(spare)), handle };
    }
    auto mapper = std::make_shared<const ContextMapper>(&context, scope, lend);
    const IMapItemAccessor* handle = mapper.get();
    return ContextMap{ GenericMap(std::move(mapper)), handle };
}

void EndContextMap(const IMapItemAccessor* handle, Value& context) noexcept
{
    if (!handle)
    {
        return;
    }
    // Made by MakeContextMap only
    static_cast<const ContextMapper*>(handle)->Expire();
    auto* map = context.GetIf<GenericMap>();
    if (!map || t_spareContext || detail::GenericAccessorAccess::Get(*map).get() != handle)
    {
        return;
    }
    t_spareContext = detail::GenericAccessorAccess::Take(*map);
}

void RethrowAsPublicError()
{
    try
    {
        throw;
    }
    catch (const std::bad_alloc&)
    {
        throw;
    }
    catch (const ErrorInfo&)
    {
        throw;
    }
    catch (const ErrorInfoW&)
    {
        throw;
    }
    catch (const detail::RenderFailure& failure)
    {
        // A located error of the render (a limit, a failed include)
        throw failure.ErrorAs<char>();
    }
    catch (...)
    {
        // The host sees the engine's errors as the public ErrorInfo (a SecurityError, an UndefinedError, ...), never
        // as the engine's own exception types; a host's own exception stays in GetException
        throw detail::CurrentExceptionToError<char>(std::string());
    }
}
} // namespace host_call

Value ContextView(const Value& context, ContextNames names)
{
    const auto* map = context.GetIf<GenericMap>();
    if (!map)
    {
        return {};
    }
    auto source = std::dynamic_pointer_cast<const host_call::ContextMapper>(detail::GenericAccessorAccess::Get(*map));
    if (!source)
    {
        return {};
    }
    return Value(GenericMap(std::make_shared<const host_call::ContextPartMapper>(std::move(source), names)));
}

std::optional<LentCallableInfo> DescribeLentCallable(const UserCallable& callable)
{
    const auto* lent = host_call::LentCallableOf(callable);
    const auto* fn = lent ? GetIf<Callable>(lent) : nullptr;
    if (!fn)
    {
        return std::nullopt;
    }
    LentCallableInfo info;
    // The repr a template's {{ x|pprint }} prints (ValueRendererBase), and Python's type of it
    visitors::RenderValue(*lent, info.repr, true);
    info.typeName = visitors::PythonTypeName(*fn);
    // A global that is Python's own object holds its names (SetupGlobals); a macro or a bound method has neither
    if (const auto& attributes = fn->GetAttributes(); attributes && fn->GetKind() != Callable::Macro && !fn->GetBoundReceiver())
    {
        const auto module = attributes->find("__module__");
        const auto qualname = attributes->find("__qualname__");
        if (module != attributes->end() && qualname != attributes->end() && IsStringValue(module->second) && IsStringValue(qualname->second))
        {
            info.pythonModule = AsString(module->second);
            info.pythonQualname = AsString(qualname->second);
        }
    }
    return info;
}
} // namespace v2
} // namespace jinja2
