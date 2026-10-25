#include <jinja2cpp/error_info.h>
#include <jinja2cpp/exception.h>

#include "helpers.h"

#include <jinja2cpp/big_int.h>
#include <jinja2cpp/generic_list.h>
#include <jinja2cpp/markup.h>
#include <jinja2cpp/string_helpers.h>
#include <jinja2cpp/value.h>

#include <fmt/format.h>
#include <fmt/xchar.h>

#include <cstddef>
#include <iterator>
#include <ostream>
#include <string>
#include <string_view>
#include <system_error>

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

    constexpr void operator()(bool val) const
    {
        fmt::format_to(
            ctx->out(),
            UNIVERSAL_STR("{}").GetValue<CharT>(),
            (val ? UNIVERSAL_STR("True").GetValue<CharT>() : UNIVERSAL_STR("False").GetValue<CharT>()));
    }
    void operator()(const jinja2::NoneValue&) const { fmt::format_to(ctx->out(), UNIVERSAL_STR("").GetValue<CharT>()); }
    template<typename CharU>
    void operator()(const std::basic_string<CharU>& val) const
    {
        fmt::format_to(ctx->out(), UNIVERSAL_STR("{}").GetValue<CharT>(), jinja2::ConvertString<std::basic_string<CharT>>(val));
    }

    // A Markup prints as its text; the generic overload below would hand fmt the Markup
    template<typename CharU>
    void operator()(const jinja2::BasicMarkup<CharU>& val) const
    {
        this->operator()(val.str());
    }

    template<typename CharU>
    void operator()(const std::basic_string_view<CharU>& val) const
    {
        fmt::format_to(ctx->out(), UNIVERSAL_STR("{}").GetValue<CharT>(), jinja2::ConvertString<std::basic_string<CharT>>(val));
    }

    void operator()(const jinja2::ValuesList& vals) const
    {
        fmt::format_to(ctx->out(), UNIVERSAL_STR("{{").GetValue<CharT>());
        bool isFirst = true;
        for (const auto& val : vals)
        {
            if (isFirst)
            {
                isFirst = false;
            }
            else
            {
                fmt::format_to(ctx->out(), UNIVERSAL_STR(", ").GetValue<CharT>());
            }
            std::visit(ValueRenderer<FmtCtx>(ctx), val.GetData());
        }
        fmt::format_to(ctx->out(), UNIVERSAL_STR("}}").GetValue<CharT>());
    }

    void operator()(const jinja2::ValuesMap& vals) const
    {
        fmt::format_to(ctx->out(), UNIVERSAL_STR("{{").GetValue<CharT>());
        bool isFirst = true;
        for (const auto& val : vals)
        {
            if (isFirst)
            {
                isFirst = false;
            }
            else
            {
                fmt::format_to(ctx->out(), UNIVERSAL_STR(", ").GetValue<CharT>());
            }

            fmt::format_to(ctx->out(), UNIVERSAL_STR("{{\"{}\",").GetValue<CharT>(), jinja2::ConvertString<std::basic_string<CharT>>(val.first));
            std::visit(ValueRenderer<FmtCtx>(ctx), val.second.GetData());
            fmt::format_to(ctx->out(), UNIVERSAL_STR("}}").GetValue<CharT>());
        }
        fmt::format_to(ctx->out(), UNIVERSAL_STR("}}").GetValue<CharT>());
    }

    template<typename T>
    void operator()(const jinja2::RecWrapper<T>& val) const
    {
        this->operator()(const_cast<const T&>(*val));
    }

    void operator()(const jinja2::GenericMap& /*val*/) const {}

    void operator()(const jinja2::GenericList& /*val*/) const {}

    void operator()(const jinja2::UserCallable& /*val*/) const {}

    void operator()(const jinja2::BigInt& val) const { this->operator()(val.ToString()); }

    template<typename T>
    void operator()(const T& val) const
    {
        fmt::format_to(ctx->out(), UNIVERSAL_STR("{}").GetValue<CharT>(), val);
    }
};
} // namespace

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
    auto format(const jinja2::Value& val, FormatContext& ctx) const
    {
        std::visit(ValueRenderer<FormatContext>(&ctx), val.GetData());
        return fmt::format_to(ctx.out(), UNIVERSAL_STR("").GetValue<CharT>());
    }
};
} // namespace fmt

namespace jinja2
{
inline namespace v2
{
Exception::~Exception() = default;

namespace
{
const char* CodeDescription(ErrorCode code) noexcept
{
    switch (code)
    {
    case ErrorCode::Unspecified:
        return "unspecified error";
    case ErrorCode::UnexpectedException:
        return "unexpected exception";
    case ErrorCode::YetUnsupported:
        return "feature not supported yet";
    case ErrorCode::FileNotFound:
        return "file not found";
    case ErrorCode::ExtensionDisabled:
        return "extension disabled";
    case ErrorCode::TemplateEnvAbsent:
        return "template environment not set";
    case ErrorCode::TemplateNotFound:
        return "template not found";
    case ErrorCode::TemplateNotParsed:
        return "template not parsed";
    case ErrorCode::InvalidValueType:
        return "invalid value type";
    case ErrorCode::InvalidTemplateName:
        return "invalid template name";
    case ErrorCode::MetadataParseError:
        return "template metadata parse error";
    case ErrorCode::UndefinedError:
        return "undefined value";
    case ErrorCode::RecursionLimitExceeded:
        return "maximum recursion depth exceeded";
    case ErrorCode::UnknownFilter:
        return "unknown filter";
    case ErrorCode::UnknownTest:
        return "unknown test";
    case ErrorCode::UserCallableException:
        return "exception thrown by a user callable";
    case ErrorCode::IntegerOutOfRange:
        return "integer out of range";
    case ErrorCode::TemplateRuntimeError:
        return "template runtime error";
    case ErrorCode::ResourceLimitExceeded:
        return "resource limit exceeded";
    case ErrorCode::TemplateExpired:
        return "template expired";
    case ErrorCode::DataReadError:
        return "data read error";
    case ErrorCode::ExpectedStringLiteral:
        return "string literal expected";
    case ErrorCode::ExpectedIdentifier:
        return "identifier expected";
    case ErrorCode::ExpectedSquareBracket:
        return "']' expected";
    case ErrorCode::ExpectedRoundBracket:
        return "')' expected";
    case ErrorCode::ExpectedCurlyBracket:
        return "'}' expected";
    case ErrorCode::ExpectedToken:
        return "token expected";
    case ErrorCode::ExpectedExpression:
        return "expression expected";
    case ErrorCode::ExpectedEndOfStatement:
        return "end of statement expected";
    case ErrorCode::ExpectedRawEnd:
        return "end of raw block expected";
    case ErrorCode::ExpectedMetaEnd:
        return "end of meta block expected";
    case ErrorCode::UnexpectedToken:
        return "unexpected token";
    case ErrorCode::UnexpectedStatement:
        return "unexpected statement";
    case ErrorCode::UnexpectedCommentBegin:
        return "unexpected comment begin";
    case ErrorCode::UnexpectedCommentEnd:
        return "unexpected comment end";
    case ErrorCode::UnexpectedExprBegin:
        return "unexpected expression block begin";
    case ErrorCode::UnexpectedExprEnd:
        return "unexpected expression block end";
    case ErrorCode::UnexpectedStmtBegin:
        return "unexpected statement block begin";
    case ErrorCode::UnexpectedStmtEnd:
        return "unexpected statement block end";
    case ErrorCode::UnexpectedRawBegin:
        return "unexpected raw block begin";
    case ErrorCode::UnexpectedRawEnd:
        return "unexpected raw block end";
    case ErrorCode::UnexpectedMetaBegin:
        return "unexpected meta block begin";
    case ErrorCode::UnexpectedMetaEnd:
        return "unexpected meta block end";
    }
    return "unknown error";
}

class ErrorCategory final : public std::error_category
{
public:
    [[nodiscard]] const char* name() const noexcept override { return "jinja2cpp"; }
    [[nodiscard]] std::string message(int code) const override { return CodeDescription(static_cast<ErrorCode>(code)); }
    [[nodiscard]] std::error_condition default_error_condition(int code) const noexcept override
    {
        switch (static_cast<ErrorCode>(code))
        {
        case ErrorCode::FileNotFound:
        case ErrorCode::TemplateNotFound:
            return std::errc::no_such_file_or_directory;
        case ErrorCode::IntegerOutOfRange:
        case ErrorCode::ResourceLimitExceeded:
            return std::errc::value_too_large;
        case ErrorCode::InvalidValueType:
        case ErrorCode::InvalidTemplateName:
            return std::errc::invalid_argument;
        default:
            return { code, *this };
        }
    }
};
} // namespace

const std::error_category& error_category() noexcept
{
    static const ErrorCategory category;
    return category;
}

template<typename CharT>
void RenderErrorInfo(std::basic_string<CharT>& result, const BasicErrorInfo<CharT>& errInfo)
{
    using string_t = std::basic_string<CharT>;
    auto out = fmt::basic_memory_buffer<CharT>();

    auto& loc = errInfo.GetErrorLocation();

    fmt::format_to(std::back_inserter(out), UNIVERSAL_STR("{}:{}:{}: error: ").GetValue<CharT>(), ConvertString<string_t>(loc.fileName), loc.line, loc.col);
    ErrorCode errCode = errInfo.GetCode();
    switch (errCode)
    {
    case ErrorCode::Unspecified:
        fmt::format_to(std::back_inserter(out), UNIVERSAL_STR("Parse error").GetValue<CharT>());
        break;
    case ErrorCode::UnexpectedException:
    {
        auto& extraParams = errInfo.GetExtraParams();
        fmt::format_to(std::back_inserter(out), UNIVERSAL_STR("Unexpected exception occurred during template processing. Exception: {}").GetValue<CharT>(), extraParams[0]);
        break;
    }
    case ErrorCode::MetadataParseError:
    {
        auto& extraParams = errInfo.GetExtraParams();
        fmt::format_to(std::back_inserter(out), UNIVERSAL_STR("Error occurred during template metadata parsing. Error: {}").GetValue<CharT>(), extraParams[0]);
        break;
    }
    case ErrorCode::UndefinedError:
        fmt::format_to(std::back_inserter(out), UNIVERSAL_STR("Undefined value: {}").GetValue<CharT>(), errInfo.GetExtraParams()[0]);
        break;
    case ErrorCode::RecursionLimitExceeded:
        fmt::format_to(std::back_inserter(out), UNIVERSAL_STR("Maximum recursion depth exceeded").GetValue<CharT>());
        break;
    case ErrorCode::UnknownFilter:
        fmt::format_to(std::back_inserter(out), UNIVERSAL_STR("No filter named '{}'.").GetValue<CharT>(), errInfo.GetExtraParams()[0]);
        break;
    case ErrorCode::UnknownTest:
        fmt::format_to(std::back_inserter(out), UNIVERSAL_STR("No test named '{}'.").GetValue<CharT>(), errInfo.GetExtraParams()[0]);
        break;
    case ErrorCode::UserCallableException:
        fmt::format_to(std::back_inserter(out), UNIVERSAL_STR("Exception thrown by a user callable: {}").GetValue<CharT>(), errInfo.GetExtraParams()[0]);
        break;
    case ErrorCode::IntegerOutOfRange:
        fmt::format_to(std::back_inserter(out), UNIVERSAL_STR("Integer out of range: {}").GetValue<CharT>(), errInfo.GetExtraParams()[0]);
        break;
    case ErrorCode::TemplateRuntimeError:
        fmt::format_to(std::back_inserter(out), UNIVERSAL_STR("{}").GetValue<CharT>(), errInfo.GetExtraParams()[0]);
        break;
    case ErrorCode::ResourceLimitExceeded:
        fmt::format_to(std::back_inserter(out), UNIVERSAL_STR("Resource limit exceeded: {}").GetValue<CharT>(), errInfo.GetExtraParams()[0]);
        break;
    case ErrorCode::TemplateExpired:
        fmt::format_to(std::back_inserter(out), UNIVERSAL_STR("The template of macro '{}' was loaded again since its module was made").GetValue<CharT>(), errInfo.GetExtraParams()[0]);
        break;
    case ErrorCode::DataReadError:
        fmt::format_to(std::back_inserter(out), UNIVERSAL_STR("Data read error: {}").GetValue<CharT>(), errInfo.GetExtraParams()[0]);
        break;
    case ErrorCode::YetUnsupported:
        fmt::format_to(std::back_inserter(out), UNIVERSAL_STR("This feature has not been supported yet").GetValue<CharT>());
        break;
    case ErrorCode::FileNotFound:
        fmt::format_to(std::back_inserter(out), UNIVERSAL_STR("File not found").GetValue<CharT>());
        break;
    case ErrorCode::ExpectedStringLiteral:
        fmt::format_to(std::back_inserter(out), UNIVERSAL_STR("String expected").GetValue<CharT>());
        break;
    case ErrorCode::ExpectedIdentifier:
        fmt::format_to(std::back_inserter(out), UNIVERSAL_STR("Identifier expected").GetValue<CharT>());
        break;
    case ErrorCode::ExpectedSquareBracket:
        fmt::format_to(std::back_inserter(out), UNIVERSAL_STR("']' expected").GetValue<CharT>());
        break;
    case ErrorCode::ExpectedRoundBracket:
        fmt::format_to(std::back_inserter(out), UNIVERSAL_STR("')' expected").GetValue<CharT>());
        break;
    case ErrorCode::ExpectedCurlyBracket:
        fmt::format_to(std::back_inserter(out), UNIVERSAL_STR("'}}' expected").GetValue<CharT>());
        break;
    case ErrorCode::ExpectedToken:
    {
        auto& extraParams = errInfo.GetExtraParams();
        fmt::format_to(std::back_inserter(out), UNIVERSAL_STR("Unexpected token '{}'").GetValue<CharT>(), extraParams[0]);
        if (extraParams.size() > 1)
        {
            fmt::format_to(std::back_inserter(out), UNIVERSAL_STR(". Expected: ").GetValue<CharT>());
            for (std::size_t i = 1; i < extraParams.size(); ++i)
            {
                if (i != 1)
                {
                    fmt::format_to(std::back_inserter(out), UNIVERSAL_STR(", ").GetValue<CharT>());
                }
                fmt::format_to(std::back_inserter(out), UNIVERSAL_STR("\'{}\'").GetValue<CharT>(), extraParams[i]);
            }
        }
        break;
    }
    case ErrorCode::ExpectedExpression:
    {
        auto& extraParams = errInfo.GetExtraParams();
        fmt::format_to(std::back_inserter(out), UNIVERSAL_STR("Expected expression, got: '{}'").GetValue<CharT>(), extraParams[0]);
        break;
    }
    case ErrorCode::ExpectedEndOfStatement:
    {
        auto& extraParams = errInfo.GetExtraParams();
        fmt::format_to(std::back_inserter(out), UNIVERSAL_STR("Expected end of statement, got: '{}'").GetValue<CharT>(), extraParams[0]);
        break;
    }
    case ErrorCode::ExpectedRawEnd:
        fmt::format_to(std::back_inserter(out), UNIVERSAL_STR("Expected end of raw block").GetValue<CharT>());
        break;
    case ErrorCode::ExpectedMetaEnd:
        fmt::format_to(std::back_inserter(out), UNIVERSAL_STR("Expected end of meta block").GetValue<CharT>());
        break;
    case ErrorCode::UnexpectedToken:
    {
        auto& extraParams = errInfo.GetExtraParams();
        fmt::format_to(std::back_inserter(out), UNIVERSAL_STR("Unexpected token: '{}'").GetValue<CharT>(), extraParams[0]);
        break;
    }
    case ErrorCode::UnexpectedStatement:
    {
        auto& extraParams = errInfo.GetExtraParams();
        fmt::format_to(std::back_inserter(out), UNIVERSAL_STR("Unexpected statement: '{}'").GetValue<CharT>(), extraParams[0]);
        break;
    }
    case ErrorCode::UnexpectedCommentBegin:
        fmt::format_to(std::back_inserter(out), UNIVERSAL_STR("Unexpected comment begin").GetValue<CharT>());
        break;
    case ErrorCode::UnexpectedCommentEnd:
        fmt::format_to(std::back_inserter(out), UNIVERSAL_STR("Unexpected comment end").GetValue<CharT>());
        break;
    case ErrorCode::UnexpectedRawBegin:
        fmt::format_to(std::back_inserter(out), UNIVERSAL_STR("Unexpected raw block begin").GetValue<CharT>());
        break;
    case ErrorCode::UnexpectedRawEnd:
        fmt::format_to(std::back_inserter(out), UNIVERSAL_STR("Unexpected raw block end").GetValue<CharT>());
        break;
    case ErrorCode::UnexpectedMetaBegin:
        fmt::format_to(std::back_inserter(out), UNIVERSAL_STR("Unexpected meta block begin").GetValue<CharT>());
        break;
    case ErrorCode::UnexpectedMetaEnd:
        fmt::format_to(std::back_inserter(out), UNIVERSAL_STR("Unexpected meta block end").GetValue<CharT>());
        break;
    case ErrorCode::UnexpectedExprBegin:
        fmt::format_to(std::back_inserter(out), UNIVERSAL_STR("Unexpected expression block begin").GetValue<CharT>());
        break;
    case ErrorCode::UnexpectedExprEnd:
        fmt::format_to(std::back_inserter(out), UNIVERSAL_STR("Unexpected expression block end").GetValue<CharT>());
        break;
    case ErrorCode::UnexpectedStmtBegin:
        fmt::format_to(std::back_inserter(out), UNIVERSAL_STR("Unexpected statement block begin").GetValue<CharT>());
        break;
    case ErrorCode::UnexpectedStmtEnd:
        fmt::format_to(std::back_inserter(out), UNIVERSAL_STR("Unexpected statement block end").GetValue<CharT>());
        break;
    case ErrorCode::TemplateNotParsed:
        fmt::format_to(std::back_inserter(out), UNIVERSAL_STR("Template not parsed").GetValue<CharT>());
        break;
    case ErrorCode::TemplateNotFound:
        fmt::format_to(std::back_inserter(out), UNIVERSAL_STR("Template(s) not found: {}").GetValue<CharT>(), errInfo.GetExtraParams()[0]);
        break;
    case ErrorCode::InvalidTemplateName:
        fmt::format_to(std::back_inserter(out), UNIVERSAL_STR("Invalid template name: {}").GetValue<CharT>(), errInfo.GetExtraParams()[0]);
        break;
    case ErrorCode::InvalidValueType:
        fmt::format_to(std::back_inserter(out), UNIVERSAL_STR("Invalid value type").GetValue<CharT>());
        break;
    case ErrorCode::ExtensionDisabled:
        fmt::format_to(std::back_inserter(out), UNIVERSAL_STR("Extension disabled").GetValue<CharT>());
        break;
    case ErrorCode::TemplateEnvAbsent:
        fmt::format_to(std::back_inserter(out), UNIVERSAL_STR("Template environment doesn't set").GetValue<CharT>());
        break;
    }
    format_to(std::back_inserter(out), UNIVERSAL_STR("\n{}").GetValue<CharT>(), errInfo.GetLocationDescr());
    result = { out.data(), out.size() };
}

template<>
std::string BasicErrorInfo<char>::ToString() const
{
    std::string result;
    RenderErrorInfo(result, *this);
    return result;
}

template<>
std::wstring BasicErrorInfo<wchar_t>::ToString() const
{
    std::wstring result;
    RenderErrorInfo(result, *this);
    return result;
}

std::ostream& operator<<(std::ostream& os, const ErrorInfo& res)
{
    os << res.ToString();
    return os;
}
std::wostream& operator<<(std::wostream& os, const ErrorInfoW& res)
{
    os << res.ToString();
    return os;
}
} // namespace v2
} // namespace jinja2
