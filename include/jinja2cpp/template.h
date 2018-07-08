#ifndef JINJA2_TEMPLATE_H
#define JINJA2_TEMPLATE_H

#include <string>
#include <iostream>
#include <memory>

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

    bool Load(const char* tpl);
    bool Load(const std::string& str);
    bool Load(std::istream& stream);
    bool LoadFromFile(const std::string& fileName);

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

    bool Load(const wchar_t* tpl);
    bool Load(const std::wstring& str);
    bool Load(std::wistream& stream);
    bool LoadFromFile(const std::string& fileName);

    void Render(std::wostream& os, const ValuesMap& params);
    std::wstring RenderAsString(const ValuesMap& params);

private:
    std::shared_ptr<ITemplateImpl> m_impl;
    friend class TemplateImpl<wchar_t>;
};
} // jinja2

#endif // JINJA2_TEMPLATE_H
