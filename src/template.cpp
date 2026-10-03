#include "jinja2cpp/template.h"
#include "make_unexpected.h"
#include "template_impl.h"

#include <fmt/format.h>

#include <fstream>
#include <sstream>

namespace jinja2
{
namespace
{
template<typename CharT>
auto GetImpl(const std::shared_ptr<ITemplateImpl>& impl)
{
    return static_cast<TemplateImpl<CharT>*>(impl.get());
}

template<typename CharT>
Result<void, CharT> ToResult(boost::optional<BasicErrorInfo<CharT>> error)
{
    if (!error)
    {
        return {};
    }
    return MakeUnexpected(std::move(error.get()));
}

template<typename CharT>
struct FileStream;

template<>
struct FileStream<char>
{
    using Type = std::ifstream;
};

template<>
struct FileStream<wchar_t>
{
    using Type = std::wifstream;
};
} // namespace

template<typename CharT>
BasicTemplate<CharT>::BasicTemplate(TemplateEnv* env)
    : m_impl(std::make_shared<TemplateImpl<CharT>>(env))
{
}

template<typename CharT>
BasicTemplate<CharT>::~BasicTemplate() = default;

template<typename CharT>
Result<void, CharT> BasicTemplate<CharT>::Load(StringViewType source, std::string name)
{
    return ToResult(GetImpl<CharT>(m_impl)->Load(StringType(source), std::move(name)));
}

template<typename CharT>
Result<void, CharT> BasicTemplate<CharT>::Load(std::basic_istream<CharT>& stream, std::string name)
{
    StringType t;

    while (stream.good() && !stream.eof())
    {
        CharT buff[0x10000];
        stream.read(buff, sizeof(buff) / sizeof(CharT));
        auto read = stream.gcount();
        if (read)
        {
            t.append(buff, buff + read);
        }
    }

    return ToResult(GetImpl<CharT>(m_impl)->Load(std::move(t), std::move(name)));
}

template<typename CharT>
Result<void, CharT> BasicTemplate<CharT>::LoadFromFile(const std::string& fileName)
{
    typename FileStream<CharT>::Type file(fileName);

    if (!file.good())
    {
        return {};
    }

    return Load(file, fileName);
}

template<typename CharT>
Result<void, CharT> BasicTemplate<CharT>::Render(std::basic_ostream<CharT>& os, const ValuesMap& params) const
{
    StringType buffer;
    auto result = GetImpl<CharT>(m_impl)->Render(buffer, params);

    if (!result)
    {
        os.write(buffer.data(), static_cast<std::streamsize>(buffer.size()));
    }

    return ToResult(std::move(result));
}

template<typename CharT>
Result<void, CharT> BasicTemplate<CharT>::RenderGeneric(std::basic_ostream<CharT>& os, const GenericMap& params) const
{
    StringType buffer;
    auto result = GetImpl<CharT>(m_impl)->Render(buffer, params);

    if (!result)
    {
        os.write(buffer.data(), static_cast<std::streamsize>(buffer.size()));
    }

    return ToResult(std::move(result));
}

template<typename CharT>
auto BasicTemplate<CharT>::RenderAsString(const ValuesMap& params) const -> Result<StringType, CharT>
{
    StringType buffer;
    auto result = GetImpl<CharT>(m_impl)->Render(buffer, params);
    if (result)
    {
        return MakeUnexpected(std::move(result.get()));
    }
    return buffer;
}

template<typename CharT>
auto BasicTemplate<CharT>::RenderAsStringGeneric(const GenericMap& params) const -> Result<StringType, CharT>
{
    StringType buffer;
    auto result = GetImpl<CharT>(m_impl)->Render(buffer, params);
    if (result)
    {
        return MakeUnexpected(std::move(result.get()));
    }
    return buffer;
}

template<typename CharT>
Result<GenericMap, CharT> BasicTemplate<CharT>::GetMetadata() const
{
    return GetImpl<CharT>(m_impl)->GetMetadata();
}

template<typename CharT>
Result<MetadataInfo<CharT>, CharT> BasicTemplate<CharT>::GetMetadataRaw() const
{
    return GetImpl<CharT>(m_impl)->GetMetadataRaw();
}

template<typename CharT>
bool BasicTemplate<CharT>::IsEqual(const BasicTemplate& other) const
{
    return m_impl == other.m_impl;
}

template class JINJA2CPP_EXPORT BasicTemplate<char>;
template class JINJA2CPP_EXPORT BasicTemplate<wchar_t>;

} // namespace jinja2
