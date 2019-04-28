#ifndef JINJA2CPP_ERROR_INFO_H
#define JINJA2CPP_ERROR_INFO_H

#include "value.h"

#include <iostream>
#include <vector>

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
    UnexpectedStmtEnd
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
    ErrorInfoTpl(const ErrorInfoTpl&) = default;
    ErrorInfoTpl(ErrorInfoTpl&&) noexcept = default;


    ErrorInfoTpl& operator =(const ErrorInfoTpl&) = default;
    ErrorInfoTpl& operator =(ErrorInfoTpl&&) noexcept = default;

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
