#include "nlohmann_json_serializer.h"

#include "../value_visitors.h"

#include <fmt/ostream.h>

#include <iterator>
#include <numeric>
#include <string>


//template <> struct fmt::formatter<nlohmann::json::value> : ostream_formatter {};

namespace jinja2
{
namespace nlohmann_json_serializer
{
namespace
{
struct JsonInserter : visitors::BaseVisitor<nlohmann::json>
{
    using BaseVisitor::operator();

    explicit JsonInserter() {}

    nlohmann::json operator()(const ListAdapter& list) const
    {
        nlohmann::json listValue; //(nlohmann::json::kind::array);

        for (auto& v : list)
        {
            listValue.push_back(Apply<JsonInserter>(v));
        }
        return listValue;
    }

    nlohmann::json operator()(const MapAdapter& map) const
    {
        nlohmann::json mapNode; //(nlohmann::json::kind::object);

        const auto& keys = map.GetKeys();
        for (auto& k : keys)
        {
            mapNode.emplace(k.c_str(), Apply<JsonInserter>(map.GetValueByName(k)));
        }

        return mapNode;
    }

    nlohmann::json operator()(const KeyValuePair& kwPair) const
    {
        nlohmann::json pairNode; //(nlohmann::json::kind::object);
        pairNode.emplace(kwPair.key.c_str(), Apply<JsonInserter>(kwPair.value));

        return pairNode;
    }

    nlohmann::json operator()(const std::string& str) const { return nlohmann::json(str.c_str()); }

    nlohmann::json operator()(const nonstd::string_view& str) const
    {
        return nlohmann::json(std::string{str.data(), str.size()});
    }

    nlohmann::json operator()(const std::wstring& str) const
    {
        auto s = ConvertString<std::string>(str);
        return nlohmann::json({s.c_str()});
    }

    nlohmann::json operator()(const nonstd::wstring_view& str) const
    {
        auto s = ConvertString<std::string>(str);
        return nlohmann::json(s.c_str());
    }

    nlohmann::json operator()(bool val) const { return nlohmann::json(val); }

    nlohmann::json operator()(EmptyValue) const { return nlohmann::json(); }

    nlohmann::json operator()(const Callable&) const { return nlohmann::json("<callable>"); }

    nlohmann::json operator()(double val) const { return nlohmann::json(val); }

    nlohmann::json operator()(int64_t val) const { return nlohmann::json(val); }

};
} // namespace
/*
DocumentWrapper::DocumentWrapper()
{
}

ValueWrapper DocumentWrapper::CreateValue(const InternalValue& value) const
{
    auto v = Apply<JsonInserter>(value);
    return ValueWrapper(std::move(v));
}

ValueWrapper::ValueWrapper(nlohmann::json::value&& value)
    : m_value(std::move(value))
{
}
 
void PrettyPrint(fmt::basic_memory_buffer<char>& os, const nlohmann::json::value& jv, uint8_t indent = 4, int level = 0)
{
    switch (jv.kind())
    {
	case nlohmann::json::kind::object:
    {
        fmt::format_to(std::back_inserter(os), "{}", '{');
        if (indent != 0)
        {
            fmt::format_to(std::back_inserter(os), "{}", "\n");
        }
		const auto& obj = jv.get_object();
		if (!obj.empty())
		{
			auto it = obj.begin();
			for (;;)
			{
                auto key = nlohmann::json::serialize(it->key());
                fmt::format_to(
                        std::back_inserter(os),
                        "{: >{}}{: <{}}",
                        key,
                        key.size() + indent * (level + 1),
                        ":",
                        (indent == 0) ? 0 : 2
                );
				PrettyPrint(os, it->value(), indent, level + 1);
				if (++it == obj.end())
					break;
                fmt::format_to(std::back_inserter(os), "{: <{}}", ",", (indent == 0) ? 0 : 2);
			}
		}
        if (indent != 0)
        {
            fmt::format_to(std::back_inserter(os), "{}", "\n");
        }
        fmt::format_to(std::back_inserter(os), "{: >{}}", "}", (indent * level) + 1);
	    break;
	}

	case nlohmann::json::kind::array:
    {
        fmt::format_to(std::back_inserter(os), "[");
		auto const& arr = jv.get_array();
		if (!arr.empty())
		{
			auto it = arr.begin();
			for (;;)
			{
				PrettyPrint(os, *it, indent, level + 1);
				if (++it == arr.end())
					break;
                fmt::format_to(std::back_inserter(os), "{: <{}}", ",", (indent == 0) ? 0 : 2);
			}
		}
        fmt::format_to(std::back_inserter(os), "]");
		break;
	}

	case nlohmann::json::kind::string:
    {
        fmt::format_to(std::back_inserter(os), "{}", nlohmann::json::serialize(jv.get_string()));
		break;
	}

	case nlohmann::json::kind::uint64:
	case nlohmann::json::kind::int64:
	case nlohmann::json::kind::double_:
    {
        fmt::format_to(std::back_inserter(os), "{}", jv);
        break;
    }
	case nlohmann::json::kind::bool_:
    {
        fmt::format_to(std::back_inserter(os), "{}", jv.get_bool());
		break;
    }

	case nlohmann::json::kind::null:
    {
        fmt::format_to(std::back_inserter(os), "null");
		break;
    }
    }
}

std::string ValueWrapper::AsString(const uint8_t indent) const
{
    fmt::memory_buffer out;
	PrettyPrint(out, m_value, indent);
	return fmt::to_string(out);
}
*/
} // namespace nlohmann_json_serializer

std::string ToJson(const InternalValue& value, uint8_t indent)
{
    using namespace std::literals;
    //boost_json_serializer::DocumentWrapper jsonDoc;
    auto jsonValue = Apply<nlohmann_json_serializer::JsonInserter>(value);
    auto jsonString = jsonValue.dump(indent == 0 ? -1 : indent);
    //const auto jsonValue = jsonDoc.CreateValue(value);
    //const auto jsonString = jsonValue.AsString(static_cast<uint8_t>(indent));
    const auto result = std::accumulate(jsonString.begin(), jsonString.end(), ""s, [](const auto &str, const auto &c)
    {
        switch (c)
        {
       case '<':
            return str + "\\u003c";
            break;
        case '>':
            return str +"\\u003e";
            break;
        case '&':
            return str +"\\u0026";
            break;
        case '\'':
            return str +"\\u0027";
            break;
        default:
            return str + c;
            break;
        }
    });
    return result;
}

} // namespace jinja2
