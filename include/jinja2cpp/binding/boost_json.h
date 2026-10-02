#ifndef JINJA2CPP_BINDING_BOOST_JSON_H
#define JINJA2CPP_BINDING_BOOST_JSON_H

#include <boost/json.hpp>
#include <boost/json/visit.hpp>
#include <jinja2cpp/reflected_value.h>

namespace jinja2
{
namespace detail
{

class BoostJsonObjectAccessor
    : public IMapItemAccessor
    , public ReflectedDataHolder<boost::json::value>
{
    struct SizeVisitor
    {
        size_t operator()(std::nullptr_t) { return {}; }
        size_t operator()(bool) { return 1; }
        size_t operator()(std::int64_t) { return 1; }
        size_t operator()(std::uint64_t) { return 1; }
        size_t operator()(double) { return 1; }
        size_t operator()(const boost::json::string&) { return 1; }
        size_t operator()(const boost::json::array& val) { return val.size(); }
        size_t operator()(const boost::json::object& val) { return val.size(); }
        size_t operator()(...) { return 0; }
    };

public:
    using ReflectedDataHolder<boost::json::value>::ReflectedDataHolder;
    ~BoostJsonObjectAccessor() override = default;

    [[nodiscard]] size_t GetSize() const override
    {
        const auto* j = this->GetValue();
        if (!j)
            return {};
        // simulate nlohmann semantics
        SizeVisitor sv;
        return boost::json::visit(sv, *j);
    }

    [[nodiscard]] bool HasValue(const std::string& name) const override
    {
        const auto* j = this->GetValue();
        if (!j)
            return false;
        const auto* obj = j->if_object();
        return obj ? obj->contains(name) : false;
    }

    [[nodiscard]] Value GetValueByName(const std::string& name) const override
    {
        const auto* j = this->GetValue();
        if (!j)
            return Value();
        const auto* obj = j->if_object();
        if (!obj)
            return Value();
        const auto* val = obj->if_contains(name);
        if (!val)
            return Value();
        return Reflect(*val);
    }

    [[nodiscard]] std::vector<std::string> GetKeys() const override
    {
        const auto* j = this->GetValue();
        if (!j)
            return {};
        const auto* obj = j->if_object();
        if (!obj)
            return {};
        std::vector<std::string> result;
        result.reserve(obj->size());
        for (const auto& item : *obj)
        {
            result.emplace_back(item.key());
        }
        return result;
    }
    [[nodiscard]] bool IsEqual(const IComparable& other) const override
    {
        const auto* val = dynamic_cast<const BoostJsonObjectAccessor*>(&other);
        if (!val)
            return false;
        return this->GetValue() == val->GetValue();
    }
};

struct BoostJsonArrayAccessor
    : IListItemAccessor
    , IIndexBasedAccessor
    , ReflectedDataHolder<boost::json::array>
{
    using ReflectedDataHolder<boost::json::array>::ReflectedDataHolder;

    [[nodiscard]] std::optional<size_t> GetSize() const override
    {
        const auto* j = this->GetValue();
        return j ? j->size() : std::optional<size_t>();
    }

    [[nodiscard]] const IIndexBasedAccessor* GetIndexer() const override { return this; }

    [[nodiscard]] std::optional<ListEnumeratorPtr> CreateEnumerator() const override
    {
        using Enum = Enumerator<typename boost::json::array::const_iterator>;
        const auto* j = this->GetValue();
        if (!j)
            return {};
        return jinja2::ListEnumeratorPtr{ types::in_place_type_t<Enum>{}, j->begin(), j->end() };
    }

    [[nodiscard]] Value GetItemByIndex(int64_t idx) const override
    {
        const auto* j = this->GetValue();
        if (!j)
            return Value();

        return Reflect((*j)[idx]);
    }

    [[nodiscard]] bool IsEqual(const IComparable& other) const override
    {
        const auto* val = dynamic_cast<const BoostJsonArrayAccessor*>(&other);
        if (!val)
            return false;
        return GetValue() == val->GetValue();
    }
};

} // namespace detail

template<>
struct Reflector<boost::json::value>
{
    static Value Create(boost::json::value val)
    {
        Value result;
        switch (val.kind())
        {
        default: // unreachable()?
        case boost::json::kind::null:
            break;
        case boost::json::kind::bool_:
            result = val.get_bool();
            break;
        case boost::json::kind::int64:
            result = val.get_int64();
            break;
        case boost::json::kind::uint64:
            result = static_cast<int64_t>(val.get_uint64());
            break;
        case boost::json::kind::double_:
            result = val.get_double();
            break;
        case boost::json::kind::string:
            result = std::string(val.get_string().c_str());
            break;
        case boost::json::kind::array:
        {
            auto array = val.get_array();
            result = GenericList([accessor = detail::BoostJsonArrayAccessor(std::move(array))]() { return &accessor; });
            break;
        }
        case boost::json::kind::object:
        {
            auto obj = val.get_object();
            result = GenericMap([accessor = detail::BoostJsonObjectAccessor(std::move(val))]() { return &accessor; });
            break;
        }
        }
        return result;
    }

    static Value CreateFromPtr(const boost::json::value* val)
    {
        Value result;
        switch (val->kind())
        {
        default: // unreachable()?
        case boost::json::kind::null:
            break;
        case boost::json::kind::bool_:
            result = val->get_bool();
            break;
        case boost::json::kind::int64:
            result = val->get_int64();
            break;
        case boost::json::kind::uint64:
            result = static_cast<int64_t>(val->get_uint64());
            break;
        case boost::json::kind::double_:
            result = val->get_double();
            break;
        case boost::json::kind::string:
            result = std::string(val->get_string().c_str());
            break;
        case boost::json::kind::array:
        {
            auto array = val->get_array();
            result = GenericList([accessor = detail::BoostJsonArrayAccessor(std::move(array))]() { return &accessor; });
            break;
        }
        case boost::json::kind::object:
        {
            auto obj = val->get_object();
            result = GenericMap([accessor = detail::BoostJsonObjectAccessor(std::move(val))]() { return &accessor; });
            break;
        }
        }
        return result;
    }
};

} // namespace jinja2

#endif //  JINJA2CPP_BINDING_BOOST_JSON_H
