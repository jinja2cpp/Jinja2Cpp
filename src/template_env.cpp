#include <jinja2cpp/template.h>
#include <jinja2cpp/template_env.h>


namespace jinja2
{
template<typename CharT>
struct TemplateFunctions;

template<>
struct TemplateFunctions<char>
{
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
    auto result = Functions::CreateTemplate(env);

    for (auto& fh : filesystemHandlers)
    {
        if (!fh.prefix.empty() && fileName.find(fh.prefix) != 0)
            continue;


        auto stream = Functions::LoadFile(fileName, fh.handler.get());
        if (!stream)
        {
            result.Load(*stream);
            break;
        }
    }

    return result;
}

Template TemplateEnv::LoadTemplate(std::string fileName)
{
    return LoadTemplateImpl<char>(this, std::move(fileName), m_filesystemHandlers);
}

TemplateW TemplateEnv::LoadTemplateW(std::string fileName)
{
    return LoadTemplateImpl<wchar_t>(this, std::move(fileName), m_filesystemHandlers);
}

} // jinja2
