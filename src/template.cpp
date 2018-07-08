#include "jinja2cpp/template.h"
#include "template_impl.h"

#include <fstream>
#include <sstream>

namespace jinja2
{

template<typename CharT>
auto GetImpl(std::shared_ptr<ITemplateImpl> impl)
{
    return static_cast<TemplateImpl<CharT>*>(impl.get());
}

Template::Template(TemplateEnv* env)
    : m_impl(new TemplateImpl<char>(env))
{

}

Template::~Template()
{

}

bool Template::Load(const char* tpl)
{
    std::string t(tpl);
    return GetImpl<char>(m_impl)->Load(std::move(t));
}

bool Template::Load(const std::string& str)
{
    return GetImpl<char>(m_impl)->Load(str);
}

bool Template::Load(std::istream& stream)
{
    std::string t;

    while (stream.good() && !stream.eof())
    {
        char buff[0x10000];
        stream.read(buff, sizeof(buff));
        auto read = stream.gcount();
        if (read)
            t.append(buff, buff + read);
    }

    return GetImpl<char>(m_impl)->Load(std::move(t));
}

bool Template::LoadFromFile(const std::string& fileName)
{
    std::ifstream file(fileName);

    if (!file.good())
        return false;

    return Load(file);
}

void Template::Render(std::ostream& os, const jinja2::ValuesMap& params)
{
    GetImpl<char>(m_impl)->Render(os, params);
}

std::string Template::RenderAsString(const jinja2::ValuesMap& params)
{
    std::ostringstream os;
    Render(os, params);

    return os.str();
}

TemplateW::TemplateW(TemplateEnv* env)
    : m_impl(new TemplateImpl<wchar_t>(env))
{

}

TemplateW::~TemplateW()
{

}

bool TemplateW::Load(const wchar_t* tpl)
{
    std::wstring t(tpl);
    return GetImpl<wchar_t>(m_impl)->Load(t);
}

bool TemplateW::Load(const std::wstring& str)
{
    return GetImpl<wchar_t>(m_impl)->Load(str);
}

bool TemplateW::Load(std::wistream& stream)
{
    std::wstring t;

    while (stream.good() && !stream.eof())
    {
        wchar_t buff[0x10000];
        stream.read(buff, sizeof(buff));
        auto read = stream.gcount();
        if (read)
            t.append(buff, buff + read);
    }

    return GetImpl<wchar_t>(m_impl)->Load(t);
}

bool TemplateW::LoadFromFile(const std::string& fileName)
{
    std::wifstream file(fileName);

    if (!file.good())
        return false;

    return Load(file);
}

void TemplateW::Render(std::wostream& os, const jinja2::ValuesMap& params)
{
    GetImpl<wchar_t>(m_impl)->Render(os, params);
}

std::wstring TemplateW::RenderAsString(const jinja2::ValuesMap& params)
{
    std::wostringstream os;
    GetImpl<wchar_t>(m_impl)->Render(os, params);

    return os.str();
}
} // jinga2
