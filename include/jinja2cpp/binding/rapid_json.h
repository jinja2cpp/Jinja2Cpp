#ifndef JINJA2CPP_BINDING_RAPID_JSON_H
#define JINJA2CPP_BINDING_RAPID_JSON_H

#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>

#include <jinja2cpp/reflected_value.h>

namespace jinja2
{
namespace detail
{

template<typename T>
class RapidJsonObjectAccessor : public MapItemAccessor, public ReflectedDataHolder<T, false>
{
public:
    using ReflectedDataHolder<T, false>::ReflectedDataHolder;
    ~RapidJsonObjectAccessor() override = default;

    size_t GetSize() const override
    {
        auto j = this->GetValue();
        return j ? j->MemberCount() : 0ULL;
    }

    bool HasValue(const std::string& name) const override
    {
        auto j = this->GetValue();
        return j ? j->HasMember(name.c_str()) : false;
    }

    Value GetValueByName(const std::string& name) const override
    {
        auto j = this->GetValue();
        if (!j || !j->HasMember(name.c_str()))
            return Value();

        return Reflect(&(*j)[name.c_str()]);
    }

    std::vector<std::string> GetKeys() const override
    {
        auto j = this->GetValue();
        if (!j)
            return {};

        std::vector<std::string> result;
        // for (auto& item : j->())
        for (auto it = j->MemberBegin(); it != j->MemberEnd(); ++ it)
        {
            result.emplace_back(it->name.GetString());
        }
        return result;
    }
};


struct RapidJsonArrayAccessor : ListItemAccessor, IndexBasedAccessor, ReflectedDataHolder<rapidjson::Value, false>
{
    using ReflectedDataHolder<rapidjson::Value, false>::ReflectedDataHolder;

    nonstd::optional<size_t> GetSize() const override
    {
        auto j = this->GetValue();
        return j ? j->Size() : nonstd::optional<size_t>();
    }
    const IndexBasedAccessor* GetIndexer() const override
    {
        return this;
    }

    ListEnumeratorPtr CreateEnumerator() const override
    {
        using Enum = Enumerator<rapidjson::Value::ConstValueIterator>;
        auto j = this->GetValue();
        if (!j)
            jinja2::ListEnumeratorPtr(nullptr, Enum::Deleter);

        return jinja2::ListEnumeratorPtr(new Enum(j->Begin(), j->End()), Enum::Deleter);
    }

    Value GetItemByIndex(int64_t idx) const override
    {
        auto j = this->GetValue();
        if (!j)
            return Value();

        return Reflect((*j)[idx]);
    }

    size_t GetItemsCount() const override
    {
        auto sz = this->GetSize();
        return sz.value_or(0ULL);
    }
};

template<>
struct Reflector<rapidjson::Value>
{
    static Value CreateFromPtr(const rapidjson::Value *val)
    {
        Value result;
        switch (val->GetType())
        {
        case rapidjson::kNullType:
            break;
        case rapidjson::kFalseType:
            result = Value(false);
            break;
        case rapidjson::kTrueType:
            result = Value(true);
            break;
        case rapidjson::kObjectType:
            result = GenericMap([accessor = RapidJsonObjectAccessor<rapidjson::Value>(val)]() { return &accessor; });
            break;
        case rapidjson::kArrayType:
            result = GenericList([accessor = RapidJsonArrayAccessor(val)]() { return &accessor; });
            break;
        case rapidjson::kStringType:
            result = std::string(val->GetString(), val->GetStringLength());
            break;
        case rapidjson::kNumberType:
            if (val->IsInt64() || val->IsUint64())
                result = val->GetInt64();
            else if (val->IsInt() || val->IsUint())
                result = val->GetInt();
            else
                result = val->GetDouble();
            break;
        }
        return result;
    }

};

template<>
struct Reflector<rapidjson::Document>
{
    static Value Create(const rapidjson::Document& val)
    {
        return GenericMap([accessor = RapidJsonObjectAccessor<rapidjson::Document>(&val)]() { return &accessor; });
    }

    static Value CreateFromPtr(const rapidjson::Document *val)
    {
        return GenericMap([accessor = RapidJsonObjectAccessor<rapidjson::Document>(val)]() { return &accessor; });
    }

};
} // namespace detail
} // namespace jinja2

#endif // JINJA2CPP_BINDING_RAPID_JSON_H
