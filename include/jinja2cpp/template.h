#ifndef JINJA2_TEMPLATE_H
#define JINJA2_TEMPLATE_H

#include <string>
#include <iostream>
#include <memory>

#include "value.h"

namespace jinja2
{
class ITemplateImpl;

class Template
{
public:
    Template();
    ~Template();

    bool Load(const char* tpl);
    bool Load(const std::string& str);
    bool Load(std::istream& stream);
    bool LoadFromFile(const std::string& fileName);

    void Render(std::ostream& os, const ValuesMap& params);
    std::string RenderAsString(const ValuesMap& params);

private:
    std::shared_ptr<ITemplateImpl> m_impl;
};


class TemplateW
{
public:
    TemplateW();
    ~TemplateW();

    bool Load(const wchar_t* tpl);
    bool Load(const std::wstring& str);
    bool Load(std::wistream& stream);
    bool LoadFromFile(const std::string& fileName);

    void Render(std::wostream& os, const ValuesMap& params);
    std::wstring RenderAsString(const ValuesMap& params);

private:
    std::shared_ptr<ITemplateImpl> m_impl;
};
} // jinja2

#endif // JINJA2_TEMPLATE_H
