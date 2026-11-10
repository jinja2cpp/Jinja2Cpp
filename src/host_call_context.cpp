#include "host_call_context.h"

#include "error_boundary.h"
#include "internal_value.h"
#include "render_context.h"
#include "render_failure.h"

#include <jinja2cpp/error_info.h>

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
    LendFn m_lend;
};
} // namespace

ContextMap MakeContextMap(RenderContext& context, void* scope, LendFn lend)
{
    auto mapper = std::make_shared<const ContextMapper>(&context, scope, lend);
    const IMapItemAccessor* handle = mapper.get();
    return ContextMap{ GenericMap(std::move(mapper)), handle };
}

void ExpireContextMap(const IMapItemAccessor* handle) noexcept
{
    if (handle)
    {
        // Made by MakeContextMap only
        static_cast<const ContextMapper*>(handle)->Expire();
    }
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
} // namespace v2
} // namespace jinja2
