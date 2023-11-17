#include "boost_json_serializer.h"

#include "../value_visitors.h"

// #include <boost::json/prettywriter.h>

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
        return boost::json::value(boost::json::string(str));
        // str.data(), static_cast<std::size_t>(str.size()));
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

    //    boost::json::Document::AllocatorType& m_allocator;
};
} // namespace

DocumentWrapper::DocumentWrapper()
//    : m_document(std::make_shared<boost::json::Document>())
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

void PrettyPrint(std::ostream& os, const boost::json::value& jv, uint8_t indent = 4, std::string* indentString = nullptr)
{
    std::string indentString_;
    if (!indentString)
        indentString = &indentString_;
    switch (jv.kind())
    {
	case boost::json::kind::object:
    {
		os << "{\n";
		indentString->append(indent, ' ');
		auto const& obj = jv.get_object();
		if (!obj.empty())
		{
			auto it = obj.begin();
			for (;;)
			{
				os << *indentString << boost::json::serialize(it->key()) << " : ";
				PrettyPrint(os, it->value(), indent, indentString);
				if (++it == obj.end())
					break;
				os << ",\n";
			}
		}
		os << "\n";
		indentString->resize(indentString->size() - indent);
		os << *indentString << "}";
		break;
	}

	case boost::json::kind::array:
    {
		//os << "[\n";
        os << "[";
		indentString->append(1, ' ');
		auto const& arr = jv.get_array();
		if (!arr.empty())
		{
			auto it = arr.begin();
			for (;;)
			{
				os << ((it == arr.begin()) ? "" : *indentString);
				PrettyPrint(os, *it, indent, indentString);
				if (++it == arr.end())
					break;
				//os << ",\n";
                os << ",";
			}
		}
		//os << "\n";
		indentString->resize(indentString->size() - indent);
		os << *indentString << "]";
		break;
	}

	case boost::json::kind::string:
    {
		os << boost::json::serialize(jv.get_string());
		break;
	}

	case boost::json::kind::uint64:
	case boost::json::kind::int64:
	case boost::json::kind::double_:
		os << jv;
		break;

	case boost::json::kind::bool_:
		if (jv.get_bool())
			os << "true";
		else
			os << "false";
		break;

	case boost::json::kind::null:
		os << "null";
		break;
    }

    //if (indentString->empty())
    //    os << "\n";
}

std::string ValueWrapper::AsString(const uint8_t indent) const
{
    //    using Writer = boost::json::Writer<boost::json::StringBuffer, boost::json::Document::EncodingType, boost::json::UTF8<>>;
    //    using PrettyWriter = boost::json::PrettyWriter<boost::json::StringBuffer, boost::json::Document::EncodingType, boost::json::UTF8<>>;
	std::stringstream ss;
	PrettyPrint(ss, m_value, indent);
	return ss.str();
    /* boost::json::StringBuffer buffer; */
    /* if (indent == 0) */
    /* { */
    /*     Writer writer(buffer); */
    /*     m_value.Accept(writer); */
    /* } */
    /* else */
    /* { */
    /*     PrettyWriter writer(buffer); */
    /*     writer.SetIndent(' ', indent); */
    /*     writer.SetFormatOptions(boost::json::kind::FormatSingleLineArray); */
    /*     m_value.Accept(writer); */
    /* } */

    /* return buffer.GetString(); */
}

} // namespace boost_json_serializer
} // namespace jinja2
