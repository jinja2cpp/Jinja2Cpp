#include <jinja2cpp/error_info.h>
#include "helpers.h"

namespace jinja2
{

namespace
{
template<typename CharT>
struct ValueRenderer : boost::static_visitor<void>
{
    std::basic_ostream<CharT>& os;

    ValueRenderer(std::basic_ostream<CharT>& osRef) : os(osRef) {}

    void operator() (bool val) const
    {
        os << (val ? MULTI_STR_LITERAL("True") : MULTI_STR_LITERAL("False"));
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
                os << MULTI_STR_LITERAL(", ");

            boost::apply_visitor(ValueRenderer<CharT>(os), val.data());
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
                os << MULTI_STR_LITERAL(", ");

            os << '{' << '"' << ConvertString<std::basic_string<CharT>>(val.first) << '"' << ',';
            boost::apply_visitor(ValueRenderer<CharT>(os), val.second.data());
            os << '}';
        }
        os << '}';
    }

    void operator() (const GenericMap& /*val*/) const
    {
    }

    void operator() (const GenericList& /*val*/) const
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
    boost::apply_visitor(ValueRenderer<CharT>(os), val.data());
    return os;
}
}

template<typename CharT>
void RenderErrorInfo(std::basic_ostream<CharT>& os, const ErrorInfoTpl<CharT>& errInfo)
{
    using string_t = std::basic_string<CharT>;

    auto& loc = errInfo.GetErrorLocation();
    os << ConvertString<string_t>(loc.fileName) << ':' << loc.line << ':' << loc.col << ':';
    os << MULTI_STR_LITERAL(" error: ");
    ErrorCode errCode = errInfo.GetCode();
    switch (errCode)
    {
    case ErrorCode::Unspecified:
        os << MULTI_STR_LITERAL("Parse error");
        break;
    case ErrorCode::UnexpectedException:
        os << MULTI_STR_LITERAL("Unexpected exception occurred during template processing");
        break;
    case ErrorCode::YetUnsupported:
        os << MULTI_STR_LITERAL("This feature has not been supported yet");
        break;
    case ErrorCode::FileNotFound:
        os << MULTI_STR_LITERAL("File not found");
        break;
    case ErrorCode::ExpectedStringLiteral:
        os << MULTI_STR_LITERAL("String expected");
        break;
    case ErrorCode::ExpectedIdentifier:
        os << MULTI_STR_LITERAL("Identifier expected");
        break;
    case ErrorCode::ExpectedSquareBracket:
        os << MULTI_STR_LITERAL("']' expected");
        break;
    case ErrorCode::ExpectedRoundBracket:
        os << MULTI_STR_LITERAL("')' expected");
        break;
    case ErrorCode::ExpectedCurlyBracket:
        os << MULTI_STR_LITERAL("'}' expected");
        break;
    case ErrorCode::ExpectedToken:
    {
        auto& extraParams = errInfo.GetExtraParams();
        os << MULTI_STR_LITERAL("Unexpected token '") << extraParams[0] << '\'';
        if (extraParams.size() > 1)
        {
            os << MULTI_STR_LITERAL(". Expected: ");
            for (int i = 1; i < extraParams.size(); ++ i)
            {
                if (i != 1)
                    os << MULTI_STR_LITERAL(", ");
                os << '\'' << extraParams[i] << '\'';
            }
        }
        break;
    }
    case ErrorCode::ExpectedExpression:
        os << MULTI_STR_LITERAL("Expression expected");
        break;
    case ErrorCode::ExpectedEndOfStatement:
    case ErrorCode::UnexpectedToken:
    case ErrorCode::UnexpectedStatement:
    case ErrorCode::UnexpectedCommentBegin:
    case ErrorCode::UnexpectedCommentEnd:
    case ErrorCode::UnexpectedExprBegin:
    case ErrorCode::UnexpectedExprEnd:
    case ErrorCode::UnexpectedStmtBegin:
    case ErrorCode::UnexpectedStmtEnd:
        break;
    }
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
