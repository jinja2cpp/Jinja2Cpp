#include <jinja2cpp/error_info.h>
#include "helpers.h"

namespace jinja2
{

namespace
{
template<typename CharT>
struct ValueRenderer
{
    std::basic_ostream<CharT>& os;

    ValueRenderer(std::basic_ostream<CharT>& osRef) : os(osRef) {}

    void operator() (bool val) const
    {
        os << (val ? UNIVERSAL_STR("True") : UNIVERSAL_STR("False"));
    }
    void operator() (const EmptyValue&) const
    {
    }
    template<typename CharU>
    void operator() (const std::basic_string<CharU>& val) const
    {
        os << ConvertString<std::basic_string<CharT>>(val);
    }

    void operator() (const ValuesList& vals) const
    {
        os << '{';
        bool isFirst = true;
        for (auto& val : vals)
        {
            if (isFirst)
                isFirst = false;
            else
                os << UNIVERSAL_STR(", ");

            nonstd::visit(ValueRenderer<CharT>(os), val.data());
        }
        os << '}';
    }

    void operator() (const ValuesMap& vals) const
    {
        os << '{';
        bool isFirst = true;
        for (auto& val : vals)
        {
            if (isFirst)
                isFirst = false;
            else
                os << UNIVERSAL_STR(", ");

            os << '{' << '"' << ConvertString<std::basic_string<CharT>>(val.first) << '"' << ',';
            nonstd::visit(ValueRenderer<CharT>(os), val.second.data());
            os << '}';
        }
        os << '}';
    }


    template<typename T>
    void operator()(const RecWrapper<T>& val) const
    {
        return this->operator()(const_cast<const T&>(*val.get()));
    }

    void operator() (const GenericMap& /*val*/) const
    {
    }

    void operator() (const GenericList& /*val*/) const
    {
    }


    void operator() (const UserCallable& val) const
    {
    }

    template<typename T>
    void operator() (const T& val) const
    {
        os << val;
    }
};

template<typename CharT>
std::basic_ostream<CharT>& operator << (std::basic_ostream<CharT>& os, Value val)
{
    nonstd::visit(ValueRenderer<CharT>(os), val.data());
    return os;
}
}

template<typename CharT>
void RenderErrorInfo(std::basic_ostream<CharT>& os, const ErrorInfoTpl<CharT>& errInfo)
{
    using string_t = std::basic_string<CharT>;

    auto& loc = errInfo.GetErrorLocation();
    os << ConvertString<string_t>(loc.fileName) << ':' << loc.line << ':' << loc.col << ':';
    os << UNIVERSAL_STR(" error: ");
    ErrorCode errCode = errInfo.GetCode();
    switch (errCode)
    {
    case ErrorCode::Unspecified:
        os << UNIVERSAL_STR("Parse error");
        break;
    case ErrorCode::UnexpectedException:
        os << UNIVERSAL_STR("Unexpected exception occurred during template processing");
        break;
    case ErrorCode::YetUnsupported:
        os << UNIVERSAL_STR("This feature has not been supported yet");
        break;
    case ErrorCode::FileNotFound:
        os << UNIVERSAL_STR("File not found");
        break;
    case ErrorCode::ExpectedStringLiteral:
        os << UNIVERSAL_STR("String expected");
        break;
    case ErrorCode::ExpectedIdentifier:
        os << UNIVERSAL_STR("Identifier expected");
        break;
    case ErrorCode::ExpectedSquareBracket:
        os << UNIVERSAL_STR("']' expected");
        break;
    case ErrorCode::ExpectedRoundBracket:
        os << UNIVERSAL_STR("')' expected");
        break;
    case ErrorCode::ExpectedCurlyBracket:
        os << UNIVERSAL_STR("'}' expected");
        break;
    case ErrorCode::ExpectedToken:
    {
        auto& extraParams = errInfo.GetExtraParams();
        os << UNIVERSAL_STR("Unexpected token '") << extraParams[0] << '\'';
        if (extraParams.size() > 1)
        {
            os << UNIVERSAL_STR(". Expected: ");
            for (std::size_t i = 1; i < extraParams.size(); ++ i)
            {
                if (i != 1)
                    os << UNIVERSAL_STR(", ");
                os << '\'' << extraParams[i] << '\'';
            }
        }
        break;
    }
    case ErrorCode::ExpectedExpression:
    {
        auto& extraParams = errInfo.GetExtraParams();
        os << UNIVERSAL_STR("Expected expression, got: '") << extraParams[0] << '\'';
        break;
    }
    case ErrorCode::ExpectedEndOfStatement:
    {
        auto& extraParams = errInfo.GetExtraParams();
        os << UNIVERSAL_STR("Expected end of statement, got: '") << extraParams[0] << '\'';
        break;
    }
    case ErrorCode::UnexpectedToken:
    {
        auto& extraParams = errInfo.GetExtraParams();
        os << UNIVERSAL_STR("Unexpected token: '") << extraParams[0] << '\'';
        break;
    }
    case ErrorCode::UnexpectedStatement:
    {
        auto& extraParams = errInfo.GetExtraParams();
        os << UNIVERSAL_STR("Unexpected statement: '") << extraParams[0] << '\'';
        break;
    }
    case ErrorCode::UnexpectedCommentBegin:
        os << UNIVERSAL_STR("Unexpected comment begin");
        break;
    case ErrorCode::UnexpectedCommentEnd:
        os << UNIVERSAL_STR("Unexpected comment end");
        break;
    case ErrorCode::UnexpectedExprBegin:
        os << UNIVERSAL_STR("Unexpected expression block begin");
        break;
    case ErrorCode::UnexpectedExprEnd:
        os << UNIVERSAL_STR("Unexpected expression block end");
        break;
    case ErrorCode::UnexpectedStmtBegin:
        os << UNIVERSAL_STR("Unexpected statement block begin");
        break;
    case ErrorCode::UnexpectedStmtEnd:
        os << UNIVERSAL_STR("Unexpected statement block end");
        break;
    case ErrorCode::TemplateNotParsed:
        os << UNIVERSAL_STR("Template not parsed");
        break;
    case ErrorCode::TemplateNotFound:
        os << UNIVERSAL_STR("Template(s) not found: ") << errInfo.GetExtraParams()[0];
        break;
    case ErrorCode::InvalidValueType:
        os << UNIVERSAL_STR("Invalid value type");
        break;
    case ErrorCode::ExtensionDisabled:
        os << UNIVERSAL_STR("Extension disabled");
        break;
    case ErrorCode::TemplateEnvAbsent:
        os << UNIVERSAL_STR("Template environment doesn't set");
        break;
    }
    os << std::endl << errInfo.GetLocationDescr();
}

std::ostream& operator << (std::ostream& os, const ErrorInfo& res)
{
    RenderErrorInfo(os, res);
    return os;
}
std::wostream& operator << (std::wostream& os, const ErrorInfoW& res)
{
    RenderErrorInfo(os, res);
    return os;
}
} // jinja2
