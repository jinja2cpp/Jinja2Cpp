#ifndef JINJA2CPP_ERROR_INFO_H
#define JINJA2CPP_ERROR_INFO_H

#include "value.h"

#include <iostream>
#include <vector>
#include <type_traits>

namespace jinja2
{
enum class ErrorCode
{
    Unspecified = 0,
    UnexpectedException = 1,
    YetUnsupported,
    FileNotFound,
    ExpectedStringLiteral = 1001,
    ExpectedIdentifier,
    ExpectedSquareBracket,
    ExpectedRoundBracket,
    ExpectedCurlyBracket,
    ExpectedToken,
    ExpectedExpression,
    ExpectedEndOfStatement,
    UnexpectedToken,
    UnexpectedStatement,
    UnexpectedCommentBegin,
    UnexpectedCommentEnd,
    UnexpectedExprBegin,
    UnexpectedExprEnd,
    UnexpectedStmtBegin,
    UnexpectedStmtEnd,
    TemplateNotFound,
    TemplateNotParsed,
    InvalidValueType,
    InvalidTemplateName,
    ExtensionDisabled,
    TemplateEnvAbsent,
};

struct SourceLocation
{
    std::string fileName;
    unsigned line = 0;
    unsigned col = 0;
};

template<typename CharT>
class ErrorInfoTpl
{
public:
    struct Data
    {
        ErrorCode code = ErrorCode::Unspecified;
        SourceLocation srcLoc;
        std::vector<SourceLocation> relatedLocs;
        std::vector<Value> extraParams;
        std::basic_string<CharT> locationDescr;
    };

    ErrorInfoTpl() = default;
    ErrorInfoTpl(Data data)
        : m_errorData(std::move(data))
    {}

    ~ErrorInfoTpl() noexcept
    {
        static_assert(std::is_nothrow_move_constructible<ErrorCode>::value, "Should be nothrow-moveable");
        static_assert(std::is_nothrow_move_constructible<SourceLocation>::value, "Should be nothrow-moveable");
        static_assert(std::is_nothrow_move_constructible<std::vector<SourceLocation>>::value, "Should be nothrow-moveable");
        static_assert(std::is_nothrow_move_constructible<Value>::value, "Should be nothrow-moveable");
        static_assert(std::is_nothrow_move_constructible<std::vector<Value>>::value, "Should be nothrow-moveable");
        static_assert(std::is_nothrow_move_constructible<std::basic_string<CharT>>::value, "Should be nothrow-moveable");
        static_assert(std::is_nothrow_move_constructible<Data>::value, "Should be nothrow-moveable");
        static_assert(std::is_nothrow_move_constructible<ErrorInfoTpl<CharT>>::value, "Should be nothrow-moveable");
        static_assert(std::is_nothrow_move_assignable<ErrorInfoTpl<CharT>>::value, "Should be nothrow-moveable");
    }
    ErrorInfoTpl(const ErrorInfoTpl<CharT>&) = default;
    ErrorInfoTpl(ErrorInfoTpl<CharT>&& val) noexcept
        : m_errorData(std::move(val.m_errorData))
    { }


    ErrorInfoTpl& operator =(const ErrorInfoTpl<CharT>&) = default;
    ErrorInfoTpl& operator =(ErrorInfoTpl<CharT>&& val) noexcept
    {
        if (this == &val)
            return *this;

        std::swap(m_errorData.code, val.m_errorData.code);
        std::swap(m_errorData.srcLoc, val.m_errorData.srcLoc);
        std::swap(m_errorData.relatedLocs, val.m_errorData.relatedLocs);
        std::swap(m_errorData.extraParams, val.m_errorData.extraParams);
        std::swap(m_errorData.locationDescr, val.m_errorData.locationDescr);

        return *this;
    }

    ErrorCode GetCode() const
    {
        return m_errorData.code;
    }

    auto& GetErrorLocation() const
    {
        return m_errorData.srcLoc;
    }

    auto& GetRelatedLocations() const
    {
        return m_errorData.relatedLocs;
    }

    const std::basic_string<CharT>& GetLocationDescr() const
    {
        return m_errorData.locationDescr;
    }

    auto& GetExtraParams() const
    {
        return m_errorData.extraParams;
    }

private:
    Data m_errorData;
};

using ErrorInfo = ErrorInfoTpl<char>;
using ErrorInfoW = ErrorInfoTpl<wchar_t>;

std::ostream& operator << (std::ostream& os, const ErrorInfo& res);
std::wostream& operator << (std::wostream& os, const ErrorInfoW& res);
} // jinja2

#endif // JINJA2CPP_ERROR_INFO_H
