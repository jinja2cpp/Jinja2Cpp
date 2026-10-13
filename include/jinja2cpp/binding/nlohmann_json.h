#ifndef JINJA2CPP_BINDING_NLOHMANN_JSON_H
#define JINJA2CPP_BINDING_NLOHMANN_JSON_H

#include <nlohmann/json.hpp>

#include <jinja2cpp/reflected_value.h>

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

namespace jinja2
{
namespace detail
{

// Json is nlohmann::json, whose objects iterate in key order, or nlohmann::ordered_json, whose objects keep
// document order (docs/tasks/0043)
template<typename Json>
class BasicNLohmannJsonObjectAccessor : public IMapItemAccessor
    , public ReflectedDataHolder<Json>
{
public:
    using ReflectedDataHolder<Json>::ReflectedDataHolder;
    ~BasicNLohmannJsonObjectAccessor() override = default;

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

    bool EmptyIsNone() const override { return true; }

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
        auto* val = dynamic_cast<const BasicNLohmannJsonObjectAccessor*>(&other);
        if (!val)
        {
            return false;
        }
        return this->GetValue() == val->GetValue();
    }
};


template<typename Json>
struct BasicNLohmannJsonArrayAccessor
    : IListItemAccessor
    , IIndexBasedAccessor
    , ReflectedDataHolder<Json>
{
    using ReflectedDataHolder<Json>::ReflectedDataHolder;

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
        using Enum = Enumerator<typename Json::const_iterator>;
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

        return Reflect((*j)[static_cast<std::size_t>(idx)]);
    }

    bool IsEqual(const IComparable& other) const override
    {
        auto* val = dynamic_cast<const BasicNLohmannJsonArrayAccessor*>(&other);
        if (!val)
        {
            return false;
        }
        return this->GetValue() == val->GetValue();
    }
};

using NLohmannJsonObjectAccessor = BasicNLohmannJsonObjectAccessor<nlohmann::json>;
using NLohmannJsonArrayAccessor = BasicNLohmannJsonArrayAccessor<nlohmann::json>;

template<typename Json>
struct NLohmannJsonReflector
{
    static Value Create(Json val)
    {
        Value result;
        switch (val.type())
        {
        case nlohmann::detail::value_t::binary:
            break;
        case nlohmann::detail::value_t::null:
            break;
        case nlohmann::detail::value_t::object:
            result = GenericMap([accessor = BasicNLohmannJsonObjectAccessor<Json>(std::move(val))]() { return &accessor; });
            break;
        case nlohmann::detail::value_t::array:
            result = GenericList([accessor = BasicNLohmannJsonArrayAccessor<Json>(std::move(val))]() { return &accessor; });
            break;
        case nlohmann::detail::value_t::string:
            result = val.template get<std::string>();
            break;
        case nlohmann::detail::value_t::boolean:
            result = val.template get<bool>();
            break;
        case nlohmann::detail::value_t::number_integer:
        case nlohmann::detail::value_t::number_unsigned:
            result = val.template get<int64_t>();
            break;
        case nlohmann::detail::value_t::number_float:
            result = val.template get<double>();
            break;
        case nlohmann::detail::value_t::discarded:
            break;
        }
        return result;
    }

    static Value CreateFromPtr(const Json* val)
    {
        Value result;
        switch (val->type())
        {
        case nlohmann::detail::value_t::binary:
            break;
        case nlohmann::detail::value_t::null:
            break;
        case nlohmann::detail::value_t::object:
            result = GenericMap([accessor = BasicNLohmannJsonObjectAccessor<Json>(val)]() { return &accessor; });
            break;
        case nlohmann::detail::value_t::array:
            result = GenericList([accessor = BasicNLohmannJsonArrayAccessor<Json>(val)]() { return &accessor; });
            break;
        case nlohmann::detail::value_t::string:
            result = val->template get<std::string>();
            break;
        case nlohmann::detail::value_t::boolean:
            result = val->template get<bool>();
            break;
        case nlohmann::detail::value_t::number_integer:
        case nlohmann::detail::value_t::number_unsigned:
            result = val->template get<int64_t>();
            break;
        case nlohmann::detail::value_t::number_float:
            result = val->template get<double>();
            break;
        case nlohmann::detail::value_t::discarded:
            break;
        }
        return result;
    }
};

} // namespace detail

template<>
struct Reflector<nlohmann::json> : detail::NLohmannJsonReflector<nlohmann::json>
{
};

// ordered_json (nlohmann 3.9 and later) keeps the document order of objects, as a Python dict from json.loads does
#if NLOHMANN_JSON_VERSION_MAJOR > 3 || (NLOHMANN_JSON_VERSION_MAJOR == 3 && NLOHMANN_JSON_VERSION_MINOR >= 9)
template<>
struct Reflector<nlohmann::ordered_json> : detail::NLohmannJsonReflector<nlohmann::ordered_json>
{
};
#endif

} // namespace jinja2

#endif // JINJA2CPP_BINDING_NLOHMANN_JSON_H
