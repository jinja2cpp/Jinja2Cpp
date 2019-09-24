#include "helpers.h"

#include <fmt/format.h>
#include <jinja2cpp/error_info.h>

namespace
{
template<typename FmtCtx>
struct ValueRenderer
{
    using CharT = typename FmtCtx::char_type;
    FmtCtx* ctx;

    explicit ValueRenderer(FmtCtx* c)
        : ctx(c)
    {
    }

    void operator()(bool val) const { fmt::format_to(ctx->out(), (val ? UNIVERSAL_STR("True") : UNIVERSAL_STR("False")).GetValue<CharT>()); }
    void operator()(const jinja2::EmptyValue&) const { fmt::format_to(ctx->out(), UNIVERSAL_STR("").GetValue<CharT>()); }
    template<typename CharU>
    void operator()(const std::basic_string<CharU>& val) const
    {
        fmt::format_to(ctx->out(), UNIVERSAL_STR("{}").GetValue<CharT>(), jinja2::ConvertString<std::basic_string<CharT>>(val));
    }

    template<typename CharU>
    void operator()(const nonstd::basic_string_view<CharU>& val) const
    {
        fmt::format_to(ctx->out(), UNIVERSAL_STR("{}").GetValue<CharT>(), jinja2::ConvertString<std::basic_string<CharT>>(val));
    }

    void operator()(const jinja2::ValuesList& vals) const
    {
        fmt::format_to(ctx->out(), UNIVERSAL_STR("{{").GetValue<CharT>());
        bool isFirst = true;
        for (auto& val : vals)
        {
            if (isFirst)
                isFirst = false;
            else
                fmt::format_to(ctx->out(), UNIVERSAL_STR(", ").GetValue<CharT>());
            nonstd::visit(ValueRenderer<FmtCtx>(ctx), val.data());
        }
        fmt::format_to(ctx->out(), UNIVERSAL_STR("}}").GetValue<CharT>());
    }

    void operator()(const jinja2::ValuesMap& vals) const
    {
        fmt::format_to(ctx->out(), UNIVERSAL_STR("{{").GetValue<CharT>());
        bool isFirst = true;
        for (auto& val : vals)
        {
            if (isFirst)
                isFirst = false;
            else
                fmt::format_to(ctx->out(), UNIVERSAL_STR(", ").GetValue<CharT>());

            fmt::format_to(ctx->out(), UNIVERSAL_STR("{{\"{}\",").GetValue<CharT>(), jinja2::ConvertString<std::basic_string<CharT>>(val.first));
            nonstd::visit(ValueRenderer<FmtCtx>(ctx), val.second.data());
            fmt::format_to(ctx->out(), UNIVERSAL_STR("}}").GetValue<CharT>());
        }
        fmt::format_to(ctx->out(), UNIVERSAL_STR("}}").GetValue<CharT>());
    }

    template<typename T>
    void operator()(const jinja2::RecWrapper<T>& val) const
    {
        this->operator()(const_cast<const T&>(*val.get()));
    }

    void operator()(const jinja2::GenericMap& /*val*/) const {}

    void operator()(const jinja2::GenericList& /*val*/) const {}

    void operator()(const jinja2::UserCallable& /*val*/) const {}

    template<typename T>
    void operator()(const T& val) const
    {
        fmt::format_to(ctx->out(), UNIVERSAL_STR("{}").GetValue<CharT>(), val);
    }
};
}

namespace fmt
{
template<typename CharT>
struct formatter<jinja2::Value, CharT>
{
    template<typename ParseContext>
    constexpr auto parse(ParseContext& ctx)
    {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(const jinja2::Value& val, FormatContext& ctx)
    {
        nonstd::visit(ValueRenderer<FormatContext>(&ctx), val.data());
        return fmt::format_to(ctx.out(), UNIVERSAL_STR("").GetValue<CharT>());
    }
};
}

namespace jinja2
{

template<typename CharT>
void RenderErrorInfo(std::basic_string<CharT>& result, const ErrorInfoTpl<CharT>& errInfo)
{
    using string_t = std::basic_string<CharT>;
    fmt::basic_memory_buffer<CharT> out;

    auto& loc = errInfo.GetErrorLocation();

    fmt::format_to(out, UNIVERSAL_STR("{}:{}:{}: error: ").GetValue<CharT>(), ConvertString<string_t>(loc.fileName), loc.line, loc.col);
    ErrorCode errCode = errInfo.GetCode();
    switch (errCode)
    {
    case ErrorCode::Unspecified:
            format_to(out, UNIVERSAL_STR("Parse error").GetValue<CharT>());
            break;
    case ErrorCode::UnexpectedException:
    {
        auto& extraParams = errInfo.GetExtraParams();
        format_to(out, UNIVERSAL_STR("Unexpected exception occurred during template processing. Exception: {}").GetValue<CharT>(), extraParams[0]);
        break;
    }
    case ErrorCode::YetUnsupported:
        format_to(out, UNIVERSAL_STR("This feature has not been supported yet").GetValue<CharT>());
        break;
    case ErrorCode::FileNotFound:
        format_to(out, UNIVERSAL_STR("File not found").GetValue<CharT>());
        break;
    case ErrorCode::ExpectedStringLiteral:
        format_to(out, UNIVERSAL_STR("String expected").GetValue<CharT>());
        break;
    case ErrorCode::ExpectedIdentifier:
        format_to(out, UNIVERSAL_STR("Identifier expected").GetValue<CharT>());
        break;
    case ErrorCode::ExpectedSquareBracket:
        format_to(out, UNIVERSAL_STR("']' expected").GetValue<CharT>());
        break;
    case ErrorCode::ExpectedRoundBracket:
        format_to(out, UNIVERSAL_STR("')' expected").GetValue<CharT>());
        break;
    case ErrorCode::ExpectedCurlyBracket:
        format_to(out, UNIVERSAL_STR("'}}' expected").GetValue<CharT>());
        break;
    case ErrorCode::ExpectedToken:
    {
        auto& extraParams = errInfo.GetExtraParams();
        format_to(out, UNIVERSAL_STR("Unexpected token '{}'").GetValue<CharT>(), extraParams[0]);
        if (extraParams.size() > 1)
        {
            format_to(out, UNIVERSAL_STR(". Expected: ").GetValue<CharT>());
            for (std::size_t i = 1; i < extraParams.size(); ++ i)
            {
                if (i != 1)
                    format_to(out, UNIVERSAL_STR(", ").GetValue<CharT>());
                format_to(out, UNIVERSAL_STR("\'{}\'").GetValue<CharT>(), extraParams[i]);
            }
        }
        break;
    }
    case ErrorCode::ExpectedExpression:
    {
        auto& extraParams = errInfo.GetExtraParams();
        format_to(out, UNIVERSAL_STR("Expected expression, got: '{}'").GetValue<CharT>(), extraParams[0]);
        break;
    }
    case ErrorCode::ExpectedEndOfStatement:
    {
        auto& extraParams = errInfo.GetExtraParams();
        format_to(out, UNIVERSAL_STR("Expected end of statement, got: '{}'").GetValue<CharT>(), extraParams[0]);
        break;
    }
    case ErrorCode::UnexpectedToken:
    {
        auto& extraParams = errInfo.GetExtraParams();
        format_to(out, UNIVERSAL_STR("Unexpected token: '{}'").GetValue<CharT>(), extraParams[0]);
        break;
    }
    case ErrorCode::UnexpectedStatement:
    {
        auto& extraParams = errInfo.GetExtraParams();
        format_to(out, UNIVERSAL_STR("Unexpected statement: '{}'").GetValue<CharT>(), extraParams[0]);
        break;
    }
    case ErrorCode::UnexpectedCommentBegin:
        format_to(out, UNIVERSAL_STR("Unexpected comment begin").GetValue<CharT>());
        break;
    case ErrorCode::UnexpectedCommentEnd:
        format_to(out, UNIVERSAL_STR("Unexpected comment end").GetValue<CharT>());
        break;
    case ErrorCode::UnexpectedExprBegin:
        format_to(out, UNIVERSAL_STR("Unexpected expression block begin").GetValue<CharT>());
        break;
    case ErrorCode::UnexpectedExprEnd:
        format_to(out, UNIVERSAL_STR("Unexpected expression block end").GetValue<CharT>());
        break;
    case ErrorCode::UnexpectedStmtBegin:
        format_to(out, UNIVERSAL_STR("Unexpected statement block begin").GetValue<CharT>());
        break;
    case ErrorCode::UnexpectedStmtEnd:
        format_to(out, UNIVERSAL_STR("Unexpected statement block end").GetValue<CharT>());
        break;
    case ErrorCode::TemplateNotParsed:
        format_to(out, UNIVERSAL_STR("Template not parsed").GetValue<CharT>());
        break;
    case ErrorCode::TemplateNotFound:
        format_to(out, UNIVERSAL_STR("Template(s) not found: {}").GetValue<CharT>(), errInfo.GetExtraParams()[0]);
        break;
    case ErrorCode::InvalidTemplateName:
        format_to(out, UNIVERSAL_STR("Invalid template name: {}").GetValue<CharT>(), errInfo.GetExtraParams()[0]);
        break;
    case ErrorCode::InvalidValueType:
        format_to(out, UNIVERSAL_STR("Invalid value type").GetValue<CharT>());
        break;
    case ErrorCode::ExtensionDisabled:
        format_to(out, UNIVERSAL_STR("Extension disabled").GetValue<CharT>());
        break;
    case ErrorCode::TemplateEnvAbsent:
        format_to(out, UNIVERSAL_STR("Template environment doesn't set").GetValue<CharT>());
        break;
    }
    format_to(out, UNIVERSAL_STR("\n{}").GetValue<CharT>(), errInfo.GetLocationDescr());
    result = to_string(out);
}

template<>
std::string ErrorInfoTpl<char>::ToString() const
{
    std::string result;
    RenderErrorInfo(result, *this);
    return result;
}

template<>
std::wstring ErrorInfoTpl<wchar_t>::ToString() const
{
    std::wstring result;
    RenderErrorInfo(result, *this);
    return result;
}

std::ostream& operator << (std::ostream& os, const ErrorInfo& res)
{
    os << res.ToString();
    return os;
}
std::wostream& operator << (std::wostream& os, const ErrorInfoW& res)
{
    os << res.ToString();
    return os;
}
} // jinja2
