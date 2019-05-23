#ifndef JINJA2_TEMPLATE_H
#define JINJA2_TEMPLATE_H

#include "error_info.h"
#include "value.h"

#include <nonstd/expected.hpp>

#include <string>
#include <iostream>
#include <memory>

namespace jinja2
{
class ITemplateImpl;
class TemplateEnv;
template<typename CharT> class TemplateImpl;
template<typename U>
using Result = nonstd::expected<U, ErrorInfo>;
template<typename U>
using ResultW = nonstd::expected<U, ErrorInfoW>;

class Template
{
public:
    Template() : Template(nullptr) {}
    explicit Template(TemplateEnv* env);
    ~Template();

    Result<void> Load(const char* tpl, std::string tplName = std::string());
    Result<void> Load(const std::string& str, std::string tplName = std::string());
    Result<void> Load(std::istream& stream, std::string tplName = std::string());
    Result<void> LoadFromFile(const std::string& fileName);

    Result<void> Render(std::ostream& os, const ValuesMap& params);
    Result<std::string> RenderAsString(const ValuesMap& params);

private:
    std::shared_ptr<ITemplateImpl> m_impl;
    friend class TemplateImpl<char>;
};


class TemplateW
{
public:
    TemplateW() : TemplateW(nullptr) {}
    explicit TemplateW(TemplateEnv* env);
    ~TemplateW();

    ResultW<void> Load(const wchar_t* tpl, std::string tplName = std::string());
    ResultW<void> Load(const std::wstring& str, std::string tplName = std::string());
    ResultW<void> Load(std::wistream& stream, std::string tplName = std::string());
    ResultW<void> LoadFromFile(const std::string& fileName);

    ResultW<void> Render(std::wostream& os, const ValuesMap& params);
    ResultW<std::wstring> RenderAsString(const ValuesMap& params);

private:
    std::shared_ptr<ITemplateImpl> m_impl;
    friend class TemplateImpl<wchar_t>;
};
} // jinja2

#endif // JINJA2_TEMPLATE_H
