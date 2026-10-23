#include <jinja2cpp/template.h>

#include "make_unexpected.h"
#include "markup.h"
#include "render_workspace.h"
#include "template_impl.h"

#include <jinja2cpp/config.h>
#include <jinja2cpp/error_info.h>
#include <jinja2cpp/value.h>

#include <fmt/format.h>

#include <fstream>
#include <ios>
#include <istream>
#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

namespace jinja2
{
inline namespace v2
{
namespace
{
template<typename CharT>
auto GetImpl(const std::shared_ptr<ITemplateImpl>& impl)
{
    return static_cast<TemplateImpl<CharT>*>(impl.get());
}

template<typename CharT>
Result<void, CharT> ToResult(std::optional<BasicErrorInfo<CharT>> error)
{
    if (!error)
    {
        return {};
    }
    return MakeUnexpected(std::move(*error));
}

// Truncates a RenderTo string back to its size at entry, unless the render succeeded: on an error
// and when std::bad_alloc goes through. Shrinking never allocates
template<typename StringT>
class TruncateOnError
{
public:
    explicit TruncateOnError(StringT& out) noexcept
        : m_out(out)
        , m_size(out.size())
    {
    }
    TruncateOnError(const TruncateOnError&) = delete;
    TruncateOnError& operator=(const TruncateOnError&) = delete;
    ~TruncateOnError()
    {
        if (!m_succeeded)
        {
            m_out.resize(m_size);
        }
    }

    template<typename ErrorT>
    auto Done(std::optional<ErrorT> error)
    {
        m_succeeded = !error;
        return error;
    }

private:
    StringT& m_out;
    typename StringT::size_type m_size;
    bool m_succeeded = false;
};

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
    auto* impl = GetImpl<CharT>(m_impl);
    if (impl->SourceTooLong(source.size()))
    {
        return ToResult(std::optional(impl->SourceTooLongError(name)));
    }
    return ToResult(impl->Load(StringType(source), std::move(name)));
}

template<typename CharT>
Result<void, CharT> BasicTemplate<CharT>::Load(std::basic_istream<CharT>& stream, std::string name)
{
    auto* impl = GetImpl<CharT>(m_impl);
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
        // Stops reading a source past maxTemplateSize
        if (impl->SourceTooLong(t.size()))
        {
            return ToResult(std::optional(impl->SourceTooLongError(name)));
        }
    }

    return ToResult(impl->Load(std::move(t), std::move(name)));
}

template<typename CharT>
Result<void, CharT> BasicTemplate<CharT>::LoadFromFile(const std::string& fileName)
{
    typename FileStream<CharT>::Type file(fileName);

    if (!file.good())
    {
        typename BasicErrorInfo<CharT>::Data errorData;
        errorData.code = ErrorCode::FileNotFound;
        errorData.srcLoc.col = 1;
        errorData.srcLoc.line = 1;
        errorData.srcLoc.fileName = fileName;
        errorData.extraParams.emplace_back(fileName);
        return MakeUnexpected(BasicErrorInfo<CharT>(std::move(errorData)));
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
        return MakeUnexpected(std::move(*result));
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
        return MakeUnexpected(std::move(*result));
    }
    return buffer;
}

template<typename CharT>
Result<void, CharT> BasicTemplate<CharT>::RenderTo(StringType& out, const ValuesMap& params) const
{
    TruncateOnError guard(out);
    return ToResult(guard.Done(GetImpl<CharT>(m_impl)->Render(out, params)));
}

template<typename CharT>
Result<void, CharT> BasicTemplate<CharT>::RenderToGeneric(StringType& out, const GenericMap& params) const
{
    TruncateOnError guard(out);
    return ToResult(guard.Done(GetImpl<CharT>(m_impl)->Render(out, params)));
}

template<typename CharT>
Result<void, CharT> BasicTemplate<CharT>::RenderTo(BasicOutputSink<CharT>& sink, const ValuesMap& params) const
{
    return ToResult(GetImpl<CharT>(m_impl)->Render(sink, params));
}

template<typename CharT>
Result<void, CharT> BasicTemplate<CharT>::RenderToGeneric(BasicOutputSink<CharT>& sink, const GenericMap& params) const
{
    return ToResult(GetImpl<CharT>(m_impl)->Render(sink, params));
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
Result<std::vector<std::string>, CharT> BasicTemplate<CharT>::FindUndeclaredVariables() const
{
    return GetImpl<CharT>(m_impl)->FindUndeclaredVariables();
}

template<typename CharT>
bool BasicTemplate<CharT>::IsEqual(const BasicTemplate& other) const
{
    return m_impl == other.m_impl;
}

bool ReleaseThreadCaches() noexcept
{
    RenderWorkspace* workspace = nullptr;
    try
    {
        workspace = &RenderWorkspace::ForThisThread();
    }
    catch (...)
    {
        // Out of memory making the workspace of a thread that never rendered: nothing to free
        return true;
    }
    if (!workspace->ReleaseCaches())
    {
        return false;
    }
    std::string().swap(detail::HtmlEscapeBuffer());
    return true;
}

#ifdef _WIN32
JINJA2CPP_SUPPRESS_DLL_INTERFACE_WARNINGS_BEGIN
template class JINJA2CPP_EXPORT BasicTemplate<char>;
template class JINJA2CPP_EXPORT BasicTemplate<wchar_t>;
JINJA2CPP_SUPPRESS_DLL_INTERFACE_WARNINGS_END
#else
// Exported by the explicit instantiation declarations in template.h
template class BasicTemplate<char>;
template class BasicTemplate<wchar_t>;
#endif

} // namespace v2
} // namespace jinja2
