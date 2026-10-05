#ifndef JINJA2CPP_BINDING_NLOHMANN_JSON_H
#define JINJA2CPP_BINDING_NLOHMANN_JSON_H

#include <nlohmann/json.hpp>

#include <jinja2cpp/reflected_value.h>

#include <optional>
#include <string>
#include <string_view>

namespace jinja2
{
namespace detail
{

class NLohmannJsonObjectAccessor : public IMapItemAccessor
    , public ReflectedDataHolder<nlohmann::json>
{
public:
    using ReflectedDataHolder<nlohmann::json>::ReflectedDataHolder;
    ~NLohmannJsonObjectAccessor() override = default;

    size_t GetSize() const override
    {
        auto j = this->GetValue();
        return j ? j->size() : 0ULL;
    }

    // nlohmann::json looks a key up by string_view from 3.11 on
#if NLOHMANN_JSON_VERSION_MAJOR > 3 || (NLOHMANN_JSON_VERSION_MAJOR == 3 && NLOHMANN_JSON_VERSION_MINOR >= 11)
    static std::string_view Key(std::string_view name) { return name; }
#else
    static std::string Key(std::string_view name) { return std::string(name); }
#endif

    bool Contains(std::string_view name) const override
    {
        auto j = this->GetValue();
        return j ? j->contains(Key(name)) : false;
    }

    std::optional<Value> Find(std::string_view name) const override
    {
        auto j = this->GetValue();
        if (!j)
        {
            return std::nullopt;
        }
        auto p = j->find(Key(name));
        if (p == j->end())
        {
            return std::nullopt;
        }

        return Reflect(&*p);
    }

    std::vector<std::string> GetKeys() const override
    {
        auto j = this->GetValue();
        if (!j)
        {
            return {};
        }

        std::vector<std::string> result;
        result.reserve(j->size());
        for (auto& item : j->items())
        {
            result.emplace_back(item.key());
        }
        return result;
    }

    bool IsEqual(const IComparable& other) const override
    {
        auto* val = dynamic_cast<const NLohmannJsonObjectAccessor*>(&other);
        if (!val)
        {
            return false;
        }
        return GetValue() == val->GetValue();
    }
};


struct NLohmannJsonArrayAccessor
    : IListItemAccessor
    , IIndexBasedAccessor
    , ReflectedDataHolder<nlohmann::json>
{
    using ReflectedDataHolder<nlohmann::json>::ReflectedDataHolder;

    std::optional<size_t> GetSize() const override
    {
        auto j = this->GetValue();
        return j ? j->size() : std::optional<size_t>();
    }

    const IIndexBasedAccessor* GetIndexer() const override
    {
        return this;
    }

    std::optional<ListEnumeratorPtr> CreateEnumerator() const override
    {
        using Enum = Enumerator<typename nlohmann::json::const_iterator>;
        auto j = this->GetValue();
        if (!j)
        {
            return {};
        }
        return jinja2::ListEnumeratorPtr{ types::in_place_type_t<Enum>{}, j->begin(), j->end() };
    }

    Value GetItemByIndex(int64_t idx) const override
    {
        auto j = this->GetValue();
        if (!j)
        {
            return Value();
        }

        return Reflect((*j)[idx]);
    }

    bool IsEqual(const IComparable& other) const override
    {
        auto* val = dynamic_cast<const NLohmannJsonArrayAccessor*>(&other);
        if (!val)
        {
            return false;
        }
        return GetValue() == val->GetValue();
    }
};

} // namespace detail

template<>
struct Reflector<nlohmann::json>
{
    static Value Create(nlohmann::json val)
    {
        Value result;
        switch (val.type())
        {
        case nlohmann::detail::value_t::binary:
            break;
        case nlohmann::detail::value_t::null:
            break;
        case nlohmann::detail::value_t::object:
            result = GenericMap([accessor = detail::NLohmannJsonObjectAccessor(std::move(val))]() { return &accessor; });
            break;
        case nlohmann::detail::value_t::array:
            result = GenericList([accessor = detail::NLohmannJsonArrayAccessor(std::move(val))]() { return &accessor; });
            break;
        case nlohmann::detail::value_t::string:
            result = val.get<std::string>();
            break;
        case nlohmann::detail::value_t::boolean:
            result = val.get<bool>();
            break;
        case nlohmann::detail::value_t::number_integer:
        case nlohmann::detail::value_t::number_unsigned:
            result = val.get<int64_t>();
            break;
        case nlohmann::detail::value_t::number_float:
            result = val.get<double>();
            break;
        case nlohmann::detail::value_t::discarded:
            break;
        }
        return result;
    }

    static Value CreateFromPtr(const nlohmann::json* val)
    {
        Value result;
        switch (val->type())
        {
        case nlohmann::detail::value_t::binary:
            break;
        case nlohmann::detail::value_t::null:
            break;
        case nlohmann::detail::value_t::object:
            result = GenericMap([accessor = detail::NLohmannJsonObjectAccessor(val)]() { return &accessor; });
            break;
        case nlohmann::detail::value_t::array:
            result = GenericList([accessor = detail::NLohmannJsonArrayAccessor(val)]() { return &accessor; });
            break;
        case nlohmann::detail::value_t::string:
            result = val->get<std::string>();
            break;
        case nlohmann::detail::value_t::boolean:
            result = val->get<bool>();
            break;
        case nlohmann::detail::value_t::number_integer:
        case nlohmann::detail::value_t::number_unsigned:
            result = val->get<int64_t>();
            break;
        case nlohmann::detail::value_t::number_float:
            result = val->get<double>();
            break;
        case nlohmann::detail::value_t::discarded:
            break;
        }
        return result;
    }
};

} // namespace jinja2

#endif // JINJA2CPP_BINDING_NLOHMANN_JSON_H
