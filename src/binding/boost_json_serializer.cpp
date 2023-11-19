#include "boost_json_serializer.h"

#include "../value_visitors.h"
#include <iterator>
#include <fmt/ostream.h>

template <> struct fmt::formatter<boost::json::value> : ostream_formatter {};

namespace jinja2
{
namespace boost_json_serializer
{
namespace
{
struct JsonInserter : visitors::BaseVisitor<boost::json::value>
{
    using BaseVisitor::operator();

    explicit JsonInserter() {}

    boost::json::value operator()(const ListAdapter& list) const
    {
        boost::json::array listValue; //(boost::json::kind::array);

        for (auto& v : list)
        {
            listValue.push_back(Apply<JsonInserter>(v));
        }
        return listValue;
    }

    boost::json::value operator()(const MapAdapter& map) const
    {
        boost::json::object mapNode; //(boost::json::kind::object);

        const auto& keys = map.GetKeys();
        for (auto& k : keys)
        {
            mapNode.emplace(k.c_str(), Apply<JsonInserter>(map.GetValueByName(k)));
        }

        return mapNode;
    }

    boost::json::value operator()(const KeyValuePair& kwPair) const
    {
        boost::json::object pairNode; //(boost::json::kind::object);
        pairNode.emplace(kwPair.key.c_str(), Apply<JsonInserter>(kwPair.value));

        return pairNode;
    }

    boost::json::value operator()(const std::string& str) const { return boost::json::value(str.c_str()); }

    boost::json::value operator()(const nonstd::string_view& str) const
    {
        return boost::json::value(boost::json::string(str.data(), str.size()));
    }

    boost::json::value operator()(const std::wstring& str) const
    {
        auto s = ConvertString<std::string>(str);
        return boost::json::value(s.c_str());
    }

    boost::json::value operator()(const nonstd::wstring_view& str) const
    {
        auto s = ConvertString<std::string>(str);
        return boost::json::value(s.c_str());
    }

    boost::json::value operator()(bool val) const { return boost::json::value(val); }

    boost::json::value operator()(EmptyValue) const { return boost::json::value(); }

    boost::json::value operator()(const Callable&) const { return boost::json::value("<callable>"); }

    boost::json::value operator()(double val) const { return boost::json::value(val); }

    boost::json::value operator()(int64_t val) const { return boost::json::value(val); }

};
} // namespace

DocumentWrapper::DocumentWrapper()
{
}

ValueWrapper DocumentWrapper::CreateValue(const InternalValue& value) const
{
    auto v = Apply<JsonInserter>(value);
    return ValueWrapper(std::move(v));
}

ValueWrapper::ValueWrapper(boost::json::value&& value)
    : m_value(std::move(value))
{
}

void PrettyPrint(fmt::basic_memory_buffer<char>& os, const boost::json::value& jv, uint8_t indent = 4, int level = 0)
{
    switch (jv.kind())
    {
	case boost::json::kind::object:
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
                auto key = boost::json::serialize(it->key());
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

	case boost::json::kind::array:
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

	case boost::json::kind::string:
    {
        fmt::format_to(std::back_inserter(os), "{}", boost::json::serialize(jv.get_string()));
		break;
	}

	case boost::json::kind::uint64:
	case boost::json::kind::int64:
	case boost::json::kind::double_:
    {
        fmt::format_to(std::back_inserter(os), "{}", jv);
        break;
    }
	case boost::json::kind::bool_:
    {
        fmt::format_to(std::back_inserter(os), "{}", jv.get_bool());
		break;
    }

	case boost::json::kind::null:
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

} // namespace boost_json_serializer
} // namespace jinja2
