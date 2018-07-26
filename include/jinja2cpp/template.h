#ifndef JINJA2_TEMPLATE_H
#define JINJA2_TEMPLATE_H

#include <string>
#include <iostream>
#include <memory>

#include "parse_result.h"
#include "value.h"

namespace jinja2
{
class ITemplateImpl;
class TemplateEnv;
template<typename CharT> class TemplateImpl;

class Template
{
public:
    Template(TemplateEnv* env = nullptr);
    ~Template();

    ParseResult Load(const char* tpl, std::string tplName = std::string());
    ParseResult Load(const std::string& str, std::string tplName = std::string());
    ParseResult Load(std::istream& stream, std::string tplName = std::string());
    ParseResult LoadFromFile(const std::string& fileName);

    void Render(std::ostream& os, const ValuesMap& params);
    std::string RenderAsString(const ValuesMap& params);

private:
    std::shared_ptr<ITemplateImpl> m_impl;
    friend class TemplateImpl<char>;
};


class TemplateW
{
public:
    TemplateW(TemplateEnv* env = nullptr);
    ~TemplateW();

    ParseResultW Load(const wchar_t* tpl, std::string tplName = std::string());
    ParseResultW Load(const std::wstring& str, std::string tplName = std::string());
    ParseResultW Load(std::wistream& stream, std::string tplName = std::string());
    ParseResultW LoadFromFile(const std::string& fileName);

    void Render(std::wostream& os, const ValuesMap& params);
    std::wstring RenderAsString(const ValuesMap& params);

private:
    std::shared_ptr<ITemplateImpl> m_impl;
    friend class TemplateImpl<wchar_t>;
};
} // jinja2

#endif // JINJA2_TEMPLATE_H
