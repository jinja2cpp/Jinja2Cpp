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

ParseResult Template::Load(const char* tpl, std::string tplName)
{
    std::string t(tpl);
    auto result = GetImpl<char>(m_impl)->Load(std::move(t), std::move(tplName));
    return !result ? ParseResult() : nonstd::make_unexpected(std::move(result.get()));
}

ParseResult Template::Load(const std::string& str, std::string tplName)
{
    auto result = GetImpl<char>(m_impl)->Load(str, std::move(tplName));
    return !result ? ParseResult() : nonstd::make_unexpected(std::move(result.get()));
}

ParseResult Template::Load(std::istream& stream, std::string tplName)
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

    auto result = GetImpl<char>(m_impl)->Load(std::move(t), std::move(tplName));
    return !result ? ParseResult() : nonstd::make_unexpected(std::move(result.get()));
}

ParseResult Template::LoadFromFile(const std::string& fileName)
{
    std::ifstream file(fileName);

    if (!file.good())
        return ParseResult();

    return Load(file, fileName);
}

void Template::Render(std::ostream& os, const jinja2::ValuesMap& params)
{
    GetImpl<char>(m_impl)->Render(os, params);
}

std::string Template::RenderAsString(const jinja2::ValuesMap& params)
{
    std::string outStr;
    outStr.reserve(10000);
    std::ostringstream os(outStr);
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

ParseResultW TemplateW::Load(const wchar_t* tpl, std::string tplName)
{
    std::wstring t(tpl);
    auto result = GetImpl<wchar_t>(m_impl)->Load(t, std::move(tplName));
    return !result ? ParseResultW() : nonstd::make_unexpected(std::move(result.get()));
}

ParseResultW TemplateW::Load(const std::wstring& str, std::string tplName)
{
    auto result = GetImpl<wchar_t>(m_impl)->Load(str, std::move(tplName));
    return !result ? ParseResultW() : nonstd::make_unexpected(std::move(result.get()));
}

ParseResultW TemplateW::Load(std::wistream& stream, std::string tplName)
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

    auto result = GetImpl<wchar_t>(m_impl)->Load(t, std::move(tplName));
    return !result ? ParseResultW() : nonstd::make_unexpected(std::move(result.get()));
}

ParseResultW TemplateW::LoadFromFile(const std::string& fileName)
{
    std::wifstream file(fileName);

    if (!file.good())
        return ParseResultW();

    return Load(file, fileName);
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
