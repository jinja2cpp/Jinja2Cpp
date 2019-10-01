#ifndef JINJA2CPP_BINDING_NLOHMANN_JSON_H
#define JINJA2CPP_BINDING_NLOHMANN_JSON_H

#include <nlohmann/json.hpp>

#include <jinja2cpp/reflected_value.h>

namespace jinja2
{
namespace detail
{

class NLohmannJsonObjectAccessor : public MapItemAccessor, public ReflectedDataHolder<nlohmann::json>
{
public:
    using ReflectedDataHolder<nlohmann::json>::ReflectedDataHolder;
    ~NLohmannJsonObjectAccessor() override = default;

    size_t GetSize() const override
    {
        auto j = this->GetValue();
        return j ? j->size() : 0ULL;
    }

    bool HasValue(const std::string& name) const override
    {
        auto j = this->GetValue();
        return j ? j->contains(name) : false;
    }

    Value GetValueByName(const std::string& name) const override
    {
        auto j = this->GetValue();
        if (!j || !j->contains(name))
            return Value();

        return Reflect(&(*j)[name]);
    }

    std::vector<std::string> GetKeys() const override
    {
        auto j = this->GetValue();
        if (!j)
            return {};

        std::vector<std::string> result;
        for (auto& item : j->items())
        {
            result.emplace_back(item.key());
        }
        return result;
    }
};


struct NLohmannJsonArrayAccessor : ListItemAccessor, IndexBasedAccessor, ReflectedDataHolder<nlohmann::json>
{
    using ReflectedDataHolder<nlohmann::json>::ReflectedDataHolder;

    nonstd::optional<size_t> GetSize() const override
    {
        auto j = this->GetValue();
        return j ? j->size() : nonstd::optional<size_t>();
    }
    const IndexBasedAccessor* GetIndexer() const override
    {
        return this;
    }

    ListEnumeratorPtr CreateEnumerator() const override
    {
        using Enum = Enumerator<typename nlohmann::json::const_iterator>;
        auto j = this->GetValue();
        if (!j)
            jinja2::ListEnumeratorPtr(nullptr, Enum::Deleter);

        return jinja2::ListEnumeratorPtr(new Enum(j->begin(), j->end()), Enum::Deleter);
    }

    Value GetItemByIndex(int64_t idx) const override
    {
        auto j = this->GetValue();
        if (!j)
            return Value();

        return Reflect((*j)[idx]);
    }
};

template<>
struct Reflector<nlohmann::json>
{
    static Value Create(nlohmann::json val)
    {
        Value result;
        switch (val.type())
        {
        case nlohmann::detail::value_t::null:
            break;
        case nlohmann::detail::value_t::object:
            result = GenericMap([accessor = NLohmannJsonObjectAccessor(std::move(val))]() { return &accessor; });
            break;
        case nlohmann::detail::value_t::array:
            result = GenericList([accessor = NLohmannJsonArrayAccessor(std::move(val))]() { return &accessor; });
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

    static Value CreateFromPtr(const nlohmann::json *val)
    {
        Value result;
        switch (val->type())
        {
        case nlohmann::detail::value_t::null:
            break;
        case nlohmann::detail::value_t::object:
            result = GenericMap([accessor = NLohmannJsonObjectAccessor(val)]() { return &accessor; });
            break;
        case nlohmann::detail::value_t::array:
            result = GenericList([accessor = NLohmannJsonArrayAccessor(val)]() {return &accessor;});
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

} // namespace detail
} // namespace jinja2

#endif // JINJA2CPP_BINDING_NLOHMANN_JSON_H