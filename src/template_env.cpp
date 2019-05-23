#include <jinja2cpp/template.h>
#include <jinja2cpp/template_env.h>


namespace jinja2
{
template<typename CharT>
struct TemplateFunctions;

template<>
struct TemplateFunctions<char>
{
    using ResultType = nonstd::expected<Template, ErrorInfo>;
    static Template CreateTemplate(TemplateEnv* env)
    {
        return Template(env);
    }
    static auto LoadFile(const std::string& fileName, const IFilesystemHandler* fs)
    {
        return fs->OpenStream(fileName);
    }
};

template<>
struct TemplateFunctions<wchar_t>
{
    using ResultType = nonstd::expected<TemplateW, ErrorInfoW>;
    static TemplateW CreateTemplate(TemplateEnv* env)
    {
        return TemplateW(env);
    }
    static auto LoadFile(const std::string& fileName, const IFilesystemHandler* fs)
    {
        return fs->OpenWStream(fileName);
    }
};

template<typename CharT, typename T>
auto LoadTemplateImpl(TemplateEnv* env, std::string fileName, const T& filesystemHandlers)
{
    using Functions = TemplateFunctions<CharT>;
    using ResultType = typename Functions::ResultType;
	using ErrorType = typename ResultType::error_type;
    auto tpl = Functions::CreateTemplate(env);

    for (auto& fh : filesystemHandlers)
    {
        if (!fh.prefix.empty() && fileName.find(fh.prefix) != 0)
            continue;


        auto stream = Functions::LoadFile(fileName, fh.handler.get());
        if (stream)
        {
            auto res = tpl.Load(*stream);
            if (!res)
                return ResultType(res.get_unexpected());

			return ResultType(tpl);
        }
    }

	typename ErrorType::Data errorData;
	errorData.code = ErrorCode::FileNotFound;
	errorData.srcLoc.col = 1;
	errorData.srcLoc.line = 1;
	errorData.srcLoc.fileName = "";
	errorData.extraParams.push_back(Value(fileName));

    return ResultType(nonstd::make_unexpected(ErrorType(errorData)));
}

nonstd::expected<Template, ErrorInfo> TemplateEnv::LoadTemplate(std::string fileName)
{
    return LoadTemplateImpl<char>(this, std::move(fileName), m_filesystemHandlers);
}

nonstd::expected<TemplateW, ErrorInfoW> TemplateEnv::LoadTemplateW(std::string fileName)
{
    return LoadTemplateImpl<wchar_t>(this, std::move(fileName), m_filesystemHandlers);
}

} // jinja2
